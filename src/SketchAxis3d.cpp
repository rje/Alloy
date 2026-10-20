// SPDX-License-Identifier: MIT
#include "SketchAxis3d.h"
#include <QJsonArray>
#include <QUuid>
#include <cmath>
#include <stdexcept>
namespace {
void need(bool ok,const char* text){if(!ok)throw std::invalid_argument(text);}
QString uuid(){return QUuid::createUuid().toString(QUuid::WithoutBraces);}
bool validId(const QJsonValue& v){return v.isString() && !QUuid(v.toString()).isNull() && QUuid(v.toString()).toString(QUuid::WithoutBraces)==v.toString();}
void coordinate(const QJsonObject& p,const char* key){need(p[key].isDouble() && std::isfinite(p[key].toDouble()) && std::abs(p[key].toDouble())<=100000,"3D construction coordinates must be finite within ±100000 mm");}
bool mentions(const QJsonValue& v,const QSet<QString>& ids){if(v.isString())return ids.contains(v.toString());if(v.isArray())for(const auto& row:v.toArray())if(mentions(row,ids))return true;if(v.isObject())for(const auto& row:v.toObject())if(mentions(row,ids))return true;return false;}
}
namespace SketchAxis3d {
QSet<QString> ids(const QJsonObject& p){QSet<QString> result;for(const auto& row:p["constructionLines3d"].toArray()){const auto a=row.toObject();result.insert(a["id"].toString());result.insert(a["start"].toObject()["id"].toString());result.insert(a["end"].toObject()["id"].toString());}return result;}
QString validate(const QJsonObject& p){try{
 if(!p.contains("constructionLines3d"))return {};
 need(p["constructionLines3d"].isArray() && p["constructionLines3d"].toArray().size()<=1,"Only one normal 3D construction Line is supported");QSet<QString> used;
 for(const auto& row:p["constructionLines3d"].toArray()){
  need(row.isObject(),"3D construction Line must be an object");const auto a=row.toObject();need(p["plane"]=="XY" && a.size()==5 && a["type"]=="line" && a["construction"]==true && validId(a["id"]),"3D construction Line requires an XY sketch, Line identity and construction role");
  need(a["start"].isObject() && a["end"].isObject(),"3D construction Line requires authored start/end points");for(const auto& point:{a["start"].toObject(),a["end"].toObject()}){need(point.size()==4 && validId(point["id"]),"Invalid 3D endpoint identity/fields");for(const auto* key:{"uMm","vMm","wMm"})coordinate(point,key);need(!used.contains(point["id"].toString()),"Duplicate 3D construction identity");used.insert(point["id"].toString());}need(!used.contains(a["id"].toString()),"Duplicate 3D construction identity");used.insert(a["id"].toString());const auto start=a["start"].toObject(),end=a["end"].toObject();need(start["wMm"].toDouble()==0 && end["uMm"]==start["uMm"] && end["vMm"]==start["vMm"] && end["wMm"].toDouble()>=.001,"3D construction Line must start at (u,v,0) and end at (u,v,h), h ≥ 0.001 mm along +Z");
 }
 return {};
 }catch(const std::exception& e){return QString::fromUtf8(e.what());}}
bool referenced(const QJsonObject& p,const QString& id){QSet<QString> owned;for(const auto& row:p["constructionLines3d"].toArray()){const auto a=row.toObject();if(a["id"]==id)owned={id,a["start"].toObject()["id"].toString(),a["end"].toObject()["id"].toString()};}for(auto it=p.begin();it!=p.end();++it)if(it.key()!="constructionLines3d" && mentions(it.value(),owned))return true;return false;}
QJsonObject upsert(const QJsonObject& p,double u,double v,double h,const QString& id){need(validate(p).isEmpty(),"Existing 3D construction Line is invalid");const auto old=p["constructionLines3d"].toArray();QJsonObject a;
 if(id.isEmpty()){need(old.isEmpty(),"This sketch already has its normal 3D Line; edit that identity");a={{"id",uuid()},{"type","line"},{"construction",true},{"start",QJsonObject{{"id",uuid()}}},{"end",QJsonObject{{"id",uuid()}}}};}
 else{need(old.size()==1 && old.first().toObject()["id"]==id,"3D construction Line ID is unavailable in this sketch");need(!referenced(p,id),"3D construction Line is referenced; edit its owner before changing the axis");a=old.first().toObject();}
 auto start=a["start"].toObject(),end=a["end"].toObject();start["uMm"]=u;start["vMm"]=v;start["wMm"]=0;end["uMm"]=u;end["vMm"]=v;end["wMm"]=h;a["start"]=start;a["end"]=end;auto result=p;result["constructionLines3d"]=QJsonArray{a};const auto error=validate(result);need(error.isEmpty(),qPrintable(error));return result;
}
QJsonObject erase(const QJsonObject& p,const QString& id){const auto rows=p["constructionLines3d"].toArray();need(rows.size()==1 && rows.first().toObject()["id"]==id,"3D construction Line ID is unavailable in this sketch");need(!referenced(p,id),"3D construction Line is referenced; remove its dependent owner first");auto result=p;result.remove("constructionLines3d");return result;}
QJsonArray inspect(const QJsonObject& p){QJsonArray result;for(const auto& row:p["constructionLines3d"].toArray()){auto a=row.toObject();a["direction"]=QJsonArray{0,0,1};a["lengthMm"]=a["end"].toObject()["wMm"];a["referenceOnly"]=true;a["solverVariables"]=0;a["profileBoundary"]=false;result.append(a);}return result;}
}
