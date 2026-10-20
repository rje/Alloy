// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <cmath>
namespace {
QJsonObject failure(const QString& code,const QString& message){return {{"ok",false},{"error",QJsonObject{{"code",code},{"message",message}}}};}
QString field(const QString& role){return role=="center" ? "centerPointId" : role=="objects" ? "sourceEntityId" : role=="direction1" ? "direction1EntityId" : "direction2EntityId";}
}
void AlloyWindow::syncSketchPatternInteractionControls(const QJsonObject& inputs,const QString& message){
 if(!sketchPatternDialog_)return;
 for(const auto& pair:{std::pair{"sourceEntityId","source"},std::pair{"centerPointId","center"},std::pair{"direction1EntityId","direction1"},std::pair{"direction2EntityId","direction2"}})if(inputs.contains(pair.first))if(auto* combo=sketchPatternDialog_->findChild<QComboBox*>("sketch.pattern."+QString(pair.second))){const QSignalBlocker block(combo);combo->setCurrentIndex(combo->findData(inputs[pair.first].toString()));}
 for(int axis=1;axis<=2;++axis){
  const auto qkey=QString("quantity%1").arg(axis),skey=QString("spacing%1Mm").arg(axis);
  if(inputs.contains(qkey))if(auto* spin=sketchPatternDialog_->findChild<QSpinBox*>("sketch.pattern.quantity"+QString::number(axis))){const QSignalBlocker block(spin);spin->setValue(inputs[qkey].toInt());}
  if(inputs.contains(skey))if(auto* spin=sketchPatternDialog_->findChild<QDoubleSpinBox*>("sketch.pattern.spacing"+QString::number(axis))){const QSignalBlocker block(spin);spin->setValue(inputs[skey].toDouble());}
 }
 for(const auto& pair:{std::pair{"quantity","quantity"},std::pair{"totalAngleDeg","angle"}})if(inputs.contains(pair.first)){
  if(auto* spin=sketchPatternDialog_->findChild<QSpinBox*>("sketch.pattern."+QString(pair.second))){const QSignalBlocker block(spin);spin->setValue(inputs[pair.first].toInt());}
  if(auto* spin=sketchPatternDialog_->findChild<QDoubleSpinBox*>("sketch.pattern."+QString(pair.second))){const QSignalBlocker block(spin);spin->setValue(inputs[pair.first].toDouble());}
 }
 const auto state=session_["patternInteraction"].toObject();const auto role=state["role"].toString();
 for(const auto& key:{"objects","center","direction1","direction2","values"})if(auto* button=sketchPatternDialog_->findChild<QPushButton*>("sketch.pattern.role."+QString(key))){const QSignalBlocker block(button);button->setChecked(role==key);}
 if(auto* label=sketchPatternDialog_->findChild<QLabel*>("sketch.pattern.selectionPrompt"))label->setText(role=="objects" ? "Objects · click the source Circle" : role=="center" ? "Center Point · click a standalone authored Point" : role=="direction1" ? "Direction 1 · click a construction Line" : role=="direction2" ? "Direction 2 · click a distinct nonparallel construction Line" : state["type"]=="circular" ? (state["inputs"].toObject()["mode"]=="full" ? "Full · 360° fixed · drag N for Quantity" : "Drag A for Total Angle · drag N for Quantity") : "Drag D1 / D2 for Spacing · drag N1 / N2 for Quantity");
 if(auto* label=sketchPatternDialog_->findChild<QLabel*>("sketch.pattern.status"))label->setText(message.isEmpty() ? "Preview · source and derived circles" : message);
 if(auto* buttons=sketchPatternDialog_->findChild<QDialogButtonBox*>())buttons->button(QDialogButtonBox::Ok)->setEnabled(session_.contains("pendingSketchPattern"));
}
void AlloyWindow::registerSketchPatternInteractionCommands(){
 auto add=[this](const QString& id,const QJsonObject& schema,std::function<QJsonObject(const QJsonObject&)> run){commands_.push_back({id,id,"Sketch Pattern canvas input",schema,true,std::move(run)});};
 const auto active=[this]{return session_["patternInteraction"].toObject()["active"].toBool() && session_["command"]=="sketch.edit";};
 const auto pick=[this](const QJsonObject& a){
  const auto role=session_["patternInteraction"].toObject()["role"].toString();
  const auto hit=view_->pickSketch(session_["draftSketch"].toObject(),a["x"].toDouble(),a["y"].toDouble(),role=="center" ? "point" : "entity",session_["resolvedFrame"].toObject());
  const auto id=hit["entityId"].toString();QJsonObject entity;for(const auto& row:session_["draftSketch"].toObject()["parameters"].toObject()["entities"].toArray())if(row.toObject()["id"]==id)entity=row.toObject();
  if(role=="center")return entity["type"]=="point" ? entity["pointId"].toString() : QString{};
  return (role=="objects" ? entity["type"]=="circle" : (role=="direction1" || role=="direction2") && entity["type"]=="line" && entity["construction"].toBool()) ? id : QString{};
 };
 const QJsonObject pixel{{"type","number"},{"minimum",0},{"maximum",10000}};
 add("sketch.patternRole",{{"role",QJsonObject{{"type","string"},{"enum",QJsonArray{"objects","center","direction1","direction2","values"}}}}},[this,active](const auto& a){if(!active())return failure("no_pattern_controls","Open Sketch Pattern first");auto s=session_["patternInteraction"].toObject();s["role"]=a["role"];s["hoveredEntityId"]="";s["hoveredPointId"]="";session_["patternInteraction"]=s;syncSketchPatternInteractionControls({});view_->setFocus(Qt::OtherFocusReason);return QJsonObject{{"role",a["role"]}};});
 add("sketch.patternHover",{{"x",pixel},{"y",pixel}},[this,active,pick](const auto& a){if(!active())return QJsonObject{};auto s=session_["patternInteraction"].toObject();const auto id=pick(a);s["hoveredEntityId"]=s["role"]=="center" ? QString{} : id;s["hoveredPointId"]=s["role"]=="center" ? id : QString{};session_["patternInteraction"]=s;return QJsonObject{{"entityId",id},{"role",s["role"]}};});
 add("sketch.patternPick",{{"x",pixel},{"y",pixel}},[this,active,pick](const auto& a){
  if(!active())return failure("no_pattern_controls","Open Sketch Pattern first");const auto id=pick(a);if(id.isEmpty())return failure("pattern_pick_unavailable","Pick a source Circle, standalone Center Point or construction Line for the active role");
  auto s=session_["patternInteraction"].toObject();const auto role=s["role"].toString();auto inputs=s["inputs"].toObject();inputs[field(role)]=id;
  const auto prior=session_.value("pendingSketchPattern");const auto response=execute("sketch.patternPreview",inputs);
  if(!response["ok"].toBool()){
   if(prior.isObject()){session_["pendingSketchPattern"]=prior;auto interaction=session_["patternInteraction"].toObject();interaction.remove("invalid");session_["patternInteraction"]=interaction;view_->showSketchDraft(evaluateSketchDraft(prior.toObject()["draft"].toObject()));}
   syncSketchPatternInteractionControls({},response["error"].toObject()["message"].toString());return response;
  }
  s=session_["patternInteraction"].toObject();auto roles=s["pickedRoles"].toObject();roles[role]=id;s["pickedRoles"]=roles;QJsonArray picked;for(const auto& value:roles)if(!picked.contains(value))picked.append(value);s["pickedEntityIds"]=picked;QJsonArray points;if(roles.contains("center"))points.append(roles["center"]);s["pickedPointIds"]=points;s["hoveredEntityId"]="";s["hoveredPointId"]="";s["role"]=role=="objects" ? (s["type"]=="circular" ? "center" : "direction1") : role=="direction1" ? "direction2" : "values";session_["patternInteraction"]=s;syncSketchPatternInteractionControls(inputs);return QJsonObject{{"entityId",id},{"role",s["role"]},{"inputs",inputs}};
 });
 const QJsonObject number{{"type","number"},{"minimum",-100000},{"maximum",100000},{"required",false}};
 add("sketch.patternHandle",{{"phase",QJsonObject{{"type","string"},{"enum",QJsonArray{"begin","move","end","cancel"}}}},{"key",QJsonObject{{"type","string"},{"required",false}}},{"value",number}},[this,active](const auto& a){
  if(!active())return failure("no_pattern_controls","Open Sketch Pattern first");const auto phase=a["phase"].toString();
  if(phase=="begin"){
   if(!session_.contains("pendingSketchPattern"))return failure("pattern_preview_required","Restore a valid Pattern preview before dragging a handle");
   const auto key=a["key"].toString();if(!QStringList{"spacing1Mm","spacing2Mm","quantity1","quantity2","quantity","totalAngleDeg"}.contains(key))return failure("pattern_handle_unavailable","Choose a current distance or Quantity handle");
   session_["patternHandleGesture"]=QJsonObject{{"key",key},{"inputs",session_["patternInteraction"].toObject()["inputs"]},{"pending",session_["pendingSketchPattern"]},{"invalid",false}};return QJsonObject{{"active",true}};
  }
  auto gesture=session_["patternHandleGesture"].toObject();if(gesture.isEmpty())return failure("no_pattern_gesture","Begin the Pattern handle gesture first");
  if(phase=="cancel" || (phase=="end" && gesture["invalid"].toBool())){
   auto s=session_["patternInteraction"].toObject();s["inputs"]=gesture["inputs"];s.remove("error");session_["patternInteraction"]=s;session_["pendingSketchPattern"]=gesture["pending"];view_->showSketchDraft(evaluateSketchDraft(gesture["pending"].toObject()["draft"].toObject()));session_.remove("patternHandleGesture");syncSketchPatternInteractionControls(gesture["inputs"].toObject());return QJsonObject{{"active",false},{"restored",true}};
  }
  if(phase=="end"){session_.remove("patternHandleGesture");return QJsonObject{{"active",false},{"inputs",session_["patternInteraction"].toObject()["inputs"]}};}
  if(!a["value"].isDouble() || !std::isfinite(a["value"].toDouble()))return failure("invalid_pattern_value","Pattern handle needs a finite value");
  auto inputs=gesture["inputs"].toObject();const auto key=gesture["key"].toString();inputs[key]=a["value"];
  const auto prior=session_.value("pendingSketchPattern");const auto response=execute("sketch.patternPreview",inputs);gesture["invalid"]=!response["ok"].toBool();session_["patternHandleGesture"]=gesture;
  if(gesture["invalid"].toBool()){
   if(prior.isObject()){session_["pendingSketchPattern"]=prior;auto interaction=session_["patternInteraction"].toObject();interaction.remove("invalid");session_["patternInteraction"]=interaction;view_->showSketchDraft(evaluateSketchDraft(prior.toObject()["draft"].toObject()));}
   syncSketchPatternInteractionControls({},response["error"].toObject()["message"].toString());return response;
  }
  syncSketchPatternInteractionControls(inputs);return QJsonObject{{"active",true},{"inputs",inputs},{"authoredChange",false}};
 });
 add("sketch.patternExit",{},[this](const auto&){if(sketchPatternDialog_)sketchPatternDialog_->reject();else if(session_["patternInteraction"].toObject()["active"].toBool())execute("sketch.patternCancel");return QJsonObject{{"active",false}};});
}
