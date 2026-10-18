// SPDX-License-Identifier: MIT
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QUuid>
#include <cmath>
#include <iostream>
#include <stdexcept>
static void check(bool ok,const QString& error){if(!ok)throw std::runtime_error(error.toStdString());}
static QJsonObject last(const QJsonObject& s){return s.value("parameters").toObject().value("entities").toArray().last().toObject();}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    for(bool reverse:{false,true})for(const auto& role:{QString("start"),QString("end")})for(double sweep:{90.,-90.})for(int mode:{0,1,2}){
        auto s=SketchGeometry::newFeature(QUuid::createUuid().toString(QUuid::WithoutBraces),"XY");
        s=SketchGeometry::addCircle(s,{{"centerUMm",0},{"centerVMm",0},{"diameterMm",20}});const auto circle=last(s);
        const double x=mode==0 ? 18 : mode==1 ? 6 : -6,r=mode==0 ? 8 : mode==1 ? 4 : 16,angle=mode==0 ? 180 : 0;
        s=SketchGeometry::addArc(s,{{"centerUMm",x},{"centerVMm",0},{"radiusMm",r},{"startAngleDeg",angle-(role=="end" ? sweep : 0)},{"sweepAngleDeg",sweep}});const auto arc=last(s);
        const QJsonObject args{{"type","tangent"},{"endpoint",role},{"entityIds",reverse ? QJsonArray{circle["id"],arc["id"]} : QJsonArray{arc["id"],circle["id"]}}};
        auto result=SketchGeometry::addConstraint(s,args);check(result.ok,result.error);s=result.feature;
        const auto relation=s.value("parameters").toObject().value("constraints").toArray().last().toObject();
        check(relation["type"]=="arcEndpointCircleTangent" && relation["side"].toInt()==(mode==0 ? 1 : -1),"retained endpoint branch");
        check(relation["contact"].toObject()["pointId"]==arc[role+"PointId"] && relation["contact"].toObject()["endpoint"]==role,"stable endpoint identity");
        check(SketchGeometry::solveDraft(s).ok,"endpoint record roundtrip");
        result=SketchGeometry::addConstraint(s,args);check(!result.ok && result.feature==s,"atomic duplicate refusal");
        auto invalid=s;auto p=invalid["parameters"].toObject();auto constraints=p["constraints"].toArray();auto corrupt=relation;auto contact=corrupt["contact"].toObject();contact["pointId"]=arc[role=="start" ? "endPointId" : "startPointId"];corrupt["contact"]=contact;constraints[constraints.size()-1]=corrupt;p["constraints"]=constraints;invalid["parameters"]=p;
        check(!SketchGeometry::solveDraft(invalid).ok,"endpoint identity corruption refused");
    }
    auto s=SketchGeometry::newFeature(QUuid::createUuid().toString(QUuid::WithoutBraces),"XY");
    s=SketchGeometry::addCircle(s,{{"centerUMm",0},{"centerVMm",0},{"diameterMm",20}});const auto circle=last(s);
    s=SketchGeometry::addArc(s,{{"centerUMm",0},{"centerVMm",0},{"radiusMm",8},{"startAngleDeg",0},{"sweepAngleDeg",90}});
    const auto result=SketchGeometry::addConstraint(s,{{"type","tangent"},{"endpoint","start"},{"entityIds",QJsonArray{circle["id"],last(s)["id"]}}});check(!result.ok && result.feature==s,"atomic coincident-center ambiguity refusal");
    std::cout<<"endpoint-circle tangent: roles, pick orders, signed sweeps, external/internal branches, identity and atomic refusals passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
