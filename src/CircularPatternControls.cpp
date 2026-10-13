// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include <algorithm>
#include "ReferenceView.h"
#include <QDockWidget>
#include <QFormLayout>
#include <QPushButton>
#include <QSpinBox>
#include <QComboBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <QShortcut>
#include <QSignalBlocker>
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>
namespace {QJsonObject reject(const QString& c,const QString& m){return {{"ok",false},{"error",QJsonObject{{"code",c},{"message",m}}}};}}
QJsonObject AlloyWindow::circularPatternCandidate(const QJsonObject& inputs) const {auto args=inputs;if(!args.contains("featureId")&&session_.value("command")=="solid.circularPattern")args["newFeatureId"]=session_["newFeatureId"];return CadDocument::circularPatternCandidate(document_->authored(),authoringArguments(args));}
std::vector<EvaluatedBody> AlloyWindow::circularInputBodies() const {const auto edit=session_.value("inputs").toObject()["featureId"].toString();return edit.isEmpty()?document_->bodies():document_->evaluationBefore(edit).bodies;}
QJsonObject AlloyWindow::beginCircularPattern(const QJsonObject& overrides){
 if(!session_.isEmpty())return reject("session_busy","Complete or cancel the active command first");
 QJsonObject inputs{{"sourceBodyId",selectedBody_},{"axis",QJsonObject{}},{"extent","full"},{"quantity",3},{"name","Circular Pattern"}};
 if(overrides.contains("featureId")){bool found=false;for(const auto& v:document_->authored()["features"].toArray()){const auto f=v.toObject();if(f["id"]!=overrides["featureId"])continue;if(f["type"]!="circularPattern")return reject("pattern_unavailable","Circular Pattern feature unavailable");inputs=f["parameters"].toObject();inputs["featureId"]=f["id"];inputs["name"]=f["name"];found=true;}if(!found)return reject("pattern_unavailable","Circular Pattern feature unavailable");}
 for(auto it=overrides.begin();it!=overrides.end();++it)inputs[it.key()]=it.value();const auto error=validateArguments(*findCommand("solid.circularPattern"),inputs);if(!error.isEmpty())return reject("invalid_arguments",error);
 session_={{"id",CadDocument::newId()},{"command","solid.circularPattern"},{"status","active"},{"inputs",inputs},{"newFeatureId",CadDocument::newId()},{"selectionRole",inputs["sourceBodyId"].toString().isEmpty()?"objects":"axis"},{"baseDocumentRevision",qint64(document_->revision())},{"previewed",false},{"geometryEvaluated",false},{"inputSchema",findCommand("solid.circularPattern")->parameters}};
 if(inputs.contains("featureId"))view_->setBodyStage(circularInputBodies(),"circular-source:"+session_["id"].toString());showCircularPattern();previewCircularPattern();view_->setFocus();return session_;
}
QJsonObject AlloyWindow::selectCircularBody(const QString& id){
 const auto bodies=circularInputBodies();bool found=false;for(const auto& b:bodies)found|=b.bodyId==id;
 if(!found)return reject("pattern_body_unavailable","Select a current body before the pattern");
 auto response=execute("command.update",{{"session",session_["id"]},{"inputs",QJsonObject{{"sourceBodyId",id}}}});if(!response["ok"].toBool())return response;session_["selectionRole"]="axis";view_->selectBody(id);return previewCircularPattern();
}
QJsonObject AlloyWindow::previewCircularPattern(){
 session_["previewed"]=false;session_["geometryEvaluated"]=false;session_.remove("geometry");session_.remove("documentDraft");view_->clearPreview();view_->clearPatternAxes();
 auto failure=[&](const QString& c,const QString& m){const auto r=reject(c,m);session_["previewError"]=r["error"];refreshCircularControls();return r;};
 if(session_["baseDocumentRevision"].toInteger()!=qint64(document_->revision()))return failure("stale_session","The document changed; restart Circular Pattern");
 const auto inputs=session_["inputs"].toObject();if(inputs["sourceBodyId"].toString().isEmpty())return failure("body_required","Select one body to pattern");if(inputs["axis"].toObject().isEmpty())return failure("axis_required","Select Origin Z");
 QJsonObject candidate;try{candidate=circularPatternCandidate(inputs);}catch(const std::exception& e){return failure("invalid_arguments",QString::fromUtf8(e.what()));}const auto evaluated=CadDocument::evaluateCandidate(candidate);if(!evaluated.ok)return failure("evaluation_failed",evaluated.error);
 const auto featureId=inputs.contains("featureId")?inputs["featureId"].toString():session_["newFeatureId"].toString();auto stage=evaluated.evaluation;
 if(inputs.contains("featureId")){QJsonArray prefix;for(const auto& v:candidate["features"].toArray()){prefix.append(v);if(v.toObject()["id"]==featureId)break;}auto draft=candidate;draft["features"]=prefix;const auto early=CadDocument::evaluateCandidate(draft);if(!early.ok)return failure("evaluation_failed",early.error);stage=early.evaluation;}
 TopoDS_Compound copies;BRep_Builder builder;builder.MakeCompound(copies);QJsonArray occurrences;std::vector<EvaluatedBody> bodies;
 for(const auto& b:stage->bodies)if(b.summary["circularPattern"].toObject()["featureId"]==featureId){auto details=b.summary;details["bodyId"]=b.bodyId;occurrences.append(details);bodies.push_back(b);if(b.bodyId!=inputs["sourceBodyId"])builder.Add(copies,b.shape);}
 if(bodies.size()!=size_t(inputs["quantity"].toInt()))return failure("occurrences_unavailable","Pattern occurrences are unavailable at this history stage");
 view_->showPreview(copies,{},true);view_->selectBody(inputs["sourceBodyId"].toString());if(!session_["previewFitted"].toBool()){view_->fit(bodies,{});session_["previewFitted"]=true;}
 const auto bounds=bodies[0].summary["boundsMm"].toObject();const auto low=bounds["min"].toArray(),high=bounds["max"].toArray();const double start=std::min(0.,low[2].toDouble())-2,span=std::max(10.,high[2].toDouble()-start+2);
 view_->showPatternAxes({{"seedCenterMm",QJsonArray{0,0,start}},{"patternAxes",QJsonArray{QJsonObject{{"direction",QJsonArray{0,0,1}},{"totalSpanMm",span}}}}});
 session_["documentDraft"]=candidate;session_["geometry"]=QJsonObject{{"featureId",featureId},{"sourceBodyId",inputs["sourceBodyId"]},{"axis",inputs["axis"]},{"quantity",inputs["quantity"]},{"extent","full"},{"occurrences",occurrences}};session_["previewed"]=true;session_["geometryEvaluated"]=true;session_.remove("previewError");refreshCircularControls();return session_;
}
void AlloyWindow::refreshCircularControls(){
 if(!circularDock_)return;const auto inputs=session_["inputs"].toObject();if(auto* object=circularDock_->findChild<QPushButton*>("circular.objects"))object->setText(inputs["sourceBodyId"].toString().isEmpty()?"Select body":"1 selected");
 if(auto* axis=circularDock_->findChild<QComboBox*>("circular.axis")){const QSignalBlocker block(axis);axis->setCurrentIndex(inputs["axis"].toObject().isEmpty()?0:1);}if(auto* q=circularDock_->findChild<QSpinBox*>("circular.quantity")){const QSignalBlocker block(q);q->setValue(inputs["quantity"].toInt(3));}
 if(auto* error=circularDock_->findChild<QLabel*>("circular.error"))error->setText(session_.value("previewError").toObject()["message"].toString());if(auto* buttons=circularDock_->findChild<QDialogButtonBox*>())buttons->button(QDialogButtonBox::Ok)->setEnabled(session_["previewed"].toBool()&&session_["geometryEvaluated"].toBool());
}
void AlloyWindow::showCircularPattern(){
 if(circularDock_){canvasWindow_->removeDockWidget(circularDock_);delete circularDock_;}circularDock_=new QDockWidget("Circular Pattern",canvasWindow_);circularDock_->setObjectName("circularPatternDock");circularDock_->setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);auto* panel=new QWidget(circularDock_);circularDock_->setWidget(panel);auto* form=new QFormLayout(panel);
 form->addRow("Pattern Type",new QLabel("Bodies"));auto* object=new QPushButton;object->setObjectName("circular.objects");form->addRow("Objects",object);connect(object,&QPushButton::clicked,this,[this]{session_["selectionRole"]="objects";view_->setFocus();});auto* axis=new QComboBox;axis->setObjectName("circular.axis");axis->addItem("Select axis");axis->addItem("Origin Z");form->addRow("Axis",axis);connect(axis,qOverload<int>(&QComboBox::activated),this,[this](int i){const auto r=execute("command.update",{{"session",session_["id"]},{"inputs",QJsonObject{{"axis",i?QJsonObject{{"kind","originAxis"},{"axis","Z"}}:QJsonObject{}}}}});if(r["ok"].toBool())previewCircularPattern();view_->setFocus();});
 form->addRow("Type",new QLabel("Full"));auto* quantity=new QSpinBox;quantity->setObjectName("circular.quantity");quantity->setRange(2,128);quantity->setKeyboardTracking(false);form->addRow("Quantity",quantity);connect(quantity,qOverload<int>(&QSpinBox::valueChanged),this,[this](int n){const auto r=execute("command.update",{{"session",session_["id"]},{"inputs",QJsonObject{{"quantity",n}}}});if(r["ok"].toBool())previewCircularPattern();});auto* note=new QLabel("Total includes the original body");note->setWordWrap(true);form->addRow(note);auto* error=new QLabel;error->setObjectName("circular.error");error->setWordWrap(true);form->addRow(error);auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);form->addRow(buttons);connect(buttons,&QDialogButtonBox::accepted,this,[this]{execute("command.commit",{{"session",session_["id"]}});});connect(buttons,&QDialogButtonBox::rejected,this,[this]{execute("command.cancel",{{"session",session_["id"]}});});auto* escape=new QShortcut(QKeySequence(Qt::Key_Escape),panel);escape->setContext(Qt::WidgetWithChildrenShortcut);connect(escape,&QShortcut::activated,this,[this]{execute("command.cancel",{{"session",session_["id"]}});});if(!circularEscape_){circularEscape_=new QShortcut(QKeySequence(Qt::Key_Escape),view_);circularEscape_->setContext(Qt::WidgetShortcut);connect(circularEscape_,&QShortcut::activated,this,[this]{if(session_.value("command")=="solid.circularPattern")execute("command.cancel",{{"session",session_["id"]}});});}circularEscape_->setEnabled(true);canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,circularDock_);circularDock_->show();refreshCircularControls();
}
