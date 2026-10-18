// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cmath>
#include <array>
#include <stdexcept>
#include <iostream>
namespace {
void check(bool good,const char* msg){if(!good)throw std::runtime_error(msg);}
QJsonObject need(const SketchGeometry::SolveResult& r){if(!r.ok)throw std::runtime_error(r.error.toStdString());return r.feature;}
QJsonObject circle(const QJsonObject& f){return f["parameters"].toObject()["entities"].toArray().last().toObject();}
void inspect(const QJsonObject& f,double x,double y,double radius){const auto p=f["parameters"].toObject(),c=circle(f);check(std::abs(c["radiusMm"].toDouble()-radius)<1e-6,"radius");for(const auto& row:p["points"].toArray())if(row.toObject()["id"]==c["centerPointId"]){check(std::abs(row.toObject()["uMm"].toDouble()-x)<1e-6,"center x");check(std::abs(row.toObject()["vMm"].toDouble()-y)<1e-6,"center y");}int count=0;for(const auto& row:p["constraints"].toArray())if(row.toObject()["type"]=="tangent"){check(row.toObject()["entityIds"].toArray()[0]==c["id"],"circle tangent ID");++count;}check(count==3,"three genuine tangents");check(SketchGeometry::evaluateFeature(f).profiles.size()==1,"circle profile");}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
 CadDocument doc;auto f=SketchGeometry::newFeature(doc.activeOwnerId(),"XY");f=SketchGeometry::addRectangle(f,{{"u1Mm",0},{"v1Mm",0},{"u2Mm",30},{"v2Mm",30}});const auto edges=f["parameters"].toObject()["entities"].toArray();QJsonArray ids;for(const auto& e:edges)ids.append(e.toObject()["id"]);f=need(SketchGeometry::setConstruction(f,{{"entityIds",ids},{"construction",true}}));f=need(SketchGeometry::addConstraint(f,{{"type","fixed"},{"pointIds",QJsonArray{edges[0].toObject()["startPointId"]}}}));f=need(SketchGeometry::addDimension(f,{{"kind","horizontal"},{"entityId",ids[0]},{"valueMm",30}}));const auto width=f["parameters"].toObject()["dimensions"].toArray().last().toObject().value("id");f=need(SketchGeometry::addDimension(f,{{"kind","vertical"},{"entityId",ids[1]},{"valueMm",30}}));
 const auto height=f["parameters"].toObject()["dimensions"].toArray().last().toObject().value("id");const QJsonObject args{{"lineIds",QJsonArray{ids[0],ids[1],ids[2]}}};
 const auto source=f;const auto dimensions=source["parameters"].toObject()["dimensions"].toArray();f=need(SketchGeometry::addThreeTangentCircle(f,args));inspect(f,15,15,15);const auto id=circle(f).value("id");check(f["parameters"].toObject()["dimensions"].toArray()==dimensions,"no radius input/dimension invented");
 inspect(need(SketchGeometry::addThreeTangentCircle(source,{{"lineIds",QJsonArray{ids[2],ids[0],ids[1]}}})),15,15,15);
 for(const auto& invalid:{QJsonArray{ids[0],ids[0],ids[2]},QJsonArray{ids[0],ids[1]},QJsonArray{ids[0],ids[1],id}}){const auto refused=SketchGeometry::addThreeTangentCircle(f,{{"lineIds",invalid}});check(!refused.ok && refused.feature==f,"transactional invalid target/count");}
 auto triple=[&](const std::array<std::array<double,4>,3>& coordinates){auto sketch=SketchGeometry::newFeature(doc.activeOwnerId(),"XY");QJsonArray lines;for(const auto& a:coordinates){sketch=SketchGeometry::addLine(sketch,{{"u1Mm",a[0]},{"v1Mm",a[1]},{"u2Mm",a[2]},{"v2Mm",a[3]}});lines.append(circle(sketch)["id"]);}const auto refused=SketchGeometry::addThreeTangentCircle(sketch,{{"lineIds",lines}});check(!refused.ok && refused.feature==sketch,"transactional no-solution/ambiguous branches");return refused.error;};
 triple({{{0,0,30,0},{0,10,30,10},{0,30,30,30}}});
 check(triple({{{-100,0,100,0},{0,-100,0,100},{-100,-90,100,110}}}).contains("ambiguous"),"multiple viable finite branches explicitly refused");
 check(doc.commit(SketchGeometry::replaceFeature(doc.authored(),f),"Three-tangent Circle").isEmpty(),"commit");
 f=need(SketchGeometry::updateDimension(f,{{"dimensionId",height},{"valueMm",40}}));inspect(f,10,20,20);check(circle(f)["id"]==id,"stable source circle identity");check(doc.commit(SketchGeometry::replaceFeature(doc.authored(),f),"Source height").isEmpty(),"source edit commit");const auto final=doc.authored();doc.history().undo();inspect(doc.authored()["features"].toArray()[0].toObject(),15,15,15);doc.history().redo();check(doc.authored()==final,"exact redo");QTemporaryDir dir;const auto path=dir.path()+"/three-tangent.alloy";check(doc.save(path).isEmpty(),"save");CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==final,"reopen");std::cout<<"Three-tangent Circle retained geometry passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
