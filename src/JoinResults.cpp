// SPDX-License-Identifier: MIT
#include "GeometryKernel.h"
#include "SolidFillet.h"
#include "PlanarFaces.h"
#include <memory>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <BRepAlgoAPI_BooleanOperation.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <QUuid>
#include <QSet>
#include <algorithm>
#include <cmath>

namespace {
void reject(const QString& code,const QString& message){throw GeometryKernel::EvaluationError({{"code",code},{"message",message}});}
QJsonArray array(QStringList values){QJsonArray result;for(const auto& value:values)result.append(value);return result;}
QStringList strings(const QJsonArray& values){QStringList result;for(const auto& value:values)result.append(value.toString());return result;}
TopoDS_Shape fuse(const std::vector<TopoDS_Shape>& inputs, std::unique_ptr<BRepAlgoAPI_BooleanOperation>* history=nullptr){
    if(inputs.empty())reject("join_input_invalid","Join requires material inputs");if(inputs.size()==1)return inputs[0];
    TopTools_ListOfShape objects,tools;objects.Append(inputs[0]);for(size_t i=1;i<inputs.size();++i)tools.Append(inputs[i]);
    auto owned=std::make_unique<BRepAlgoAPI_BooleanOperation>();auto& operation=*owned;operation.SetArguments(objects);operation.SetTools(tools);operation.SetOperation(BOPAlgo_FUSE);
    operation.SetNonDestructive(true);operation.SetRunParallel(false);operation.SetFuzzyValue(0.0);operation.Build();
    if(!operation.IsDone() || operation.HasErrors())reject("join_kernel_failed","OCCT could not unite the Join inputs");
    operation.SimplifyResult(true,true,1e-12);const auto shape=operation.Shape();if(history)*history=std::move(owned);return shape;
}
struct CoverageVolume {double volume,error;};
CoverageVolume coverageVolume(const TopoDS_Shape& shape){
    GProp_GProps properties;
    const double error=BRepGProp::VolumePropertiesGK(shape,properties,1e-11,true,false);
    const double volume=std::abs(properties.Mass());
    if(!std::isfinite(error)||error<0||error>1e-8||!std::isfinite(volume))
        reject("join_lineage_failed","Adaptive Join coverage integration did not converge");
    return {volume,error};
}
bool covers(const TopoDS_Shape& result,const TopoDS_Shape& input){
    // The two-shape constructor runs immediately, before immutability flags.
    TopTools_ListOfShape objects,tools;objects.Append(result);tools.Append(input);
    BRepAlgoAPI_Common shared;shared.SetArguments(objects);shared.SetTools(tools);
    shared.SetNonDestructive(true);shared.SetRunParallel(false);shared.Build();
    if(!shared.IsDone() || shared.HasErrors() || shared.Shape().IsNull() || !BRepCheck_Analyzer(shared.Shape()).IsValid())reject("join_lineage_failed","OCCT could not verify Join component lineage");
    // Use the same adaptive authority even when a Boolean replaces the exact
    // offset-edge representation; summarize dispatch is topology-dependent.
    if(!TopExp_Explorer(shared.Shape(),TopAbs_SOLID).More())return false;
    const auto inputVolume=coverageVolume(input),sharedVolume=coverageVolume(shared.Shape());
    if(sharedVolume.volume<=1e-7)return false;
    const double uncertainty=inputVolume.volume*inputVolume.error+sharedVolume.volume*sharedVolume.error;
    if(std::abs(sharedVolume.volume-inputVolume.volume)+uncertainty
        >std::max(1e-6,inputVolume.volume*1e-8))
        reject("join_lineage_ambiguous","A connected Join input does not map wholly to one output solid");
    return true;
}
void supportSummary(EvaluatedBody& body){QJsonArray descriptions;
    for(const auto& cap:body.supportFaces)descriptions.append(QJsonObject{{"faceId",cap.faceId},{"bodyId",cap.bodyId},
        {"producerFeatureId",cap.producerFeatureId},{"role",cap.role},{"planar",true},{"frame",cap.frame},{"edges",GeometryKernel::supportEdgeDescriptors(cap)}});
    if(!descriptions.isEmpty())body.summary["supportFaces"]=descriptions;
}
}

GeometryKernel::JoinResult GeometryKernel::joinResults(const QJsonObject& feature,const std::vector<EvaluatedBody>& participants,
    const std::vector<EvaluatedProfile>& profiles){
    if(participants.empty() || profiles.empty())reject("join_input_invalid","Join needs earlier participants and selected profiles");
    const auto id=feature["id"].toString();const auto parameters=feature["parameters"].toObject();const double distance=parameters["distanceMm"].toDouble();
    std::vector<TopoDS_Shape> inputs,tools;std::vector<EvaluatedSupportFace> caps;std::vector<EvaluatedSolidEdge> edges;
    for(const auto& body:participants){const auto summary=summarize(body.shape);
        if(!summary["valid"].toBool() || !summary["closed"].toBool() || summary["topology"].toObject()["solids"].toInt()!=1)
            reject("join_input_invalid","Every Join participant must be one valid closed solid");inputs.push_back(body.shape);edges.insert(edges.end(),body.solidEdges.begin(),body.solidEdges.end());}
    for(const auto& profile:profiles){EvaluatedSupportFace cap;std::vector<EvaluatedSolidEdge> generated;auto shape=extrusion(profile,distance,&cap,&generated,id);edges.insert(edges.end(),generated.begin(),generated.end());inputs.push_back(shape);tools.push_back(shape);caps.push_back(cap);}
    std::unique_ptr<BRepAlgoAPI_BooleanOperation> history;const auto material=fuse(inputs,&history);TopTools_IndexedMapOfShape solids;TopExp::MapShapes(material,TopAbs_SOLID,solids);
    if(solids.IsEmpty())reject("join_empty_result","Join produced no connected solids");
    TopTools_IndexedMapOfShape componentFaces,componentEdges,componentVertices;
    for(int i=1;i<=solids.Extent();++i){TopExp::MapShapes(solids(i),TopAbs_FACE,componentFaces);TopExp::MapShapes(solids(i),TopAbs_EDGE,componentEdges);TopExp::MapShapes(solids(i),TopAbs_VERTEX,componentVertices);}
    for(const auto& item:std::vector<std::pair<TopAbs_ShapeEnum,int>>{{TopAbs_FACE,componentFaces.Extent()},{TopAbs_EDGE,componentEdges.Extent()},{TopAbs_VERTEX,componentVertices.Extent()}}){
        TopTools_IndexedMapOfShape present;TopExp::MapShapes(material,item.first,present);if(present.Extent()!=item.second)reject("join_mixed_result","Join produced geometry outside its closed result solids");}
    JoinResult result;std::vector<int> participantUses(participants.size()),profileUses(profiles.size());bool participatingAddition=false;
    struct Group {EvaluatedBody body;QJsonObject record;};std::vector<Group> groups;
    for(int i=1;i<=solids.Extent();++i){const auto shape=solids(i);auto summary=summarize(shape);
        if(!summary["valid"].toBool() || !summary["closed"].toBool() || summary["volumeMm3"].toDouble()<=0)
            reject("join_result_invalid","Every Join output must be a valid closed positive-volume solid");
        // FUSE is the union of every explicit solid input. With one validated
        // result its ownership destination is unique; repeating Common for
        // each input adds no grouping information. Multiple results still
        // require the full coverage checks below.
        const bool singleResult=solids.Extent()==1;
        QStringList targets,profileIds,entityIds;std::vector<TopoDS_Shape> prior;std::vector<EvaluatedSupportFace> candidates;
        const EvaluatedBody* primary=nullptr;double sourceArea=0;
        for(size_t j=0;j<participants.size();++j)if(singleResult || covers(shape,participants[j].shape)){
            ++participantUses[j];targets.append(participants[j].bodyId);prior.push_back(participants[j].shape);
            if(!primary)primary=&participants[j];candidates.insert(candidates.end(),participants[j].supportFaces.begin(),participants[j].supportFaces.end());}
        for(size_t j=0;j<profiles.size();++j)if(singleResult || covers(shape,tools[j])){
            ++profileUses[j];profileIds.append(profiles[j].profileId);sourceArea+=profiles[j].summary["areaMm2"].toDouble();
            entityIds.append(strings(profiles[j].summary["sourceEntityIds"].toArray()));
            if(!caps[j].face.IsNull()){auto cap=caps[j];cap.producerFeatureId=id;
                const auto key=QByteArray("alloy-face/extrude-end-cap/1")+(profiles.size()==1 ? QByteArray{} : QByteArray("\n")+profiles[j].profileId.toUtf8());
                cap.faceId=QUuid::createUuidV5(QUuid(id),key).toString(QUuid::WithoutBraces);candidates.push_back(cap);}}
        if(profileIds.isEmpty())reject("join_nonparticipant","An explicitly selected Join participant is untouched by the profile material");
        profileIds.sort();entityIds.removeDuplicates();entityIds.sort();
        const double priorVolume=prior.empty() ? 0 : summarize(fuse(prior))["volumeMm3"].toDouble();
        const double added=summary["volumeMm3"].toDouble()-priorVolume;
        participatingAddition|=primary && added>std::max(1e-6,priorVolume*1e-12);
        const auto bodyId=primary ? primary->bodyId : QUuid::createUuidV5(QUuid(id),QByteArray("alloy-join-body/1\n")+profileIds.join('\n').toUtf8()).toString(QUuid::WithoutBraces);
        QJsonObject record{{"bodyId",bodyId},{"participantBodyIds",array(targets)},{"profileIds",array(profileIds)}};
        EvaluatedBody body=primary ? *primary : EvaluatedBody{id,bodyId,feature["name"].toString()+" · detached",feature["visible"].toBool(),{},{},{}};
        body.shape=shape;body.supportFaces.clear();body.summary=summary;
        body.solidEdges=SolidFillet::transferEdges(*history,edges,shape);body.summary["solidEdges"]=SolidFillet::descriptors(body.solidEdges);
        body.summary["operation"]="join";body.summary["lastModifierFeatureId"]=id;body.summary["producerFeatureId"]=body.featureId;
        body.summary["targetBodyIds"]=parameters["targetBodyIds"];body.summary["participantBodyIds"]=array(targets);
        body.summary["sourceSketchId"]=parameters["sketchId"];body.summary["sourceProfileIds"]=array(profileIds);
        if(profileIds.size()==1)body.summary["sourceProfileId"]=profileIds[0];
        body.summary["sourceEntityIds"]=array(entityIds);body.summary["sourceAreaMm2"]=sourceArea;
        body.summary["distanceMm"]=distance;body.summary["addedVolumeMm3"]=added;
        body.supportFaces=PlanarFaces::transfer(*history,candidates,shape);
        for(auto& face:body.supportFaces)face.bodyId=bodyId;
        supportSummary(body);groups.push_back({body,record});
    }
    if(!participatingAddition)reject("join_no_addition","Join must add material to at least one existing participant");
    for(const auto uses:participantUses)if(uses!=1)reject("join_lineage_ambiguous","A Join participant has no unique connected result");
    for(const auto uses:profileUses)if(uses!=1)reject("join_lineage_ambiguous","A selected profile prism has no unique connected result");
    // Stored ownership is the identity boundary. Recompute may move material,
    // but may not silently split/merge participant groups or replace a detached
    // source group with another body carrying an old UUID.
    const auto accepted=feature["bodyResults"].toArray();
    if(!accepted.isEmpty()){
        if(accepted.size()!=int(groups.size()))reject("join_result_changed","Join connectivity changed; repair its body-result references explicitly");
        for(auto& group:groups){bool matched=false;
            for(const auto& value:accepted){const auto old=value.toObject();
                if(old["participantBodyIds"]!=group.record["participantBodyIds"])continue;
                if(old["participantBodyIds"].toArray().isEmpty() && old["profileIds"]!=group.record["profileIds"])continue;
                if(!old["participantBodyIds"].toArray().isEmpty() && old["bodyId"]!=group.record["bodyId"])continue;
                matched=true;group.body.bodyId=old["bodyId"].toString();group.record["bodyId"]=old["bodyId"];
                for(auto& cap:group.body.supportFaces)cap.bodyId=group.body.bodyId;supportSummary(group.body);break;}
            if(!matched)reject("join_result_changed","Join participant or detached profile grouping changed; repair its body-result references explicitly");}
    }
    std::sort(groups.begin(),groups.end(),[&](const auto& a,const auto& b){
        const auto ai=parameters["targetBodyIds"].toArray().contains(a.body.bodyId),bi=parameters["targetBodyIds"].toArray().contains(b.body.bodyId);
        if(ai!=bi)return ai;return a.body.bodyId<b.body.bodyId;});
    QSet<QString> surviving;for(const auto& group:groups){result.ownership.append(group.record);surviving.insert(group.body.bodyId);}
    QJsonArray consumed;for(const auto& participant:participants)if(!surviving.contains(participant.bodyId))consumed.append(participant.bodyId);
    for(auto& group:groups){group.body.summary["bodyResults"]=result.ownership;group.body.summary["consumedBodyIds"]=consumed;result.bodies.push_back(std::move(group.body));}
    return result;
}
