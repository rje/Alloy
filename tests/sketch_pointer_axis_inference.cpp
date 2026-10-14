// SPDX-License-Identifier: MIT
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
QJsonObject accept(const SketchGeometry::SolveResult& result){
    if(!result.ok)throw std::runtime_error(result.error.toStdString());return result.feature;
}
QJsonObject lastLine(const QJsonObject& sketch){return sketch["parameters"].toObject()["entities"].toArray().last().toObject();}
QJsonObject point(const QJsonObject& sketch,const QJsonValue& id){
    for(const auto& row:sketch["parameters"].toObject()["points"].toArray())
        if(row.toObject().value("id")==id)return row.toObject();
    return {};
}
bool hasAxis(const QJsonObject& sketch,const QJsonValue& lineId,const QString& type){
    for(const auto& row:sketch["parameters"].toObject()["constraints"].toArray()) {
        const auto relation=row.toObject();
        if(relation.value("type")==type && relation.value("entityIds").toArray()==QJsonArray{lineId})return true;
    }
    return false;
}
}

int main(int argc,char** argv){
    QCoreApplication app(argc,argv);
    try {
        const QString owner="13fa543c-e237-4e2e-947c-320071321821";
        auto sketch=SketchGeometry::newFeature(owner,"XY");
        sketch=accept(SketchGeometry::addPointerEntity(sketch,"line",{{"u1Mm",-50},{"v1Mm",75},
            {"u2Mm",0},{"v2Mm",0},{"inferAxis",true},
            {"endSnapTarget",QJsonObject{{"kind","origin"}}}}));
        const auto diagonal=lastLine(sketch);check(!hasAxis(sketch,diagonal.value("id"),"horizontal"),"diagonal remains free");
        const auto diagonalEnd=diagonal.value("endPointId");
        sketch=accept(SketchGeometry::addPointerEntity(sketch,"line",{{"u1Mm",0},{"v1Mm",0},
            {"u2Mm",100},{"v2Mm",0.3},{"inferAxis",true},
            {"startSnapTarget",QJsonObject{{"kind","point"},{"pointId",diagonalEnd}}}}));
        const auto bottom=lastLine(sketch);check(hasAxis(sketch,bottom.value("id"),"horizontal"),"near-horizontal pointer line retains H");
        const auto bottomStart=point(sketch,bottom.value("startPointId")),bottomEnd=point(sketch,bottom.value("endPointId"));
        check(std::abs(bottomStart.value("vMm").toDouble()-bottomEnd.value("vMm").toDouble())<1e-6,
            "H relation solves exact horizontal line");
        check(std::abs(point(sketch,diagonalEnd).value("vMm").toDouble())<1e-6,"source snap point stays fixed");
        sketch=accept(SketchGeometry::addPointerEntity(sketch,"line",{{"u1Mm",100},{"v1Mm",0},
            {"u2Mm",100.4},{"v2Mm",100},{"inferAxis",true},
            {"startSnapTarget",QJsonObject{{"kind","point"},{"pointId",bottom.value("endPointId")}}}}));
        const auto right=lastLine(sketch);check(hasAxis(sketch,right.value("id"),"vertical"),"near-vertical pointer line retains V");
        const auto rightStart=point(sketch,right.value("startPointId")),rightEnd=point(sketch,right.value("endPointId"));
        check(std::abs(rightStart.value("uMm").toDouble()-rightEnd.value("uMm").toDouble())<1e-6,
            "V relation solves exact vertical line");
        sketch=accept(SketchGeometry::addPointerEntity(sketch,"line",{{"u1Mm",100},{"v1Mm",100},
            {"u2Mm",-50},{"v2Mm",75},{"inferAxis",true},
            {"startSnapTarget",QJsonObject{{"kind","point"},{"pointId",right.value("endPointId")}}},
            {"endSnapTarget",QJsonObject{{"kind","point"},{"pointId",diagonal.value("startPointId")}}}}));
        const auto top=lastLine(sketch);check(!hasAxis(sketch,top.value("id"),"horizontal"),
            "sloped closing line remains free until Perpendicular");

        auto snapped=SketchGeometry::newFeature(owner,"XY");
        snapped=accept(SketchGeometry::addStandalonePoint(snapped,{{"uMm",0},{"vMm",0}}));
        const auto a=lastLine(snapped).value("pointId");
        snapped=accept(SketchGeometry::addStandalonePoint(snapped,{{"uMm",100},{"vMm",0.4}}));
        const auto b=lastLine(snapped).value("pointId");
        const auto prior=snapped;
        snapped=accept(SketchGeometry::addPointerEntity(snapped,"line",{{"u1Mm",0},{"v1Mm",0},
            {"u2Mm",100},{"v2Mm",0.4},{"inferAxis",true},
            {"startSnapTarget",QJsonObject{{"kind","point"},{"pointId",a}}},
            {"endSnapTarget",QJsonObject{{"kind","point"},{"pointId",b}}}}));
        check(!hasAxis(snapped,lastLine(snapped).value("id"),"horizontal"),
            "two misaligned retained snaps suppress inferred H");
        check(point(snapped,a)==point(prior,a) && point(snapped,b)==point(prior,b),
            "suppressed inference cannot move retained snap targets");
        auto suppressed=SketchGeometry::newFeature(owner,"XY");
        suppressed=accept(SketchGeometry::addPointerEntity(suppressed,"line",{{"u1Mm",0},{"v1Mm",0},
            {"u2Mm",100},{"v2Mm",0.3}}));
        check(!hasAxis(suppressed,lastLine(suppressed).value("id"),"horizontal"),
            "explicit suppression leaves near-axis Line free");
        std::cout<<"Pointer Line H/V inference contract passed\n";return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
