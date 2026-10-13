// SPDX-License-Identifier: MIT
#include "ReferenceView.h"
#include "SketchSpline.h"
#include <AIS_Shape.hxx>
#include <AIS_Point.hxx>
#include <Geom_CartesianPoint.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <Prs3d_PointAspect.hxx>
#include <Prs3d_LineAspect.hxx>
#include <Graphic3d_ZLayerId.hxx>
#include <QJsonArray>
void ReferenceView::decorateSplineHandles(const QJsonObject& feature,const QJsonObject& selection){
 splineHandleState_={};if(context_.IsNull())return;const auto parameters=feature["parameters"].toObject();const auto plane=parameters["plane"].toString();QJsonArray grips,curves,cages,combs;QMap<QString,QJsonObject> points;for(const auto& value:parameters["points"].toArray())points[value.toObject()["id"].toString()]=value.toObject();
 for(const auto& v:parameters["entities"].toArray()){
  const auto entity=v.toObject();if(entity["type"]!="fitSpline"&&entity["type"]!="controlSpline")continue;const auto comb=SketchSpline::comb(parameters,entity);if(comb["enabled"].toBool()){auto display=comb;display["entityId"]=entity["id"];combs.append(display);}
  bool selected=selection["entityIds"].toArray().contains(entity["id"]);for(const auto& id:entity["pointIds"].toArray())selected|=selection["pointIds"].toArray().contains(id);if(entity["type"]=="controlSpline"){
   QJsonArray poles,legs;const auto ids=entity["pointIds"].toArray();
   for(int i=0;i<ids.size();++i){const auto p=points[ids[i].toString()];const auto screen=projectSketchPoint(p["uMm"].toDouble(),p["vMm"].toDouble(),plane,sketchFrame_);poles.append(QJsonObject{{"pointId",ids[i]},{"uMm",p["uMm"]},{"vMm",p["vMm"]},{"x",screen["x"]},{"y",screen["y"]}});
    if(i==0)continue;const auto a=points[ids[i-1].toString()];const auto start=sketchWorld(plane,a["uMm"].toDouble(),a["vMm"].toDouble()),end=sketchWorld(plane,p["uMm"].toDouble(),p["vMm"].toDouble());legs.append(QJsonObject{{"startPointId",ids[i-1]},{"endPointId",ids[i]}});if(start.Distance(end)<1e-9)continue;
    Handle(AIS_Shape) cage=new AIS_Shape(BRepBuilderAPI_MakeEdge(start,end).Edge());const Quantity_Color color(0.42,0.24,0.08,Quantity_TOC_RGB);Handle(Prs3d_LineAspect) dashed=new Prs3d_LineAspect(color,Aspect_TOL_DASH,selected?2.0:1.5);cage->SetColor(color);cage->Attributes()->SetWireAspect(dashed);cage->Attributes()->SetLineAspect(dashed);cage->SetZLayer(Graphic3d_ZLayerId_Topmost);context_->Display(cage,AIS_WireFrame,-1,false);context_->Deactivate(cage);sketchDecorations_.push_back(cage);
   }
   cages.append(QJsonObject{{"entityId",entity["id"]},{"poles",poles},{"legs",legs},{"selected",selected}});continue;
  }
  if(!selected)continue;
  const auto eval=SketchSpline::evaluate(parameters,entity);curves.append(QJsonObject{{"entityId",entity["id"]},{"degree",5},{"continuity","C2"}});
  for(const auto& item:eval.handles){const auto h=item.toObject();const double u=h["uMm"].toDouble(),w=h["vMm"].toDouble(),du=h["duMm"].toDouble()/3,dv=h["dvMm"].toDouble()/3;
   const auto negative=sketchWorld(plane,u-du,w-dv),positive=sketchWorld(plane,u+du,w+dv);
   if(negative.Distance(positive)>1e-8){Handle(AIS_Shape) line=new AIS_Shape(BRepBuilderAPI_MakeEdge(negative,positive).Edge());line->SetColor(Quantity_Color(0.06,0.44,0.22,Quantity_TOC_RGB));line->SetWidth(1.75*devicePixelRatioF());line->SetZLayer(Graphic3d_ZLayerId_Topmost);context_->Display(line,AIS_WireFrame,-1,false);context_->Deactivate(line);sketchDecorations_.push_back(line);}
   for(int sign:{-1,1}){const auto p=sketchWorld(plane,u+sign*du,w+sign*dv);Handle(AIS_Point) marker=new AIS_Point(new Geom_CartesianPoint(p));marker->Attributes()->SetPointAspect(new Prs3d_PointAspect(Aspect_TOM_O,Quantity_Color(0.03,0.40,0.18,Quantity_TOC_RGB),4*devicePixelRatioF()));marker->SetZLayer(Graphic3d_ZLayerId_Topmost);context_->Display(marker,0,-1,false);context_->Deactivate(marker);sketchDecorations_.push_back(marker);const auto screen=projectSketchPoint(u+sign*du,w+sign*dv,plane,sketchFrame_);grips.append(QJsonObject{{"entityId",entity["id"]},{"pointId",h["pointId"]},{"kind","tangent"},{"sign",sign},{"x",screen["x"]},{"y",screen["y"]}});}
  }
 }
 splineHandleState_={{"grips",grips},{"curves",curves},{"cages",cages},{"curvatureCombs",combs}};
}
