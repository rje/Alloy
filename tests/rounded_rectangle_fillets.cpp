// SPDX-License-Identifier: MIT
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QJsonArray>
#include <QUuid>
#include <cmath>
#include <iostream>
#include <limits>
#include <numbers>
#include <stdexcept>

static void check(bool ok,const QString& message){if(!ok)throw std::runtime_error(message.toStdString());}
static QJsonObject accepted(const SketchGeometry::SolveResult& result){check(result.ok,result.error);return result.feature;}
static QJsonObject dimension(const QJsonObject& sketch,const QString& id){
    for(const auto& value:sketch["parameters"].toObject()["dimensions"].toArray())if(value.toObject()["id"]==id)return value.toObject();return {};
}
static QJsonObject point(const QJsonObject& sketch,const QString& id){
    for(const auto& value:sketch["parameters"].toObject()["points"].toArray())if(value.toObject()["id"]==id)return value.toObject();return {};
}
static double span(const QJsonObject& sketch,const QJsonObject& d){
    const auto ids=d["pointIds"].toArray();const auto a=point(sketch,ids[0].toString()),b=point(sketch,ids[1].toString());
    return std::abs(d["kind"]=="horizontal" ? b["uMm"].toDouble()-a["uMm"].toDouble() : b["vMm"].toDouble()-a["vMm"].toDouble());
}
static QPair<double,double> boundarySize(const QJsonObject& sketch){
    double minU=std::numeric_limits<double>::infinity(),maxU=-minU,minV=minU,maxV=-minU;
    for(const auto& value:sketch["parameters"].toObject()["entities"].toArray()){
        const auto e=value.toObject();
        for(const auto& field:{"startPointId","endPointId"}){
            const auto p=point(sketch,e[field].toString());
            minU=std::min(minU,p["uMm"].toDouble());maxU=std::max(maxU,p["uMm"].toDouble());
            minV=std::min(minV,p["vMm"].toDouble());maxV=std::max(maxV,p["vMm"].toDouble());
        }
    }
    return {maxU-minU,maxV-minV};
}

int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    auto sketch=SketchGeometry::newFeature(QUuid::createUuid().toString(QUuid::WithoutBraces),"XY");
    sketch=accepted(SketchGeometry::addPointerEntity(sketch,"rectangle",{{"u1Mm",0},{"v1Mm",0},{"u2Mm",60},{"v2Mm",-30},{"startSnapTarget",QJsonObject{{"kind","origin"}}}}));
    const auto source=sketch["parameters"].toObject()["entities"].toArray();
    QStringList lines;for(const auto& value:source)lines<<value.toObject()["id"].toString();
    sketch=accepted(SketchGeometry::addDimension(sketch,{{"kind","horizontal"},{"entityId",lines[0]},{"valueMm",60}}));
    const auto widthId=sketch["parameters"].toObject()["dimensions"].toArray().last().toObject()["id"].toString();
    sketch=accepted(SketchGeometry::addDimension(sketch,{{"kind","vertical"},{"entityId",lines[1]},{"valueMm",30}}));
    const auto heightId=sketch["parameters"].toObject()["dimensions"].toArray().last().toObject()["id"].toString();
    for(int i=0;i<4;++i){
        const auto before=sketch;
        const auto corners=QJsonArray{QJsonObject{{"entityIds",QJsonArray{lines[i],lines[(i+1)%4]}}}};
        const auto result=SketchGeometry::filletCorners(sketch,{{"corners",corners},{"radiusMm",6}});
        check(result.ok,QString("corner %1: %2").arg(i).arg(result.error));
        if(i==3)check(result.diagnostics["releasedOriginCoincidence"].toBool(),"Origin anchor release was not disclosed");
        sketch=result.feature;
        const auto width=dimension(sketch,widthId),height=dimension(sketch,heightId);
        check(!width.isEmpty() && !height.isEmpty(),"Overall dimension ID lost");
        for(const auto& d:{width,height})check(!d["driving"].toBool() && !d.contains("valueMm"),"Overall span is not an honest reference");
        check(std::abs(span(sketch,width)-60)<1e-5 && std::abs(span(sketch,height)-30)<1e-5,"Overall spans changed to shortened side length");
        check(sketch!=before,"Fillet did not change source sketch");
    }
    int linesLeft=0,arcs=0;QString radiusId;
    for(const auto& value:sketch["parameters"].toObject()["entities"].toArray()){
        const auto entity=value.toObject();linesLeft+=entity["type"]=="line";arcs+=entity["type"]=="arc";
    }
    for(const auto& value:sketch["parameters"].toObject()["dimensions"].toArray())if(value.toObject()["kind"]=="radius")radiusId=value.toObject()["id"].toString();
    check(linesLeft==4 && arcs==4,"Rounded rectangle needs four surviving lines and four real arcs");
    check(!radiusId.isEmpty(),"Radius dimension missing");
    const auto evaluated=SketchGeometry::evaluateFeature(sketch);
    check(evaluated.profiles.size()==1,"Rounded rectangle must have one closed profile");
    check(std::abs(evaluated.profiles[0].summary["areaMm2"].toDouble()-(1800-36*(4-std::numbers::pi)))<1e-5,"Rounded rectangle area is not analytic R6 area");
    const auto edited=SketchGeometry::updateDimension(sketch,{{"dimensionId",radiusId},{"valueMm",5.5}});
    check(edited.ok,QString("Radius edit: %1").arg(edited.error));
    check(!dimension(edited.feature,widthId)["driving"].toBool() && !dimension(edited.feature,heightId)["driving"].toBool(),"Radius edit lost overall reference dimensions");
    const auto editedSize=boundarySize(edited.feature);
    check(std::abs(span(edited.feature,dimension(edited.feature,widthId))-editedSize.first)<1e-5
        && std::abs(span(edited.feature,dimension(edited.feature,heightId))-editedSize.second)<1e-5,
        "Edited dimensions no longer measure overall boundary bounds");
    std::cout<<"Edited measured spans "<<editedSize.first<<" x "<<editedSize.second<<" mm\n";
    std::cout<<"PASS: four sequential rectangle fillets retain 60/30 overall reference IDs\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
