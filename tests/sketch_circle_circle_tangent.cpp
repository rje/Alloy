// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <stdexcept>
static void check(bool v,const QString& m){if(!v)throw std::runtime_error(m.toStdString());}
static QJsonObject last(const QJsonObject& s){return s.value("parameters").toObject().value("entities").toArray().last().toObject();}
static void contact(const QJsonObject& s,int side){const auto p=s.value("parameters").toObject();QMap<QString,QJsonObject> points,entities;
 for(const auto& v:p.value("points").toArray())points[v.toObject().value("id").toString()]=v.toObject();
 for(const auto& v:p.value("entities").toArray())entities[v.toObject().value("id").toString()]=v.toObject();
 const auto c=p.value("constraints").toArray().last().toObject();check(c.value("type")=="circleTangent" && c.value("side")==side,"retained branch");
 const auto ids=c.value("entityIds").toArray();const auto a=entities[ids[0].toString()],b=entities[ids[1].toString()];
 const auto x=points[a.value("centerPointId").toString()],y=points[b.value("centerPointId").toString()];
 check(std::abs(std::hypot(x.value("uMm").toDouble()-y.value("uMm").toDouble(),x.value("vMm").toDouble()-y.value("vMm").toDouble())-a.value("radiusMm").toDouble()-side*b.value("radiusMm").toDouble())<1e-6,"selected tangent contact");}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{CadDocument doc;
 for(int side:{1,-1}){auto s=SketchGeometry::newFeature(doc.authored().value("documentId").toString(),"XY");
 s=SketchGeometry::addCircle(s,{{"centerUMm",0},{"centerVMm",0},{"diameterMm",20}});const auto a=last(s);
 s=SketchGeometry::addCircle(s,{{"centerUMm",side==1?18:5},{"centerVMm",0},{"diameterMm",6}});const auto b=last(s);
 auto result=SketchGeometry::addConstraint(s,{{"type","tangent"},{"entityIds",QJsonArray{b.value("id"),a.value("id")}}});check(result.ok,result.error);s=result.feature;contact(s,side);
 auto bad=SketchGeometry::addConstraint(s,{{"type","tangent"},{"entityIds",QJsonArray{a.value("id"),b.value("id")}}});check(!bad.ok&&bad.feature==s,"atomic duplicate");
 check(doc.commit(SketchGeometry::replaceFeature(doc.authored(),s),"Circle Tangent").isEmpty(),"commit");const auto authored=doc.authored();doc.history().undo();doc.history().redo();check(doc.authored()==authored,"history");QTemporaryDir dir;const auto path=dir.path()+"/tangent.alloy";check(doc.save(path).isEmpty(),"save");CadDocument reopened;check(reopened.open(path).isEmpty()&&reopened.authored()==authored,"reopen");}
 for(int offset:{0,10}){auto s=SketchGeometry::newFeature(doc.authored().value("documentId").toString(),"XY");s=SketchGeometry::addCircle(s,{{"centerUMm",0},{"centerVMm",0},{"diameterMm",20}});auto a=last(s);s=SketchGeometry::addCircle(s,{{"centerUMm",offset},{"centerVMm",0},{"diameterMm",6}});auto result=SketchGeometry::addConstraint(s,{{"type","tangent"},{"entityIds",QJsonArray{a.value("id"),last(s).value("id")}}});check(!result.ok&&result.feature==s,"atomic ambiguous branch");}
 std::cout<<"circle-circle external/internal contact, duplicate, ambiguity, history, reopen passed\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
