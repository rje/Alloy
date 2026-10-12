#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QJsonArray>
#include <QMap>
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
QJsonObject dependencyError(const QJsonObject& candidate) {
    const auto features = candidate["features"].toArray();
    QMap<QString,int> positions;
    for (int i = 0; i < features.size(); ++i) positions[features[i].toObject()["id"].toString()] = i;
    for (int i = 0; i < features.size(); ++i) {
        const auto feature = features[i].toObject();
        if (feature["type"] != "extrude") continue;
        const auto parameters = feature["parameters"].toObject();
        const auto source = parameters["sketchId"].toString(), id = feature["id"].toString();
        QJsonObject diagnostic{{"featureId",id},{"featureType","extrude"},{"sketchId",source},{"profileId",parameters["profileId"]}};
        if (source == id) {diagnostic["code"] = "cyclic_dependency";diagnostic["message"] = "An extrusion cannot reference itself";}
        else if (!positions.contains(source)) {diagnostic["code"] = "missing_dependency";diagnostic["message"] = "Extrusion input sketch does not exist";}
        else if (features[positions.value(source)].toObject()["type"] != "sketch") {
            diagnostic["code"] = "unsupported_dependency";diagnostic["message"] = "Extrusion input must reference a sketch feature";
        } else if (positions.value(source) >= i) {
            diagnostic["code"] = "forward_dependency";diagnostic["message"] = "Extrusion input sketch must precede it in feature history";
        } else continue;
        return diagnostic;
    }
    // Supported dependencies only point from an extrusion to an earlier sketch;
    // sketches have no feature inputs, so accepted histories cannot form cycles.
    return {};
}
QJsonObject solveSketches(QJsonObject candidate) {
    auto features = candidate["features"].toArray();
    for (int i = 0; i < features.size(); ++i) {
        const auto feature = features[i].toObject();
        if (feature["type"].toString() != "sketch") continue;
        const auto solution = SketchGeometry::solveDraft(feature);
        if (!solution.ok) throw GeometryKernel::EvaluationError({{"code","sketch_solve_failed"},
            {"featureId",feature["id"]},{"featureType","sketch"},{"message",solution.error},{"solverDiagnostics",solution.diagnostics}});
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
    lastEvaluationError_ = {};
    apply({{"schema", "alloy-document/1"}, {"documentId", newId()},
        {"displayUnits", "mm"}, {"features", QJsonArray{}}},std::make_shared<const EvaluatedDocument>());
    history_.setClean();
}
void CadDocument::apply(const QJsonObject& value, std::shared_ptr<const EvaluatedDocument> evaluation, const QString& transaction) {
    authored_ = value; evaluation_=std::move(evaluation); lastTransaction_ = transaction; ++revision_;
}
QString CadDocument::commit(const QJsonObject& candidate, const QString& label) {
    lastEvaluationError_ = {};
    if (candidate["documentId"] != authored_["documentId"]) {
        const QString error = "An edit cannot change document identity";
        lastEvaluationError_ = {{"code","invalid_document"},{"message",error}}; return error;
    }
    if (candidate == authored_) return {};
    auto prepared = evaluateCandidate(candidate);
    if (!prepared.ok) {lastEvaluationError_ = prepared.diagnostics;return prepared.error;}
    if (prepared.authored == authored_) return {};
    history_.push(new DocumentEdit(*this,prepared.authored,std::move(prepared.evaluation),label));
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
    lastEvaluationError_ = {};
    QFile file(path);
    if (!QDir::isAbsolutePath(path) || !file.open(QIODevice::ReadOnly)) return "Cannot read native document";
    QJsonParseError parse; const auto data=QJsonDocument::fromJson(file.readAll(),&parse);
    if (parse.error!=QJsonParseError::NoError || !data.isObject()) return "Malformed native document";
    auto prepared = evaluateCandidate(data.object());
    if (!prepared.ok) {lastEvaluationError_ = prepared.diagnostics;return prepared.error;}
    history_.clear(); apply(prepared.authored,std::move(prepared.evaluation)); path_=path; history_.setClean();
    return {};
}
CadDocument::CandidateEvaluation CadDocument::evaluateCandidate(const QJsonObject& candidate) {
    CandidateEvaluation result;
    result.authored = candidate;
    QJsonObject structuralContext;
    const auto structural = validateStructure(candidate,&structuralContext);
    if (!structural.isEmpty()) {
        result.error = structural; result.diagnostics = structuralContext;
        result.diagnostics["code"] = "invalid_document";result.diagnostics["message"] = structural;return result;
    }
    const auto dependency = dependencyError(candidate);
    if (!dependency.isEmpty()) {result.diagnostics = dependency;result.error = dependency["message"].toString();return result;}
    try {
        const auto solved = solveSketches(candidate);
        auto evaluated = std::make_shared<const EvaluatedDocument>(GeometryKernel::evaluateDocument(solved));
        result.authored = solved; result.evaluation = std::move(evaluated); result.ok = true;
        result.diagnostics = {{"code","evaluated"},{"recompute","full_ordered"},{"featureCount",solved["features"].toArray().size()}};
    } catch (const GeometryKernel::EvaluationError& error) {result.diagnostics = error.diagnostics();result.error = QString::fromUtf8(error.what());}
      catch (const Standard_Failure& error) {
        result.error = "Kernel evaluation failed: "+QString::fromUtf8(error.GetMessageString());
        result.diagnostics = {{"code","kernel_failure"},{"message",result.error}};
    } catch (const std::exception& error) {
        result.error = "Evaluation failed: "+QString::fromUtf8(error.what());
        result.diagnostics = {{"code","evaluation_failed"},{"message",result.error}};
    }
    return result;
}
QJsonObject CadDocument::extrusionCandidate(const QJsonObject& authored, const QJsonObject& arguments) {
    auto reject = [](const QString& message) {throw std::invalid_argument(message.toStdString());};
    const auto initial = validate(authored);
    if (!initial.isEmpty()) reject(initial);
    const QSet<QString> allowed{"sketchId","profileId","distanceMm","name","featureId","newFeatureId","newBodyId"};
    for (auto it = arguments.begin(); it != arguments.end(); ++it)
        if (!allowed.contains(it.key())) reject("Unknown extrusion argument: "+it.key());
    const bool edit = arguments.contains("featureId"), newFeature = arguments.contains("newFeatureId"), newBody = arguments.contains("newBodyId");
    if (newFeature != newBody || (edit && newFeature)) reject("Choose an existing extrusion featureId or a newFeatureId/newBodyId pair");
    auto candidate = authored; auto features = authored["features"].toArray();
    QJsonObject feature; int index = -1;
    if (edit) {
        for (int i = 0; i < features.size(); ++i) if (features[i].toObject()["id"] == arguments["featureId"]) {
            feature = features[i].toObject();index = i;break;
        }
        if (index < 0 || feature["type"] != "extrude") reject("Extrusion feature identity/type is no longer available");
    } else feature = {{"id",newFeature ? arguments["newFeatureId"] : QJsonValue(newId())},
        {"bodyId",newBody ? arguments["newBodyId"] : QJsonValue(newId())},{"ownerId",authored["documentId"]},
        {"type","extrude"},{"name","Extrude"},{"visible",true}};
    if (arguments.contains("name")) feature["name"] = arguments["name"];
    feature["parameters"] = QJsonObject{{"sketchId",arguments["sketchId"]},{"profileId",arguments["profileId"]},{"distanceMm",arguments["distanceMm"]}};
    if (index >= 0) features[index] = feature; else features.append(feature);
    candidate["features"] = features;
    // Resolve references in evaluateCandidate so failures retain feature/input context.
    const auto error = validateStructure(candidate);
    if (!error.isEmpty()) reject(error);
    return candidate;
}
double CadDocument::millimeters(double value, const QString& units) {
    if ((units != "mm" && units != "in") || !std::isfinite(value))
        throw std::invalid_argument("Expected finite length with mm or in units");
    const double converted = value * (units == "in" ? 25.4 : 1.0);
    if (!std::isfinite(converted)) throw std::invalid_argument("Length conversion overflow");
    return converted;
}
QString CadDocument::validate(const QJsonObject& candidate) {
    const auto structural = validateStructure(candidate);
    return structural.isEmpty() ? dependencyError(candidate)["message"].toString() : structural;
}
QString CadDocument::validateStructure(const QJsonObject& candidate, QJsonObject* errorContext) {
    if (errorContext) *errorContext = {};
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
        if (errorContext) *errorContext = {{"featureId",feature["id"]},{"featureType",feature["type"]}};
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
        if (type != "box" && type != "cylinder" && type != "extrude") return "Unsupported feature type";
        const auto bodyId = feature["bodyId"].toString();
        if (!validId(bodyId) || ids.contains(bodyId)) return "Invalid or duplicate authored UUID";
        ids.insert(bodyId);
        if (type == "extrude") {
            if (errorContext) {(*errorContext)["sketchId"] = p["sketchId"];(*errorContext)["profileId"] = p["profileId"];}
            if (p.size() != 3 || !p["sketchId"].isString() || !validId(p["sketchId"].toString())
                || !p["profileId"].isString() || !validId(p["profileId"].toString())) return "Extrusion requires stable sketchId and profileId inputs";
            if (!p["distanceMm"].isDouble() || !std::isfinite(p["distanceMm"].toDouble())
                || std::abs(p["distanceMm"].toDouble()) < 0.001 || std::abs(p["distanceMm"].toDouble()) > 100000)
                return "Extrusion distance must be signed 0.001..100000 mm; zero is unsupported";
            continue;
        }
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
    if (errorContext) *errorContext = {};
    return {};
}
QJsonObject CadDocument::state() const {
    QJsonArray order;
    for (const auto& id : evaluation_->featureOrder) order.append(id);
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
        {"evaluation",QJsonObject{{"status","current"},{"recompute","full_ordered"},{"featureOrder",order},
            {"documentRevision",static_cast<qint64>(revision_)},{"bodies",evaluated},{"sketches",evaluatedSketches}}},
        {"editableHistory", true}, {"authored", authored_},
        {"undo", QJsonObject{{"canUndo", history_.canUndo()}, {"canRedo", history_.canRedo()},
            {"count", history_.count()}, {"index", history_.index()},
            {"undoText", history_.undoText()}, {"redoText", history_.redoText()}}}};
}
