#include "SolidFillet.h"
#include "CadDocument.h"
#include <utility>
#include <vector>
#include "DocumentAppend.h"
#include "CircularPattern.h"
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
#include <functional>
#include <numbers>
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
        if(feature["type"]=="constructionPlane" || (feature["type"]=="sketch" && feature["parameters"].toObject()["plane"]=="construction")) {
            const auto source=feature["type"]=="constructionPlane" ? feature["parameters"].toObject()["source"].toObject() : QJsonObject{{"planeId",feature["parameters"].toObject()["planeId"]}};
            const auto reference=source.contains("planeId") ? source["planeId"].toString() : source["sketchId"].toString();
            if(!reference.isEmpty()) {
                const auto expected=source.contains("planeId") ? "constructionPlane" : "sketch";
                if(!positions.contains(reference) || positions[reference]>=i || features[positions[reference]].toObject()["type"]!=expected)
                    return {{"featureId",feature["id"]},{"featureType",feature["type"]},{"code","plane_dependency_unavailable"},{"message","Plane source must be an earlier feature of the expected type"}};
            }
        }
        if(feature["type"]=="rectangularPattern") {
            const auto p=feature["parameters"].toObject();const auto seed=p["sourceFeatureId"].toString();
            if(!positions.contains(seed) || positions[seed]>=i)
                return {{"featureId",feature["id"]},{"code","pattern_seed_unavailable"},{"message","Pattern seed must be an earlier feature"}};
            const auto source=features[positions[seed]].toObject();
            if(source["type"]!="extrude" && source["type"]!="hole")
                return {{"featureId",feature["id"]},{"code","pattern_seed_unavailable"},{"message","Pattern seed must be an earlier supported feature"}};
            if((source["type"]=="hole")!=p.contains("distribution"))
                return {{"featureId",feature["id"]},{"code","pattern_mode_mismatch"},{"message","Pattern distribution must match its retained seed type"}};
            if(source["type"]=="hole" && (source["parameters"].toObject()["targetBodyId"]!=p["targetBodyId"] || source["ownerId"]!=feature["ownerId"]))
                return {{"featureId",feature["id"]},{"code","pattern_seed_unavailable"},{"message","Hole Pattern seed and target must share body and owner"}};
        }
        if(feature["type"]=="loft")for(const auto& value:feature["parameters"].toObject()["profiles"].toArray()) {
            const auto sketch=value.toObject()["sketchId"].toString();
            if(!positions.contains(sketch) || positions[sketch]>=i || features[positions[sketch]].toObject()["type"]!="sketch")return {{"featureId",feature["id"]},{"featureType","loft"},{"code","loft_profile_unavailable"},{"message","Loft sections must reference earlier sketch features"}};
        }
        if(feature["type"]=="circularPattern"){const auto p=feature["parameters"].toObject();for(int n=1;n<p["quantity"].toInt();++n)bodyPositions[CircularPattern::bodyId(feature["id"].toString(),p["sourceBodyId"].toString(),n)]=i;}
        if (feature.contains("bodyId")) bodyPositions[feature["bodyId"].toString()] = i;
        for(const auto& value:feature["bodyResults"].toArray()) {const auto record=value.toObject();if(record["participantBodyIds"].toArray().isEmpty())bodyPositions[record["bodyId"].toString()]=i;}
    }
    QMap<QString,QString> referencedOwners;
    for(const auto& value:features) {
        const auto f=value.toObject();const auto owner=f["ownerId"].toString();referencedOwners[f["id"].toString()]=owner;
        if(f.contains("bodyId"))referencedOwners[f["bodyId"].toString()]=owner;
        if(f["type"]=="circularPattern"){const auto p=f["parameters"].toObject();for(int n=1;n<p["quantity"].toInt();++n)referencedOwners[CircularPattern::bodyId(f["id"].toString(),p["sourceBodyId"].toString(),n)]=owner;}
        for(const auto& result:f["bodyResults"].toArray()){const auto body=result.toObject()["bodyId"].toString();if(!referencedOwners.contains(body))referencedOwners[body]=owner;}
    }
    QSet<QString> identityChildren;
    const auto root=candidate["documentId"].toString();
    for(const auto& value:candidate["occurrences"].toArray()) {
        const auto occurrence=value.toObject();
        if(occurrence["parentId"]==root && occurrence["placement"]=="identity")
            identityChildren.insert(occurrence["definitionId"].toString());
    }
    for(const auto& value:features) {
        const auto f=value.toObject();const auto owner=f["ownerId"].toString();
        std::function<bool(const QJsonValue&)> crossOwner=[&](const QJsonValue& v) {
            if(v.isString())return referencedOwners.contains(v.toString()) && referencedOwners[v.toString()]!=owner;
            if(v.isArray())for(const auto& child:v.toArray())if(crossOwner(child))return true;
            if(v.isObject())for(const auto& child:v.toObject())if(crossOwner(child))return true;
            return false;
        };
        auto guarded=f["parameters"].toObject();
        if(f["type"]=="sketch" && guarded["plane"]=="face") {
            auto permitted=[&](const QJsonObject& source)->bool {
                const auto body=source["bodyId"].toString(),producer=source["producerFeatureId"].toString();
                if(!referencedOwners.contains(body))return false;
                const auto sourceOwner=referencedOwners[body];
                if(sourceOwner==owner)return true;
                if(!identityChildren.contains(owner) || !identityChildren.contains(sourceOwner))return false;
                return producer.isEmpty() || (referencedOwners.value(producer)==sourceOwner);
            };
            const auto support=guarded["support"].toObject();
            if(!support.isEmpty()) {
                if(!permitted(support))return {{"featureId",f["id"]},{"code","cross_component_dependency"},{"message","Sketch support requires an earlier identity sibling body"}};
                guarded.remove("support");
            }
            for(const auto& key:{"projections","pointProjections"}) {
                bool valid=true;
                for(const auto& item:guarded[key].toArray())valid &= permitted(item.toObject()["source"].toObject());
                if(!valid)return {{"featureId",f["id"]},{"code","cross_component_dependency"},{"message","Linked Project requires an identity sibling source body"}};
                guarded.remove(key);
            }
        }
        if(crossOwner(guarded))return {{"featureId",f["id"]},{"code","cross_component_dependency"},{"message","Cross-component modeling references are not supported"}};
    }
    QSet<QString> activeBodies;
    for (int i = 0; i < features.size(); ++i) {
        const auto feature = features[i].toObject();
        if(feature["type"]=="sketch" && feature["parameters"].toObject()["plane"]=="face") {
            const auto parameters=feature["parameters"].toObject();
            auto earlierBody=[&](const QJsonObject& source) {
                const auto body=source["bodyId"].toString();
                return bodyPositions.contains(body) && bodyPositions[body]<i && activeBodies.contains(body);
            };
            if(!earlierBody(parameters["support"].toObject()))return {{"featureId",feature["id"]},
                {"code","sketch_support_unavailable"},{"message","Face sketch support must be an earlier active body"}};
            for(const auto& key:{"projections","pointProjections"})for(const auto& item:parameters[key].toArray())
                if(!earlierBody(item.toObject()["source"].toObject()))return {{"featureId",feature["id"]},
                    {"code","projection_unavailable"},{"message","Projected source must be an earlier active body"}};
        }
        if (feature.contains("bodyId")) activeBodies.insert(feature["bodyId"].toString());
        if (feature["type"] == "move" || feature["type"] == "fillet" || feature["type"] == "chamfer" || feature["type"] == "shell" || feature["type"]=="rectangularPattern") {
            const auto body = feature["parameters"].toObject()["targetBodyId"].toString();
            if (!bodyPositions.contains(body) || bodyPositions[body]>=i || !activeBodies.contains(body))
                return {{"featureId",feature["id"]},{"featureType",feature["type"]},{"bodyId",body},{"code","body_unavailable"},{"message","Solid modifier requires an active earlier body"}};
            continue;
        }
        if(feature["type"]=="circularPattern"){
            const auto p=feature["parameters"].toObject();const auto source=p["sourceBodyId"].toString();
            if(!bodyPositions.contains(source)||bodyPositions[source]>=i||!activeBodies.contains(source))return {{"featureId",feature["id"]},{"code","pattern_body_unavailable"},{"message","Circular Pattern requires an active earlier body"}};
            for(int n=1;n<p["quantity"].toInt();++n)activeBodies.insert(CircularPattern::bodyId(feature["id"].toString(),source,n));continue;
        }
        if(feature["type"]=="hole") {
            const auto p=feature["parameters"].toObject();const auto sketch=p["sourceSketchId"].toString(),body=p["targetBodyId"].toString();
            if(!positions.contains(sketch) || positions[sketch]>=i || features[positions[sketch]].toObject()["type"]!="sketch"
                || !bodyPositions.contains(body) || bodyPositions[body]>=i || !activeBodies.contains(body))
                return {{"featureId",feature["id"]},{"featureType","hole"},{"code","hole_dependency_unavailable"},
                    {"message","Hole requires an earlier supported sketch point and active target body"}};
            const auto source=features[positions[sketch]].toObject();const auto support=source["parameters"].toObject()["support"].toObject();
            if(source["ownerId"]!=feature["ownerId"] || source["parameters"].toObject()["plane"]!="face" || support["bodyId"]!=body)
                return {{"featureId",feature["id"]},{"featureType","hole"},{"code","hole_support_unavailable"},
                    {"message","Hole source must be a supported sketch on its same-owner target body"}};
            continue;
        }
        if(feature["type"]=="sphere") {
            const auto p=feature["parameters"].toObject(),support=p["support"].toObject();
            const auto body=support["bodyId"].toString(),producer=support["producerFeatureId"].toString();
            if(!bodyPositions.contains(body) || bodyPositions[body]>=i || !activeBodies.contains(body)
                || !positions.contains(producer) || positions[producer]>=i)
                return {{"featureId",feature["id"]},{"code","sphere_support_unavailable"},{"message","Sphere requires an active earlier planar support"}};
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
            // Boolean descendants can carry an earlier producer's face onto
            // a different surviving body. The evaluated support lookup below
            // verifies the exact producer/body/face triple, never proximity.
            else if(!bodyPositions.contains(body) || bodyPositions[body]>=i){diagnostic["code"]="unsupported_dependency";diagnostic["message"]="Planar support requires an earlier evaluated body";}
            else if(!activeBodies.contains(body)){diagnostic["code"]="consumed_dependency";diagnostic["message"]="Sketch support body was consumed earlier in history";}
            else continue;
            return diagnostic;
        }
        if(feature["type"]=="extrude" && (feature["parameters"].toObject()["operation"]=="cut" || feature["parameters"].toObject()["operation"]=="join" || feature["parameters"].toObject()["operation"]=="intersect")) {
            for(const auto& value:feature["parameters"].toObject()["targetBodyIds"].toArray()) {const auto target=value.toString();
            QJsonObject diagnostic{{"featureId",feature["id"]},{"featureType","extrude"},{"targetBodyId",target}};
            if(!bodyPositions.contains(target)){diagnostic["code"]="missing_dependency";diagnostic["message"]="Extrusion target body does not exist";return diagnostic;}
            if(bodyPositions.value(target)>=i){diagnostic["code"]="forward_dependency";diagnostic["message"]="Extrusion target must precede its modifier";return diagnostic;}
            if(!activeBodies.contains(target)){diagnostic["code"]="consumed_dependency";diagnostic["message"]="Extrusion target was consumed by an earlier feature";return diagnostic;}}
            QSet<QString> survivors;for(const auto& value:feature["bodyResults"].toArray())survivors.insert(value.toObject()["bodyId"].toString());
            if(!survivors.isEmpty()) {for(const auto& value:feature["parameters"].toObject()["targetBodyIds"].toArray())if(!survivors.contains(value.toString()))activeBodies.remove(value.toString());activeBodies.unite(survivors);}
        }
        if(feature["type"]=="sweep")for(const auto& field:{"profileSketchId","pathSketchId"}) {
            const auto source=feature["parameters"].toObject()[field].toString();if(positions.contains(source) && positions.value(source)<i && features[positions.value(source)].toObject()["type"]=="sketch")continue;
            return QJsonObject{{"code",!positions.contains(source)?"missing_dependency":positions.value(source)>=i?"forward_dependency":"unsupported_dependency"},{"message","Sweep sources must be earlier retained sketches"},{"featureId",feature["id"]},{"featureType","sweep"},{"input",field},{"sketchId",source}};
        }
        if (feature["type"] != "extrude" && feature["type"] != "revolve") continue;
        const auto parameters = feature["parameters"].toObject();
        const auto source = parameters["sketchId"].toString(), id = feature["id"].toString();
        QJsonObject diagnostic{{"featureId",id},{"featureType",feature["type"]},{"sketchId",source}};
        if(parameters.contains("profileIds"))diagnostic["profileIds"]=parameters["profileIds"];
        else diagnostic["profileId"]=parameters["profileId"];
        const QString operationName=feature["type"]=="revolve" ? "Revolve":"Extrusion";
        if(feature["type"]=="revolve")diagnostic["axisEntityId"]=parameters["axisEntityId"];
        if (source == id) {diagnostic["code"] = "cyclic_dependency";diagnostic["message"] = operationName+" cannot reference itself";}
        else if (!positions.contains(source)) {diagnostic["code"] = "missing_dependency";diagnostic["message"] = operationName+" input sketch does not exist";}
        else if (features[positions.value(source)].toObject()["type"] != "sketch") {
            diagnostic["code"] = "unsupported_dependency";diagnostic["message"] = operationName+" input must reference a sketch feature";
        } else if (positions.value(source) >= i) {
            diagnostic["code"] = "forward_dependency";diagnostic["message"] = operationName+" input sketch must precede it in feature history";
        } else continue;
        return diagnostic;
    }
    // Every accepted feature input precedes its consumer; accepted histories
    // cannot form cycles and consumed bodies cannot be used as stale inputs.
    return {};
}
QJsonObject solveSketches(QJsonObject candidate,int firstFeature=0,const EvaluatedDocument* seed=nullptr) {
    auto features = candidate["features"].toArray();
    for (int i = firstFeature; i < features.size(); ++i) {
        const auto feature = features[i].toObject();
        if (feature["type"].toString() != "sketch") continue;
        QJsonObject projectionContext;
        if(feature["parameters"].toObject()["plane"]=="face") {
            auto prior=candidate;QJsonArray prefix;for(int j=0;j<i;++j)prefix.append(features[j]);prior["features"]=prefix;
            try {projectionContext=GeometryKernel::projectionContext(seed ? *seed : GeometryKernel::evaluateDocument(prior),feature);}
            catch(const GeometryKernel::EvaluationError& error) {
                auto diagnostics=error.diagnostics();
                if(!diagnostics.contains("featureId"))diagnostics["featureId"]=feature["id"];
                if(diagnostics["featureId"]==feature["id"] && !diagnostics.contains("featureType"))diagnostics["featureType"]="sketch";
                throw GeometryKernel::EvaluationError(diagnostics);
            }
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

bool CadDocument::hasOwner(const QJsonObject& authored,const QString& ownerId) {
    if(ownerId==authored["documentId"].toString())return true;
    for(const auto& value:authored["components"].toArray())if(value.toObject()["id"]==ownerId)return true;
    return false;
}
QString CadDocument::activeOwnerId() const {return hasOwner(authored_,activeOwnerId_) ? activeOwnerId_ : authored_["documentId"].toString();}
QString CadDocument::activateOwner(const QString& ownerId) {
    if(!hasOwner(authored_,ownerId))return "Component definition does not exist";
    activeOwnerId_=ownerId;return {};
}
QJsonObject CadDocument::componentCandidate(const QJsonObject& authored,const QString& name) {
    if(name.trimmed().isEmpty())throw std::invalid_argument("Component name is required");
    auto candidate=authored;auto components=candidate["components"].toArray();auto occurrences=candidate["occurrences"].toArray();
    const auto id=newId();components.append(QJsonObject{{"id",id},{"name",name.trimmed()}});
    occurrences.append(QJsonObject{{"id",newId()},{"definitionId",id},{"parentId",authored["documentId"]},{"placement","identity"}});
    candidate["components"]=components;candidate["occurrences"]=occurrences;return candidate;
}
QJsonObject CadDocument::lengthParameterCandidate(const QJsonObject& authored,const QJsonObject& arguments) {
    const QSet<QString> allowed{"parameterId","name","value","units"};
    for(auto it=arguments.begin();it!=arguments.end();++it)if(!allowed.contains(it.key()))throw std::invalid_argument("Unknown length parameter argument");
    auto candidate=authored;auto rows=candidate["userParameters"].toArray();
    const auto id=arguments.value("parameterId").toString();int index=-1;
    if(!id.isEmpty())for(int i=0;i<rows.size();++i)if(rows[i].toObject()["id"]==id){index=i;break;}
    if(!id.isEmpty() && index<0)throw std::invalid_argument("Length parameter ID is unavailable");
    auto row=index<0 ? QJsonObject{{"id",newId()},{"name",""},{"units","mm"},{"valueMm",0}} : rows[index].toObject();
    if(arguments.contains("name"))row["name"]=arguments["name"];
    if(arguments.contains("units"))row["units"]=arguments["units"];
    if(arguments.contains("value"))row["valueMm"]=millimeters(arguments["value"].toDouble(),row["units"].toString());
    else if(index<0)throw std::invalid_argument("Length parameter value is required");
    if(index<0)rows.append(row);else rows[index]=row;
    candidate["userParameters"]=rows;
    const auto error=validateStructure(candidate);if(!error.isEmpty())throw std::invalid_argument(error.toStdString());
    return candidate;
}
QJsonObject CadDocument::resolveLengthBindings(const QJsonObject& authored) {
    QMap<QString,double> values;
    for(const auto& entry:authored["userParameters"].toArray()) {
        const auto row=entry.toObject();values[row["id"].toString()]=row["valueMm"].toDouble();
    }
    auto result=authored;auto features=result["features"].toArray();
    for(int i=0;i<features.size();++i) {
        auto feature=features[i].toObject();auto p=feature["parameters"].toObject();
        if(feature["type"]=="sketch") {
            auto dims=p.value("dimensions").toArray();bool changed=false;
            for(int j=0;j<dims.size();++j) {
                auto dim=dims[j].toObject();if(!dim.contains("parameterId"))continue;
                const auto id=dim["parameterId"].toString();
                if(!values.contains(id))throw std::invalid_argument("Sketch dimension references a missing length parameter");
                if(!dim["driving"].toBool() || dim["kind"]=="angle" || dim["kind"]=="arcSweep")throw std::invalid_argument("Only driving length dimensions accept length parameters");
                dim["valueMm"]=values[id];dims[j]=dim;changed=true;
            }
            if(changed)p["dimensions"]=dims;
        } else if(feature["type"]=="extrude") {
            for(const auto& binding:std::vector<std::pair<QString,QString>>{{"distanceParameterId","distanceMm"},{"startOffsetParameterId","startOffsetMm"}}) {
                if(!p.contains(binding.first))continue;
                const auto id=p[binding.first].toString();
                if(!values.contains(id))throw std::invalid_argument("Extrude references a missing length parameter");
                p[binding.second]=values[id];
            }
        } else if(feature["type"]=="box" || feature["type"]=="shell") {
            const QStringList fields=feature["type"]=="box" ? QStringList{"width","depth","height"} : QStringList{"thickness"};
            for(const auto& field:fields) {
                const auto idKey=field+"ParameterId";
                if(!p.contains(idKey))continue;
                const auto id=p[idKey].toString();
                if(!values.contains(id))throw std::invalid_argument((feature["type"].toString()+" references a missing length parameter").toStdString());
                p[field+"Mm"]=values[id];
            }
        }
        feature["parameters"]=p;features[i]=feature;
    }
    result["features"]=features;return result;
}
QString CadDocument::newId() { return QUuid::createUuid().toString(QUuid::WithoutBraces); }
CadDocument::CadDocument() { reset(); }
void CadDocument::reset() {
    history_.clear();
    activeOwnerId_.clear();
    path_.clear();
    lastEvaluationError_ = {};
    apply({{"schema", "alloy-document/1"}, {"documentId", newId()},
        {"displayUnits", "mm"}, {"features", QJsonArray{}}},std::make_shared<const EvaluatedDocument>());
    history_.setClean();
}
void CadDocument::apply(const QJsonObject& value, std::shared_ptr<const EvaluatedDocument> evaluation, const QString& transaction) {
    authored_ = value;
    if(!hasOwner(authored_,activeOwnerId_))activeOwnerId_=authored_["documentId"].toString();
    evaluation_=std::move(evaluation); lastTransaction_ = transaction; ++revision_;
}
QString CadDocument::commitAnalyses(const QJsonArray& analyses,const QString& label) {
    auto candidate=authored_;candidate["analyses"]=analyses;
    lastEvaluationError_={};const auto error=validateStructure(candidate);
    if(!error.isEmpty()){lastEvaluationError_={{"code","invalid_document"},{"message",error}};return error;}
    if(candidate!=authored_)history_.push(new DocumentEdit(*this,candidate,evaluation_,label));
    return {};
}
QJsonObject CadDocument::sectionCandidate(const QJsonObject& authored,const QJsonObject& args) {
    const QSet<QString> allowed{"analysisId","newAnalysisId","name","plane","visible","hatch"};
    for(auto it=args.begin();it!=args.end();++it)if(!allowed.contains(it.key()))throw std::invalid_argument("Unknown section argument");
    auto candidate=authored;auto rows=candidate["analyses"].toArray();int index=-1;
    const auto id=args["analysisId"].toString();
    if(!id.isEmpty())for(int i=0;i<rows.size();++i)if(rows[i].toObject()["id"]==id){index=i;break;}
    if(!id.isEmpty() && index<0)throw std::invalid_argument("Section analysis is unavailable");
    auto row=index<0 ? QJsonObject{{"id",args.value("newAnalysisId").toString(newId())},{"type","section"},{"name","Section Analysis"},{"plane","XZ"},{"visible",true},{"hatch",true}} : rows[index].toObject();
    for(const auto& key:{"name","plane","visible","hatch"})if(args.contains(key))row[key]=args[key];
    if(row["visible"].toBool())for(int i=0;i<rows.size();++i){auto prior=rows[i].toObject();prior["visible"]=false;rows[i]=prior;}
    if(index<0)rows.append(row);else rows[index]=row;
    candidate["analyses"]=rows;const auto error=validateStructure(candidate);
    if(!error.isEmpty())throw std::invalid_argument(error.toStdString());return candidate;
}
QString CadDocument::commit(const QJsonObject& candidate, const QString& label) {
    lastEvaluationError_ = {};
    if (candidate["documentId"] != authored_["documentId"]) {
        const QString error = "An edit cannot change document identity";
        lastEvaluationError_ = {{"code","invalid_document"},{"message",error}}; return error;
    }
    if (candidate == authored_) return {};
    auto prepared = prepareCandidate(candidate);
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
    activeOwnerId_.clear();
    history_.clear(); apply(prepared.authored,std::move(prepared.evaluation)); path_=path; history_.setClean();
    return {};
}
CadDocument::CandidateEvaluation CadDocument::prepareCandidate(const QJsonObject& candidate) const {
    if(evaluation_)try {
        const auto plan=DocumentAppend::classify(authored_,candidate,resolveLengthBindings(authored_),
            resolveLengthBindings(candidate),evaluation_->featureOrder);
        if(plan) {
            auto prefix=*evaluation_;
            if(!plan->hiddenSketchId.isEmpty())for(auto& sketch:prefix.sketches)
                if(sketch.featureId==plan->hiddenSketchId)sketch.visible=false;
            return evaluateCandidateFromPrefix(candidate,&prefix,plan->featureCount);
        }
    } catch(const std::exception&) {} // Ordinary evaluator owns structured errors.
    return evaluateCandidate(candidate);
}
CadDocument::CandidateEvaluation CadDocument::evaluateCandidate(const QJsonObject& candidate) {
    return evaluateCandidateFromPrefix(candidate,nullptr,0);
}
CadDocument::CandidateEvaluation CadDocument::evaluateCandidateFromPrefix(const QJsonObject& candidate,
    const EvaluatedDocument* prefix,int firstFeature) {
    CandidateEvaluation result;
    result.authored = candidate;
    QJsonObject structuralContext;
    QJsonObject resolved;
    try {resolved=resolveLengthBindings(candidate);}
    catch(const std::exception& error){result.error=QString::fromUtf8(error.what());result.diagnostics={{"code","invalid_parameter_binding"},{"message",result.error}};return result;}
    const auto structural = validateStructure(resolved,&structuralContext);
    if (!structural.isEmpty()) {
        result.error = structural; result.diagnostics = structuralContext;
        result.diagnostics["code"] = "invalid_document";result.diagnostics["message"] = structural;return result;
    }
    const auto dependency = dependencyError(resolved);
    if (!dependency.isEmpty()) {result.diagnostics = dependency;result.error = dependency["message"].toString();return result;}
    try {
        const auto solved = solveSketches(resolved,firstFeature,prefix);
        auto evaluated = std::make_shared<const EvaluatedDocument>(prefix
            ? GeometryKernel::evaluateDocument(solved,*prefix) : GeometryKernel::evaluateDocument(solved));
        result.authored = solved;
        auto normalized=solved["features"].toArray();
        for(int i=0;i<normalized.size();++i){auto feature=normalized[i].toObject();if(evaluated->joinBodyResults.contains(feature["id"].toString())){feature["bodyResults"]=evaluated->joinBodyResults[feature["id"].toString()];normalized[i]=feature;}}
        result.authored["features"]=normalized;
        result.evaluation = std::move(evaluated); result.ok = true;
        result.diagnostics = {{"code","evaluated"},{"recompute",prefix ? "exact_prefix_append" : "full_ordered"},{"featureCount",solved["features"].toArray().size()}};
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
    const QSet<QString> allowed{"ownerId","path", "name", "newFeatureId", "newBodyId"};
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
        {"ownerId",arguments.value("ownerId").toString(authored["documentId"].toString())},{"type","stepImport"},
        {"name",name},{"visible",true},{"parameters",QJsonObject{{"source",source}}}});
    candidate["features"]=features;
    return candidate;
}
QJsonObject CadDocument::extrusionCandidate(const QJsonObject& authored, const QJsonObject& arguments) {
    auto reject = [](const QString& message) {throw std::invalid_argument(message.toStdString());};
    const auto initial = validate(authored);
    if (!initial.isEmpty()) reject(initial);
    const QSet<QString> allowed{"ownerId","sketchId","profileId","profileIds","distanceMm","distanceParameterId","start","startOffsetMm","startOffsetParameterId","name","featureId","newFeatureId","newBodyId","operation","targetBodyIds"};
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
    if(operation!="newBody" && operation!="cut" && operation!="join" && operation!="intersect")reject("This extrusion slice supports New Body, Join, Cut or Intersect only");
    const bool cut=operation!="newBody";
    if(edit && operation!=previousParameters.value("operation").toString("newBody"))reject("Changing an existing extrusion operation requires explicit body identity migration");
    if(cut && newBody)reject("Join/Cut/Intersect modifies retained targets and cannot own a new body ID");
    if(!cut && !edit && newFeature!=newBody)reject("New Body reserves newFeatureId/newBodyId together");
    if(!cut && arguments.contains("targetBodyIds"))reject("New Body cannot carry retained target references");
    if(!edit) {
        feature={{"id",newFeature ? arguments["newFeatureId"] : QJsonValue(newId())},{"ownerId",arguments.value("ownerId").toString(authored["documentId"].toString())},{"type","extrude"},{"name",cut ? (operation=="join" ? "Extrude Join" : operation=="intersect" ? "Extrude Intersect" : "Extrude Cut") : "Extrude"},{"visible",true}};
        if(!cut)feature["bodyId"]=newBody ? arguments["newBodyId"] : QJsonValue(newId());
        if(operation=="join")feature["bodyResults"]=QJsonArray{};
    }
    if (arguments.contains("name")) feature["name"] = arguments["name"];
    if(arguments.contains("profileId")==arguments.contains("profileIds"))reject("Choose profileId or profileIds exclusively");
    auto profileParameters=QJsonObject{{"sketchId",arguments["sketchId"]},{"distanceMm",arguments["distanceMm"]}};
    if(arguments.contains("distanceParameterId"))profileParameters["distanceParameterId"]=arguments["distanceParameterId"];
    const auto start=arguments.value("start").toString(arguments.contains("startOffsetMm") || arguments.contains("startOffsetParameterId") ? "offset" : previousParameters.value("start").toString("profilePlane"));
    if(start!="profilePlane" && start!="offset")reject("Extrusion start must be Profile Plane or Offset");
    if(start=="profilePlane" && (arguments.contains("startOffsetMm") || arguments.contains("startOffsetParameterId")))reject("Profile Plane start cannot carry an offset");
    if(start=="offset"){
        const auto startOffset=arguments.contains("startOffsetMm") ? arguments["startOffsetMm"] : previousParameters.value("startOffsetMm");
        if(startOffset.isUndefined())reject("Offset start requires a signed length");
        profileParameters["start"]="offset";profileParameters["startOffsetMm"]=startOffset;
        if(arguments.contains("startOffsetParameterId") || (edit && !arguments.contains("startOffsetMm") && previousParameters.contains("startOffsetParameterId")))
            profileParameters["startOffsetParameterId"]=arguments.contains("startOffsetParameterId") ? arguments.value("startOffsetParameterId") : previousParameters.value("startOffsetParameterId");
    }
    if(cut) {
        const auto targets=arguments.contains("targetBodyIds") ? arguments["targetBodyIds"] : previousParameters["targetBodyIds"];
        if(!targets.isArray() || targets.toArray().isEmpty() || (operation!="join" && targets.toArray().size()!=1))reject("Join requires participants; Cut/Intersect require one target");
        QSet<QString> chosen;for(const auto& value:targets.toArray()){if(!value.isString() || !validId(value.toString()) || chosen.contains(value.toString()))reject("Join/Cut/Intersect targets must be distinct stable body IDs");chosen.insert(value.toString());}
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
    const QSet<QString> allowed{"ownerId","targetBodyId","edgeIds","radiusMm","name","featureId","newFeatureId","tangentChain"};
    for(auto it=arguments.begin();it!=arguments.end();++it)if(!allowed.contains(it.key()))throw std::invalid_argument("Unknown Fillet argument");
    if(arguments.contains("featureId") && arguments.contains("newFeatureId"))throw std::invalid_argument("Choose existing or new Fillet identity");
    if(arguments.contains("tangentChain") && (!arguments["tangentChain"].isBool() || !arguments["tangentChain"].toBool()))throw std::invalid_argument("Only Tangent Chain On is supported");
    auto candidate=authored;auto features=candidate["features"].toArray();QJsonObject feature;int index=-1;
    if(arguments.contains("featureId")) {
        for(int i=0;i<features.size();++i)if(features[i].toObject()["id"]==arguments["featureId"]){feature=features[i].toObject();index=i;break;}
        if(index<0 || feature["type"]!="fillet")throw std::invalid_argument("Fillet feature unavailable");
    } else feature={{"id",arguments.value("newFeatureId").toString(newId())},{"ownerId",arguments.value("ownerId").toString(authored["documentId"].toString())},{"type","fillet"},{"name","Fillet"},{"visible",true}};
    if(arguments.contains("name"))feature["name"]=arguments["name"];
    feature["parameters"]=QJsonObject{{"targetBodyId",arguments["targetBodyId"]},{"edgeIds",arguments["edgeIds"]},{"radiusMm",arguments["radiusMm"]},{"tangentChain",arguments.value("tangentChain").toBool(true)}};
    if(index>=0)features[index]=feature;else features.append(feature);candidate["features"]=features;
    const auto error=validateStructure(candidate);if(!error.isEmpty())throw std::invalid_argument(error.toStdString());return candidate;
}
QJsonObject CadDocument::chamferCandidate(const QJsonObject& authored,const QJsonObject& arguments) {
    const QSet<QString> allowed{"targetBodyId","edgeIds","distanceMm","name","featureId","newFeatureId","ownerId"};
    for(auto it=arguments.begin();it!=arguments.end();++it)if(!allowed.contains(it.key()))throw std::invalid_argument("Unknown Chamfer argument");
    if(arguments.contains("featureId") && arguments.contains("newFeatureId"))throw std::invalid_argument("Choose existing or new Chamfer identity");
    auto candidate=authored;auto features=candidate["features"].toArray();QJsonObject feature;int index=-1;
    if(arguments.contains("featureId")) {
        for(int i=0;i<features.size();++i)if(features[i].toObject()["id"]==arguments["featureId"]){feature=features[i].toObject();index=i;break;}
        if(index<0 || feature["type"]!="chamfer")throw std::invalid_argument("Chamfer feature unavailable");
    } else feature={{"id",arguments.value("newFeatureId").toString(newId())},{"ownerId",arguments.value("ownerId").toString(authored["documentId"].toString())},{"type","chamfer"},{"name","Chamfer"},{"visible",true}};
    if(arguments.contains("name"))feature["name"]=arguments["name"];
    feature["parameters"]=QJsonObject{{"targetBodyId",arguments["targetBodyId"]},{"edgeIds",arguments["edgeIds"]},{"distanceMm",arguments["distanceMm"]}};
    if(index>=0)features[index]=feature;else features.append(feature);candidate["features"]=features;
    const auto error=validateStructure(candidate);if(!error.isEmpty())throw std::invalid_argument(error.toStdString());return candidate;
}
QJsonObject CadDocument::patternCandidate(const QJsonObject& authored,const QJsonObject& arguments) {
    const QSet<QString> allowed{"sourceFeatureId","targetBodyId","axes","distribution","computeType","name","featureId","newFeatureId","ownerId"};
    for(auto it=arguments.begin();it!=arguments.end();++it)if(!allowed.contains(it.key()))throw std::invalid_argument("Unknown Pattern argument");
    if(arguments.contains("featureId") && arguments.contains("newFeatureId"))throw std::invalid_argument("Choose an existing or new Pattern identity");
    auto candidate=authored;auto features=candidate["features"].toArray();QJsonObject feature;int index=-1;
    if(arguments.contains("featureId")){for(int i=0;i<features.size();++i)if(features[i].toObject()["id"]==arguments["featureId"]){feature=features[i].toObject();index=i;break;}if(index<0 || feature["type"]!="rectangularPattern")throw std::invalid_argument("Pattern feature unavailable");}
    else feature={{"id",arguments.value("newFeatureId").toString(newId())},{"ownerId",arguments.value("ownerId").toString(authored["documentId"].toString())},{"type","rectangularPattern"},{"name","Rectangular Pattern"},{"visible",true}};
    auto p=feature["parameters"].toObject();for(const auto& key:{"sourceFeatureId","targetBodyId","axes","distribution","computeType"})if(arguments.contains(key))p[key]=arguments[key];
    QJsonObject seed;for(const auto& item:features)if(item.toObject()["id"]==p["sourceFeatureId"]){seed=item.toObject();break;}
    if(seed["type"]=="extrude") {
        if((p.contains("distribution") && p["distribution"]!="extent") || (p.contains("computeType") && p["computeType"]!="identical"))
            throw std::invalid_argument("Additive Pattern supports Extent and Identical only");
        p.remove("distribution");p.remove("computeType");
    } else if(seed["type"]=="hole") {
        if(!p.contains("distribution"))p["distribution"]="spacing";
        if(!p.contains("computeType"))p["computeType"]="adjust";
    }
    feature["parameters"]=p;if(arguments.contains("name"))feature["name"]=arguments["name"];
    if(index<0)features.append(feature);else features[index]=feature;candidate["features"]=features;return candidate;
}

QJsonObject CadDocument::sweepCandidate(const QJsonObject& authored,const QJsonObject& arguments) {
    const QSet<QString> allowed{"ownerId","profileSketchId","profileId","pathSketchId","pathEntityIds","pathStartPointId","extent","orientation","operation","name","featureId","newFeatureId","newBodyId"};
    for(auto it=arguments.begin();it!=arguments.end();++it)if(!allowed.contains(it.key()))throw std::invalid_argument("Unknown Sweep argument");
    if(arguments.contains("featureId") && (arguments.contains("newFeatureId") || arguments.contains("newBodyId")))throw std::invalid_argument("Choose existing or reserved Sweep identities");
    if(!arguments.contains("featureId") && arguments.contains("newFeatureId")!=arguments.contains("newBodyId"))throw std::invalid_argument("Reserve Sweep feature and body identities together");
    auto candidate=authored;auto features=candidate["features"].toArray();QJsonObject feature;int index=-1;
    if(arguments.contains("featureId")){for(int i=0;i<features.size();++i)if(features[i].toObject()["id"]==arguments["featureId"]){feature=features[i].toObject();index=i;break;}if(index<0 || feature["type"]!="sweep")throw std::invalid_argument("Sweep feature unavailable");}
    else feature={{"id",arguments.value("newFeatureId").toString(newId())},{"bodyId",arguments.value("newBodyId").toString(newId())},{"ownerId",arguments.value("ownerId").toString(authored["documentId"].toString())},{"type","sweep"},{"name","Sweep"},{"visible",true}};
    feature["parameters"]=QJsonObject{{"profileSketchId",arguments["profileSketchId"]},{"profileId",arguments["profileId"]},{"pathSketchId",arguments["pathSketchId"]},{"pathEntityIds",arguments["pathEntityIds"]},{"pathStartPointId",arguments["pathStartPointId"]},{"extent",arguments.contains("extent")?arguments["extent"]:QJsonValue("full")},{"orientation",arguments.contains("orientation")?arguments["orientation"]:QJsonValue("perpendicular")},{"operation",arguments.contains("operation")?arguments["operation"]:QJsonValue("newBody")}};
    if(arguments.contains("name"))feature["name"]=arguments["name"];if(index<0)features.append(feature);else features[index]=feature;candidate["features"]=features;const auto error=validateStructure(candidate);if(!error.isEmpty())throw std::invalid_argument(error.toStdString());return candidate;
}

QJsonObject CadDocument::revolveCandidate(const QJsonObject& authored,const QJsonObject& arguments) {
    const QSet<QString> allowed{"ownerId","sketchId","profileId","axisEntityId","extent","angleDeg","name","featureId","newFeatureId","newBodyId"};
    for(auto it=arguments.begin();it!=arguments.end();++it)if(!allowed.contains(it.key()))throw std::invalid_argument("Unknown Revolve argument");
    if(arguments.contains("featureId") && (arguments.contains("newFeatureId") || arguments.contains("newBodyId")))throw std::invalid_argument("Choose existing or new Revolve identity");
    if(!arguments.contains("featureId") && arguments.contains("newFeatureId")!=arguments.contains("newBodyId"))throw std::invalid_argument("Reserve feature and body identities together");
    auto candidate=authored;auto features=candidate["features"].toArray();QJsonObject feature;int index=-1;
    if(arguments.contains("featureId")){for(int i=0;i<features.size();++i)if(features[i].toObject()["id"]==arguments["featureId"]){feature=features[i].toObject();index=i;break;}if(index<0 || feature["type"]!="revolve")throw std::invalid_argument("Revolve feature unavailable");}
    else feature={{"id",arguments.value("newFeatureId").toString(newId())},{"bodyId",arguments.value("newBodyId").toString(newId())},{"ownerId",arguments.value("ownerId").toString(authored["documentId"].toString())},{"type","revolve"},{"name","Revolve"},{"visible",true}};
    const auto extent=arguments.value("extent").toString("full");if(arguments.contains("extent") && !arguments["extent"].isString())throw std::invalid_argument("Revolve extent must be a string");if(arguments.contains("angleDeg") && !arguments["angleDeg"].isDouble())throw std::invalid_argument("Revolve angle must be numeric");feature["parameters"]=QJsonObject{{"sketchId",arguments["sketchId"]},{"profileId",arguments["profileId"]},{"axisEntityId",arguments["axisEntityId"]},{"extent",extent},{"angleDeg",arguments.value("angleDeg").toDouble(360)}};
    if(arguments.contains("name"))feature["name"]=arguments["name"];if(index<0)features.append(feature);else features[index]=feature;candidate["features"]=features;const auto error=validateStructure(candidate);if(!error.isEmpty())throw std::invalid_argument(error.toStdString());return candidate;
}

QJsonObject CadDocument::loftCandidate(const QJsonObject& authored,const QJsonObject& arguments) {
    auto candidate=authored;auto features=candidate["features"].toArray();QJsonObject feature;int index=-1;
    if(arguments.contains("featureId")) {
        for(int i=0;i<features.size();++i)if(features[i].toObject()["id"]==arguments["featureId"]){feature=features[i].toObject();index=i;break;}
        if(index<0 || feature["type"]!="loft")throw std::invalid_argument("Loft feature unavailable");
    } else feature={{"id",arguments.value("newFeatureId").toString(newId())},{"bodyId",arguments.value("newBodyId").toString(newId())},{"ownerId",arguments.value("ownerId").toString(authored["documentId"].toString())},{"type","loft"},{"name","Loft"},{"visible",true}};
    feature["parameters"]=QJsonObject{{"profiles",arguments["profiles"]}};
    if(arguments.contains("name"))feature["name"]=arguments["name"];
    if(index<0)features.append(feature);else features[index]=feature;candidate["features"]=features;return candidate;
}

QJsonObject CadDocument::sphereCandidate(const QJsonObject& authored,const QJsonObject& arguments) {
    const QSet<QString> allowed{"support","uMm","vMm","diameterMm","operation","targetBodyId","name","featureId","newFeatureId","newBodyId","ownerId"};
    for(auto it=arguments.begin();it!=arguments.end();++it)if(!allowed.contains(it.key()))throw std::invalid_argument("Unknown Sphere argument");
    if(arguments.contains("featureId") && (arguments.contains("newFeatureId") || arguments.contains("newBodyId")))throw std::invalid_argument("Choose an existing or new Sphere identity");
    auto candidate=authored;auto features=candidate["features"].toArray();QJsonObject feature;int index=-1;
    if(arguments.contains("featureId")) {
        for(int i=0;i<features.size();++i)if(features[i].toObject()["id"]==arguments["featureId"]){feature=features[i].toObject();index=i;break;}
        if(index<0 || feature["type"]!="sphere")throw std::invalid_argument("Sphere feature unavailable");
    } else feature={{"id",arguments.value("newFeatureId").toString(newId())},{"ownerId",arguments.value("ownerId").toString(authored["documentId"].toString())},{"type","sphere"},{"name","Sphere"},{"visible",true}};
    if(arguments.contains("name"))feature["name"]=arguments["name"];
    const auto operation=arguments.value("operation").toString("cut");
    if(index>=0 && feature["parameters"].toObject()["operation"]!=operation)throw std::invalid_argument("Sphere edits preserve Cut or New Body ownership");
    QJsonObject parameters{{"support",arguments["support"]},{"uMm",arguments["uMm"]},{"vMm",arguments["vMm"]},{"diameterMm",arguments["diameterMm"]},{"operation",operation}};
    if(operation=="cut")parameters["targetBodyId"]=arguments["targetBodyId"];
    else if(operation=="newBody" && !feature.contains("bodyId"))feature["bodyId"]=arguments.value("newBodyId").toString(newId());
    feature["parameters"]=parameters;
    if(index>=0)features[index]=feature;else features.append(feature);candidate["features"]=features;
    const auto error=validate(candidate);if(!error.isEmpty())throw std::invalid_argument(error.toStdString());return candidate;
}
QJsonObject CadDocument::holeCandidate(const QJsonObject& authored,const QJsonObject& arguments) {
    const QSet<QString> allowed{"sourceSketchId","sourceEntityId","sourcePointId","targetBodyId","placement","holeType","extent",
        "boreDiameterMm","sinkDiameterMm","includedAngleDeg","name","featureId","newFeatureId","ownerId"};
    for(auto it=arguments.begin();it!=arguments.end();++it)if(!allowed.contains(it.key()))throw std::invalid_argument("Unknown Hole argument");
    if(arguments.contains("featureId") && arguments.contains("newFeatureId"))throw std::invalid_argument("Choose an existing or new Hole identity");
    auto candidate=authored;auto features=candidate["features"].toArray();QJsonObject feature;int index=-1;
    if(arguments.contains("featureId")) {
        for(int i=0;i<features.size();++i)if(features[i].toObject()["id"]==arguments["featureId"]){feature=features[i].toObject();index=i;break;}
        if(index<0 || feature["type"]!="hole")throw std::invalid_argument("Hole feature unavailable");
    } else feature={{"id",arguments.value("newFeatureId").toString(newId())},{"ownerId",arguments.value("ownerId").toString(authored["documentId"].toString())},
        {"type","hole"},{"name","Hole"},{"visible",true}};
    auto p=feature["parameters"].toObject();
    for(const auto& key:{"sourceSketchId","sourceEntityId","sourcePointId","targetBodyId","placement","holeType","extent",
        "boreDiameterMm","sinkDiameterMm","includedAngleDeg"})if(arguments.contains(key))p[key]=arguments[key];
    if(!p.contains("placement"))p["placement"]="fromSketch";
    if(!p.contains("holeType"))p["holeType"]="countersink";
    if(!p.contains("extent"))p["extent"]="all";
    feature["parameters"]=p;if(arguments.contains("name"))feature["name"]=arguments["name"];
    if(index<0)features.append(feature);else features[index]=feature;candidate["features"]=features;
    const auto error=validate(candidate);if(!error.isEmpty())throw std::invalid_argument(error.toStdString());return candidate;
}
QJsonObject CadDocument::shellCandidate(const QJsonObject& authored,const QJsonObject& arguments) {
    const QSet<QString> allowed{"ownerId","targetBodyId","removedFace","thicknessMm","thicknessParameterId","name","featureId","newFeatureId"};
    for(auto it=arguments.begin();it!=arguments.end();++it)if(!allowed.contains(it.key()))throw std::invalid_argument("Unknown Shell argument");
    if(arguments.contains("featureId") && arguments.contains("newFeatureId"))throw std::invalid_argument("Choose existing or new Shell identity");
    auto candidate=authored;auto features=candidate["features"].toArray();QJsonObject feature;int index=-1;
    if(arguments.contains("featureId")) {
        for(int i=0;i<features.size();++i)if(features[i].toObject()["id"]==arguments["featureId"]){feature=features[i].toObject();index=i;break;}
        if(index<0 || feature["type"]!="shell")throw std::invalid_argument("Shell feature unavailable");
    } else feature={{"id",arguments.value("newFeatureId").toString(newId())},{"ownerId",arguments.value("ownerId").toString(authored["documentId"].toString())},{"type","shell"},{"name","Shell"},{"visible",true}};
    if(arguments.contains("name"))feature["name"]=arguments["name"];
    QJsonObject parameters{{"targetBodyId",arguments["targetBodyId"]},{"removedFace",arguments["removedFace"]},{"thicknessMm",arguments["thicknessMm"]}};
    if(arguments.contains("thicknessParameterId"))parameters["thicknessParameterId"]=arguments["thicknessParameterId"];
    feature["parameters"]=parameters;
    if(index>=0)features[index]=feature;else features.append(feature);candidate["features"]=features;
    const auto error=validateStructure(candidate);if(!error.isEmpty())throw std::invalid_argument(error.toStdString());return candidate;
}
QJsonObject CadDocument::circularPatternCandidate(const QJsonObject& authored,const QJsonObject& arguments){
    const QSet<QString> allowed{"ownerId","sourceBodyId","axis","extent","quantity","name","featureId","newFeatureId"};for(auto it=arguments.begin();it!=arguments.end();++it)if(!allowed.contains(it.key()))throw std::invalid_argument("Unknown Circular Pattern argument");
    auto candidate=authored;auto features=candidate["features"].toArray();QJsonObject feature;int index=-1;
    if(arguments.contains("featureId")){for(int i=0;i<features.size();++i)if(features[i].toObject()["id"]==arguments["featureId"]){feature=features[i].toObject();index=i;break;}if(index<0||feature["type"]!="circularPattern")throw std::invalid_argument("Circular Pattern feature unavailable");}
    else feature={{"id",arguments.value("newFeatureId").toString(newId())},{"ownerId",arguments.value("ownerId").toString(authored["documentId"].toString())},{"type","circularPattern"},{"name","Circular Pattern"},{"visible",true}};
    if(arguments.contains("name"))feature["name"]=arguments["name"];
    feature["parameters"]=QJsonObject{{"sourceBodyId",arguments["sourceBodyId"]},{"axis",arguments["axis"]},{"extent",arguments.value("extent").toString("full")},{"quantity",arguments.contains("quantity") ? arguments.value("quantity") : QJsonValue(3)}};
    if(index<0)features.append(feature);else features[index]=feature;candidate["features"]=features;const auto error=validateStructure(candidate);if(!error.isEmpty())throw std::invalid_argument(error.toStdString());return candidate;
}
QJsonObject CadDocument::mirrorCandidate(const QJsonObject& authored,const QJsonObject& arguments) {
    const QSet<QString> allowed{"ownerId","sourceBodyId","mirrorPlane","name","featureId","newFeatureId","newBodyId"};
    for(auto it=arguments.begin();it!=arguments.end();++it)if(!allowed.contains(it.key()))throw std::invalid_argument("Unknown Mirror argument");
    if(arguments.contains("featureId") && (arguments.contains("newFeatureId") || arguments.contains("newBodyId")))throw std::invalid_argument("Choose existing or new Mirror identity");
    auto candidate=authored;auto features=candidate["features"].toArray();QJsonObject feature;int index=-1;
    if(arguments.contains("featureId")){
        for(int i=0;i<features.size();++i)if(features[i].toObject()["id"]==arguments["featureId"]){feature=features[i].toObject();index=i;break;}
        if(index<0 || feature["type"]!="mirror")throw std::invalid_argument("Mirror feature unavailable");
    }else feature={{"id",arguments.value("newFeatureId").toString(newId())},{"ownerId",arguments.value("ownerId").toString(authored["documentId"].toString())},
        {"bodyId",arguments.value("newBodyId").toString(newId())},{"type","mirror"},{"name","Mirror"},{"visible",true}};
    if(arguments.contains("name"))feature["name"]=arguments["name"];
    feature["parameters"]=QJsonObject{{"sourceBodyId",arguments["sourceBodyId"]},{"mirrorPlane",arguments["mirrorPlane"]}};
    if(index>=0)features[index]=feature;else features.append(feature);candidate["features"]=features;
    const auto error=validateStructure(candidate);if(!error.isEmpty())throw std::invalid_argument(error.toStdString());return candidate;
}
QJsonObject CadDocument::moveCandidate(const QJsonObject& authored, const QJsonObject& arguments) {
    const QSet<QString> allowed{"ownerId","targetBodyId","xMm","yMm","zMm","xDeg","yDeg","zDeg","pivotXMm","pivotYMm","pivotZMm","name","featureId","newFeatureId","steps"};
    for(auto it=arguments.begin();it!=arguments.end();++it)if(!allowed.contains(it.key()))throw std::invalid_argument("Unknown Move argument");
    if(arguments.contains("featureId") && arguments.contains("newFeatureId"))throw std::invalid_argument("Choose existing or new Move identity");
    for(const auto& key:{"xMm","yMm","zMm","xDeg","yDeg","zDeg","pivotXMm","pivotYMm","pivotZMm"})if(arguments.contains(key) && !arguments[key].isDouble())throw std::invalid_argument("Move values must be numeric");
    if(arguments.contains("steps") && !arguments["steps"].isArray())throw std::invalid_argument("Move steps must be an array");
    auto candidate=authored;auto features=authored["features"].toArray();QJsonObject feature;int index=-1;
    if(arguments.contains("featureId")) {
        for(int i=0;i<features.size();++i)if(features[i].toObject()["id"]==arguments["featureId"]){feature=features[i].toObject();index=i;break;}
        if(index<0 || feature["type"]!="move")throw std::invalid_argument("Move feature unavailable");
    } else feature={{"id",arguments.value("newFeatureId").toString(newId())},{"ownerId",arguments.value("ownerId").toString(authored["documentId"].toString())},{"type","move"},{"name","Move"},{"visible",true}};
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
    const QSet<QString> allowed{"ownerId","operation","targetBodyId","toolBodyIds","keepTools","name","featureId","newFeatureId"};
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
        {"ownerId",arguments.value("ownerId").toString(authored["documentId"].toString())},{"type","boolean"},{"name","Combine"},{"visible",true}};
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
    try {
        const auto resolved=resolveLengthBindings(candidate);
        const auto structural=validateStructure(resolved);
        return structural.isEmpty() ? dependencyError(resolved)["message"].toString() : structural;
    } catch(const std::exception& error) {return QString::fromUtf8(error.what());}
}
QString CadDocument::validateStructure(const QJsonObject& candidate, QJsonObject* errorContext) {
    if (errorContext) *errorContext = {};
    if (candidate["schema"].toString() != "alloy-document/1") return "Unsupported document schema";
    const auto root = candidate["documentId"].toString();
    if (!validId(root)) return "Invalid document UUID";
    const auto units = candidate["displayUnits"].toString();
    if (units != "mm" && units != "in") return "Unsupported display units";
    if (!candidate["features"].isArray()) return "Features must be an array";
    QSet<QString> ids{root}, owners{root};
    if(candidate.contains("analyses") && !candidate["analyses"].isArray())return "Analyses must be an array";
    int activeSections=0;
    for(const auto& entry:candidate["analyses"].toArray()) {
        const auto row=entry.toObject();const auto id=row["id"].toString(),plane=row["plane"].toString();
        if(!entry.isObject() || row.size()!=6 || !validId(id) || ids.contains(id) || row["type"]!="section"
            || !row["name"].isString() || row["name"].toString().trimmed().isEmpty()
            || (plane!="XY" && plane!="XZ" && plane!="YZ") || !row["visible"].isBool() || !row["hatch"].isBool())return "Invalid section analysis record";
        ids.insert(id);if(row["visible"].toBool())++activeSections;
    }
    if(activeSections>1)return "Only one section analysis may be visible";
    if(candidate.contains("userParameters") && !candidate["userParameters"].isArray())return "User parameters must be an array";
    QSet<QString> names;
    static const QRegularExpression parameterName("^[A-Za-z_][A-Za-z0-9_]*$");
    for(const auto& entry:candidate["userParameters"].toArray()) {
        const auto p=entry.toObject();const auto id=p["id"].toString(),name=p["name"].toString(),units=p["units"].toString();
        const double value=p["valueMm"].toDouble();
        if(!entry.isObject() || p.size()!=4 || !validId(id) || ids.contains(id) || !p["name"].isString()
            || !parameterName.match(name).hasMatch() || names.contains(name) || (units!="mm" && units!="in")
            || !p["valueMm"].isDouble() || !std::isfinite(value) || value<0.001 || value>100000)
            return "User length parameters require unique IDs/names and positive finite mm/in values";
        ids.insert(id);names.insert(name);
    }
    if(candidate.contains("components") && !candidate["components"].isArray())return "Components must be an array";
    if(candidate.contains("occurrences") && !candidate["occurrences"].isArray())return "Occurrences must be an array";
    for(const auto& value:candidate["components"].toArray()) {
        const auto component=value.toObject();const auto id=component["id"].toString();
        if(component.size()!=2 || !validId(id) || ids.contains(id) || !component["name"].isString() || component["name"].toString().trimmed().isEmpty())return "Invalid local component definition";
        ids.insert(id);owners.insert(id);
    }
    QSet<QString> instantiated;
    for(const auto& value:candidate["occurrences"].toArray()) {
        const auto occurrence=value.toObject();const auto id=occurrence["id"].toString(),definition=occurrence["definitionId"].toString();
        if(occurrence.size()!=4 || !validId(id) || ids.contains(id) || definition==root || !owners.contains(definition) || instantiated.contains(definition) || occurrence["parentId"]!=root || occurrence["placement"]!="identity")return "Only one root-child identity occurrence per local component is supported";
        ids.insert(id);instantiated.insert(definition);
    }
    if(instantiated.size()!=owners.size()-1)return "Each local component requires one occurrence";
    for (const auto& item : candidate["features"].toArray()) {
        if (!item.isObject()) return "Feature must be an object";
        const auto feature = item.toObject();
        if (errorContext) *errorContext = {{"featureId",feature["id"]},{"featureType",feature["type"]}};
        const auto id = feature["id"].toString();
        if (!validId(id) || ids.contains(id)) return "Invalid or duplicate authored UUID";
        ids.insert(id);
        if (!owners.contains(feature["ownerId"].toString())) return "Feature owner must be an existing component definition";
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
        if(type=="hole") {
            if(feature.contains("bodyId") || p.size()!=10 || !validId(p["sourceSketchId"].toString()) || !validId(p["sourceEntityId"].toString())
                || !validId(p["sourcePointId"].toString()) || !validId(p["targetBodyId"].toString())
                || p["placement"]!="fromSketch" || p["holeType"]!="countersink" || p["extent"]!="all")
                return "Hole requires one retained sketch point, one target and From Sketch Countersink All";
            for(const auto& key:{"boreDiameterMm","sinkDiameterMm","includedAngleDeg"})if(!p[key].isDouble() || !std::isfinite(p[key].toDouble()))return "Hole dimensions must be finite";
            const auto bore=p["boreDiameterMm"].toDouble(),sink=p["sinkDiameterMm"].toDouble(),angle=p["includedAngleDeg"].toDouble();
            if(bore<0.001 || bore>100000 || sink<=bore || sink>100000 || angle<=1 || angle>=179)
                return "Hole requires positive bore, larger sink and included angle within (1,179) degrees";
            const auto depth=(sink-bore)/(2*std::tan(angle*std::numbers::pi/360));
            if(!std::isfinite(depth) || depth<0.001 || depth>100000)return "Hole countersink depth is outside supported bounds";
            continue;
        }
        if(type=="chamfer") {
            if(feature.contains("bodyId") || p.size()!=3 || !validId(p["targetBodyId"].toString()) || !p["edgeIds"].isArray() || p["edgeIds"].toArray().isEmpty() || p["edgeIds"].toArray().size()>100)return "Chamfer requires one target, edge references";
            QSet<QString> edges;for(const auto& edge:p["edgeIds"].toArray()){if(!edge.isString() || !validId(edge.toString()) || edges.contains(edge.toString()))return "Chamfer edge references must be distinct stable UUIDs";edges.insert(edge.toString());}
            if(!p["distanceMm"].isDouble() || !std::isfinite(p["distanceMm"].toDouble()) || p["distanceMm"].toDouble()<0.001 || p["distanceMm"].toDouble()>100000)return "Chamfer distance must be 0.001..100000 mm";
            continue;
        }
        if(type=="rectangularPattern") {
            const bool hole=p.contains("distribution") || p.contains("computeType");
            const auto axisCount=p["axes"].toArray().size();
            if(feature.contains("bodyId") || p.size()!=(hole ? 5:3) || !validId(p["sourceFeatureId"].toString()) || !validId(p["targetBodyId"].toString())
                || !p["axes"].isArray() || (hole ? axisCount!=2 : axisCount<1 || axisCount>2))
                return "Pattern requires one seed, target body and supported axes";
            if(hole && (p["distribution"]!="spacing" || p["computeType"]!="adjust"))return "Hole Pattern requires Spacing and Adjust";
            QSet<QString> edges;int total=1;for(const auto& value:p["axes"].toArray()){
                const auto axis=value.toObject();const auto quantityValue=axis["quantity"];
                const auto quantity=quantityValue.toDouble(),distance=axis[hole ? "spacingMm":"extentMm"].toDouble();
                if(!value.isObject() || axis.size()!=(hole ? 4:3) || !validId(axis["edgeId"].toString()) || edges.contains(axis["edgeId"].toString())
                    || !quantityValue.isDouble() || quantity!=std::floor(quantity) || quantity<2 || quantity>100
                    || !axis[hole ? "spacingMm":"extentMm"].isDouble() || !std::isfinite(distance) || std::abs(distance)<0.001 || std::abs(distance)>100000
                    || (hole && axis["direction"]!="one"))return "Pattern axes require distinct edges, quantity2–100 and signed nonzero distance";
                edges.insert(axis["edgeId"].toString());total*=int(quantity);
            }if(total>1000)return "Pattern exceeds 1000 instances";
            continue;
        }
        if(type=="sweep") {
            if(!validId(feature["bodyId"].toString()) || ids.contains(feature["bodyId"].toString()) || p.size()!=8)return "Sweep requires a distinct body and exact supported parameters";
            for(const auto& key:{"profileSketchId","profileId","pathSketchId","pathStartPointId"})if(!p[key].isString() || !validId(p[key].toString()))return "Sweep requires stable profile/path/start identities";
            if(!p["pathEntityIds"].isArray() || p["pathEntityIds"].toArray().isEmpty() || p["pathEntityIds"].toArray().size()>1000)return "Sweep requires an ordered retained path";
            QSet<QString> path;for(const auto& id:p["pathEntityIds"].toArray()){if(!id.isString() || !validId(id.toString()) || path.contains(id.toString()))return "Sweep path entities must be distinct UUIDs";path.insert(id.toString());}
            if(p["extent"]!="full" || p["orientation"]!="perpendicular" || p["operation"]!="newBody")return "Sweep supports Full Perpendicular New Body only";
            ids.insert(feature["bodyId"].toString());continue;
        }
        if(type=="revolve") {
            if(!validId(feature["bodyId"].toString()) || ids.contains(feature["bodyId"].toString()) || p.size()!=5 || !validId(p["sketchId"].toString()) || !validId(p["profileId"].toString()) || !validId(p["axisEntityId"].toString()))return "Revolve requires a distinct body and stable sketch/profile/axis identities";
            ids.insert(feature["bodyId"].toString());const auto angle=p["angleDeg"].toDouble();if(!p["angleDeg"].isDouble() || !std::isfinite(angle) || (p["extent"]!="full" && p["extent"]!="partial") || (p["extent"]=="full" ? angle!=360 : angle<.001 || angle>=360))return "Revolve requires Full360 or Partial angle0.001..<360 degrees";continue;
        }
        if(type=="loft") {
            if(!validId(feature["bodyId"].toString()) || ids.contains(feature["bodyId"].toString()) || p.size()!=1 || !p["profiles"].isArray() || p["profiles"].toArray().size()<2 || p["profiles"].toArray().size()>32)return "Loft requires a distinct body ID and 2–32 ordered profile references";
            ids.insert(feature["bodyId"].toString());QSet<QString> profiles;
            for(const auto& value:p["profiles"].toArray()){const auto ref=value.toObject();const auto key=ref["sketchId"].toString()+"/"+ref["profileId"].toString();if(ref.size()!=2 || !validId(ref["sketchId"].toString()) || !validId(ref["profileId"].toString()) || profiles.contains(key))return "Loft profiles must be distinct stable sketch/profile references";profiles.insert(key);}
            continue;
        }
        if(type=="sphere") {
            const auto support=p["support"].toObject();
            if(support.size()!=3 || !validId(support["bodyId"].toString()) || !validId(support["faceId"].toString()) || !validId(support["producerFeatureId"].toString()))return "Sphere requires a stable planar face reference";
            for(const auto& key:{"uMm","vMm","diameterMm"})if(!p[key].isDouble() || !std::isfinite(p[key].toDouble()) || std::abs(p[key].toDouble())>100000)return "Invalid Sphere dimension";
            if(p["diameterMm"].toDouble()<0.001)return "Sphere diameter must be positive";
            if(p["operation"]=="cut") {
                if(p.size()!=6 || feature.contains("bodyId") || p["targetBodyId"]!=support["bodyId"])return "Sphere Cut must target the supported body";
            } else if(p["operation"]=="newBody") {
                const auto body=feature["bodyId"].toString();
                if(p.size()!=5 || !validId(body) || ids.contains(body))return "Sphere New Body requires a distinct stable body identity";
                ids.insert(body);
            } else return "Sphere supports Cut or New Body";
            continue;
        }
        if(type=="shell") {
            if(feature.contains("bodyId") || p.size()!=3+int(p.contains("thicknessParameterId")) || !validId(p["targetBodyId"].toString()) || !p["removedFace"].isObject())return "Shell requires one active target body and a stable face reference";
            if(p.contains("thicknessParameterId") && !validId(p["thicknessParameterId"].toString()))return "Shell thickness binding requires a stable parameter ID";
            const auto face=p["removedFace"].toObject();
            if(face.size()!=3 || face["bodyId"]!=p["targetBodyId"] || !validId(face["producerFeatureId"].toString()) || !validId(face["bodyId"].toString()) || !validId(face["faceId"].toString()))return "Shell requires one stable face on its target body";
            if(!p["thicknessMm"].isDouble() || !std::isfinite(p["thicknessMm"].toDouble()) || p["thicknessMm"].toDouble()<0.001 || p["thicknessMm"].toDouble()>100000)return "Shell thickness must be 0.001..100000 mm";
            continue;
        }
        if(type=="circularPattern"){
            const auto axis=p["axis"].toObject();const double q=p["quantity"].toDouble();
            if(feature.contains("bodyId")||p.size()!=4||!validId(p["sourceBodyId"].toString())||axis.size()!=2||axis["kind"]!="originAxis"||axis["axis"]!="Z"||p["extent"]!="full"||!p["quantity"].isDouble()||q!=p["quantity"].toInt()||q<2||q>128)return "Circular Pattern requires one retained body, Origin Z, Full and total quantity2..128";
            for(int n=1;n<int(q);++n){const auto body=CircularPattern::bodyId(id,p["sourceBodyId"].toString(),n);if(ids.contains(body))return "Duplicate circular occurrence UUID";ids.insert(body);}continue;
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
        const bool cut=type=="extrude" && (p["operation"]=="cut" || p["operation"]=="join" || p["operation"]=="intersect");
        if(cut) {
            if(feature.contains("bodyId"))return "Extrude Join/Cut/Intersect modifies a target and cannot own a new body ID";
            if(!p["targetBodyIds"].isArray() || p["targetBodyIds"].toArray().isEmpty() || ((p["operation"]=="cut" || !feature.contains("bodyResults")) && p["targetBodyIds"].toArray().size()!=1))return "Join requires participants; Cut/Intersect and legacy Join require one target";
            QSet<QString> targets;for(const auto& value:p["targetBodyIds"].toArray()){if(!value.isString() || !validId(value.toString()) || targets.contains(value.toString()))return "Join/Cut/Intersect targets must be distinct stable body IDs";targets.insert(value.toString());}
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
            if (p.size()!=(cut ? 5 : 3)+int(p.contains("distanceParameterId"))+int(p.contains("start"))+int(p.contains("startOffsetMm"))+int(p.contains("startOffsetParameterId")) || !p["sketchId"].isString() || !validId(p["sketchId"].toString())
                || p.contains("profileId")==p.contains("profileIds")) return "Extrusion requires sketchId and exclusive profileId or profileIds";
            if(p.contains("distanceParameterId") && !validId(p["distanceParameterId"].toString()))return "Extrusion length binding requires a stable parameter ID";
            if((p.contains("start") && (p["start"]!="offset" || !p.contains("startOffsetMm"))) || (!p.contains("start") && (p.contains("startOffsetMm") || p.contains("startOffsetParameterId"))))return "Extrusion start must be Profile Plane or Offset with a signed length";
            if(p.contains("startOffsetParameterId") && (!p.contains("startOffsetMm") || !validId(p["startOffsetParameterId"].toString())))return "Extrusion start binding requires a stable parameter ID and cached offset";
            if(p.contains("startOffsetMm") && (!p["startOffsetMm"].isDouble() || !std::isfinite(p["startOffsetMm"].toDouble()) || std::abs(p["startOffsetMm"].toDouble())>100000))return "Extrusion start offset must be signed and within 100000 mm";
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
        int bindings=0;
        if(type=="box")for(const auto& field:QStringList{"width","depth","height"})if(p.contains(field+"ParameterId")) {
            if(!validId(p[field+"ParameterId"].toString()))return "Box dimension binding requires a stable parameter ID";
            ++bindings;
        }
        if (p.size() != dimensions.size() + 3 + bindings) return "Unknown feature parameter";
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
    for(const auto& plane:evaluation_->constructionPlanes)evaluatedPlanes.append(QJsonObject{{"ownerId",plane.ownerId},{"featureId",plane.featureId},{"name",plane.name},{"visible",plane.visible},{"frame",plane.frame}});
    return {{"activeOwnerId",activeOwnerId()},{"components",authored_["components"].toArray()},{"occurrences",authored_["occurrences"].toArray()},{"userParameters",authored_["userParameters"].toArray()},{"loaded", true}, {"id", authored_["documentId"]}, {"label", path_.isEmpty() ? "Untitled" : QFileInfo(path_).completeBaseName()}, {"path",path_},
        {"units", authored_["displayUnits"]}, {"canonicalLengthUnit", "mm"},
        {"revision", static_cast<qint64>(revision_)}, {"dirty", !history_.isClean()},
        {"lastTransactionId", lastTransaction_},
        {"features", authored_["features"].toArray().size()}, {"bodies",static_cast<int>(evaluation_->bodies.size())}, {"sketches", static_cast<int>(evaluation_->sketches.size())},
        {"evaluation",QJsonObject{{"status","current"},{"recompute",evaluation_->recompute},{"featureOrder",order},{"joinBodyResults",evaluation_->joinBodyResults},
            {"documentRevision",static_cast<qint64>(revision_)},{"bodies",evaluated},{"sketches",evaluatedSketches},{"constructionPlanes",evaluatedPlanes}}},
        {"editableHistory", true}, {"authored", authored_},
        {"undo", QJsonObject{{"canUndo", history_.canUndo()}, {"canRedo", history_.canRedo()},
            {"count", history_.count()}, {"index", history_.index()},
            {"undoText", history_.undoText()}, {"redoText", history_.redoText()}}}};
}
