// SPDX-License-Identifier: MIT
#include "SurfaceStitch.h"
#include "PlanarFaces.h"
#include "SolidFillet.h"
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepBuilderAPI_Sewing.hxx>
#include <BRepGProp.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRep_Tool.hxx>
#include <GProp_GProps.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <QUuid>
#include <cmath>
#include <array>

namespace {
double maxTolerance(const TopoDS_Shape& s){double t=0;for(auto type:{TopAbs_FACE,TopAbs_EDGE,TopAbs_VERTEX}){TopTools_IndexedMapOfShape m;TopExp::MapShapes(s,type,m);for(int i=1;i<=m.Extent();++i)t=std::max(t,type==TopAbs_FACE?BRep_Tool::Tolerance(TopoDS::Face(m(i))):type==TopAbs_EDGE?BRep_Tool::Tolerance(TopoDS::Edge(m(i))):BRep_Tool::Tolerance(TopoDS::Vertex(m(i))));}return t;}
gp_Pnt center(const TopoDS_Shape& s){GProp_GProps p;BRepGProp::SurfaceProperties(s,p);return p.CentreOfMass();}
}
EvaluatedBody SurfaceStitch::evaluate(const QJsonObject& feature,const EvaluatedDocument& prior){
    const auto p=feature.value("parameters").toObject();const auto refs=p.value("sourceFaces").toArray();const auto id=feature.value("id").toString();const double tolerance=p.value("toleranceMm").toDouble();
    QJsonObject diagnostic{{"featureId",id},{"featureType","surfaceStitch"},{"sourceFaces",refs},{"requestedToleranceMm",tolerance},{"gapUnit","mm"}};
    auto reject=[&](const QString& code,const QString& message){auto d=diagnostic;d["code"]=code;d["message"]=message;throw GeometryKernel::EvaluationError(d);};
    if(refs.size()!=2 || !std::isfinite(tolerance) || tolerance<1e-7 || tolerance>1.)reject("invalid_stitch_inputs","Select two distinct rectangular sheets and a tolerance from 0.0000001 to 1 mm");
    std::array<const EvaluatedBody*,2> sources{};std::array<EvaluatedSupportFace,2> faces;std::array<std::array<gp_Pnt,4>,2> starts,ends;
    double before=0;
    for(int k=0;k<2;++k){const auto ref=refs[k].toObject();int matches=0;
        for(const auto& body:prior.bodies)if(body.bodyId==ref.value("bodyId").toString())for(const auto& face:body.supportFaces)
            if(face.faceId==ref.value("faceId").toString() && face.producerFeatureId==ref.value("producerFeatureId").toString()){sources[k]=&body;faces[k]=face;++matches;}
        if(matches!=1)reject("surface_source_unavailable","Each selected face must identify one unique earlier retained sheet");
        const auto& body=*sources[k];TopTools_IndexedMapOfShape actual,edges,solids;TopExp::MapShapes(body.shape,TopAbs_FACE,actual);TopExp::MapShapes(body.shape,TopAbs_EDGE,edges);TopExp::MapShapes(body.shape,TopAbs_SOLID,solids);
        if(!body.occurrenceId.isEmpty() || (!body.ownerId.isEmpty() && body.ownerId!=feature.value("ownerId").toString()))reject("unsupported_instance_edit","Stitch requires two local same-owner sheets");
        if(actual.Extent()!=1 || !actual.Contains(faces[k].face) || solids.Extent()!=0 || edges.Extent()!=4 || faces[k].boundaryEdges.size()!=4 || !BRepCheck_Analyzer(body.shape).IsValid() || BRepAdaptor_Surface(faces[k].face).GetType()!=GeomAbs_Plane)reject("unsupported_stitch_source","Stitch currently accepts finite planar rectangular single-face sheets");
        TopTools_IndexedMapOfShape named;
        for(int j=0;j<4;++j){const auto& edge=faces[k].boundaryEdges[j];if(!edges.Contains(edge.edge) || named.Contains(edge.edge) || edge.edgeId.isEmpty())reject("stitch_lineage_unavailable","Each source requires four distinct named boundaries");named.Add(edge.edge);const BRepAdaptor_Curve curve(edge.edge);if(curve.GetType()!=GeomAbs_Line)reject("unsupported_stitch_source","Only straight rectangle boundaries are supported");starts[k][j]=curve.Value(curve.FirstParameter());ends[k][j]=curve.Value(curve.LastParameter());if(starts[k][j].Distance(ends[k][j])<=1e-7)reject("unsupported_stitch_source","Degenerate boundaries cannot be stitched");}
        // Every rectangle edge has exactly one parallel opposite and two perpendicular neighbors.
        for(int j=0;j<4;++j){gp_Vec a(starts[k][j],ends[k][j]);a.Normalize();int parallel=0,perpendicular=0;for(int n=0;n<4;++n)if(n!=j){gp_Vec b(starts[k][n],ends[k][n]);b.Normalize();const double dot=std::abs(a.Dot(b));if(dot>1.-1e-9)++parallel;else if(dot<1e-9)++perpendicular;}if(parallel!=1 || perpendicular!=2)reject("unsupported_stitch_source","Each sheet must be a rectangle");}
        before=std::max(before,maxTolerance(body.shape));
    }
    if(sources[0]->bodyId==sources[1]->bodyId)reject("duplicate_stitch_source","Select two different sheet bodies");
    diagnostic["maxInputToleranceMm"]=before;
    if(before>tolerance+1e-12)reject("stitch_input_tolerance_exceeded","An input B-rep tolerance exceeds the declared operation tolerance");
    const auto plane0=BRepAdaptor_Surface(faces[0].face).Plane(),plane1=BRepAdaptor_Surface(faces[1].face).Plane();
    if(std::abs(plane0.Axis().Direction().Dot(plane1.Axis().Direction()))<1.-1e-9 || plane0.Distance(plane1.Location())>1e-7)reject("unsupported_stitch_source","This Stitch slice requires coplanar rectangles");
    int first=-1,second=-1,candidates=0;double gap=0;
    for(int a=0;a<4;++a)for(int b=0;b<4;++b){gp_Vec direction(starts[0][a],ends[0][a]);const double length=direction.Magnitude();direction.Normalize();gp_Vec other(starts[1][b],ends[1][b]);if(std::abs(length-other.Magnitude())>1e-7)continue;other.Normalize();if(std::abs(direction.Dot(other))<1.-1e-9)continue;
        const double forward=std::max(starts[0][a].Distance(starts[1][b]),ends[0][a].Distance(ends[1][b]));const double reverse=std::max(starts[0][a].Distance(ends[1][b]),ends[0][a].Distance(starts[1][b]));const double g=std::min(forward,reverse);
        const auto match=forward<=reverse?starts[1][b]:ends[1][b];if(std::abs(gp_Vec(starts[0][a],match).Dot(direction))>1e-7)continue;
        const auto lateral=gp_Vec(plane0.Axis().Direction()).Crossed(direction);const double side0=gp_Vec(starts[0][a],center(faces[0].face)).Dot(lateral),side1=gp_Vec(starts[1][b],center(faces[1].face)).Dot(lateral);
        if(side0*side1>=-1e-12)continue;
        // A candidate must face outward toward the other sheet, not bridge overlapping rectangles.
        if(g>1e-9 && gp_Vec(starts[0][a],match).Dot(lateral)*side0>=0)continue;
        ++candidates;first=a;second=b;gap=g;
    }
    if(candidates!=1)reject("unsupported_stitch_seam","Select nonoverlapping rectangles with exactly one full matching straight seam; partial or competing seams are unsupported");
    diagnostic["measuredSeamGapMm"]=gap;
    if(gap>tolerance+1e-12)reject("stitch_gap_exceeded","The measured full seam gap exceeds the declared tolerance");
    // Sewing must never mutate retained source tolerances during candidate evaluation.
    BRepBuilderAPI_Copy copy0(sources[0]->shape),copy1(sources[1]->shape);
    BRepBuilderAPI_Sewing sewing(tolerance,true,true,true,false);sewing.SetMaxTolerance(tolerance);sewing.Add(copy0.Shape());sewing.Add(copy1.Shape());sewing.Perform();
    const auto shape=sewing.SewedShape();TopTools_IndexedMapOfShape outputFaces,outputEdges,shells,solids;TopExp::MapShapes(shape,TopAbs_FACE,outputFaces);TopExp::MapShapes(shape,TopAbs_EDGE,outputEdges);TopExp::MapShapes(shape,TopAbs_SHELL,shells);TopExp::MapShapes(shape,TopAbs_SOLID,solids);
    if(shape.IsNull() || shape.ShapeType()!=TopAbs_SHELL || shells.Extent()!=1 || outputFaces.Extent()!=2 || solids.Extent()!=0 || sewing.NbMultipleEdges()!=0 || !BRepCheck_Analyzer(shape).IsValid() || BRep_Tool::IsClosed(shape))reject("stitch_invalid_result","Sewing must yield one valid connected open two-face shell without solids or multiple edges");
    TopTools_IndexedDataMapOfShapeListOfShape adjacency;TopExp::MapShapesAndAncestors(shape,TopAbs_EDGE,TopAbs_FACE,adjacency);int free=0,sewn=0;for(int i=1;i<=adjacency.Extent();++i){if(adjacency.FindFromIndex(i).Extent()==1)++free;else if(adjacency.FindFromIndex(i).Extent()==2)++sewn;else reject("stitch_nonmanifold","Sewing produced a non-manifold edge");}
    if(sewn!=1 || free!=6 || outputEdges.Extent()!=7)reject("stitch_invalid_result","The supported result requires one sewn seam and six outer free boundaries");
    const double after=maxTolerance(shape);diagnostic["maxOutputToleranceMm"]=after;diagnostic["toleranceIncreaseMm"]=std::max(0.,after-before);
    if(after>tolerance+1e-12)reject("stitch_output_tolerance_exceeded","Sewing requires output tolerance beyond the declared threshold");
    EvaluatedBody body;body.featureId=id;body.bodyId=feature.value("bodyId").toString();body.ownerId=feature.value("ownerId").toString();body.name=feature.value("name").toString();body.visible=feature.value("visible").toBool(true);body.shape=shape;body.summary=GeometryKernel::summarize(shape);
    auto stable=[&](const QString& role){return QUuid::createUuidV5(QUuid(id),("alloy-surface-stitch/1/"+role).toUtf8()).toString(QUuid::WithoutBraces);};
    TopTools_IndexedMapOfShape retainedEdges;
    for(int k=0;k<2;++k){auto& copier=k==0?copy0:copy1;const auto copied=copier.ModifiedShape(faces[k].face);auto mapped=sewing.IsModifiedSubShape(copied)?sewing.ModifiedSubShape(copied):copied;if(mapped.ShapeType()!=TopAbs_FACE || !outputFaces.Contains(mapped))reject("stitch_lineage_unavailable","Sewing did not preserve an exact source-face descendant");
        auto support=faces[k];support.face=TopoDS::Face(mapped);support.faceId=stable("face/"+faces[k].faceId);support.bodyId=body.bodyId;support.producerFeatureId=id;support.role="surfaceStitch.sheet";support.provenance={{"sourceFace",refs[k]},{"generatorFeatureId",id},{"lineage","surfaceStitchSewing"}};support.boundaryEdges.clear();
        for(int j=0;j<4;++j){const auto& edge=faces[k].boundaryEdges[j];const auto original=copier.ModifiedShape(edge.edge);const auto next=sewing.IsModifiedSubShape(original)?sewing.ModifiedSubShape(original):original;if(next.ShapeType()!=TopAbs_EDGE || !outputEdges.Contains(next))reject("stitch_lineage_unavailable","Sewing did not preserve an exact boundary descendant");const auto result=TopoDS::Edge(next);const bool seam=j==(k==0?first:second);if((adjacency.FindFromKey(result).Extent()==2)!=seam)reject("stitch_lineage_unavailable","Only the selected full candidate boundaries may become the shared seam");const auto edgeId=stable(seam?"seam/"+faces[0].boundaryEdges[first].edgeId+"/"+faces[1].boundaryEdges[second].edgeId:"edge/"+edge.edgeId);auto provenance=support.provenance;provenance["sourceEdgeId"]=edge.edgeId;provenance["sewnSeam"]=seam;PlanarFaces::updateStraightBoundary(edge.edge,result,provenance);support.boundaryEdges.push_back({edgeId,"line",result,provenance});if(!retainedEdges.Contains(result)){retainedEdges.Add(result);body.solidEdges.push_back({edgeId,result,provenance});}}
        body.supportFaces.push_back(support);
    }
    for(auto i=diagnostic.begin();i!=diagnostic.end();++i)body.summary[i.key()]=i.value();body.summary["sewnEdgeCount"]=sewn;body.summary["freeEdgeCount"]=free;body.summary["isSolid"]=false;body.summary["shellClassification"]="open";body.summary["operation"]="surfaceStitch";body.summary["sourceBodyIds"]=QJsonArray{sources[0]->bodyId,sources[1]->bodyId};body.summary["supportFaces"]=PlanarFaces::descriptors(body.supportFaces);body.summary["solidEdges"]=SolidFillet::descriptors(body.solidEdges);return body;
}
