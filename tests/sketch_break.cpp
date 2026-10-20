// SPDX-License-Identifier: MIT
#include "SketchGeometry.h"
#include "CadDocument.h"
#include <QCoreApplication>
#include <iostream>
#include <QUuid>
#include <cmath>
#include <stdexcept>
static void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
 CadDocument document;auto blank=SketchGeometry::newFeature(document.authored().value("documentId").toString(),"XY","Break fixture");
 auto line=[](QJsonObject s,double a,double b,double c,double d){return SketchGeometry::addLine(s,{{"u1Mm",a},{"v1Mm",b},{"u2Mm",c},{"v2Mm",d}});};
 auto entity=[](const QJsonObject& s,int n){return s.value("parameters").toObject().value("entities").toArray()[n].toObject();};
 auto split=[&](const QJsonObject& s,double u,double v){return SketchGeometry::breakCurve(s,{{"entityId",entity(s,0).value("id")},{"uMm",u},{"vMm",v}});};
 auto s=line(line(blank,-10,0,10,0),0,-5,0,5);auto result=split(s,5,0);check(result.ok,qPrintable(result.error));
 auto es=result.feature.value("parameters").toObject().value("entities").toArray();check(es.size()==3&&es[0].toObject().value("id")==entity(s,0).value("id"),"stable first piece");check(es[0].toObject().value("endPointId")==es[1].toObject().value("startPointId"),"shared split point");
 auto multiple=line(s,5,-5,5,5);auto rejected=split(multiple,2,0);check(!rejected.ok&&rejected.feature==multiple,"multiple crossings refuse atomically");
 auto circle=SketchGeometry::addCircle(blank,{{"centerUMm",0},{"centerVMm",0},{"diameterMm",20}});circle=line(circle,-15,0,15,0);result=split(circle,0,10);check(result.ok,qPrintable(result.error));
 es=result.feature.value("parameters").toObject().value("entities").toArray();double sweep=0;for(int i=0;i<2;++i){const auto e=es[i].toObject();check(e.value("type")=="arc"&&std::abs(e.value("radiusMm").toDouble()-10)<1e-8,"analytic circle pieces");sweep+=e.value("sweepAngleDeg").toDouble();}check(std::abs(sweep-360)<1e-8,"complete circle coverage");
 auto refuse=[&](const QJsonObject& input,double u,double v){const auto r=split(input,u,v);check(!r.ok&&r.feature==input,"atomic refusal");};
 refuse(SketchGeometry::addCircle(blank,{{"centerUMm",0},{"centerVMm",0},{"diameterMm",20}}),0,10);
 refuse(line(SketchGeometry::addCircle(blank,{{"centerUMm",0},{"centerVMm",0},{"diameterMm",20}}),-15,10,15,10),0,-10);
 refuse(line(circle,0,-15,0,15),0,10);refuse(line(blank,0,0,10,0),0,0);
 refuse(line(SketchGeometry::addCircle(blank,{{"centerUMm",0},{"centerVMm",0},{"diameterMm",20}}),-10,0,15,0),0,10);
 auto arc=SketchGeometry::addArc(blank,{{"centerUMm",0},{"centerVMm",0},{"radiusMm",10},{"startAngleDeg",0},{"sweepAngleDeg",180}});arc=line(arc,0,-15,0,15);result=split(arc,7,7);check(result.ok,qPrintable(result.error));es=result.feature.value("parameters").toObject().value("entities").toArray();check(std::abs(es[0].toObject().value("sweepAngleDeg").toDouble()-90)<1e-7&&std::abs(es[1].toObject().value("sweepAngleDeg").toDouble()-90)<1e-7,"finite arc split");
 auto annotated=s;auto p=annotated.value("parameters").toObject();const auto constraintId=QUuid::createUuid().toString(QUuid::WithoutBraces);p["constraints"]=QJsonArray{QJsonObject{{"id",constraintId},{"type","horizontal"},{"entityIds",QJsonArray{entity(s,0).value("id")}}}};const auto retainedId=QUuid::createUuid().toString(QUuid::WithoutBraces);auto constraints=p.value("constraints").toArray();constraints.append(QJsonObject{{"id",retainedId},{"type","fixed"},{"pointIds",QJsonArray{entity(s,0).value("startPointId")}},{"uMm",-10},{"vMm",0}});p["constraints"]=constraints;annotated["parameters"]=p;result=split(annotated,5,0);check(result.ok,qPrintable(result.error));check(result.diagnostics.value("removedConstraintIds").toArray().contains(constraintId),"explicit constraint retirement");check(result.diagnostics.value("retainedConstraintIds").toArray().contains(retainedId),"unchanged endpoint relation retained");
 std::cout<<"Sketch Break focused checks passed\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
