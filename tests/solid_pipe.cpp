// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QJsonDocument>
#include <BRepClass3d_SolidClassifier.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <TopExp_Explorer.hxx>
#include <numbers>
#include <cmath>
#include <iostream>
namespace {
void check(bool ok,const QString& message){if(!ok)throw std::runtime_error(message.toStdString());}
QJsonObject sources(CadDocument& d,bool clip=false){
 auto path=SketchGeometry::newFeature(d.activeOwnerId(),"XY","Retained Pipe path");
 auto line=[&](double x,double y,double xx,double yy){path=SketchGeometry::addLine(path,{{"u1Mm",x},{"v1Mm",y},{"u2Mm",xx},{"v2Mm",yy}});};
 auto arc=[&](double x,double y,double r,double angle,double sweep){path=SketchGeometry::addArc(path,{{"centerUMm",x},{"centerVMm",y},{"radiusMm",r},{"startAngleDeg",angle},{"sweepAngleDeg",sweep}});};
 if(clip){line(0,0,0,33);arc(7,33,7,180,-90);arc(7,33,7,90,-90);line(14,33,14,-14.5);arc(8.5,-14.5,5.5,0,-90);arc(8.5,-14.5,5.5,-90,-90);line(3,-14.5,3,20);arc(7,20,4,180,-90);arc(7,20,4,90,-90);line(11,20,11,0);}else{line(0,0,0,10);arc(10,10,10,180,-90);}
 check(d.commit(SketchGeometry::replaceFeature(d.authored(),path),"Path").isEmpty(),"source");
 QJsonArray ids;for(const auto& q:path["parameters"].toObject()["entities"].toArray())ids.append(q.toObject()["id"]);
 return {{"pathSketchId",path["id"]},{"pathEntityIds",ids},{"pathStartPointId",path["parameters"].toObject()["entities"].toArray().first().toObject()["startPointId"]},{"diameterMm",4.},{"wallThicknessMm",1.}};
}
void sample(const EvaluatedBody& body,const gp_Pnt& p,TopAbs_State expected){BRepClass3d_SolidClassifier test(body.shape,p,1e-7);check(test.State()==expected,"tube section sample");}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
 CadDocument d;auto args=sources(d);const auto before=d.authored();auto draft=CadDocument::pipeCandidate(before,args);const auto preview=CadDocument::evaluateCandidate(draft);check(preview.ok,preview.error+QString::fromUtf8(QJsonDocument(preview.diagnostics).toJson()));check(d.authored()==before && d.bodies().empty(),"detached preview");check(d.commit(draft,"Pipe").isEmpty(),"commit");
 const auto body=d.bodies().front();const auto summary=body.summary;const double length=10+5*std::numbers::pi;check(summary["valid"].toBool() && summary["closed"].toBool() && summary["topology"].toObject()["solids"]==1,"one closed valid solid");check(std::abs(summary["pathLengthMm"].toDouble()-length)<1e-7 && std::abs(summary["volumeMm3"].toDouble()-3*std::numbers::pi*length)<1e-5,"analytic length and annular volume");
 sample(body,{0,5,.5},TopAbs_OUT);sample(body,{0,5,1.5},TopAbs_IN);sample(body,{0,5,2.5},TopAbs_OUT);const double v=std::sqrt(.5)*10;sample(body,{10-v,10+v,.5},TopAbs_OUT);sample(body,{10-v,10+v,1.5},TopAbs_IN);sample(body,{10-v,10+v,2.5},TopAbs_OUT);
 check(body.supportFaces.size()==2,"builder end lineage");for(const auto& cap:body.supportFaces){int wires=0;for(TopExp_Explorer e(cap.face,TopAbs_WIRE);e.More();e.Next())++wires;GProp_GProps area;BRepGProp::SurfaceProperties(cap.face,area);check(wires==2 && std::abs(area.Mass()-3*std::numbers::pi)<1e-6,"annular open ends");}
 const auto accepted=d.authored();d.history().undo();check(d.authored()==before,"undo");d.history().redo();check(d.authored()==accepted,"redo");args["featureId"]=body.featureId;args["diameterMm"]=5.;args["wallThicknessMm"]=.75;check(d.commit(CadDocument::pipeCandidate(d.authored(),args),"Edit").isEmpty(),"parameter edit");check(d.bodies().front().bodyId==body.bodyId && d.bodies().front().supportFaces.front().faceId==body.supportFaces.front().faceId,"edit identity");
 auto edited=d.authored();auto features=edited["features"].toArray();auto path=features[0].toObject();auto p=path["parameters"].toObject();auto points=p["points"].toArray();for(int i=0;i<points.size();++i){auto q=points[i].toObject();if(q["id"]==args["pathStartPointId"]){q["vMm"]=-2.;points[i]=q;}}p["points"]=points;path["parameters"]=p;features[0]=path;edited["features"]=features;check(d.commit(edited,"Edit source").isEmpty(),"line source recompute");check(std::abs(d.bodies().front().summary["pathLengthMm"].toDouble()-length-2)<1e-7,"source length");
 auto atomic=[&](const QJsonObject& candidate){const auto state=d.authored();const auto revision=d.revision();const QJsonValue undo=d.state()["undo"];bool failed=false;try{failed=!d.commit(candidate,"Refuse").isEmpty();}catch(const std::exception&){failed=true;}check(failed && d.authored()==state && d.revision()==revision && d.state()["undo"]==undo,"transactional refusal");};
 auto tight=args;tight["diameterMm"]=22.;atomic(CadDocument::pipeCandidate(d.authored(),tight));check(d.lastEvaluationError()["code"]=="pipe_bend_too_tight","tight bend diagnostic");
 auto bad=args;bad["pathEntityIds"]=QJsonArray{CadDocument::newId()};atomic(CadDocument::pipeCandidate(d.authored(),bad));bad=args;auto reverse=args["pathEntityIds"].toArray();auto first=reverse.takeAt(0);reverse.append(first);bad["pathEntityIds"]=reverse;atomic(CadDocument::pipeCandidate(d.authored(),bad));
 for(const auto& pair:QList<QPair<double,double>>{{0,1},{-1,1},{4,0},{4,-1},{4,2},{4,3}}){bad=args;bad["diameterMm"]=pair.first;bad["wallThicknessMm"]=pair.second;bool refused=false;try{CadDocument::pipeCandidate(d.authored(),bad);}catch(const std::exception&){refused=true;}check(refused,"invalid section schema refusal");}
 // A local regular section can still collide with a distant leg. The whole
 // B-rep interference check must refuse it, rather than relying on bend radius.
 CadDocument collision;auto collisionArgs=sources(collision,true);check(!collision.commit(CadDocument::pipeCandidate(collision.authored(),collisionArgs),"Collision").isEmpty(),"global overlap refusal");check(collision.lastEvaluationError()["code"]=="pipe_self_intersection","global interference diagnostic");check(collision.bodies().empty(),"collision atomic");
 QTemporaryDir tmp;check(d.save(tmp.filePath("pipe.alloy")).isEmpty(),"save");CadDocument reopened;check(reopened.open(tmp.filePath("pipe.alloy")).isEmpty() && reopened.authored()==d.authored(),"reopen");
 std::cout<<"PASS hollow Line/Arc Pipe: analytic sections/volume/annular ends, identity, source edits, history/storage and atomic overlap refusal\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
