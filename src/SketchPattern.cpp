// SPDX-License-Identifier: MIT
#include "SketchPattern.h"
#include <QJsonArray>
#include <QMap>
#include <QPointF>
#include <QUuid>
#include <cmath>
#include <stdexcept>
namespace {
void require(bool ok,const char* reason){if(!ok)throw std::invalid_argument(reason);}
QMap<QString,QJsonObject> rows(const QJsonObject& p,const char* field){QMap<QString,QJsonObject> out;for(const auto& v:p[field].toArray())out[v.toObject()["id"].toString()]=v.toObject();return out;}
QString identity(const QJsonObject& g,int i,int j,bool point){return QUuid::createUuidV5(QUuid(g["id"].toString()),QString("alloy-sketch-rectangular/1/%1/%2/%3/%4").arg(g["sourceEntityId"].toString()).arg(i).arg(j).arg(point ? "point" : "circle").toUtf8()).toString(QUuid::WithoutBraces);}
QPointF pos(const QJsonObject& p){return {p["uMm"].toDouble(),p["vMm"].toDouble()};}
int count(const QJsonObject& g,const char* key){const auto v=g[key];require(v.isDouble() && v.toDouble()==std::floor(v.toDouble()) && v.toDouble()>=2 && v.toDouble()<=16,"Pattern Quantity must be an integer from 2 to 16, including the source");return v.toInt();}
QPointF direction(const QJsonObject& g,const char* key,const QMap<QString,QJsonObject>& entities,const QMap<QString,QJsonObject>& points){
 const auto line=entities.value(g[key].toString());require(line["type"]=="line" && line["construction"].toBool(),"Pattern direction must be an authored construction line");
 const auto a=points.value(line["startPointId"].toString()),b=points.value(line["endPointId"].toString());require(!a.isEmpty() && !b.isEmpty(),"Pattern direction points are missing");
 const auto d=pos(b)-pos(a);const double n=std::hypot(d.x(),d.y());require(n>=.001,"Pattern direction is degenerate");return d/n;
}
QJsonArray instances(const QJsonObject& g){QJsonArray out;for(int i=0;i<count(g,"quantity1");++i)for(int j=0;j<count(g,"quantity2");++j)if(i || j)out.append(QJsonObject{{"i",i},{"j",j},{"entityId",identity(g,i,j,false)},{"pointId",identity(g,i,j,true)}});return out;}
void definition(const QJsonObject& p,const QJsonObject& g){
 require(g.size()==12 && !QUuid(g["id"].toString()).isNull() && g["type"]=="rectangular" && g["distribution"]=="spacing" && g["directionType"]=="oneDirection","Invalid rectangular Pattern definition");
 const auto es=rows(p,"entities"),ps=rows(p,"points");const auto source=es.value(g["sourceEntityId"].toString());
 require(source["type"]=="circle" && !source.contains("offsetSourceEntityId") && !source.contains("offsetMm"),"Pattern requires an ordinary authored circle");
 for(const auto& row:p["constraints"].toArray()){const auto r=row.toObject();if(r["type"]=="circleOffset" && r["entityIds"].toArray().size()==2)require(r["entityIds"].toArray()[1]!=g["sourceEntityId"],"Pattern seed cannot be a Circle Offset result");}
 for(const auto& row:p["mirrorGroups"].toArray())for(const auto& pair:row.toObject()["entityPairs"].toArray()){const auto id=pair.toObject().value("resultEntityId");require(id!=g["sourceEntityId"] && id!=g["direction1EntityId"] && id!=g["direction2EntityId"],"Pattern inputs cannot be Mirror results");}
 require(count(g,"quantity1")*count(g,"quantity2")<=64,"Pattern supports at most 64 total circles");
 for(const auto* key:{"spacing1Mm","spacing2Mm"})require(g[key].isDouble() && std::isfinite(g[key].toDouble()) && g[key].toDouble()>=.001 && g[key].toDouble()<=100000,"Pattern Spacing must be finite and positive in 0.001..100000 mm");
 const auto a=direction(g,"direction1EntityId",es,ps),b=direction(g,"direction2EntityId",es,ps);require(std::abs(QPointF::dotProduct(a,b))<1e-7,"Pattern requires perpendicular directions in this slice");
 require(g["instances"].isArray() && g["instances"].toArray()==instances(g),"Pattern indexed result identities changed");
}
}
namespace SketchPattern {
QSet<QString> resultIds(const QJsonObject& p,bool points){QSet<QString> out;for(const auto& row:p["patternGroups"].toArray())for(const auto& v:row.toObject()["instances"].toArray())out.insert(v.toObject()[points ? "pointId" : "entityId"].toString());return out;}
QJsonObject sourceOnly(const QJsonObject& p){auto out=p;const auto es=resultIds(p),ps=resultIds(p,true);for(const auto* field:{"entities","points"}){QJsonArray keep;for(const auto& v:p[field].toArray())if(!(QString(field)=="entities" ? es : ps).contains(v.toObject()["id"].toString()))keep.append(v);out[field]=keep;}return out;}
QString validate(const QJsonObject& p){try{
 if(!p.contains("patternGroups"))return {};
 require(p["patternGroups"].isArray() && p["patternGroups"].toArray().size()<=1,"This slice supports one rectangular sketch Pattern group");
 const auto es=rows(p,"entities"),ps=rows(p,"points");const auto owned=resultIds(p),ownedPoints=resultIds(p,true);
 for(const auto& row:p["patternGroups"].toArray()){
  const auto g=row.toObject();definition(p,g);const auto seed=es.value(g["sourceEntityId"].toString());
  require(!owned.contains(g["sourceEntityId"].toString()),"Pattern results cannot seed another retained owner");
  for(const auto& value:g["instances"].toArray()){
   const auto instance=value.toObject(),e=es.value(instance["entityId"].toString());
   require(e.size()==seed.size() && e["type"]=="circle" && e["centerPointId"]==instance["pointId"] && e["construction"]==seed["construction"] && e["centerline"]==seed["centerline"] && ps.contains(instance["pointId"].toString()),"Pattern owned result is missing or changed");
  }
 }
 for(const auto* field:{"constraints","dimensions"})for(const auto& v:p[field].toArray()){
  const auto r=v.toObject();require(!owned.contains(r["entityId"].toString()) && !ownedPoints.contains(r["pointId"].toString()),"Edit or constrain the Pattern source, not a derived result");
  for(const auto& id:r["entityIds"].toArray())require(!owned.contains(id.toString()),"Edit or constrain the Pattern source, not a derived result");
  for(const auto& id:r["pointIds"].toArray())require(!ownedPoints.contains(id.toString()),"Edit or constrain the Pattern source, not a derived result");
 }
 // No retained operation may silently acquire the generated rows as independent inputs.
 for(auto it=p.begin();it!=p.end();++it)if(it.key()!="patternGroups" && it.key()!="entities" && it.key()!="points" && it.value().isArray())
  for(const auto& v:it.value().toArray())if(v.isObject()){
   const auto object=v.toObject();for(auto f=object.begin();f!=object.end();++f){
    if(f.value().isString())require(!owned.contains(f.value().toString()) && !ownedPoints.contains(f.value().toString()),"Pattern results cannot be retained operation sources");
    if(f.value().isArray())for(const auto& id:f.value().toArray())if(id.isString())require(!owned.contains(id.toString()) && !ownedPoints.contains(id.toString()),"Pattern results cannot be retained operation sources");
   }
  }
 for(const auto& v:p["entities"].toArray()){const auto e=v.toObject();if(owned.contains(e["id"].toString()))continue;for(auto it=e.begin();it!=e.end();++it)if(it.value().isString())require(!owned.contains(it.value().toString()) && !ownedPoints.contains(it.value().toString()),"Pattern result cannot be a dependent curve source");}
 return {};
 }catch(const std::exception& error){return QString::fromUtf8(error.what());}}
QJsonObject derive(const QJsonObject& p){
 auto out=sourceOnly(p);auto entities=out["entities"].toArray(),points=out["points"].toArray();const auto es=rows(out,"entities"),ps=rows(out,"points");
 for(const auto& row:p["patternGroups"].toArray()){
  const auto g=row.toObject();definition(p,g);const auto seed=es.value(g["sourceEntityId"].toString()),center=ps.value(seed["centerPointId"].toString());
  require(!center.isEmpty(),"Pattern source center is missing");const auto a=direction(g,"direction1EntityId",es,ps)*g["spacing1Mm"].toDouble(),b=direction(g,"direction2EntityId",es,ps)*g["spacing2Mm"].toDouble();
  QJsonArray centers{QJsonObject{{"uMm",pos(center).x()},{"vMm",pos(center).y()}}};
  for(const auto& value:g["instances"].toArray()){
   const auto instance=value.toObject();auto e=seed,c=center;const auto xy=pos(center)+a*instance["i"].toInt()+b*instance["j"].toInt();require(std::isfinite(xy.x()) && std::isfinite(xy.y()) && std::abs(xy.x())<=100000 && std::abs(xy.y())<=100000,"Pattern output exceeds coordinate bounds");
   for(const auto& prior:centers){const auto d=xy-pos(prior.toObject());require(std::hypot(d.x(),d.y())>2*seed["radiusMm"].toDouble()+1e-6,"Pattern circles overlap or touch");}
   c["id"]=instance["pointId"];c["uMm"]=xy.x();c["vMm"]=xy.y();e["id"]=instance["entityId"];e["centerPointId"]=c["id"];points.append(c);entities.append(e);centers.append(c);
  }
 }
 out["points"]=points;out["entities"]=entities;return out;
}
QJsonObject create(const QJsonObject& p,const QJsonObject& arguments){
 const QSet<QString> allowed{"groupId","sourceEntityId","direction1EntityId","direction2EntityId","quantity1","quantity2","spacing1Mm","spacing2Mm"};for(auto it=arguments.begin();it!=arguments.end();++it)require(allowed.contains(it.key()),"Unknown Pattern input");
 auto out=p;auto groups=p["patternGroups"].toArray();QJsonObject g;const auto edit=arguments["groupId"].toString();
 if(!edit.isEmpty()){require(groups.size()==1 && groups.first().toObject()["id"]==edit,"Pattern group is missing");g=groups.first().toObject();out=sourceOnly(p);}else{require(groups.isEmpty(),"This slice supports one rectangular sketch Pattern group");g={{"id",QUuid::createUuid().toString(QUuid::WithoutBraces)}};}
 for(auto it=arguments.begin();it!=arguments.end();++it)if(it.key()!="groupId")g[it.key()]=it.value();
 g["type"]="rectangular";g["distribution"]="spacing";g["directionType"]="oneDirection";g["instances"]=instances(g);
 out["patternGroups"]=QJsonArray{g};out=derive(out);const auto error=validate(out);if(!error.isEmpty())throw std::invalid_argument(error.toStdString());return out;
}
}
