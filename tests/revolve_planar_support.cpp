// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include "SolidShell.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QDir>
#include <BRepClass3d_SolidClassifier.hxx>
#include <BRepAlgoAPI_Check.hxx>
#include <BRepTools.hxx>
#include <sstream>
#include <IntCurvesFace_ShapeIntersector.hxx>
#include <gp_Lin.hxx>
#include <iostream>
#include <BRepGProp.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <GProp_GProps.hxx>
#include <QJsonDocument>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <cmath>
#include <numbers>
namespace {
void check(bool b,const QString& message){if(!b)throw std::runtime_error(message.toStdString());}
QJsonObject accept(const SketchGeometry::SolveResult& r){check(r.ok,r.error);return r.feature;}
std::string brep(const TopoDS_Shape& shape){std::ostringstream stream;BRepTools::Write(shape,stream);return stream.str();}
void closedCap(const EvaluatedBody& body,double expectedVolume){
 check(body.summary["shellPolicy"]=="exactCircularClosedCap","exact circular policy selected");
 check(body.supportFaces.size()==1&&body.supportFaces[0].role=="shell-closed-cap","closed cap role");
 check(BRepAlgoAPI_Check(body.shape,true,true).IsValid(),"closed cap Boolean argument valid");
 check(std::abs(body.summary["volumeMm3"].toDouble()-expectedVolume)<1e-5,"independent disk integral volume");
}
void thickness(const TopoDS_Shape& shape,double height,double radius){
 const double length=std::hypot(5.,height),offset=std::sqrt(radius*radius-length*length/4),cx=2.5-height/length*offset,cz=height/2-5/length*offset,azimuth=.4;
 for(double angle:{-.25,0.,.25}){auto point=[&](double r){const double x=cx+r*std::cos(angle);return gp_Pnt(x*std::cos(azimuth),x*std::sin(azimuth),cz+r*std::sin(angle));};IntCurvesFace_ShapeIntersector ray;ray.Load(shape,1e-7);ray.Perform(gp_Lin(point(radius),gp_Dir(-std::cos(angle)*std::cos(azimuth),-std::cos(angle)*std::sin(azimuth),-std::sin(angle))),0,10);check(ray.IsDone(),"normal intersection");double wall=100;for(int i=1;i<=ray.NbPnt();++i)if(ray.WParameter(i)>.01)wall=std::min(wall,ray.WParameter(i));check(std::abs(wall-5)<1e-5,"actual regular normal wall5mm");BRepClass3d_SolidClassifier material(shape,point(radius-2.5),1e-6),cavity(shape,point(radius-10),1e-6),outside(shape,point(radius+2),1e-6);check(material.State()==TopAbs_IN&&cavity.State()==TopAbs_OUT&&outside.State()==TopAbs_OUT,"material/cavity/exterior classification");}
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
 CadDocument document;auto sketch=SketchGeometry::newFeature(document.activeOwnerId(),"XZ","Radial profile");sketch=SketchGeometry::addLine(sketch,{{"u1Mm",0},{"v1Mm",0},{"u2Mm",0},{"v2Mm",80}});auto axis=sketch["parameters"].toObject()["entities"].toArray().last().toObject();sketch=SketchGeometry::addLine(sketch,{{"u1Mm",0},{"v1Mm",0},{"u2Mm",5},{"v2Mm",0},{"startPointId",axis["startPointId"]}});auto radial=sketch["parameters"].toObject()["entities"].toArray().last().toObject();
 const double length=std::hypot(5.,80.),offset=std::sqrt(2500-length*length/4),cx=2.5-80/length*offset,cy=40-5/length*offset,a=std::atan2(-cy,5-cx),b=std::atan2(80-cy,-cx);
 sketch=SketchGeometry::addArc(sketch,{{"centerUMm",cx},{"centerVMm",cy},{"radiusMm",50},{"startAngleDeg",a*180/std::numbers::pi},{"sweepAngleDeg",(b-a)*180/std::numbers::pi},{"startPointId",radial["endPointId"]},{"endPointId",axis["endPointId"]}});const auto arc=sketch["parameters"].toObject()["entities"].toArray().last().toObject();
 sketch=accept(SketchGeometry::addConstraint(sketch,{{"type","fixed"},{"pointIds",QJsonArray{axis["startPointId"]}}}));sketch=accept(SketchGeometry::addConstraint(sketch,{{"type","vertical"},{"entityIds",QJsonArray{axis["id"]}}}));sketch=accept(SketchGeometry::addConstraint(sketch,{{"type","horizontal"},{"entityIds",QJsonArray{radial["id"]}}}));
 sketch=accept(SketchGeometry::addDimension(sketch,{{"kind","linear"},{"entityId",axis["id"]},{"valueMm",80}}));const auto heightId=sketch["parameters"].toObject()["dimensions"].toArray().last().toObject()["id"].toString();sketch=accept(SketchGeometry::addDimension(sketch,{{"kind","linear"},{"entityId",radial["id"]},{"valueMm",5}}));sketch=accept(SketchGeometry::addDimension(sketch,{{"kind","radius"},{"entityId",arc["id"]},{"valueMm",50}}));const auto radiusId=sketch["parameters"].toObject()["dimensions"].toArray().last().toObject()["id"].toString();check(document.commit(SketchGeometry::replaceFeature(document.authored(),sketch),"Source").isEmpty(),"source commit");
 check(document.commit(CadDocument::revolveCandidate(document.authored(),{{"sketchId",sketch["id"]},{"profileId",document.sketches()[0].profiles[0].profileId},{"axisEntityId",axis["id"]}}),"Full Revolve").isEmpty(),"full revolve");auto body=document.bodies()[0];check(body.supportFaces.size()==1,"one unique generated radial disk");const auto face=body.supportFaces[0];check(face.role=="revolve.radial"&&face.provenance["sourceEntityId"]==radial["id"],"actual retained radial generator lineage");check(face.frame["normal"].toArray()[2].toDouble()<-.99999,"actual bottom disk outward orientation");QJsonObject support{{"producerFeatureId",face.producerFeatureId},{"bodyId",face.bodyId},{"faceId",face.faceId}};const auto full=document.authored();const auto evidence=qEnvironmentVariable("ALLOY_REVOLVE_SUPPORT_EVIDENCE");if(!evidence.isEmpty()){QDir().mkpath(evidence);check(document.save(evidence+"/source-full-revolve.alloy").isEmpty(),"save exact full source for native ingredient");}
 const auto originalBrep=brep(body.shape);auto evaluated=body;SolidShell::evaluate(evaluated,{{"removedFace",support},{"thicknessMm",5}},CadDocument::newId());check(brep(body.shape)==originalBrep,"exact original BRep preserved by closed cavity Cut");closedCap(evaluated,37836.66233494);
 auto ambiguous=body;ambiguous.supportFaces.push_back(face);bool refused=false;try{SolidShell::evaluate(ambiguous,{{"removedFace",support},{"thicknessMm",5}},CadDocument::newId());}catch(const GeometryKernel::EvaluationError& e){refused=e.diagnostics()["code"]=="shell_face_ambiguous";}check(refused,"ambiguous removed-face lineage refusal");
 check(document.commit(CadDocument::shellCandidate(full,{{"targetBodyId",body.bodyId},{"removedFace",support},{"thicknessMm",5}}),"Inside Shell5").isEmpty(),document.lastEvaluationError()["message"].toString());check(document.bodies()[0].summary["valid"].toBool()&&document.bodies()[0].summary["closed"].toBool(),"real valid hollow solid");TopTools_IndexedMapOfShape rimWires;TopExp::MapShapes(document.bodies()[0].supportFaces[0].face,TopAbs_WIRE,rimWires);check(rimWires.Extent()==1,"actual retained closed bottom cap");BRepClass3d_SolidClassifier opening(document.bodies()[0].shape,gp_Pnt(0,0,.1),1e-6);check(opening.State()==TopAbs_IN,"closed cap retains material below the exact cavity");closedCap(document.bodies()[0],37836.66233494);thickness(document.bodies()[0].shape,80,50);const auto before=document.authored();const auto volume=document.bodies()[0].summary["volumeMm3"].toDouble();
 auto edit=accept(SketchGeometry::updateDimension(SketchGeometry::findFeature(before,sketch["id"].toString()),{{"dimensionId",heightId},{"valueMm",85}}));edit=accept(SketchGeometry::updateDimension(edit,{{"dimensionId",radiusId},{"valueMm",55}}));check(document.commit(SketchGeometry::replaceFeature(before,edit),"Height and radius edit").isEmpty(),"upstream edit through shell");check(document.bodies()[0].bodyId==body.bodyId&&document.bodies()[0].supportFaces[0].faceId==face.faceId&&document.bodies()[0].summary["volumeMm3"].toDouble()!=volume,"retained closed cap identity and changed geometry");closedCap(document.bodies()[0],39907.16395561);thickness(document.bodies()[0].shape,85,55);const auto after=document.authored();document.history().undo();check(document.authored()==before,"exact upstream undo");document.history().redo();check(document.authored()==after,"exact upstream redo");
 auto partial=CadDocument::revolveCandidate(after,{{"featureId",body.featureId},{"sketchId",sketch["id"]},{"profileId",document.sketches()[0].profiles[0].profileId},{"axisEntityId",axis["id"]},{"extent","partial"},{"angleDeg",90}});check(!document.commit(partial,"Missing full radial lineage").isEmpty()&&document.lastEvaluationError()["code"]=="shell_face_unavailable"&&document.authored()==after,"missing generated full-Revolve descendant refuses dependent Shell atomically");
 auto bad=support;bad["faceId"]=CadDocument::newId();check(!document.commit(CadDocument::shellCandidate(after,{{"targetBodyId",body.bodyId},{"removedFace",bad},{"thicknessMm",5}}),"Missing disk").isEmpty()&&document.authored()==after,"missing face refuses atomically");QTemporaryDir dir;const auto path=dir.filePath("revolve-shell.alloy");check(document.save(path).isEmpty(),"save");CadDocument reopened;check(reopened.open(path).isEmpty()&&reopened.authored()==after,"exact reopen");check(reopened.bodies()[0].supportFaces[0].faceId==face.faceId,"reopen lineage");closedCap(reopened.bodies()[0],39907.16395561);thickness(reopened.bodies()[0].shape,85,55);std::cout<<"actual80/5/R50 Full Revolve/Shell5, measured wall, generated lineage, edits, refusal/history/reopen passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
