// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include "SurfaceExtend.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QTemporaryDir>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepTools.hxx>
#include <gp_Cylinder.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <cmath>
#include <iostream>
#include <limits>
#include <sstream>
namespace {
void check(bool v,const QString& s){if(!v)throw std::runtime_error(s.toStdString());}
void near(double a,double b){check(std::abs(a-b)<1e-6,QString("%1 != %2").arg(a,0,'g',14).arg(b,0,'g',14));}
QByteArray hash(const TopoDS_Shape& shape){std::ostringstream bytes;BRepTools::Write(shape,bytes);return QCryptographicHash::hash(QByteArray::fromStdString(bytes.str()),QCryptographicHash::Sha256);}
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
    check(!bottom.edge.IsNull(),"bottom boundary");const auto sourceHash=hash(source.shape);
    const QJsonObject ref{{"bodyId",source.bodyId},{"producerFeatureId",face.producerFeatureId},{"faceId",face.faceId},{"edgeId",bottom.edgeId}};const auto baseline=doc.authored();
    auto candidate=CadDocument::surfaceExtendCandidate(baseline,{{"sourceEdge",ref}});const auto feature=candidate.value("features").toArray().last().toObject();const auto id=feature.value("id").toString();const auto defaults=feature.value("parameters").toObject();
    near(defaults.value("distanceMm").toDouble(),20);check(defaults.value("extendType")=="natural" && defaults.value("edgeAlignment")=="freeEdges" && defaults.value("chaining")==false && !feature.contains("bodyId"),"Natural modifier defaults");
    const auto preview=CadDocument::evaluateCandidate(candidate);check(preview.ok,preview.error);check(doc.authored()==baseline && hash(source.shape)==sourceHash,"preview isolated and source BRep immutable");check(doc.commit(candidate,"Extend").isEmpty(),"commit");
    auto verify=[&](double width,double distance){check(doc.bodies().size()==1,"one in-place body");const auto& body=doc.bodies().front();const auto t=body.summary.value("topology").toObject();check(body.summary.value("valid").toBool() && !body.summary.value("closed").toBool() && t.value("faces")==1 && t.value("edges")==4 && t.value("solids")==0 && body.summary.value("volumeMm3").isNull(),"open exact sheet");near(body.summary.value("areaMm2").toDouble(),width*(30+distance));check(body.bodyId==source.bodyId && body.featureId==source.featureId && body.supportFaces.front().faceId==face.faceId && body.supportFaces.front().producerFeatureId==face.producerFeatureId,"retained body and face identities");
        check(body.supportFaces.front().boundaryEdges.size()==4 && body.solidEdges.size()==4,"four retained boundaries");
        for(size_t i=0;i<4;++i){const auto& edge=body.supportFaces.front().boundaryEdges[i];check(edge.edgeId==face.boundaryEdges[i].edgeId,"stable edge IDs");check(edge.provenance.value("sourceEdge")==ref && edge.provenance.value("extendLineage")=="surfaceExtendRectangleSides" && edge.provenance.value("sourceBoundaryEdgeId")==edge.edgeId,"exact retained side lineage");const BRepAdaptor_Curve curve(edge.edge);check(curve.GetType()==GeomAbs_Line,"straight output");const auto a=curve.Value(curve.FirstParameter()),b=curve.Value(curve.LastParameter());near(a.Z(),0);near(b.Z(),0);if(edge.edgeId==bottom.edgeId){near(a.Y(),-distance);near(b.Y(),-distance);near(a.Distance(b),width);}}
        near(BRepAdaptor_Surface(body.supportFaces.front().face).Plane().Axis().Direction().Z(),1);
        const auto before=doc.evaluationBefore(id);check(before.bodies.size()==1,"prior stage body");near(before.bodies.front().summary.value("areaMm2").toDouble(),width*30);if(width==40)check(hash(before.bodies.front().shape)==sourceHash,"unchanged prior-stage BRep hash");check(hash(source.shape)==sourceHash,"held original shape immutable");
    };
    verify(40,20);doc.history().undo();near(doc.bodies().front().summary.value("areaMm2").toDouble(),1200);check(hash(doc.bodies().front().shape)==sourceHash,"undo source hash");doc.history().redo();verify(40,20);
    check(doc.commit(CadDocument::surfaceExtendCandidate(doc.authored(),{{"featureId",id},{"distanceMm",10.}}),"Distance").isEmpty(),"distance edit");verify(40,10);doc.history().undo();verify(40,20);doc.history().redo();verify(40,10);
    check(doc.commit(CadDocument::surfaceExtendCandidate(doc.authored(),{{"featureId",id},{"distanceMm",20.}}),"Distance").isEmpty(),"distance restore");
    sketch=SketchGeometry::updateDimension(sketch,{{"dimensionId",dimension},{"valueMm",45}}).feature;check(doc.commit(SketchGeometry::replaceFeature(doc.authored(),sketch),"Width").isEmpty(),"upstream edit");verify(45,20);doc.history().undo();verify(40,20);doc.history().redo();verify(45,20);
    const auto accepted=doc.authored();const auto revision=doc.revision();const int count=doc.history().count(),index=doc.history().index();const auto currentHash=hash(doc.bodies().front().shape);
    auto atomic=[&]{check(doc.authored()==accepted && doc.revision()==revision && doc.history().count()==count && doc.history().index()==index && hash(doc.bodies().front().shape)==currentHash && hash(source.shape)==sourceHash,"refusal atomicity");};
    for(const auto& args:std::vector<QJsonObject>{{{"distanceMm",0.}},{{"distanceMm",-1.}},{{"distanceMm",std::numeric_limits<double>::infinity()}},{{"distanceMm","20"}},{{"extendType","tangent"}},{{"edgeAlignment","alignEdges"}},{{"chaining",true}},{{"sourceEdge",QJsonObject{}}},{{"targetBodyId",source.bodyId}}}){auto edit=args;edit["featureId"]=id;bool refused=false;try{doc.commit(CadDocument::surfaceExtendCandidate(accepted,edit),"Invalid");}catch(const std::invalid_argument&){refused=true;}check(refused,"invalid contract refusal");atomic();}
    for(const auto& key:{"bodyId","producerFeatureId","faceId","edgeId"}){auto stale=ref;stale[key]=CadDocument::newId();const auto failed=CadDocument::surfaceExtendCandidate(accepted,{{"featureId",id},{"sourceEdge",stale}});const auto evaluated=CadDocument::evaluateCandidate(failed);check(!evaluated.ok && evaluated.diagnostics.value("sourceEdge")==stale && evaluated.diagnostics.value("code")=="surface_source_unavailable","structured stale source refusal");check(!doc.commit(failed,"Stale").isEmpty(),"stale refused");atomic();}
    // Eligibility tests use actual curved/shared topology; these are not self-intersection tests.
    auto curved=doc.evaluationBefore(id);const auto cylinder=BRepBuilderAPI_MakeFace(gp_Cylinder(gp_Ax3(),10),0,1,0,30).Face();curved.bodies.front().shape=cylinder;curved.bodies.front().supportFaces.front().face=cylinder;
    bool curveTested=false;for(TopExp_Explorer e(cylinder,TopAbs_EDGE);e.More() && !curveTested;e.Next())if(BRepAdaptor_Curve(TopoDS::Edge(e.Current())).GetType()==GeomAbs_Line){curved.bodies.front().supportFaces.front().boundaryEdges.front().edge=TopoDS::Edge(e.Current());auto curveRef=ref;curveRef["edgeId"]=curved.bodies.front().supportFaces.front().boundaryEdges.front().edgeId;bool rejected=false;try{SurfaceExtend::resolveSource(curved,curveRef);}catch(const GeometryKernel::EvaluationError& ex){rejected=ex.diagnostics().value("code")=="unsupported_surface_source";}check(rejected,"curved face refusal");curveTested=true;}check(curveTested,"curved fixture");
    auto shared=doc.evaluationBefore(id);shared.bodies.front().shape=BRepPrimAPI_MakeBox(45,30,10).Shape();bool tested=false;for(TopExp_Explorer f(shared.bodies.front().shape,TopAbs_FACE);f.More() && !tested;f.Next())for(TopExp_Explorer e(f.Current(),TopAbs_EDGE);e.More() && !tested;e.Next()){shared.bodies.front().supportFaces.front().face=TopoDS::Face(f.Current());shared.bodies.front().supportFaces.front().boundaryEdges.front().edge=TopoDS::Edge(e.Current());auto sharedRef=ref;sharedRef["edgeId"]=shared.bodies.front().supportFaces.front().boundaryEdges.front().edgeId;bool rejected=false;try{SurfaceExtend::resolveSource(shared,sharedRef);}catch(const GeometryKernel::EvaluationError& ex){rejected=ex.diagnostics().value("code")=="ambiguous_surface_source";}check(rejected,"shared solid edge refusal");tested=true;}
    for(const auto& edge:face.boundaryEdges){auto other=ref;other["edgeId"]=edge.edgeId;const auto result=CadDocument::evaluateCandidate(CadDocument::surfaceExtendCandidate(baseline,{{"sourceEdge",other}}));check(result.ok,result.error);const BRepAdaptor_Curve c(edge.edge);const auto length=c.Value(c.FirstParameter()).Distance(c.Value(c.LastParameter()));near(result.evaluation->bodies.front().summary.value("areaMm2").toDouble(),1200+length*20);check(hash(source.shape)==sourceHash,"each side preserves source");}
    check(doc.commit(CadDocument::surfaceExtendCandidate(accepted,{{"featureId",id},{"distanceMm",25.}}),"Retry").isEmpty(),"valid retry");verify(45,25);doc.history().undo();verify(45,20);doc.history().redo();verify(45,25);
    QTemporaryDir tmp;const auto path=tmp.filePath("extend.alloy");check(doc.save(path).isEmpty(),"save");CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==doc.authored(),"reopen authored");near(reopened.bodies().front().summary.value("areaMm2").toDouble(),2475);check(reopened.bodies().front().bodyId==source.bodyId,"reopen identity");
    std::cout<<"PASS Natural Extend in-place rectangle, exact four-side lineage, immutable prior BRep, upstream width, typed/stale/shared/curved refusal, history/reopen\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
