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
 const QSet<QString> allowed=update ? QSet<QString>{"polygonId","count","sizeMm","uMm","vMm","angleDeg","side"} : QSet<QString>{"mode","count","sizeMm","uMm","vMm","angleDeg","side"};
 for(auto it=a.begin();it!=a.end();++it)require(allowed.contains(it.key()),"Unknown Polygon argument");
 auto groups=p["polygonGroups"].toArray();QJsonObject group;int index=-1;
 if(update){for(int i=0;i<groups.size();++i)if(groups[i].toObject()["id"]==a["polygonId"]){group=groups[i].toObject();index=i;break;}require(index>=0,"Polygon identity is unavailable");}
 else group={{"id",uuid()},{"count",6},{"angleDeg",0},{"side",1}};
 for(auto it=a.begin();it!=a.end();++it)if(it.key()!="polygonId")group[it.key()]=it.value();
 const auto positions=vertices(group);const auto oldPoints=group["pointIds"].toArray(),oldEdges=group["entityIds"].toArray(),oldConstraints=group["constraintIds"].toArray();
 QJsonArray ids,edges,fixes;for(int i=0;i<positions.size();++i){ids.append(i<oldPoints.size()?oldPoints[i]:QJsonValue(uuid()));edges.append(i<oldEdges.size()?oldEdges[i]:QJsonValue(uuid()));fixes.append(i<oldConstraints.size()?oldConstraints[i]:QJsonValue(uuid()));}
 QSet<QString> removed;for(int i=positions.size();i<oldPoints.size();++i){removed.insert(oldPoints[i].toString());removed.insert(oldEdges[i].toString());}
 // Refuse a topology edit that strands any external authored reference.
 for(const auto& field:{"entities","constraints","dimensions","polygonGroups","mirrorGroups","slotGroups","arcSlotGroups","closedRectOffsets","openChainOffsets","projections"})for(const auto& value:p[field].toArray()){
   const auto object=value.toObject();if(oldEdges.contains(object["id"]) || oldConstraints.contains(object["id"]) || object["id"]==group["id"])continue;
   const auto bytes=QJsonDocument(object).toJson(QJsonDocument::Compact);for(const auto& id:removed)require(!bytes.contains(id.toUtf8()),"Polygon count edit would remove a referenced vertex or edge");
 }
 auto result=p;QJsonArray points,entities,constraints;
 for(const auto& value:p["points"].toArray())if(!oldPoints.contains(value.toObject()["id"]))points.append(value);
 for(const auto& value:p["entities"].toArray())if(!oldEdges.contains(value.toObject()["id"]))entities.append(value);
 for(const auto& value:p["constraints"].toArray())if(!oldConstraints.contains(value.toObject()["id"]))constraints.append(value);
 for(int i=0;i<positions.size();++i){auto point=positions[i].toObject();point["id"]=ids[i];points.append(point);
 entities.append(QJsonObject{{"id",edges[i]},{"type","line"},{"startPointId",ids[i]},{"endPointId",ids[(i+1)%ids.size()]}});
 constraints.append(QJsonObject{{"id",fixes[i]},{"type","fixed"},{"pointIds",QJsonArray{ids[i]}},{"uMm",point["uMm"]},{"vMm",point["vMm"]}});}
 group["pointIds"]=ids;group["entityIds"]=edges;group["constraintIds"]=fixes;if(update)groups[index]=group;else groups.append(group);
 result["points"]=points;result["entities"]=entities;result["constraints"]=constraints;result["polygonGroups"]=groups;return result;
}
QString validate(const QJsonObject& p){try{
 if(!p.contains("polygonGroups"))return {};
 require(p["polygonGroups"].isArray(),"Polygon groups must be an array");
 QMap<QString,QJsonObject> points,entities,constraints;for(const auto& v:p["points"].toArray())points[v.toObject()["id"].toString()]=v.toObject();for(const auto& v:p["entities"].toArray())entities[v.toObject()["id"].toString()]=v.toObject();for(const auto& v:p["constraints"].toArray())constraints[v.toObject()["id"].toString()]=v.toObject();
 QSet<QString> ownedIds;
 for(const auto& value:p["polygonGroups"].toArray()){
 require(value.isObject(),"Polygon group must be an object");const auto g=value.toObject();require(g.size()==11,"Invalid retained Polygon fields");
 const auto expected=vertices(g),ps=g["pointIds"].toArray(),es=g["entityIds"].toArray(),cs=g["constraintIds"].toArray();
 require(ps.size()==expected.size() && es.size()==ps.size() && cs.size()==ps.size(),"Polygon lost retained boundary identities");
 for(int i=0;i<ps.size();++i){const auto pid=ps[i].toString(),eid=es[i].toString(),cid=cs[i].toString();require(!ownedIds.contains(pid) && !ownedIds.contains(eid) && !ownedIds.contains(cid),"Polygon identities are repeated");ownedIds.unite({pid,eid,cid});
 const auto point=points.value(pid),edge=entities.value(eid),fixed=constraints.value(cid),position=expected[i].toObject();
 require(!point.isEmpty() && edge["type"]=="line" && !edge["construction"].toBool() && edge["startPointId"]==ps[i] && edge["endPointId"]==ps[(i+1)%ps.size()],"Polygon boundary changed outside Polygon controls");
 require(fixed["type"]=="fixed" && fixed["pointIds"]==QJsonArray{ps[i]},"Polygon lost retained vertex driver");
 for(const auto& coordinate:{"uMm","vMm"})require(std::abs(number(point,coordinate)-number(position,coordinate))<1e-7 && std::abs(number(fixed,coordinate)-number(position,coordinate))<1e-7,"Polygon vertex left its retained driver");
 }
 }
 return {};
 }catch(const std::exception& e){return QString::fromUtf8(e.what());}}
}
