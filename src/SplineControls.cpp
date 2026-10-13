// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include "SketchSpline.h"
#include <QJsonArray>
#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <cmath>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QSignalBlocker>
namespace { QJsonObject failure(const QString& code,const QString& message){return {{"ok",false},{"error",QJsonObject{{"code",code},{"message",message}}}};} }
QJsonObject AlloyWindow::splineCommand(const QString& op,const QJsonObject& in){
 if(session_.value("command")!="sketch.edit")return failure("no_active_sketch","Enter a sketch first");
 if(session_.value("baseDocumentRevision").toInteger()!=static_cast<qint64>(document_->revision()))return failure("stale_session","Restart the sketch after a document change");
 const auto draft=session_.value("draftSketch").toObject(),context=session_.value("projectionContext").toObject();
 if(op=="insertMode"){bool found=false;for(const auto& v:draft["parameters"].toObject()["entities"].toArray())if(v.toObject()["id"]==in["entityId"]&&v.toObject()["type"]=="fitSpline")found=true;if(!found)return failure("spline_required","Select a fit point spline");execute("sketch.tool",{{"tool","insertFitPoint"}});session_["splineInsertEntityId"]=in["entityId"];return QJsonObject{{"pending",true},{"authoredChange",false}};}
 if(op=="complete"){const auto picks=session_.value("pendingSplinePoints").toArray();if(picks.size()<2){execute("sketch.tool",{{"tool","select"}});return QJsonObject{{"pending",false},{"authoredChange",false}};}const bool control=session_.value("tool")=="controlSpline";QJsonObject args{{"points",picks}};if(control)args["maximumDegree"]=session_.value("splineMaximumDegree").toInt(5);auto result=splineCommand(control?"addControl":"add",args);if(!result.value("ok").isBool()||result.value("ok").toBool()){session_.remove("pendingSplinePoints");view_->clearSketchCursor();}return result;}
 if(op=="drag"){
  const auto phase=in["phase"].toString();
  if(phase=="begin"){
   if(session_.value("tool")!="select" || session_.contains("pendingSplineDrag"))return failure("drag_unavailable","Return to Select and finish the current gesture first");
   bool owns=false;for(const auto& v:draft["parameters"].toObject()["entities"].toArray()){const auto e=v.toObject();owns|=e["id"]==in["entityId"]&&(e["type"]=="fitSpline"||e["type"]=="controlSpline")&&e["pointIds"].toArray().contains(in["pointId"]);}
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
 const auto solved=op=="add"?SketchSpline::add(draft,in,context):op=="addControl"?SketchSpline::addControl(draft,in,context):op=="comb"?SketchSpline::setComb(draft,in,context):SketchSpline::edit(draft,op,in,context);
 auto result=publishSketchSolve(solved);if(solved.ok){if(op=="remove")setSketchSelection({});else{const auto id=(op=="add"||op=="addControl")?solved.diagnostics["createdSplineId"]:in["entityId"];setSketchSelection({{"entityIds",QJsonArray{id}}});}}return result;
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
 if(picks.size()>=2){auto preview=SketchGeometry::newFeature(document_->activeOwnerId(),"XY","Spline Preview");auto result=tool=="controlSpline"?SketchSpline::addControl(preview,{{"points",picks},{"maximumDegree",session_.value("splineMaximumDegree").toInt(5)}}):SketchSpline::add(preview,{{"points",picks}});if(result.ok)view_->showSketchCurveCursor(result.feature);else view_->clearSketchCursor();}
 return QJsonObject{{"pendingPoints",session_.value("pendingSplinePoints")},{"authoredChange",false}};
}

QJsonObject AlloyWindow::selectedSplineEntity() const {
 const auto draft=session_.value("draftSketch").toObject(),selection=session_.value("selection").toObject();QJsonObject chosen;
 for(const auto& value:draft["parameters"].toObject()["entities"].toArray()){const auto e=value.toObject();if(e["type"]!="fitSpline"&&e["type"]!="controlSpline")continue;bool selected=selection["entityIds"].toArray().contains(e["id"]);for(const auto& id:e["pointIds"].toArray())selected|=selection["pointIds"].toArray().contains(id);if(!selected)continue;if(!chosen.isEmpty())return {};chosen=e;}return chosen;
}
void AlloyWindow::buildSplinePalette(QVBoxLayout* layout){
 splineDegreeControls_=new QWidget;splineDegreeControls_->setObjectName("sketch.spline.degreeControls");auto* degreeForm=new QFormLayout(splineDegreeControls_);degreeForm->setContentsMargins(0,0,0,0);splineDegree_=new QComboBox;splineDegree_->setObjectName("sketch.spline.maximumDegree");splineDegree_->addItem("Degree 5",5);splineDegree_->addItem("Degree 3",3);degreeForm->addRow("Spline Degree",splineDegree_);layout->addWidget(splineDegreeControls_);splineDegreeControls_->hide();connect(splineDegree_,qOverload<int>(&QComboBox::currentIndexChanged),this,[this]{sketchResponse(execute("sketch.splineDegree",{{"maximumDegree",splineDegree_->currentData().toInt()}}));view_->setFocus();});
 splineCombControls_=new QWidget;splineCombControls_->setObjectName("sketch.spline.combControls");auto* box=new QVBoxLayout(splineCombControls_);box->setContentsMargins(0,0,0,0);splineComb_=new QCheckBox("Curvature Comb");splineComb_->setObjectName("sketch.spline.curvatureComb");box->addWidget(splineComb_);auto* form=new QFormLayout;splineCombDensity_=new QSpinBox;splineCombDensity_->setObjectName("sketch.spline.combDensity");splineCombDensity_->setRange(4,256);splineCombDensity_->setValue(32);splineCombScale_=new QDoubleSpinBox;splineCombScale_->setObjectName("sketch.spline.combScale");splineCombScale_->setRange(.01,100);splineCombScale_->setDecimals(2);splineCombScale_->setSingleStep(.1);splineCombScale_->setValue(1);form->addRow("Density",splineCombDensity_);form->addRow("Scale",splineCombScale_);box->addLayout(form);splineCombInfo_=new QLabel;splineCombInfo_->setWordWrap(true);splineCombInfo_->setObjectName("sketch.spline.combInfo");box->addWidget(splineCombInfo_);layout->addWidget(splineCombControls_);splineCombControls_->hide();
 connect(splineComb_,&QCheckBox::toggled,this,[this](bool enabled){const auto e=selectedSplineEntity();if(e.isEmpty())return;sketchResponse(execute("sketch.curvatureComb",{{"entityId",e["id"]},{"enabled",enabled}}));view_->setFocus();});
 auto settings=[this]{const auto e=selectedSplineEntity();if(e.isEmpty()||!splineComb_->isChecked())return;sketchResponse(execute("sketch.curvatureComb",{{"entityId",e["id"]},{"enabled",true},{"density",splineCombDensity_->value()},{"scale",splineCombScale_->value()}}));};
 connect(splineCombDensity_,qOverload<int>(&QSpinBox::valueChanged),this,[settings](int){settings();});connect(splineCombScale_,qOverload<double>(&QDoubleSpinBox::valueChanged),this,[settings](double){settings();});
}
void AlloyWindow::refreshSplinePalette(){
 if(!splineDegreeControls_)return;const bool control=session_.value("tool")=="controlSpline";splineDegreeControls_->setVisible(control);{const QSignalBlocker block(splineDegree_);splineDegree_->setCurrentIndex(splineDegree_->findData(session_.value("splineMaximumDegree").toInt(5)));}
 const auto e=selectedSplineEntity();const bool eligible=session_.value("tool")=="select"&&!e.isEmpty();splineCombControls_->setVisible(eligible);if(!eligible)return;const auto c=e.value("curvatureComb").toObject(QJsonObject{{"enabled",false},{"density",32},{"scale",1}});const bool enabled=c["enabled"].toBool();{const QSignalBlocker a(splineComb_),b(splineCombDensity_),d(splineCombScale_);splineComb_->setChecked(enabled);splineCombDensity_->setValue(c["density"].toInt());splineCombScale_->setValue(c["scale"].toDouble());}splineCombDensity_->setEnabled(enabled);splineCombScale_->setEnabled(enabled);
 QString text;if(enabled){const auto details=SketchSpline::comb(session_.value("draftSketch").toObject()["parameters"].toObject(),e);if(details["clippedCount"].toInt())text=QString("%1 clipped teeth; reduce Scale.").arg(details["clippedCount"].toInt());if(details["undefinedCount"].toInt())text+=QString(" %1 undefined samples.").arg(details["undefinedCount"].toInt());}splineCombInfo_->setText(text);splineCombInfo_->setVisible(!text.isEmpty());
}
