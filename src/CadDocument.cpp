#include "CadDocument.h"
#include "SketchGeometry.h"
#include "StepExchange.h"
#include <QJsonArray>
#include <QMap>
#include <QSet>
#include <QUuid>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QSaveFile>
#include <QCryptographicHash>
#include <QRegularExpression>
#include <Standard_Failure.hxx>
#include <cmath>
#include <stdexcept>

namespace {
bool validId(const QString& id) { return !QUuid(id).isNull() && QUuid(id).toString(QUuid::WithoutBraces) == id; }
QJsonObject dependencyError(const QJsonObject& candidate) {
    const auto features = candidate["features"].toArray();
    QMap<QString,int> positions;
    QMap<QString,int> bodyPositions;
    for (int i = 0; i < features.size(); ++i) positions[features[i].toObject()["id"].toString()] = i;
    for (int i = 0; i < features.size(); ++i) {
        const auto feature = features[i].toObject();
        if (feature.contains("bodyId")) bodyPositions[feature["bodyId"].toString()] = i;
    }
    QSet<QString> activeBodies;
    for (int i = 0; i < features.size(); ++i) {
        const auto feature = features[i].toObject();
        if (feature.contains("bodyId")) activeBodies.insert(feature["bodyId"].toString());
        if (feature["type"] == "boolean") {
            const auto parameters = feature["parameters"].toObject();
            const auto target = parameters["targetBodyId"].toString();
            const auto tools = parameters["toolBodyIds"].toArray();
            auto inputs = tools;inputs.prepend(target);
            for (const auto& input : inputs) {
                const auto bodyId = input.toString();
                QJsonObject diagnostic{{"featureId",feature["id"]},{"featureType","boolean"},
                    {"targetBodyId",target},{"toolBodyIds",tools},{"bodyId",bodyId}};
                if (bodyId == feature["id"].toString()) {
                    diagnostic["code"] = "cyclic_dependency";diagnostic["message"] = "A Boolean cannot reference itself";
                } else if (!bodyPositions.contains(bodyId)) {
                    diagnostic["code"] = "missing_dependency";diagnostic["message"] = "Boolean input body does not exist";
                } else if (bodyPositions.value(bodyId) >= i) {
                    diagnostic["code"] = "forward_dependency";diagnostic["message"] = "Boolean input bodies must precede it in feature history";
                } else if (!activeBodies.contains(bodyId)) {
                    diagnostic["code"] = "consumed_dependency";diagnostic["message"] = "Boolean input body was consumed by an earlier feature; enable Keep Tools there or select an active body";
                } else continue;
                return diagnostic;
            }
            if (!parameters["keepTools"].toBool())
                for (const auto& tool : tools) activeBodies.remove(tool.toString());
            continue;
        }
        if(feature["type"]=="sketch" && feature["parameters"].toObject()["plane"]=="face") {
            const auto support=feature["parameters"].toObject()["support"].toObject();const auto producer=support["producerFeatureId"].toString(),body=support["bodyId"].toString();
            QJsonObject diagnostic{{"featureId",feature["id"]},{"featureType","sketch"},{"support",support}};
            if(!positions.contains(producer)){diagnostic["code"]="missing_dependency";diagnostic["message"]="Sketch support producer does not exist";}
            else if(positions[producer]>=i){diagnostic["code"]="forward_dependency";diagnostic["message"]="Sketch support producer must precede its sketch";}
            else if(features[positions[producer]].toObject()["type"]!="extrude" || features[positions[producer]].toObject()["bodyId"]!=support["bodyId"]){diagnostic["code"]="unsupported_dependency";diagnostic["message"]="Sketch support requires its exact extrusion producer/body";}
            else if(!activeBodies.contains(body)){diagnostic["code"]="consumed_dependency";diagnostic["message"]="Sketch support body was consumed earlier in history";}
            else continue;
            return diagnostic;
        }
        if(feature["type"]=="extrude" && feature["parameters"].toObject()["operation"]=="cut") {
            const auto target=feature["parameters"].toObject()["targetBodyIds"].toArray()[0].toString();
            QJsonObject diagnostic{{"featureId",feature["id"]},{"featureType","extrude"},{"targetBodyId",target}};
            if(!bodyPositions.contains(target)){diagnostic["code"]="missing_dependency";diagnostic["message"]="Cut target body does not exist";return diagnostic;}
            if(bodyPositions.value(target)>=i){diagnostic["code"]="forward_dependency";diagnostic["message"]="Cut target must precede its modifier";return diagnostic;}
            if(!activeBodies.contains(target)){diagnostic["code"]="consumed_dependency";diagnostic["message"]="Cut target was consumed by an earlier feature";return diagnostic;}
        }
        if (feature["type"] != "extrude") continue;
        const auto parameters = feature["parameters"].toObject();
        const auto source = parameters["sketchId"].toString(), id = feature["id"].toString();
        QJsonObject diagnostic{{"featureId",id},{"featureType","extrude"},{"sketchId",source}};
        if(parameters.contains("profileIds"))diagnostic["profileIds"]=parameters["profileIds"];
        else diagnostic["profileId"]=parameters["profileId"];
        if (source == id) {diagnostic["code"] = "cyclic_dependency";diagnostic["message"] = "An extrusion cannot reference itself";}
        else if (!positions.contains(source)) {diagnostic["code"] = "missing_dependency";diagnostic["message"] = "Extrusion input sketch does not exist";}
        else if (features[positions.value(source)].toObject()["type"] != "sketch") {
            diagnostic["code"] = "unsupported_dependency";diagnostic["message"] = "Extrusion input must reference a sketch feature";
        } else if (positions.value(source) >= i) {
            diagnostic["code"] = "forward_dependency";diagnostic["message"] = "Extrusion input sketch must precede it in feature history";
        } else continue;
        return diagnostic;
    }
    // Every accepted feature input precedes its consumer; accepted histories
    // cannot form cycles and consumed bodies cannot be used as stale inputs.
    return {};
}
QJsonObject solveSketches(QJsonObject candidate) {
    auto features = candidate["features"].toArray();
    for (int i = 0; i < features.size(); ++i) {
        const auto feature = features[i].toObject();
        if (feature["type"].toString() != "sketch") continue;
        QJsonObject projectionContext;
        if(feature["parameters"].toObject()["plane"]=="face") {
            auto prior=candidate;QJsonArray prefix;for(int j=0;j<i;++j)prefix.append(features[j]);prior["features"]=prefix;
            projectionContext=GeometryKernel::projectionContext(GeometryKernel::evaluateDocument(prior),feature);
        }
        const auto solution = SketchGeometry::solveDraft(feature,projectionContext);
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
EvaluatedDocument CadDocument::evaluationBefore(const QString& featureId) const {
    auto prefix=authored_;QJsonArray features;bool found=false;
    for(const auto& value:authored_["features"].toArray()){if(value.toObject()["id"]==featureId){found=true;break;}features.append(value);}
    if(!found)throw std::invalid_argument("History input feature is unavailable");prefix["features"]=features;
    return GeometryKernel::evaluateDocument(prefix);
}
QJsonObject CadDocument::stepImportCandidate(const QJsonObject& authored, const QJsonObject& arguments) {
    if (!validate(authored).isEmpty())
        throw StepExchange::Error({{"code","invalid_document"},{"message","Cannot import into an invalid document"}});
    const QSet<QString> allowed{"path", "name", "newFeatureId", "newBodyId"};
    for (auto it=arguments.begin();it!=arguments.end();++it)
        if (!allowed.contains(it.key()))
            throw StepExchange::Error({{"code","invalid_arguments"},{"message","Unknown STEP import argument"},{"field",it.key()}});
    for (const auto& field : {"path","name","newFeatureId","newBodyId"})
        if (arguments.contains(field) && !arguments[field].isString())
            throw StepExchange::Error({{"code","invalid_arguments"},{"message","STEP import argument must be a string"},{"field",field}});
    const auto path=arguments.value("path").toString();
    const QFileInfo info(path);
    if (!QDir::isAbsolutePath(path) || !info.isFile() ||
        !QStringList{"step","stp","ste"}.contains(info.suffix().toLower()))
        throw StepExchange::Error({{"code","step_source_path"},{"message","Expected a readable absolute STEP file (.step, .stp or .ste)"}});
    if (info.size()<1 || info.size()>StepExchange::maximumSourceBytes)
        throw StepExchange::Error({{"code","step_source_size"},{"message","STEP source must contain 1 byte to 32 MiB of data"}});
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        throw StepExchange::Error({{"code","step_source_read"},{"message","Cannot read STEP source"}});
    const auto bytes=file.read(StepExchange::maximumSourceBytes+1);
    if (bytes.isEmpty() || bytes.size()>StepExchange::maximumSourceBytes || !file.atEnd())
        throw StepExchange::Error({{"code","step_source_size"},{"message","STEP source must contain 1 byte to 32 MiB of data"}});

    const auto featureId=arguments.value("newFeatureId").toString(newId());
    const auto bodyId=arguments.value("newBodyId").toString(newId());
    const auto name=arguments.value("name").toString(info.completeBaseName()).trimmed();
    if (!validId(featureId) || !validId(bodyId) || featureId==bodyId || name.isEmpty())
        throw StepExchange::Error({{"code","invalid_arguments"},{"message","STEP import needs distinct UUIDs and a nonempty name"}});
    const QJsonObject source{{"policy","embedded"},{"format","step"},
        {"bytesBase64",QString::fromLatin1(bytes.toBase64())},
        {"sha256",QString::fromLatin1(QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex())},
        {"byteCount",static_cast<qint64>(bytes.size())},{"originalName",info.fileName()}};
    auto candidate=authored;
    auto features=candidate.value("features").toArray();
    features.append(QJsonObject{{"id",featureId},{"bodyId",bodyId},
        {"ownerId",candidate.value("documentId")},{"type","stepImport"},
        {"name",name},{"visible",true},{"parameters",QJsonObject{{"source",source}}}});
    candidate["features"]=features;
    return candidate;
}
QJsonObject CadDocument::extrusionCandidate(const QJsonObject& authored, const QJsonObject& arguments) {
    auto reject = [](const QString& message) {throw std::invalid_argument(message.toStdString());};
    const auto initial = validate(authored);
    if (!initial.isEmpty()) reject(initial);
    const QSet<QString> allowed{"sketchId","profileId","profileIds","distanceMm","name","featureId","newFeatureId","newBodyId","operation","targetBodyIds"};
    for (auto it = arguments.begin(); it != arguments.end(); ++it)
        if (!allowed.contains(it.key())) reject("Unknown extrusion argument: "+it.key());
    const bool edit = arguments.contains("featureId"), newFeature = arguments.contains("newFeatureId"), newBody = arguments.contains("newBodyId");
    if(edit && (newFeature || newBody))reject("Existing extrusion edits cannot reserve new feature/body IDs");
    auto candidate = authored; auto features = authored["features"].toArray();
    QJsonObject feature; int index = -1;
    if (edit) {
        for (int i = 0; i < features.size(); ++i) if (features[i].toObject()["id"] == arguments["featureId"]) {
            feature = features[i].toObject();index = i;break;
        }
        if (index < 0 || feature["type"] != "extrude") reject("Extrusion feature identity/type is no longer available");
    }
    if(arguments.contains("operation") && !arguments["operation"].isString())reject("Extrusion operation must be a string");
    const auto previousParameters=feature["parameters"].toObject();
    const auto operation=arguments.value("operation").toString(previousParameters.value("operation").toString("newBody"));
    if(operation!="newBody" && operation!="cut")reject("This extrusion slice supports New Body or Cut only");
    const bool cut=operation=="cut";
    if(edit && cut!=(previousParameters["operation"]=="cut"))reject("Changing an existing extrusion between New Body and Cut requires explicit body identity migration");
    if(cut && newBody)reject("Cut modifies retained targets and cannot own a new body ID");
    if(!cut && !edit && newFeature!=newBody)reject("New Body reserves newFeatureId/newBodyId together");
    if(!cut && arguments.contains("targetBodyIds"))reject("New Body cannot carry Cut target references");
    if(!edit) {
        feature={{"id",newFeature ? arguments["newFeatureId"] : QJsonValue(newId())},{"ownerId",authored["documentId"]},{"type","extrude"},{"name",cut ? "Extrude Cut" : "Extrude"},{"visible",true}};
        if(!cut)feature["bodyId"]=newBody ? arguments["newBodyId"] : QJsonValue(newId());
    }
    if (arguments.contains("name")) feature["name"] = arguments["name"];
    if(arguments.contains("profileId")==arguments.contains("profileIds"))reject("Choose profileId or profileIds exclusively");
    auto profileParameters=QJsonObject{{"sketchId",arguments["sketchId"]},{"distanceMm",arguments["distanceMm"]}};
    if(cut) {
        const auto targets=arguments.contains("targetBodyIds") ? arguments["targetBodyIds"] : previousParameters["targetBodyIds"];
        if(!targets.isArray() || targets.toArray().size()!=1 || !targets.toArray()[0].isString() || !validId(targets.toArray()[0].toString()))reject("Cut requires exactly one explicit stable targetBodyIds entry");
        profileParameters["operation"]="cut";profileParameters["targetBodyIds"]=targets;
    }
    if(arguments.contains("profileIds")) {
        if(!arguments["profileIds"].isArray() || arguments["profileIds"].toArray().isEmpty())reject("Choose a nonempty profileIds array");
        QStringList ids;for(const auto& id:arguments["profileIds"].toArray()){
            if(!id.isString() || !validId(id.toString()) || ids.contains(id.toString()))reject("Profile IDs must be distinct UUIDs");ids.append(id.toString());}
        ids.sort();QJsonArray references;for(const auto& id:ids)references.append(id);profileParameters["profileIds"]=references;
    } else profileParameters["profileId"]=arguments["profileId"];
    feature["parameters"] = profileParameters;
    if (index >= 0) features[index] = feature; else features.append(feature);
    candidate["features"] = features;
    // Resolve references in evaluateCandidate so failures retain feature/input context.
    const auto error = validateStructure(candidate);
    if (!error.isEmpty()) reject(error);
    return candidate;
}
QJsonObject CadDocument::booleanCandidate(const QJsonObject& authored, const QJsonObject& arguments) {
    auto reject = [](const QString& message) {throw std::invalid_argument(message.toStdString());};
    const auto initial = validate(authored);
    if (!initial.isEmpty()) reject(initial);
    const QSet<QString> allowed{"operation","targetBodyId","toolBodyIds","keepTools","name","featureId","newFeatureId"};
    for (auto it = arguments.begin(); it != arguments.end(); ++it)
        if (!allowed.contains(it.key())) reject("Unknown Boolean argument: "+it.key());
    if (arguments.contains("featureId") && arguments.contains("newFeatureId"))
        reject("Choose an existing Boolean featureId or a newFeatureId");
    auto candidate = authored;auto features = authored["features"].toArray();
    QJsonObject feature;int index = -1;
    if (arguments.contains("featureId")) {
        for (int i = 0; i < features.size(); ++i) if (features[i].toObject()["id"] == arguments["featureId"]) {
            feature = features[i].toObject();index = i;break;
        }
        if (index < 0 || feature["type"] != "boolean") reject("Boolean feature identity/type is no longer available");
    } else feature = {{"id",arguments.contains("newFeatureId") ? arguments["newFeatureId"] : QJsonValue(newId())},
        {"ownerId",authored["documentId"]},{"type","boolean"},{"name","Combine"},{"visible",true}};
    if (arguments.contains("name")) feature["name"] = arguments["name"];
    feature["parameters"] = QJsonObject{{"operation",arguments["operation"]},{"targetBodyId",arguments["targetBodyId"]},
        {"toolBodyIds",arguments["toolBodyIds"]},{"keepTools",arguments["keepTools"]}};
    if (index >= 0) features[index] = feature;else features.append(feature);
    candidate["features"] = features;
    // Evaluation supplies dependency diagnostics; construction only validates
    // the authored representation so failed previews never mutate live state.
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
        if (type == "stepImport") {
            const auto bodyId=feature["bodyId"].toString();
            if (!validId(bodyId) || ids.contains(bodyId)) return "Invalid or duplicate imported body UUID";
            ids.insert(bodyId);
            if (p.size()!=1 || !p["source"].isObject()) return "STEP import requires one embedded source";
            const auto source=p["source"].toObject();
            static const QRegularExpression shaPattern("^[0-9a-f]{64}$");
            if (source.size()!=6 || source["policy"]!="embedded" || source["format"]!="step"
                || !source["bytesBase64"].isString() || source["bytesBase64"].toString().size()>44739244
                || !source["sha256"].isString() || !shaPattern.match(source["sha256"].toString()).hasMatch()
                || !source["byteCount"].isDouble() || source["byteCount"].toInteger()<1
                || source["byteCount"].toInteger()>StepExchange::maximumSourceBytes
                || !source["originalName"].isString() || source["originalName"].toString().isEmpty())
                return "Invalid embedded STEP source metadata";
            continue;
        }
        if (type == "sketch") {
            if (feature.contains("bodyId")) return "Sketches do not own solid body IDs";
            const auto error = SketchGeometry::validateParameters(p, ids);
            if (!error.isEmpty()) return error;
            continue;
        }
        if (type == "boolean") {
            if (feature.contains("bodyId")) return "Boolean features modify a target body and do not own a new body ID";
            if (errorContext) {(*errorContext)["targetBodyId"] = p["targetBodyId"];(*errorContext)["toolBodyIds"] = p["toolBodyIds"];}
            if (p.size() != 4 || !p["operation"].isString()
                || (p["operation"] != "join" && p["operation"] != "cut" && p["operation"] != "intersect"))
                return "Boolean operation must be join, cut or intersect";
            if (!p["targetBodyId"].isString() || !validId(p["targetBodyId"].toString())
                || !p["toolBodyIds"].isArray() || p["toolBodyIds"].toArray().isEmpty() || !p["keepTools"].isBool())
                return "Boolean requires a stable targetBodyId, nonempty toolBodyIds and explicit keepTools";
            QSet<QString> inputs{p["targetBodyId"].toString()};
            for (const auto& input : p["toolBodyIds"].toArray()) {
                if (!input.isString() || !validId(input.toString()) || inputs.contains(input.toString()))
                    return "Boolean target and tools must be distinct stable body IDs";
                inputs.insert(input.toString());
            }
            continue;
        }
        if (type != "box" && type != "cylinder" && type != "extrude") return "Unsupported feature type";
        const bool cut=type=="extrude" && p["operation"]=="cut";
        if(cut) {
            if(feature.contains("bodyId"))return "Extrude Cut modifies a target and cannot own a new body ID";
            if(!p["targetBodyIds"].isArray() || p["targetBodyIds"].toArray().size()!=1 || !p["targetBodyIds"].toArray()[0].isString() || !validId(p["targetBodyIds"].toArray()[0].toString()))return "Cut requires exactly one stable targetBodyIds entry";
        } else {
            const auto bodyId = feature["bodyId"].toString();
            if (!validId(bodyId) || ids.contains(bodyId)) return "Invalid or duplicate authored UUID";
            ids.insert(bodyId);
        }
        if (type == "extrude") {
            if (errorContext) {(*errorContext)["sketchId"] = p["sketchId"];(*errorContext)["profileId"] = p["profileId"];}
            if (p.size()!=(cut ? 5 : 3) || !p["sketchId"].isString() || !validId(p["sketchId"].toString())
                || p.contains("profileId")==p.contains("profileIds")) return "Extrusion requires sketchId and exclusive profileId or profileIds";
            if(p.contains("profileIds")) {
                if(!p["profileIds"].isArray() || p["profileIds"].toArray().isEmpty())return "Extrusion requires a nonempty profileIds array";
                QSet<QString> chosen;
                for(const auto& id:p["profileIds"].toArray()) {
                    if(!id.isString() || !validId(id.toString()) || chosen.contains(id.toString()))return "Extrusion profileIds must be distinct UUIDs";
                    chosen.insert(id.toString());
                }
                if(errorContext)(*errorContext)["profileIds"]=p["profileIds"];
            } else if(!p["profileId"].isString() || !validId(p["profileId"].toString()))return "Extrusion requires a stable profileId";
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
