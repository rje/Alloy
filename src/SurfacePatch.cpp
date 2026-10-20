// SPDX-License-Identifier: MIT
#include "SurfacePatch.h"
#include "PlanarFaces.h"
#include "SolidFillet.h"
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepAlgoAPI_Check.hxx>
#include <BRepGProp.hxx>
#include <BRep_Tool.hxx>
#include <GProp_GProps.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <QUuid>
#include <cmath>

EvaluatedBody SurfacePatch::evaluate(const QJsonObject& feature,const EvaluatedDocument& prior) {
    const auto p=feature.value("parameters").toObject();const auto featureId=feature.value("id").toString();
    const auto sketchId=p.value("sketchId").toString(),profileId=p.value("profileId").toString();
    auto reject=[&](const QString& code,const QString& message){auto d=p;d["featureId"]=featureId;d["featureType"]="surfacePatch";d["code"]=code;d["message"]=message;throw GeometryKernel::EvaluationError(d);};
    const EvaluatedSketch* sketch=nullptr;for(const auto& s:prior.sketches)if(s.featureId==sketchId)sketch=&s;
    if(!sketch)reject("surface_source_unavailable","Patch requires an earlier solved planar sketch");
    if(sketch->profiles.size()!=1 || sketch->profiles.front().profileId!=profileId)reject("patch_profile_unavailable","Select exactly one closed profile in a single-loop sketch");
    const auto& profile=sketch->profiles.front();
    if(profile.summary.value("type")!="closed_line_profile" || profile.summary.value("innerLoopCount").toInt()!=0 || profile.boundaryEdges.size()<3)
        reject("unsupported_patch_profile","Patch currently requires one simple closed ordinary-Line loop without holes or arrangement cells");
    QSet<QString> sources;
    for(const auto& boundary:profile.boundaryEdges){
        if(boundary.type!="line" || boundary.sourceEntityId.isEmpty() || sources.contains(boundary.sourceEntityId))reject("unsupported_patch_profile","Patch boundaries must retain distinct ordinary Lines");
        bool ordinary=false;for(const auto& q:sketch->summary.value("entities").toArray()){const auto e=q.toObject();if(e.value("id")==boundary.sourceEntityId)ordinary=e.value("type")=="line" && !e.value("construction").toBool();}
        if(!ordinary)reject("unsupported_patch_profile","Patch boundary source must be an ordinary Line");sources.insert(boundary.sourceEntityId);
    }
    EvaluatedBody body;body.featureId=featureId;body.bodyId=feature.value("bodyId").toString();body.name=feature.value("name").toString();body.visible=feature.value("visible").toBool(true);body.shape=profile.face;body.summary=GeometryKernel::summarize(body.shape);
    TopTools_IndexedMapOfShape wires;TopExp::MapShapes(body.shape,TopAbs_WIRE,wires);const int wireCount=wires.Extent();
    const auto topology=body.summary.value("topology").toObject();const auto area=body.summary.value("areaMm2").toDouble();const BRepAdaptor_Surface surface(profile.face);
    if(!body.summary.value("valid").toBool() || topology.value("faces").toInt()!=1 || topology.value("solids").toInt()!=0 || (wireCount!=1 || !BRep_Tool::IsClosed(wires(1))) || topology.value("edges").toInt()!=sources.size() || surface.GetType()!=GeomAbs_Plane || !std::isfinite(area) || area<=1e-12 || !body.summary.value("volumeMm3").isNull())reject("patch_invalid","Patch must produce one valid positive-area planar face, one wire and zero solids");
    BRepAlgoAPI_Check interference(body.shape,false,true);if(!interference.IsValid())reject("patch_self_intersection","Patch boundary has geometric self-interference");
    auto xyz=[](const auto& q){return QJsonArray{q.X(),q.Y(),q.Z()};};
    auto stable=[&](const QByteArray& role){return QUuid::createUuidV5(QUuid(featureId),QByteArray("alloy-surface-patch/1/")+sketchId.toUtf8()+"/"+profileId.toUtf8()+"/"+role).toString(QUuid::WithoutBraces);};
    QJsonObject provenance{{"generatorFeatureId",featureId},{"sourceSketchId",sketchId},{"sourceProfileId",profileId},{"lineage","surfacePatchProfile"}};
    const auto plane=surface.Plane();auto u=plane.Position().XDirection(),v=plane.Position().YDirection();auto n=u.Crossed(v);if(profile.face.Orientation()==TopAbs_REVERSED){v.Reverse();n.Reverse();}
    EvaluatedSupportFace support{stable("face"),body.bodyId,featureId,"surfacePatch.sheet",profile.face,{{"origin",xyz(plane.Location())},{"uAxis",xyz(u)},{"vAxis",xyz(v)},{"normal",xyz(n)}},{},provenance};
    TopTools_IndexedMapOfShape retained;double perimeter=0;
    for(const auto& boundary:profile.boundaryEdges){
        if(retained.Contains(boundary.edge))reject("patch_lineage_unavailable","Patch boundary sources must identify distinct exact edges");retained.Add(boundary.edge);
        const BRepAdaptor_Curve curve(boundary.edge);const auto a=curve.Value(curve.FirstParameter()),b=curve.Value(curve.LastParameter());
        if(curve.GetType()!=GeomAbs_Line || plane.Distance(a)>1e-7 || plane.Distance(b)>1e-7 || a.Distance(b)<=1e-7)reject("patch_nonplanar_or_degenerate","Patch requires nondegenerate coplanar Line boundaries within 1e-7 mm");
        auto detail=provenance;detail["sourceEntityId"]=boundary.sourceEntityId;detail["startMm"]=xyz(a);detail["endMm"]=xyz(b);
        const auto id=stable("edge/"+boundary.sourceEntityId.toUtf8());body.solidEdges.push_back({id,boundary.edge,detail});support.boundaryEdges.push_back({id,"line",boundary.edge,detail});GProp_GProps length;BRepGProp::LinearProperties(boundary.edge,length);perimeter+=length.Mass();
    }
    body.summary["boundaryLoopCount"]=wireCount;body.summary["closedBoundary"]=BRep_Tool::IsClosed(wires(1));
    body.supportFaces.push_back(std::move(support));body.summary["supportFaces"]=PlanarFaces::descriptors(body.supportFaces);body.summary["solidEdges"]=SolidFillet::descriptors(body.solidEdges);
    for(auto i=p.begin();i!=p.end();++i)body.summary[i.key()]=i.value();body.summary["sourceSketchId"]=sketchId;body.summary["sourceProfileId"]=profileId;body.summary["perimeterMm"]=perimeter;body.summary["normal"]=xyz(n);return body;
}
