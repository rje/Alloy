// SPDX-License-Identifier: MIT
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QJsonArray>
#include <QUuid>
#include <QPointF>
#include <cmath>
#include <iostream>
#include <numbers>
#include <stdexcept>
void check(bool ok,const QString& error){if(!ok)throw std::runtime_error(error.toStdString());}
QJsonObject accept(const SketchGeometry::SolveResult& r){check(r.ok,r.error);return r.feature;}
QJsonObject entity(const QJsonObject& f,const QString& id){for(const auto& v:f["parameters"].toObject()["entities"].toArray())if(v.toObject()["id"]==id)return v.toObject();return {};}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
 auto f=SketchGeometry::newFeature(QUuid::createUuid().toString(QUuid::WithoutBraces),"XY");
 const double v[][2]={{0,0},{0,40},{14,40},{14,-20},{3,-20},{3,24},{11,24},{11,0}};QStringList lines,widths;
 for(int i=0;i<7;++i){QJsonObject args{{"u1Mm",v[i][0]},{"v1Mm",v[i][1]},{"u2Mm",v[i+1][0]},{"v2Mm",v[i+1][1]}};
  if(i)args["startPointId"]=entity(f,lines.last())["endPointId"];
  f=SketchGeometry::addLine(f,args);const auto line=f["parameters"].toObject()["entities"].toArray().last().toObject();lines<<line["id"].toString();
  f=accept(SketchGeometry::addConstraint(f,{{"type",i%2 ? "horizontal" : "vertical"},{"entityIds",QJsonArray{lines.last()}}}));
  f=accept(SketchGeometry::addDimension(f,{{"kind",i%2 ? "horizontal" : "vertical"},{"entityId",lines.last()},{"valueMm",std::hypot(v[i+1][0]-v[i][0],v[i+1][1]-v[i][1])}}));
  if(i%2)widths<<f["parameters"].toObject()["dimensions"].toArray().last().toObject()["id"].toString();
 }
 f=accept(SketchGeometry::addConstraint(f,{{"type","fixed"},{"pointIds",QJsonArray{entity(f,lines[0])["startPointId"]}}}));
 for(int pair=0;pair<3;++pair){const double radius[]={7,5.5,4};const int a=pair*2;const QJsonArray corners{QJsonObject{{"entityIds",QJsonArray{lines[a],lines[a+1]}}},QJsonObject{{"entityIds",QJsonArray{lines[a+1],lines[a+2]}}}};
  if(pair==0){const auto dependent=SketchGeometry::addLine(f,{{"u1Mm",0},{"v1Mm",40},{"u2Mm",-10},{"v2Mm",40},{"startPointId",entity(f,lines[0])["endPointId"]}});
   const auto refusal=SketchGeometry::filletCorners(dependent,{{"corners",corners},{"radiusMm",radius[pair]}});check(!refusal.ok && refusal.feature==dependent,"Unrelated corner geometry was silently discarded");}
  const auto before=f;const auto bad=SketchGeometry::filletCorners(f,{{"corners",corners},{"radiusMm",radius[pair]+0.1}});check(!bad.ok && bad.feature==before,"Negative residual changed draft");
  const auto result=SketchGeometry::filletCorners(f,{{"corners",corners},{"radiusMm",radius[pair]}});f=accept(result);
  check(result.diagnostics["removedEntityIds"].toArray()==QJsonArray{lines[a+1]},"Shared line retirement missing");
  check(result.diagnostics["convertedReferenceDimensionIds"].toArray()==QJsonArray{widths[pair]},"Width dimension identity conversion missing");
 }
 const auto entities=f["parameters"].toObject()["entities"].toArray();check(entities.size()==10,"Expected four lines and six arcs");
 QMap<QString,QPointF> points;for(const auto& v:f["parameters"].toObject()["points"].toArray()){const auto p=v.toObject();points[p["id"].toString()]={p["uMm"].toDouble(),p["vMm"].toDouble()};}
 double length=0;int arcs=0;for(const auto& v:entities){const auto e=v.toObject();const auto d=points[e["endPointId"].toString()]-points[e["startPointId"].toString()];if(e["type"]=="line")length+=std::hypot(d.x(),d.y());else{++arcs;check(std::abs(std::abs(e["sweepAngleDeg"].toDouble())-90)<1e-6,"Quarter arc orientation changed");length+=e["radiusMm"].toDouble()*std::abs(e["sweepAngleDeg"].toDouble())*std::numbers::pi/180;}}
 check(arcs==6 && std::abs(length-(135+16.5*std::numbers::pi))<1e-6,"Source analytic path mismatch");
 for(int i:{0,2,4,6})check(!entity(f,lines[i]).isEmpty(),"Surviving line ID lost");
 for(const auto& id:widths){bool found=false;for(const auto& v:f["parameters"].toObject()["dimensions"].toArray()){const auto d=v.toObject();if(d["id"]==id){found=true;check(d["kind"]=="horizontal" && !d["driving"].toBool() && d["pointIds"].toArray().size()==2,"Converted span is not reference");}}check(found,"Width disappeared");}
 // Shared contact derivatives must align in traversal order, including a radius edit.
 auto g1=[&](const QJsonObject& shape){const auto es=shape["parameters"].toObject()["entities"].toArray();for(int i=4;i<10;i+=2){const auto a=es[i].toObject(),b=es[i+1].toObject();check(a["centerPointId"]==b["centerPointId"] && a["endPointId"]==b["startPointId"],"Return contact identities differ");
  const double end=(a["startAngleDeg"].toDouble()+a["sweepAngleDeg"].toDouble())*std::numbers::pi/180,start=b["startAngleDeg"].toDouble()*std::numbers::pi/180;
  const double sa=a["sweepAngleDeg"].toDouble()>0 ? 1 : -1,sb=b["sweepAngleDeg"].toDouble()>0 ? 1 : -1;
  check(sa*sb*std::cos(end-start)>1-1e-8,"Return has a cusp or reversed tangent");}};
 g1(f);QString radiusId;for(const auto& v:f["parameters"].toObject()["dimensions"].toArray())if(v.toObject()["kind"]=="radius"){radiusId=v.toObject()["id"].toString();break;}
 const auto edited=accept(SketchGeometry::updateDimension(f,{{"dimensionId",radiusId},{"valueMm",6.5}}));g1(edited);
 check(edited["parameters"].toObject()["entities"].toArray().size()==10,"Edit resurrected exhausted line");
 std::cout<<"PASS: actual paired source fillets, width identity conversion, source length and atomic negative residual\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
