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
 auto commit=[&](const QJsonObject& c){auto e=d.commit(c,"Fixture");if(!e.isEmpty())throw std::runtime_error(e.toStdString());};commit(SketchGeometry::replaceFeature(d.authored(),s));QJsonObject ex{{"sketchId",s["id"]},{"profileId",d.sketches()[0].profiles[0].profileId},{"distanceMm",10}};commit(CadDocument::extrusionCandidate(d.authored(),ex));const auto source=d.authored();const auto b=d.bodies()[0];QString edge;EvaluatedSupportFace top,front;
 for(const auto& e:b.solidEdges){BRepAdaptor_Curve c(e.edge);auto a=c.Value(c.FirstParameter()),z=c.Value(c.LastParameter());if(std::abs(a.Y())<1e-7 && std::abs(z.Y())<1e-7 && std::abs(a.Z()-10)<1e-7 && std::abs(z.Z()-10)<1e-7)edge=e.edgeId;}
 for(const auto& f:b.supportFaces){if(f.frame["normal"].toArray()[2].toDouble()>.99)top=f;if(f.frame["normal"].toArray()[1].toDouble()<-.99)front=f;}
 check(!edge.isEmpty() && !top.faceId.isEmpty(),"exact fixture edge and top");QJsonObject args{{"targetBodyId",b.bodyId},{"edgeIds",QJsonArray{edge}},{"mode","distanceAngle"},{"distanceMm",2},{"angleDeg",30},{"referenceFace",QJsonObject{{"bodyId",top.bodyId},{"producerFeatureId",top.producerFeatureId},{"faceId",top.faceId}}}};
 for(bool flip:{false,true}){const auto& reference=flip?front:top;args["referenceFace"]=QJsonObject{{"bodyId",reference.bodyId},{"producerFeatureId",reference.producerFeatureId},{"faceId",reference.faceId}};const auto p=CadDocument::evaluateCandidate(CadDocument::chamferCandidate(source,args));if(!p.ok)throw std::runtime_error(p.error.toStdString());const auto& result=p.evaluation->bodies[0];check(std::abs(result.summary["volumeMm3"].toDouble()-(6000-60/std::tan(std::acos(-1.)/6)))<1e-6,"exact unequal bevel volume");TopTools_IndexedMapOfShape vertices;TopExp::MapShapes(result.shape,TopAbs_VERTEX,vertices);bool topSetback=false,frontSetback=false;for(int i=1;i<=vertices.Extent();++i){auto q=BRep_Tool::Pnt(TopoDS::Vertex(vertices(i)));if(std::abs(q.X())<1e-7){topSetback|=std::abs(q.Y()-(flip?2/std::tan(std::acos(-1.)/6):2))<1e-7 && std::abs(q.Z()-10)<1e-7;frontSetback|=std::abs(q.Y())<1e-7 && std::abs(q.Z()-(flip?8:10-2/std::tan(std::acos(-1.)/6)))<1e-7;}}check(topSetback && frontSetback,"reference face swaps exact 2mm/30degree setbacks");check(d.authored()==source && std::abs(GeometryKernel::summarize(d.bodies()[0].shape)["volumeMm3"].toDouble()-6000)<1e-6,"preview preserves authored and exact source");}
 // Explicit reference identity must be incident; a valid UUID alone is insufficient.
 {auto bad=args;for(const auto& f:b.supportFaces)if(f.frame["normal"].toArray()[2].toDouble()<-.99)bad["referenceFace"]=QJsonObject{{"bodyId",f.bodyId},{"producerFeatureId",f.producerFeatureId},{"faceId",f.faceId}};auto rejected=CadDocument::evaluateCandidate(CadDocument::chamferCandidate(source,bad));check(!rejected.ok && rejected.diagnostics["code"]=="chamfer_reference_face_unavailable","nonincident retained face refused");}
 args["referenceFace"]=QJsonObject{{"bodyId",top.bodyId},{"producerFeatureId",top.producerFeatureId},{"faceId",top.faceId}};commit(CadDocument::chamferCandidate(source,args));const auto created=d.authored();args["featureId"]=created["features"].toArray().last().toObject()["id"];args["angleDeg"]=45;commit(CadDocument::chamferCandidate(d.authored(),args));check(std::abs(d.bodies()[0].summary["volumeMm3"].toDouble()-5940)<1e-6,"independent edit");d.history().undo();check(d.authored()==created,"edit undo");d.history().redo();const auto stable=d.authored();auto rev=d.revision();auto count=d.history().count();
 for(double bad:{0.,-1.,100.}){args["distanceMm"]=bad;bool refused=false;try{refused=!d.commit(CadDocument::chamferCandidate(stable,args),"Bad").isEmpty();}catch(const std::exception&){refused=true;}check(refused && d.authored()==stable && d.revision()==rev && d.history().count()==count,"atomic invalid refusal");}
 for(double bad:{-30.,0.,90.,100.}){auto invalid=args;invalid["distanceMm"]=2;invalid["angleDeg"]=bad;bool refused=false;try{refused=!d.commit(CadDocument::chamferCandidate(stable,invalid),"Bad angle").isEmpty();}catch(const std::exception&){refused=true;}check(refused && d.authored()==stable && d.revision()==rev && d.history().count()==count,"atomic invalid angle refusal");}
 QTemporaryDir temp;auto file=temp.filePath("chamfer.alloy");check(d.save(file).isEmpty(),"save");CadDocument reopened;check(reopened.open(file).isEmpty() && reopened.authored()==stable,"retained reopen");auto fs=stable["features"].toArray(); // upstream height remains an independent retained extrusion input
 ex["featureId"]=fs[1].toObject()["id"];ex["distanceMm"]=12;commit(CadDocument::extrusionCandidate(stable,ex));check(std::abs(d.bodies()[0].summary["volumeMm3"].toDouble()-7140)<1e-6,"upstream retained edge recompute");
 std::cout<<"Distance And Angle Chamfer exact 2mm/30degree sides/reference/edit/history/reopen/refusal passed\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
