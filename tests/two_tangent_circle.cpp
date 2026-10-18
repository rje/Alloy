// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cmath>
#include <stdexcept>
#include <iostream>
namespace {
void check(bool good,const char* msg){if(!good)throw std::runtime_error(msg);}
QJsonObject need(const SketchGeometry::SolveResult& r){if(!r.ok)throw std::runtime_error(r.error.toStdString());return r.feature;}
QJsonObject circle(const QJsonObject& f){return f["parameters"].toObject()["entities"].toArray().last().toObject();}
void inspect(const QJsonObject& f,double x,double y,double radius){const auto p=f["parameters"].toObject(),c=circle(f);check(std::abs(c["radiusMm"].toDouble()-radius)<1e-6,"radius");for(const auto& row:p["points"].toArray())if(row.toObject()["id"]==c["centerPointId"]){check(std::abs(row.toObject()["uMm"].toDouble()-x)<1e-6,"center x");check(std::abs(row.toObject()["vMm"].toDouble()-y)<1e-6,"center y");}int count=0;for(const auto& row:p["constraints"].toArray())if(row.toObject()["type"]=="tangent"){check(row.toObject()["entityIds"].toArray()[0]==c["id"],"circle tangent ID");++count;}check(count==2,"two genuine tangents");check(SketchGeometry::evaluateFeature(f).profiles.size()==1,"circle profile");}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
 CadDocument doc;auto f=SketchGeometry::newFeature(doc.activeOwnerId(),"XY");f=SketchGeometry::addRectangle(f,{{"u1Mm",0},{"v1Mm",0},{"u2Mm",30},{"v2Mm",30}});const auto edges=f["parameters"].toObject()["entities"].toArray();QJsonArray ids;for(const auto& e:edges)ids.append(e.toObject()["id"]);f=need(SketchGeometry::setConstruction(f,{{"entityIds",ids},{"construction",true}}));f=need(SketchGeometry::addConstraint(f,{{"type","fixed"},{"pointIds",QJsonArray{edges[0].toObject()["startPointId"]}}}));f=need(SketchGeometry::addDimension(f,{{"kind","horizontal"},{"entityId",ids[0]},{"valueMm",30}}));const auto width=f["parameters"].toObject()["dimensions"].toArray().last().toObject().value("id");f=need(SketchGeometry::addDimension(f,{{"kind","vertical"},{"entityId",ids[1]},{"valueMm",30}}));
 QJsonObject args{{"lineIds",QJsonArray{ids[0],ids[1]}},{"uMm",20},{"vMm",10},{"radiusMm",10}};
 const auto source=f;f=need(SketchGeometry::addTwoTangentCircle(f,args));inspect(f,20,10,10);const auto id=circle(f).value("id"),radius=f["parameters"].toObject()["dimensions"].toArray().last().toObject().value("id");
 auto pointer=args;pointer.remove("radiusMm");inspect(need(SketchGeometry::addTwoTangentCircle(source,pointer)),20,10,10);
 for(const auto& bad:{QJsonObject{{"radiusMm",40}},QJsonObject{{"uMm",30}},QJsonObject{{"radiusMm",-1}},QJsonObject{{"lineIds",QJsonArray{ids[0],ids[0]}}},QJsonObject{{"lineIds",QJsonArray{ids[0],ids[2]}}},QJsonObject{{"lineIds",QJsonArray{ids[0],id}}}}){auto a=args;for(auto it=bad.begin();it!=bad.end();++it)a[it.key()]=it.value();const auto refusal=SketchGeometry::addTwoTangentCircle(f,a);check(!refusal.ok && refusal.feature==f,"transactional invalid radius/branch/target");}
 const auto impossible=SketchGeometry::updateDimension(f,{{"dimensionId",radius},{"valueMm",50}});check(!impossible.ok && impossible.feature==f,"incompatible radius edit transactional");
 check(doc.commit(SketchGeometry::replaceFeature(doc.authored(),f),"Tangent circle").isEmpty(),"commit");
 f=need(SketchGeometry::updateDimension(f,{{"dimensionId",width},{"valueMm",40}}));inspect(f,30,10,10);f=need(SketchGeometry::updateDimension(f,{{"dimensionId",radius},{"valueMm",8}}));inspect(f,32,8,8);check(circle(f)["id"]==id,"stable circle source identity");check(doc.commit(SketchGeometry::replaceFeature(doc.authored(),f),"Source and radius").isEmpty(),"edit commit");const auto final=doc.authored();doc.history().undo();inspect(doc.authored()["features"].toArray()[0].toObject(),20,10,10);doc.history().redo();check(doc.authored()==final,"exact redo");QTemporaryDir dir;const auto path=dir.path()+"/two-tangent.alloy";check(doc.save(path).isEmpty(),"save");CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==final,"reopen");std::cout<<"Two-tangent Circle retained geometry passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
