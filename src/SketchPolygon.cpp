#include "SketchPolygon.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QMap>
#include <QSet>
#include <QUuid>
#include <cmath>
#include <numbers>
#include <stdexcept>
namespace {
void require(bool good,const char* message){if(!good)throw std::runtime_error(message);}
QString uuid(){return QUuid::createUuid().toString(QUuid::WithoutBraces);}
double number(const QJsonObject& o,const QString& key){const auto v=o.value(key);require(v.isDouble() && std::isfinite(v.toDouble()),"Polygon requires finite numeric inputs");return v.toDouble();}
QJsonArray vertices(const QJsonObject& group){
 const auto mode=group["mode"].toString();require(mode=="inscribed" || mode=="circumscribed" || mode=="edge","Unknown Polygon mode");
 const double raw=number(group,"count");require(raw>=3 && raw<=128,"Polygon side count must be an integer from 3 to 128");const int count=int(raw);require(raw==count,"Polygon side count must be an integer from 3 to 128");
 const double size=number(group,"sizeMm"),angle=number(group,"angleDeg")*std::numbers::pi/180;
 require(size>=0.001 && size<=100000,"Polygon size must be positive and within range");
 double u=number(group,"uMm"),v=number(group,"vMm"),r=size,start=angle;
 const double half=std::numbers::pi/count;const double side=number(group,"side");require(side==1 || side==-1,"Polygon side must be +1 or -1");
 if(mode=="circumscribed"){r=size/std::cos(half);start-=half;}
 if(mode=="edge"){
   r=size/(2*std::sin(half));const double apothem=size/(2*std::tan(half));
   u+=size*std::cos(angle)/2-side*apothem*std::sin(angle);
   v+=size*std::sin(angle)/2+side*apothem*std::cos(angle);
   start=angle-side*(std::numbers::pi/2+half);
 }
 QJsonArray result;for(int i=0;i<count;++i){const double a=start+(mode=="edge" ? side : 1)*2*std::numbers::pi*i/count,x=u+r*std::cos(a),y=v+r*std::sin(a);
 require(std::abs(x)<=100000 && std::abs(y)<=100000,"Polygon vertex outside supported range");result.append(QJsonObject{{"uMm",x},{"vMm",y}});}return result;
}
}
namespace SketchPolygon {
QJsonObject apply(const QJsonObject& p,const QJsonObject& a,bool update){
 const QSet<QString> allowed=update ? QSet<QString>{"polygonId","count","sizeMm","uMm","vMm","angleDeg","side"} : QSet<QString>{"mode","count","sizeMm","uMm","vMm","angleDeg","side","anchorSource"};
 for(auto it=a.begin();it!=a.end();++it)require(allowed.contains(it.key()),"Unknown Polygon argument");
 auto groups=p["polygonGroups"].toArray();QJsonObject group;int index=-1;
 if(update){for(int i=0;i<groups.size();++i)if(groups[i].toObject()["id"]==a["polygonId"]){group=groups[i].toObject();index=i;break;}require(index>=0,"Polygon identity is unavailable");}
 else group={{"id",uuid()},{"count",6},{"angleDeg",0},{"side",1}};
 const bool retained=group.contains("anchorPointId");
 if(retained && group["anchorSource"].toObject()["kind"]!="fixed")require(!a.contains("uMm") && !a.contains("vMm"),"Polygon anchor follows its retained source; edit that source instead");
 if(retained)for(const auto& row:p["points"].toArray())if(row.toObject()["id"]==group["anchorPointId"]){group["uMm"]=row.toObject()["uMm"];group["vMm"]=row.toObject()["vMm"];break;}
 for(auto it=a.begin();it!=a.end();++it)if(it.key()!="polygonId")group[it.key()]=it.value();
 // Stable V5 identities also make legacy migration repeatable and undo-safe.
 const auto groupId=QUuid(group["id"].toString());
 if(!retained){
  group["anchorPointId"]=QUuid::createUuidV5(groupId,"alloy-polygon-anchor/1").toString(QUuid::WithoutBraces);
  group["anchorConstraintId"]=QUuid::createUuidV5(groupId,"alloy-polygon-anchor-relation/1").toString(QUuid::WithoutBraces);
  if(!group.contains("anchorSource"))group["anchorSource"]=QJsonObject{{"kind","fixed"}};
 }
 const auto positions=vertices(group);const auto oldPoints=group["pointIds"].toArray(),oldEdges=group["entityIds"].toArray(),oldConstraints=group["constraintIds"].toArray();
 QJsonArray ids,edges,drivers;for(int i=0;i<positions.size();++i){ids.append(i<oldPoints.size()?oldPoints[i]:QJsonValue(uuid()));edges.append(i<oldEdges.size()?oldEdges[i]:QJsonValue(uuid()));drivers.append(i<oldConstraints.size()?oldConstraints[i]:QJsonValue(uuid()));}
 QSet<QString> removed;for(int i=positions.size();i<oldPoints.size();++i){removed.insert(oldPoints[i].toString());removed.insert(oldEdges[i].toString());}
 for(const auto& field:{"entities","constraints","dimensions","polygonGroups","mirrorGroups","slotGroups","arcSlotGroups","closedRectOffsets","openChainOffsets","projections"})for(const auto& value:p[field].toArray()){
  const auto object=value.toObject();if(oldEdges.contains(object["id"]) || oldConstraints.contains(object["id"]) || object["id"]==group["id"])continue;
  const auto bytes=QJsonDocument(object).toJson(QJsonDocument::Compact);for(const auto& id:removed)require(!bytes.contains(id.toUtf8()),"Polygon count edit would remove a referenced vertex or edge");
 }
 auto result=p;QJsonArray points,entities,constraints;
 for(const auto& value:p["points"].toArray())if(!oldPoints.contains(value.toObject()["id"]) && value.toObject()["id"]!=group["anchorPointId"])points.append(value);
 for(const auto& value:p["entities"].toArray())if(!oldEdges.contains(value.toObject()["id"]))entities.append(value);
 for(const auto& value:p["constraints"].toArray())if(!oldConstraints.contains(value.toObject()["id"]) && value.toObject()["id"]!=group["anchorConstraintId"])constraints.append(value);
 const auto anchor=group["anchorPointId"].toString();points.append(QJsonObject{{"id",anchor},{"uMm",group["uMm"]},{"vMm",group["vMm"]}});
 const auto source=group["anchorSource"].toObject();const auto kind=source["kind"].toString();
 QJsonObject relation{{"id",group["anchorConstraintId"]},{"type",kind=="fixed" ? "fixed" : kind=="midpoint" ? "midpoint" : "coincident"}};
 if(kind=="fixed"){relation["pointIds"]=QJsonArray{anchor};relation["uMm"]=group["uMm"];relation["vMm"]=group["vMm"];}
 else if(kind=="midpoint"){relation["pointIds"]=QJsonArray{anchor};relation["entityIds"]=QJsonArray{source["entityId"]};}
 else {require(kind=="origin" || kind=="point","Unsupported Polygon anchor source");relation["pointIds"]=QJsonArray{anchor,source["pointId"]};}
 constraints.append(relation);
 for(int i=0;i<positions.size();++i){auto point=positions[i].toObject();point["id"]=ids[i];points.append(point);
  entities.append(QJsonObject{{"id",edges[i]},{"type","line"},{"startPointId",ids[i]},{"endPointId",ids[(i+1)%ids.size()]}});
  constraints.append(QJsonObject{{"id",drivers[i]},{"type","polygonVertex"},{"pointIds",QJsonArray{anchor,ids[i]}},
   {"offsetUMm",point["uMm"].toDouble()-group["uMm"].toDouble()},{"offsetVMm",point["vMm"].toDouble()-group["vMm"].toDouble()}});
 }
 group["pointIds"]=ids;group["entityIds"]=edges;group["constraintIds"]=drivers;if(update)groups[index]=group;else groups.append(group);
 result["points"]=points;result["entities"]=entities;result["constraints"]=constraints;result["polygonGroups"]=groups;return result;
}
QJsonObject refreshAnchors(const QJsonObject& p){
 if(!p.contains("polygonGroups"))return p;auto result=p;auto groups=p["polygonGroups"].toArray();QMap<QString,QJsonObject> points;
 for(const auto& row:p["points"].toArray())points[row.toObject()["id"].toString()]=row.toObject();
 for(int i=0;i<groups.size();++i){auto group=groups[i].toObject();if(!group.contains("anchorPointId"))continue;
  const auto anchor=points.value(group["anchorPointId"].toString());require(!anchor.isEmpty(),"Polygon lost its anchor point");group["uMm"]=anchor["uMm"];group["vMm"]=anchor["vMm"];groups[i]=group;}
 result["polygonGroups"]=groups;return result;
}
QJsonObject seed(const QJsonObject& p){
 if(!p.contains("polygonGroups"))return p;auto result=p;auto points=p["points"].toArray();QMap<QString,int> pointIndex;QMap<QString,QJsonObject> entities;
 for(int i=0;i<points.size();++i)pointIndex[points[i].toObject()["id"].toString()]=i;
 for(const auto& row:p["entities"].toArray())entities[row.toObject()["id"].toString()]=row.toObject();
 auto at=[&](const QString& id){require(pointIndex.contains(id),"Polygon anchor source point is unavailable");return points[pointIndex.value(id)].toObject();};
 for(const auto& row:p["polygonGroups"].toArray()){
  auto group=row.toObject();if(!group.contains("anchorPointId"))continue;const auto source=group["anchorSource"].toObject();const auto kind=source["kind"].toString();
  double u=number(group,"uMm"),v=number(group,"vMm");
  if(kind=="point" || kind=="origin"){const auto target=at(source["pointId"].toString());u=number(target,"uMm");v=number(target,"vMm");}
  else if(kind=="midpoint"){const auto line=entities.value(source["entityId"].toString());require(line["type"]=="line","Polygon midpoint source is unavailable");const auto a=at(line["startPointId"].toString()),b=at(line["endPointId"].toString());u=(number(a,"uMm")+number(b,"uMm"))/2;v=(number(a,"vMm")+number(b,"vMm"))/2;}
  group["uMm"]=u;group["vMm"]=v;const auto positions=vertices(group);const auto ids=group["pointIds"].toArray();
  auto anchor=at(group["anchorPointId"].toString());anchor["uMm"]=u;anchor["vMm"]=v;points[pointIndex.value(group["anchorPointId"].toString())]=anchor;
  for(int i=0;i<ids.size();++i){auto point=at(ids[i].toString());point["uMm"]=positions[i].toObject()["uMm"];point["vMm"]=positions[i].toObject()["vMm"];points[pointIndex.value(ids[i].toString())]=point;}
 }
 result["points"]=points;return refreshAnchors(result);
}
QString validate(const QJsonObject& p){try{
 if(!p.contains("polygonGroups"))return {};require(p["polygonGroups"].isArray(),"Polygon groups must be an array");
 QMap<QString,QJsonObject> points,entities,constraints;for(const auto& v:p["points"].toArray())points[v.toObject()["id"].toString()]=v.toObject();for(const auto& v:p["entities"].toArray())entities[v.toObject()["id"].toString()]=v.toObject();for(const auto& v:p["constraints"].toArray())constraints[v.toObject()["id"].toString()]=v.toObject();
 QSet<QString> ownedIds;
 for(const auto& value:p["polygonGroups"].toArray()){
  require(value.isObject(),"Polygon group must be an object");auto g=value.toObject();const bool retained=g.contains("anchorPointId");require(g.size()==(retained ? 14 : 11),"Invalid retained Polygon fields");
  const auto ps=g["pointIds"].toArray(),es=g["entityIds"].toArray(),cs=g["constraintIds"].toArray();
  if(retained){
   const auto anchor=g["anchorPointId"].toString();require(points.contains(anchor) && !ps.contains(anchor),"Polygon lost its distinct anchor point");
   require(!ownedIds.contains(anchor),"Polygon anchor belongs to another group");ownedIds.insert(anchor);
   const auto source=g["anchorSource"].toObject(),relation=constraints.value(g["anchorConstraintId"].toString());const auto kind=source["kind"].toString();
   require(!relation.isEmpty(),"Polygon lost its retained anchor relation");
   if(kind=="fixed")require(source.size()==1 && relation["type"]=="fixed" && relation["pointIds"]==QJsonArray{anchor},"Invalid fixed Polygon anchor");
   else if(kind=="point" || kind=="origin"){
    const auto target=source["pointId"].toString();require(source.size()==2 && target!=anchor && !ps.contains(target) && points.contains(target),"Polygon anchor source is missing or self-referential");
    if(kind=="origin")require(source["pointId"]==p["originPointId"],"Polygon origin identity changed");
    require(relation["type"]=="coincident" && relation["pointIds"]==QJsonArray{anchor,target},"Polygon lost its anchor Coincident relation");
   }else if(kind=="midpoint"){
    const auto id=source["entityId"].toString();require(source.size()==2 && !es.contains(id) && entities.value(id)["type"]=="line","Polygon midpoint source is missing or self-referential");
    const auto line=entities.value(id);require(line["startPointId"]!=anchor && line["endPointId"]!=anchor && !ps.contains(line["startPointId"]) && !ps.contains(line["endPointId"]),"Polygon midpoint source is self-referential");
    require(relation["type"]=="midpoint" && relation["pointIds"]==QJsonArray{anchor} && relation["entityIds"]==QJsonArray{id},"Polygon lost its anchor Midpoint relation");
   }else require(false,"Unsupported Polygon anchor source");
   g["uMm"]=points.value(anchor)["uMm"];g["vMm"]=points.value(anchor)["vMm"];
  }
  const auto expected=vertices(g);require(ps.size()==expected.size() && es.size()==ps.size() && cs.size()==ps.size(),"Polygon lost retained boundary identities");
  for(int i=0;i<ps.size();++i){const auto pid=ps[i].toString(),eid=es[i].toString(),cid=cs[i].toString();require(!ownedIds.contains(pid) && !ownedIds.contains(eid) && !ownedIds.contains(cid),"Polygon identities are repeated");ownedIds.unite({pid,eid,cid});
   const auto point=points.value(pid),edge=entities.value(eid),driver=constraints.value(cid),position=expected[i].toObject();
   require(!point.isEmpty() && edge["type"]=="line" && !edge["construction"].toBool() && edge["startPointId"]==ps[i] && edge["endPointId"]==ps[(i+1)%ps.size()],"Polygon boundary changed outside Polygon controls");
   if(retained){
    require(driver.size()==5 && driver["type"]=="polygonVertex" && driver["pointIds"]==QJsonArray{g["anchorPointId"],ps[i]},"Polygon lost retained relative vertex driver");
    require(std::abs(number(driver,"offsetUMm")-(number(position,"uMm")-number(g,"uMm")))<1e-7 && std::abs(number(driver,"offsetVMm")-(number(position,"vMm")-number(g,"vMm")))<1e-7,"Polygon relative driver changed outside Polygon controls");
   }else require(driver["type"]=="fixed" && driver["pointIds"]==QJsonArray{ps[i]},"Legacy Polygon lost retained vertex driver");
   for(const auto& coordinate:{"uMm","vMm"})require(std::abs(number(point,coordinate)-number(position,coordinate))<1e-6 && (retained || std::abs(number(driver,coordinate)-number(position,coordinate))<1e-7),"Polygon vertex left its retained driver");
  }
 }
 return {};
 }catch(const std::exception& e){return QString::fromUtf8(e.what());}}
}
