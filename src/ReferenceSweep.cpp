// SPDX-License-Identifier: MIT
#include "ReferenceView.h"
#include <AIS_Shape.hxx>
#include <Graphic3d_ZLayerId.hxx>
void ReferenceView::setCommandSketches(const QSet<QString>& ids){commandSketches_=ids;applyEditDisplay();}
void ReferenceView::clearSweepPath(){if(!context_.IsNull())for(const auto& model:sweepPathOverlays_)context_->Remove(model,false);sweepPathOverlays_.clear();if(!context_.IsNull() && !sweepProfileOverlay_.IsNull())context_->Remove(sweepProfileOverlay_,false);sweepProfileOverlay_.Nullify();sweepPathState_={};}
void ReferenceView::showSweepPath(const EvaluatedSketch& sketch,const QJsonArray& entityIds){
    clearSweepPath();if(context_.IsNull())return;QJsonArray shown;
    for(const auto& id:entityIds){const auto edge=sketch.entityEdges.value(id.toString());if(edge.IsNull())continue;Handle(AIS_Shape) model=new AIS_Shape(edge);model->SetColor(Quantity_Color(.96,.44,.08,Quantity_TOC_sRGB));model->SetWidth(2.8*devicePixelRatioF());model->SetZLayer(Graphic3d_ZLayerId_Topmost);context_->Display(model,AIS_WireFrame,-1,false);context_->Deactivate(model);sweepPathOverlays_.push_back(model);shown.append(id);}
    sweepPathState_={{"sketchId",sketch.featureId},{"entityIds",shown},{"edgeCount",shown.size()},{"color","orange"},{"topmost",true}};context_->UpdateCurrentViewer();
}

void ReferenceView::showSweepProfile(const EvaluatedProfile& profile){
    if(context_.IsNull())return;if(!sweepProfileOverlay_.IsNull())context_->Remove(sweepProfileOverlay_,false);sweepProfileOverlay_=new AIS_Shape(profile.face);sweepProfileOverlay_->SetColor(Quantity_Color(.17,.48,.90,Quantity_TOC_sRGB));sweepProfileOverlay_->SetWidth(2.4*devicePixelRatioF());sweepProfileOverlay_->SetZLayer(Graphic3d_ZLayerId_Topmost);context_->Display(sweepProfileOverlay_,AIS_WireFrame,-1,false);context_->Deactivate(sweepProfileOverlay_);sweepPathState_["profileSketchId"]=profile.sketchId;sweepPathState_["profileId"]=profile.profileId;sweepPathState_["profileColor"]="blue";context_->UpdateCurrentViewer();
}
