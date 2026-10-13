// SPDX-License-Identifier: MIT
#include "SolidHole.h"
#include "PlanarFaces.h"
#include "SolidFillet.h"
#include <BRepClass3d_SolidClassifier.hxx>
#include <BRepClass_FaceClassifier.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <TopAbs_State.hxx>
#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace {
[[noreturn]] void fail(const QString& code,const QString& message,const QJsonObject& context={}) {
    auto details=context;details["code"]=code;details["message"]=message;
    throw GeometryKernel::EvaluationError(details);
}
gp_Pnt point(const QJsonArray& values) {
    if(values.size()!=3)fail("hole_point_unavailable","The retained point has no world placement");
    return {values[0].toDouble(),values[1].toDouble(),values[2].toDouble()};
}
}

TopoDS_Shape SolidHole::tool(const TopoDS_Shape& target,const gp_Pnt& entry,const gp_Dir& outward,
    double boreDiameterMm,double sinkDiameterMm,double includedAngleDeg,double* sinkDepthMm) {
    if(target.IsNull() || !std::isfinite(boreDiameterMm) || !std::isfinite(sinkDiameterMm) || !std::isfinite(includedAngleDeg)
        || boreDiameterMm<0.001 || sinkDiameterMm<=boreDiameterMm || sinkDiameterMm>100000
        || includedAngleDeg<=1 || includedAngleDeg>=179)
        fail("invalid_hole_dimensions","The Countersink dimensions are outside supported bounds");
    const auto depth=(sinkDiameterMm-boreDiameterMm)/(2*std::tan(includedAngleDeg*std::numbers::pi/360));
    if(!std::isfinite(depth) || depth<0.001 || depth>100000)
        fail("invalid_hole_dimensions","The Countersink depth is outside supported bounds");
    if(sinkDepthMm)*sinkDepthMm=depth;
    const gp_Dir inward=outward.Reversed();
    // An occurrence must still begin on material and enter it in the supported
    // direction. Do not silently move a stale point to the nearest face.
    BRepClass3d_SolidClassifier classifier(target);
    classifier.Perform(entry,1e-7);
    if(classifier.State()!=TopAbs_ON)fail("hole_entry_unavailable","The retained point is not on the current target boundary");
    classifier.Perform(entry.Translated(gp_Vec(inward)*1e-4),1e-7);
    if(classifier.State()!=TopAbs_IN)fail("hole_direction_unavailable","The support normal does not enter the target at this point");
    Bnd_Box box;BRepBndLib::AddOptimal(target,box,false,false);
    if(box.IsVoid())fail("hole_target_unavailable","The Hole target has no finite bounds");
    double x0,y0,z0,x1,y1,z1;box.Get(x0,y0,z0,x1,y1,z1);
    const std::array<double,2> xs{x0,x1},ys{y0,y1},zs{z0,z1};
    double furthest=0;
    for(const double x:xs)for(const double y:ys)for(const double z:zs)
        furthest=std::max(furthest,gp_Vec(entry,gp_Pnt(x,y,z)).Dot(gp_Vec(inward)));
    const auto margin=std::max(1.0,0.01*std::hypot(std::hypot(x1-x0,y1-y0),z1-z0));
    const auto boreLength=furthest+margin;
    if(!std::isfinite(boreLength) || boreLength<0.001 || boreLength>200000)
        fail("hole_target_unavailable","The Hole target bounds exceed supported All reach");
    const auto start=entry.Translated(gp_Vec(inward)*-1e-3);
    const gp_Ax2 boreAxis(start,inward),sinkAxis(entry,inward);
    const auto bore=BRepPrimAPI_MakeCylinder(boreAxis,boreDiameterMm/2,boreLength+1e-3).Shape();
    const auto sink=BRepPrimAPI_MakeCone(sinkAxis,sinkDiameterMm/2,boreDiameterMm/2,depth).Shape();
    return GeometryKernel::booleanOperation("join",bore,{sink});
}

TopoDS_Shape SolidHole::cutter(const EvaluatedDocument& document,const QJsonObject& feature,
    const QJsonObject& sourceSketch,QJsonObject* details) {
    const auto p=feature["parameters"].toObject();const auto sketchId=p["sourceSketchId"].toString();
    const auto sketch=std::find_if(document.sketches.begin(),document.sketches.end(),[&](const auto& item){return item.featureId==sketchId;});
    const auto target=std::find_if(document.bodies.begin(),document.bodies.end(),[&](const auto& item){return item.bodyId==p["targetBodyId"].toString();});
    if(sketch==document.sketches.end() || target==document.bodies.end() || sourceSketch["type"]!="sketch"
        || sourceSketch["parameters"].toObject()["plane"]!="face")
        fail("hole_source_unavailable","Hole requires an earlier evaluated supported sketch and active body");
    const auto support=sourceSketch["parameters"].toObject()["support"].toObject();
    if(support["bodyId"]!=p["targetBodyId"])fail("hole_support_unavailable","The sketch support and Hole target must be the same body");
    const auto face=GeometryKernel::resolveSketchSupport(document,support);
    const auto frame=sketch->resolvedFrame;
    if(frame!=face.frame)fail("hole_support_unavailable","The retained source frame no longer matches its target face");
    QJsonObject chosen;
    for(const auto& value:sketch->summary["standalonePoints"].toArray()) {
        const auto item=value.toObject();
        if(item["id"]==p["sourceEntityId"] && item["pointId"]==p["sourcePointId"]) {chosen=item;break;}
    }
    if(chosen.isEmpty())fail("hole_point_unavailable","The retained standalone sketch point is unavailable");
    const auto entry=point(chosen["positionMm"].toArray());
    BRepClass_FaceClassifier faceClassifier(face.face,entry,1e-7);
    if(faceClassifier.State()!=TopAbs_IN)fail("hole_entry_unavailable","The sketch point is not inside its surviving trimmed support face");
    const auto n=frame["normal"].toArray();if(n.size()!=3)fail("hole_support_unavailable","The retained support has no normal");
    const gp_Dir outward(n[0].toDouble(),n[1].toDouble(),n[2].toDouble());
    double depth=0;const auto shape=tool(target->shape,entry,outward,p["boreDiameterMm"].toDouble(),
        p["sinkDiameterMm"].toDouble(),p["includedAngleDeg"].toDouble(),&depth);
    if(details)*details={{"entryMm",chosen["positionMm"]},{"sinkDepthMm",depth}};
    return shape;
}

void SolidHole::evaluate(EvaluatedDocument& document,const QJsonObject& feature,const QJsonObject& sourceSketch) {
    const auto p=feature["parameters"].toObject();QJsonObject details;
    const auto cutterShape=cutter(document,feature,sourceSketch,&details);
    const auto target=std::find_if(document.bodies.begin(),document.bodies.end(),[&](const auto& item){return item.bodyId==p["targetBodyId"].toString();});
    const auto before=GeometryKernel::summarize(target->shape)["volumeMm3"].toDouble();
    auto faces=target->supportFaces;auto edges=target->solidEdges;
    const auto result=GeometryKernel::booleanOperation("cut",target->shape,{cutterShape},&faces,&edges);
    auto summary=GeometryKernel::summarize(result);const auto removed=before-summary["volumeMm3"].toDouble();
    if(!std::isfinite(removed) || removed<=1e-9)fail("hole_cut_no_effect","The Hole did not remove material from its target");
    summary["operation"]="hole";summary["lastModifierFeatureId"]=feature["id"];
    summary["sourceSketchId"]=p["sourceSketchId"];summary["sourceEntityId"]=p["sourceEntityId"];
    summary["sourcePointId"]=p["sourcePointId"];summary["targetBodyId"]=p["targetBodyId"];
    summary["boreDiameterMm"]=p["boreDiameterMm"];summary["sinkDiameterMm"]=p["sinkDiameterMm"];
    summary["includedAngleDeg"]=p["includedAngleDeg"];summary["sinkDepthMm"]=details["sinkDepthMm"];
    summary["removedVolumeMm3"]=removed;summary["entryMm"]=details["entryMm"];
    summary["supportFaces"]=PlanarFaces::descriptors(faces);summary["solidEdges"]=SolidFillet::descriptors(edges);
    target->shape=result;target->summary=summary;target->supportFaces=std::move(faces);target->solidEdges=std::move(edges);
}
