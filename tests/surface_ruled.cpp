// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include "SurfaceRuled.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRep_Tool.hxx>
#include <gp_Cylinder.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <cmath>
#include <limits>
#include <numbers>
#include <iostream>
namespace {
void check(bool v,const QString& s){if(!v)throw std::runtime_error(s.toStdString());}
void near(double a,double b){check(std::abs(a-b)<1e-6,QString("%1 != %2").arg(a,0,'g',14).arg(b,0,'g',14));}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument doc;auto sketch=SketchGeometry::addRectangle(SketchGeometry::newFeature(doc.authored().value("documentId").toString(),"XY","Boundary"),{{"u1Mm",0},{"v1Mm",0},{"u2Mm",40},{"v2Mm",30}});
    const auto entities=sketch.value("parameters").toObject().value("entities").toArray();
    sketch=SketchGeometry::addDimension(sketch,{{"kind","horizontal"},{"entityId",entities[0].toObject().value("id")},{"valueMm",40}}).feature;
    const auto dimension=sketch.value("parameters").toObject().value("dimensions").toArray().last().toObject().value("id").toString();
    check(doc.commit(SketchGeometry::replaceFeature(doc.authored(),sketch),"Boundary").isEmpty(),"sketch");
    check(doc.commit(CadDocument::surfacePatchCandidate(doc.authored(),{{"sketchId",sketch.value("id")},{"profileId",doc.evaluation().sketches.front().profiles.front().profileId}}),"Patch").isEmpty(),"patch");
    const auto source=doc.bodies().front();const auto face=source.supportFaces.front();EvaluatedSupportEdge bottom;
    for(const auto& edge:face.boundaryEdges){const BRepAdaptor_Curve c(edge.edge);const auto a=c.Value(c.FirstParameter()),b=c.Value(c.LastParameter());if(std::abs(a.Y())<1e-7 && std::abs(b.Y())<1e-7)bottom=edge;}
    check(!bottom.edge.IsNull(),"bottom boundary");TopoDS_Vertex a,b;TopExp::Vertices(bottom.edge,a,b,true);near(BRep_Tool::Pnt(a).X(),0);near(BRep_Tool::Pnt(b).X(),40);
    const QJsonObject ref{{"bodyId",source.bodyId},{"producerFeatureId",face.producerFeatureId},{"faceId",face.faceId},{"edgeId",bottom.edgeId}};const auto baseline=doc.authored();
    auto candidate=CadDocument::surfaceRuledCandidate(baseline,{{"sourceEdge",ref}});const auto defaults=candidate.value("features").toArray().last().toObject().value("parameters").toObject();near(defaults.value("distanceMm").toDouble(),20);near(defaults.value("angleDeg").toDouble(),0);check(defaults.value("ruleType")=="normal","Normal default");
    const auto preview=CadDocument::evaluateCandidate(candidate);check(preview.ok,preview.error);check(doc.authored()==baseline,"preview isolated");check(doc.commit(candidate,"Ruled").isEmpty(),"commit");const auto initial=doc.bodies().back();
    auto verify=[&](double length,double angle){const auto& body=doc.bodies().back();const auto t=body.summary.value("topology").toObject();check(body.summary.value("valid").toBool() && !body.summary.value("closed").toBool() && t.value("faces")==1 && t.value("edges")==4 && t.value("solids")==0 && !body.summary.value("volumeMm3").isDouble(),"open exact sheet");near(body.summary.value("areaMm2").toDouble(),length*20);check(body.bodyId==initial.bodyId && body.featureId==initial.featureId && body.supportFaces.front().faceId==initial.supportFaces.front().faceId,"stable identities");
        const double rad=angle*std::numbers::pi/180.;near(body.summary.value("rulingDirection").toArray()[1].toDouble(),std::sin(rad));near(body.summary.value("rulingDirection").toArray()[2].toDouble(),std::cos(rad));
        check(body.supportFaces.front().boundaryEdges.size()==4,"four boundaries");for(size_t i=0;i<4;++i){const auto& edge=body.supportFaces.front().boundaryEdges[i];check(edge.edgeId==initial.supportFaces.front().boundaryEdges[i].edgeId,"stable edge IDs");check(edge.provenance.value("sourceEdge")==ref && edge.provenance.value("lineage")=="surfaceRuledPrismBuilder","exact retained lineage");const BRepAdaptor_Curve curve(edge.edge);const auto first=curve.Value(curve.FirstParameter()),last=curve.Value(curve.LastParameter());const auto role=edge.provenance.value("role").toString();near(first.Distance(last),(role=="source" || role=="opposite")?length:20);if(role=="opposite"){near(first.Y(),20*std::sin(rad));near(last.Y(),20*std::sin(rad));near(first.Z(),20*std::cos(rad));near(last.Z(),20*std::cos(rad));}if(role=="source"){near(first.Y(),0);near(last.Y(),0);near(first.Z(),0);near(last.Z(),0);}}
    };
    verify(40,0);for(double angle:{30.,-30.,90.,120.,210.,-450.,0.}){check(doc.commit(CadDocument::surfaceRuledCandidate(doc.authored(),{{"featureId",initial.featureId},{"angleDeg",angle}}),"Angle").isEmpty(),"signed angle edit");verify(40,angle);}
    check(doc.commit(CadDocument::surfaceRuledCandidate(doc.authored(),{{"featureId",initial.featureId},{"angleDeg",30.}}),"Angle").isEmpty(),"30 baseline");
    sketch=SketchGeometry::updateDimension(sketch,{{"dimensionId",dimension},{"valueMm",45}}).feature;check(doc.commit(SketchGeometry::replaceFeature(doc.authored(),sketch),"Width").isEmpty(),"upstream edit");verify(45,30);doc.history().undo();verify(40,30);doc.history().redo();verify(45,30);
    const auto accepted=doc.authored();const auto revision=doc.revision();const int count=doc.history().count(),index=doc.history().index();const auto summary=doc.bodies().back().summary;
    for(const auto& args:std::vector<QJsonObject>{{{"distanceMm",0.}},{{"distanceMm",-1.}},{{"distanceMm",std::numeric_limits<double>::infinity()}},{{"angleDeg",std::numeric_limits<double>::infinity()}},{{"angleDeg","30"}},{{"ruleType","tangent"}},{{"isSolid",true}},{{"operation","join"}},{{"sourceEdge",QJsonObject{}}}}){auto edit=args;edit["featureId"]=initial.featureId;bool refused=false;try{doc.commit(CadDocument::surfaceRuledCandidate(accepted,edit),"Invalid");}catch(const std::invalid_argument&){refused=true;}check(refused,"invalid contract refusal");check(doc.authored()==accepted && doc.revision()==revision && doc.history().count()==count && doc.history().index()==index && doc.bodies().back().summary==summary,"invalid atomicity");}
    for(const auto& key:{"bodyId","producerFeatureId","faceId","edgeId"}){auto stale=ref;stale[key]=CadDocument::newId();const auto failed=CadDocument::surfaceRuledCandidate(accepted,{{"featureId",initial.featureId},{"sourceEdge",stale}});const auto evaluated=CadDocument::evaluateCandidate(failed);check(!evaluated.ok && evaluated.diagnostics.value("sourceEdge")==stale && evaluated.diagnostics.value("code")=="surface_source_unavailable","structured stale source refusal");check(!doc.commit(failed,"Stale").isEmpty() && doc.authored()==accepted && doc.revision()==revision && doc.history().count()==count && doc.history().index()==index,"atomic stale refusal");}
    auto curved=doc.evaluation();const auto cylinder=BRepBuilderAPI_MakeFace(gp_Cylinder(gp_Ax3(),10),0,1,0,30).Face();curved.bodies.front().shape=cylinder;curved.bodies.front().supportFaces.front().face=cylinder;
    bool curveTested=false;for(TopExp_Explorer e(cylinder,TopAbs_EDGE);e.More() && !curveTested;e.Next())if(BRepAdaptor_Curve(TopoDS::Edge(e.Current())).GetType()==GeomAbs_Line){curved.bodies.front().supportFaces.front().boundaryEdges.front().edge=TopoDS::Edge(e.Current());auto curveRef=ref;curveRef["edgeId"]=curved.bodies.front().supportFaces.front().boundaryEdges.front().edgeId;bool rejected=false;try{SurfaceRuled::resolveSource(curved,curveRef);}catch(const GeometryKernel::EvaluationError& ex){rejected=ex.diagnostics().value("code")=="unsupported_surface_source";}check(rejected,"actual curved-face refusal");curveTested=true;}check(curveTested,"curved source fixture");
    auto duplicated=doc.evaluation();duplicated.bodies.front().supportFaces.front().boundaryEdges.push_back(bottom);bool ambiguous=false;try{SurfaceRuled::resolveSource(duplicated,ref);}catch(const GeometryKernel::EvaluationError& e){ambiguous=e.diagnostics().value("code")=="ambiguous_surface_source";}check(ambiguous,"duplicate reference refusal");
    auto stale=doc.evaluation();stale.bodies.front().supportFaces.front().face=BRepBuilderAPI_MakeFace(gp_Pln(gp::XOY()),0,45,0,30).Face();bool unavailable=false;try{SurfaceRuled::resolveSource(stale,ref);}catch(const GeometryKernel::EvaluationError&){unavailable=true;}check(unavailable,"stale face membership refusal");
    auto shared=doc.evaluation();shared.bodies.front().shape=BRepPrimAPI_MakeBox(45,30,10).Shape();const auto& box=shared.bodies.front().shape;bool tested=false;for(TopExp_Explorer f(box,TopAbs_FACE);f.More() && !tested;f.Next())for(TopExp_Explorer e(f.Current(),TopAbs_EDGE);e.More() && !tested;e.Next()){shared.bodies.front().supportFaces.front().face=TopoDS::Face(f.Current());shared.bodies.front().supportFaces.front().boundaryEdges.front().edge=TopoDS::Edge(e.Current());auto sharedRef=ref;sharedRef["edgeId"]=shared.bodies.front().supportFaces.front().boundaryEdges.front().edgeId;bool rejected=false;try{SurfaceRuled::resolveSource(shared,sharedRef);}catch(const GeometryKernel::EvaluationError& ex){rejected=ex.diagnostics().value("code")=="ambiguous_surface_source";}check(rejected,"shared solid edge refusal");tested=true;}
    check(doc.commit(CadDocument::surfaceRuledCandidate(accepted,{{"featureId",initial.featureId},{"angleDeg",120.}}),"Retry").isEmpty(),"valid retry");verify(45,120);doc.history().undo();verify(45,30);doc.history().redo();verify(45,120);
    QTemporaryDir tmp;const auto path=tmp.filePath("ruled.alloy");check(doc.save(path).isEmpty(),"save");CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==doc.authored(),"reopen authored");near(reopened.bodies().back().summary.value("areaMm2").toDouble(),900);
    std::cout<<"PASS Normal ruled exact flange, signed 0/30/-30/90/120/210 angles, four-boundary lineage, upstream recompute, atomic refusal/retry, undo/redo/reopen\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
