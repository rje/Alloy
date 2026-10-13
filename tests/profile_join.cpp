// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QJsonArray>
#include <QTemporaryDir>
#include <cmath>
#include <algorithm>
#include <numbers>
#include <iostream>
#include <stdexcept>
void check(bool ok,const QString& error){if(!ok)throw std::runtime_error(error.toStdString());}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
 CadDocument doc;auto owner=doc.authored()["documentId"].toString();
 auto circle=[&](QJsonObject sketch,double diameter){sketch=SketchGeometry::addCircle(sketch,{{"centerUMm",0},{"centerVMm",0},{"diameterMm",diameter}});check(doc.commit(SketchGeometry::replaceFeature(doc.authored(),sketch),"Circle").isEmpty(),"circle commit");return sketch;};
 auto base=circle(SketchGeometry::newFeature(owner,"XY","Bung flange"),26);
 auto baseArgs=QJsonObject{{"sketchId",base["id"]},{"profileId",doc.sketches()[0].profiles[0].profileId},{"distanceMm",3}};
 check(doc.commit(CadDocument::extrusionCandidate(doc.authored(),baseArgs),"Flange").isEmpty(),"base extrude");auto original=doc.bodies()[0];
 auto supported=[&](const QString& name){const auto& faces=doc.bodies()[0].supportFaces;const auto found=std::find_if(faces.rbegin(),faces.rend(),[](const auto& f){return f.role=="extrude.endCap";});check(found!=faces.rend(),"retained end cap");auto cap=*found;return SketchGeometry::newSupportedFeature(owner,{{"producerFeatureId",cap.producerFeatureId},{"bodyId",cap.bodyId},{"faceId",cap.faceId}},name,GeometryKernel::supportEdgeDescriptors(cap));};
 auto neck=circle(supported("Bung neck"),19);auto before=doc.authored();
 auto joinArgs=QJsonObject{{"sketchId",neck["id"]},{"profileId",doc.sketches()[1].profiles[0].profileId},{"distanceMm",8},{"operation","join"},{"targetBodyIds",QJsonArray{original.bodyId}}};
 auto error=doc.commit(CadDocument::extrusionCandidate(doc.authored(),joinArgs),"Join neck");check(error.isEmpty(),error);
 auto joined=doc.authored();auto joinFeature=joined["features"].toArray().last().toObject();
 check(doc.bodies().size()==1 && doc.bodies()[0].bodyId==original.bodyId && doc.bodies()[0].featureId==original.featureId && !joinFeature.contains("bodyId"),"join ownership");
 auto inspect=[&](double depth){auto body=doc.bodies()[0];check(body.summary["valid"].toBool() && body.summary["closed"].toBool() && body.summary["topology"].toObject()["solids"].toInt()==1,"join solid");check(std::abs(body.summary["volumeMm3"].toDouble()-std::numbers::pi*(169*3+90.25*depth))<1e-5,"join volume");check(std::any_of(body.supportFaces.begin(),body.supportFaces.end(),[&](const auto& f){return f.role=="extrude.endCap" && f.producerFeatureId==joinFeature["id"].toString();}),"join positive cap");};inspect(8);
 doc.history().undo();check(doc.authored()==before,"join undo");doc.history().redo();check(doc.authored()==joined,"join redo");
 auto tool=circle(supported("Straw tool"),6.2);auto toolArgs=QJsonObject{{"sketchId",tool["id"]},{"profileId",doc.sketches()[2].profiles[0].profileId},{"distanceMm",30}};
 check(doc.commit(CadDocument::extrusionCandidate(doc.authored(),toolArgs),"Straw tool").isEmpty(),"circle/extrude on Join cap");check(doc.bodies().size()==2,"New Body tool");
 joinArgs["featureId"]=joinFeature["id"];joinArgs["distanceMm"]=10;check(doc.commit(CadDocument::extrusionCandidate(doc.authored(),joinArgs),"Edit Join").isEmpty(),"Join upstream recompute");inspect(10);
 auto stable=doc.authored();auto revision=doc.revision();auto count=doc.history().count();auto invalid=joinArgs;invalid["targetBodyIds"]=QJsonArray{CadDocument::newId()};check(!doc.commit(CadDocument::extrusionCandidate(stable,invalid),"Missing target").isEmpty(),"missing target accepted");
 invalid=joinArgs;invalid["distanceMm"]=-1;check(!doc.commit(CadDocument::extrusionCandidate(stable,invalid),"Contained Join").isEmpty(),"contained join accepted");check(doc.authored()==stable && doc.revision()==revision && doc.history().count()==count,"atomic refusal");
 QTemporaryDir dir;check(doc.save(dir.path()+"/bung.alloy").isEmpty(),"save");CadDocument reopened;check(reopened.open(dir.path()+"/bung.alloy").isEmpty() && reopened.authored()==stable && reopened.bodies().size()==2,"reopen");
 std::cout<<"PASS: retained Join, positive cap circle tool, edit, history, atomic refusal and reopen\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
