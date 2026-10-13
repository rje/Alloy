// SPDX-License-Identifier: MIT
#include "ReferenceView.h"
#include <BRep_Builder.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <TopoDS_Compound.hxx>
#include <gp_Circ.hxx>
#include <gp_Ax2.hxx>
#include <QJsonArray>
#include <QLineF>
#include <algorithm>
#include <cmath>
#include <numbers>

void ReferenceView::clearSketchTextHandles(){
    if(!context_.IsNull())for(const auto& item:sketchTextHandles_)context_->Remove(item,false);
    sketchTextHandles_.clear();sketchTextHandleState_={};sketchTextDrag_={};
    if(!view_.IsNull())view_->Redraw();
}

void ReferenceView::showSketchTextHandles(const QJsonObject& text,const QString& plane,const QJsonObject& frame){
    const auto drag=sketchTextDrag_;clearSketchTextHandles();sketchTextDrag_=drag;if(view_.IsNull())return;
    const auto actualFrame=frame.isEmpty() ? frameFor(plane) : frame;
    const auto o=actualFrame["origin"].toArray(),u=actualFrame["uAxis"].toArray(),v=actualFrame["vAxis"].toArray(),n=actualFrame["normal"].toArray();
    auto world=[&](double x,double y){return gp_Pnt(o[0].toDouble()+x*u[0].toDouble()+y*v[0].toDouble(),
        o[1].toDouble()+x*u[1].toDouble()+y*v[1].toDouble(),o[2].toDouble()+x*u[2].toDouble()+y*v[2].toDouble());};
    const double x=text["uMm"].toDouble(),y=text["vMm"].toDouble(),angle=text["angleDeg"].toDouble()*std::numbers::pi/180.;
    const double arm=std::max(8.,text["heightMm"].toDouble()*1.35);
    const auto pivot=world(x,y),rotate=world(x+arm*std::cos(angle),y+arm*std::sin(angle));
    const gp_Dir normal(n[0].toDouble(),n[1].toDouble(),n[2].toDouble());
    const auto radius=std::max(.25,view_->Camera()->Scale()*7./std::max(1,height()));
    BRep_Builder builder;TopoDS_Compound guide;builder.MakeCompound(guide);
    builder.Add(guide,BRepBuilderAPI_MakeEdge(pivot,rotate).Edge());
    builder.Add(guide,BRepBuilderAPI_MakeEdge(gp_Circ(gp_Ax2(pivot,normal),radius)).Edge());
    builder.Add(guide,BRepBuilderAPI_MakeEdge(gp_Circ(gp_Ax2(rotate,normal),radius)).Edge());
    Handle(AIS_Shape) item=new AIS_Shape(guide);item->SetColor(Quantity_Color(.93,.42,.08,Quantity_TOC_sRGB));
    item->SetWidth(2.5*devicePixelRatioF());item->SetZLayer(Graphic3d_ZLayerId_Topmost);
    context_->Display(item,AIS_WireFrame,-1,false);context_->Deactivate(item);sketchTextHandles_.push_back(item);
    sketchTextHandleState_={{"textId",text["id"]},{"plane",plane},{"move",projectSketchPoint(x,y,plane,actualFrame)},
        {"rotate",projectSketchPoint(x+arm*std::cos(angle),y+arm*std::sin(angle),plane,actualFrame)},
        {"uMm",x},{"vMm",y},{"angleDeg",text["angleDeg"]},{"viewportLocal",true}};
    view_->Redraw();
}
