// SPDX-License-Identifier: MIT
#include "SurfaceSweep.h"
#include "PlanarFaces.h"
#include "SolidFillet.h"
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepOffsetAPI_MakePipe.hxx>
#include <BRepAlgoAPI_Check.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <QUuid>
#include <array>
#include <cmath>

EvaluatedBody SurfaceSweep::evaluate(const QJsonObject& feature,const EvaluatedDocument& prior){
    const auto p=feature.value("parameters").toObject();const auto featureId=feature.value("id").toString();
    const auto sketchId=p.value("sketchId").toString(),entityId=p.value("entityId").toString();
    const auto pathSketchId=p.value("pathSketchId").toString(),pathEntityId=p.value("pathEntityId").toString(),startId=p.value("pathStartPointId").toString();
    auto reject=[&](const QString& code,const QString& message){auto d=p;d["featureId"]=featureId;d["featureType"]="surfaceSweep";d["code"]=code;d["message"]=message;throw GeometryKernel::EvaluationError(d);};
    const EvaluatedSketch *sketch=nullptr,*path=nullptr;
    for(const auto& s:prior.sketches){if(s.featureId==sketchId)sketch=&s;if(s.featureId==pathSketchId)path=&s;}
    if(!sketch || !path)reject("surface_source_unavailable","Surface Sweep requires earlier solved section and path sketches");
    QJsonObject source,pathSource;for(const auto& q:sketch->summary.value("entities").toArray())if(q.toObject().value("id")==entityId)source=q.toObject();
    for(const auto& q:path->summary.value("entities").toArray())if(q.toObject().value("id")==pathEntityId)pathSource=q.toObject();
    if(source.value("type")!="line" || source.value("construction").toBool() || !sketch->entityEdges.contains(entityId))reject("unsupported_surface_source","Select one ordinary open straight Line section");
    if(entityId==pathEntityId || (pathSource.value("type")!="line" && pathSource.value("type")!="arc") || pathSource.value("construction").toBool() || !path->entityEdges.contains(pathEntityId))reject("unsupported_path_entity","Select one distinct ordinary analytic Line or Arc path");
    if(startId!=pathSource.value("startPointId").toString() && startId!=pathSource.value("endPointId").toString())reject("open_path_required","Path start must be a retained endpoint of the selected entity");
    auto edge=TopoDS::Edge(BRepBuilderAPI_Copy(sketch->entityEdges.value(entityId),false,false).Shape());
    auto pathEdge=TopoDS::Edge(BRepBuilderAPI_Copy(path->entityEdges.value(pathEntityId),false,false).Shape());
    const BRepAdaptor_Curve section(edge),curve(pathEdge);
    if(section.GetType()!=GeomAbs_Line || section.IsClosed() || (curve.GetType()!=GeomAbs_Line && curve.GetType()!=GeomAbs_Circle) || curve.IsClosed())reject("unsupported_path_entity","Open analytic section and path required");
    gp_Pnt retainedStart;bool found=false;for(const auto& q:path->summary.value("worldPoints").toArray()){const auto pt=q.toObject();if(pt.value("id")==startId){const auto a=pt.value("positionMm").toArray();retainedStart=gp_Pnt(a[0].toDouble(),a[1].toDouble(),a[2].toDouble());found=true;}}
    if(!found)reject("open_path_required","Retained path endpoint is unavailable");
    const bool backwards=curve.Value(curve.FirstParameter()).Distance(retainedStart)>1e-7;
    gp_Pnt start,finish;gp_Vec tangent,endTangent;curve.D1(backwards?curve.LastParameter():curve.FirstParameter(),start,tangent);curve.D1(backwards?curve.FirstParameter():curve.LastParameter(),finish,endTangent);
    if(start.Distance(retainedStart)>1e-7 || start.Distance(finish)<1e-7 || tangent.Magnitude()<1e-10)reject("degenerate_path","Path must have distinct retained endpoints and a nonzero tangent");
    if(backwards){pathEdge.Reverse();tangent.Reverse();}tangent.Normalize();
    const auto a=section.Value(section.FirstParameter()),b=section.Value(section.LastParameter());const gp_Vec width(a,b);
    if(width.Magnitude()<1e-7)reject("degenerate_section","Section must have positive solved length");
    const gp_Pnt midpoint((a.X()+b.X())/2,(a.Y()+b.Y())/2,(a.Z()+b.Z())/2);
    if(midpoint.Distance(start)>1e-7)reject("profile_start_misaligned","Section midpoint must coincide with the retained path start");
    if(std::abs(width.Normalized().Dot(tangent))>1e-10)reject("profile_plane_misaligned","Section must be perpendicular to the initial path tangent");
    BRepBuilderAPI_MakeWire wire(pathEdge);BRepOffsetAPI_MakePipe sweep(wire.Wire(),edge,GeomFill_IsCorrectedFrenet,false);sweep.Build();
    if(!sweep.IsDone())reject("surface_sweep_failed","OCCT could not sweep the retained open section");
    EvaluatedBody body;body.featureId=featureId;body.bodyId=feature.value("bodyId").toString();body.name=feature.value("name").toString();body.visible=feature.value("visible").toBool(true);body.shape=sweep.Shape();body.summary=GeometryKernel::summarize(body.shape);
    const auto topology=body.summary.value("topology").toObject();
    if(!body.summary.value("valid").toBool() || body.summary.value("closed").toBool() || topology.value("faces").toInt()!=1 || topology.value("solids").toInt()!=0 || topology.value("edges").toInt()!=4 || !body.summary.value("volumeMm3").isNull() || !std::isfinite(body.summary.value("areaMm2").toDouble()) || body.summary.value("areaMm2").toDouble()<=0)reject("surface_sweep_invalid","Sweep must produce exactly one valid positive-area open sheet with no caps or solids");
    BRepAlgoAPI_Check interference(body.shape,false,true);if(!interference.IsValid())reject("surface_sweep_self_intersection","Sweep has geometric self-interference");
    auto xyz=[](const auto& q){return QJsonArray{q.X(),q.Y(),q.Z()};};
    auto stable=[&](const QByteArray& role){return QUuid::createUuidV5(QUuid(featureId),QByteArray("alloy-surface-sweep/1/")+sketchId.toUtf8()+"/"+entityId.toUtf8()+"/"+pathSketchId.toUtf8()+"/"+pathEntityId.toUtf8()+"/"+startId.toUtf8()+"/"+role).toString(QUuid::WithoutBraces);};
    QJsonObject provenance{{"generatorFeatureId",featureId},{"sourceSketchId",sketchId},{"sourceEntityId",entityId},{"pathSketchId",pathSketchId},{"pathEntityId",pathEntityId},{"pathStartPointId",startId},{"lineage","surfaceSweepBuilder"}};
    TopTools_IndexedMapOfShape faces,edges;TopExp::MapShapes(body.shape,TopAbs_FACE,faces);TopExp::MapShapes(body.shape,TopAbs_EDGE,edges);
    EvaluatedSupportFace support{stable("face"),body.bodyId,featureId,"surfaceSweep.sheet",TopoDS::Face(faces(1)),{},{},provenance};const BRepAdaptor_Surface surface(support.face);
    if(surface.GetType()==GeomAbs_Plane){const auto plane=surface.Plane();auto u=plane.Position().XDirection(),v=plane.Position().YDirection();auto n=u.Crossed(v);if(support.face.Orientation()==TopAbs_REVERSED){v.Reverse();n.Reverse();}support.frame={{"origin",xyz(plane.Location())},{"uAxis",xyz(u)},{"vAxis",xyz(v)},{"normal",xyz(n)}};}
    auto exact=[&](const TopoDS_Shape& shape)->TopoDS_Edge{if(shape.IsNull() || shape.ShapeType()!=TopAbs_EDGE || !edges.Contains(shape))return {};return TopoDS::Edge(edges(edges.FindIndex(shape)));};
    TopoDS_Vertex va,vb;TopExp::Vertices(edge,va,vb,true);auto generated=[&](const TopoDS_Vertex& vertex)->TopoDS_Edge{return exact(sweep.Generated(pathEdge,vertex));};
    const std::array<std::pair<QByteArray,TopoDS_Edge>,4> boundaries{{{"start",exact(sweep.FirstShape())},{"end",exact(sweep.LastShape())},{"endA",generated(va)},{"endB",generated(vb)}}};TopTools_IndexedMapOfShape retained;
    for(const auto& [role,current]:boundaries){if(current.IsNull() || retained.Contains(current))reject("surface_lineage_unavailable","Could not identify four distinct sheet boundaries");retained.Add(current);auto detail=provenance;detail["role"]=QString::fromLatin1(role);const BRepAdaptor_Curve c(current);detail["startMm"]=xyz(c.Value(c.FirstParameter()));detail["endMm"]=xyz(c.Value(c.LastParameter()));const auto id=stable(QByteArray("edge/")+role);body.solidEdges.push_back({id,current,detail});support.boundaryEdges.push_back({id,c.GetType()==GeomAbs_Line?"line":"curve",current,detail});}
    body.supportFaces.push_back(std::move(support));body.summary["supportFaces"]=PlanarFaces::descriptors(body.supportFaces);body.summary["solidEdges"]=SolidFillet::descriptors(body.solidEdges);
    for(auto i=p.begin();i!=p.end();++i)body.summary[i.key()]=i.value();body.summary["sourceSketchId"]=sketchId;body.summary["sourceEntityId"]=entityId;body.summary["pathStartMm"]=xyz(start);body.summary["pathEndMm"]=xyz(finish);GProp_GProps measure;BRepGProp::LinearProperties(pathEdge,measure);body.summary["pathLengthMm"]=measure.Mass();body.summary["sectionLengthMm"]=width.Magnitude();return body;
}
