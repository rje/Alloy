// SPDX-License-Identifier: MIT
#include "SolidPipe.h"
#include "RetainedSweepPath.h"
#include "PlanarFaces.h"
#include <BRepOffsetAPI_MakePipe.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <gp_Circ.hxx>
#include <gp_Pln.hxx>
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

EvaluatedBody SolidPipe::evaluate(const QJsonObject& feature,const EvaluatedDocument& prior,const QJsonObject& pathSource) {
    const auto p=feature["parameters"].toObject();
    auto reject=[&](const QString& code,const QString& message,QJsonObject extra=QJsonObject{}){extra["featureId"]=feature["id"];extra["featureType"]="pipe";extra["pathSketchId"]=p["pathSketchId"];extra["pathEntityIds"]=p["pathEntityIds"];extra["pathStartPointId"]=p["pathStartPointId"];extra["code"]=code;extra["message"]=message;throw GeometryKernel::EvaluationError(extra);};
    const EvaluatedSketch* pathSketch=nullptr;
    for(const auto& sketch:prior.sketches)if(sketch.featureId==p["pathSketchId"])pathSketch=&sketch;
    if(!pathSketch)reject("source_unavailable","Pipe requires an earlier retained sketch path");
    const double radius=p["diameterMm"].toDouble()/2,wall=p["wallThicknessMm"].toDouble();
    if(!std::isfinite(radius) || !std::isfinite(wall) || radius<=0 || wall<=0 || wall>=radius)reject("invalid_section","Pipe requires a positive finite outer diameter and wall thickness smaller than half the diameter");
    const auto path=retainedSweepPath(p,*pathSketch,pathSource,reject);
    const auto& initialTangent=path.initialTangent;const auto& lastTangent=path.lastTangent;const auto& finish=path.finish;const auto ids=p["pathEntityIds"].toArray();const auto& oriented=path.oriented;const double length=path.length;
    for(TopExp_Explorer edge(path.wire,TopAbs_EDGE);edge.More();edge.Next()){
        BRepAdaptor_Curve curve(TopoDS::Edge(edge.Current()));
        if(curve.GetType()==GeomAbs_Circle && curve.Circle().Radius()<=radius+1e-7)
            reject("pipe_bend_too_tight","Pipe outer radius must be smaller than every path arc radius; reduce Section Size or increase the bend radius");
    }
    const auto origin=path.origin;
    gp_Vec sectionU(1,0,0);sectionU-=initialTangent*sectionU.Dot(initialTangent);
    if(sectionU.Magnitude()<1e-8){sectionU=gp_Vec(0,1,0);sectionU-=initialTangent*sectionU.Dot(initialTangent);}sectionU.Normalize();
    const gp_Ax2 sectionFrame(origin,gp_Dir(initialTangent),gp_Dir(sectionU));
    auto outer=BRepBuilderAPI_MakeWire(BRepBuilderAPI_MakeEdge(gp_Circ(sectionFrame,radius)).Edge()).Wire();
    auto inner=BRepBuilderAPI_MakeWire(BRepBuilderAPI_MakeEdge(gp_Circ(sectionFrame,radius-wall)).Edge()).Wire();inner.Reverse();
    BRepBuilderAPI_MakeFace section(gp_Pln(origin,gp_Dir(initialTangent)),outer,true);section.Add(inner);
    if(!section.IsDone())reject("section_failed","Could not construct the annular Pipe section");
    BRepOffsetAPI_MakePipe pipe(path.wire,section.Face(),GeomFill_IsCorrectedFrenet,false);if(!pipe.IsDone())reject("pipe_failed","OCCT Pipe did not complete");
    EvaluatedBody body;body.featureId=feature["id"].toString();body.bodyId=feature["bodyId"].toString();body.name=feature["name"].toString();body.visible=feature["visible"].toBool();body.shape=pipe.Shape();body.summary=GeometryKernel::summarize(body.shape);
    if(!body.summary["valid"].toBool() || !body.summary["closed"].toBool() || body.summary["topology"].toObject()["solids"]!=1 || !std::isfinite(body.summary["volumeMm3"].toDouble()) || body.summary["volumeMm3"].toDouble()<=0)reject("pipe_invalid","Pipe must produce one valid closed positive-volume solid");
    BRepAlgoAPI_Check interference(body.shape,false,true);if(!interference.IsValid())reject("pipe_self_intersection","Pipe result has geometric self-interference; correct the path or reduce the profile radius");
    // First/LastShape are builder-provided lineage, never proximity matches.
    for(int end=0;end<2;++end){const auto lineage=end?pipe.LastShape():pipe.FirstShape();if(lineage.IsNull() || lineage.ShapeType()!=TopAbs_FACE)reject("pipe_end_lineage_failed","Pipe builder did not retain an annular end face");TopoDS_Face cap;int matches=0;for(TopExp_Explorer f(body.shape,TopAbs_FACE);f.More();f.Next())if(f.Current().IsSame(lineage)){cap=TopoDS::Face(f.Current());++matches;}if(matches!=1 || BRepAdaptor_Surface(cap).GetType()!=GeomAbs_Plane)reject("pipe_end_lineage_failed","Pipe end must resolve to one exact planar annular face");int wires=0;for(TopExp_Explorer boundary(cap,TopAbs_WIRE);boundary.More();boundary.Next())++wires;if(wires!=2)reject("pipe_end_invalid","Pipe end must retain both outer and inner boundary wires");const auto n=end?lastTangent:-initialTangent;gp_Vec u=sectionU;u-=n*u.Dot(n);if(u.Magnitude()<1e-8){u=gp_Vec(1,0,0)-n*n.X();if(u.Magnitude()<1e-8)u=gp_Vec(0,1,0)-n*n.Y();}u.Normalize();const auto v=n.Crossed(u);const auto o=end?finish:origin;const QJsonObject frame{{"origin",QJsonArray{o.X(),o.Y(),o.Z()}},{"uAxis",QJsonArray{u.X(),u.Y(),u.Z()}},{"vAxis",QJsonArray{v.X(),v.Y(),v.Z()}},{"normal",QJsonArray{n.X(),n.Y(),n.Z()}}};const QString role=end?"pipe.end":"pipe.start";body.supportFaces.push_back({QUuid::createUuidV5(QUuid(body.featureId),("alloy-face/"+role+"/1").toUtf8()).toString(QUuid::WithoutBraces),body.bodyId,body.featureId,role,cap,frame,{}});}
    body.summary["supportFaces"]=PlanarFaces::descriptors(body.supportFaces);body.summary["operation"]="pipe";body.summary["pathSketchId"]=p["pathSketchId"];body.summary["pathEntityIds"]=ids;body.summary["pathStartPointId"]=p["pathStartPointId"];body.summary["orientedPath"]=oriented;body.summary["pathLengthMm"]=length;body.summary["outerRadiusMm"]=radius;body.summary["innerRadiusMm"]=radius-wall;body.summary["diameterMm"]=2*radius;body.summary["wallThicknessMm"]=wall;body.summary["sectionType"]="circular";body.summary["isHollow"]=true;body.summary["distanceOne"]=1.;body.summary["extent"]="full";body.summary["orientation"]="perpendicular";body.summary["pathStartMm"]=QJsonArray{origin.X(),origin.Y(),origin.Z()};body.summary["pathEndMm"]=QJsonArray{finish.X(),finish.Y(),finish.Z()};return body;
}
