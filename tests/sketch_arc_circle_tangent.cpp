// SPDX-License-Identifier: MIT
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QUuid>
#include <iostream>
#include <stdexcept>
static void check(bool ok,const QString& error){if(!ok)throw std::runtime_error(error.toStdString());}
static QJsonObject last(const QJsonObject& s){return s.value("parameters").toObject().value("entities").toArray().last().toObject();}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    for(bool reverse:{false,true})for(double sweep:{180.,-180.})for(int mode:{0,1,2}){
        auto s=SketchGeometry::newFeature(QUuid::createUuid().toString(QUuid::WithoutBraces),"XY");
        s=SketchGeometry::addCircle(s,{{"centerUMm",0},{"centerVMm",0},{"diameterMm",20}});auto circle=last(s);
        // External contact, inner arc in circle, and outer arc around circle.
        double x=mode==0 ? 18 : mode==1 ? 6 : -6,r=mode==2 ? 16 : mode==1 ? 4 : 8;
        double start=mode==0 ? 90 : -90;if(sweep<0)start+=180;
        s=SketchGeometry::addArc(s,{{"centerUMm",x},{"centerVMm",0},{"radiusMm",r},{"startAngleDeg",start},{"sweepAngleDeg",sweep}});auto arc=last(s);
        auto result=SketchGeometry::addConstraint(s,{{"type","tangent"},{"entityIds",reverse ? QJsonArray{circle["id"],arc["id"]} : QJsonArray{arc["id"],circle["id"]}}});
        check(result.ok,result.error);s=result.feature;
        auto c=s.value("parameters").toObject().value("constraints").toArray().last().toObject();
        check(c.value("type")=="arcCircleTangent" && c.value("side").toInt()==(mode==0 ? 1 : -1),"retained contact branch");
        check(std::abs(c.value("contact").toObject().value("arcParameter").toDouble()-.5)<1e-6,"finite arc contact");
        result=SketchGeometry::addConstraint(s,{{"type","tangent"},{"entityIds",QJsonArray{circle["id"],arc["id"]}}});check(!result.ok && result.feature==s,"atomic duplicate");
    }
    for(int x:{0,10,18}){
        auto s=SketchGeometry::newFeature(QUuid::createUuid().toString(QUuid::WithoutBraces),"XY");s=SketchGeometry::addCircle(s,{{"centerUMm",0},{"centerVMm",0},{"diameterMm",20}});auto circle=last(s);
        s=SketchGeometry::addArc(s,{{"centerUMm",x},{"centerVMm",0},{"radiusMm",8},{"startAngleDeg",-45},{"sweepAngleDeg",90}});
        auto result=SketchGeometry::addConstraint(s,{{"type","tangent"},{"entityIds",QJsonArray{circle["id"],last(s).value("id")}}});check(!result.ok && result.feature==s,"atomic coincident/tied/out-of-sweep refusal");
    }
    std::cout<<"arc-circle tangent: both orders, signed sweep, external/internal branches and atomic refusals passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
