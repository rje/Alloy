// SPDX-License-Identifier: MIT
#include "SolidRevolve.h"
#include "PlanarFaces.h"
#include <BRepPrimAPI_MakeRevol.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <Bnd_Box.hxx>
#include <TopoDS.hxx>
#include <QUuid>
#include <QSet>
#include <algorithm>
#include <numbers>
#include <cmath>

namespace {
void retainGeneratedRadialFaces(BRepPrimAPI_MakeRevol& sweep,const EvaluatedProfile& profile,const gp_Ax1& axis,EvaluatedBody& body){
    TopTools_IndexedMapOfShape solidFaces;TopExp::MapShapes(body.shape,TopAbs_FACE,solidFaces);QJsonArray unresolved;QSet<QString> blocked;
    for(const auto& boundary:profile.boundaryEdges){
        if(boundary.type!="line"||boundary.edge.IsNull())continue;
        const BRepAdaptor_Curve curve(boundary.edge);if(curve.GetType()!=GeomAbs_Line||std::abs(curve.Line().Direction().Dot(axis.Direction()))>1e-7)continue;
        const auto start=curve.Value(curve.FirstParameter()),end=curve.Value(curve.LastParameter());
        if(gp_Lin(axis).Distance(start)<1e-7&&gp_Lin(axis).Distance(end)<1e-7)continue;
        QStringList contacts{boundary.startContact,boundary.endContact};contacts.sort();
        const auto faceId=QUuid::createUuidV5(QUuid(body.featureId),QByteArray("alloy-revolve-radial-face/1\n")+profile.sketchId.toUtf8()+'\n'+profile.profileId.toUtf8()+'\n'+boundary.sourceEntityId.toUtf8()+'\n'+contacts.join('/').toUtf8()).toString(QUuid::WithoutBraces);
        if(blocked.contains(faceId))continue;
        auto refuse=[&](const QString& code){blocked.insert(faceId);std::erase_if(body.supportFaces,[&](const auto& f){return f.faceId==faceId;});unresolved.append(QJsonObject{{"faceId",faceId},{"sourceEntityId",boundary.sourceEntityId},{"code",code}});};
        TopTools_IndexedMapOfShape generated;
        for(const auto& next:sweep.Generated(boundary.edge))if(next.ShapeType()==TopAbs_FACE&&solidFaces.Contains(next))generated.Add(next);
        if(generated.Extent()!=1){refuse(generated.IsEmpty()?"generated_face_missing":"generated_face_ambiguous");continue;}
        const auto face=TopoDS::Face(solidFaces(solidFaces.FindIndex(generated(1))));const BRepAdaptor_Surface surface(face);
        if(surface.GetType()!=GeomAbs_Plane||!surface.Plane().Axis().Direction().IsParallel(axis.Direction(),1e-7)){refuse("generated_face_not_planar");continue;}
        auto normal=surface.Plane().Position().XDirection().Crossed(surface.Plane().Position().YDirection());if(face.Orientation()==TopAbs_REVERSED)normal.Reverse();
        const auto origin=axis.Location().Translated(gp_Vec(axis.Direction())*(gp_Vec(axis.Location(),surface.Plane().Location()).Dot(gp_Vec(normal))/axis.Direction().Dot(normal)));
        gp_Vec radial(origin,end);if(radial.Magnitude()<1e-7)radial=gp_Vec(origin,start);radial-=gp_Vec(normal)*radial.Dot(gp_Vec(normal));const gp_Dir u(radial),v=normal.Crossed(u);
        auto xyz=[](const auto& q){return QJsonArray{q.X(),q.Y(),q.Z()};};
        QJsonObject frame{{"origin",xyz(origin)},{"uAxis",xyz(u)},{"vAxis",xyz(v)},{"normal",xyz(normal)}};
        QJsonObject provenance{{"kind","uniqueKernelGeneratedRadialFace"},{"sourceSketchId",profile.sketchId},{"sourceProfileId",profile.profileId},{"sourceEntityId",boundary.sourceEntityId},{"startContact",boundary.startContact},{"endContact",boundary.endContact}};
        const auto prior=std::find_if(body.supportFaces.begin(),body.supportFaces.end(),[&](const auto& f){return f.faceId==faceId;});
        if(prior!=body.supportFaces.end()){if(!prior->face.IsSame(face)){refuse("generated_face_ambiguous");}continue;}
        body.supportFaces.push_back({faceId,body.bodyId,body.featureId,"revolve.radial",face,frame,{},provenance});
    }
    if(!unresolved.isEmpty())body.summary["generatedPlanarSupportErrors"]=unresolved;
}
}
EvaluatedBody SolidRevolve::evaluate(const QJsonObject& feature,const EvaluatedDocument& prior,const QJsonObject& source) {
    const auto p=feature["parameters"].toObject();
    auto reject=[&](const QString& code,const QString& message){throw GeometryKernel::EvaluationError({{"featureId",feature["id"]},{"featureType","revolve"},{"sketchId",p["sketchId"]},{"profileId",p["profileId"]},{"axisEntityId",p["axisEntityId"]},{"code",code},{"message",message}});};
    const EvaluatedSketch* sketch=nullptr;for(const auto& s:prior.sketches)if(s.featureId==p["sketchId"].toString())sketch=&s;
    if(!sketch)reject("source_unavailable","Revolve requires an earlier solved sketch");
    const EvaluatedProfile* profile=nullptr;for(const auto& q:sketch->profiles)if(q.profileId==p["profileId"].toString())profile=&q;
    if(!profile)reject("profile_unavailable","Revolve closed profile is unavailable; correct the source sketch or profile reference");
    QJsonObject line;const auto params=source["parameters"].toObject();for(const auto& e:params["entities"].toArray())if(e.toObject()["id"]==p["axisEntityId"])line=e.toObject();
    if(line["type"]!="line")reject("axis_unavailable","Revolve axis must resolve to an authored straight line in the same sketch");
    auto point=[&](const QJsonValue& id){for(const auto& v:params["points"].toArray())if(v.toObject()["id"]==id){const auto q=v.toObject();const auto f=sketch->resolvedFrame;auto o=f["origin"].toArray(),u=f["uAxis"].toArray(),w=f["vAxis"].toArray();return gp_Pnt(o[0].toDouble()+q["uMm"].toDouble()*u[0].toDouble()+q["vMm"].toDouble()*w[0].toDouble(),o[1].toDouble()+q["uMm"].toDouble()*u[1].toDouble()+q["vMm"].toDouble()*w[1].toDouble(),o[2].toDouble()+q["uMm"].toDouble()*u[2].toDouble()+q["vMm"].toDouble()*w[2].toDouble());}reject("axis_unavailable","Revolve axis endpoint is unavailable");return gp_Pnt();};
    const auto a=point(line["startPointId"]),b=point(line["endPointId"]);const gp_Vec direction(a,b);if(direction.Magnitude()<.001)reject("degenerate_axis","Revolve axis must have nonzero length");
    const gp_Ax1 axis(a,gp_Dir(direction));const auto n=sketch->resolvedFrame["normal"].toArray();const gp_Dir normal(n[0].toDouble(),n[1].toDouble(),n[2].toDouble());
    // Exact kernel bounds in the transverse direction refuse an axis crossing
    // the profile interior; boundary contact with the axis remains supported.
    const gp_Dir transverse(gp_Vec(axis.Direction()).Crossed(gp_Vec(normal)));gp_Trsf local;local.SetTransformation(gp_Ax3(a,normal,transverse));
    const auto aligned=BRepBuilderAPI_Transform(profile->face,local,true).Shape();Bnd_Box box;BRepBndLib::AddOptimal(aligned,box,false,false);double x0,y0,z0,x1,y1,z1;box.Get(x0,y0,z0,x1,y1,z1);
    if(x0< -1e-7 && x1>1e-7)reject("axis_crosses_profile","This Revolve requires the profile on one side of its axis; crossing profiles can self-intersect");
    const double radians=p["angleDeg"].toDouble()*std::numbers::pi/180.;BRepPrimAPI_MakeRevol sweep(profile->face,axis,radians,true);if(!sweep.IsDone())reject("revolve_failed","OCCT Revolve did not complete");
    EvaluatedBody body;body.featureId=feature["id"].toString();body.bodyId=feature["bodyId"].toString();body.name=feature["name"].toString();body.visible=feature["visible"].toBool();body.shape=sweep.Shape();body.summary=GeometryKernel::summarize(body.shape);
    if(!body.summary["valid"].toBool() || !body.summary["closed"].toBool() || body.summary["topology"].toObject()["solids"]!=1 || !std::isfinite(body.summary["volumeMm3"].toDouble()) || body.summary["volumeMm3"].toDouble()<=0)reject("revolve_invalid","Revolve must produce one valid closed solid with positive volume");
    if(p["extent"]=="partial")for(int end=0;end<2;++end){const auto shape=end ? sweep.LastShape() : sweep.FirstShape();if(shape.IsNull() || shape.ShapeType()!=TopAbs_FACE || BRepAdaptor_Surface(TopoDS::Face(shape)).GetType()!=GeomAbs_Plane)continue;auto frame=sketch->resolvedFrame;gp_Trsf rotation;rotation.SetRotation(axis,end ? radians:0);for(const auto& key:{"origin","uAxis","vAxis","normal"}){const auto q=frame[key].toArray();if(QString(key)=="origin"){gp_Pnt v(q[0].toDouble(),q[1].toDouble(),q[2].toDouble());v.Transform(rotation);frame[key]=QJsonArray{v.X(),v.Y(),v.Z()};}else{gp_Vec v(q[0].toDouble(),q[1].toDouble(),q[2].toDouble());v.Transform(rotation);frame[key]=QJsonArray{v.X(),v.Y(),v.Z()};}}if((end ? -1:1)*(x1>1e-7 ? 1:-1)<0){for(const auto& key:{"normal","vAxis"}){auto values=frame[key].toArray();for(int i=0;i<3;++i)values[i]=-values[i].toDouble();frame[key]=values;}}const QString role=end ? "revolve.end":"revolve.start";body.supportFaces.push_back({QUuid::createUuidV5(QUuid(body.featureId),("alloy-face/"+role+"/1").toUtf8()).toString(QUuid::WithoutBraces),body.bodyId,body.featureId,role,TopoDS::Face(shape),frame,{}});}
    if(p["extent"]=="full")retainGeneratedRadialFaces(sweep,*profile,axis,body);
    body.summary["supportFaces"]=PlanarFaces::descriptors(body.supportFaces);body.summary["operation"]="revolve";body.summary["sourceSketchId"]=p["sketchId"];body.summary["sourceProfileId"]=p["profileId"];body.summary["axisEntityId"]=p["axisEntityId"];body.summary["extent"]=p["extent"];body.summary["angleDeg"]=p["angleDeg"];body.summary["axisStartMm"]=QJsonArray{a.X(),a.Y(),a.Z()};body.summary["axisDirection"]=QJsonArray{axis.Direction().X(),axis.Direction().Y(),axis.Direction().Z()};return body;
}
