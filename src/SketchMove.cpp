// SPDX-License-Identifier: MIT
#include "SketchMove.h"
#include <QJsonArray>
#include <QMap>
#include <QUuid>
#include <cmath>
#include <stdexcept>
namespace {
void require(bool value,const char* message){if(!value)throw std::invalid_argument(message);}
QSet<QString> ids(const QJsonArray& rows){QSet<QString> result;for(const auto& value:rows){require(value.isString() && !value.toString().isEmpty() && !result.contains(value.toString()),"Move membership must contain distinct IDs");result.insert(value.toString());}return result;}
QMap<QString,QJsonObject> rows(const QJsonObject& p,const char* key){QMap<QString,QJsonObject> result;for(const auto& value:p[key].toArray())result[value.toObject()["id"].toString()]=value.toObject();return result;}
bool mentions(const QJsonValue& value,const QSet<QString>& targets){if(value.isString())return targets.contains(value.toString());if(value.isArray())for(const auto& v:value.toArray())if(mentions(v,targets))return true;if(value.isObject())for(const auto& v:value.toObject())if(mentions(v,targets))return true;return false;}
void finite(const QJsonObject& row,const char* key){require(row[key].isDouble() && std::isfinite(row[key].toDouble()) && std::abs(row[key].toDouble())<=100000,"Move coordinates/displacements must be finite within ±100000 mm");}
}
namespace SketchMove {
QSet<QString> owned(const QJsonObject& p,bool points){QSet<QString> result;for(const auto& row:p["moveGroups"].toArray()){const auto g=row.toObject();if(points)for(const auto& v:g["sourcePoints"].toArray())result.insert(v.toObject()["pointId"].toString());else for(const auto& v:g["entityIds"].toArray())result.insert(v.toString());}return result;}
bool touches(const QJsonObject& p,const QJsonValue& value){return mentions(value,owned(p)+owned(p,true));}
QSet<QString> anchors(const QJsonObject& p){QSet<QString> result;for(const auto& row:p["moveGroups"].toArray())result.insert(row.toObject()["anchorPointId"].toString());return result;}
QString validate(const QJsonObject& p){try{
 if(!p.contains("moveGroups"))return {};
 require(p["moveGroups"].isArray() && p["moveGroups"].toArray().size()<=1,"Only one retained Move is supported");
 if(!p["moveGroups"].toArray().isEmpty())require(!p.contains("texts") && !p.contains("loopOffsets"),"Retained Move requires a solver-backed sketch without text or legacy loop offsets");
 const auto es=rows(p,"entities"),ps=rows(p,"points");
 for(const auto& row:p["moveGroups"].toArray()){
  const auto g=row.toObject();require(g.size()==10 && !QUuid(g["id"].toString()).isNull() && g["type"]=="translate" && p["plane"]=="XY","Invalid retained Move definition");finite(g,"deltaUMm");finite(g,"deltaVMm");
  require(g["entityIds"].isArray() && g["sourcePoints"].isArray() && g["topology"].isArray() && g["constraintIds"].isArray() && g["dimensionIds"].isArray(),"Invalid Move membership arrays");
  const auto selected=ids(g["entityIds"].toArray());require(selected.size()==4 && g["sourcePoints"].toArray().size()==4 && g["topology"].toArray().size()==4,"Move owns exactly four Lines and four source vertices");
  QSet<QString> vertices,topology;QMap<QString,QStringList> neighbors;
  for(const auto& v:g["sourcePoints"].toArray()){const auto point=v.toObject();require(point.size()==3 && point["pointId"].isString() && ps.contains(point["pointId"].toString()) && !vertices.contains(point["pointId"].toString()),"Invalid Move source point identity");finite(point,"uMm");finite(point,"vMm");const auto out=ps.value(point["pointId"].toString());require(std::abs(out["uMm"].toDouble()-point["uMm"].toDouble()-g["deltaUMm"].toDouble())<=1e-9 && std::abs(out["vMm"].toDouble()-point["vMm"].toDouble()-g["deltaVMm"].toDouble())<=1e-9,"Move output cache changed; edit the retained Move or source size dimension");vertices.insert(point["pointId"].toString());}
  require(vertices.contains(g["anchorPointId"].toString()) && g["anchorPointId"]!=p["originPointId"],"Move source anchor is unavailable");
  for(const auto& v:g["topology"].toArray()){const auto t=v.toObject(),e=es.value(t["entityId"].toString());const auto key=t["entityId"].toString(),a=t["startPointId"].toString(),b=t["endPointId"].toString();require(t.size()==3 && selected.contains(key) && !topology.contains(key) && e["type"]=="line" && !e["construction"].toBool() && !e["centerline"].toBool() && e["startPointId"]==t["startPointId"] && e["endPointId"]==t["endPointId"] && vertices.contains(a) && vertices.contains(b) && a!=b,"Move topology changed; edit its source size or retained displacement");topology.insert(key);neighbors[a].append(b);neighbors[b].append(a);}
  QSet<QString> connected;QString cursor=*vertices.begin(),previous;for(int n=0;n<4;++n){require(neighbors[cursor].size()==2 && !connected.contains(cursor),"Move requires one closed four-Line topology");connected.insert(cursor);const auto next=neighbors[cursor][0]==previous ? neighbors[cursor][1] : neighbors[cursor][0];previous=cursor;cursor=next;}require(cursor==*vertices.begin() && connected==vertices,"Move topology is disconnected");
  const auto targets=selected+vertices;
  for(const auto& e:es)if(!selected.contains(e["id"].toString()))require(!mentions(e,vertices),"Move shares points with unselected geometry");
  for(auto it=p.begin();it!=p.end();++it)if(it.key()!="moveGroups" && it.key()!="points" && it.key()!="entities" && it.key()!="constraints" && it.key()!="dimensions")require(!mentions(it.value(),targets),"Move geometry cannot acquire another retained owner or external reference");
  QJsonArray constraints,dimensions;
  for(const auto& value:p["constraints"].toArray()){const auto c=value.toObject();if(!mentions(c,targets))continue;require(QStringList{"horizontal","vertical","parallel","perpendicular","equal"}.contains(c["type"].toString()) && !c.contains("pointIds") && c["entityIds"].isArray(),"Retained Move supports internal translation-invariant line constraints only");for(const auto& id:c["entityIds"].toArray())require(selected.contains(id.toString()),"Move constraints cannot cross to unselected geometry");constraints.append(c["id"]);}
  for(const auto& value:p["dimensions"].toArray()){const auto d=value.toObject();if(!mentions(d,targets))continue;require(d["driving"].toBool() && QStringList{"horizontal","vertical","linear"}.contains(d["kind"].toString()),"Move permits existing internal driving side-size dimensions only");bool side=selected.contains(d["entityId"].toString());if(d["pointIds"].isArray()){const auto pair=ids(d["pointIds"].toArray());for(const auto& key:selected){const auto e=es.value(key);side |= pair==QSet<QString>{e["startPointId"].toString(),e["endPointId"].toString()};}}require(side,"Move size dimensions must reference one owned side");for(const auto& id:d["pointIds"].toArray())require(vertices.contains(id.toString()),"Move dimensions cannot reference unselected points");dimensions.append(d["id"]);}
  require(ids(constraints)==ids(g["constraintIds"].toArray()) && ids(dimensions)==ids(g["dimensionIds"].toArray()),"Move constraint/dimension membership changed; only existing source size values are editable");
 }
 return {};
 }catch(const std::exception& e){return QString::fromUtf8(e.what());}}
QJsonObject source(const QJsonObject& p){auto result=p;auto points=p["points"].toArray();for(const auto& row:p["moveGroups"].toArray())for(const auto& value:row.toObject()["sourcePoints"].toArray()){const auto q=value.toObject();for(int n=0;n<points.size();++n)if(points[n].toObject()["id"]==q["pointId"]){auto point=points[n].toObject();point["uMm"]=q["uMm"];point["vMm"]=q["vMm"];points[n]=point;}}
 result["points"]=points;result.remove("moveGroups");return result;}
QJsonObject materialize(const QJsonObject& p,const QJsonArray& groups,bool refreshSource){auto result=p;auto points=p["points"].toArray();QJsonArray updated;
 for(const auto& row:groups){auto g=row.toObject();auto sources=g["sourcePoints"].toArray();for(int k=0;k<sources.size();++k){auto src=sources[k].toObject();for(int n=0;n<points.size();++n)if(points[n].toObject()["id"]==src["pointId"]){auto point=points[n].toObject();if(refreshSource){src["uMm"]=point["uMm"];src["vMm"]=point["vMm"];}point["uMm"]=src["uMm"].toDouble()+g["deltaUMm"].toDouble();point["vMm"]=src["vMm"].toDouble()+g["deltaVMm"].toDouble();require(std::isfinite(point["uMm"].toDouble()) && std::isfinite(point["vMm"].toDouble()) && std::abs(point["uMm"].toDouble())<=100000 && std::abs(point["vMm"].toDouble())<=100000,"Move output exceeds coordinate bounds");points[n]=point;}sources[k]=src;}g["sourcePoints"]=sources;updated.append(g);}
 result["points"]=points;if(!groups.isEmpty())result["moveGroups"]=updated;return result;}
QJsonObject create(const QJsonObject& p,const QJsonArray& entityIds,double du,double dv,const QString& groupId){auto groups=p["moveGroups"].toArray();QJsonObject g;
 if(groupId.isEmpty()){
  require(groups.isEmpty(),"This sketch already has a retained Move; use Edit Move");const auto selected=ids(entityIds);const auto es=rows(p,"entities");QSet<QString> vertices;QJsonArray topology,sources,cs,ds;
  for(const auto& key:entityIds){const auto e=es.value(key.toString());require(e["type"]=="line","Move requires authored Lines");vertices.insert(e["startPointId"].toString());vertices.insert(e["endPointId"].toString());topology.append(QJsonObject{{"entityId",key},{"startPointId",e["startPointId"]},{"endPointId",e["endPointId"]}});}
  const auto targets=selected+vertices;for(const auto& value:p["points"].toArray()){const auto point=value.toObject();if(vertices.contains(point["id"].toString()))sources.append(QJsonObject{{"pointId",point["id"]},{"uMm",point["uMm"]},{"vMm",point["vMm"]}});}
  for(const auto& v:p["constraints"].toArray())if(mentions(v,targets))cs.append(v.toObject()["id"]);for(const auto& v:p["dimensions"].toArray())if(mentions(v,targets))ds.append(v.toObject()["id"]);
  g={{"id",QUuid::createUuid().toString(QUuid::WithoutBraces)},{"type","translate"},{"entityIds",entityIds},{"sourcePoints",sources},{"anchorPointId",es.value(entityIds.first().toString())["startPointId"]},{"constraintIds",cs},{"dimensionIds",ds},{"topology",topology}};
 }else{require(groups.size()==1 && groups.first().toObject()["id"]==groupId,"Retained Move group is unavailable");g=groups.first().toObject();require(entityIds.isEmpty() || ids(entityIds)==ids(g["entityIds"].toArray()),"Edit Move must retain its original four Lines");}
 g["deltaUMm"]=du;g["deltaVMm"]=dv;auto result=materialize(source(p),QJsonArray{g},false);const auto error=validate(result);require(error.isEmpty(),qPrintable(error));return result;
}
}
