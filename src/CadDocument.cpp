#include "CadDocument.h"
#include <QJsonArray>
#include <QSet>
#include <QUuid>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QSaveFile>
#include <Standard_Failure.hxx>
#include <cmath>
#include <stdexcept>

class DocumentEdit final : public QUndoCommand {
public:
    DocumentEdit(CadDocument& document, QJsonObject candidate,
        std::shared_ptr<const std::vector<EvaluatedBody>> evaluated, const QString& label)
        : document_(document), before_(document.authored()), after_(std::move(candidate)),
          beforeBodies_(document.bodies_), afterBodies_(std::move(evaluated)) { setText(label); }
    void undo() override { document_.apply(before_, beforeBodies_, id_); }
    void redo() override { document_.apply(after_, afterBodies_, id_); }
private:
    CadDocument& document_;
    QJsonObject before_, after_;
    std::shared_ptr<const std::vector<EvaluatedBody>> beforeBodies_, afterBodies_;
    QString id_ = CadDocument::newId();
};

QString CadDocument::newId() { return QUuid::createUuid().toString(QUuid::WithoutBraces); }
CadDocument::CadDocument() { reset(); }
void CadDocument::reset() {
    history_.clear();
    path_.clear();
    apply({{"schema", "alloy-document/1"}, {"documentId", newId()},
        {"displayUnits", "mm"}, {"features", QJsonArray{}}},std::make_shared<const std::vector<EvaluatedBody>>());
    history_.setClean();
}
void CadDocument::apply(const QJsonObject& value, std::shared_ptr<const std::vector<EvaluatedBody>> bodies, const QString& transaction) {
    authored_ = value; bodies_=std::move(bodies); lastTransaction_ = transaction; ++revision_;
}
QString CadDocument::commit(const QJsonObject& candidate, const QString& label) {
    const auto error = validate(candidate);
    if (!error.isEmpty()) return error;
    if (candidate["documentId"] != authored_["documentId"]) return "An edit cannot change document identity";
    if (candidate != authored_) {
        try {
            auto evaluated=std::make_shared<const std::vector<EvaluatedBody>>(GeometryKernel::evaluate(candidate));
            history_.push(new DocumentEdit(*this,candidate,std::move(evaluated),label));
        } catch(const Standard_Failure& error) { return "Kernel evaluation failed: "+QString::fromUtf8(error.GetMessageString()); }
          catch(const std::exception& error) { return "Evaluation failed: "+QString::fromUtf8(error.what()); }
    }
    return {};
}
QString CadDocument::save(const QString& path) {
    if (!QDir::isAbsolutePath(path) || !path.endsWith(".alloy",Qt::CaseInsensitive)) return "Expected an absolute .alloy path";
    QSaveFile file(path); file.setDirectWriteFallback(false);
    if (!file.open(QIODevice::WriteOnly)) return file.errorString();
    const auto bytes=QJsonDocument(authored_).toJson();
    if (file.write(bytes)!=bytes.size() || !file.commit()) return file.errorString();
    path_=path; history_.setClean(); return {};
}
QString CadDocument::open(const QString& path) {
    QFile file(path);
    if (!QDir::isAbsolutePath(path) || !file.open(QIODevice::ReadOnly)) return "Cannot read native document";
    QJsonParseError parse; const auto data=QJsonDocument::fromJson(file.readAll(),&parse);
    if (parse.error!=QJsonParseError::NoError || !data.isObject()) return "Malformed native document";
    const auto candidate=data.object(); const auto error=validate(candidate);
    if (!error.isEmpty()) return error;
    try {
        auto evaluated=std::make_shared<const std::vector<EvaluatedBody>>(GeometryKernel::evaluate(candidate));
        history_.clear(); apply(candidate,std::move(evaluated)); path_=path; history_.setClean();
    } catch(const Standard_Failure& error) { return "Kernel evaluation failed: "+QString::fromUtf8(error.GetMessageString()); }
      catch(const std::exception& error) { return "Evaluation failed: "+QString::fromUtf8(error.what()); }
    return {};
}
double CadDocument::millimeters(double value, const QString& units) {
    if ((units != "mm" && units != "in") || !std::isfinite(value))
        throw std::invalid_argument("Expected finite length with mm or in units");
    const double converted = value * (units == "in" ? 25.4 : 1.0);
    if (!std::isfinite(converted)) throw std::invalid_argument("Length conversion overflow");
    return converted;
}
QString CadDocument::validate(const QJsonObject& candidate) {
    if (candidate["schema"].toString() != "alloy-document/1") return "Unsupported document schema";
    const auto root = candidate["documentId"].toString();
    auto validId = [](const QString& id) { return !QUuid(id).isNull() && QUuid(id).toString(QUuid::WithoutBraces) == id; };
    if (!validId(root)) return "Invalid document UUID";
    const auto units = candidate["displayUnits"].toString();
    if (units != "mm" && units != "in") return "Unsupported display units";
    if (!candidate["features"].isArray()) return "Features must be an array";
    QSet<QString> ids{root};
    for (const auto& item : candidate["features"].toArray()) {
        if (!item.isObject()) return "Feature must be an object";
        const auto feature = item.toObject();
        for (const auto& field : {"id", "bodyId"}) {
            const auto id = feature[field].toString();
            if (!validId(id) || ids.contains(id)) return "Invalid or duplicate authored UUID";
            ids.insert(id);
        }
        if (feature["ownerId"].toString() != root) return "Feature must belong to the document root";
        if (!feature["name"].isString() || feature["name"].toString().trimmed().isEmpty()
            || !feature["visible"].isBool()) return "Feature name and visibility are required";
        const auto type = feature["type"].toString();
        if (type != "box" && type != "cylinder") return "Unsupported feature type";
        if (!feature["parameters"].isObject()) return "Feature parameters must be an object";
        const auto p = feature["parameters"].toObject();
        const QStringList dimensions = type == "box" ? QStringList{"widthMm", "depthMm", "heightMm"}
                                                     : QStringList{"radiusMm", "heightMm"};
        for (const auto& field : dimensions) {
            if (!p[field].isDouble() || !std::isfinite(p[field].toDouble())
                || p[field].toDouble() < 0.001 || p[field].toDouble() > 100000)
                return "Dimension out of supported range (0.001..100000 mm): " + field;
        }
        for (const auto& field : {"xMm", "yMm", "zMm"}) {
            if (!p[field].isDouble() || !std::isfinite(p[field].toDouble()) || std::abs(p[field].toDouble()) > 100000)
                return "Placement out of supported range: " + QString(field);
        }
        if (p.size() != dimensions.size() + 3) return "Unknown feature parameter";
    }
    return {};
}
QJsonObject CadDocument::state() const {
    QJsonArray evaluated;
    for(const auto& body:*bodies_) {
        auto summary=body.summary; summary["featureId"]=body.featureId; summary["bodyId"]=body.bodyId;
        summary["name"]=body.name; summary["visible"]=body.visible; evaluated.append(summary);
    }
    return {{"loaded", true}, {"id", authored_["documentId"]}, {"label", path_.isEmpty() ? "Untitled" : QFileInfo(path_).completeBaseName()}, {"path",path_},
        {"units", authored_["displayUnits"]}, {"canonicalLengthUnit", "mm"},
        {"revision", static_cast<qint64>(revision_)}, {"dirty", !history_.isClean()},
        {"lastTransactionId", lastTransaction_},
        {"features", authored_["features"].toArray().size()}, {"bodies",static_cast<int>(bodies_->size())}, {"sketches", 0},
        {"evaluation",QJsonObject{{"status","current"},{"documentRevision",static_cast<qint64>(revision_)},{"bodies",evaluated}}},
        {"editableHistory", true}, {"authored", authored_},
        {"undo", QJsonObject{{"canUndo", history_.canUndo()}, {"canRedo", history_.canRedo()},
            {"count", history_.count()}, {"index", history_.index()},
            {"undoText", history_.undoText()}, {"redoText", history_.redoText()}}}};
}
