// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include "SketchSpline.h"
#include <QJsonArray>
#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <cmath>
namespace { QJsonObject failure(const QString& code,const QString& message){return {{"ok",false},{"error",QJsonObject{{"code",code},{"message",message}}}};} }
QJsonObject AlloyWindow::splineCommand(const QString& op,const QJsonObject& in){
 if(session_.value("command")!="sketch.edit")return failure("no_active_sketch","Enter a sketch first");
 if(session_.value("baseDocumentRevision").toInteger()!=static_cast<qint64>(document_->revision()))return failure("stale_session","Restart the sketch after a document change");
 const auto draft=session_.value("draftSketch").toObject(),context=session_.value("projectionContext").toObject();
 if(op=="insertMode"){bool found=false;for(const auto& v:draft["parameters"].toObject()["entities"].toArray())if(v.toObject()["id"]==in["entityId"]&&v.toObject()["type"]=="fitSpline")found=true;if(!found)return failure("spline_required","Select a fit point spline");execute("sketch.tool",{{"tool","insertFitPoint"}});session_["splineInsertEntityId"]=in["entityId"];return QJsonObject{{"pending",true},{"authoredChange",false}};}
 if(op=="complete"){const auto picks=session_.value("pendingSplinePoints").toArray();if(picks.size()<2){execute("sketch.tool",{{"tool","select"}});return QJsonObject{{"pending",false},{"authoredChange",false}};}auto result=splineCommand("add",{{"points",picks}});if(!result.value("ok").isBool()||result.value("ok").toBool()){session_.remove("pendingSplinePoints");view_->clearSketchCursor();}return result;}
 if(op=="drag"){
  const auto phase=in["phase"].toString();
  if(phase=="begin"){
   if(session_.value("tool")!="select" || session_.contains("pendingSplineDrag"))return failure("drag_unavailable","Return to Select and finish the current gesture first");
   bool owns=false;for(const auto& v:draft["parameters"].toObject()["entities"].toArray()){const auto e=v.toObject();owns|=e["id"]==in["entityId"]&&e["type"]=="fitSpline"&&e["pointIds"].toArray().contains(in["pointId"]);}
   const auto kind=in["kind"].toString();const double sign=in.value("sign").toDouble(1);if(!owns || (kind!="point"&&kind!="tangent") || (sign!=1&&sign!=-1))return failure("invalid_spline_drag","Choose a fit point or its positive/negative tangent grip");
   session_["pendingSplineDrag"]=QJsonObject{{"draft",draft},{"entityId",in["entityId"]},{"pointId",in["pointId"]},{"kind",in["kind"]},{"sign",in.value("sign").toDouble(1)}};return QJsonObject{{"pending",true},{"authoredChange",false}};}
  const auto pending=session_.value("pendingSplineDrag").toObject();if(pending.isEmpty())return failure("drag_inactive","Begin a spline drag first");
  if(phase=="cancel"){view_->clearSplineDrag();session_.remove("pendingSplineDrag");view_->showSketchDraft(evaluateSketchDraft(draft));return QJsonObject{{"pending",false},{"authoredChange",false}};}
  if(draft!=pending["draft"].toObject()){session_.remove("pendingSplineDrag");view_->clearSplineDrag();view_->showSketchDraft(evaluateSketchDraft(draft));return failure("stale_spline_drag","The sketch changed during this preview; start a new drag");}
  if(!in["uMm"].isDouble()||!in["vMm"].isDouble()||!std::isfinite(in["uMm"].toDouble())||!std::isfinite(in["vMm"].toDouble()))return failure("invalid_spline_drag","Drag coordinates must be finite millimeters");
  const auto base=pending["draft"].toObject();SketchGeometry::SolveResult solved;
  if(pending["kind"]=="point")solved=SketchGeometry::movePoint(base,{{"pointId",pending["pointId"]},{"uMm",in["uMm"]},{"vMm",in["vMm"]}},context);
  else{QJsonObject point;for(const auto& v:base["parameters"].toObject()["points"].toArray())if(v.toObject()["id"]==pending["pointId"])point=v.toObject();const double sign=pending["sign"].toDouble();solved=SketchSpline::edit(base,"tangent",{{"entityId",pending["entityId"]},{"pointId",pending["pointId"]},{"duMm",3*sign*(in["uMm"].toDouble()-point["uMm"].toDouble())},{"dvMm",3*sign*(in["vMm"].toDouble()-point["vMm"].toDouble())}},context);}
  if(!solved.ok){if(phase=="end"){session_.remove("pendingSplineDrag");view_->showSketchDraft(evaluateSketchDraft(draft));}return failure("spline_edit_failed",solved.error);}
  if(phase=="end"){session_.remove("pendingSplineDrag");return publishSketchSolve(solved);}
  auto preview=pending;preview["previewDraft"]=solved.feature;preview["diagnostics"]=solved.diagnostics;session_["pendingSplineDrag"]=preview;
  view_->showSketchDraft(evaluateSketchDraft(solved.feature));auto selected=session_.value("selection").toObject();selected["tool"]="select";view_->decorateSketch(solved.feature,selected,solved.diagnostics,session_.value("inputs").toObject()["units"].toString());return QJsonObject{{"pending",true},{"authoredChange",false},{"previewDraft",solved.feature}};
 }
 const auto solved=op=="add"?SketchSpline::add(draft,in,context):SketchSpline::edit(draft,op,in,context);
 auto result=publishSketchSolve(solved);if(solved.ok){if(op=="remove")setSketchSelection({});else{const auto id=op=="add"?solved.diagnostics["createdSplineId"]:in["entityId"];setSketchSelection({{"entityIds",QJsonArray{id}}});}}return result;
}
QJsonObject AlloyWindow::splinePointer(const QJsonObject& in,bool place){
 const auto plane=session_.value("inputs").toObject()["plane"].toString(),tool=session_.value("tool").toString();const auto frame=session_.value("resolvedFrame").toObject();const auto point=view_->sketchPoint(in["x"].toDouble(),in["y"].toDouble(),plane,frame);
 if(tool=="insertFitPoint"){
  if(!place)return QJsonObject{{"authoredChange",false}};
  const auto draft=session_.value("draftSketch").toObject(),params=draft["parameters"].toObject();const auto hit=view_->pickSketch(draft,in["x"].toDouble(),in["y"].toDouble(),"entity",frame);if(hit["entityId"]!=session_.value("splineInsertEntityId"))return failure("spline_required","Click the selected spline curve");
  QJsonObject entity;for(const auto& v:params["entities"].toArray())if(v.toObject()["id"]==hit["entityId"])entity=v.toObject();const auto curve=SketchSpline::evaluate(params,entity).curve;GeomAPI_ProjectPointOnCurve projection(gp_Pnt(point["uMm"].toDouble(),point["vMm"].toDouble(),0),curve);if(projection.NbPoints()==0)return failure("spline_pick_failed","Could not resolve a curve parameter");auto result=splineCommand("insert",{{"entityId",entity["id"]},{"parameter",projection.LowerDistanceParameter()}});if(!result.value("ok").isBool()||result.value("ok").toBool())execute("sketch.tool",{{"tool","select"}});return result;
 }
 auto picks=session_.value("pendingSplinePoints").toArray();const QJsonObject next{{"uMm",point["uMm"]},{"vMm",point["vMm"]}};
 if(place){if(!picks.isEmpty()){const auto prior=picks.last().toObject();if(std::hypot(prior["uMm"].toDouble()-next["uMm"].toDouble(),prior["vMm"].toDouble()-next["vMm"].toDouble())<0.001)return failure("duplicate_fit_point","Place distinct consecutive fit points");}picks.append(next);session_["pendingSplinePoints"]=picks;}
 else picks.append(next);
 if(picks.size()>=2){auto preview=SketchGeometry::newFeature(document_->activeOwnerId(),"XY","Spline Preview");auto result=SketchSpline::add(preview,{{"points",picks}});if(result.ok)view_->showSketchCurveCursor(result.feature);else view_->clearSketchCursor();}
 return QJsonObject{{"pendingPoints",session_.value("pendingSplinePoints")},{"authoredChange",false}};
}
