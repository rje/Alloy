// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QUuid>
#include <QPointF>
#include <iostream>
#include <cmath>
#include <stdexcept>
namespace {
void check(bool ok,const char* text){if(!ok)throw std::runtime_error(text);}
QString id(){return QUuid::createUuid().toString(QUuid::WithoutBraces);}
void near(double a,double b){check(std::abs(a-b)<1e-6,"wrong diameter/geometry");}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument doc;auto sketch=SketchGeometry::newFeature(doc.authored()["documentId"].toString(),"XZ");
    QJsonArray points,entities,constraints;QStringList p,l;
    for(const auto& xy:QList<QPointF>{{0,0},{0,150},{50,0},{82.5,0},{82.5,100},{50,100}}){const auto key=id();p<<key;points.append(QJsonObject{{"id",key},{"uMm",xy.x()},{"vMm",xy.y()}});}
    for(const auto& ends:QList<QPair<int,int>>{{0,1},{2,3},{3,4},{4,5},{5,2}}){const auto key=id();l<<key;entities.append(QJsonObject{{"id",key},{"type","line"},{"startPointId",p[ends.first]},{"endPointId",p[ends.second]}});}
    auto relation=[&](QString type,QString field,QJsonArray refs){constraints.append(QJsonObject{{"id",id()},{"type",type},{field,refs}});};
    constraints.append(QJsonObject{{"id",id()},{"type","fixed"},{"pointIds",QJsonArray{p[0]}},{"uMm",0},{"vMm",0}});constraints.append(QJsonObject{{"id",id()},{"type","fixed"},{"pointIds",QJsonArray{p[1]}},{"uMm",0},{"vMm",150}});
    relation("horizontal","entityIds",{l[1]});relation("horizontal","entityIds",{l[3]});relation("vertical","entityIds",{l[2]});relation("vertical","entityIds",{l[4]});
    auto params=sketch["parameters"].toObject();params["points"]=points;params["entities"]=entities;params["constraints"]=constraints;sketch["parameters"]=params;
    auto solved=SketchGeometry::solveDraft(sketch);check(solved.ok,solved.error.toUtf8().constData());sketch=solved.feature;const int initialDof=solved.diagnostics["dof"].toInt();
    auto converted=SketchGeometry::setCenterline(sketch,{{"entityIds",QJsonArray{l[0]}},{"centerline",true}});check(converted.ok,converted.error.toUtf8().constData());sketch=converted.feature;
    auto geometry=SketchGeometry::evaluateFeature(sketch);check(geometry.profiles.size()==1,"detached Centerline must preserve closed profile");const auto profileId=geometry.profiles[0].profileId;
    // A boundary Centerline is still a boundary, not Construction.
    auto boundary=SketchGeometry::setCenterline(sketch,{{"entityIds",QJsonArray{l[1]}},{"centerline",true}});check(boundary.ok,"boundary conversion");check(SketchGeometry::evaluateFeature(boundary.feature).profiles[0].profileId==profileId,"boundary conversion must retain profile identity");
    boundary=SketchGeometry::setCenterline(boundary.feature,{{"entityIds",QJsonArray{l[1]}},{"centerline",false}});check(boundary.ok && boundary.feature==sketch,"normal conversion preserves source IDs");
    near(SketchGeometry::centerlineDiameterValue(sketch,{l[4],l[0]}),100);
    solved=SketchGeometry::addDimension(sketch,{{"kind","centerlineDiameter"},{"entityIds",QJsonArray{l[4],l[0]}},{"valueMm",100}});check(solved.ok,solved.error.toUtf8().constData());sketch=solved.feature;check(solved.diagnostics["dof"].toInt()==initialDof-1,"diameter removes one distance DOF only");
    auto dimension=sketch["parameters"].toObject()["dimensions"].toArray()[0].toObject();check(dimension["entityIds"].toArray()==QJsonArray{l[0],l[4]},"normalized stable axis/edge refs");check(dimension["side"].toInt()==-1,"retained side");
    solved=SketchGeometry::updateDimension(sketch,{{"dimensionId",dimension["id"]},{"valueMm",110}});check(solved.ok,solved.error.toUtf8().constData());sketch=solved.feature;near(SketchGeometry::centerlineDiameterValue(sketch,{l[0],l[4]}),110);check(SketchGeometry::evaluateFeature(sketch).profiles.size()==1,"edited closed profile");
    auto driven=SketchGeometry::addDimension(sketch,{{"kind","centerlineDiameter"},{"entityIds",QJsonArray{l[0],l[2]}},{"driving",false}});check(driven.ok,"driven diameter");check(driven.diagnostics["dof"]==solved.diagnostics["dof"],"driven does not remove DOF");
    const auto drivenId=driven.feature["parameters"].toObject()["dimensions"].toArray()[1].toObject()["id"];
    check(!SketchGeometry::updateDimension(driven.feature,{{"dimensionId",drivenId},{"valueMm",200}}).ok,"driven read-only");
    auto refusal=SketchGeometry::addDimension(sketch,{{"kind","centerlineDiameter"},{"entityIds",QJsonArray{l[0],l[1]}},{"valueMm",100}});check(!refusal.ok && refusal.feature==sketch,"nonparallel refuses transactionally");
    refusal=SketchGeometry::setConstruction(sketch,{{"entityIds",QJsonArray{l[0]}},{"construction",true}});check(!refusal.ok && refusal.feature==sketch,"referenced axis conversion refuses transactionally");
    // No hidden parallel equation: an initially parallel free endpoint may
    // rotate under a distance edit; refuse it instead of publishing that draft.
    auto freePair=SketchGeometry::newFeature(doc.authored()["documentId"].toString(),"XZ");
    auto fp=freePair["parameters"].toObject();auto freeEntities=QJsonArray{entities[0],entities[4]};auto fa=freeEntities[0].toObject();fa["centerline"]=true;freeEntities[0]=fa;
    fp["points"]=QJsonArray{points[0],points[1],points[2],points[5]};fp["entities"]=freeEntities;
    fp["constraints"]=QJsonArray{constraints[0],constraints[1],QJsonObject{{"id",id()},{"type","fixed"},{"pointIds",QJsonArray{p[2]}},{"uMm",50},{"vMm",0}}};freePair["parameters"]=fp;
    auto freeDimension=SketchGeometry::addDimension(freePair,{{"kind","centerlineDiameter"},{"entityIds",QJsonArray{l[0],l[4]}},{"valueMm",100}});check(freeDimension.ok,"initially parallel pair");
    const auto freeId=freeDimension.feature["parameters"].toObject()["dimensions"].toArray()[0].toObject()["id"];
    refusal=SketchGeometry::updateDimension(freeDimension.feature,{{"dimensionId",freeId},{"valueMm",110}});check(!refusal.ok && refusal.feature==freeDimension.feature,"lost orientation refuses whole transaction");
    auto construction=SketchGeometry::setConstruction(boundary.feature,{{"entityIds",QJsonArray{l[0]}},{"construction",true}});check(construction.ok,"explicit construction conversion");
    auto restored=SketchGeometry::setCenterline(construction.feature,{{"entityIds",QJsonArray{l[0]}},{"centerline",true}});check(restored.ok,"Construction to Centerline conversion");check(!restored.feature["parameters"].toObject()["entities"].toArray()[0].toObject()["construction"].toBool(),"conversion clears Construction intent");
    check(doc.commit(SketchGeometry::replaceFeature(doc.authored(),driven.feature),"Centerline diameter").isEmpty(),"commit");const auto authored=doc.authored();doc.history().undo();doc.history().redo();check(doc.authored()==authored,"undo redo preserves intent");QTemporaryDir dir;check(doc.save(dir.path()+"/centerline.alloy").isEmpty(),"save");CadDocument reopened;check(reopened.open(dir.path()+"/centerline.alloy").isEmpty() && reopened.authored()==authored,"reopen");
    std::cout<<"PASS: Centerline boundary/detached axis, full diameter refs/side/edit/DOF, driven, refusal, undo and storage\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
