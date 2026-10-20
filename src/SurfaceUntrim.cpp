// SPDX-License-Identifier: MIT
#include "SurfaceUntrim.h"
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRep_Tool.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <cmath>
#include <QSet>

EvaluatedBody SurfaceUntrim::resolveSource(const EvaluatedDocument& prior,const QJsonObject& ref,const QString& trimId){
    auto reject=[&](const QString& code,const QString& message){throw GeometryKernel::EvaluationError({{"sourceFace",ref},{"trimFeatureId",trimId},{"code",code},{"message",message}});};
    const EvaluatedBody* current=nullptr;const EvaluatedSupportFace* face=nullptr;int bodies=0,faces=0;
    for(const auto& body:prior.bodies)if(body.bodyId==ref.value("bodyId").toString()){
        ++bodies;current=&body;
        for(const auto& f:body.supportFaces)if(f.faceId==ref.value("faceId").toString() && f.producerFeatureId==ref.value("producerFeatureId").toString()){++faces;face=&f;}
    }
    if(bodies>1 || faces>1)reject("untrim_history_ambiguous","Source resolves to competing retained bodies or faces");
    if(!current || !face || face->face.IsNull())reject("surface_source_unavailable","Select one exact current retained trimmed face");
    if(!prior.surfaceTrimInputs.contains(trimId))reject("untrim_history_unavailable","The exact earlier Trim source stage is unavailable");
    if(ref.value("producerFeatureId")!=trimId || face->producerFeatureId!=trimId
        || current->summary.value("lastModifierFeatureId")!=trimId || current->summary.value("operation")!="surfaceTrim")
        reject("untrim_history_ambiguous","The selected face must be the unmodified result of its exact retained Trim");
    const auto source=prior.surfaceTrimInputs.value(trimId);
    if(source.bodyId!=current->bodyId || source.featureId!=current->featureId || source.ownerId!=current->ownerId
        || source.supportFaces.size()!=1 || source.supportFaces.front().faceId!=face->provenance.value("sourceFaceId")
        || !source.summary.value("lastModifierFeatureId").isUndefined())
        reject("untrim_history_ambiguous","Trim ancestry must identify one unmodified pretrim face on the same body");
    if(!current->occurrenceId.isEmpty() || !source.occurrenceId.isEmpty() || current->supportFaces.size()!=1
        || (face->role!="surfaceTrim.positive" && face->role!="surfaceTrim.negative")
        || current->summary.value("projection").toObject().value("mode")!="surfaceIntersection"
        || source.supportFaces.front().role!="surfaceExtrude.sheet")
        reject("unsupported_surface_source","Untrim currently restores one planar crossing-sheet Trim portion");
    auto qualify=[&](const EvaluatedBody& body){
        TopTools_IndexedMapOfShape actualFaces,edges,wires,solids;
        TopExp::MapShapes(body.shape,TopAbs_FACE,actualFaces);TopExp::MapShapes(body.shape,TopAbs_EDGE,edges);
        TopExp::MapShapes(body.shape,TopAbs_WIRE,wires);TopExp::MapShapes(body.shape,TopAbs_SOLID,solids);
        const auto& support=body.supportFaces.front();
        if(body.shape.IsNull() || body.summary.value("type")!="surface" || !BRepCheck_Analyzer(body.shape).IsValid()
            || actualFaces.Extent()!=1 || !actualFaces.Contains(support.face) || edges.Extent()!=4 || wires.Extent()!=1
            || solids.Extent()!=0 || !BRep_Tool::IsClosed(wires(1)) || BRepAdaptor_Surface(support.face).GetType()!=GeomAbs_Plane
            || body.solidEdges.size()!=4 || support.boundaryEdges.size()!=4)
            reject("unsupported_surface_source","Restoration requires one valid finite planar four-edge sheet and its exact Trim portion");
        QSet<QString> ids;TopTools_IndexedMapOfShape named;
        for(const auto& edge:support.boundaryEdges){
            if(edge.edgeId.isEmpty() || ids.contains(edge.edgeId) || !edges.Contains(edge.edge) || named.Contains(edge.edge)
                || BRepAdaptor_Curve(edge.edge).GetType()!=GeomAbs_Line)
                reject("untrim_history_ambiguous","A retained face boundary has missing or competing edge ancestry");
            int matches=0;for(const auto& solid:body.solidEdges)if(solid.edgeId==edge.edgeId && solid.edge.IsSame(edge.edge))++matches;
            if(matches!=1)reject("untrim_history_ambiguous","Retained body and face boundaries disagree");
            ids.insert(edge.edgeId);named.Add(edge.edge);
        }
    };
    qualify(*current);qualify(source);
    const double before=source.summary.value("areaMm2").toDouble(),after=current->summary.value("areaMm2").toDouble();
    if(!std::isfinite(before) || !std::isfinite(after) || after<=0 || before<=after)
        reject("untrim_history_ambiguous","The retained pretrim face must restore removed finite area");
    return source;
}

void SurfaceUntrim::evaluate(EvaluatedBody& target,const QJsonObject& feature,const EvaluatedDocument& prior){
    const auto p=feature.value("parameters").toObject(),ref=p.value("sourceFace").toObject();const auto id=feature.value("id").toString();
    auto reject=[&](const QString& code,const QString& message){throw GeometryKernel::EvaluationError({{"featureId",id},{"featureType","surfaceUntrim"},{"sourceFace",ref},{"trimFeatureId",p.value("trimFeatureId")},{"code",code},{"message",message}});};
    if(p.value("untrimType")!="externalEdges")reject("unsupported_untrim_mode","This Untrim slice supports External Edges only");
    EvaluatedBody source;try{source=resolveSource(prior,ref,p.value("trimFeatureId").toString());}
    catch(const GeometryKernel::EvaluationError& e){reject(e.diagnostics().value("code").toString(),QString::fromUtf8(e.what()));}
    if(target.bodyId!=source.bodyId || (!target.ownerId.isEmpty() && target.ownerId!=feature.value("ownerId").toString()))reject("surface_source_unavailable","Untrim requires its exact same-owner active target body");
    // Restore the immutable evaluated stage, never enlarge an unbounded support
    // surface or erase the authored Trim. Preserve current presentation metadata.
    target.shape=source.shape;target.supportFaces=source.supportFaces;target.solidEdges=source.solidEdges;target.summary=source.summary;
    target.summary["lastModifierFeatureId"]=id;target.summary["operation"]="surfaceUntrim";
    target.summary["sourceFace"]=ref;target.summary["trimFeatureId"]=p.value("trimFeatureId");
    target.summary["untrimType"]="externalEdges";target.summary["restorationPolicy"]="retainedFinitePretrimBoundary";
}
