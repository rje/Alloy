// SPDX-License-Identifier: MIT
#include "SketchGeometry.h"
#include <QMap>
#include <QUuid>
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace {
struct Point { double u=0,v=0; };
Point operator-(Point a,Point b){return {a.u-b.u,a.v-b.v};}
Point operator+(Point a,Point b){return {a.u+b.u,a.v+b.v};}
Point operator*(Point a,double t){return {a.u*t,a.v*t};}
double dot(Point a,Point b){return a.u*b.u+a.v*b.v;}
double cross(Point a,Point b){return a.u*b.v-a.v*b.u;}
double length(Point a){return std::hypot(a.u,a.v);}
QString freshId(){return QUuid::createUuid().toString(QUuid::WithoutBraces);}
struct Hit {double distance=0;Point location;QString lineId,pointId;};
}

SketchGeometry::SolveResult SketchGeometry::extendLine(const QJsonObject& draft,const QJsonObject& arguments,const QJsonObject& context) {
    try {
        for(auto it=arguments.begin();it!=arguments.end();++it)
            if(it.key()!="entityId" && it.key()!="uMm" && it.key()!="vMm")throw std::invalid_argument("Unknown Extend argument");
        if(!arguments.value("uMm").isDouble() || !arguments.value("vMm").isDouble())
            throw std::invalid_argument("Extend needs a finite sketch point");
        const Point picked{arguments.value("uMm").toDouble(),arguments.value("vMm").toDouble()};
        if(!std::isfinite(picked.u) || !std::isfinite(picked.v))throw std::invalid_argument("Extend point is nonfinite");
        const auto baseline=solveDraft(draft,context);
        if(!baseline.ok)return {false,draft,baseline.diagnostics,baseline.error};
        auto parameters=baseline.feature.value("parameters").toObject();
        const auto entities=parameters.value("entities").toArray();
        QMap<QString,QJsonObject> points;
        for(const auto& value:parameters.value("points").toArray()){
            const auto point=value.toObject();points.insert(point.value("id").toString(),point);
        }
        auto point=[&](const QString& id){
            const auto row=points.value(id);if(row.isEmpty())throw std::invalid_argument("Extend point reference unavailable");
            return Point{row.value("uMm").toDouble(),row.value("vMm").toDouble()};
        };
        const auto sourceId=arguments.value("entityId").toString();QJsonObject source;
        for(const auto& value:entities)if(value.toObject().value("id")==sourceId)source=value.toObject();
        if(source.value("type")!="line" || source.value("projected").toBool())
            throw std::invalid_argument("Extend requires an authored straight line");
        const auto startId=source.value("startPointId").toString(),endId=source.value("endPointId").toString();
        const auto start=point(startId),end=point(endId);
        const auto span=end-start;const double sourceLength=length(span);
        if(sourceLength<.001)throw std::invalid_argument("Source line is too short to extend");
        const double startPick=length(picked-start),endPick=length(picked-end);
        if(std::abs(startPick-endPick)<1e-6)throw std::invalid_argument("Pick closer to the endpoint to extend");
        const bool extendEnd=endPick<startPick;
        const auto selected=extendEnd ? end:start,opposite=extendEnd ? start:end;
        const auto direction=selected-opposite;
        QVector<Hit> hits;double nearestOverlap=std::numeric_limits<double>::infinity();
        for(const auto& value:entities){
            const auto other=value.toObject();if(other.value("id")==sourceId || other.value("type")!="line" || other.value("projected").toBool())continue;
            const auto targetA=point(other.value("startPointId").toString()),targetB=point(other.value("endPointId").toString());
            const auto edge=targetB-targetA;const auto denominator=cross(direction,edge);
            const double scale=sourceLength*length(edge);
            if(scale<1e-6)continue;
            if(std::abs(denominator)<=1e-10*scale){
                if(std::abs(cross(targetA-opposite,direction))<=1e-8*scale){
                    const auto squared=dot(direction,direction);
                    const double a=dot(targetA-opposite,direction)/squared,b=dot(targetB-opposite,direction)/squared;
                    if(std::max(a,b)>1+1e-8)
                        nearestOverlap=std::min(nearestOverlap,std::max(0.0,std::min(a,b)-1)*sourceLength);
                }
                continue;
            }
            const auto offset=targetA-opposite;
            const double sourceT=cross(offset,edge)/denominator,targetT=cross(offset,direction)/denominator;
            if(sourceT<=1+1e-8 || targetT< -1e-8 || targetT>1+1e-8)continue;
            const auto intersection=opposite+direction*sourceT;
            const double travel=length(intersection-selected);
            if(travel<.001 || travel>100000)continue;
            QString targetPoint;
            if(length(intersection-targetA)<=1e-6)targetPoint=other.value("startPointId").toString();
            else if(length(intersection-targetB)<=1e-6)targetPoint=other.value("endPointId").toString();
            hits.push_back({travel,intersection,other.value("id").toString(),targetPoint});
        }
        if(hits.isEmpty()){
            if(std::isfinite(nearestOverlap))throw std::invalid_argument("Extend overlaps another line; intersection is ambiguous");
            throw std::invalid_argument("No forward line intersection is available");
        }
        std::sort(hits.begin(),hits.end(),[](const Hit& a,const Hit& b){return a.distance<b.distance;});
        if(nearestOverlap<=hits[0].distance+std::max(1e-6,hits[0].distance*1e-8))
            throw std::invalid_argument("Nearest Extend intersection overlaps another line");
        if(hits.size()>1 && hits[1].distance-hits[0].distance<=std::max(1e-6,hits[0].distance*1e-8))
            throw std::invalid_argument("Nearest Extend intersection is ambiguous");
        const auto hit=hits.front();const auto movingId=extendEnd ? endId:startId;
        auto updated=points.value(movingId);updated["uMm"]=hit.location.u;updated["vMm"]=hit.location.v;
        QJsonArray updatedPoints;
        for(const auto& value:parameters.value("points").toArray()){
            const auto row=value.toObject();updatedPoints.append(row.value("id")==movingId ? updated:row);
        }
        parameters["points"]=updatedPoints;
        auto constraints=parameters.value("constraints").toArray();
        QJsonObject relation{{"id",freshId()},{"type",hit.pointId.isEmpty() ? "pointOnCurve":"coincident"},
            {"pointIds",hit.pointId.isEmpty() ? QJsonArray{movingId}:QJsonArray{movingId,hit.pointId}}};
        if(hit.pointId.isEmpty())relation["entityIds"]=QJsonArray{hit.lineId};
        constraints.append(relation);parameters["constraints"]=constraints;
        auto candidate=baseline.feature;candidate["parameters"]=parameters;
        auto result=solveDraft(candidate,context);
        if(!result.ok)return {false,draft,result.diagnostics,result.error};
        QMap<QString,Point> solved;
        for(const auto& value:result.feature.value("parameters").toObject().value("points").toArray()){
            const auto row=value.toObject();solved.insert(row.value("id").toString(),{row.value("uMm").toDouble(),row.value("vMm").toDouble()});
        }
        for(auto it=points.cbegin();it!=points.cend();++it){
            const auto original=point(it.key());const auto now=solved.value(it.key());
            if(it.key()==movingId){if(length(now-hit.location)>1e-6)throw std::invalid_argument("Extend endpoint did not reach its intersection");}
            else if(length(now-original)>1e-6)throw std::invalid_argument("Extend would move existing sketch geometry");
        }
        result.diagnostics["extendedSegment"]=QJsonObject{{"entityId",sourceId},{"movingPointId",movingId},
            {"targetEntityId",hit.lineId},{"targetPointId",hit.pointId},{"relationId",relation.value("id")},
            {"u1Mm",selected.u},{"v1Mm",selected.v},{"u2Mm",hit.location.u},{"v2Mm",hit.location.v}};
        return result;
    }catch(const std::exception& error){return {false,draft,{{"operation","extend"},{"status","invalid"}},QString::fromUtf8(error.what())};}
}
