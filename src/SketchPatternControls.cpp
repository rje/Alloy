// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include "GeometryKernel.h"
#include <QComboBox>
#include <QCloseEvent>
#include <QDialog>
#include <QDockWidget>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QShortcut>
#include <QVBoxLayout>
namespace {
class PatternDock final:public QDockWidget{public:using QDockWidget::QDockWidget;std::function<void()> closed;protected:void closeEvent(QCloseEvent* event)override{if(closed)closed();QDockWidget::closeEvent(event);}};
QJsonObject failure(const QString& code,const QString& message){return {{"ok",false},{"error",QJsonObject{{"code",code},{"message",message}}}};} }
void AlloyWindow::registerSketchPatternCommands(){
 auto add=[this](const QString& id,const QString& title,const QJsonObject& parameters,std::function<QJsonObject(const QJsonObject&)> run){commands_.push_back({id,title,title,parameters,true,std::move(run)});};
 // The same retained solver and transient draft serve native and local control.
 QJsonObject schema;for(const auto* key:{"groupId","sourceEntityId","direction1EntityId","direction2EntityId","type","centerPointId","mode"})schema[key]=QJsonObject{{"type","string"},{"required",QString(key)=="sourceEntityId"}};
 for(const auto* key:{"quantity1","quantity2","spacing1Mm","spacing2Mm","quantity","totalAngleDeg"})schema[key]=QJsonObject{{"type","number"},{"minimum",-100000},{"maximum",100000},{"required",false}};
 add("sketch.patternPreview","Preview Sketch Pattern",schema,[this](const auto& a){
  if(session_.value("command")!="sketch.edit")return failure("no_active_sketch","Enter a sketch first");
  auto base=session_["draftSketch"].toObject(),inputs=a;const auto prior=session_["pendingSketchPattern"].toObject();
  if(!prior.isEmpty() && prior["base"]==base && !inputs.contains("groupId")){
   base=prior["draft"].toObject();inputs["groupId"]=base["parameters"].toObject()["patternGroups"].toArray().first().toObject()["id"];
  }
  const auto solved=SketchGeometry::rectangularPattern(base,inputs,session_["projectionContext"].toObject());
  if(!solved.ok){session_.remove("pendingSketchPattern");view_->showSketchDraft(evaluateSketchDraft(session_["draftSketch"].toObject()));return failure("sketch_pattern_failed",solved.error);}
  // An existing explicit downstream profile set must remain complete. Refuse
  // removed indices before publishing a draft history item or preview.
  if(!session_["inputs"].toObject()["sketchId"].toString().isEmpty())try{
   GeometryKernel::evaluateDocument(SketchGeometry::replaceFeature(document_->authored(),solved.feature));
  }catch(const std::exception& error){session_.remove("pendingSketchPattern");view_->showSketchDraft(evaluateSketchDraft(session_["draftSketch"].toObject()));return failure("sketch_pattern_dependency",QString::fromUtf8(error.what()));}
  const auto geometry=evaluateSketchDraft(solved.feature);
  session_["pendingSketchPattern"]=QJsonObject{{"base",session_["draftSketch"]},{"draft",solved.feature},{"diagnostics",solved.diagnostics},{"geometry",geometry.summary}};
  view_->showSketchDraft(geometry);displayedSketchDraft_=session_["draftSketch"].toObject();
  return QJsonObject{{"pending",true},{"authoredChange",false},{"quantitySemantics","Total includes source"},{"patternGroup",solved.feature["parameters"].toObject()["patternGroups"].toArray().first()},{"sketchGeometry",geometry.summary},{"diagnostics",solved.diagnostics}};
 });
 add("sketch.patternAccept","Accept Sketch Pattern",{},[this](const auto&){
  const auto pending=session_["pendingSketchPattern"].toObject();if(pending.isEmpty())return failure("pattern_preview_required","Preview Rectangular Pattern first");
  if(pending["base"]!=session_["draftSketch"])return failure("stale_pattern_preview","The sketch changed; preview again");
  return publishSketchSolve(SketchGeometry::solveDraft(pending["draft"].toObject(),session_["projectionContext"].toObject()));
 });
 add("sketch.patternCancel","Cancel Sketch Pattern",{},[this](const auto&){
  if(session_.value("command")!="sketch.edit")return failure("no_active_sketch","Enter a sketch first");
  session_.remove("pendingSketchPattern");const auto draft=session_["draftSketch"].toObject();view_->showSketchDraft(evaluateSketchDraft(draft));displayedSketchDraft_=draft;
  return QJsonObject{{"pending",false},{"authoredChange",false}};
 });
 add("sketch.patternEdit","Edit Sketch Pattern",{{"groupId",QJsonObject{{"type","string"}}}},[this](const auto& a){if(session_.value("command")!="sketch.edit")return failure("no_active_sketch","Enter a sketch first");const auto groups=session_["draftSketch"].toObject()["parameters"].toObject()["patternGroups"].toArray();if(groups.isEmpty() || groups.first().toObject()["id"]!=a["groupId"])return failure("pattern_missing","Pattern association is unavailable");showSketchPatternControls();return QJsonObject{{"visible",true},{"groupId",a["groupId"]}};});
 add("sketch.patternStart","Rectangular Pattern",{},[this](const auto&){if(session_.value("command")!="sketch.edit")return failure("no_active_sketch","Enter a sketch first");const auto groups=session_["draftSketch"].toObject()["parameters"].toObject()["patternGroups"].toArray();if(!groups.isEmpty() && groups.first().toObject()["type"]!="rectangular")return failure("pattern_owner_exists","This sketch already has a different Pattern association; edit its visible glyph or use a separate sketch");session_["newSketchPatternType"]="rectangular";showSketchPatternControls();return QJsonObject{{"visible",true}};});
 add("sketch.circularPatternStart","Circular Pattern",{},[this](const auto&){if(session_.value("command")!="sketch.edit")return failure("no_active_sketch","Enter a sketch first");const auto groups=session_["draftSketch"].toObject()["parameters"].toObject()["patternGroups"].toArray();if(!groups.isEmpty() && groups.first().toObject()["type"]!="circular")return failure("pattern_owner_exists","This sketch already has a different Pattern association; edit its visible glyph or use a separate sketch");session_["newSketchPatternType"]="circular";showSketchPatternControls();return QJsonObject{{"visible",true}};});
}
void AlloyWindow::showSketchPatternControls(){
 if(sketchPatternDialog_){if(auto* dock=qobject_cast<QDockWidget*>(sketchPatternDialog_->parentWidget()))dock->raise();return;}
 const auto parameters=session_["draftSketch"].toObject()["parameters"].toObject();const auto groups=parameters["patternGroups"].toArray();const auto group=groups.isEmpty()?QJsonObject{}:groups.first().toObject();
 const bool circular=group.isEmpty() ? session_["newSketchPatternType"]=="circular" : group["type"]=="circular";
 const auto title=circular ? "Circular Pattern" : "Rectangular Pattern";
 auto* dock=new PatternDock(title,canvasWindow_);dock->setObjectName("sketch.pattern.dock");dock->setAttribute(Qt::WA_DeleteOnClose);
 auto* dialog=new QDialog(dock);dialog->setWindowFlags(Qt::Widget);sketchPatternDialog_=dialog;dialog->setObjectName("sketch.pattern.dialog");dialog->setWindowTitle(title);
 auto* layout=new QVBoxLayout(dialog);auto* form=new QFormLayout;layout->addLayout(form);
 auto* policy=new QLabel(circular ? "Full / Angle · positive sketch UV rotation\nQuantity includes the source circle" : "Spacing · One Direction\nQuantity includes the source circle");policy->setWordWrap(true);form->addRow(policy);
 auto* source=new QComboBox;source->setObjectName("sketch.pattern.source");auto* d1=new QComboBox;d1->setObjectName("sketch.pattern.direction1");auto* d2=new QComboBox;d2->setObjectName("sketch.pattern.direction2");
 auto* center=new QComboBox;center->setObjectName("sketch.pattern.center");
 auto* mode=new QComboBox;mode->setObjectName("sketch.pattern.mode");mode->addItem("Full","full");mode->addItem("Angle","angle");
 QSet<QString> generated;for(const auto& v:group["instances"].toArray())generated.insert(v.toObject()["entityId"].toString());
 for(const auto& v:parameters["entities"].toArray()){
  const auto e=v.toObject();const auto id=e["id"].toString();if(e["type"]=="circle" && !generated.contains(id))source->addItem("Circle · "+id.left(8),id);
  if(e["type"]=="point")center->addItem("Point · "+e["pointId"].toString().left(8),e["pointId"].toString());
  if(e["type"]=="line" && e["construction"].toBool()){d1->addItem("Line · "+id.left(8),id);d2->addItem("Line · "+id.left(8),id);}
 }
 if(!group.isEmpty()){source->setCurrentIndex(source->findData(group["sourceEntityId"].toString()));d1->setCurrentIndex(d1->findData(group["direction1EntityId"].toString()));d2->setCurrentIndex(d2->findData(group["direction2EntityId"].toString()));}else if(d2->count()>1)d2->setCurrentIndex(1);
 if(circular && !group.isEmpty()){center->setCurrentIndex(center->findData(group["centerPointId"].toString()));mode->setCurrentIndex(mode->findData(group["mode"].toString()));}
 form->addRow("Objects",source);if(circular){form->addRow("Center Point",center);form->addRow("Angular Spacing",mode);}else{form->addRow("Direction 1",d1);form->addRow("Direction 2",d2);}
 auto* q1=new QSpinBox;auto* q2=new QSpinBox;auto* s1=new QDoubleSpinBox;auto* s2=new QDoubleSpinBox;
 q1->setObjectName("sketch.pattern.quantity1");q2->setObjectName("sketch.pattern.quantity2");s1->setObjectName("sketch.pattern.spacing1");s2->setObjectName("sketch.pattern.spacing2");
 for(auto* q:{q1,q2})q->setRange(2,16);for(auto* s:{s1,s2}){s->setRange(.001,100000);s->setDecimals(3);s->setSuffix(" mm");}
 q1->setValue(group["quantity1"].toInt(2));q2->setValue(group["quantity2"].toInt(3));s1->setValue(group["spacing1Mm"].toDouble(14));s2->setValue(group["spacing2Mm"].toDouble(11));
 if(circular){q1->setObjectName("sketch.pattern.quantity");q1->setValue(group["quantity"].toInt(6));s1->setObjectName("sketch.pattern.angle");s1->setRange(.001,360);s1->setSuffix(QString::fromUtf8(" °"));s1->setValue(group["totalAngleDeg"].toDouble(360));s1->setEnabled(mode->currentData()=="angle");form->addRow("Quantity",q1);form->addRow("Total Angle",s1);}else{form->addRow("Quantity 1",q1);form->addRow("Distance 1",s1);form->addRow("Quantity 2",q2);form->addRow("Distance 2",s2);}
 if(circular){delete d1;delete d2;d1=nullptr;d2=nullptr;delete q2;delete s2;q2=nullptr;s2=nullptr;}else{delete center;delete mode;center=nullptr;mode=nullptr;}
 auto* status=new QLabel;status->setWordWrap(true);layout->addWidget(status);auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);layout->addWidget(buttons);buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
 const auto sessionId=session_["id"].toString();
 auto preview=[this,sessionId,source,d1,d2,q1,q2,s1,s2,status,buttons,group,circular,center,mode]{
  if(session_["id"].toString()!=sessionId){buttons->button(QDialogButtonBox::Ok)->setEnabled(false);status->setText("Sketch session ended");return;}
  QJsonObject a{{"sourceEntityId",source->currentData().toString()}};
  if(circular){a["type"]="circular";a["centerPointId"]=center->currentData().toString();a["quantity"]=q1->value();a["mode"]=mode->currentData().toString();a["totalAngleDeg"]=s1->value();}
  else{a["direction1EntityId"]=d1->currentData().toString();a["direction2EntityId"]=d2->currentData().toString();a["quantity1"]=q1->value();a["quantity2"]=q2->value();a["spacing1Mm"]=s1->value();a["spacing2Mm"]=s2->value();}if(!group.isEmpty())a["groupId"]=group["id"];
  const auto result=execute("sketch.patternPreview",a);const bool ok=!result.contains("error");buttons->button(QDialogButtonBox::Ok)->setEnabled(ok);status->setText(ok ? "Preview · source and derived circles" : result["error"].toObject()["message"].toString());
 };
 for(auto* combo:{source,d1,d2,center})if(combo)connect(combo,&QComboBox::currentIndexChanged,dialog,preview);
 for(auto* q:{q1,q2})if(q)connect(q,&QSpinBox::valueChanged,dialog,preview);for(auto* s:{s1,s2})if(s)connect(s,&QDoubleSpinBox::valueChanged,dialog,preview);
 if(mode)connect(mode,&QComboBox::currentIndexChanged,dialog,[mode,s1,preview]{const bool angle=mode->currentData()=="angle";s1->setEnabled(angle);if(!angle)s1->setValue(360);else if(s1->value()>=360)s1->setValue(120);preview();});
 connect(buttons,&QDialogButtonBox::accepted,dialog,[this,dialog,status,sessionId]{if(session_["id"].toString()!=sessionId){status->setText("Sketch session ended");return;}const auto result=execute("sketch.patternAccept");if(result.contains("error")){status->setText(result["error"].toObject()["message"].toString());return;}dialog->accept();});
 connect(buttons,&QDialogButtonBox::rejected,dialog,&QDialog::reject);connect(dialog,&QDialog::rejected,this,[this,sessionId]{if(session_["id"].toString()==sessionId)execute("sketch.patternCancel");});dock->setWidget(dialog);canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,dock);if(sketchDock_)canvasWindow_->tabifyDockWidget(sketchDock_,dock);
 connect(dialog,&QDialog::finished,dock,[dock]{dock->close();});dock->closed=[this,sessionId]{if(session_["id"].toString()==sessionId && session_.contains("pendingSketchPattern"))execute("sketch.patternCancel");if(sketchDock_)sketchDock_->raise();};
 auto* escape=new QShortcut(QKeySequence(Qt::Key_Escape),dialog);escape->setContext(Qt::WidgetWithChildrenShortcut);connect(escape,&QShortcut::activated,dialog,&QDialog::reject);
 dock->show();dock->raise();dialog->show();source->setFocus(Qt::OtherFocusReason);preview();
}
