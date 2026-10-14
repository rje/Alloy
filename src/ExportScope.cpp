#include "ExportScope.h"
#include "LocalOccurrences.h"
#include <QSet>
#include <stdexcept>

namespace {
[[noreturn]] void reject(const char* code,const QString& message){
    throw GeometryKernel::EvaluationError({{"code",code},{"message",message}});
}
QString occurrenceName(const QJsonObject& authored,const QString& id){
    for(const auto& v:LocalOccurrences::inspect(authored,EvaluatedDocument{})["occurrences"].toArray())
        if(v.toObject()["occurrenceId"]==id)return v.toObject()["path"].toString();
    return id;
}
}
ExportScope::Selection ExportScope::select(const QJsonObject& authored,
    const std::vector<EvaluatedBody>& placed,const QJsonObject& request){
    const auto scope=request.value("occurrenceId").toString();
    const auto bodyId=request.value("bodyId").toString();
    const auto ids=request.value("bodyIds").toArray();
    const bool all=request.value("allVisible").toBool();
    if(!scope.isEmpty() && (!ids.isEmpty() || request.contains("allVisible")))
        reject("invalid_body_selection","OccurrenceId may only qualify bodyId or select its visible subtree");
    if(int(!scope.isEmpty() && bodyId.isEmpty())+int(!bodyId.isEmpty())+int(!ids.isEmpty())+int(all)!=1)
        reject("invalid_body_selection","Choose one occurrence, body, body list, or all visible bodies");
    if(!scope.isEmpty()){
        bool exists=false;for(const auto& v:authored["occurrences"].toArray())
            exists|=v.toObject()["id"].toString()==scope;
        if(!exists)reject("invalid_component","Component occurrence does not exist");
    }
    QSet<QString> requested;
    for(const auto& v:ids){if(!v.isString()||v.toString().isEmpty()||requested.contains(v.toString()))
        reject("invalid_body_selection","Body IDs must be distinct nonempty strings");requested.insert(v.toString());}
    QSet<QString> descendants;
    if(!scope.isEmpty())for(const auto& v:authored["occurrences"].toArray()){
        const auto id=v.toObject()["id"].toString();QString cursor=id;
        while(!cursor.isEmpty()){
            if(cursor==scope){descendants.insert(id);break;}
            cursor=LocalOccurrences::parentOccurrenceId(authored,cursor);
        }
    }
    Selection result;
    result.label=all ? "All visible bodies" : !bodyId.isEmpty() ? "Body" : !ids.isEmpty() ? "Selected bodies" : occurrenceName(authored,scope);
    QSet<QString> found,instances;QHash<QString,int> selectedCounts;
    for(const auto& body:placed){
        const bool chosen=all ? body.visible : !bodyId.isEmpty() ? body.bodyId==bodyId && (scope.isEmpty()||body.occurrenceId==scope)
            : !ids.isEmpty() ? requested.contains(body.bodyId) : body.visible && descendants.contains(body.occurrenceId);
        if(!chosen)continue;
        const auto key=body.occurrenceId+":"+body.bodyId;
        if(instances.contains(key))reject("invalid_body_selection","Duplicate placed body instance");
        instances.insert(key);found.insert(body.bodyId);++selectedCounts[body.bodyId];result.bodies.push_back(body);
        result.instances.append(QJsonObject{{"occurrenceId",body.occurrenceId},{"definitionId",body.definitionId},
            {"bodyId",body.bodyId},{"boundsMm",body.summary["boundsMm"]}});
    }
    if(!bodyId.isEmpty() && result.bodies.size()>1)reject("ambiguous_body","Specify occurrenceId for a linked body");
    if(!ids.isEmpty())for(auto it=selectedCounts.begin();it!=selectedCounts.end();++it)
        if(it.value()>1)reject("ambiguous_body","A selected body has linked instances; choose an occurrence or all visible bodies");
    if(!bodyId.isEmpty() && result.bodies.empty() || !ids.isEmpty() && found!=requested)
        reject("stale_body","A selected solid no longer exists");
    if(result.bodies.empty())reject("solid_required","Select at least one committed visible solid");
    return result;
}
