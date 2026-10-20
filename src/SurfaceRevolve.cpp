// SPDX-License-Identifier: MIT
#include "SurfaceRevolve.h"
#include "PlanarFaces.h"
#include "SolidFillet.h"
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepPrimAPI_MakeRevol.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <QUuid>
#include <array>
#include <cmath>
#include <numbers>

EvaluatedBody SurfaceRevolve::evaluate(const QJsonObject& feature,const EvaluatedDocument& prior) {
    const auto p=feature.value("parameters").toObject();
    const auto featureId=feature.value("id").toString(),sketchId=p.value("sketchId").toString();
    const auto entityId=p.value("entityId").toString(),axisId=p.value("axisEntityId").toString();
    auto reject=[&](const QString& code,const QString& message){throw GeometryKernel::EvaluationError({
        {"featureId",featureId},{"featureType","surfaceRevolve"},{"sketchId",sketchId},
        {"entityId",entityId},{"axisEntityId",axisId},{"code",code},{"message",message}});};
    const EvaluatedSketch* sketch=nullptr;
    for(const auto& candidate:prior.sketches)if(candidate.featureId==sketchId){sketch=&candidate;break;}
    if(!sketch)reject("surface_source_unavailable","Surface Revolve requires an earlier solved sketch");
    QJsonObject source,axisSource;
    for(const auto& value:sketch->summary.value("entities").toArray()){
        const auto e=value.toObject();if(e.value("id")==entityId)source=e;if(e.value("id")==axisId)axisSource=e;
    }
    if(source.value("type")!="line" || source.value("construction").toBool() || !sketch->entityEdges.contains(entityId))
        reject("unsupported_surface_source","Select one ordinary open straight sketch Line; connected chains remain unsupported");
    if(axisId==entityId || axisSource.value("type")!="line" || !axisSource.value("construction").toBool()
        || !sketch->entityEdges.contains(axisId))
        reject("axis_unavailable","Select a distinct retained construction Line in the same sketch");
    auto edge=TopoDS::Edge(BRepBuilderAPI_Copy(sketch->entityEdges.value(entityId),false,false).Shape());
    const BRepAdaptor_Curve profile(edge),axisCurve(sketch->entityEdges.value(axisId));
    if(profile.GetType()!=GeomAbs_Line || profile.IsClosed())reject("unsupported_surface_source","Profile must remain one open straight Line");
    if(axisCurve.GetType()!=GeomAbs_Line)reject("axis_unavailable","Axis must remain a straight construction Line");
    const auto a=axisCurve.Value(axisCurve.FirstParameter()),b=axisCurve.Value(axisCurve.LastParameter());
    if(a.Distance(b)<1e-7)reject("degenerate_axis","Construction axis must have nonzero solved length");
    const gp_Ax1 axis(a,gp_Dir(gp_Vec(a,b)));
    const auto normalArray=sketch->resolvedFrame.value("normal").toArray();
    if(normalArray.size()!=3)reject("surface_source_unavailable","Source sketch must have a resolved plane");
    const gp_Vec normal(normalArray[0].toDouble(),normalArray[1].toDouble(),normalArray[2].toDouble());
    const gp_Vec transverse=gp_Vec(axis.Direction()).Crossed(normal);
    if(transverse.Magnitude()<1e-7)reject("axis_unavailable","Axis must lie in the source sketch plane");
    const gp_Dir side(transverse);
    const auto first=profile.Value(profile.FirstParameter()),last=profile.Value(profile.LastParameter());
    const double d1=gp_Vec(a,first).Dot(gp_Vec(side)),d2=gp_Vec(a,last).Dot(gp_Vec(side));
    if(std::abs(d1)<=1e-7 || std::abs(d2)<=1e-7 || (d1<0)!=(d2<0))
        reject("axis_crosses_profile","Surface section must stay strictly on one side of its axis, without touching it");
    const double angle=p.value("angleDeg").toDouble();
    if(!std::isfinite(angle) || angle<=0 || angle>=360)reject("invalid_angle","Partial Surface Revolve angle must be greater than 0 and less than 360 degrees");
    const double radians=angle*std::numbers::pi/180.*(p.value("reverseDirection").toBool() ? -1.:1.);
    BRepPrimAPI_MakeRevol sweep(edge,axis,radians,true);sweep.Build();
    if(!sweep.IsDone())reject("surface_revolve_failed","OCCT could not revolve the retained open section");
    EvaluatedBody body;body.featureId=featureId;body.bodyId=feature.value("bodyId").toString();
    body.name=feature.value("name").toString();body.visible=feature.value("visible").toBool(true);
    body.shape=sweep.Shape();body.summary=GeometryKernel::summarize(body.shape);
    const auto topology=body.summary.value("topology").toObject();
    if(!body.summary.value("valid").toBool() || body.summary.value("closed").toBool()
        || topology.value("faces").toInt()!=1 || topology.value("solids").toInt()!=0
        || topology.value("edges").toInt()!=4 || !body.summary.value("volumeMm3").isNull()
        || !std::isfinite(body.summary.value("areaMm2").toDouble()) || body.summary.value("areaMm2").toDouble()<=0)
        reject("surface_revolve_invalid","Surface Revolve must produce one valid positive-area open sheet with no caps or solids");
    auto stable=[&](const QByteArray& role){return QUuid::createUuidV5(QUuid(featureId),
        QByteArray("alloy-surface-revolve/1/")+sketchId.toUtf8()+"/"+entityId.toUtf8()+"/"+axisId.toUtf8()+"/"+role).toString(QUuid::WithoutBraces);};
    QJsonObject provenance{{"generatorFeatureId",featureId},{"sourceSketchId",sketchId},
        {"sourceEntityId",entityId},{"axisEntityId",axisId},{"lineage","surfaceRevolveBuilder"}};
    TopTools_IndexedMapOfShape faces,edges;TopExp::MapShapes(body.shape,TopAbs_FACE,faces);TopExp::MapShapes(body.shape,TopAbs_EDGE,edges);
    EvaluatedSupportFace support{stable("face"),body.bodyId,featureId,"surfaceRevolve.sheet",TopoDS::Face(faces(1)),{},{},provenance};
    const BRepAdaptor_Surface surface(support.face);
    auto xyz=[](const auto& q){return QJsonArray{q.X(),q.Y(),q.Z()};};
    if(surface.GetType()==GeomAbs_Plane){const auto plane=surface.Plane();auto u=plane.Position().XDirection(),v=plane.Position().YDirection();auto n=u.Crossed(v);
        if(support.face.Orientation()==TopAbs_REVERSED){v.Reverse();n.Reverse();}
        support.frame={{"origin",xyz(plane.Location())},{"uAxis",xyz(u)},{"vAxis",xyz(v)},{"normal",xyz(n)}};
    }
    auto exact=[&](const TopoDS_Shape& shape)->TopoDS_Edge{if(shape.IsNull() || shape.ShapeType()!=TopAbs_EDGE || !edges.Contains(shape))return {};return TopoDS::Edge(edges(edges.FindIndex(shape)));};
    TopoDS_Vertex va,vb;TopExp::Vertices(edge,va,vb,true);
    auto generated=[&](const TopoDS_Vertex& vertex)->TopoDS_Edge{TopTools_IndexedMapOfShape found;
        for(const auto& shape:sweep.Generated(vertex))if(shape.ShapeType()==TopAbs_EDGE && edges.Contains(shape))found.Add(shape);
        return found.Extent()==1 ? exact(found(1)) : TopoDS_Edge{};};
    const std::array<std::pair<QByteArray,TopoDS_Edge>,4> boundaries{{{"start",exact(sweep.FirstShape())},
        {"end",exact(sweep.LastShape())},{"endA",generated(va)},{"endB",generated(vb)}}};
    TopTools_IndexedMapOfShape retained;
    for(const auto& [role,current]:boundaries){
        if(current.IsNull() || retained.Contains(current))reject("surface_lineage_unavailable","Could not identify four distinct current sheet boundaries");retained.Add(current);
        auto detail=provenance;detail["role"]=QString::fromLatin1(role);const BRepAdaptor_Curve curve(current);
        detail["startMm"]=xyz(curve.Value(curve.FirstParameter()));detail["endMm"]=xyz(curve.Value(curve.LastParameter()));
        const auto id=stable(QByteArray("edge/")+role);body.solidEdges.push_back({id,current,detail});
        support.boundaryEdges.push_back({id,curve.GetType()==GeomAbs_Line ? "line":"curve",current,detail});
    }
    body.supportFaces.push_back(std::move(support));
    body.summary["supportFaces"]=PlanarFaces::descriptors(body.supportFaces);body.summary["solidEdges"]=SolidFillet::descriptors(body.solidEdges);
    body.summary["sourceSketchId"]=sketchId;body.summary["sourceEntityId"]=entityId;body.summary["axisEntityId"]=axisId;
    for(const auto& key:{"extent","direction","angleDeg","reverseDirection","operation","isSolid"})body.summary[key]=p.value(key);
    body.summary["axisStartMm"]=xyz(a);body.summary["axisDirection"]=xyz(axis.Direction());return body;
}
