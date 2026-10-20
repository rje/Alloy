// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <QCloseEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QShortcut>
#include <QVBoxLayout>
namespace {
QJsonObject failure(const QString& code,const QString& message){return {{"ok",false},{"error",QJsonObject{{"code",code},{"message",message}}}};}
class MoveDock final:public QDockWidget{public:using QDockWidget::QDockWidget;std::function<void()> closed;protected:void closeEvent(QCloseEvent* e)override{const auto callback=closed;if(callback)callback();QDockWidget::closeEvent(e);}};
}
void AlloyWindow::closeSketchMoveControls(){
 session_.remove("pendingSketchMove");session_.remove("moveInteraction");
 auto* dialog=sketchMoveDialog_.data();sketchMoveDialog_=nullptr;
 if(dialog){
  QObject::disconnect(dialog,SIGNAL(rejected()),dialog,nullptr);dialog->hide();
  if(auto* dock=dynamic_cast<MoveDock*>(dialog->parentWidget())){dock->closed={};dock->hide();dock->deleteLater();}else dialog->deleteLater();
 }
 if(session_.value("command")=="sketch.edit"){
  view_->showSketchDraft(evaluateSketchDraft(session_["draftSketch"].toObject()));refreshSketchControls();
  if(sketchDock_)sketchDock_->raise();view_->setFocus(Qt::OtherFocusReason);
 }
}
void AlloyWindow::syncSketchMoveControls(const QString& message){
 if(!sketchMoveDialog_)return;
 const auto interaction=session_["moveInteraction"].toObject();
 if(auto* label=sketchMoveDialog_->findChild<QLabel*>("sketch.move.selection"))label->setText(QString("%1 / 4 Lines selected · click edges to select or remove").arg(interaction["entityIds"].toArray().size()));
 if(auto* label=sketchMoveDialog_->findChild<QLabel*>("sketch.move.status"))label->setText(message.isEmpty() ? session_.contains("pendingSketchMove") ? "Preview · rigid translation" : "Select one isolated closed four-Line profile, then enter X/Y" : message);
 if(auto* buttons=sketchMoveDialog_->findChild<QDialogButtonBox*>())buttons->button(QDialogButtonBox::Ok)->setEnabled(session_.contains("pendingSketchMove"));
}
void AlloyWindow::registerSketchMoveCommands(){
 auto add=[this](const QString& id,const QJsonObject& schema,std::function<QJsonObject(const QJsonObject&)> run){commands_.push_back({id,id,"Atomic in-plane Move of one isolated closed four-Line profile",schema,true,std::move(run)});};
 const auto active=[this]{return session_.value("command")=="sketch.edit";};
 const QJsonObject number{{"type","number"},{"minimum",-100000},{"maximum",100000}};
 add("sketch.movePreview",{{"entityIds",QJsonObject{{"type","array"}}},{"x",number},{"y",number},{"units",QJsonObject{{"type","string"},{"enum",QJsonArray{"mm","in"}},{"required",false}}}},[this,active](const auto& a){
  if(!active())return failure("no_active_sketch","Enter an XY sketch first");
  const auto units=a["units"].toString(session_["inputs"].toObject()["units"].toString("mm"));
  const auto solved=SketchGeometry::translateProfile(session_["draftSketch"].toObject(),{{"entityIds",a["entityIds"]},{"deltaUMm",CadDocument::millimeters(a["x"].toDouble(),units)},{"deltaVMm",CadDocument::millimeters(a["y"].toDouble(),units)}},session_["projectionContext"].toObject());
  auto refuse=[this](const QString& code,const QString& message){session_.remove("pendingSketchMove");view_->showSketchDraft(evaluateSketchDraft(session_["draftSketch"].toObject()));syncSketchMoveControls(message);return failure(code,message);};
  if(!solved.ok)return refuse("sketch_move_failed",solved.error);
  if(!session_["inputs"].toObject()["sketchId"].toString().isEmpty())try{GeometryKernel::evaluateDocument(SketchGeometry::replaceFeature(document_->authored(),solved.feature));}catch(const std::exception& e){return refuse("sketch_move_dependency",QString::fromUtf8(e.what()));}
  session_["pendingSketchMove"]=QJsonObject{{"base",session_["draftSketch"]},{"draft",solved.feature},{"diagnostics",solved.diagnostics},{"entityIds",a["entityIds"]},{"deltaUMm",CadDocument::millimeters(a["x"].toDouble(),units)},{"deltaVMm",CadDocument::millimeters(a["y"].toDouble(),units)}};
  view_->showSketchDraft(evaluateSketchDraft(solved.feature));syncSketchMoveControls();return QJsonObject{{"pending",true},{"authoredChange",false},{"retained",false},{"diagnostics",solved.diagnostics},{"draft",solved.feature}};
 });
 add("sketch.moveAccept",{},[this,active](const auto&){if(!active())return failure("no_active_sketch","Enter a sketch first");const auto p=session_["pendingSketchMove"].toObject();if(p.isEmpty())return failure("move_preview_required","Preview a valid Move first");if(p["base"]!=session_["draftSketch"])return failure("stale_move_preview","The sketch changed; preview again");const auto result=publishSketchSolve({true,p["draft"].toObject(),p["diagnostics"].toObject(),{}});if(!result.contains("error")){auto selection=session_["selection"].toObject();selection["entityIds"]=p["entityIds"];selection["pointIds"]=QJsonArray{};selection["profileIds"]=QJsonArray{};session_["selection"]=selection;closeSketchMoveControls();}return result;});
 add("sketch.moveCancel",{},[this](const auto&){closeSketchMoveControls();return QJsonObject{{"authoredChange",false},{"cancelled",true}};});
 add("sketch.moveStart",{},[this,active](const auto&){if(!active())return failure("no_active_sketch","Enter an XY sketch first");if(session_["draftSketch"].toObject()["parameters"].toObject()["plane"]!="XY")return failure("move_plane_unsupported","Move supports XY sketch objects in this slice");showSketchMoveControls();return QJsonObject{{"visible",true},{"retained",false}};});
 add("sketch.movePick",{{"x",number},{"y",number}},[this,active](const auto& a){if(!active() || !session_["moveInteraction"].toObject()["active"].toBool())return failure("no_move_controls","Open Move/Copy first");const auto visible=session_.contains("pendingSketchMove") ? session_["pendingSketchMove"].toObject()["draft"].toObject() : session_["draftSketch"].toObject();const auto hit=view_->pickSketch(visible,a["x"].toDouble(),a["y"].toDouble(),"entity",session_["resolvedFrame"].toObject());const auto id=hit["entityId"].toString();QJsonObject edge;for(const auto& row:session_["draftSketch"].toObject()["parameters"].toObject()["entities"].toArray())if(row.toObject()["id"]==id)edge=row.toObject();if(id.isEmpty() || edge["type"]!="line" || edge["construction"].toBool())return failure("move_pick_unavailable","Click an ordinary authored Line from this sketch");auto interaction=session_["moveInteraction"].toObject();auto ids=interaction["entityIds"].toArray();if(ids.contains(id)){QJsonArray keep;for(const auto& value:ids)if(value!=id)keep.append(value);ids=keep;}else ids.append(id);interaction["entityIds"]=ids;session_["moveInteraction"]=interaction;session_.remove("pendingSketchMove");if(auto* x=sketchMoveDialog_ ? sketchMoveDialog_->findChild<QDoubleSpinBox*>("sketch.move.x") : nullptr)execute("sketch.movePreview",{{"entityIds",ids},{"x",x->value()},{"y",sketchMoveDialog_->findChild<QDoubleSpinBox*>("sketch.move.y")->value()},{"units",interaction["units"]}});syncSketchMoveControls();return QJsonObject{{"entityIds",ids}};});
 add("sketch.moveExit",{},[this](const auto&){closeSketchMoveControls();return QJsonObject{{"active",false}};});
}
void AlloyWindow::showSketchMoveControls(){
 if(sketchMoveDialog_)return;
 const auto sessionId=session_["id"].toString(),units=session_["inputs"].toObject()["units"].toString("mm");
 auto* dock=new MoveDock("Move/Copy",canvasWindow_);dock->setObjectName("sketch.move.dock");dock->setAttribute(Qt::WA_DeleteOnClose);
 auto* dialog=new QDialog(dock);dialog->setWindowFlags(Qt::Widget);dialog->setObjectName("sketch.move.dialog");sketchMoveDialog_=dialog;
 auto* layout=new QVBoxLayout(dialog);auto* form=new QFormLayout;layout->addLayout(form);form->addRow("Move Object",new QLabel("Sketch Objects"));form->addRow("Move Type",new QLabel("Translate"));
 auto* policy=new QLabel("Displacement from current position · XY plane");policy->setWordWrap(true);layout->addWidget(policy);
 auto* selection=new QLabel;selection->setWordWrap(true);selection->setObjectName("sketch.move.selection");layout->addWidget(selection);
 auto* x=new QDoubleSpinBox;auto* y=new QDoubleSpinBox;for(auto* spin:{x,y}){spin->setRange(-100000,100000);spin->setDecimals(3);spin->setSuffix(" "+units);}x->setObjectName("sketch.move.x");y->setObjectName("sketch.move.y");form->addRow("X Distance",x);form->addRow("Y Distance",y);auto* z=new QLineEdit("0 · out-of-plane unavailable");z->setEnabled(false);form->addRow("Z Distance",z);
 auto* status=new QLabel;status->setWordWrap(true);status->setObjectName("sketch.move.status");layout->addWidget(status);auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);layout->addWidget(buttons);
 session_["moveInteraction"]=QJsonObject{{"active",true},{"units",units},{"entityIds",session_["selection"].toObject()["entityIds"].toArray()}};
 auto preview=[this,x,y,units,sessionId]{if(session_["id"]!=sessionId)return;const auto result=execute("sketch.movePreview",{{"entityIds",session_["moveInteraction"].toObject()["entityIds"]},{"x",x->value()},{"y",y->value()},{"units",units}});syncSketchMoveControls(result["error"].toObject()["message"].toString());};
 connect(x,&QDoubleSpinBox::valueChanged,dialog,preview);connect(y,&QDoubleSpinBox::valueChanged,dialog,preview);
 connect(buttons,&QDialogButtonBox::accepted,dialog,[this,sessionId]{if(session_["id"]!=sessionId)return;const auto result=execute("sketch.moveAccept");if(result.contains("error")){syncSketchMoveControls(result["error"].toObject()["message"].toString());return;}});connect(buttons,&QDialogButtonBox::rejected,dialog,[this,sessionId]{if(session_["id"]==sessionId)execute("sketch.moveCancel");});
 connect(dialog,&QDialog::rejected,dialog,[this,sessionId]{if(session_["id"]==sessionId)closeSketchMoveControls();});
 dock->closed=[this,sessionId]{if(session_["id"]==sessionId)closeSketchMoveControls();};auto* escape=new QShortcut(QKeySequence(Qt::Key_Escape),dialog);escape->setContext(Qt::WidgetWithChildrenShortcut);connect(escape,&QShortcut::activated,dialog,[this,sessionId]{if(session_["id"]==sessionId)closeSketchMoveControls();});

 dock->setWidget(dialog);canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,dock);if(sketchDock_)canvasWindow_->tabifyDockWidget(sketchDock_,dock);dock->show();dock->raise();dialog->show();syncSketchMoveControls();view_->setFocus(Qt::OtherFocusReason);
}
