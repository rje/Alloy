// SPDX-License-Identifier: MIT
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <iostream>
#include <cmath>
#include <stdexcept>
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
QJsonObject accept(const SketchGeometry::SolveResult& r){if(!r.ok)throw std::runtime_error(r.error.toStdString());return r.feature;}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
 auto s=SketchGeometry::newFeature("13fa543c-e237-4e2e-947c-320071321821","XY");
 s=SketchGeometry::addLine(s,{{"u1Mm",-30},{"v1Mm",10},{"u2Mm",40},{"v2Mm",10}});
 s=SketchGeometry::addLine(s,{{"u1Mm",10},{"v1Mm",-20},{"u2Mm",10},{"v2Mm",40}});
 const auto es=s.value("parameters").toObject().value("entities").toArray();const QJsonArray ids{es[0].toObject().value("id"),es[1].toObject().value("id")};
 const auto crossing=SketchGeometry::pointerLineIntersection(s.value("parameters").toObject(),ids);check(std::abs(crossing.value("uMm").toDouble()-10)<1e-6 && std::abs(crossing.value("vMm").toDouble()-10)<1e-6,"finite crossing");
 check(SketchGeometry::pointerLineIntersection(s.value("parameters").toObject(),{ids[0],ids[0]}).isEmpty(),"same source omitted");
 auto sources=s.value("parameters").toObject();
 auto unavailable=[&](double x1,double y1,double x2,double y2){auto p=sources;auto ps=p.value("points").toArray();const auto line=es[1].toObject();for(int i=0;i<ps.size();++i){auto pt=ps[i].toObject();if(pt.value("id")==line.value("startPointId")){pt["uMm"]=x1;pt["vMm"]=y1;}if(pt.value("id")==line.value("endPointId")){pt["uMm"]=x2;pt["vMm"]=y2;}ps[i]=pt;}p["points"]=ps;check(SketchGeometry::pointerLineIntersection(p,ids).isEmpty(),"invalid pair candidate omitted");};
 unavailable(-20,20,30,20);unavailable(-20,10,30,10);unavailable(10,20,10,40);
 const auto before=s;s=accept(SketchGeometry::addPointerEntity(s,"line",{{"u1Mm",-10},{"v1Mm",-10},{"u2Mm",10.2},{"v2Mm",10.2},{"endSnapTarget",QJsonObject{{"kind","intersection"},{"entityIds",ids}}}}));
 auto p=s.value("parameters").toObject();const auto relation=p.value("constraints").toArray().last().toObject();check(relation.value("type")=="intersection" && relation.value("entityIds")==ids,"retained pair");
 const auto endpoint=relation.value("pointIds").toArray().at(0);
 for(const auto& row:p.value("points").toArray())if(row.toObject().value("id")==endpoint)check(std::abs(row.toObject().value("uMm").toDouble()-10)<1e-6,"snapped endpoint");
 auto edited=p;auto ps=p.value("points").toArray();for(int i=0;i<ps.size();++i){auto pt=ps[i].toObject();for(const auto& row:before.value("parameters").toObject().value("points").toArray())if(pt.value("id")==row.toObject().value("id") && std::abs(pt.value("vMm").toDouble()-10)<1e-6){pt["vMm"]=15;ps[i]=pt;}}
 edited["points"]=ps;s["parameters"]=edited;s=accept(SketchGeometry::solveDraft(s));p=s.value("parameters").toObject();const auto after=SketchGeometry::pointerLineIntersection(p,ids);check(!after.isEmpty(),"source edit keeps finite crossing");
 for(const auto& row:p.value("points").toArray())if(row.toObject().value("id")==endpoint)check(std::hypot(row.toObject().value("uMm").toDouble()-after.value("uMm").toDouble(),row.toObject().value("vMm").toDouble()-after.value("vMm").toDouble())<1e-6,"endpoint follows pair");
 auto stale=SketchGeometry::addPointerEntity(s,"line",{{"u1Mm",-10},{"v1Mm",-10},{"u2Mm",10},{"v2Mm",10},{"endSnapTarget",QJsonObject{{"kind","intersection"},{"entityIds",QJsonArray{ids[0],"00000000-0000-4000-8000-000000000001"}}}}});check(!stale.ok && stale.feature==s,"stale refusal atomic");
 std::cout<<"Pointer intersection contract passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
