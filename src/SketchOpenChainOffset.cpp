// SPDX-License-Identifier: MIT
#include "SketchOpenChainOffset.h"
#include <IntAna2d_AnaIntersection.hxx>
#include <gp_Lin2d.hxx>
#include <gp_Pnt2d.hxx>
#include <gp_Dir2d.hxx>
#include <QJsonArray>
#include <QMap>
#include <QPointF>
#include <QUuid>
#include <cmath>
#include <array>
#include <stdexcept>

namespace {
constexpr double tolerance=1e-6;
void require(bool condition,const char* message){if(!condition)throw std::invalid_argument(message);}
QString fresh(){return QUuid::createUuid().toString(QUuid::WithoutBraces);}
bool validId(const QJsonValue& value){return value.isString() && !QUuid(value.toString()).isNull();}
QPointF location(const QJsonObject& point){return {point["uMm"].toDouble(),point["vMm"].toDouble()};}
double cross(QPointF a,QPointF b){return a.x()*b.y()-a.y()*b.x();}
double dot(QPointF a,QPointF b){return a.x()*b.x()+a.y()*b.y();}
double length(QPointF a){return std::hypot(a.x(),a.y());}
bool chainNear(QPointF a,QPointF b){return length(a-b)<tolerance;}
struct Segment {QString id;QPointF a,b;};
QPointF intersection(const Segment& a,const Segment& b){
    const auto av=a.b-a.a,bv=b.b-b.a;
    require(length(av)>0.001 && length(bv)>0.001,"Offset source line collapsed");
    IntAna2d_AnaIntersection query(gp_Lin2d(gp_Pnt2d(a.a.x(),a.a.y()),gp_Dir2d(av.x(),av.y())),
                                   gp_Lin2d(gp_Pnt2d(b.a.x(),b.a.y()),gp_Dir2d(bv.x(),bv.y())));
    require(query.IsDone() && query.NbPoints()==1,
            "Offset adjacent support lines lack one miter intersection");
    const auto p=query.Point(1).Value();
    require(std::isfinite(p.X()) && std::isfinite(p.Y()),"Offset miter is nonfinite");
    return {p.X(),p.Y()};
}
double fraction(const Segment& line,QPointF p){const auto d=line.b-line.a;return dot(p-line.a,d)/dot(d,d);}
QMap<QString,QPointF> pointMap(const QJsonObject& p){QMap<QString,QPointF> result;for(const auto& v:p["points"].toArray()){
    const auto o=v.toObject();result[o["id"].toString()]=location(o);}return result;}
QMap<QString,QJsonObject> lineMap(const QJsonObject& p){QMap<QString,QJsonObject> result;for(const auto& v:p["entities"].toArray()){
    const auto o=v.toObject();if(o["type"]=="line")result[o["id"].toString()]=o;}return result;}
Segment segment(const QString& id,const QMap<QString,QJsonObject>& lines,const QMap<QString,QPointF>& points){
    require(lines.contains(id),"Offset source line is unavailable");const auto e=lines[id];
    require(points.contains(e["startPointId"].toString()) && points.contains(e["endPointId"].toString()),
            "Offset source endpoint is unavailable");
    return {id,points[e["startPointId"].toString()],points[e["endPointId"].toString()]};
}
QJsonArray ids(const QJsonObject& group,const char* key,int count){
    const auto values=group.value(QLatin1String(key)).toArray();
    require(group.value(QLatin1String(key)).isArray() && values.size()==count,"Offset retained identity count changed");
    QSet<QString> unique;for(const auto& value:values){require(validId(value) && !unique.contains(value.toString()),"Offset has duplicate or invalid identities");unique.insert(value.toString());}
    return values;
}
QJsonObject group(const QJsonObject& p){const auto all=p.value("openChainOffsets").toArray();
    require(all.size()==1,"One retained open-chain offset is supported per sketch");return all.first().toObject();}
QJsonObject setCoordinate(const QJsonObject& p,const QMap<QString,QPointF>& derived){
    auto result=p;auto points=p["points"].toArray();QSet<QString> found;
    for(int i=0;i<points.size();++i){auto item=points[i].toObject();const auto id=item["id"].toString();if(!derived.contains(id))continue;
        item["uMm"]=derived[id].x();item["vMm"]=derived[id].y();points[i]=item;found.insert(id);}
    require(found.size()==derived.size(),"Offset lost a generated endpoint");result["points"]=points;return result;
}
}

namespace SketchOpenChainOffset {
QSet<QString> derivedPointIds(const QJsonObject& p){QSet<QString> result;
    for(const auto& v:p.value("openChainOffsets").toArray()){
        const auto g=v.toObject();for(const auto& id:g["generatedPointIds"].toArray())result.insert(id.toString());
        for(const auto& id:g["topIntersectionPointIds"].toArray())result.insert(id.toString());
    }return result;
}
QSet<QString> derivedEntityIds(const QJsonObject& p){QSet<QString> result;
    for(const auto& v:p.value("openChainOffsets").toArray())for(const auto& id:v.toObject()["generatedEntityIds"].toArray())result.insert(id.toString());return result;
}
QString validate(const QJsonObject& p){
    if(!p.contains("openChainOffsets"))return {};
    try{
        require(p["openChainOffsets"].isArray() && p["openChainOffsets"].toArray().size()==1,
                "One retained open-chain offset is supported per sketch");
        const auto g=group(p);const int extras=g.contains("distanceParameterId")+g.contains("sideSign");
        require(g.size()==5+extras+int(g.contains("topEntityId"))+int(g.contains("topIntersectionPointIds")) &&
                validId(g["id"]) && g["signedDistanceMm"].isDouble(),
                "Invalid retained open-chain offset");
        const auto d=g["signedDistanceMm"].toDouble();
        require(std::isfinite(d) && std::abs(d)>=0.001 && std::abs(d)<=100000,
                "Offset needs a finite nonzero signed distance");
        if(g.contains("distanceParameterId"))require(validId(g["distanceParameterId"]) && g["sideSign"].isDouble()
            && std::abs(g["sideSign"].toInt())==1,"Offset length binding is invalid");
        else require(!g.contains("sideSign"),"Offset side sign requires a length binding");
        const auto sources=ids(g,"sourceEntityIds",5),points=ids(g,"generatedPointIds",6),
            generated=ids(g,"generatedEntityIds",5);
        const auto tops=g.contains("topIntersectionPointIds") ? ids(g,"topIntersectionPointIds",2) : QJsonArray{};
        if(g.contains("topEntityId"))require(validId(g["topEntityId"]),"Offset original top line is unavailable");
        QSet<QString> unique;for(const auto& list:{sources,points,generated,tops})for(const auto& value:list){
            require(!unique.contains(value.toString()),"Offset identities overlap");unique.insert(value.toString());}
        if(g.contains("topEntityId"))require(!unique.contains(g["topEntityId"].toString()),"Offset chain must exclude original top line");
        const auto lines=lineMap(p);const auto locations=pointMap(p);
        if(g.contains("topEntityId"))require(lines.contains(g["topEntityId"].toString()),"Offset original top line is unavailable");
        for(const auto& value:sources)require(lines.contains(value.toString()),"Offset source line is unavailable");
        for(const auto& value:points)require(locations.contains(value.toString()),"Offset generated point is unavailable");
        for(const auto& value:tops)require(locations.contains(value.toString()),"Offset top intersection is unavailable");
        for(int i=0;i<5;++i){const auto e=lines.value(generated[i].toString());
            require(e["type"]=="line" && e.size()==4 && e["startPointId"]==points[i] && e["endPointId"]==points[i+1],
                    "Offset generated line identity changed");}
        return {};
    }catch(const std::exception& e){return QString::fromUtf8(e.what());}
}
QJsonObject derive(const QJsonObject& p){
    if(!p.contains("openChainOffsets"))return p;
    const auto error=validate(p);require(error.isEmpty(),"Invalid retained open-chain offset");
    const auto g=group(p);const auto sourceIds=g["sourceEntityIds"].toArray(),pointIds=g["generatedPointIds"].toArray();
    const auto lines=lineMap(p);const auto points=pointMap(p);
    std::array<Segment,5> source;
    for(int i=0;i<5;++i)source[i]=segment(sourceIds[i].toString(),lines,points);
    // The retained order fixes signed side; connectivity fixes each direction.
    require(!chainNear(source[0].a,source[0].b),"Offset source line collapsed");
    if(chainNear(source[0].a,source[1].a) || chainNear(source[0].a,source[1].b))std::swap(source[0].a,source[0].b);
    else require(chainNear(source[0].b,source[1].a)||chainNear(source[0].b,source[1].b),"Offset source chain is disconnected");
    for(int i=1;i<5;++i){
        if(chainNear(source[i].b,source[i-1].b))std::swap(source[i].a,source[i].b);
        require(chainNear(source[i].a,source[i-1].b) && !chainNear(source[i].b,source[i].a),
                "Offset source chain is branched or disconnected");
    }
    require(std::abs(source[0].a.y()-source[0].b.y())<tolerance &&
            std::abs(source[4].a.y()-source[4].b.y())<tolerance &&
            std::abs(source[0].b.y()-source[4].a.y())<tolerance,
            "Offset shoulders no longer share the original horizontal top support");
    const double distance=g["signedDistanceMm"].toDouble();
    std::array<Segment,5> shifted;
    for(int i=0;i<5;++i){const auto delta=source[i].b-source[i].a,n=QPointF(delta.y(),-delta.x())*(distance/length(delta));
        shifted[i]={source[i].id,source[i].a+n,source[i].b+n};}
    std::array<QPointF,6> derived;derived[0]=shifted[0].a;derived[5]=shifted[4].b;
    for(int i=1;i<5;++i)derived[i]=intersection(shifted[i-1],shifted[i]);
    std::array<Segment,5> output;
    for(int i=0;i<5;++i){
        output[i]={pointIds[i].toString(),derived[i],derived[i+1]};
        require(length(derived[i+1]-derived[i])>=0.001 &&
                dot(derived[i+1]-derived[i],source[i].b-source[i].a)>tolerance,
                "Offset changed source topology or collapsed a segment");
    }
    // The requested five-segment branch must remain simple. Adjacent contacts
    // are its authored miters; other crossings would change topology.
    for(int i=0;i<5;++i)for(int j=i+2;j<5;++j){
        const auto av=output[i].b-output[i].a,bv=output[j].b-output[j].a;
        if(std::abs(cross(av,bv))<1e-10)continue;
        const auto hit=intersection(output[i],output[j]);
        if(fraction(output[i],hit)>tolerance && fraction(output[i],hit)<1-tolerance &&
           fraction(output[j],hit)>tolerance && fraction(output[j],hit)<1-tolerance)
            throw std::invalid_argument("Offset self-intersects");
    }
    // On a cut front the removed central rim is not an edge. The retained
    // projected shoulder/slant junctions define its original top height.
    const Segment top=g.contains("topEntityId") ? segment(g["topEntityId"].toString(),lines,points)
        : Segment{"originalTop",source[0].b,source[3].b};
    require(std::abs(top.a.y()-top.b.y())<tolerance && length(top.b-top.a)>0.001,
            "Offset original top height is not one horizontal source level");
    QMap<QString,QPointF> values;for(int i=0;i<6;++i)values[pointIds[i].toString()]=derived[i];
    if(g.contains("topIntersectionPointIds")){
        const auto left=intersection(top,output[1]),right=intersection(top,output[3]);
        for(const auto& pair:std::initializer_list<std::pair<Segment,QPointF>>{{output[1],left},{output[3],right}}){
            require(fraction(pair.first,pair.second)>tolerance && fraction(pair.first,pair.second)<1-tolerance,
                    "Offset slant no longer crosses the original top height");
            require(fraction(top,pair.second)>-tolerance && fraction(top,pair.second)<1+tolerance,
                    "Offset intersection left the original top line");
        }
        require(length(right-left)>0.001,"Offset top intersections collapsed");
        const auto tops=g["topIntersectionPointIds"].toArray();values[tops[0].toString()]=left;values[tops[1].toString()]=right;
    }
    return setCoordinate(p,values);
}
QJsonObject add(const QJsonObject& draft,const QJsonObject& expanded,const QJsonObject& args){
    require(!draft["parameters"].toObject().contains("openChainOffsets"),"This sketch already has an open-chain offset");
    require(args["sourceEntityIds"].isArray() && args["sourceEntityIds"].toArray().size()==5 &&
            args["signedDistanceMm"].isDouble() && (!args.contains("topEntityId") || validId(args["topEntityId"])),
            "Offset needs five ordered source lines and signed distance");
    auto result=draft,p=draft["parameters"].toObject();auto points=p["points"].toArray(),entities=p["entities"].toArray();
    QJsonArray pointIds,entityIds,topIds;for(int i=0;i<6;++i){const auto id=fresh();pointIds.append(id);
        points.append(QJsonObject{{"id",id},{"uMm",0},{"vMm",0}});}
    for(int i=0;i<5;++i){const auto id=fresh();entityIds.append(id);entities.append(QJsonObject{{"id",id},{"type","line"},
        {"startPointId",pointIds[i]},{"endPointId",pointIds[i+1]}});}
    for(int i=0;i<2;++i){const auto id=fresh();topIds.append(id);points.append(QJsonObject{{"id",id},{"uMm",0},{"vMm",0}});}
    QJsonObject g{{"id",fresh()},{"sourceEntityIds",args["sourceEntityIds"]},
        {"signedDistanceMm",args["signedDistanceMm"]},
        {"generatedPointIds",pointIds},{"generatedEntityIds",entityIds},
        {"topIntersectionPointIds",topIds}};
    if(args.contains("topEntityId"))g["topEntityId"]=args["topEntityId"];
    if(args.contains("distanceParameterId")){g["distanceParameterId"]=args["distanceParameterId"];g["sideSign"]=args["sideSign"];}
    p["points"]=points;p["entities"]=entities;p["openChainOffsets"]=QJsonArray{g};
    auto expandedCandidate=expanded;auto e=expandedCandidate["entities"].toArray(),pt=expandedCandidate["points"].toArray();
    for(const auto& v:entities)if(entityIds.contains(v.toObject()["id"]))e.append(v);
    for(const auto& v:points)if(pointIds.contains(v.toObject()["id"])||topIds.contains(v.toObject()["id"]))pt.append(v);
    expandedCandidate["entities"]=e;expandedCandidate["points"]=pt;expandedCandidate["openChainOffsets"]=QJsonArray{g};
    QJsonObject derived;
    try{derived=derive(expandedCandidate);}catch(const std::invalid_argument&){
        // The opposite signed side can be a valid open Offset even when its
        // slants do not cross the original-height Line closure support.
        g.remove("topIntersectionPointIds");p["openChainOffsets"]=QJsonArray{g};
        for(int i=points.size()-1;i>=0;--i)if(topIds.contains(points[i].toObject()["id"]))points.removeAt(i);
        p["points"]=points;expandedCandidate["openChainOffsets"]=QJsonArray{g};
        for(int i=pt.size()-1;i>=0;--i)if(topIds.contains(pt[i].toObject()["id"]))pt.removeAt(i);
        expandedCandidate["points"]=pt;topIds={};derived=derive(expandedCandidate);
    }
    const auto derivedPoints=pointMap(derived);
    QMap<QString,QPointF> values;for(const auto& id:pointIds)values[id.toString()]=derivedPoints[id.toString()];
    for(const auto& id:topIds)values[id.toString()]=derivedPoints[id.toString()];
    result["parameters"]=setCoordinate(p,values);return result;
}
QJsonObject update(const QJsonObject& draft,const QJsonObject& expanded,const QJsonObject& args){
    auto result=draft,p=draft["parameters"].toObject();auto groups=p["openChainOffsets"].toArray();
    require(groups.size()==1 && groups[0].toObject()["id"]==args["offsetId"],"Open-chain offset ID is unavailable");
    auto g=groups[0].toObject();require(args["signedDistanceMm"].isDouble(),"Offset needs a signed distance");
    g["signedDistanceMm"]=args["signedDistanceMm"];
    if(args.contains("distanceParameterId")){g["distanceParameterId"]=args["distanceParameterId"];g["sideSign"]=args["sideSign"];}
    else {g.remove("distanceParameterId");g.remove("sideSign");}
    const auto oldTopIds=g.value("topIntersectionPointIds").toArray();
    QJsonArray topIds=oldTopIds;
    if(topIds.isEmpty()){for(int i=0;i<2;++i)topIds.append(fresh());
        auto points=p["points"].toArray();for(const auto& id:topIds)points.append(QJsonObject{{"id",id},{"uMm",0},{"vMm",0}});p["points"]=points;}
    g["topIntersectionPointIds"]=topIds;groups[0]=g;p["openChainOffsets"]=groups;
    auto expandedCandidate=expanded;expandedCandidate["openChainOffsets"]=groups;
    if(oldTopIds.isEmpty()){auto points=expandedCandidate["points"].toArray();
        for(const auto& id:topIds)points.append(QJsonObject{{"id",id},{"uMm",0},{"vMm",0}});
        expandedCandidate["points"]=points;}
    QJsonObject derived;
    try{derived=derive(expandedCandidate);}catch(const std::invalid_argument&){
        if(!oldTopIds.isEmpty())for(const auto& value:p["entities"].toArray()){
            const auto line=value.toObject();if(oldTopIds.contains(line["startPointId"]) || oldTopIds.contains(line["endPointId"]))
                throw std::invalid_argument("Offset closure depends on original-height intersections; remove it before flipping side");}
        g.remove("topIntersectionPointIds");groups[0]=g;p["openChainOffsets"]=groups;
        auto points=p["points"].toArray();for(int i=points.size()-1;i>=0;--i)
            if(topIds.contains(points[i].toObject()["id"]))points.removeAt(i);
        p["points"]=points;
        // Keep only resolved Project points in the expanded candidate.
        auto resolvedPoints=expanded["points"].toArray();for(int i=resolvedPoints.size()-1;i>=0;--i)
            if(topIds.contains(resolvedPoints[i].toObject()["id"]))resolvedPoints.removeAt(i);
        expandedCandidate["points"]=resolvedPoints;expandedCandidate["openChainOffsets"]=groups;
        derived=derive(expandedCandidate);
    }
    const auto derivedPoints=pointMap(derived);
    QMap<QString,QPointF> values;for(const auto& id:derivedPointIds(p))values[id]=derivedPoints[id];
    result["parameters"]=setCoordinate(p,values);return result;
}
}
