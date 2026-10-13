// SPDX-License-Identifier: MIT
#include "SketchGeometry.h"
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
 auto regions=SketchProfiles::deriveRegions(f);check(regions.ok,regions.error);double area=0;
 for(const auto& p:regions.profiles){area+=p.summary["areaMm2"].toDouble();}
 check(regions.profiles.size()==5,"Expected post-Trim disk, bridge and three strips");
 double bandHalf=5*std::sqrt(31.25)+56.25*std::asin(2.0/3.0);double expected=std::numbers::pi*56.25+155-bandHalf+90;
 check(std::abs(area-expected)<1e-5,"Partition area differs from illustrated outline");

 QSet<QString> profileIds;QString disk,bridge;int strips=0;bool coincidentProvenance=false;
 for(const auto& profile:regions.profiles){profileIds.insert(profile.profileId);const auto a=profile.summary["areaMm2"].toDouble();
  if(std::abs(a-std::numbers::pi*56.25)<1e-5)disk=profile.profileId;
  else if(std::abs(a-30)<1e-5)++strips;else bridge=profile.profileId;
  for(const auto& edge:profile.summary["edgeProvenance"].toArray())if(edge.toObject()["sourceEntityIds"].toArray().size()>1)coincidentProvenance=true;
 }
 check(!disk.isEmpty() && !bridge.isEmpty() && strips==3 && coincidentProvenance,"Missing retained source-set partitions");
 auto reopened=QJsonDocument::fromJson(QJsonDocument(f).toJson()).object();QSet<QString> restored;
 for(const auto& profile:SketchProfiles::deriveRegions(reopened).profiles)restored.insert(profile.profileId);
 check(profileIds==restored,"Storage changed region IDs");
 auto reordered=f;auto parameters=f["parameters"].toObject();auto entities=parameters["entities"].toArray();QJsonArray reversed;
 for(int i=entities.size()-1;i>=0;--i)reversed.append(entities[i]);parameters["entities"]=reversed;reordered["parameters"]=parameters;
 QSet<QString> reordering;for(const auto& profile:SketchProfiles::deriveRegions(reordered).profiles)reordering.insert(profile.profileId);
 check(profileIds==reordering,"Authored entity order changed split provenance IDs");
 // The untrimmed source is deliberately a different atomic arrangement.
 auto raw=SketchGeometry::newFeature(owner,"XY","Untrimmed source");
 raw=SketchGeometry::addRectangle(raw,{{"u1Mm",0},{"v1Mm",0},{"u2Mm",32},{"v2Mm",-10}});
 raw=line(raw,0,-5,7.5,-5);raw=SketchGeometry::addCircle(raw,{{"centerUMm",7.5},{"centerVMm",-5},{"diameterMm",15}});
 for(int x:{23,26,29})raw=SketchGeometry::addRectangle(raw,{{"u1Mm",x},{"v1Mm",0},{"u2Mm",x+3},{"v2Mm",-10}});
 auto untrimmed=SketchProfiles::deriveRegions(raw);check(untrimmed.ok,untrimmed.error);check(untrimmed.profiles.size()==9,"Expected nine actual raw arrangement cells");
 double rawArea=0;for(const auto& profile:untrimmed.profiles)rawArea+=profile.summary["areaMm2"].toDouble();
 check(std::abs(rawArea-(320+std::numbers::pi*56.25-2*bandHalf))<1e-5,"Raw union region area mismatch");
 for(const auto& profile:untrimmed.profiles)check(std::abs(profile.summary["areaMm2"].toDouble()-std::numbers::pi*56.25)>1e-5,"Raw crossing sketch exposed fake full disk");
 // Dimension edits preserve a disk selection while changed bridge references
 // fail atomically rather than silently selecting a newly arranged face.
 CadDocument document;f["ownerId"]=document.authored()["documentId"];
 QString circleId,centerId;for(const auto& entity:f["parameters"].toObject()["entities"].toArray())if(entity.toObject()["type"]=="circle"){circleId=entity.toObject()["id"].toString();centerId=entity.toObject()["centerPointId"].toString();}
 auto fixed=SketchGeometry::addConstraint(f,{{"type","fixed"},{"pointIds",QJsonArray{centerId}}});check(fixed.ok,fixed.error);f=fixed.feature;
 auto dimensioned=SketchGeometry::addDimension(f,{{"kind","diameter"},{"entityId",circleId},{"valueMm",15}});check(dimensioned.ok,dimensioned.error);f=dimensioned.feature;
 auto dimension=f["parameters"].toObject()["dimensions"].toArray().last().toObject()["id"];
 auto error=document.commit(SketchGeometry::replaceFeature(document.authored(),f),"Post-Trim fixture");check(error.isEmpty(),error);
 auto args=QJsonObject{{"sketchId",f["id"]},{"profileId",disk},{"distanceMm",20}};
 error=document.commit(CadDocument::extrusionCandidate(document.authored(),args),"Disk");check(error.isEmpty(),error);
 auto modified=SketchGeometry::updateDimension(f,{{"dimensionId",dimension},{"valueMm",16}});check(modified.ok,modified.error);
 error=document.commit(SketchGeometry::replaceFeature(document.authored(),modified.feature),"Resize disk");check(error.isEmpty(),error);
 check(std::abs(document.bodies()[0].summary["volumeMm3"].toDouble()-std::numbers::pi*64*20)<1e-5,"Disk selection was not retained through diameter edit");
 document.history().undo();args["profileId"]=bridge;args["distanceMm"]=4;
 error=document.commit(CadDocument::extrusionCandidate(document.authored(),args),"Bridge");check(error.isEmpty(),error);auto stable=document.authored();auto revision=document.revision();
 error=document.commit(SketchGeometry::replaceFeature(stable,modified.feature),"Changed bridge");check(!error.isEmpty() && document.lastEvaluationError()["code"]=="profile_unavailable","Changed bridge reference silently selected another face");
 check(document.authored()==stable && document.revision()==revision,"Changed profile refusal edited live intent");
 document.history().undo();document.history().redo();check(document.authored()==stable,"Undo/redo changed retained regions");
 QTemporaryDir folder;check(document.save(folder.path()+"/support.alloy").isEmpty(),"Save regions");CadDocument opened;check(opened.open(folder.path()+"/support.alloy").isEmpty() && opened.authored()==stable,"Reopen changed region intent");
 std::cout<<"PASS: OCCT support partitions, coincident provenance, raw intersections, stable order/storage, disk edit, explicit bridge refusal and history\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
