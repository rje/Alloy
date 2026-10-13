// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QPointF>
#include <QTemporaryDir>
#include <QUuid>
#include <cmath>
#include <iostream>
#include <stdexcept>
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
QString id(){return QUuid::createUuid().toString(QUuid::WithoutBraces);}
double distance(const QJsonObject& sketch,const QString& first,const QString& second){
    const auto params=sketch["parameters"].toObject();QMap<QString,QPointF> points;QMap<QString,QJsonObject> lines;
    for(const auto& row:params["points"].toArray()){const auto p=row.toObject();points[p["id"].toString()]={p["uMm"].toDouble(),p["vMm"].toDouble()};}
    for(const auto& row:params["entities"].toArray()){const auto line=row.toObject();lines[line["id"].toString()]=line;}
    const auto a=points[lines[first]["startPointId"].toString()],b=points[lines[first]["endPointId"].toString()],c=points[lines[second]["startPointId"].toString()];
    const auto d=b-a;return (d.x()*(c.y()-a.y())-d.y()*(c.x()-a.x()))/std::hypot(d.x(),d.y());
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument doc;auto sketch=SketchGeometry::newFeature(doc.authored()["documentId"].toString(),"XY");
    QJsonArray points,lines,constraints;QStringList p,l;
    for(const auto& xy:QList<QPointF>{{0,0},{50,0},{0,11},{50,11},{0,39},{50,39},{0,0},{20,10}}){const auto key=id();p<<key;points.append(QJsonObject{{"id",key},{"uMm",xy.x()},{"vMm",xy.y()}});}
    for(const auto& ends:QList<QPair<int,int>>{{0,1},{2,3},{4,5},{6,7}}){const auto key=id();l<<key;lines.append(QJsonObject{{"id",key},{"type","line"},{"startPointId",p[ends.first]},{"endPointId",p[ends.second]},{"construction",true}});}
    for(int i=0;i<3;++i)constraints.append(QJsonObject{{"id",id()},{"type","horizontal"},{"entityIds",QJsonArray{l[i]}}});
    constraints.append(QJsonObject{{"id",id()},{"type","fixed"},{"pointIds",QJsonArray{p[0]}},{"uMm",0},{"vMm",0}});
    auto params=sketch["parameters"].toObject();params["points"]=points;params["entities"]=lines;params["constraints"]=constraints;sketch["parameters"]=params;
    auto solved=SketchGeometry::solveDraft(sketch);check(solved.ok,"base solve");sketch=solved.feature;const int dof=solved.diagnostics["dof"].toInt();
    for(const auto& input:QList<QPair<QJsonArray,double>>{{QJsonArray{l[0],l[1]},11},{QJsonArray{l[1],l[2]},28}}){
        solved=SketchGeometry::addDimension(sketch,{{"kind","parallelLineDistance"},{"entityIds",input.first},{"valueMm",input.second},{"annotation",QJsonObject{{"uMm",25},{"vMm",input.second}}}});
        check(solved.ok,"parallel distance solve");sketch=solved.feature;
    }
    check(solved.diagnostics["dof"].toInt()==dof-2,"each distance removes one DOF");
    auto dimensions=sketch["parameters"].toObject()["dimensions"].toArray();check(dimensions.size()==2,"two retained distances");
    check(dimensions[0].toObject()["entityIds"].toArray()==QJsonArray{l[0],l[1]} && dimensions[0].toObject()["side"].toInt()==1,"ordered UUIDs and side");
    check(std::abs(distance(sketch,l[0],l[1])-11)<1e-6 && std::abs(distance(sketch,l[1],l[2])-28)<1e-6,"initial perpendicular distances");
    const auto firstId=dimensions[0].toObject()["id"],secondId=dimensions[1].toObject()["id"];
    solved=SketchGeometry::updateDimension(sketch,{{"dimensionId",firstId},{"valueMm",14}});check(solved.ok,"edit first distance");sketch=solved.feature;
    solved=SketchGeometry::updateDimension(sketch,{{"dimensionId",secondId},{"valueMm",30}});check(solved.ok,"edit second distance");sketch=solved.feature;
    check(std::abs(distance(sketch,l[0],l[1])-14)<1e-6 && std::abs(distance(sketch,l[1],l[2])-30)<1e-6,"edited geometry");
    auto refusal=SketchGeometry::addDimension(sketch,{{"kind","parallelLineDistance"},{"entityIds",QJsonArray{l[1],l[0]}},{"valueMm",14}});
    check(!refusal.ok && refusal.feature==sketch,"reversed duplicate refuses atomically");
    refusal=SketchGeometry::addDimension(sketch,{{"kind","parallelLineDistance"},{"entityIds",QJsonArray{l[0],l[3]}},{"valueMm",10}});
    check(!refusal.ok && refusal.feature==sketch,"nonparallel pair refuses atomically");
    // Initially parallel coordinates do not create a hidden Parallel relation.
    auto free=SketchGeometry::newFeature(doc.authored()["documentId"].toString(),"XY");auto freeParams=free["parameters"].toObject();
    freeParams["points"]=QJsonArray{points[0],points[1],points[2],points[3]};freeParams["entities"]=QJsonArray{lines[0],lines[1]};
    freeParams["constraints"]=QJsonArray{QJsonObject{{"id",id()},{"type","fixed"},{"pointIds",QJsonArray{p[0]}},{"uMm",0},{"vMm",0}},QJsonObject{{"id",id()},{"type","fixed"},{"pointIds",QJsonArray{p[1]}},{"uMm",50},{"vMm",0}}};free["parameters"]=freeParams;
    auto initiallyParallel=SketchGeometry::addDimension(free,{{"kind","parallelLineDistance"},{"entityIds",QJsonArray{l[0],l[1]}},{"valueMm",11}});
    check(initiallyParallel.ok,"initially parallel free pair");const auto freeId=initiallyParallel.feature["parameters"].toObject()["dimensions"].toArray()[0].toObject()["id"];
    refusal=SketchGeometry::updateDimension(initiallyParallel.feature,{{"dimensionId",freeId},{"valueMm",14}});
    check(!refusal.ok && refusal.feature==initiallyParallel.feature,"orientation loss refuses whole edit");
    check(doc.commit(SketchGeometry::replaceFeature(doc.authored(),sketch),"Parallel distances").isEmpty(),"commit");
    const auto authored=doc.authored();doc.history().undo();doc.history().redo();check(doc.authored()==authored,"history preserves relation");
    QTemporaryDir folder;check(doc.save(folder.path()+"/parallel.alloy").isEmpty(),"save");CadDocument reopened;
    check(reopened.open(folder.path()+"/parallel.alloy").isEmpty() && reopened.authored()==authored,"reopen preserves UUIDs and values");
    std::cout<<"PASS: two parallel distances, edits, side, DOF, refusal, history and reopen\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
