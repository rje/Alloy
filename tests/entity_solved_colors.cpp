// SPDX-License-Identifier: MIT
#include "SketchGeometry.h"
#include "SketchConic.h"
#include <QCoreApplication>
#include <QJsonArray>
#include <QUuid>
#include <stdexcept>
#include <iostream>
namespace {
void check(bool b,const char* message){if(!b)throw std::runtime_error(message);}
QJsonObject accept(const SketchGeometry::SolveResult& r){if(!r.ok)throw std::runtime_error(r.error.toStdString());return r.feature;}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
 auto draft=accept(SketchGeometry::addCenterPointEllipse(SketchGeometry::newFeature(QUuid::createUuid().toString(QUuid::WithoutBraces),"XY"),{{"centerUMm",0},{"centerVMm",0},{"firstUMm",130},{"firstVMm",0},{"contourUMm",78},{"contourVMm",40},{"centerSnapTarget",QJsonObject{{"kind","origin"}}}}));
 const auto ellipse=draft["parameters"].toObject()["entities"].toArray().last().toObject();
 draft=accept(SketchGeometry::addConstraint(draft,{{"type","horizontal"},{"entityIds",QJsonArray{ellipse["firstAxisId"]}}}));
 draft=accept(SketchGeometry::addDimension(draft,{{"kind","linear"},{"entityId",ellipse["firstAxisId"]},{"valueMm",275}}));
 draft=accept(SketchGeometry::addDimension(draft,{{"kind","linear"},{"entityId",ellipse["secondAxisId"]},{"valueMm",100}}));
 const auto removeId=draft["parameters"].toObject()["dimensions"].toArray().last().toObject()["id"].toString();
 draft=accept(SketchConic::add(draft,{{"start",QJsonObject{{"uMm",200},{"vMm",0}}},{"end",QJsonObject{{"uMm",250},{"vMm",0}}},{"vertex",QJsonObject{{"uMm",225},{"vMm",30}}},{"rho",.541}}));
 const auto conic=draft["parameters"].toObject()["entities"].toArray().last().toObject();
 auto solved=SketchGeometry::solveDraft(draft);check(solved.ok&&solved.diagnostics["dof"].toInt()==7,"mixed global freedom");auto freedom=solved.diagnostics["entityFreedom"].toObject();
 for(const auto& key:{"id","firstAxisId","secondAxisId"})check(freedom[ellipse[key].toString()].toObject()["fullyConstrained"].toBool(),"ellipse and full axes fixed despite unrelated free curve");
 check(solved.diagnostics["pointFreedom"].toObject()[ellipse["centerPointId"].toString()].toObject()["fullyConstrained"].toBool(),"solved origin center fixed");check(!freedom[conic["id"].toString()].toObject()["fullyConstrained"].toBool(),"free conic remains free");
 auto parameters=draft["parameters"].toObject();QJsonArray dimensions;for(const auto& d:parameters["dimensions"].toArray())if(d.toObject()["id"]!=removeId)dimensions.append(d);parameters["dimensions"]=dimensions;draft["parameters"]=parameters;solved=SketchGeometry::solveDraft(draft);check(solved.ok&&solved.diagnostics["dof"].toInt()==8,"removed driving dimension restores freedom");freedom=solved.diagnostics["entityFreedom"].toObject();check(!freedom[ellipse["id"].toString()].toObject()["fullyConstrained"].toBool()&&!freedom[ellipse["secondAxisId"].toString()].toObject()["fullyConstrained"].toBool(),"ellipse and unconstrained axis become free");check(freedom[ellipse["firstAxisId"].toString()].toObject()["fullyConstrained"].toBool(),"independent retained axis stays fixed");
 auto roles=solved.diagnostics.value("entityRoles").toArray();QJsonObject roleById;
 for(const auto& row:roles){const auto value=row.toObject();check(!value.contains("dof"),"entity roles must not invent numeric DOF");roleById[value.value("entityId").toString()]=value.value("role");}
 check(roleById.value(ellipse["id"].toString())=="unconstrainedOrdinary","removed ellipse dimension refreshes semantic role");
 check(roleById.value(ellipse["firstAxisId"].toString())=="fullyConstrained","independent constrained construction axis remains black");
 check(roleById.value(ellipse["secondAxisId"].toString())=="unconstrainedConstruction","free construction axis has its own role");
 check(!solved.diagnostics.value("underconstrainedEntities").toArray().isEmpty(),"live list reports movable authored entities");
 std::cout<<"PlaneGCS mixed entity freedom and dimension removal passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
