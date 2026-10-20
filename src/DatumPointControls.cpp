// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "CadDocument.h"
#include "DatumPoint.h"
#include "ReferenceView.h"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QShortcut>
#include <QSignalBlocker>
#include <QVBoxLayout>
#include <algorithm>
namespace {
QJsonObject failure(const QString& code,const QString& message){return {{"ok",false},{"error",QJsonObject{{"code",code},{"message",message}}}};}
}
QJsonObject AlloyWindow::beginPoint(const QJsonObject& overrides) {
 if(!session_.isEmpty())return failure("session_busy","Complete or cancel the active command first");
 QJsonObject inputs{{"name","Point"}};
 if(overrides.contains("featureId")){
  bool found=false;for(const auto& value:document_->authored().value("features").toArray()){const auto row=value.toObject();if(row.value("id")!=overrides.value("featureId"))continue;
   if(row.value("type")!="constructionPoint")return failure("stale_entity","Point does not exist");
   inputs=row.value("parameters").toObject();inputs["featureId"]=row.value("id");inputs["name"]=row.value("name");found=true;break;}
  if(!found)return failure("stale_entity","Point does not exist");
 }
 for(auto it=overrides.begin();it!=overrides.end();++it)inputs[it.key()]=it.value();
 session_={{"id",CadDocument::newId()},{"command","construction.point"},{"status","active"},{"inputs",inputs},{"previewed",false},
  {"baseDocumentRevision",static_cast<qint64>(document_->revision())},{"inputSchema",findCommand("construction.point")->parameters}};
 showPoint();previewPoint();return session_;
}
QJsonObject AlloyWindow::previewPoint() {
 if(session_.value("command")!="construction.point")return failure("invalid_session","Point is not active");
 const auto inputs=session_.value("inputs").toObject(),source=inputs.value("source").toObject(),location=inputs.value("location").toObject();
 auto* status=pointDock_?pointDock_->findChild<QLabel*>("point.status"):nullptr;
 auto* buttons=pointDock_?pointDock_->findChild<QDialogButtonBox*>("point.buttons"):nullptr;
 const bool line=source.contains("line"),physical=location.value("mode")=="physical";
 if(pointDock_){
  for(const auto& key:{"method","mode"})if(auto* combo=pointDock_->findChild<QComboBox*>(QString("point.")+key)){const QSignalBlocker block(combo);combo->setCurrentIndex(QString(key)=="method"?(line?1:0):(physical?1:0));if(QString(key)=="mode")combo->setEnabled(line);}
  if(auto* field=pointDock_->findChild<QDoubleSpinBox*>("point.value")){const QSignalBlocker block(field);field->setEnabled(line);field->setMaximum(physical?100000:1);const auto units=document_->authored().value("displayUnits").toString();field->setSuffix(physical?" "+units:"");field->setValue(physical?location.value("distanceMm").toDouble()/CadDocument::millimeters(1,units):location.value("ratio").toDouble(.5));}
  for(const auto& key:{"vertex","line"})if(auto* combo=pointDock_->findChild<QComboBox*>(QString("point.")+key)){
   combo->setEnabled((QString(key)=="line")==line);const auto wanted=source.value(key).toObject();const QSignalBlocker block(combo);combo->setCurrentIndex(0);bool found=wanted.isEmpty();
   for(int i=1;i<combo->count();++i)if(combo->itemData(i).toJsonObject()==wanted){combo->setCurrentIndex(i);found=true;break;}
   if(!found){combo->addItem("Unavailable retained source",wanted);combo->setCurrentIndex(combo->count()-1);}
  }
 }
 auto points=document_->evaluation().constructionPoints;
 const auto clear=[&](const QString& text){session_["previewed"]=false;session_.remove("previewPoint");session_.remove("previewError");view_->setConstructionPoints(points,++presentationRevision_);if(status)status->setText(text);if(buttons)buttons->button(QDialogButtonBox::Ok)->setEnabled(false);};
 if(session_.value("baseDocumentRevision").toInteger()!=static_cast<qint64>(document_->revision())){clear("Document changed. Cancel and restart Point.");return failure("stale_session","Document changed. Cancel and restart Point");}
 if(source.isEmpty()){clear("Select one Box edge endpoint or a straight sketch Line.");return session_;}
 try {
  const auto edit=inputs.value("featureId").toString();const auto& evaluation=edit.isEmpty()?document_->evaluation():document_->evaluationBefore(edit);
  QJsonObject parameters{{"source",source}};if(inputs.contains("location"))parameters["location"]=inputs.value("location");
  const auto point=DatumPoint::evaluate(evaluation,parameters,document_->authored().value("features").toArray());
  for(auto it=points.begin();it!=points.end();++it)if(it->featureId==edit){points.erase(it);break;}
  points.push_back({"point-preview","Point Preview",true,point,document_->activeOwnerId()});view_->setConstructionPoints(points,++presentationRevision_);
  session_["previewed"]=true;session_["previewPoint"]=point;session_.remove("previewError");
  if(status){const auto xyz=point.value("positionMm").toArray();status->setText(QString("Position: %1, %2, %3 mm\n%4").arg(xyz[0].toDouble(),0,'g',8).arg(xyz[1].toDouble(),0,'g',8).arg(xyz[2].toDouble(),0,'g',8).arg(line?"Authored start → end (Alloy policy). Finite segment only.":"Exact current semantic Box edge endpoint."));}
  if(buttons)buttons->button(QDialogButtonBox::Ok)->setEnabled(true);return session_;
 }catch(const GeometryKernel::EvaluationError& error){clear(QString::fromUtf8(error.what()));session_["previewError"]=error.diagnostics();return failure(error.diagnostics().value("code").toString(),QString::fromUtf8(error.what()));}
 catch(const std::exception& error){clear(QString::fromUtf8(error.what()));return failure("point_source_unavailable",QString::fromUtf8(error.what()));}
}
void AlloyWindow::showPoint(){
 if(pointDock_)delete pointDock_;pointDock_=new QDockWidget("Point",canvasWindow_);pointDock_->setObjectName("pointDock");pointDock_->setAllowedAreas(Qt::LeftDockWidgetArea|Qt::RightDockWidgetArea);
 auto* panel=new QWidget(pointDock_);panel->setFixedWidth(304);pointDock_->setWidget(panel);auto* layout=new QVBoxLayout(panel);layout->setContentsMargins(14,14,14,12);layout->setSpacing(12);
 auto* hint=new QLabel("Create one retained reference point from a Box vertex or an authored straight sketch Line.",panel);hint->setWordWrap(true);layout->addWidget(hint);
 auto* method=new QComboBox(panel);method->setObjectName("point.method");method->addItems({"At Vertex","Along Path"});
 auto* vertex=new QComboBox(panel);vertex->setObjectName("point.vertex");auto* line=new QComboBox(panel);line->setObjectName("point.line");vertex->addItem("Select Box endpoint",QJsonObject{});line->addItem("Select Line",QJsonObject{});
 const auto edit=session_.value("inputs").toObject().value("featureId").toString();const auto& evaluation=edit.isEmpty()?document_->evaluation():document_->evaluationBefore(edit);
 for(const auto& body:evaluation.bodies)for(const auto& edge:body.solidEdges)if(edge.provenance.value("role")=="box.edge" && edge.provenance.value("endpointProvenance")=="currentKernelBoundary")for(int endpoint=0;endpoint<2;++endpoint)vertex->addItem(body.name+" · "+edge.provenance.value("boxEdge").toString()+QString(" · %1").arg(endpoint),QJsonObject{{"bodyId",body.bodyId},{"edgeId",edge.edgeId},{"endpoint",endpoint}});
 for(const auto& value:document_->authored().value("features").toArray()){const auto sketch=value.toObject();if(sketch.value("id")==edit)break;if(sketch.value("type")!="sketch")continue;
  for(const auto& item:sketch.value("parameters").toObject().value("entities").toArray()){const auto entity=item.toObject();if(entity.value("type")!="line")continue;
   const QJsonObject ref{{"sketchId",sketch.value("id")},{"entityId",entity.value("id")},{"startPointId",entity.value("startPointId")},{"endPointId",entity.value("endPointId")}};
   line->addItem(sketch.value("name").toString()+" · "+entity.value("id").toString().left(8),ref);
  }
 }
 auto* mode=new QComboBox(panel);mode->setObjectName("point.mode");mode->addItems({"Proportional","Physical"});auto* value=new QDoubleSpinBox(panel);value->setObjectName("point.value");value->setDecimals(8);value->setRange(0,1);value->setSingleStep(.1);value->setValue(.5);value->setKeyboardTracking(false);
 auto* form=new QFormLayout;form->addRow("Type",new QLabel("Point",panel));form->addRow("Method",method);form->addRow("Vertex",vertex);form->addRow("Path",line);form->addRow("Extent Type",mode);form->addRow("Distance",value);layout->addLayout(form);
 auto update=[this,method,vertex,line,mode,value]{QJsonObject updates;
  if(method->currentIndex()==0)updates["source"]=QJsonObject{{"vertex",vertex->currentData().toJsonObject()}};
  else{updates["source"]=QJsonObject{{"line",line->currentData().toJsonObject()}};updates["location"]=mode->currentIndex()==0?QJsonObject{{"mode","proportional"},{"ratio",value->value()}}:QJsonObject{{"mode","physical"},{"distanceMm",CadDocument::millimeters(value->value(),document_->authored().value("displayUnits").toString())}};}
  execute("command.update",{{"session",session_.value("id")},{"inputs",updates}});};
 for(auto* combo:{method,vertex,line})connect(combo,&QComboBox::currentIndexChanged,this,update);
 connect(mode,&QComboBox::currentIndexChanged,this,[this,mode,value,update]{
  const auto preview=session_.value("previewPoint").toObject();const auto previous=session_.value("inputs").toObject().value("location").toObject();
  const double length=preview.value("sourceLengthMm").toDouble();const bool physical=mode->currentIndex()==1;
  const double ratio=previous.value("mode")=="physical"?(length>0?previous.value("distanceMm").toDouble()/length:0):previous.value("ratio").toDouble(.5);
  {const QSignalBlocker block(value);value->setMaximum(physical?100000:1);value->setValue(physical?ratio*length/CadDocument::millimeters(1,document_->authored().value("displayUnits").toString()):std::clamp(ratio,0.,1.));}
  update();
 });connect(value,&QDoubleSpinBox::editingFinished,this,update);
 auto* status=new QLabel(panel);status->setObjectName("point.status");status->setWordWrap(true);status->setMinimumHeight(90);layout->addWidget(status);layout->addStretch();
 auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,panel);buttons->setObjectName("point.buttons");layout->addWidget(buttons);
 connect(buttons,&QDialogButtonBox::accepted,this,[this,status]{const auto r=execute("command.commit",{{"session",session_.value("id")}});if(!r.value("ok").toBool())status->setText(r.value("error").toObject().value("message").toString());});
 auto cancel=[this]{if(session_.value("command")=="construction.point")execute("command.cancel",{{"session",session_.value("id")}});};connect(buttons,&QDialogButtonBox::rejected,this,cancel);auto* escape=new QShortcut(QKeySequence(Qt::Key_Escape),panel);escape->setContext(Qt::WindowShortcut);connect(escape,&QShortcut::activated,this,cancel);
 canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,pointDock_);pointDock_->show();view_->setFocus(Qt::OtherFocusReason);
}
