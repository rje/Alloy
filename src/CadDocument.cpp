#include "SolidFillet.h"
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
bool resultOwns(const QJsonObject& feature,const QJsonValue& body) {for(const auto& value:feature["bodyResults"].toArray())if(value.toObject()["bodyId"]==body)return true;return false;}
bool validId(const QString& id) { return !QUuid(id).isNull() && QUuid(id).toString(QUuid::WithoutBraces) == id; }
QJsonObject dependencyError(const QJsonObject& candidate) {
    const auto features = candidate["features"].toArray();
    QMap<QString,int> positions;
    QMap<QString,int> bodyPositions;
    for (int i = 0; i < features.size(); ++i) positions[features[i].toObject()["id"].toString()] = i;
    for (int i = 0; i < features.size(); ++i) {
        const auto feature = features[i].toObject();
        if(feature["type"]=="constructionPlane" || (feature["type"]=="sketch" && feature["parameters"].toObject()["plane"]=="construction")) {
            const auto source=feature["type"]=="constructionPlane" ? feature["parameters"].toObject()["source"].toObject() : QJsonObject{{"planeId",feature["parameters"].toObject()["planeId"]}};
            const auto reference=source.contains("planeId") ? source["planeId"].toString() : source["sketchId"].toString();
            if(!reference.isEmpty()) {
                const auto expected=source.contains("planeId") ? "constructionPlane" : "sketch";
                if(!positions.contains(reference) || positions[reference]>=i || features[positions[reference]].toObject()["type"]!=expected)
                    return {{"featureId",feature["id"]},{"featureType",feature["type"]},{"code","plane_dependency_unavailable"},{"message","Plane source must be an earlier feature of the expected type"}};
            }
        }
        if (feature.contains("bodyId")) bodyPositions[feature["bodyId"].toString()] = i;
        for(const auto& value:feature["bodyResults"].toArray()) {const auto record=value.toObject();if(record["participantBodyIds"].toArray().isEmpty())bodyPositions[record["bodyId"].toString()]=i;}
    }
    QSet<QString> activeBodies;
    for (int i = 0; i < features.size(); ++i) {
        const auto feature = features[i].toObject();
        if (feature.contains("bodyId")) activeBodies.insert(feature["bodyId"].toString());
        if (feature["type"] == "move" || feature["type"] == "fillet" || feature["type"] == "shell") {
            const auto body = feature["parameters"].toObject()["targetBodyId"].toString();
            if (!bodyPositions.contains(body) || bodyPositions[body]>=i || !activeBodies.contains(body))
                return {{"featureId",feature["id"]},{"featureType",feature["type"]},{"bodyId",body},{"code","body_unavailable"},{"message","Solid modifier requires an active earlier body"}};
            continue;
        }
        if(feature["type"]=="mirror") {
            const auto parameters=feature["parameters"].toObject();const auto source=parameters["sourceBodyId"].toString();
            const auto plane=parameters["mirrorPlane"].toObject();const auto planeBody=plane["bodyId"].toString();
            for(const auto& body:{source,planeBody})if(!bodyPositions.contains(body) || bodyPositions[body]>=i || !activeBodies.contains(body))
                return {{"featureId",feature["id"]},{"featureType","mirror"},{"bodyId",body},{"code","body_unavailable"},{"message","Mirror source and plane body must be active earlier in history"}};
            const auto producer=plane["producerFeatureId"].toString();
            if(!positions.contains(producer) || positions[producer]>=i)
                return {{"featureId",feature["id"]},{"featureType","mirror"},{"code","plane_unavailable"},{"message","Mirror plane producer must precede this feature"}};
            continue;
        }
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
        if((feature["type"]=="sketch" && feature["parameters"].toObject()["plane"]=="face") ||
            (feature["type"]=="constructionPlane" && feature["parameters"].toObject()["source"].toObject().contains("support"))) {
            const auto support=feature["type"]=="sketch" ? feature["parameters"].toObject()["support"].toObject() : feature["parameters"].toObject()["source"].toObject()["support"].toObject();const auto producer=support["producerFeatureId"].toString(),body=support["bodyId"].toString();
            QJsonObject diagnostic{{"featureId",feature["id"]},{"featureType","sketch"},{"support",support}};
            if(!positions.contains(producer)){diagnostic["code"]="missing_dependency";diagnostic["message"]="Sketch support producer does not exist";}
            else if(positions[producer]>=i){diagnostic["code"]="forward_dependency";diagnostic["message"]="Sketch support producer must precede its sketch";}
            else if(features[positions[producer]].toObject()["type"]!="extrude" || (features[positions[producer]].toObject()["bodyId"]!=support["bodyId"] && !features[positions[producer]].toObject()["parameters"].toObject()["targetBodyIds"].toArray().contains(support["bodyId"]) && !resultOwns(features[positions[producer]].toObject(),support["bodyId"]))){diagnostic["code"]="unsupported_dependency";diagnostic["message"]="Planar support requires its exact extrusion producer/body";}
            else if(!activeBodies.contains(body)){diagnostic["code"]="consumed_dependency";diagnostic["message"]="Sketch support body was consumed earlier in history";}
            else continue;
            return diagnostic;
        }
        if(feature["type"]=="extrude" && (feature["parameters"].toObject()["operation"]=="cut" || feature["parameters"].toObject()["operation"]=="join")) {
            for(const auto& value:feature["parameters"].toObject()["targetBodyIds"].toArray()) {const auto target=value.toString();
            QJsonObject diagnostic{{"featureId",feature["id"]},{"featureType","extrude"},{"targetBodyId",target}};
            if(!bodyPositions.contains(target)){diagnostic["code"]="missing_dependency";diagnostic["message"]="Extrusion target body does not exist";return diagnostic;}
            if(bodyPositions.value(target)>=i){diagnostic["code"]="forward_dependency";diagnostic["message"]="Extrusion target must precede its modifier";return diagnostic;}
            if(!activeBodies.contains(target)){diagnostic["code"]="consumed_dependency";diagnostic["message"]="Extrusion target was consumed by an earlier feature";return diagnostic;}}
            QSet<QString> survivors;for(const auto& value:feature["bodyResults"].toArray())survivors.insert(value.toObject()["bodyId"].toString());
            if(!survivors.isEmpty()) {for(const auto& value:feature["parameters"].toObject()["targetBodyIds"].toArray())if(!survivors.contains(value.toString()))activeBodies.remove(value.toString());activeBodies.unite(survivors);}
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
        result.authored = solved;
        auto normalized=solved["features"].toArray();
        for(int i=0;i<normalized.size();++i){auto feature=normalized[i].toObject();if(evaluated->joinBodyResults.contains(feature["id"].toString())){feature["bodyResults"]=evaluated->joinBodyResults[feature["id"].toString()];normalized[i]=feature;}}
        result.authored["features"]=normalized;
        result.evaluation = std::move(evaluated); result.ok = true;
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
    if(operation!="newBody" && operation!="cut" && operation!="join")reject("This extrusion slice supports New Body, Join or Cut only");
    const bool cut=operation!="newBody";
    if(edit && operation!=previousParameters.value("operation").toString("newBody"))reject("Changing an existing extrusion operation requires explicit body identity migration");
    if(cut && newBody)reject("Join/Cut modifies retained targets and cannot own a new body ID");
    if(!cut && !edit && newFeature!=newBody)reject("New Body reserves newFeatureId/newBodyId together");
    if(!cut && arguments.contains("targetBodyIds"))reject("New Body cannot carry retained target references");
    if(!edit) {
        feature={{"id",newFeature ? arguments["newFeatureId"] : QJsonValue(newId())},{"ownerId",authored["documentId"]},{"type","extrude"},{"name",cut ? (operation=="join" ? "Extrude Join" : "Extrude Cut") : "Extrude"},{"visible",true}};
        if(!cut)feature["bodyId"]=newBody ? arguments["newBodyId"] : QJsonValue(newId());
        if(operation=="join")feature["bodyResults"]=QJsonArray{};
    }
    if (arguments.contains("name")) feature["name"] = arguments["name"];
    if(arguments.contains("profileId")==arguments.contains("profileIds"))reject("Choose profileId or profileIds exclusively");
    auto profileParameters=QJsonObject{{"sketchId",arguments["sketchId"]},{"distanceMm",arguments["distanceMm"]}};
    if(cut) {
        const auto targets=arguments.contains("targetBodyIds") ? arguments["targetBodyIds"] : previousParameters["targetBodyIds"];
        if(!targets.isArray() || targets.toArray().isEmpty() || (operation=="cut" && targets.toArray().size()!=1))reject("Join requires participants; Cut requires one target");
        QSet<QString> chosen;for(const auto& value:targets.toArray()){if(!value.isString() || !validId(value.toString()) || chosen.contains(value.toString()))reject("Join/Cut targets must be distinct stable body IDs");chosen.insert(value.toString());}
        profileParameters["operation"]=operation;profileParameters["targetBodyIds"]=targets;
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
QJsonObject CadDocument::filletCandidate(const QJsonObject& authored,const QJsonObject& arguments) {
    const QSet<QString> allowed{"targetBodyId","edgeIds","radiusMm","name","featureId","newFeatureId","tangentChain"};
    for(auto it=arguments.begin();it!=arguments.end();++it)if(!allowed.contains(it.key()))throw std::invalid_argument("Unknown Fillet argument");
    if(arguments.contains("featureId") && arguments.contains("newFeatureId"))throw std::invalid_argument("Choose existing or new Fillet identity");
    if(arguments.contains("tangentChain") && (!arguments["tangentChain"].isBool() || !arguments["tangentChain"].toBool()))throw std::invalid_argument("Only Tangent Chain On is supported");
    auto candidate=authored;auto features=candidate["features"].toArray();QJsonObject feature;int index=-1;
    if(arguments.contains("featureId")) {
        for(int i=0;i<features.size();++i)if(features[i].toObject()["id"]==arguments["featureId"]){feature=features[i].toObject();index=i;break;}
        if(index<0 || feature["type"]!="fillet")throw std::invalid_argument("Fillet feature unavailable");
    } else feature={{"id",arguments.value("newFeatureId").toString(newId())},{"ownerId",authored["documentId"]},{"type","fillet"},{"name","Fillet"},{"visible",true}};
    if(arguments.contains("name"))feature["name"]=arguments["name"];
    feature["parameters"]=QJsonObject{{"targetBodyId",arguments["targetBodyId"]},{"edgeIds",arguments["edgeIds"]},{"radiusMm",arguments["radiusMm"]},{"tangentChain",arguments.value("tangentChain").toBool(true)}};
    if(index>=0)features[index]=feature;else features.append(feature);candidate["features"]=features;
    const auto error=validateStructure(candidate);if(!error.isEmpty())throw std::invalid_argument(error.toStdString());return candidate;
}
QJsonObject CadDocument::shellCandidate(const QJsonObject& authored,const QJsonObject& arguments) {
    const QSet<QString> allowed{"targetBodyId","removedFace","thicknessMm","name","featureId","newFeatureId"};
    for(auto it=arguments.begin();it!=arguments.end();++it)if(!allowed.contains(it.key()))throw std::invalid_argument("Unknown Shell argument");
    if(arguments.contains("featureId") && arguments.contains("newFeatureId"))throw std::invalid_argument("Choose existing or new Shell identity");
    auto candidate=authored;auto features=candidate["features"].toArray();QJsonObject feature;int index=-1;
    if(arguments.contains("featureId")) {
        for(int i=0;i<features.size();++i)if(features[i].toObject()["id"]==arguments["featureId"]){feature=features[i].toObject();index=i;break;}
        if(index<0 || feature["type"]!="shell")throw std::invalid_argument("Shell feature unavailable");
    } else feature={{"id",arguments.value("newFeatureId").toString(newId())},{"ownerId",authored["documentId"]},{"type","shell"},{"name","Shell"},{"visible",true}};
    if(arguments.contains("name"))feature["name"]=arguments["name"];
    feature["parameters"]=QJsonObject{{"targetBodyId",arguments["targetBodyId"]},{"removedFace",arguments["removedFace"]},{"thicknessMm",arguments["thicknessMm"]}};
    if(index>=0)features[index]=feature;else features.append(feature);candidate["features"]=features;
    const auto error=validateStructure(candidate);if(!error.isEmpty())throw std::invalid_argument(error.toStdString());return candidate;
}
QJsonObject CadDocument::mirrorCandidate(const QJsonObject& authored,const QJsonObject& arguments) {
    const QSet<QString> allowed{"sourceBodyId","mirrorPlane","name","featureId","newFeatureId","newBodyId"};
    for(auto it=arguments.begin();it!=arguments.end();++it)if(!allowed.contains(it.key()))throw std::invalid_argument("Unknown Mirror argument");
    if(arguments.contains("featureId") && (arguments.contains("newFeatureId") || arguments.contains("newBodyId")))throw std::invalid_argument("Choose existing or new Mirror identity");
    auto candidate=authored;auto features=candidate["features"].toArray();QJsonObject feature;int index=-1;
    if(arguments.contains("featureId")){
        for(int i=0;i<features.size();++i)if(features[i].toObject()["id"]==arguments["featureId"]){feature=features[i].toObject();index=i;break;}
        if(index<0 || feature["type"]!="mirror")throw std::invalid_argument("Mirror feature unavailable");
    }else feature={{"id",arguments.value("newFeatureId").toString(newId())},{"ownerId",authored["documentId"]},
        {"bodyId",arguments.value("newBodyId").toString(newId())},{"type","mirror"},{"name","Mirror"},{"visible",true}};
    if(arguments.contains("name"))feature["name"]=arguments["name"];
    feature["parameters"]=QJsonObject{{"sourceBodyId",arguments["sourceBodyId"]},{"mirrorPlane",arguments["mirrorPlane"]}};
    if(index>=0)features[index]=feature;else features.append(feature);candidate["features"]=features;
    const auto error=validateStructure(candidate);if(!error.isEmpty())throw std::invalid_argument(error.toStdString());return candidate;
}
QJsonObject CadDocument::moveCandidate(const QJsonObject& authored, const QJsonObject& arguments) {
    const QSet<QString> allowed{"targetBodyId","xMm","yMm","zMm","xDeg","yDeg","zDeg","pivotXMm","pivotYMm","pivotZMm","name","featureId","newFeatureId","steps"};
    for(auto it=arguments.begin();it!=arguments.end();++it)if(!allowed.contains(it.key()))throw std::invalid_argument("Unknown Move argument");
    if(arguments.contains("featureId") && arguments.contains("newFeatureId"))throw std::invalid_argument("Choose existing or new Move identity");
    for(const auto& key:{"xMm","yMm","zMm","xDeg","yDeg","zDeg","pivotXMm","pivotYMm","pivotZMm"})if(arguments.contains(key) && !arguments[key].isDouble())throw std::invalid_argument("Move values must be numeric");
    if(arguments.contains("steps") && !arguments["steps"].isArray())throw std::invalid_argument("Move steps must be an array");
    auto candidate=authored;auto features=authored["features"].toArray();QJsonObject feature;int index=-1;
    if(arguments.contains("featureId")) {
        for(int i=0;i<features.size();++i)if(features[i].toObject()["id"]==arguments["featureId"]){feature=features[i].toObject();index=i;break;}
        if(index<0 || feature["type"]!="move")throw std::invalid_argument("Move feature unavailable");
    } else feature={{"id",arguments.value("newFeatureId").toString(newId())},{"ownerId",authored["documentId"]},{"type","move"},{"name","Move"},{"visible",true}};
    if(arguments.contains("name"))feature["name"]=arguments["name"];
    QJsonObject parameters{{"targetBodyId",arguments["targetBodyId"]}};
    for(const auto& key:{"pivotXMm","pivotYMm","pivotZMm"})parameters[key]=arguments.value(key).toDouble();
    QJsonArray steps;for(const auto& entry:arguments.value("steps").toArray()){
        if(!entry.isObject())throw std::invalid_argument("Move step must be an object");const auto values=entry.toObject();QJsonObject normalized;
        const QSet<QString> keys{"xMm","yMm","zMm","xDeg","yDeg","zDeg"};for(auto it=values.begin();it!=values.end();++it)if(!keys.contains(it.key()) || !it.value().isDouble())throw std::invalid_argument("Unknown or nonnumeric Move step value");
        for(const auto& key:keys)normalized[key]=values.value(key).toDouble();steps.append(normalized);
    }QJsonObject delta;bool changed=false;
    for(const auto& key:{"xMm","yMm","zMm","xDeg","yDeg","zDeg"}){delta[key]=arguments.value(key).toDouble();changed|=delta[key].toDouble()!=0;}
    if(changed)steps.append(delta);parameters["steps"]=steps;
    feature["parameters"]=parameters;if(index>=0)features[index]=feature;else features.append(feature);candidate["features"]=features;
    const auto error=validateStructure(candidate);if(!error.isEmpty())throw std::invalid_argument(error.toStdString());return candidate;
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
        if(type=="constructionPlane") {
            if(feature.contains("bodyId") || p.size()!=2 || !p["source"].isObject() || !p["offsetMm"].isDouble()
                || !std::isfinite(p["offsetMm"].toDouble()) || std::abs(p["offsetMm"].toDouble())>100000)
                return "Construction plane requires one source and a finite offset within 100000 mm";
            const auto source=p["source"].toObject();
            if(source.size()!=1)return "Construction plane requires exactly one source";
            if(source.contains("plane")) {if(source["plane"]!="XY" && source["plane"]!="XZ" && source["plane"]!="YZ")return "Unsupported origin plane source";}
            else if(source.contains("planeId") || source.contains("sketchId")) {const auto reference=source.begin().value();if(!reference.isString() || !validId(reference.toString()))return "Plane source requires a stable feature ID";}
            else if(source.contains("support")) {const auto support=source["support"].toObject();if(support.size()!=3)return "Plane face source requires producer, body and face IDs";for(const auto& key:{"producerFeatureId","bodyId","faceId"})if(!validId(support[key].toString()))return "Plane face source requires stable IDs";}
            else return "Unsupported construction plane source";
            continue;
        }
        if(type=="fillet") {
            if(feature.contains("bodyId") || p.size()!=4 || !validId(p["targetBodyId"].toString()) || !p["edgeIds"].isArray() || p["edgeIds"].toArray().isEmpty() || p["edgeIds"].toArray().size()>100 || !p["tangentChain"].isBool() || !p["tangentChain"].toBool())return "Fillet requires one target, edge references and tangent chain enabled";
            QSet<QString> edges;for(const auto& edge:p["edgeIds"].toArray()){if(!edge.isString() || !validId(edge.toString()) || edges.contains(edge.toString()))return "Fillet edge references must be distinct stable UUIDs";edges.insert(edge.toString());}
            if(!p["radiusMm"].isDouble() || !std::isfinite(p["radiusMm"].toDouble()) || p["radiusMm"].toDouble()<0.001 || p["radiusMm"].toDouble()>100000)return "Fillet radius must be 0.001..100000 mm";
            continue;
        }
        if(type=="shell") {
            if(feature.contains("bodyId") || p.size()!=3 || !validId(p["targetBodyId"].toString()) || !p["removedFace"].isObject())return "Shell requires one active target body and a stable face reference";
            const auto face=p["removedFace"].toObject();
            if(face.size()!=3 || face["bodyId"]!=p["targetBodyId"] || !validId(face["producerFeatureId"].toString()) || !validId(face["bodyId"].toString()) || !validId(face["faceId"].toString()))return "Shell requires one stable face on its target body";
            if(!p["thicknessMm"].isDouble() || !std::isfinite(p["thicknessMm"].toDouble()) || p["thicknessMm"].toDouble()<0.001 || p["thicknessMm"].toDouble()>100000)return "Shell thickness must be 0.001..100000 mm";
            continue;
        }
        if(type=="mirror") {
            if(!validId(feature["bodyId"].toString()) || ids.contains(feature["bodyId"].toString()) || p.size()!=2 || !validId(p["sourceBodyId"].toString()) || !p["mirrorPlane"].isObject())return "Mirror requires new body identity, one source body and a planar reference";
            ids.insert(feature["bodyId"].toString());
            const auto plane=p["mirrorPlane"].toObject();
            if(plane.size()!=3 || !validId(plane["producerFeatureId"].toString()) || !validId(plane["bodyId"].toString()) || !validId(plane["faceId"].toString()))return "Mirror plane needs stable face identity";
            continue;
        }
        if (type == "move") {
            if(feature.contains("bodyId") || p.size()!=5 || !validId(p["targetBodyId"].toString()) || !p["steps"].isArray() || p["steps"].toArray().size()>1000)return "Move requires one retained target body and bounded steps";
            for(const auto& key:{"pivotXMm","pivotYMm","pivotZMm"})if(!p[key].isDouble() || !std::isfinite(p[key].toDouble()) || std::abs(p[key].toDouble())>100000)return "Invalid Move pivot";
            for(const auto& step:p["steps"].toArray()){if(!step.isObject() || step.toObject().size()!=6)return "Invalid Move step";for(const auto& key:{"xMm","yMm","zMm","xDeg","yDeg","zDeg"})if(!step.toObject()[key].isDouble() || !std::isfinite(step.toObject()[key].toDouble()) || std::abs(step.toObject()[key].toDouble())>100000)return "Invalid Move step value";}
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
        const bool cut=type=="extrude" && (p["operation"]=="cut" || p["operation"]=="join");
        if(cut) {
            if(feature.contains("bodyId"))return "Extrude Join/Cut modifies a target and cannot own a new body ID";
            if(!p["targetBodyIds"].isArray() || p["targetBodyIds"].toArray().isEmpty() || ((p["operation"]=="cut" || !feature.contains("bodyResults")) && p["targetBodyIds"].toArray().size()!=1))return "Join requires participants; Cut and legacy Join require one target";
            QSet<QString> targets;for(const auto& value:p["targetBodyIds"].toArray()){if(!value.isString() || !validId(value.toString()) || targets.contains(value.toString()))return "Join/Cut targets must be distinct stable body IDs";targets.insert(value.toString());}
        } else {
            const auto bodyId = feature["bodyId"].toString();
            if (!validId(bodyId) || ids.contains(bodyId)) return "Invalid or duplicate authored UUID";
            ids.insert(bodyId);
        }
        if(feature.contains("bodyResults")) {
            if(type!="extrude" || p["operation"]!="join" || !feature["bodyResults"].isArray())return "Body-result ownership belongs to Extrude Join only";
            QSet<QString> resultIds,participating,profiles;
            for(const auto& value:feature["bodyResults"].toArray()) {
                const auto record=value.toObject();const auto body=record["bodyId"].toString();
                if(!value.isObject() || record.size()!=3 || !validId(body) || resultIds.contains(body) || !record["participantBodyIds"].isArray() || !record["profileIds"].isArray() || record["profileIds"].toArray().isEmpty())return "Invalid Join body-result ownership";
                resultIds.insert(body);const auto participants=record["participantBodyIds"].toArray();
                if(participants.isEmpty()){if(ids.contains(body))return "Created Join body ID must be unique";ids.insert(body);}else if(body!=participants.first().toString())return "Join result retains its first ordered participant";
                for(const auto& participant:participants){if(!participant.isString() || !validId(participant.toString()) || participating.contains(participant.toString()))return "Invalid Join participant ownership";participating.insert(participant.toString());}
                for(const auto& profile:record["profileIds"].toArray()){if(!profile.isString() || !validId(profile.toString()) || profiles.contains(profile.toString()))return "Invalid Join source ownership";profiles.insert(profile.toString());}
            }

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
        auto summary=body.summary; summary["solidEdges"]=SolidFillet::descriptors(body.solidEdges); summary["featureId"]=body.featureId; summary["bodyId"]=body.bodyId;
        summary["name"]=body.name; summary["visible"]=body.visible; evaluated.append(summary);
    }
    QJsonArray evaluatedSketches;
    for (const auto& sketch : evaluation_->sketches) {
        auto summary = sketch.summary;
        summary["featureId"] = sketch.featureId; summary["sketchId"] = sketch.featureId;
        summary["name"] = sketch.name; summary["visible"] = sketch.visible;
        evaluatedSketches.append(summary);
    }
    QJsonArray evaluatedPlanes;
    for(const auto& plane:evaluation_->constructionPlanes)evaluatedPlanes.append(QJsonObject{{"featureId",plane.featureId},{"name",plane.name},{"visible",plane.visible},{"frame",plane.frame}});
    return {{"loaded", true}, {"id", authored_["documentId"]}, {"label", path_.isEmpty() ? "Untitled" : QFileInfo(path_).completeBaseName()}, {"path",path_},
        {"units", authored_["displayUnits"]}, {"canonicalLengthUnit", "mm"},
        {"revision", static_cast<qint64>(revision_)}, {"dirty", !history_.isClean()},
        {"lastTransactionId", lastTransaction_},
        {"features", authored_["features"].toArray().size()}, {"bodies",static_cast<int>(evaluation_->bodies.size())}, {"sketches", static_cast<int>(evaluation_->sketches.size())},
        {"evaluation",QJsonObject{{"status","current"},{"recompute","full_ordered"},{"featureOrder",order},{"joinBodyResults",evaluation_->joinBodyResults},
            {"documentRevision",static_cast<qint64>(revision_)},{"bodies",evaluated},{"sketches",evaluatedSketches},{"constructionPlanes",evaluatedPlanes}}},
        {"editableHistory", true}, {"authored", authored_},
        {"undo", QJsonObject{{"canUndo", history_.canUndo()}, {"canRedo", history_.canRedo()},
            {"count", history_.count()}, {"index", history_.index()},
            {"undoText", history_.undoText()}, {"redoText", history_.redoText()}}}};
}
