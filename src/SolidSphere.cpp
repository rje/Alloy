// SPDX-License-Identifier: MIT
#include "SolidSphere.h"
#include "SolidFillet.h"
#include "PlanarFaces.h"
#include <BRepClass_FaceClassifier.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <algorithm>
#include <cmath>

namespace {
[[noreturn]] void fail(const QString& code, const QString& message) {
    throw GeometryKernel::EvaluationError({{"code",code},{"message",message}});
}
}

TopoDS_Shape SolidSphere::tool(const EvaluatedDocument& document,
                              const QJsonObject& p, QJsonArray* centerMm) {
    const auto support=GeometryKernel::resolveSketchSupport(document,p["support"].toObject());
    const auto origin=support.frame["origin"].toArray();
    const auto u=support.frame["uAxis"].toArray(),v=support.frame["vAxis"].toArray();
    const double diameter=p["diameterMm"].toDouble(),x=p["uMm"].toDouble(),y=p["vMm"].toDouble();
    if(!std::isfinite(diameter) || diameter<0.001 || diameter>100000
       || !std::isfinite(x) || !std::isfinite(y) || std::abs(x)>100000 || std::abs(y)>100000)
        fail("invalid_sphere_dimensions","Sphere placement and diameter are outside supported bounds");
    QJsonArray center;
    for(int axis=0;axis<3;++axis)center.append(origin[axis].toDouble()+x*u[axis].toDouble()+y*v[axis].toDouble());
    const gp_Pnt point(center[0].toDouble(),center[1].toDouble(),center[2].toDouble());
    BRepClass_FaceClassifier classifier(support.face,point,1e-7);
    if(classifier.State()!=TopAbs_IN && classifier.State()!=TopAbs_ON)
        fail("sphere_center_outside_face","Place the sphere center on the selected trimmed face, outside existing openings");
    if(centerMm)*centerMm=center;
    // Align the sphere poles with the face normal so the cut plane crosses its
    // equator rather than coinciding with a pole/seam on side-facing supports.
    const auto n=support.frame["normal"].toArray();
    const gp_Ax2 axes(point,gp_Dir(n[0].toDouble(),n[1].toDouble(),n[2].toDouble()),
                     gp_Dir(u[0].toDouble(),u[1].toDouble(),u[2].toDouble()));
    return BRepPrimAPI_MakeSphere(axes,diameter/2).Shape();
}

void SolidSphere::evaluate(EvaluatedDocument& document,const QJsonObject& feature) {
    const auto p=feature["parameters"].toObject();QJsonArray center;
    const auto sphere=tool(document,p,&center);const auto id=feature["id"].toString();
    const auto operation=p["operation"].toString();
    auto describe=[&](QJsonObject summary) {
        summary["operation"]="sphere";summary["sphereOperation"]=operation;
        summary["centerMm"]=center;summary["diameterMm"]=p["diameterMm"];
        summary["support"]=p["support"];return summary;
    };
    if(operation=="newBody") {
        document.bodies.push_back({id,feature["bodyId"].toString(),feature["name"].toString(),
            feature["visible"].toBool(),sphere,describe(GeometryKernel::summarize(sphere)),{}, {}});
        return;
    }
    auto target=std::find_if(document.bodies.begin(),document.bodies.end(),[&](const auto& b){return b.bodyId==p["targetBodyId"].toString();});
    if(operation!="cut" || target==document.bodies.end() || p["support"].toObject()["bodyId"]!=p["targetBodyId"])
        fail("sphere_target_unavailable","Sphere Cut requires the selected face's active body");
    const double before=GeometryKernel::summarize(target->shape)["volumeMm3"].toDouble();
    auto faces=target->supportFaces;auto edges=target->solidEdges;
    const auto result=GeometryKernel::booleanOperation("cut",target->shape,{sphere},&faces,&edges);
    auto summary=describe(GeometryKernel::summarize(result));
    const double removed=before-summary["volumeMm3"].toDouble();
    if(!std::isfinite(removed) || removed<=0)fail("sphere_cut_no_effect","The sphere must remove material from its target");
    summary["removedVolumeMm3"]=removed;summary["lastModifierFeatureId"]=id;
    summary["supportFaces"]=PlanarFaces::descriptors(faces);
    summary["solidEdges"]=SolidFillet::descriptors(edges);
    target->shape=result;target->summary=summary;
    target->supportFaces=std::move(faces);target->solidEdges=std::move(edges);
}
