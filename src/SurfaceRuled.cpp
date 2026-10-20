// SPDX-License-Identifier: MIT
#include "SurfaceRuled.h"
#include "PlanarFaces.h"
#include "SolidFillet.h"
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRep_Tool.hxx>
#include <Precision.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <QUuid>
#include <cmath>
#include <array>
#include <numbers>

SurfaceRuled::Source SurfaceRuled::resolveSource(const EvaluatedDocument& prior,const QJsonObject& ref){
    auto reject=[&](const QString& code,const QString& message){throw GeometryKernel::EvaluationError({{"sourceEdge",ref},{"code",code},{"message",message}});};
    const EvaluatedSupportFace* face=nullptr;const EvaluatedSupportEdge* edge=nullptr;const EvaluatedBody* owner=nullptr;
    int owners=0;for(const auto& body:prior.bodies)if(body.bodyId==ref.value("bodyId").toString()){
        ++owners;for(const auto& f:body.supportFaces)if(f.faceId==ref.value("faceId").toString() && f.producerFeatureId==ref.value("producerFeatureId").toString()){
            if(face)reject("ambiguous_surface_source","Retained source face resolves more than once");face=&f;owner=&body;
            for(const auto& e:f.boundaryEdges)if(e.edgeId==ref.value("edgeId").toString()){
                if(edge)reject("ambiguous_surface_source","Retained source edge resolves more than once");edge=&e;
            }
        }
    }
    if(owners!=1 || !face || !edge || face->face.IsNull() || edge->edge.IsNull())reject("surface_source_unavailable","Select one exact earlier retained face boundary edge");
    int membership=0,adjacency=0,faceEdgeUses=0;
    for(TopExp_Explorer f(owner->shape,TopAbs_FACE);f.More();f.Next()){
        if(f.Current().IsSame(face->face))++membership;
        bool contains=false;for(TopExp_Explorer e(f.Current(),TopAbs_EDGE);e.More();e.Next())if(e.Current().IsSame(edge->edge)){
            contains=true;if(f.Current().IsSame(face->face))++faceEdgeUses;
        }if(contains)++adjacency;
    }
    if(membership!=1 || faceEdgeUses!=1 || adjacency!=1)reject("ambiguous_surface_source","Source must be one unique boundary edge of one actual face; shared or seam edges are unsupported");
    if((face->face.Orientation()!=TopAbs_FORWARD && face->face.Orientation()!=TopAbs_REVERSED) || !BRepCheck_Analyzer(face->face).IsValid() || BRepAdaptor_Surface(face->face).GetType()!=GeomAbs_Plane)
        reject("unsupported_surface_source","Normal Ruled requires one valid oriented planar source face");
    const BRepAdaptor_Curve curve(edge->edge);TopoDS_Vertex a,b;TopExp::Vertices(edge->edge,a,b,true);
    if(curve.GetType()!=GeomAbs_Line || a.IsNull() || b.IsNull() || a.IsSame(b) || !std::isfinite(curve.FirstParameter()) || !std::isfinite(curve.LastParameter()) || BRep_Tool::Pnt(a).Distance(BRep_Tool::Pnt(b))<=Precision::Confusion())
        reject("unsupported_surface_source","Normal Ruled requires one finite straight boundary edge with distinct endpoints");
    return {*face,*edge};
}
EvaluatedBody SurfaceRuled::evaluate(const QJsonObject& feature,const EvaluatedDocument& prior){
    try {
    const auto p=feature.value("parameters").toObject(),ref=p.value("sourceEdge").toObject();const auto id=feature.value("id").toString();
    auto reject=[&](const QString& code,const QString& message){throw GeometryKernel::EvaluationError({{"featureId",id},{"featureType","surfaceRuled"},{"sourceEdge",ref},{"code",code},{"message",message}});};
    Source source;try{source=resolveSource(prior,ref);}catch(const GeometryKernel::EvaluationError& e){reject(e.diagnostics().value("code").toString(),QString::fromUtf8(e.what()));}
    const double distance=p.value("distanceMm").toDouble(),angle=p.value("angleDeg").toDouble();
    if(p.value("ruleType")!="normal" || p.value("operation")!="newBody" || !p.value("isSolid").isBool() || p.value("isSolid").toBool())reject("unsupported_ruled_mode","Only Normal non-solid New Body is supported");
    if(!p.value("distanceMm").isDouble() || !std::isfinite(distance) || distance<=Precision::Confusion())reject("invalid_distance","Distance must be finite and greater than kernel tolerance");
    if(!p.value("angleDeg").isDouble() || !std::isfinite(angle))reject("invalid_angle","Angle must be finite degrees");
    const BRepAdaptor_Surface surface(source.face.face);auto normal=gp_Vec(surface.Plane().Axis().Direction());if(source.face.face.Orientation()==TopAbs_REVERSED)normal.Reverse();
    TopoDS_Vertex a,b;TopExp::Vertices(source.edge.edge,a,b,true);auto tangent=gp_Vec(BRep_Tool::Pnt(a),BRep_Tool::Pnt(b));tangent.Normalize();auto transverse=normal.Crossed(tangent);
    if(std::abs(normal.Dot(tangent))>1e-8 || transverse.Magnitude()<=Precision::Confusion())reject("unsupported_surface_source","Source edge must lie in its retained planar face");transverse.Normalize();
    // Alloy policy: signed rotation from oriented face normal toward n cross t.
    // Reduce degrees before conversion so any finite authored angle remains safe.
    const double radians=std::remainder(angle,360.)*std::numbers::pi/180.;const auto ruling=normal*std::cos(radians)+transverse*std::sin(radians);
    BRepPrimAPI_MakePrism prism(source.edge.edge,ruling*distance,false,true);prism.Build();if(!prism.IsDone())reject("surface_prism_failed","OCCT could not generate the ruled flange");
    EvaluatedBody body;body.featureId=id;body.bodyId=feature.value("bodyId").toString();body.name=feature.value("name").toString();body.visible=feature.value("visible").toBool(true);body.shape=prism.Shape();body.summary=GeometryKernel::summarize(body.shape);
    TopTools_IndexedMapOfShape faces,edges;TopExp::MapShapes(body.shape,TopAbs_FACE,faces);TopExp::MapShapes(body.shape,TopAbs_EDGE,edges);
    const auto topology=body.summary.value("topology").toObject();
    if(!body.summary.value("valid").toBool() || body.summary.value("closed").toBool() || faces.Extent()!=1 || edges.Extent()!=4 || topology.value("solids").toInt()!=0 || body.summary.value("volumeMm3").isDouble() || !std::isfinite(body.summary.value("areaMm2").toDouble()) || body.summary.value("areaMm2").toDouble()<=0)reject("surface_prism_invalid","Ruled flange must be one valid positive-area open four-edge sheet without solids");
    auto exact=[&](const TopoDS_Shape& shape)->TopoDS_Edge{if(shape.IsNull() || shape.ShapeType()!=TopAbs_EDGE || !edges.Contains(shape))reject("surface_lineage_unavailable","Missing exact prism boundary");return TopoDS::Edge(edges(edges.FindIndex(shape)));};
    auto generated=[&](const TopoDS_Vertex& vertex){TopTools_IndexedMapOfShape found;for(const auto& e:prism.Generated(vertex))if(e.ShapeType()==TopAbs_EDGE && edges.Contains(e))found.Add(e);if(found.Extent()!=1)reject("surface_lineage_unavailable","Missing unique generated endpoint ruling");return exact(found(1));};
    const std::array<std::pair<const char*,TopoDS_Edge>,4> boundaries{{{"source",exact(prism.FirstShape())},{"opposite",exact(prism.LastShape())},{"endA",generated(a)},{"endB",generated(b)}}};
    TopTools_IndexedMapOfShape retained;for(const auto& [role,edge]:boundaries)retained.Add(edge);if(retained.Extent()!=4)reject("surface_lineage_unavailable","Prism boundaries must be distinct");
    auto stable=[&](const QString& role){return QUuid::createUuidV5(QUuid(id),("alloy-surface-ruled/1/"+ref.value("bodyId").toString()+"/"+ref.value("producerFeatureId").toString()+"/"+ref.value("faceId").toString()+"/"+ref.value("edgeId").toString()+"/"+role).toUtf8()).toString(QUuid::WithoutBraces);};
    auto xyz=[](const auto& q){return QJsonArray{q.X(),q.Y(),q.Z()};};
    const auto resultFace=TopoDS::Face(faces(1));const BRepAdaptor_Surface result(resultFace);if(result.GetType()!=GeomAbs_Plane)reject("surface_prism_invalid","Straight Normal flange must be planar");const auto plane=result.Plane();auto u=plane.Position().XDirection(),v=plane.Position().YDirection(),n=plane.Axis().Direction();if(resultFace.Orientation()==TopAbs_REVERSED){v.Reverse();n.Reverse();}
    const QJsonObject provenance{{"generatorFeatureId",id},{"sourceEdge",ref},{"lineage","surfaceRuledPrismBuilder"}};
    EvaluatedSupportFace support{stable("face"),body.bodyId,id,"surfaceRuled.sheet",resultFace,{{"origin",xyz(plane.Location())},{"uAxis",xyz(u)},{"vAxis",xyz(v)},{"normal",xyz(n)}},{},provenance};
    for(const auto& [role,edge]:boundaries){auto detail=provenance;detail["role"]=role;const BRepAdaptor_Curve curve(edge);detail["startMm"]=xyz(curve.Value(curve.FirstParameter()));detail["endMm"]=xyz(curve.Value(curve.LastParameter()));const auto edgeId=stable("edge/"+QString(role));support.boundaryEdges.push_back({edgeId,"line",edge,detail});body.solidEdges.push_back({edgeId,edge,detail});}
    body.supportFaces.push_back(support);body.summary["supportFaces"]=PlanarFaces::descriptors(body.supportFaces);body.summary["solidEdges"]=SolidFillet::descriptors(body.solidEdges);body.summary["sourceEdge"]=ref;body.summary["sourceNormal"]=xyz(normal);body.summary["sourceTangent"]=xyz(tangent);body.summary["transverseDirection"]=xyz(transverse);body.summary["rulingDirection"]=xyz(ruling);body.summary["distanceMm"]=distance;body.summary["angleDeg"]=angle;body.summary["ruleType"]="normal";body.summary["operation"]="newBody";body.summary["isSolid"]=false;return body;
    } catch(const Standard_Failure& error){
        throw GeometryKernel::EvaluationError({{"featureId",feature.value("id")},{"featureType","surfaceRuled"},{"sourceEdge",feature.value("parameters").toObject().value("sourceEdge")},{"code","surface_prism_failed"},{"message","OCCT ruled flange evaluation failed: "+QString::fromUtf8(error.GetMessageString())}});
    }
}
