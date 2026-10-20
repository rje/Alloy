// SPDX-License-Identifier: MIT
#include "SurfaceExtend.h"
#include "PlanarFaces.h"
#include "SolidFillet.h"
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepTools.hxx>
#include <BRepTools_WireExplorer.hxx>
#include <BRep_Tool.hxx>
#include <Precision.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Wire.hxx>
#include <array>
#include <cmath>

namespace {
constexpr double tolerance=1e-7;
QJsonArray xyz(const auto& p){return {p.X(),p.Y(),p.Z()};}
struct Rectangle {std::array<gp_Pnt,4> corners;std::array<TopoDS_Edge,4> edges;};
Rectangle rectangle(const EvaluatedSupportFace& face,const QJsonObject& ref){
    auto reject=[&]{throw GeometryKernel::EvaluationError({{"sourceEdge",ref},{"code","unsupported_surface_source"},{"message","Natural Extend currently requires one rectangular four-straight-edge planar Patch sheet"}});};
    Rectangle result;int i=0;const auto wire=BRepTools::OuterWire(face.face);
    for(BRepTools_WireExplorer edge(wire,face.face);edge.More();edge.Next()){
        if(i>=4)reject();result.edges[i]=edge.Current();result.corners[i]=BRep_Tool::Pnt(edge.CurrentVertex());
        if(BRepAdaptor_Curve(result.edges[i]).GetType()!=GeomAbs_Line)reject();++i;
    }
    if(i!=4 || face.boundaryEdges.size()!=4)reject();
    QSet<QString> ids;TopTools_IndexedMapOfShape retained;
    for(const auto& edge:face.boundaryEdges){int matches=0;for(const auto& actual:result.edges)if(edge.edge.IsSame(actual))++matches;
        if(matches!=1 || edge.edgeId.isEmpty() || ids.contains(edge.edgeId) || retained.Contains(edge.edge))reject();ids.insert(edge.edgeId);retained.Add(edge.edge);}
    for(int j=0;j<4;++j){const gp_Vec a(result.corners[j],result.corners[(j+1)%4]),b(result.corners[(j+1)%4],result.corners[(j+2)%4]);
        if(a.Magnitude()<=tolerance || b.Magnitude()<=tolerance || std::abs(a.Normalized().Dot(b.Normalized()))>1e-9)reject();
    }
    return result;
}
}
SurfaceRuled::Source SurfaceExtend::resolveSource(const EvaluatedDocument& prior,const QJsonObject& ref){
    SurfaceRuled::Source source;
    try{source=SurfaceRuled::resolveSource(prior,ref);}catch(const GeometryKernel::EvaluationError& e){
        if(e.diagnostics().value("code")!="unsupported_surface_source")throw;
        auto details=e.diagnostics();details["message"]="Natural Extend requires one valid planar Patch face and a finite straight outer boundary edge";throw GeometryKernel::EvaluationError(details);
    }
    const EvaluatedBody* owner=nullptr;for(const auto& body:prior.bodies)if(body.bodyId==ref.value("bodyId").toString())owner=&body;
    TopTools_IndexedMapOfShape faces,wires,solids;TopExp::MapShapes(owner->shape,TopAbs_FACE,faces);TopExp::MapShapes(owner->shape,TopAbs_WIRE,wires);TopExp::MapShapes(owner->shape,TopAbs_SOLID,solids);
    if(!owner->occurrenceId.isEmpty() || faces.Extent()!=1 || wires.Extent()!=1 || solids.Extent()!=0 || source.face.role!="surfacePatch.sheet" || !BRep_Tool::IsClosed(wires(1)))
        throw GeometryKernel::EvaluationError({{"sourceEdge",ref},{"code","unsupported_surface_source"},{"message","Select one local open rectangular planar Patch sheet boundary"}});
    rectangle(source.face,ref);return source;
}
void SurfaceExtend::evaluate(EvaluatedBody& target,const QJsonObject& feature,const EvaluatedDocument& prior){
    const auto p=feature.value("parameters").toObject(),ref=p.value("sourceEdge").toObject();const auto id=feature.value("id").toString();
    auto reject=[&](const QString& code,const QString& message){throw GeometryKernel::EvaluationError({{"featureId",id},{"featureType","surfaceExtend"},{"sourceEdge",ref},{"code",code},{"message",message}});};
    try {
        SurfaceRuled::Source source;try{source=resolveSource(prior,ref);}catch(const GeometryKernel::EvaluationError& e){reject(e.diagnostics().value("code").toString(),QString::fromUtf8(e.what()));}
        if(target.bodyId!=ref.value("bodyId").toString())reject("surface_source_unavailable","Extend source must be the active target body");
        const double distance=p.value("distanceMm").toDouble();
        if(p.value("extendType")!="natural" || p.value("edgeAlignment")!="freeEdges" || !p.value("chaining").isBool() || p.value("chaining").toBool())reject("unsupported_extend_mode","Only one-edge Natural, Free Edges with chaining disabled is supported");
        if(!p.value("distanceMm").isDouble() || !std::isfinite(distance) || distance<=Precision::Confusion())reject("invalid_distance","Distance must be finite and greater than kernel tolerance");
        const auto original=rectangle(source.face,ref);auto corners=original.corners;int selected=-1;
        for(int i=0;i<4;++i)if(original.edges[i].IsSame(source.edge.edge))selected=i;
        if(selected<0)reject("surface_lineage_unavailable","Selected edge is not a unique rectangle side");
        const int next=(selected+1)%4;const gp_Vec inward(original.corners[selected],original.corners[(selected+3)%4]);const auto outward=-inward.Normalized();
        corners[selected].Translate(outward*distance);corners[next].Translate(outward*distance);
        std::array<TopoDS_Edge,4> edges;BRepBuilderAPI_MakeWire wire;
        for(int i=0;i<4;++i){edges[i]=BRepBuilderAPI_MakeEdge(corners[i],corners[(i+1)%4]).Edge();wire.Add(edges[i]);}
        if(!wire.IsDone())reject("surface_extend_failed","OCCT could not construct the extended boundary");
        BRepBuilderAPI_MakeFace builder(BRepAdaptor_Surface(source.face.face).Plane(),wire.Wire(),true);
        if(!builder.IsDone())reject("surface_extend_failed","OCCT could not construct the enlarged planar sheet");
        auto face=builder.Face();face.Orientation(source.face.face.Orientation());const auto summary=GeometryKernel::summarize(face);const auto topology=summary.value("topology").toObject();
        if(!summary.value("valid").toBool() || summary.value("closed").toBool() || topology.value("faces").toInt()!=1 || topology.value("edges").toInt()!=4 || topology.value("solids").toInt()!=0 || !summary.value("volumeMm3").isNull() || !std::isfinite(summary.value("areaMm2").toDouble()) || summary.value("areaMm2").toDouble()<=target.summary.value("areaMm2").toDouble())reject("surface_extend_invalid","Extension must remain one valid enlarged open rectangular sheet without volume");
        auto support=source.face;support.face=face;support.boundaryEdges.clear();auto provenance=source.face.provenance;provenance["extendModifierFeatureId"]=id;provenance["sourceEdge"]=ref;provenance["extendLineage"]="surfaceExtendRectangleSides";support.provenance=provenance;
        std::vector<EvaluatedSolidEdge> solidEdges;
        for(const auto& old:source.face.boundaryEdges){int side=-1;for(int i=0;i<4;++i)if(old.edge.IsSame(original.edges[i]))side=i;
            if(side<0)reject("surface_lineage_unavailable","Missing exact retained rectangle-side lineage");
            auto detail=old.provenance;detail["extendModifierFeatureId"]=id;detail["sourceEdge"]=ref;detail["extendLineage"]="surfaceExtendRectangleSides";detail["sourceBoundaryEdgeId"]=old.edgeId;detail["startMm"]=xyz(corners[side]);detail["endMm"]=xyz(corners[(side+1)%4]);detail["extendedBoundary"]=side==selected;
            support.boundaryEdges.push_back({old.edgeId,"line",edges[side],detail});solidEdges.push_back({old.edgeId,edges[side],detail});
        }
        // Only fresh shapes are authored above. Never mutate or heal the prior-stage B-rep.
        target.shape=face;target.summary=summary;target.supportFaces={support};target.solidEdges=std::move(solidEdges);
        target.summary["supportFaces"]=PlanarFaces::descriptors(target.supportFaces);target.summary["solidEdges"]=SolidFillet::descriptors(target.solidEdges);
        target.summary["producerFeatureId"]=target.featureId;target.summary["lastModifierFeatureId"]=id;target.summary["sourceEdge"]=ref;target.summary["distanceMm"]=distance;target.summary["extensionDirection"]=xyz(outward);target.summary["extendType"]="natural";target.summary["edgeAlignment"]="freeEdges";target.summary["chaining"]=false;
    }catch(const Standard_Failure& e){reject("surface_extend_failed","OCCT extension evaluation failed: "+QString::fromUtf8(e.GetMessageString()));}
}
