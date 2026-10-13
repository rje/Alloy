// SPDX-License-Identifier: MIT
#include "SketchArcSlot.h"
#include <QJsonArray>
#include <QMap>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace {
constexpr double degrees=std::numbers::pi/180.0;
constexpr double minimumLength=0.001;
void require(bool condition,const char* message){if(!condition)throw std::invalid_argument(message);}
double finite(const QJsonObject& object,const char* key){
    const auto value=object.value(QLatin1String(key));
    require(value.isDouble() && std::isfinite(value.toDouble()),"Arc slot has a nonfinite parameter");
    return value.toDouble();
}
QString id(const QJsonObject& object,const char* key){
    const auto result=object.value(QLatin1String(key)).toString();
    require(!result.isEmpty(),"Arc slot has a missing retained reference");return result;
}
}

namespace SketchArcSlot {
QSet<QString> derivedPointIds(const QJsonObject& parameters){
    QSet<QString> ids;
    QMap<QString,QJsonObject> entities;
    for(const auto& value:parameters["entities"].toArray()){
        const auto e=value.toObject();entities[e["id"].toString()]=e;
    }
    for(const auto& value:parameters["arcSlotGroups"].toArray()){
        const auto group=value.toObject();
        for(const auto& key:{"outerArcId","innerArcId"}){
            const auto e=entities[id(group,key)];
            ids.insert(id(e,"startPointId"));ids.insert(id(e,"endPointId"));
        }
    }
    return ids;
}
QSet<QString> derivedArcIds(const QJsonObject& parameters){
    QSet<QString> ids;
    for(const auto& value:parameters["arcSlotGroups"].toArray()){
        const auto group=value.toObject();
        for(const auto& key:{"outerArcId","innerArcId"})ids.insert(id(group,key));
        for(const auto& cap:group["capIds"].toArray())ids.insert(cap.toString());
    }
    return ids;
}
QSet<QString> capIds(const QJsonObject& parameters){
    QSet<QString> ids;
    for(const auto& value:parameters["arcSlotGroups"].toArray())
        for(const auto& cap:value.toObject()["capIds"].toArray())ids.insert(cap.toString());
    return ids;
}
QMap<QString,QString> capPrimaryIds(const QJsonObject& parameters){
    QMap<QString,QString> result;
    for(const auto& value:parameters["arcSlotGroups"].toArray()){
        const auto caps=value.toObject()["capIds"].toArray();
        require(caps.size()==2,"Arc slot needs two cap identities");
        const auto primary=caps.at(0).toString();
        require(!primary.isEmpty() && !caps.at(1).toString().isEmpty(),"Arc slot has a missing cap identity");
        for(const auto& cap:caps)result[cap.toString()]=primary;
    }
    return result;
}
QJsonObject derive(const QJsonObject& parameters){
    if(!parameters.contains("arcSlotGroups"))return parameters;
    auto result=parameters;auto points=result["points"].toArray(),entities=result["entities"].toArray();
    QMap<QString,int> pointIndex,entityIndex;
    for(int i=0;i<points.size();++i)pointIndex[points[i].toObject()["id"].toString()]=i;
    for(int i=0;i<entities.size();++i)entityIndex[entities[i].toObject()["id"].toString()]=i;
    auto point=[&](const QString& key){
        require(pointIndex.contains(key),"Arc slot lost a retained point");
        return points[pointIndex[key]].toObject();
    };
    auto entity=[&](const QString& key){
        require(entityIndex.contains(key),"Arc slot lost a retained arc");
        return entities[entityIndex[key]].toObject();
    };
    auto setPoint=[&](const QString& key,double x,double y){
        auto p=point(key);p["uMm"]=x;p["vMm"]=y;points[pointIndex[key]]=p;
    };
    auto setArc=[&](const QString& key,double radius,double start,double sweep){
        auto e=entity(key);e["radiusMm"]=radius;e["startAngleDeg"]=std::remainder(start/degrees,360.0);
        e["sweepAngleDeg"]=sweep/degrees;entities[entityIndex[key]]=e;
    };
    for(const auto& value:result["arcSlotGroups"].toArray()){
        const auto group=value.toObject();
        const auto path=entity(id(group,"pathArcId"));
        const auto origin=point(id(path,"centerPointId"));
        const auto first=point(id(path,"startPointId")),last=point(id(path,"endPointId"));
        const auto caps=group["capIds"].toArray();require(caps.size()==2,"Arc slot needs two cap identities");
        const double ox=finite(origin,"uMm"),oy=finite(origin,"vMm");
        const double ax=finite(first,"uMm"),ay=finite(first,"vMm");
        const double bx=finite(last,"uMm"),by=finite(last,"vMm");
        const double radius=finite(path,"radiusMm"),endRadius=finite(entity(caps[0].toString()),"radiusMm");
        require(radius>endRadius+minimumLength && endRadius>=minimumLength,"Arc slot path must exceed its rounded end radius");
        const double firstAngle=std::atan2(ay-oy,ax-ox),lastAngle=std::atan2(by-oy,bx-ox);
        const double sweep=std::remainder(lastAngle-firstAngle,2*std::numbers::pi);
        const int sign=group["sweepSign"].toInt();
        require((sign==1 || sign==-1) && sign*sweep>1e-5 && sign*sweep<std::numbers::pi-1e-5,
            "Arc slot path crossed its retained open sweep branch");
        const auto outer=entity(id(group,"outerArcId")),inner=entity(id(group,"innerArcId"));
        require(outer["type"]=="arc" && inner["type"]=="arc" && path["type"]=="arc",
            "Arc slot lost its analytic path or sides");
        const double c1=std::cos(firstAngle),s1=std::sin(firstAngle),c2=std::cos(lastAngle),s2=std::sin(lastAngle);
        setPoint(id(outer,"startPointId"),ox+(radius+endRadius)*c1,oy+(radius+endRadius)*s1);
        setPoint(id(outer,"endPointId"),ox+(radius+endRadius)*c2,oy+(radius+endRadius)*s2);
        setPoint(id(inner,"startPointId"),ox+(radius-endRadius)*c2,oy+(radius-endRadius)*s2);
        setPoint(id(inner,"endPointId"),ox+(radius-endRadius)*c1,oy+(radius-endRadius)*s1);
        setArc(id(group,"outerArcId"),radius+endRadius,firstAngle,sweep);
        setArc(id(group,"innerArcId"),radius-endRadius,lastAngle,-sweep);
        setArc(caps[0].toString(),endRadius,lastAngle,sign*std::numbers::pi);
        setArc(caps[1].toString(),endRadius,firstAngle+std::numbers::pi,sign*std::numbers::pi);
    }
    result["points"]=points;result["entities"]=entities;return result;
}
}
