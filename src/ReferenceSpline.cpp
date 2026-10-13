// SPDX-License-Identifier: MIT
#include "ReferenceView.h"
#include "SketchSpline.h"
#include <AIS_Shape.hxx>
#include <AIS_Point.hxx>
#include <Geom_CartesianPoint.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <Prs3d_PointAspect.hxx>
#include <Graphic3d_ZLayerId.hxx>
#include <QJsonArray>
void ReferenceView::decorateSplineHandles(const QJsonObject& feature,const QJsonObject& selection){
 splineHandleState_={};if(context_.IsNull())return;const auto parameters=feature["parameters"].toObject();const auto plane=parameters["plane"].toString();QJsonArray grips,curves;
 for(const auto& v:parameters["entities"].toArray()){
  const auto entity=v.toObject();if(entity["type"]!="fitSpline")continue;bool selected=selection["entityIds"].toArray().contains(entity["id"]);for(const auto& id:entity["pointIds"].toArray())selected|=selection["pointIds"].toArray().contains(id);if(!selected)continue;
  const auto eval=SketchSpline::evaluate(parameters,entity);curves.append(QJsonObject{{"entityId",entity["id"]},{"degree",5},{"continuity","C2"}});
  for(const auto& item:eval.handles){const auto h=item.toObject();const double u=h["uMm"].toDouble(),w=h["vMm"].toDouble(),du=h["duMm"].toDouble()/3,dv=h["dvMm"].toDouble()/3;
   const auto negative=sketchWorld(plane,u-du,w-dv),positive=sketchWorld(plane,u+du,w+dv);
   if(negative.Distance(positive)>1e-8){Handle(AIS_Shape) line=new AIS_Shape(BRepBuilderAPI_MakeEdge(negative,positive).Edge());line->SetColor(Quantity_Color(0.06,0.44,0.22,Quantity_TOC_RGB));line->SetWidth(1.75*devicePixelRatioF());line->SetZLayer(Graphic3d_ZLayerId_Topmost);context_->Display(line,AIS_WireFrame,-1,false);context_->Deactivate(line);sketchDecorations_.push_back(line);}
   for(int sign:{-1,1}){const auto p=sketchWorld(plane,u+sign*du,w+sign*dv);Handle(AIS_Point) marker=new AIS_Point(new Geom_CartesianPoint(p));marker->Attributes()->SetPointAspect(new Prs3d_PointAspect(Aspect_TOM_O,Quantity_Color(0.03,0.40,0.18,Quantity_TOC_RGB),4*devicePixelRatioF()));marker->SetZLayer(Graphic3d_ZLayerId_Topmost);context_->Display(marker,0,-1,false);context_->Deactivate(marker);sketchDecorations_.push_back(marker);const auto screen=projectSketchPoint(u+sign*du,w+sign*dv,plane,sketchFrame_);grips.append(QJsonObject{{"entityId",entity["id"]},{"pointId",h["pointId"]},{"kind","tangent"},{"sign",sign},{"x",screen["x"]},{"y",screen["y"]}});}
  }
 }
 splineHandleState_={{"grips",grips},{"curves",curves}};
}
