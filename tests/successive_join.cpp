// SPDX-License-Identifier: MIT
#include "SketchGeometry.h"
#include <QDir>
#include "CadDocument.h"
#include <QTemporaryDir>
#include <QSet>
#include "SketchProfiles.h"
#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QUuid>
#include <cmath>
#include <iostream>
#include <numbers>
#include <stdexcept>
void check(bool yes,const QString& message){if(!yes)throw std::runtime_error(message.toStdString());}
QJsonObject line(QJsonObject f,double x1,double y1,double x2,double y2){return SketchGeometry::addLine(f,{{"u1Mm",x1},{"v1Mm",y1},{"u2Mm",x2},{"v2Mm",y2}});}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
 auto owner=QUuid::createUuid().toString(QUuid::WithoutBraces);auto f=SketchGeometry::newFeature(owner,"XY","Straw support outline");
 const double crossing=7.5+std::sqrt(31.25);
 f=line(f,0,0,15-crossing,0);f=line(f,0,-10,15-crossing,-10);
 f=line(f,crossing,0,32,0);f=line(f,32,0,32,-10);f=line(f,32,-10,crossing,-10);
 f=SketchGeometry::addCircle(f,{{"centerUMm",7.5},{"centerVMm",-5},{"diameterMm",15}});
 for(int x:{23,26,29})f=SketchGeometry::addRectangle(f,{{"u1Mm",x},{"v1Mm",0},{"u2Mm",x+3},{"v2Mm",-10}});
 CadDocument document;f["ownerId"]=document.authored()["documentId"];
 auto error=document.commit(SketchGeometry::replaceFeature(document.authored(),f),"Post-Trim fixture");check(error.isEmpty(),error);
 QString disk,bridge,inner,middle,outer;double bridgeArea=0;
 for(const auto& p:document.sketches()[0].profiles){const double area=p.summary["areaMm2"].toDouble();const double x=GeometryKernel::summarize(p.face)["boundsMm"].toObject()["min"].toArray().at(0).toDouble();
  if(std::abs(area-std::numbers::pi*56.25)<1e-5)disk=p.profileId;
  else if(std::abs(area-30)<1e-5){if(std::abs(x-23)<1e-4)inner=p.profileId;else if(std::abs(x-26)<1e-4)middle=p.profileId;else if(std::abs(x-29)<1e-4)outer=p.profileId;}
  else {bridge=p.profileId;bridgeArea=area;}}
 check(!disk.isEmpty() && !bridge.isEmpty() && !inner.isEmpty() && !middle.isEmpty() && !outer.isEmpty(),"Five source partitions unavailable");
 auto extrude=[&](QJsonArray ids,double depth,QJsonArray targets={}){QJsonObject args{{"sketchId",f["id"]},{"profileIds",ids},{"distanceMm",depth}};if(!targets.isEmpty()){args["operation"]="join";args["targetBodyIds"]=targets;}auto candidate=CadDocument::extrusionCandidate(document.authored(),args);auto prepared=CadDocument::evaluateCandidate(candidate);check(prepared.ok,prepared.error);check(document.commit(prepared.authored,"Source Extrude").isEmpty(),"Commit source extrusion");return document.authored()["features"].toArray().last().toObject();};
 auto base=extrude({disk},20);const auto b0=document.bodies()[0].bodyId;const auto originalCap=document.bodies()[0].supportFaces[0];
 auto joined=extrude({bridge,middle},4,{b0});check(document.bodies().size()==2 && joined["bodyResults"].toArray().size()==2,"Intermediate disconnected Join must author two connected bodies");
 QString b1;for(const auto& body:document.bodies())if(body.bodyId!=b0)b1=body.bodyId;check(!b1.isEmpty(),"Detached output identity unavailable");
 const auto intermediate=document.authored();document.history().undo();check(document.bodies().size()==1,"Undo created result");document.history().redo();check(document.authored()==intermediate,"Redo reserved ownership");
 extrude({outer},24,{b1});check(document.bodies().size()==2,"Outer joins only the separate middle result");
 auto final=extrude({inner},22,{b0,b1});check(document.bodies().size()==1 && document.bodies()[0].bodyId==b0 && document.bodies()[0].featureId==base["id"].toString(),"Merge retains first participant/producer and consumes other result");
 const double expected=std::numbers::pi*56.25*20+bridgeArea*4+30*(4+24+22);
 check(std::abs(document.bodies()[0].summary["volumeMm3"].toDouble()-expected)<1e-5,"Source heights are absolute from sketch plane");
 bool diskCap=false;for(const auto& cap:document.bodies()[0].supportFaces)if(cap.faceId==originalCap.faceId && cap.producerFeatureId==originalCap.producerFeatureId && cap.frame["origin"].toArray()[2].toDouble()==20)diskCap=true;check(diskCap,"Original exact disk cap must survive successive Join");
 bool prongRim=false;for(const auto& edge:document.bodies()[0].solidEdges)if(!edge.edge.IsNull() && edge.provenance["role"]=="endRim")prongRim=true;check(prongRim,"Exact prism rim lineage unavailable");
 auto stable=document.authored();QJsonObject edit{{"featureId",joined["id"]},{"sketchId",f["id"]},{"profileIds",QJsonArray{bridge,middle}},{"distanceMm",5}};
 check(document.commit(CadDocument::extrusionCandidate(stable,edit),"Edit intermediate Join").isEmpty(),"Depth edit within connectivity");check(document.bodies().size()==1 && document.bodies()[0].bodyId==b0,"Depth edit changed retained output identity");
 document.history().undo();check(document.authored()==stable,"Undo depth edit");document.history().redo();document.history().undo();
 auto revision=document.revision();edit["distanceMm"]=-4;error=document.commit(CadDocument::extrusionCandidate(stable,edit),"Connectivity change");check(!error.isEmpty(),"Connectivity/no-addition change accepted");check(document.authored()==stable && document.revision()==revision,"Connectivity refusal mutated authored state");
 auto stale=QJsonObject{{"sketchId",f["id"]},{"profileId",outer},{"distanceMm",25},{"operation","join"},{"targetBodyIds",QJsonArray{b1}}};auto failed=CadDocument::evaluateCandidate(CadDocument::extrusionCandidate(stable,stale));check(!failed.ok && failed.diagnostics["code"]=="consumed_dependency","Consumed output reference accepted");
 QTemporaryDir folder;check(document.save(folder.path()+"/support.alloy").isEmpty(),"Save");CadDocument opened;check(opened.open(folder.path()+"/support.alloy").isEmpty() && opened.authored()==stable && opened.bodies()[0].bodyId==b0,"Reopen ownership");
 if(argc>1){QDir().mkpath(argv[1]);check(document.save(QString::fromLocal8Bit(argv[1])+"/support-successive.alloy").isEmpty(),"Evidence fixture save");}
 std::cout<<"PASS: source 20/4/24/22, connected output ledger, exact original cap, retained depth edit/history/storage, atomic refusal and consumed reference\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
