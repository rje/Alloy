// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <BRepAdaptor_Curve.hxx>
#include <BRep_Tool.hxx>
#include <TopExp.hxx>
#include <TopoDS.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <iostream>
#include <cmath>
void check(bool ok,const char* m){if(!ok)throw std::runtime_error(m);}
int main(int argc,char**argv){QCoreApplication app(argc,argv);try{
 CadDocument d;auto s=SketchGeometry::newFeature(d.authored()["documentId"].toString(),"XY","Stock");s=SketchGeometry::addRectangle(s,{{"u1Mm",0},{"v1Mm",0},{"u2Mm",30},{"v2Mm",20}});
 auto commit=[&](const QJsonObject& c){auto e=d.commit(c,"Fixture");if(!e.isEmpty())throw std::runtime_error(e.toStdString());};commit(SketchGeometry::replaceFeature(d.authored(),s));QJsonObject ex{{"sketchId",s["id"]},{"profileId",d.sketches()[0].profiles[0].profileId},{"distanceMm",10}};commit(CadDocument::extrusionCandidate(d.authored(),ex));const auto source=d.authored();const auto b=d.bodies()[0];QString edge;EvaluatedSupportFace top;
 for(const auto& e:b.solidEdges){BRepAdaptor_Curve c(e.edge);auto a=c.Value(c.FirstParameter()),z=c.Value(c.LastParameter());if(std::abs(a.Y())<1e-7 && std::abs(z.Y())<1e-7 && std::abs(a.Z()-10)<1e-7 && std::abs(z.Z()-10)<1e-7)edge=e.edgeId;}
 for(const auto& f:b.supportFaces)if(f.frame["normal"].toArray()[2].toDouble()>.99)top=f;
 check(!edge.isEmpty() && !top.faceId.isEmpty(),"exact fixture edge and top");QJsonObject args{{"targetBodyId",b.bodyId},{"edgeIds",QJsonArray{edge}},{"mode","twoDistance"},{"distanceOneMm",2},{"distanceTwoMm",1},{"flip",false},{"referenceFace",QJsonObject{{"bodyId",top.bodyId},{"producerFeatureId",top.producerFeatureId},{"faceId",top.faceId}}}};
 for(bool flip:{false,true}){args["flip"]=flip;const auto p=CadDocument::evaluateCandidate(CadDocument::chamferCandidate(source,args));if(!p.ok)throw std::runtime_error(p.error.toStdString());const auto& result=p.evaluation->bodies[0];check(std::abs(result.summary["volumeMm3"].toDouble()-5970)<1e-6,"exact unequal bevel volume");TopTools_IndexedMapOfShape vertices;TopExp::MapShapes(result.shape,TopAbs_VERTEX,vertices);bool topSetback=false,frontSetback=false;for(int i=1;i<=vertices.Extent();++i){auto q=BRep_Tool::Pnt(TopoDS::Vertex(vertices(i)));if(std::abs(q.X())<1e-7){topSetback|=std::abs(q.Y()-(flip?1:2))<1e-7 && std::abs(q.Z()-10)<1e-7;frontSetback|=std::abs(q.Y())<1e-7 && std::abs(q.Z()-(flip?8:9))<1e-7;}}check(topSetback && frontSetback,"Flip swaps exact side setbacks");check(d.authored()==source && std::abs(GeometryKernel::summarize(d.bodies()[0].shape)["volumeMm3"].toDouble()-6000)<1e-6,"preview preserves authored and exact source");}
 // Explicit reference identity must be incident; a valid UUID alone is insufficient.
 {auto bad=args;for(const auto& f:b.supportFaces)if(f.frame["normal"].toArray()[2].toDouble()<-.99)bad["referenceFace"]=QJsonObject{{"bodyId",f.bodyId},{"producerFeatureId",f.producerFeatureId},{"faceId",f.faceId}};auto rejected=CadDocument::evaluateCandidate(CadDocument::chamferCandidate(source,bad));check(!rejected.ok && rejected.diagnostics["code"]=="chamfer_reference_face_unavailable","nonincident retained face refused");}
 args["flip"]=false;commit(CadDocument::chamferCandidate(source,args));const auto created=d.authored();args["featureId"]=created["features"].toArray().last().toObject()["id"];args["distanceTwoMm"]=3;commit(CadDocument::chamferCandidate(d.authored(),args));check(std::abs(d.bodies()[0].summary["volumeMm3"].toDouble()-5910)<1e-6,"independent edit");d.history().undo();check(d.authored()==created,"edit undo");d.history().redo();const auto stable=d.authored();auto rev=d.revision();auto count=d.history().count();
 for(double bad:{0.,-1.,100.}){args["distanceOneMm"]=bad;bool refused=false;try{refused=!d.commit(CadDocument::chamferCandidate(stable,args),"Bad").isEmpty();}catch(const std::exception&){refused=true;}check(refused && d.authored()==stable && d.revision()==rev && d.history().count()==count,"atomic invalid refusal");}
 QTemporaryDir temp;auto file=temp.filePath("chamfer.alloy");check(d.save(file).isEmpty(),"save");CadDocument reopened;check(reopened.open(file).isEmpty() && reopened.authored()==stable,"retained reopen");auto fs=stable["features"].toArray(); // upstream height remains an independent retained extrusion input
 ex["featureId"]=fs[1].toObject()["id"];ex["distanceMm"]=12;commit(CadDocument::extrusionCandidate(stable,ex));check(std::abs(d.bodies()[0].summary["volumeMm3"].toDouble()-7110)<1e-6,"upstream retained edge recompute");
 std::cout<<"Two Distance Chamfer exact sides/Flip/edit/history/reopen/refusal passed\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
