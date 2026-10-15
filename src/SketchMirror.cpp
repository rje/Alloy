// SPDX-License-Identifier: MIT
#include "SketchMirror.h"
#include <QJsonArray>
#include <QMap>
#include <QSet>
#include <QPointF>
#include <QUuid>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace {
QString id(){return QUuid::createUuid().toString(QUuid::WithoutBraces);}
bool uuid(const QString& value){return !QUuid(value).isNull() && QUuid(value).toString(QUuid::WithoutBraces)==value;}
void require(bool ok,const char* reason){if(!ok)throw std::invalid_argument(reason);}
QMap<QString,QJsonObject> objects(const QJsonArray& values){QMap<QString,QJsonObject> out;for(const auto& value:values){const auto object=value.toObject();out.insert(object["id"].toString(),object);}return out;}
QPointF position(const QJsonObject& p){return {p["uMm"].toDouble(),p["vMm"].toDouble()};}
QPointF reflected(QPointF p,QPointF a,QPointF b){const auto d=b-a;const double norm=d.x()*d.x()+d.y()*d.y();require(norm>1e-6,"Mirror axis is degenerate");const auto foot=a+d*(QPointF::dotProduct(p-a,d)/norm);return foot*2-p;}
QStringList pointKeys(const QJsonObject& entity){return entity["type"]=="arc" ? QStringList{"startPointId","endPointId","centerPointId"} : QStringList{"startPointId","endPointId"};}
}
namespace SketchMirror {
bool supportedSource(const QJsonObject& entity){return entity["type"]=="line" || entity["type"]=="arc" || entity["type"]=="fitSpline";}
QStringList pointIds(const QJsonObject& entity){
    QStringList ids;
    if(entity["type"]=="fitSpline")for(const auto& value:entity["pointIds"].toArray())ids.append(value.toString());
    else for(const auto& key:pointKeys(entity))ids.append(entity[key].toString());
    return ids;
}
QJsonObject refreshSplineHandles(const QJsonObject& parameters){
    auto result=parameters;auto entities=parameters["entities"].toArray();auto entityById=objects(entities);
    const auto points=objects(parameters["points"].toArray());
    for(const auto& groupValue:parameters["mirrorGroups"].toArray()){
        const auto group=groupValue.toObject(),axis=entityById.value(group["axisEntityId"].toString());
        const auto direction=position(points.value(axis["endPointId"].toString()))-position(points.value(axis["startPointId"].toString()));
        const double norm=QPointF::dotProduct(direction,direction);require(norm>1e-6,"Mirror axis is degenerate");
        for(const auto& pairValue:group["entityPairs"].toArray()){
            const auto pair=pairValue.toObject(),source=entityById.value(pair["sourceEntityId"].toString());
            if(source["type"]!="fitSpline")continue;
            auto target=entityById.value(pair["resultEntityId"].toString());QJsonObject handles;
            const auto sourceIds=source["pointIds"].toArray(),targetIds=target["pointIds"].toArray();
            require(sourceIds.size()==targetIds.size(),"Mirror spline fit-point mapping is incomplete");
            const auto sourceHandles=source["tangentHandles"].toObject();
            for(int i=0;i<sourceIds.size();++i)if(sourceHandles.contains(sourceIds[i].toString())){
                const auto handle=sourceHandles.value(sourceIds[i].toString()).toObject();
                const QPointF vector(handle["duMm"].toDouble(),handle["dvMm"].toDouble());
                const auto reflected=2*direction*(QPointF::dotProduct(vector,direction)/norm)-vector;
                handles[targetIds[i].toString()]=QJsonObject{{"duMm",reflected.x()},{"dvMm",reflected.y()}};
            }
            target["tangentHandles"]=handles;entityById[target["id"].toString()]=target;
        }
    }
    for(int i=0;i<entities.size();++i)entities[i]=entityById.value(entities[i].toObject()["id"].toString());
    result["entities"]=entities;return result;
}
QString validate(const QJsonObject& parameters){
    if(!parameters.contains("mirrorGroups"))return {};
    if(!parameters["mirrorGroups"].isArray())return "Mirror groups must be an array";
    const auto entities=objects(parameters["entities"].toArray()),points=objects(parameters["points"].toArray());
    QSet<QString> groupIds,resultIds,resultPoints;
    for(const auto& value:parameters["mirrorGroups"].toArray()){
        if(!value.isObject())return "Mirror group must be an object";const auto group=value.toObject();
        if(group.size()!=4 || !uuid(group["id"].toString()) || groupIds.contains(group["id"].toString()) ||
            !group["entityPairs"].isArray() || !group["pointPairs"].isArray())return "Invalid Mirror group";
        groupIds.insert(group["id"].toString());const auto axis=entities.value(group["axisEntityId"].toString());
        if(axis["type"]!="line")return "Mirror axis line is missing";
        const auto axisStart=position(points.value(axis["startPointId"].toString()));
        const auto axisEnd=position(points.value(axis["endPointId"].toString()));
        if(group["entityPairs"].toArray().isEmpty() || group["entityPairs"].toArray().size()>64)return "Mirror requires 1..64 source entities";
        QMap<QString,QString> pointMap;QSet<QString> sourceIds;
        for(const auto& pairValue:group["pointPairs"].toArray()){
            const auto pair=pairValue.toObject();const auto source=pair["sourcePointId"].toString(),result=pair["resultPointId"].toString();
            if(pair.size()!=2 || !points.contains(source) || !points.contains(result) || source==result ||
                pointMap.contains(source) || resultPoints.contains(result))return "Invalid Mirror point mapping";
            pointMap[source]=result;resultPoints.insert(result);
        }
        for(const auto& pairValue:group["entityPairs"].toArray()){
            const auto pair=pairValue.toObject();const auto sourceId=pair["sourceEntityId"].toString(),resultId=pair["resultEntityId"].toString();
            const auto source=entities.value(sourceId),result=entities.value(resultId);
            if(pair.size()!=2 || source.isEmpty() || result.isEmpty() || sourceId==resultId || sourceId==group["axisEntityId"] ||
                sourceIds.contains(sourceId) || resultIds.contains(resultId) || source["type"]!=result["type"] ||
                !supportedSource(source) || source["construction"]!=result["construction"] || source["centerline"]!=result["centerline"])
                return "Invalid Mirror entity mapping";
            sourceIds.insert(sourceId);resultIds.insert(resultId);
            const auto sourcePointIds=pointIds(source),resultPointIds=pointIds(result);
            if(sourcePointIds.size()!=resultPointIds.size())return "Mirror spline fit-point mapping is incomplete";
            if(source["type"]=="fitSpline"){
                if(sourcePointIds.size()<2)return "Mirror spline fit-point mapping is incomplete";
                const auto span=position(points.value(sourcePointIds.first()))-position(points.value(sourcePointIds.last()));
                if(std::hypot(span.x(),span.y())<.001)return "Mirror supports open fit splines";
            }
            for(int pointIndex=0;pointIndex<sourcePointIds.size();++pointIndex) {
                const auto sourcePointId=sourcePointIds[pointIndex],resultPointId=resultPointIds[pointIndex];
                if(pointMap.contains(sourcePointId)){
                    if(pointMap.value(sourcePointId)!=resultPointId)return "Mirror entity point mapping is incomplete";
                } else {
                    const auto sourcePoint=points.value(sourcePointId);
                    if(resultPointId!=sourcePointId || sourcePoint.isEmpty() ||
                        std::hypot(reflected(position(sourcePoint),axisStart,axisEnd).x()-sourcePoint["uMm"].toDouble(),
                                   reflected(position(sourcePoint),axisStart,axisEnd).y()-sourcePoint["vMm"].toDouble())>1e-6)
                        return "Mirror shared point must remain on the axis";
                }
            }
        }
        if(pointMap.size()!=group["pointPairs"].toArray().size())return "Duplicate Mirror point mapping";
    }
    for(const auto& groupValue:parameters["mirrorGroups"].toArray()){
        const auto group=groupValue.toObject();if(resultIds.contains(group["axisEntityId"].toString()))return "Mirror axis cannot be another Mirror result";
        for(const auto& pairValue:group["entityPairs"].toArray())if(resultIds.contains(pairValue.toObject()["sourceEntityId"].toString()))return "Mirror result cannot be a source in this slice";
    }
    // Results are solved symmetry geometry. Source and axis remain the editable
    // handles in this foundation; driven measurements of results remain useful.
    for(const auto& value:parameters["constraints"].toArray()){
        const auto relation=value.toObject();
        for(const auto& id:relation["pointIds"].toArray())if(resultPoints.contains(id.toString()))return "Constrain the Mirror source, not its result";
        for(const auto& id:relation["entityIds"].toArray())if(resultIds.contains(id.toString()))return "Constrain the Mirror source, not its result";
    }
    for(const auto& value:parameters["dimensions"].toArray()){
        const auto dimension=value.toObject();if(!dimension["driving"].toBool())continue;
        if(resultIds.contains(dimension["entityId"].toString()))return "Drive the Mirror source, not its result";
        for(const auto& id:dimension["pointIds"].toArray())if(resultPoints.contains(id.toString()))return "Drive the Mirror source, not its result";
        for(const auto& id:dimension["entityIds"].toArray())if(resultIds.contains(id.toString()))return "Drive the Mirror source, not its result";
    }
    return {};
}
QJsonObject seed(const QJsonObject& parameters){
    if(!parameters.contains("mirrorGroups"))return parameters;
    auto out=parameters;auto points=out["points"].toArray(),entities=out["entities"].toArray();
    auto pointById=objects(points),entityById=objects(entities);
    for(const auto& groupValue:out["mirrorGroups"].toArray()){
        const auto group=groupValue.toObject(),axis=entityById.value(group["axisEntityId"].toString());
        require(axis["type"]=="line","Mirror axis line is missing");
        const auto a=position(pointById.value(axis["startPointId"].toString())),b=position(pointById.value(axis["endPointId"].toString()));
        const double axisAngle=std::atan2(b.y()-a.y(),b.x()-a.x())*180/std::numbers::pi;
        for(const auto& pairValue:group["pointPairs"].toArray()){
            const auto pair=pairValue.toObject();const auto source=pointById.value(pair["sourcePointId"].toString());
            auto result=pointById.value(pair["resultPointId"].toString());const auto p=reflected(position(source),a,b);
            result["uMm"]=p.x();result["vMm"]=p.y();pointById[result["id"].toString()]=result;
        }
        for(const auto& pairValue:group["entityPairs"].toArray()){
            const auto pair=pairValue.toObject(),source=entityById.value(pair["sourceEntityId"].toString());
            auto result=entityById.value(pair["resultEntityId"].toString());
            if(source["type"]=="arc"){
                result["radiusMm"]=source["radiusMm"];
                result["startAngleDeg"]=std::remainder(2*axisAngle-source["startAngleDeg"].toDouble(),360.0);
                result["sweepAngleDeg"]=-source["sweepAngleDeg"].toDouble();
            }
            entityById[result["id"].toString()]=result;
        }
    }
    for(int i=0;i<points.size();++i)points[i]=pointById.value(points[i].toObject()["id"].toString());
    for(int i=0;i<entities.size();++i)entities[i]=entityById.value(entities[i].toObject()["id"].toString());
    out["points"]=points;out["entities"]=entities;return refreshSplineHandles(out);
}
QJsonObject create(const QJsonObject& parameters,const QJsonObject& arguments){
    const auto ids=arguments["sourceEntityIds"].toArray();const auto axisId=arguments["axisEntityId"].toString();
    require(ids.size()>=1 && ids.size()<=64,"Mirror requires 1..64 source entities");
    auto out=parameters;auto points=out["points"].toArray(),entities=out["entities"].toArray(),groups=out["mirrorGroups"].toArray();
    auto pointById=objects(points),entityById=objects(entities);const auto axis=entityById.value(axisId);
    require(axis["type"]=="line","Mirror requires an existing straight axis line");
    const auto a=position(pointById.value(axis["startPointId"].toString())),b=position(pointById.value(axis["endPointId"].toString()));
    require(std::hypot(b.x()-a.x(),b.y()-a.y())>=0.001,"Mirror axis is degenerate");
    const double axisAngle=std::atan2(b.y()-a.y(),b.x()-a.x())*180/std::numbers::pi;
    QSet<QString> selected;QMap<QString,QString> pointMap;QJsonArray entityPairs,pointPairs;
    for(const auto& value:ids){
        require(value.isString(),"Mirror source IDs must be strings");const auto sourceId=value.toString();const auto source=entityById.value(sourceId);
        require(!selected.contains(sourceId) && sourceId!=axisId && supportedSource(source),"Mirror requires distinct line, arc or open fit-spline sources separate from its axis");
        selected.insert(sourceId);auto result=source;result["id"]=id();
        QJsonArray splinePoints;
        for(const auto& sourcePointId:pointIds(source)){
            if(!pointMap.contains(sourcePointId)){
                const auto sourcePoint=pointById.value(sourcePointId);require(!sourcePoint.isEmpty(),"Mirror source point is missing");
                auto targetPoint=sourcePoint;targetPoint["id"]=id();const auto p=reflected(position(sourcePoint),a,b);
                if(std::hypot(p.x()-sourcePoint["uMm"].toDouble(),p.y()-sourcePoint["vMm"].toDouble())<=1e-6){
                    pointMap[sourcePointId]=sourcePointId;continue;
                }
                targetPoint["uMm"]=p.x();targetPoint["vMm"]=p.y();points.append(targetPoint);
                pointMap[sourcePointId]=targetPoint["id"].toString();pointPairs.append(QJsonObject{{"sourcePointId",sourcePointId},{"resultPointId",targetPoint["id"]}});
            }
            if(source["type"]=="fitSpline")splinePoints.append(pointMap.value(sourcePointId));
            else for(const auto& key:pointKeys(source))if(source[key]==sourcePointId)result[key]=pointMap.value(sourcePointId);
        }
        if(source["type"]=="fitSpline"){
            result["pointIds"]=splinePoints;result["startPointId"]=splinePoints.first();result["endPointId"]=splinePoints.last();
            QJsonObject handles;const auto sourceHandles=source["tangentHandles"].toObject();
            for(auto handle=sourceHandles.begin();handle!=sourceHandles.end();++handle)handles[pointMap.value(handle.key())]=handle.value();
            result["tangentHandles"]=handles;
        }
        if(source["type"]=="arc"){
            result["startAngleDeg"]=std::remainder(2*axisAngle-source["startAngleDeg"].toDouble(),360.0);
            result["sweepAngleDeg"]=-source["sweepAngleDeg"].toDouble();
        }
        entities.append(result);entityPairs.append(QJsonObject{{"sourceEntityId",sourceId},{"resultEntityId",result["id"]}});
    }
    groups.append(QJsonObject{{"id",id()},{"axisEntityId",axisId},{"entityPairs",entityPairs},{"pointPairs",pointPairs}});
    out["points"]=points;out["entities"]=entities;out["mirrorGroups"]=groups;
    const auto error=validate(out);require(error.isEmpty(),error.toUtf8().constData());return refreshSplineHandles(out);
}
}
