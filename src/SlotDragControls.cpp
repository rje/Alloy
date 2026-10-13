// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <QJsonArray>
#include <QElapsedTimer>
#include <cmath>
#include <Standard_Failure.hxx>
namespace {QJsonObject refuse(const QString& code,const QString& message){return {{"ok",false},{"error",QJsonObject{{"code",code},{"message",message}}}};}}
void AlloyWindow::cancelSlotDrag(){
 const bool pending=session_.contains("pendingSlotDrag");session_.remove("pendingSlotDrag");view_->cancelSlotPointDrag();
 if(pending&&session_.value("command")=="sketch.edit"){
  const auto draft=session_.value("draftSketch").toObject();view_->showSketchDraft(evaluateSketchDraft(draft));displayedSketchDraft_=draft;
  auto selected=session_.value("selection").toObject();selected["tool"]=session_.value("tool");
  view_->decorateSketch(draft,selected,session_.value("solverDiagnostics").toObject(),session_.value("inputs").toObject()["units"].toString());
 }
}
QJsonObject AlloyWindow::slotDrag(const QJsonObject& args){
 if(session_.value("command")!="sketch.edit")return refuse("no_active_sketch","Enter a sketch first");
 const auto phase=args["phase"].toString();
 if(phase=="cancel"){cancelSlotDrag();return {{"pending",false},{"authoredChange",false}};}
 if(session_.value("baseDocumentRevision").toInteger()!=static_cast<qint64>(document_->revision())){cancelSlotDrag();return refuse("stale_session","Restart the sketch after a document change");}
 const auto draft=session_.value("draftSketch").toObject();
 if(phase=="begin"){
  if(session_.value("tool")!="select"||session_.contains("pendingSlotDrag")){cancelSlotDrag();return refuse("slot_drag_unavailable","Finish the current gesture and return to Select");}
  const auto p=draft["parameters"].toObject();bool owns=false;
  for(const auto& value:p["slotGroups"].toArray())for(const auto& e:p["entities"].toArray()){const auto line=e.toObject();if(line["id"]==value.toObject()["centerlineId"]&&(args["pointId"]==line["startPointId"]||args["pointId"]==line["endPointId"]))owns=true;}
  if(!owns){view_->cancelSlotPointDrag();return refuse("slot_endpoint_required","Choose a retained slot end-arc center");}
  session_["pendingSlotDrag"]=QJsonObject{{"draft",draft},{"pointId",args["pointId"]}};return {{"pending",true},{"authoredChange",false}};
 }
 const auto pending=session_.value("pendingSlotDrag").toObject();if(pending.isEmpty())return refuse("slot_drag_inactive","Begin a slot drag first");
 if(draft!=pending["draft"].toObject()){cancelSlotDrag();return refuse("stale_slot_drag","The sketch changed during this preview; start again");}
 if((phase!="move"&&phase!="end")||!args["uMm"].isDouble()||!args["vMm"].isDouble()||!std::isfinite(args["uMm"].toDouble())||!std::isfinite(args["vMm"].toDouble())){cancelSlotDrag();return refuse("invalid_slot_drag","Drag coordinates must be finite millimeters");}
 QElapsedTimer timer;timer.start();const auto solved=SketchGeometry::movePoint(draft,{{"pointId",pending["pointId"]},{"uMm",args["uMm"]},{"vMm",args["vMm"]}},session_.value("projectionContext").toObject());
 if(!solved.ok){cancelSlotDrag();return refuse("slot_drag_failed",solved.error);}
 try{
  const auto evaluated=evaluateSketchDraft(solved.feature);
  if(phase=="end"){session_.remove("pendingSlotDrag");view_->cancelSlotPointDrag();return publishSketchSolve(solved);}
  auto preview=pending;preview["previewDraft"]=solved.feature;preview["previewGeometry"]=evaluated.summary;preview["diagnostics"]=solved.diagnostics;preview["solveElapsedMs"]=double(timer.nsecsElapsed())/1e6;session_["pendingSlotDrag"]=preview;
  auto selected=session_.value("selection").toObject();selected["tool"]="select";
  view_->showSlotDragPreview(evaluated,solved.feature,selected,solved.diagnostics,session_.value("inputs").toObject()["units"].toString());
  return {{"pending",true},{"authoredChange",false},{"previewGeometry",evaluated.summary},{"solveElapsedMs",preview["solveElapsedMs"]}};
 }catch(const Standard_Failure& error){cancelSlotDrag();return refuse("slot_preview_failed",QString::fromUtf8(error.GetMessageString()));}
 catch(const std::exception& error){cancelSlotDrag();return refuse("slot_preview_failed",QString::fromUtf8(error.what()));}
}
