// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "CadDocument.h"
#include "DatumAxis.h"
#include "ReferenceView.h"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QShortcut>
#include <QSignalBlocker>
#include <QVBoxLayout>
namespace {
QJsonObject failure(const QString& code,const QString& message,const QString& field={}){QJsonObject error{{"code",code},{"message",message}};if(!field.isEmpty())error["field"]=field;return {{"ok",false},{"error",error}};}
}
QJsonObject AlloyWindow::beginAxis(const QJsonObject& overrides) {
 if(!session_.isEmpty())return failure("session_busy","Complete or cancel the active command first");
 QJsonObject inputs{{"name","Axis"}};
 if(overrides.contains("featureId")){
  bool found=false;for(const auto& value:document_->authored().value("features").toArray()){const auto row=value.toObject();if(row.value("id")!=overrides.value("featureId"))continue;
   if(row.value("type")!="constructionAxis")return failure("stale_entity","Axis does not exist","featureId");
   inputs={{"featureId",row.value("id")},{"name",row.value("name")},{"source",row.value("parameters").toObject().value("source")}};found=true;break;}
  if(!found)return failure("stale_entity","Axis does not exist","featureId");
 }
 for(auto it=overrides.begin();it!=overrides.end();++it)inputs[it.key()]=it.value();
 session_={{"id",CadDocument::newId()},{"command","construction.axis"},{"status","active"},{"inputs",inputs},{"previewed",false},
  {"baseDocumentRevision",static_cast<qint64>(document_->revision())},{"inputSchema",findCommand("construction.axis")->parameters}};
 showAxis();previewAxis();return session_;
}
QJsonObject AlloyWindow::previewAxis() {
 if(session_.value("command")!="construction.axis")return failure("invalid_session","Axis is not active");
 const auto inputs=session_.value("inputs").toObject(),source=inputs.value("source").toObject();
 auto* status=axisDock_?axisDock_->findChild<QLabel*>("axis.status"):nullptr;
 auto* buttons=axisDock_?axisDock_->findChild<QDialogButtonBox*>("axis.buttons"):nullptr;
 if(axisDock_){
  const bool line=source.contains("line");
  if(auto* method=axisDock_->findChild<QComboBox*>("axis.method")){const QSignalBlocker block(method);method->setCurrentIndex(line?1:0);}
  for(const auto& key:{"point1","point2","line"})if(auto* combo=axisDock_->findChild<QComboBox*>(QString("axis.")+key)){
   const bool isLine=QString(key)=="line";combo->setEnabled(isLine==line);
   QJsonObject wanted;if(isLine)wanted=source.value("line").toObject();else {const auto refs=source.value("points").toArray();const int index=QString(key)=="point1"?0:1;if(refs.size()>index)wanted=refs[index].toObject();}
   const QSignalBlocker block(combo);combo->setCurrentIndex(0);bool found=wanted.isEmpty();
   for(int i=1;i<combo->count();++i)if(combo->itemData(i).toJsonObject()==wanted){combo->setCurrentIndex(i);found=true;break;}
   if(!found){combo->addItem("Unavailable retained source · "+wanted.value("entityId").toString().left(8),wanted);combo->setCurrentIndex(combo->count()-1);}
  }
 }
 auto axes=document_->evaluation().constructionAxes;
 const auto clear=[&](const QString& message){session_["previewed"]=false;session_.remove("previewAxis");view_->setConstructionAxes(axes,++presentationRevision_);if(status)status->setText(message);if(buttons)buttons->button(QDialogButtonBox::Ok)->setEnabled(false);};
 if(session_.value("baseDocumentRevision").toInteger()!=static_cast<qint64>(document_->revision())){clear("Document changed. Cancel and restart Axis.");return failure("stale_session","Document changed. Cancel and restart Axis");}
 if(source.isEmpty()){clear("Choose two standalone sketch Points or one straight sketch Line.");return session_;}
 try {
  const auto edit=inputs.value("featureId").toString();const auto& evaluation=edit.isEmpty()?document_->evaluation():document_->evaluationBefore(edit);
  const auto line=DatumAxis::evaluate(evaluation,source,document_->authored().value("features").toArray());
  for(auto it=axes.begin();it!=axes.end();++it)if(it->featureId==edit){axes.erase(it);break;}
  axes.push_back({"axis-preview","Axis Preview",true,line,document_->activeOwnerId()});view_->setConstructionAxes(axes,++presentationRevision_);
  session_["previewed"]=true;session_["previewAxis"]=line;session_.remove("previewError");
  if(status){const auto o=line.value("originMm").toArray(),d=line.value("direction").toArray();status->setText(QString("Origin: %1, %2, %3 mm\nDirection: %4, %5, %6\nFirst to second (Alloy policy). Display extends 15 mm beyond sources.")
   .arg(o[0].toDouble(),0,'g',6).arg(o[1].toDouble(),0,'g',6).arg(o[2].toDouble(),0,'g',6).arg(d[0].toDouble(),0,'g',5).arg(d[1].toDouble(),0,'g',5).arg(d[2].toDouble(),0,'g',5));}
  if(buttons)buttons->button(QDialogButtonBox::Ok)->setEnabled(true);return session_;
 }catch(const GeometryKernel::EvaluationError& error){clear(QString::fromUtf8(error.what()));session_["previewError"]=error.diagnostics();return failure(error.diagnostics().value("code").toString(),QString::fromUtf8(error.what()));}
 catch(const std::exception& error){clear(QString::fromUtf8(error.what()));return failure("axis_source_unavailable",QString::fromUtf8(error.what()));}
}
void AlloyWindow::showAxis(){
 if(axisDock_)delete axisDock_;axisDock_=new QDockWidget("Axis",canvasWindow_);axisDock_->setObjectName("axisDock");axisDock_->setAllowedAreas(Qt::LeftDockWidgetArea|Qt::RightDockWidgetArea);
 auto* panel=new QWidget(axisDock_);panel->setFixedWidth(304);axisDock_->setWidget(panel);auto* layout=new QVBoxLayout(panel);layout->setContentsMargins(14,14,14,12);layout->setSpacing(12);
 auto* hint=new QLabel("Create reference geometry through two authored standalone sketch Points or one straight sketch Line.",panel);hint->setWordWrap(true);layout->addWidget(hint);
 auto* method=new QComboBox(panel);method->setObjectName("axis.method");method->addItem("Axis Through Two Points");method->addItem("Axis Through Edge · sketch Line");
 auto* p1=new QComboBox(panel);p1->setObjectName("axis.point1");auto* p2=new QComboBox(panel);p2->setObjectName("axis.point2");auto* line=new QComboBox(panel);line->setObjectName("axis.line");
 for(auto* combo:{p1,p2,line}){combo->addItem(combo==line?"Select Line":"Select Point",QJsonObject{});combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);}
 const auto edit=session_.value("inputs").toObject().value("featureId").toString();
 for(const auto& value:document_->authored().value("features").toArray()){const auto sketch=value.toObject();if(sketch.value("id")==edit)break;if(sketch.value("type")!="sketch")continue;
  for(const auto& item:sketch.value("parameters").toObject().value("entities").toArray()){const auto entity=item.toObject();const auto type=entity.value("type").toString();
   QJsonObject ref{{"sketchId",sketch.value("id")},{"entityId",entity.value("id")}};
   const auto label=sketch.value("name").toString()+" · "+entity.value("id").toString().left(8);
   if(type=="point"){ref["pointId"]=entity.value("pointId");p1->addItem(label,ref);p2->addItem(label,ref);}
   if(type=="line"){ref["startPointId"]=entity.value("startPointId");ref["endPointId"]=entity.value("endPointId");line->addItem(label,ref);}
  }
 }
 auto* form=new QFormLayout;form->addRow("Type",method);form->addRow("Point 1",p1);form->addRow("Point 2",p2);form->addRow("Line",line);layout->addLayout(form);
 auto update=[this,method,p1,p2,line]{QJsonObject source;if(method->currentIndex()==0)source={{"points",QJsonArray{p1->currentData().toJsonObject(),p2->currentData().toJsonObject()}}};else source={{"line",line->currentData().toJsonObject()}};
  execute("command.update",{{"session",session_.value("id")},{"inputs",QJsonObject{{"source",source}}}});};
 for(auto* combo:{method,p1,p2,line})connect(combo,&QComboBox::currentIndexChanged,this,update);
 auto* status=new QLabel(panel);status->setObjectName("axis.status");status->setWordWrap(true);status->setMinimumHeight(90);layout->addWidget(status);layout->addStretch();
 auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,panel);buttons->setObjectName("axis.buttons");layout->addWidget(buttons);
 connect(buttons,&QDialogButtonBox::accepted,this,[this,status]{const auto r=execute("command.commit",{{"session",session_.value("id")}});if(!r.value("ok").toBool())status->setText(r.value("error").toObject().value("message").toString());});
 auto cancel=[this]{if(session_.value("command")=="construction.axis")execute("command.cancel",{{"session",session_.value("id")}});};connect(buttons,&QDialogButtonBox::rejected,this,cancel);auto* escape=new QShortcut(QKeySequence(Qt::Key_Escape),panel);escape->setContext(Qt::WindowShortcut);connect(escape,&QShortcut::activated,this,cancel);
 canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,axisDock_);axisDock_->show();view_->setFocus(Qt::OtherFocusReason);
}
