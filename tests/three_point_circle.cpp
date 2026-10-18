// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QJsonArray>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <numbers>
#include <stdexcept>
namespace {
void check(bool good,const char* message){if(!good)throw std::runtime_error(message);}
void near(double a,double b,const char* message){check(std::abs(a-b)<1e-6,message);}
QJsonObject require(const SketchGeometry::SolveResult& r){if(!r.ok)throw std::runtime_error(r.error.toStdString());return r.feature;}
QJsonObject circle(const QJsonObject& f){return f["parameters"].toObject()["entities"].toArray().last().toObject();}
QJsonObject point(const QJsonObject& f,const QJsonValue& id){for(const auto& row:f["parameters"].toObject()["points"].toArray())if(row.toObject()["id"]==id)return row.toObject();throw std::runtime_error("missing point");}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
 CadDocument document;const auto empty=SketchGeometry::newFeature(document.activeOwnerId(),"XY");
 const QJsonObject args{{"u1Mm",0},{"v1Mm",0},{"u2Mm",20},{"v2Mm",0},{"u3Mm",20},{"v3Mm",20}};
 auto raw=SketchGeometry::addThreePointCircle(empty,args);check(raw.ok,raw.error.toUtf8().constData());check(raw.diagnostics["dof"].toInt()==3,"free picks leave ordinary circle freedom");
 auto f=raw.feature;check(f["parameters"].toObject()["entities"].toArray().size()==1 && f["parameters"].toObject()["points"].toArray().size()==1,"free picks create only analytic circle and center");check(f["parameters"].toObject()["constraints"].toArray().isEmpty() && f["parameters"].toObject()["dimensions"].toArray().isEmpty(),"no invented fixed picks or dimensions");
 near(circle(f)["radiusMm"].toDouble(),std::sqrt(200.0),"circumradius");near(point(f,circle(f)["centerPointId"])["uMm"].toDouble(),10,"circumcenter U");near(point(f,circle(f)["centerPointId"])["vMm"].toDouble(),10,"circumcenter V");
 for(const auto& invalid:{QJsonObject{{"u3Mm",40},{"v3Mm",0}},QJsonObject{{"u2Mm",0},{"v2Mm",0}},QJsonObject{{"firstSnapTarget",QJsonObject{{"kind","point"},{"pointId",CadDocument::newId()}}}}}){auto bad=args;for(auto it=invalid.begin();it!=invalid.end();++it)bad[it.key()]=it.value();const auto refused=SketchGeometry::addThreePointCircle(empty,bad);check(!refused.ok && refused.feature==empty,"degenerate/stale picks refuse transactionally");}
 f=require(SketchGeometry::addDimension(f,{{"kind","diameter"},{"entityId",circle(f)["id"]},{"valueMm",30}}));const auto diameter=f["parameters"].toObject()["dimensions"].toArray().last().toObject().value("id");
 const auto freeCircleId=circle(f).value("id");f=require(SketchGeometry::updateDimension(f,{{"dimensionId",diameter},{"valueMm",36}}));check(circle(f)["id"]==freeCircleId,"diameter edit preserves circle identity");near(circle(f)["radiusMm"].toDouble(),18,"ordinary diameter update");
 auto source=SketchGeometry::addRectangle(empty,{{"u1Mm",0},{"v1Mm",0},{"u2Mm",20},{"v2Mm",20}});const auto edges=source["parameters"].toObject()["entities"].toArray();const QJsonValue p1=edges[0].toObject()["startPointId"],p2=edges[0].toObject()["endPointId"],p3=edges[1].toObject()["endPointId"];
 source=require(SketchGeometry::addConstraint(source,{{"type","fixed"},{"pointIds",QJsonArray{p1}}}));source=require(SketchGeometry::addDimension(source,{{"kind","horizontal"},{"entityId",edges[0].toObject()["id"]},{"valueMm",20}}));source=require(SketchGeometry::addDimension(source,{{"kind","vertical"},{"entityId",edges[1].toObject()["id"]},{"valueMm",20}}));const auto height=source["parameters"].toObject()["dimensions"].toArray().last().toObject().value("id");
 QJsonArray ids;for(const auto& e:edges)ids.append(e.toObject()["id"]);source=require(SketchGeometry::setConstruction(source,{{"entityIds",ids},{"construction",true}}));
 auto snapped=args;snapped["firstSnapTarget"]=QJsonObject{{"kind","point"},{"pointId",p1}};snapped["secondSnapTarget"]=QJsonObject{{"kind","point"},{"pointId",p2}};snapped["thirdSnapTarget"]=QJsonObject{{"kind","point"},{"pointId",p3}};
 auto retained=require(SketchGeometry::addThreePointCircle(source,snapped));const auto retainedId=circle(retained).value("id");int relations=0;for(const auto& row:retained["parameters"].toObject()["constraints"].toArray()){const auto c=row.toObject();if(c["type"]=="pointOnCurve"){check(c["entityIds"].toArray()==QJsonArray{retainedId},"actual circle source relation");++relations;}}check(relations==3,"three source incidences retained");
 auto geometry=SketchGeometry::evaluateFeature(retained);check(geometry.profiles.size()==1,"single circle profile over construction sources");near(geometry.profiles[0].summary["areaMm2"].toDouble(),200*std::numbers::pi,"independent disk area");
 check(document.commit(SketchGeometry::replaceFeature(document.authored(),retained),"Three-point circle").isEmpty(),"commit");check(document.commit(CadDocument::extrusionCandidate(document.authored(),{{"sketchId",retained["id"]},{"profileId",document.sketches()[0].profiles[0].profileId},{"distanceMm",5}}),"Extrude").isEmpty(),"extrude");
 retained=require(SketchGeometry::updateDimension(retained,{{"dimensionId",height},{"valueMm",30}}));check(circle(retained)["id"]==retainedId,"source edit preserves circle ID");near(circle(retained)["radiusMm"].toDouble(),std::sqrt(325.0),"source edit recomputes circumradius");near(point(retained,circle(retained)["centerPointId"])["vMm"].toDouble(),15,"source edit recomputes center");
 check(document.commit(SketchGeometry::replaceFeature(document.authored(),retained),"Source height").isEmpty(),"source reeval");near(document.bodies()[0].summary["volumeMm3"].toDouble(),1625*std::numbers::pi,"source edit updates prism");
 auto refused=SketchGeometry::addDimension(retained,{{"kind","diameter"},{"entityId",retainedId},{"valueMm",10}});check(!refused.ok && refused.feature==retained,"conflicting diameter preserves source relations");
 const auto final=document.authored();document.history().undo();near(document.bodies()[0].summary["volumeMm3"].toDouble(),1000*std::numbers::pi,"undo source edit");document.history().redo();check(document.authored()==final,"exact redo");QTemporaryDir folder;const auto path=folder.path()+"/three-point-circle.alloy";check(document.save(path).isEmpty(),"save");CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==final,"exact native reopen");
 auto origin=args;origin["firstSnapTarget"]=QJsonObject{{"kind","origin"}};auto anchored=require(SketchGeometry::addThreePointCircle(empty,origin));check(!anchored["parameters"].toObject()["originPointId"].toString().isEmpty(),"origin snap retains real origin identity");
 std::cout<<"Three-point Circle retained geometry passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
