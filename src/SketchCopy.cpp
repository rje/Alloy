// SPDX-License-Identifier: MIT
#include "SketchGeometry.h"
#include <BRepExtrema_DistShapeShape.hxx>
#include <QJsonArray>
#include <QMap>
#include <QUuid>
#include <cmath>
#include <stdexcept>
namespace {
void need(bool ok,const QString& message){if(!ok)throw std::invalid_argument(message.toStdString());}
bool mentions(const QJsonValue& value,const QSet<QString>& targets){if(value.isString())return targets.contains(value.toString());if(value.isArray())for(const auto& row:value.toArray())if(mentions(row,targets))return true;if(value.isObject())for(const auto& row:value.toObject())if(mentions(row,targets))return true;return false;}
QJsonValue remap(const QJsonValue& value,const QMap<QString,QString>& map){if(value.isString())return map.contains(value.toString()) ? QJsonValue(map.value(value.toString())) : value;if(value.isArray()){QJsonArray a;for(const auto& row:value.toArray())a.append(remap(row,map));return a;}if(value.isObject()){auto o=value.toObject();for(auto it=o.begin();it!=o.end();++it)it.value()=remap(it.value(),map);return o;}return value;}
}
SketchGeometry::SolveResult SketchGeometry::copyProfileSelection(const QJsonObject& draft,const QJsonArray& entityIds,const QJsonObject& context){
 try{
  need(draft["parameters"].toObject()["plane"]=="XY","Copy/Paste supports an active XY sketch in this slice");
  // Reuse the rigid isolated-loop guard, including retained/external ownership.
  const auto qualified=translateProfile(draft,{{"entityIds",entityIds},{"deltaUMm",1},{"deltaVMm",1}},context);need(qualified.ok,qualified.error);
  const auto p=draft["parameters"].toObject();QSet<QString> entities,points;for(const auto& id:entityIds)entities.insert(id.toString());
  for(const auto& row:p["entities"].toArray()){const auto e=row.toObject();if(entities.contains(e["id"].toString())){points.insert(e["startPointId"].toString());points.insert(e["endPointId"].toString());}}
  const auto targets=entities+points;QJsonObject selected{{"plane","XY"}};if(p.contains("nextDimensionNameIndex"))selected["nextDimensionNameIndex"]=p["nextDimensionNameIndex"];
  for(const auto& key:{"points","entities","constraints","dimensions"}){QJsonArray rows;for(const auto& row:p[key].toArray()){
   const auto o=row.toObject();const bool included=QString(key)=="points" ? points.contains(o["id"].toString()) : QString(key)=="entities" ? entities.contains(o["id"].toString()) : mentions(o,targets);if(!included)continue;
   if(QString(key)=="constraints"){
    need(QStringList{"horizontal","vertical","parallel","perpendicular","equal"}.contains(o["type"].toString()) && o["entityIds"].isArray() && !o.contains("pointIds"),"Copy supports internal translation-invariant Line constraints only");for(const auto& id:o["entityIds"].toArray())need(entities.contains(id.toString()),"Copy cannot remap a relation to unselected geometry");
   }
   if(QString(key)=="dimensions"){
    need(o["driving"].toBool() && QStringList{"horizontal","vertical","linear"}.contains(o["kind"].toString()) && !o.contains("expression") && !o.contains("parameterId"),"Copy supports independent driving side sizes; external bindings/expressions refuse");
    need(o["pointIds"].toArray().size()==2,"Copy side dimensions require two internal vertices");for(const auto& id:o["pointIds"].toArray())need(points.contains(id.toString()),"Copy dimensions cannot reference unselected geometry");
   }
   rows.append(o);
  }selected[key]=rows;}
  return {true,draft,{{"clipboard",QJsonObject{{"sketchId",draft["id"]},{"parameters",selected}}}}, {}};
 }catch(const std::exception& e){return {false,draft,{},QString::fromUtf8(e.what())};}
}
SketchGeometry::SolveResult SketchGeometry::pasteProfile(const QJsonObject& draft,const QJsonObject& clipboard,const QJsonObject& args,const QJsonObject& context){
 try{
  need(draft["id"]==clipboard["sketchId"] && draft["parameters"].toObject()["plane"]=="XY","Paste supports the same active XY sketch as Copy");
  const auto token=QUuid(args["copyId"].toString());need(!token.isNull(),"Paste requires a stable copyId UUID");
  const double du=args["deltaUMm"].toDouble(),dv=args["deltaVMm"].toDouble();need(args["deltaUMm"].isDouble() && args["deltaVMm"].isDouble() && std::isfinite(du) && std::isfinite(dv) && std::abs(du)<=100000 && std::abs(dv)<=100000,"Paste displacement must be finite within ±100000 mm");
  auto source=draft;source["parameters"]=clipboard["parameters"];QJsonArray sourceIds;for(const auto& row:clipboard["parameters"].toObject()["entities"].toArray())sourceIds.append(row.toObject()["id"]);
  const auto captured=copyProfileSelection(source,sourceIds,context);need(captured.ok,captured.error);
  const auto sourceParameters=clipboard["parameters"].toObject();QMap<QString,QString> map;QJsonObject mapping;for(const auto& key:{"points","entities","constraints","dimensions"})for(const auto& row:sourceParameters[key].toArray()){const auto id=row.toObject()["id"].toString(),created=QUuid::createUuidV5(token,(QByteArray("alloy-sketch-copy/1/")+id.toUtf8())).toString(QUuid::WithoutBraces);map[id]=created;mapping[id]=created;}
  auto p=draft["parameters"].toObject();QJsonArray copiedIds;
  for(const auto& key:{"points","entities","constraints","dimensions"}){auto rows=p[key].toArray();for(const auto& row:sourceParameters[key].toArray()){
   auto o=remap(row,map).toObject();if(QString(key)=="points"){o["uMm"]=o["uMm"].toDouble()+du;o["vMm"]=o["vMm"].toDouble()+dv;}
   if(QString(key)=="entities")copiedIds.append(o["id"]);
   if(QString(key)=="dimensions"){o.remove("name");if(o.contains("annotation")){auto a=o["annotation"].toObject();a["uMm"]=a["uMm"].toDouble()+du;a["vMm"]=a["vMm"].toDouble()+dv;o["annotation"]=a;}}
   rows.append(o);
  }p[key]=rows;}
  auto candidate=draft;candidate["parameters"]=p;auto solved=solveDraft(candidate,context);need(solved.ok,solved.error);
  // Neither source nor any existing authored point/definition may drift in solve.
  const auto actual=solved.feature["parameters"].toObject();for(const auto& key:{"points","entities","constraints","dimensions"}){const auto rows=actual[key].toArray();const auto before=draft["parameters"].toObject()[key].toArray();for(int n=0;n<before.size();++n)need(rows[n]==before[n],"Paste would alter existing geometry or constraints; no copy was made");}
  for(const auto& row:sourceParameters["points"].toArray()){const auto old=row.toObject();const auto id=map.value(old["id"].toString());for(const auto& value:actual["points"].toArray()){const auto point=value.toObject();if(point["id"]==id)need(std::abs(point["uMm"].toDouble()-old["uMm"].toDouble()-du)<=1e-6 && std::abs(point["vMm"].toDouble()-old["vMm"].toDouble()-dv)<=1e-6,"Paste solver changed the requested rigid placement");}}
  const auto baseline=evaluateFeature(draft,{},context),evaluated=evaluateFeature(solved.feature,{},context);auto copyOnly=source;auto cp=actual;for(const auto& key:{"points","entities","constraints","dimensions"}){QJsonArray rows;for(const auto& row:actual[key].toArray())if(map.values().contains(row.toObject()["id"].toString()))rows.append(row);cp[key]=rows;}copyOnly["parameters"]=cp;const auto copy=evaluateFeature(copyOnly,{},context);need(copy.profiles.size()==1,"Paste requires one closed copied profile");
  for(const auto& profile:baseline.profiles){BRepExtrema_DistShapeShape distance(profile.face,copy.profiles[0].face);need(distance.IsDone() && distance.Value()>1e-6,QString("Paste profiles overlap or touch within 0.000001 mm: %1 and %2").arg(profile.profileId,copy.profiles[0].profileId));}
  need(evaluated.profiles.size()==baseline.profiles.size()+1,"Paste must produce one additional independent closed profile");
  solved.diagnostics["copyId"]=args["copyId"];solved.diagnostics["copiedEntityIds"]=copiedIds;solved.diagnostics["identityMap"]=mapping;return solved;
 }catch(const std::exception& e){return {false,draft,{},QString::fromUtf8(e.what())};}
}
