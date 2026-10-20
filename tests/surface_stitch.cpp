// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include "SurfaceStitch.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QJsonDocument>
#include <QCryptographicHash>
#include <BRepTools.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRep_Tool.hxx>
#include <BRep_Builder.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopoDS.hxx>
#include <sstream>
#include <iostream>
#include <cmath>
namespace {
void check(bool b,const QString& message){if(!b)throw std::runtime_error(message.toStdString());}
void near(double a,double b,double eps=1e-7){check(std::abs(a-b)<eps,QString("%1 != %2").arg(a,0,'g',14).arg(b,0,'g',14));}
QByteArray hash(const TopoDS_Shape& s){std::ostringstream out;BRepTools::Write(s,out);return QCryptographicHash::hash(QByteArray::fromStdString(out.str()),QCryptographicHash::Sha256);}
QJsonObject sheet(CadDocument& doc,double x,double y){auto sketch=SketchGeometry::addRectangle(SketchGeometry::newFeature(doc.authored().value("documentId").toString(),"XY","20 x 10 sheet boundary"),{{"u1Mm",x},{"v1Mm",y},{"u2Mm",x+20},{"v2Mm",y+10}});check(doc.commit(SketchGeometry::replaceFeature(doc.authored(),sketch),"Boundary").isEmpty(),"sketch");const auto profile=doc.evaluation().sketches.back().profiles.front().profileId;check(doc.commit(CadDocument::surfacePatchCandidate(doc.authored(),{{"sketchId",sketch.value("id")},{"profileId",profile}}),"Sheet").isEmpty(),"sheet");return sketch;}
QJsonObject ref(const EvaluatedBody& b){const auto& f=b.supportFaces.front();return {{"bodyId",b.bodyId},{"producerFeatureId",f.producerFeatureId},{"faceId",f.faceId}};}
void verify(const EvaluatedBody& b,double gap){check(BRepCheck_Analyzer(b.shape).IsValid() && b.shape.ShapeType()==TopAbs_SHELL && !BRep_Tool::IsClosed(b.shape),"independent valid open shell");GProp_GProps properties;BRepGProp::SurfaceProperties(b.shape,properties);near(properties.Mass(),400);Bnd_Box bounds;BRepBndLib::AddOptimal(b.shape,bounds,false,false);double x0,y0,z0,x1,y1,z1;bounds.Get(x0,y0,z0,x1,y1,z1);near(x0,0);near(y0,0);near(z0,0);near(x1,20);near(y1,20+gap);near(z1,0);TopTools_IndexedDataMapOfShapeListOfShape map;TopExp::MapShapesAndAncestors(b.shape,TopAbs_EDGE,TopAbs_FACE,map);int free=0,sewn=0;for(int i=1;i<=map.Extent();++i){if(map.FindFromIndex(i).Extent()==1)++free;if(map.FindFromIndex(i).Extent()==2)++sewn;}check(free==6 && sewn==1,"independent seam adjacency");const auto s=b.summary,t=s.value("topology").toObject();check(t.value("faces")==2 && t.value("shells")==1 && t.value("solids")==0 && t.value("edges")==7 && !s.value("isSolid").toBool() && s.value("shellClassification")=="open","topology classification");near(s.value("measuredSeamGapMm").toDouble(),gap);near(s.value("areaMm2").toDouble(),400);check(s.value("maxOutputToleranceMm").toDouble()<=0.01+1e-12,"output tolerance limit");check(b.supportFaces.size()==2 && b.solidEdges.size()==7,"retained descendants");}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
for(const double gap:{0.,0.005,0.02}){
    CadDocument doc;sheet(doc,0,0);auto secondSketch=sheet(doc,0,10+gap);const auto sources=doc.bodies();const QJsonArray refs{ref(sources[0]),ref(sources[1])};const auto original=doc.authored();const auto hash0=hash(sources[0].shape),hash1=hash(sources[1].shape);const auto revision=doc.revision();const int history=doc.history().count();
    const auto candidate=CadDocument::surfaceStitchCandidate(original,{{"sourceFaces",refs},{"toleranceMm",0.01}});const auto feature=candidate.value("features").toArray().last().toObject();const auto preview=CadDocument::evaluateCandidate(candidate);
    check(doc.authored()==original && hash(sources[0].shape)==hash0 && hash(sources[1].shape)==hash1,"preview immutable sources");
    if(gap>0.01){check(!preview.ok && preview.diagnostics.value("code")=="stitch_gap_exceeded","excessive gap structured refusal: "+preview.error);near(preview.diagnostics.value("measuredSeamGapMm").toDouble(),gap);check(!doc.commit(candidate,"Refused stitch").isEmpty() && doc.authored()==original && doc.revision()==revision && doc.history().count()==history && doc.bodies().size()==2 && hash(doc.bodies()[0].shape)==hash0 && hash(doc.bodies()[1].shape)==hash1,"atomic excessive gap");continue;}
    check(preview.ok,preview.error+QString::fromUtf8(QJsonDocument(preview.diagnostics).toJson()));verify(preview.evaluation->bodies.front(),gap);check(doc.commit(candidate,"Stitch").isEmpty(),"commit stitch");check(doc.bodies().size()==1,"consume two sources after success");const auto result=doc.bodies().front();verify(result,gap);
    doc.history().undo();check(doc.authored()==original && doc.bodies().size()==2,"undo sources");doc.history().redo();verify(doc.bodies().front(),gap);
    const auto id=feature.value("id").toString();check(doc.commit(CadDocument::surfaceStitchCandidate(doc.authored(),{{"featureId",id},{"toleranceMm",0.009},{"name","Edited Stitch"}}),"Edit Stitch").isEmpty(),"retained edit");check(doc.bodies().front().bodyId==result.bodyId && doc.bodies().front().supportFaces.front().faceId==result.supportFaces.front().faceId,"retained identities");
    // Move every source point vertically; the seam and its retained IDs recompute.
    auto params=secondSketch.value("parameters").toObject();auto points=params.value("points").toArray();for(int i=0;i<points.size();++i){auto q=points[i].toObject();q["vMm"]=q.value("vMm").toDouble()+0.001;points[i]=q;}params["points"]=points;secondSketch["parameters"]=params;
    check(doc.commit(SketchGeometry::replaceFeature(doc.authored(),secondSketch),"Source gap edit").isEmpty(),"source edit");verify(doc.bodies().front(),gap+0.001);check(doc.bodies().front().supportFaces.front().faceId==result.supportFaces.front().faceId,"source-edit identity");
    const auto accepted=doc.authored();const auto resultHash=hash(doc.bodies().front().shape);const auto failed=CadDocument::surfaceStitchCandidate(accepted,{{"featureId",id},{"toleranceMm",0.0005}});const auto refusal=CadDocument::evaluateCandidate(failed);check(!refusal.ok && refusal.diagnostics.value("code")=="stitch_gap_exceeded","edit gap refusal");check(!doc.commit(failed,"Refused edit").isEmpty() && doc.authored()==accepted && hash(doc.bodies().front().shape)==resultHash,"atomic failed edit");
    QTemporaryDir temp;const auto path=temp.filePath("stitch.alloy");check(doc.save(path).isEmpty(),"save");CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==accepted,"reopen intent");verify(reopened.bodies().front(),gap+0.001);check(reopened.bodies().front().bodyId==result.bodyId && reopened.bodies().front().supportFaces.front().faceId==result.supportFaces.front().faceId,"reopen identity");
    if(argc>1 && gap==0.005)check(doc.save(QString::fromLocal8Bit(argv[1])).isEmpty(),"saved app fixture");
    auto stale=refs;auto bad=stale[1].toObject();bad["faceId"]=CadDocument::newId();stale[1]=bad;const auto staleResult=CadDocument::evaluateCandidate(CadDocument::surfaceStitchCandidate(original,{{"sourceFaces",stale}}));check(!staleResult.ok && staleResult.diagnostics.value("code")=="surface_source_unavailable","stale reference refusal");
    auto prior=doc.evaluationBefore(id);BRep_Builder builder;auto high=prior.bodies[0].solidEdges[0].edge;builder.UpdateEdge(high,0.02);bool refused=false;try{SurfaceStitch::evaluate(feature,prior);}catch(const GeometryKernel::EvaluationError& e){refused=e.diagnostics().value("code")=="stitch_input_tolerance_exceeded";}check(refused,"preexisting tolerance refusal");
}
for(const double x:{1.,20.}){CadDocument doc;sheet(doc,0,0);sheet(doc,x,10);const auto failed=CadDocument::evaluateCandidate(CadDocument::surfaceStitchCandidate(doc.authored(),{{"sourceFaces",QJsonArray{ref(doc.bodies()[0]),ref(doc.bodies()[1])}}}));check(!failed.ok,"partial or corner-only seam refusal");}
std::cout<<"PASS Stitch exact/subtolerance/excessive gap, independent shell adjacency, immutable sources, diagnostics, retained edit/source/history/reopen, partial/stale/input-tolerance refusals\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
