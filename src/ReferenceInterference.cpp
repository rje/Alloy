// SPDX-License-Identifier: MIT
#include "ReferenceView.h"
#include <AIS_Shape.hxx>
#include <AIS_InteractiveContext.hxx>
#include <Graphic3d_ZLayerId.hxx>
#include <Quantity_Color.hxx>
void ReferenceView::setInterferenceFeedback(const std::vector<TopoDS_Shape>& sources,const TopoDS_Shape& result){
    if(context_.IsNull())return;
    for(const auto& shape:interferenceFeedback_)context_->Remove(shape,false);
    interferenceFeedback_.clear();
    for(const auto& source:sources)if(!source.IsNull()){
        Handle(AIS_Shape) outline=new AIS_Shape(source);outline->SetColor(Quantity_Color(.12,.46,.92,Quantity_TOC_sRGB));outline->SetWidth(2.0);
        context_->Display(outline,AIS_WireFrame,-1,false);context_->Deactivate(outline);interferenceFeedback_.push_back(outline);
    }
    if(!result.IsNull()){
        Handle(AIS_Shape) cue=new AIS_Shape(result);cue->SetColor(Quantity_Color(1.,.27,.06,Quantity_TOC_sRGB));cue->SetWidth(4.0);
        cue->SetZLayer(Graphic3d_ZLayerId_Topmost);context_->Display(cue,AIS_Shaded,-1,false);context_->Deactivate(cue);interferenceFeedback_.push_back(cue);
    }
    context_->UpdateCurrentViewer();
}
