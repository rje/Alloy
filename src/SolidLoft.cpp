// SPDX-License-Identifier: MIT
#include "SolidLoft.h"
#include "PlanarFaces.h"
#include <BRepOffsetAPI_ThruSections.hxx>
#include <BRepTools.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Wire.hxx>
#include <QUuid>
#include <cmath>

EvaluatedBody SolidLoft::evaluate(const QJsonObject& feature,const EvaluatedDocument& prior) {
    auto reject=[](const QString& code,const QString& message){throw GeometryKernel::EvaluationError({{"code",code},{"message",message}});};
    const auto references=feature["parameters"].toObject()["profiles"].toArray();
    if(references.size()<2 || references.size()>32)reject("loft_sections_required","Loft requires 2–32 ordered circular profiles");
    std::vector<EvaluatedProfile> profiles;gp_Pnt firstCenter;gp_Dir firstNormal;double lastDistance=0;int direction=0;
    BRepOffsetAPI_ThruSections loft(true,false,1e-7);loft.SetMutableInput(false);loft.CheckCompatibility(true);
    loft.SetSmoothing(false);loft.SetContinuity(GeomAbs_C2);loft.SetMaxDegree(8);
    for(const auto& value:references) {
        const auto ref=value.toObject();const EvaluatedProfile* profile=nullptr;
        for(const auto& sketch:prior.sketches)if(sketch.featureId==ref["sketchId"].toString())for(const auto& candidate:sketch.profiles)if(candidate.profileId==ref["profileId"].toString())profile=&candidate;
        if(!profile)reject("loft_profile_unavailable","Each Loft profile must resolve to an earlier sketch region");
        if(profile->summary["type"]!="closed_circle_profile" || profile->boundaryEdges.size()!=1)reject("unsupported_loft_profile","This Loft supports analytic circular sections only");
        BRepAdaptor_Curve curve(profile->boundaryEdges.front().edge);
        if(curve.GetType()!=GeomAbs_Circle)reject("unsupported_loft_profile","Each Loft section must be an analytic circle");
        const auto circle=curve.Circle();const auto center=circle.Location();const auto normal=circle.Axis().Direction();
        if(profiles.empty()){firstCenter=center;firstNormal=normal;}
        else {
            const gp_Vec delta(firstCenter,center);const auto distance=delta.Dot(gp_Vec(firstNormal));
            if(std::abs(normal.Dot(firstNormal))<1-1e-9 || (delta-gp_Vec(firstNormal)*distance).Magnitude()>1e-6)reject("unsupported_loft_alignment","Circular Loft sections must be parallel and coaxial");
            if(direction==0)direction=distance>0 ? 1 : -1;
            if(direction*(distance-lastDistance)<0.001)reject("invalid_loft_order","Choose distinct sections in monotonic order along their shared axis");
            lastDistance=distance;
        }
        profiles.push_back(*profile);loft.AddWire(BRepTools::OuterWire(profile->face));
    }
    loft.Build();if(!loft.IsDone())reject("loft_failed","OCCT smooth Loft did not complete");
    EvaluatedBody body;body.featureId=feature["id"].toString();body.bodyId=feature["bodyId"].toString();body.name=feature["name"].toString();body.visible=feature["visible"].toBool();body.shape=loft.Shape();
    body.summary=GeometryKernel::summarize(body.shape);
    if(!body.summary["valid"].toBool() || !body.summary["closed"].toBool() || body.summary["topology"].toObject()["solids"]!=1)reject("loft_invalid","Loft must produce one valid closed solid");
    for(int end=0;end<2;++end) {
        const auto shape=end ? loft.LastShape() : loft.FirstShape();
        if(shape.IsNull() || shape.ShapeType()!=TopAbs_FACE || BRepAdaptor_Surface(TopoDS::Face(shape)).GetType()!=GeomAbs_Plane)reject("loft_cap_unavailable","Loft endpoint cap provenance is unavailable");
        auto frame=(end ? profiles.back() : profiles.front()).summary["frame"].toObject();
        auto n=frame["normal"].toArray(),v=frame["vAxis"].toArray();
        const auto source=n[0].toDouble()*firstNormal.X()+n[1].toDouble()*firstNormal.Y()+n[2].toDouble()*firstNormal.Z();
        if(source*(end ? direction : -direction)<0){for(int i=0;i<3;++i){n[i]=-n[i].toDouble();v[i]=-v[i].toDouble();}frame["normal"]=n;frame["vAxis"]=v;}
        const auto role=end ? "loft.end" : "loft.start";
        const auto faceId=QUuid::createUuidV5(QUuid(body.featureId),QByteArray("alloy-loft-cap/1/")+role).toString(QUuid::WithoutBraces);
        body.supportFaces.push_back({faceId,body.bodyId,body.featureId,role,TopoDS::Face(shape),frame,{}});
    }
    body.summary["supportFaces"]=PlanarFaces::descriptors(body.supportFaces);body.summary["operation"]="loft";body.summary["profiles"]=references;
    body.summary["interpolation"]=QJsonObject{{"kernel","OCCT ThruSections"},{"ruled",false},{"continuity","C2"},{"maxDegree",8},{"smoothingOptimization",false},{"endpointCondition","Free"}};
    return body;
}
