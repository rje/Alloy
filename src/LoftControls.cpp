// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <QDockWidget>
#include <QFormLayout>
#include <QListWidget>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QShortcut>
#include <QSignalBlocker>
namespace {
QJsonObject failure(const QString& code,const QString& message){return {{"ok",false},{"error",QJsonObject{{"code",code},{"message",message}}}};}
}

QJsonObject AlloyWindow::loftCandidate(const QJsonObject& inputs) const {
    auto args=inputs;if(!args.contains("featureId") && session_.value("command")=="solid.loft") {args["newFeatureId"]=session_["newFeatureId"];args["newBodyId"]=session_["newBodyId"];}
    return CadDocument::loftCandidate(document_->authored(),args);
}
QJsonObject AlloyWindow::beginLoft(const QJsonObject& overrides) {
    if(!session_.isEmpty())return failure("session_busy","Complete or cancel the active command first");
    auto inputs=QJsonObject{{"profiles",QJsonArray{}},{"name","Loft"}};
    if(overrides.contains("featureId")) {
        bool found=false;for(const auto& entry:document_->authored()["features"].toArray()) {const auto f=entry.toObject();if(f["id"]!=overrides["featureId"])continue;if(f["type"]!="loft")return failure("stale_entity","Loft feature unavailable");inputs=f["parameters"].toObject();inputs["name"]=f["name"];inputs["featureId"]=f["id"];found=true;}
        if(!found)return failure("stale_entity","Loft feature unavailable");
    } else if(!selectedProfile_.isEmpty())inputs["profiles"]=QJsonArray{QJsonObject{{"sketchId",selectedSketch_},{"profileId",selectedProfile_}}};
    for(auto it=overrides.begin();it!=overrides.end();++it)inputs[it.key()]=it.value();
    const auto error=validateArguments(*findCommand("solid.loft"),inputs);if(!error.isEmpty())return failure("invalid_arguments",error);
    session_={{"id",CadDocument::newId()},{"command","solid.loft"},{"status","active"},{"inputs",inputs},
        {"newFeatureId",CadDocument::newId()},{"newBodyId",CadDocument::newId()},{"baseDocumentRevision",static_cast<qint64>(document_->revision())},{"previewed",false},{"geometryEvaluated",false},{"inputSchema",findCommand("solid.loft")->parameters}};
    if(inputs.contains("featureId"))view_->setBodyStage(document_->evaluationBefore(inputs["featureId"].toString()).bodies,"loft-source:"+session_["id"].toString());
    showLoft();previewLoft();view_->setFocus();return session_;
}
QJsonObject AlloyWindow::previewLoft() {
    view_->selectLoftProfiles(session_["inputs"].toObject()["profiles"].toArray());
    session_["previewed"]=false;session_["geometryEvaluated"]=false;session_.remove("geometry");session_.remove("documentDraft");view_->clearPreview();
    auto reject=[this](const QString& code,const QString& message,const QJsonObject& diagnostics=QJsonObject{}){auto r=failure(code,message);auto e=r["error"].toObject();e["diagnostics"]=diagnostics;r["error"]=e;session_["previewError"]=e;if(loftDock_)if(auto* label=loftDock_->findChild<QLabel*>("loftError"))label->setText(message);return r;};
    if(loftDock_) {
        auto* list=loftDock_->findChild<QListWidget*>("loftProfiles");const QSignalBlocker blocker(list);list->clear();
        int index=0;for(const auto& value:session_["inputs"].toObject()["profiles"].toArray()){const auto ref=value.toObject();QString name=ref["sketchId"].toString();for(const auto& s:document_->sketches())if(s.featureId==name){name=s.name;break;}list->addItem(QString::number(++index)+". "+name);}
        loftDock_->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
    if(session_["baseDocumentRevision"].toInteger()!=static_cast<qint64>(document_->revision()))return reject("stale_session","The document changed; cancel and restart Loft");
    auto candidate=loftCandidate(session_["inputs"].toObject());const auto evaluated=CadDocument::evaluateCandidate(candidate);if(!evaluated.ok)return reject("evaluation_failed",evaluated.error,evaluated.diagnostics);
    const auto id=session_["inputs"].toObject().value("featureId").toString(session_["newFeatureId"].toString());
    auto stage=evaluated.evaluation;
    if(session_["inputs"].toObject().contains("featureId")){QJsonArray fs;for(const auto& f:candidate["features"].toArray()){fs.append(f);if(f.toObject()["id"]==id)break;}auto prefix=candidate;prefix["features"]=fs;const auto early=CadDocument::evaluateCandidate(prefix);if(!early.ok)return reject("evaluation_failed",early.error,early.diagnostics);stage=early.evaluation;}
    for(const auto& body:stage->bodies)if(body.featureId==id){view_->showPreview(body.shape,session_["inputs"].toObject().contains("featureId") ? body.bodyId : QString{},true);session_["geometry"]=body.summary;session_["featureId"]=body.featureId;session_["bodyId"]=body.bodyId;break;}
    session_["documentDraft"]=candidate;session_["previewed"]=true;session_["geometryEvaluated"]=true;session_.remove("previewError");
    if(loftDock_){loftDock_->findChild<QLabel*>("loftError")->clear();loftDock_->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->setEnabled(true);}
    return session_;
}
QJsonObject AlloyWindow::selectLoftProfile(const QString& sketchId,const QString& profileId) {
    auto inputs=session_["inputs"].toObject();auto refs=inputs["profiles"].toArray();QJsonArray next;bool removed=false;
    for(const auto& value:refs){const auto r=value.toObject();if(r["sketchId"]==sketchId && r["profileId"]==profileId){removed=true;continue;}next.append(r);}
    if(!removed)next.append(QJsonObject{{"sketchId",sketchId},{"profileId",profileId}});inputs["profiles"]=next;session_["inputs"]=inputs;previewLoft();return {{"kind","profile"},{"sketchId",sketchId},{"profileId",profileId},{"profiles",next}};
}
void AlloyWindow::showLoft() {
    if(loftDock_){canvasWindow_->removeDockWidget(loftDock_);delete loftDock_;}
    loftDock_=new QDockWidget("Loft",canvasWindow_);loftDock_->setObjectName("loftDock");loftDock_->setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);
    auto* panel=new QWidget(loftDock_);loftDock_->setWidget(panel);auto* form=new QFormLayout(panel);
    auto* name=new QLineEdit(session_["inputs"].toObject()["name"].toString(),panel);name->setObjectName("loftName");form->addRow("Name",name);
    connect(name,&QLineEdit::editingFinished,panel,[this,name]{auto inputs=session_["inputs"].toObject();inputs["name"]=name->text();session_["inputs"]=inputs;});
    auto* list=new QListWidget(panel);list->setObjectName("loftProfiles");list->setMinimumHeight(100);form->addRow("Profiles",list);
    auto* help=new QLabel("Click circular profiles in order. Click again to remove.",panel);help->setWordWrap(true);form->addRow(help);
    for(const auto& action:QStringList{"Move Up","Move Down","Remove"}){auto* button=new QPushButton(action,panel);form->addRow(button);connect(button,&QPushButton::clicked,panel,[this,list,action]{auto inputs=session_["inputs"].toObject();auto refs=inputs["profiles"].toArray();const auto index=list->currentRow();if(index<0 || index>=refs.size())return;int target=index;if(action=="Remove")refs.removeAt(index);else{target=index+(action=="Move Up" ? -1 : 1);if(target<0 || target>=refs.size())return;const QJsonValue temp=refs[index];refs[index]=refs[target];refs[target]=temp;}inputs["profiles"]=refs;session_["inputs"]=inputs;previewLoft();list->setCurrentRow(std::min(target,int(refs.size())-1));});}
    form->addRow("Start/End",new QLabel("Free",panel));form->addRow("Guide Type",new QLabel("Rails · none",panel));form->addRow("Chain Selection",new QLabel("On",panel));form->addRow("Closed",new QLabel("Off",panel));form->addRow("Operation",new QLabel("New Body",panel));
    auto* error=new QLabel(panel);error->setObjectName("loftError");error->setWordWrap(true);form->addRow(error);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,panel);form->addRow(buttons);
    connect(buttons,&QDialogButtonBox::accepted,panel,[this,error]{const auto response=execute("command.commit",{{"session",session_["id"]}});if(!response["ok"].toBool())error->setText(response["error"].toObject()["message"].toString());});connect(buttons,&QDialogButtonBox::rejected,panel,[this]{execute("command.cancel",{{"session",session_["id"]}});});
    auto cancel=[this]{if(session_.value("command")=="solid.loft")execute("command.cancel",{{"session",session_["id"]}});};auto* escape=new QShortcut(QKeySequence(Qt::Key_Escape),panel);escape->setContext(Qt::WidgetWithChildrenShortcut);connect(escape,&QShortcut::activated,panel,cancel);
    if(!loftEscape_){loftEscape_=new QShortcut(QKeySequence(Qt::Key_Escape),view_);loftEscape_->setContext(Qt::WidgetShortcut);connect(loftEscape_,&QShortcut::activated,this,cancel);}loftEscape_->setEnabled(true);
    canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,loftDock_);loftDock_->show();
}
