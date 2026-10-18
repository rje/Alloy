// SPDX-License-Identifier: MIT
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QUuid>
#include <iostream>
#include <stdexcept>
static void check(bool ok,const QString& error){if(!ok)throw std::runtime_error(error.toStdString());}
static QJsonObject last(const QJsonObject& s){return s.value("parameters").toObject().value("entities").toArray().last().toObject();}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    for(bool reverse:{false,true})for(double sign:{1.,-1.})for(bool internal:{false,true}){
        auto s=SketchGeometry::newFeature(QUuid::createUuid().toString(QUuid::WithoutBraces),"XY");
        s=SketchGeometry::addArc(s,{{"centerUMm",0},{"centerVMm",0},{"radiusMm",10},{"startAngleDeg",-90*sign},{"sweepAngleDeg",180*sign}});auto a=last(s);
        s=SketchGeometry::addArc(s,{{"centerUMm",internal ? 6 : 14},{"centerVMm",0},{"radiusMm",4},{"startAngleDeg",(internal ? 0 : 180)-90*sign},{"sweepAngleDeg",180*sign}});auto b=last(s);
        auto result=SketchGeometry::addConstraint(s,{{"type","tangent"},{"entityIds",reverse ? QJsonArray{b["id"],a["id"]} : QJsonArray{a["id"],b["id"]}}});check(result.ok,result.error);s=result.feature;
        const auto c=s.value("parameters").toObject().value("constraints").toArray().last().toObject();
        check(c.value("type")=="arcArcTangent" && c.value("side").toInt()==(internal ? -1 : 1),"retained contact branch");
        for(const auto& v:c.value("contacts").toArray())check(std::abs(v.toObject().value("arcParameter").toDouble()-.5)<1e-6,"both finite contacts retained");
        result=SketchGeometry::addConstraint(s,{{"type","tangent"},{"entityIds",QJsonArray{a["id"],b["id"]}}});check(!result.ok && result.feature==s,"atomic duplicate");
        auto malformed=s;auto p=malformed.value("parameters").toObject();auto rows=p.value("constraints").toArray();auto invalid=rows.last().toObject();invalid["contacts"]=QJsonArray{c.value("contacts").toArray()[0]};rows[rows.size()-1]=invalid;p["constraints"]=rows;malformed["parameters"]=p;
        check(!SketchGeometry::solveDraft(malformed).ok,"reject malformed retained contacts");
    }
    for(int x:{0,10,14})for(bool secondFails:{false,true}){
        auto s=SketchGeometry::newFeature(QUuid::createUuid().toString(QUuid::WithoutBraces),"XY");
        s=SketchGeometry::addArc(s,{{"centerUMm",0},{"centerVMm",0},{"radiusMm",10},{"startAngleDeg",secondFails ? -90 : 90},{"sweepAngleDeg",180}});auto a=last(s);
        s=SketchGeometry::addArc(s,{{"centerUMm",x},{"centerVMm",0},{"radiusMm",4},{"startAngleDeg",secondFails ? -90 : 90},{"sweepAngleDeg",180}});
        auto result=SketchGeometry::addConstraint(s,{{"type","tangent"},{"entityIds",QJsonArray{a["id"],last(s).value("id")}}});check(!result.ok && result.feature==s,"atomic coincident/tied/out-of-either-sweep refusal");
    }
    std::cout<<"arc-arc tangent: both orders, signed sweeps, external/internal branches, malformed contacts and atomic refusals passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
