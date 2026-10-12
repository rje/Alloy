#include "CadDocument.h"
#include "SketchGeometry.h"
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

namespace {
QJsonObject solveSketches(QJsonObject candidate) {
    auto features = candidate["features"].toArray();
    for (int i = 0; i < features.size(); ++i) {
        const auto feature = features[i].toObject();
        if (feature["type"].toString() != "sketch") continue;
        const auto solution = SketchGeometry::solveDraft(feature);
        if (!solution.ok) throw std::invalid_argument(solution.error.toStdString());
        features[i] = solution.feature;
    }
    candidate["features"] = features;
    const auto error = CadDocument::validate(candidate);
    if (!error.isEmpty()) throw std::invalid_argument(error.toStdString());
    return candidate;
}
}

class DocumentEdit final : public QUndoCommand {
public:
    DocumentEdit(CadDocument& document, QJsonObject candidate,
        std::shared_ptr<const EvaluatedDocument> evaluated, const QString& label)
        : document_(document), before_(document.authored()), after_(std::move(candidate)),
          beforeEvaluation_(document.evaluation_), afterEvaluation_(std::move(evaluated)) { setText(label); }
    void undo() override { document_.apply(before_, beforeEvaluation_, id_); }
    void redo() override { document_.apply(after_, afterEvaluation_, id_); }
private:
    CadDocument& document_;
    QJsonObject before_, after_;
    std::shared_ptr<const EvaluatedDocument> beforeEvaluation_, afterEvaluation_;
    QString id_ = CadDocument::newId();
};

QString CadDocument::newId() { return QUuid::createUuid().toString(QUuid::WithoutBraces); }
CadDocument::CadDocument() { reset(); }
void CadDocument::reset() {
    history_.clear();
    path_.clear();
    apply({{"schema", "alloy-document/1"}, {"documentId", newId()},
        {"displayUnits", "mm"}, {"features", QJsonArray{}}},std::make_shared<const EvaluatedDocument>());
    history_.setClean();
}
void CadDocument::apply(const QJsonObject& value, std::shared_ptr<const EvaluatedDocument> evaluation, const QString& transaction) {
    authored_ = value; evaluation_=std::move(evaluation); lastTransaction_ = transaction; ++revision_;
}
QString CadDocument::commit(const QJsonObject& candidate, const QString& label) {
    const auto error = validate(candidate);
    if (!error.isEmpty()) return error;
    if (candidate["documentId"] != authored_["documentId"]) return "An edit cannot change document identity";
    if (candidate != authored_) {
        try {
            const auto solved = solveSketches(candidate);
            if (solved == authored_) return {};
            auto evaluated=std::make_shared<const EvaluatedDocument>(GeometryKernel::evaluateDocument(solved));
            history_.push(new DocumentEdit(*this,solved,std::move(evaluated),label));
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
        const auto solved = solveSketches(candidate);
        auto evaluated=std::make_shared<const EvaluatedDocument>(GeometryKernel::evaluateDocument(solved));
        history_.clear(); apply(solved,std::move(evaluated)); path_=path; history_.setClean();
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
        const auto id = feature["id"].toString();
        if (!validId(id) || ids.contains(id)) return "Invalid or duplicate authored UUID";
        ids.insert(id);
        if (feature["ownerId"].toString() != root) return "Feature must belong to the document root";
        if (!feature["name"].isString() || feature["name"].toString().trimmed().isEmpty()
            || !feature["visible"].isBool()) return "Feature name and visibility are required";
        const auto type = feature["type"].toString();
        if (!feature["parameters"].isObject()) return "Feature parameters must be an object";
        const auto p = feature["parameters"].toObject();
        if (type == "sketch") {
            if (feature.contains("bodyId")) return "Sketches do not own solid body IDs";
            const auto error = SketchGeometry::validateParameters(p, ids);
            if (!error.isEmpty()) return error;
            continue;
        }
        if (type != "box" && type != "cylinder") return "Unsupported feature type";
        const auto bodyId = feature["bodyId"].toString();
        if (!validId(bodyId) || ids.contains(bodyId)) return "Invalid or duplicate authored UUID";
        ids.insert(bodyId);
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
    for(const auto& body:evaluation_->bodies) {
        auto summary=body.summary; summary["featureId"]=body.featureId; summary["bodyId"]=body.bodyId;
        summary["name"]=body.name; summary["visible"]=body.visible; evaluated.append(summary);
    }
    QJsonArray evaluatedSketches;
    for (const auto& sketch : evaluation_->sketches) {
        auto summary = sketch.summary;
        summary["featureId"] = sketch.featureId; summary["sketchId"] = sketch.featureId;
        summary["name"] = sketch.name; summary["visible"] = sketch.visible;
        evaluatedSketches.append(summary);
    }
    return {{"loaded", true}, {"id", authored_["documentId"]}, {"label", path_.isEmpty() ? "Untitled" : QFileInfo(path_).completeBaseName()}, {"path",path_},
        {"units", authored_["displayUnits"]}, {"canonicalLengthUnit", "mm"},
        {"revision", static_cast<qint64>(revision_)}, {"dirty", !history_.isClean()},
        {"lastTransactionId", lastTransaction_},
        {"features", authored_["features"].toArray().size()}, {"bodies",static_cast<int>(evaluation_->bodies.size())}, {"sketches", static_cast<int>(evaluation_->sketches.size())},
        {"evaluation",QJsonObject{{"status","current"},{"documentRevision",static_cast<qint64>(revision_)},{"bodies",evaluated},{"sketches",evaluatedSketches}}},
        {"editableHistory", true}, {"authored", authored_},
        {"undo", QJsonObject{{"canUndo", history_.canUndo()}, {"canRedo", history_.canRedo()},
            {"count", history_.count()}, {"index", history_.index()},
            {"undoText", history_.undoText()}, {"redoText", history_.redoText()}}}};
}
