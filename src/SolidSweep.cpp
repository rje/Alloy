// SPDX-License-Identifier: MIT
#include "SolidSweep.h"
#include "RetainedSweepPath.h"
#include "PlanarFaces.h"
#include <BRepOffsetAPI_MakePipe.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepAlgoAPI_Check.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <TopoDS.hxx>
#include <TopExp_Explorer.hxx>
#include <QUuid>
#include <QSet>
#include <limits>
#include <cmath>
#include <algorithm>

EvaluatedBody SolidSweep::evaluate(const QJsonObject& feature,const EvaluatedDocument& prior,const QJsonObject& pathSource) {
    const auto p=feature["parameters"].toObject();
    auto reject=[&](const QString& code,const QString& message,QJsonObject extra=QJsonObject{}){extra["featureId"]=feature["id"];extra["featureType"]="sweep";extra["profileSketchId"]=p["profileSketchId"];extra["profileId"]=p["profileId"];extra["pathSketchId"]=p["pathSketchId"];extra["pathEntityIds"]=p["pathEntityIds"];extra["pathStartPointId"]=p["pathStartPointId"];extra["code"]=code;extra["message"]=message;throw GeometryKernel::EvaluationError(extra);};
    const EvaluatedSketch *profileSketch=nullptr,*pathSketch=nullptr;
    for(const auto& s:prior.sketches){if(s.featureId==p["profileSketchId"])profileSketch=&s;if(s.featureId==p["pathSketchId"])pathSketch=&s;}
    if(!profileSketch || !pathSketch)reject("source_unavailable","Sweep requires two earlier retained sketch sources");
    const EvaluatedProfile* profile=nullptr;for(const auto& q:profileSketch->profiles)if(q.profileId==p["profileId"])profile=&q;
    if(!profile)reject("profile_unavailable","Sweep profile is unavailable; correct the retained profile source");
    const auto profileIds=profile->summary["sourceEntityIds"].toArray();QJsonObject circle;
    if(profileIds.size()==1)for(const auto& q:profileSketch->summary["entities"].toArray())if(q.toObject()["id"]==profileIds.first())circle=q.toObject();
    if(circle["type"]!="circle" || profile->summary["innerLoopCount"].toInt()!=0)reject("unsupported_profile","This Sweep requires one circular disk profile without inner loops");
    const auto path=retainedSweepPath(p,*pathSketch,pathSource,reject);
    const auto& initialTangent=path.initialTangent;const auto& lastTangent=path.lastTangent;const auto& finish=path.finish;const auto ids=p["pathEntityIds"].toArray();const auto& oriented=path.oriented;const double length=path.length;
    const auto centerValues=circle["centerMm"].toArray();const gp_Pnt center(centerValues[0].toDouble(),centerValues[1].toDouble(),centerValues[2].toDouble());const auto origin=path.origin;
    if(center.Distance(origin)>1e-7)reject("profile_start_misaligned","Circle profile center must coincide with the retained path start",{{"distanceMm",center.Distance(origin)}});
    const auto normalValues=profileSketch->resolvedFrame["normal"].toArray();const gp_Vec normal(normalValues[0].toDouble(),normalValues[1].toDouble(),normalValues[2].toDouble());if(std::abs(normal.Dot(initialTangent))<1.-1e-10)reject("profile_plane_misaligned","Circle profile plane must be perpendicular to the path start tangent");
    const auto face=BRepBuilderAPI_Copy(profile->face,false,false).Shape();BRepOffsetAPI_MakePipe sweep(path.wire,face,GeomFill_IsCorrectedFrenet,false);if(!sweep.IsDone())reject("sweep_failed","OCCT Sweep did not complete");
    EvaluatedBody body;body.featureId=feature["id"].toString();body.bodyId=feature["bodyId"].toString();body.name=feature["name"].toString();body.visible=feature["visible"].toBool();body.shape=sweep.Shape();body.summary=GeometryKernel::summarize(body.shape);
    if(!body.summary["valid"].toBool() || !body.summary["closed"].toBool() || body.summary["topology"].toObject()["solids"]!=1 || !std::isfinite(body.summary["volumeMm3"].toDouble()) || body.summary["volumeMm3"].toDouble()<=0)reject("sweep_invalid","Sweep must produce one valid closed positive-volume solid");
    BRepAlgoAPI_Check interference(body.shape,false,true);if(!interference.IsValid())reject("sweep_self_intersection","Sweep result has geometric self-interference; correct the path or reduce the profile radius");
    // First/LastShape are builder-provided lineage, never proximity matches.
    for(int end=0;end<2;++end){const auto lineage=end?sweep.LastShape():sweep.FirstShape();if(lineage.IsNull() || lineage.ShapeType()!=TopAbs_FACE)continue;TopoDS_Face cap;int matches=0;for(TopExp_Explorer f(body.shape,TopAbs_FACE);f.More();f.Next())if(f.Current().IsSame(lineage)){cap=TopoDS::Face(f.Current());++matches;}if(matches!=1 || BRepAdaptor_Surface(cap).GetType()!=GeomAbs_Plane)continue;const auto n=end?lastTangent:-initialTangent;const auto oldU=profileSketch->resolvedFrame["uAxis"].toArray();gp_Vec u(oldU[0].toDouble(),oldU[1].toDouble(),oldU[2].toDouble());u-=n*u.Dot(n);if(u.Magnitude()<1e-8){u=gp_Vec(1,0,0)-n*n.X();if(u.Magnitude()<1e-8)u=gp_Vec(0,1,0)-n*n.Y();}u.Normalize();const auto v=n.Crossed(u);const auto o=end?finish:origin;const QJsonObject frame{{"origin",QJsonArray{o.X(),o.Y(),o.Z()}},{"uAxis",QJsonArray{u.X(),u.Y(),u.Z()}},{"vAxis",QJsonArray{v.X(),v.Y(),v.Z()}},{"normal",QJsonArray{n.X(),n.Y(),n.Z()}}};const QString role=end?"sweep.end":"sweep.start";body.supportFaces.push_back({QUuid::createUuidV5(QUuid(body.featureId),("alloy-face/"+role+"/1").toUtf8()).toString(QUuid::WithoutBraces),body.bodyId,body.featureId,role,cap,frame,{}});}
    body.summary["supportFaces"]=PlanarFaces::descriptors(body.supportFaces);body.summary["operation"]="sweep";body.summary["profileSketchId"]=p["profileSketchId"];body.summary["sourceProfileId"]=p["profileId"];body.summary["pathSketchId"]=p["pathSketchId"];body.summary["pathEntityIds"]=ids;body.summary["pathStartPointId"]=p["pathStartPointId"];body.summary["orientedPath"]=oriented;body.summary["pathLengthMm"]=length;body.summary["sectionRadiusMm"]=circle["radiusMm"];body.summary["extent"]="full";body.summary["orientation"]="perpendicular";body.summary["pathStartMm"]=QJsonArray{origin.X(),origin.Y(),origin.Z()};body.summary["pathEndMm"]=QJsonArray{finish.X(),finish.Y(),finish.Z()};return body;
}
