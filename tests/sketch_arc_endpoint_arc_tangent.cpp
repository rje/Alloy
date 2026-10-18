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
    for(bool independent:{false,true})for(bool reverse:{false,true})for(const auto& roleA:{QString("start"),QString("end")})for(const auto& roleB:{QString("start"),QString("end")})for(double sweep:{90.,-90.})for(int mode:{0,1}){
        auto s=SketchGeometry::newFeature(QUuid::createUuid().toString(QUuid::WithoutBraces),"XY");
        const double bx=mode==0 ? 18 : 6,br=mode==0 ? 8 : 4,ba=mode==0 ? 180 : 0;
        const double otherSweep=((roleA==roleB)==(mode==0)) ? sweep : -sweep;
        s=SketchGeometry::addArc(s,{{"centerUMm",0},{"centerVMm",0},{"radiusMm",10},{"startAngleDeg",roleA=="start" ? 0 : -sweep},{"sweepAngleDeg",sweep}});const auto a=last(s);
        s=SketchGeometry::addArc(s,{{"centerUMm",bx},{"centerVMm",0},{"radiusMm",br},{"startAngleDeg",ba-(roleB=="end" ? otherSweep : 0)},{"sweepAngleDeg",otherSweep}});auto b=last(s);
        if(independent){auto p=s["parameters"].toObject();auto points=p["points"].toArray();QJsonObject point;
            for(const auto& v:points)if(v.toObject()["id"]==b[roleB+"PointId"])point=v.toObject();
            point["id"]=QUuid::createUuid().toString(QUuid::WithoutBraces);points.append(point);b[roleB+"PointId"]=point["id"];
            auto entities=p["entities"].toArray();entities[entities.size()-1]=b;p["entities"]=entities;p["points"]=points;s["parameters"]=p;
        }
        const QJsonObject args{{"type","tangent"},{"endpoint",reverse ? roleB+":"+roleA : roleA+":"+roleB},{"entityIds",reverse ? QJsonArray{b["id"],a["id"]} : QJsonArray{a["id"],b["id"]}}};
        auto result=SketchGeometry::addConstraint(s,args);check(result.ok,result.error);s=result.feature;
        const auto relation=s.value("parameters").toObject().value("constraints").toArray().last().toObject();
        check(relation["type"]=="arcEndpointArcTangent" && relation["side"].toInt()==(mode==0 ? 1 : -1),"retained endpoint branch");
        for(int i=0;i<2;++i){const auto arc=relation["entityIds"].toArray()[i]==a["id"] ? a : b;const auto role=arc["id"]==a["id"] ? roleA : roleB;
            check(relation["contacts"].toArray()[i].toObject()["pointId"]==arc[role+"PointId"],"stable endpoint identity");}
        check(SketchGeometry::solveDraft(s).ok,"endpoint record roundtrip");
        result=SketchGeometry::addConstraint(s,args);check(!result.ok && result.feature==s,"atomic duplicate refusal");
        auto invalid=s;auto p=invalid["parameters"].toObject();auto constraints=p["constraints"].toArray();auto corrupt=relation;auto contacts=corrupt["contacts"].toArray();auto contact=contacts[0].toObject();contact["sweepSign"]=-contact["sweepSign"].toInt();contacts[0]=contact;corrupt["contacts"]=contacts;constraints[constraints.size()-1]=corrupt;p["constraints"]=constraints;invalid["parameters"]=p;
        check(!SketchGeometry::solveDraft(invalid).ok,"oriented join reversal refused");
        invalid=s;p=invalid["parameters"].toObject();auto entities=p["entities"].toArray();auto source=entities[0].toObject();source["sweepAngleDeg"]=-source["sweepAngleDeg"].toDouble();entities[0]=source;p["entities"]=entities;invalid["parameters"]=p;
        check(!SketchGeometry::solveDraft(invalid).ok,"source sweep reversal refused");
    }
    auto cusp=SketchGeometry::newFeature(QUuid::createUuid().toString(QUuid::WithoutBraces),"XY");
    cusp=SketchGeometry::addArc(cusp,{{"centerUMm",0},{"centerVMm",0},{"radiusMm",10},{"startAngleDeg",-90},{"sweepAngleDeg",90}});const auto a=last(cusp);
    cusp=SketchGeometry::addArc(cusp,{{"centerUMm",18},{"centerVMm",0},{"radiusMm",8},{"startAngleDeg",180},{"sweepAngleDeg",90}});const auto b=last(cusp);
    const auto refused=SketchGeometry::addConstraint(cusp,{{"type","tangent"},{"endpoint","end:start"},{"entityIds",QJsonArray{a["id"],b["id"]}}});
    check(!refused.ok && refused.feature==cusp,"atomic initial cusp refusal");
    std::cout<<"endpoint-pair tangent: roles, pick orders, signed sweeps, external/internal branches, identity and orientation refusals passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
