// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "BodyCopy.h"
#include "ReferenceView.h"
#include <QDockWidget>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QShortcut>
#include <QVBoxLayout>
namespace {
QJsonObject failure(const QString& code,const QString& message){return {{"ok",false},{"completion","failed"},{"error",QJsonObject{{"code",code},{"message",message}}}};}
QJsonObject field(const QString& type){return {{"type",type},{"required",false}};}
}
void AlloyWindow::registerBodyCopyCommands(){
    QJsonObject schema{{"name",field("string")}};for(const auto* key:{"xMm","yMm","zMm"}){auto f=field("number");f["minimum"]=-100000;f["maximum"]=100000;schema[key]=f;}
    commands_.push_back({"body.copySelection","Copy Body","Freeze one committed local solid in this design's body clipboard. Later source edits do not propagate.",{{"bodyId",field("string")}},true,[this](const QJsonObject& a){
        if(!session_.isEmpty())return failure("active_command","Finish or Cancel the active command before Copy");
        if(nonzeroAssemblyPose())return failure("staged_pose","Revert the assembly pose before Copy");
        const auto id=a.value("bodyId").toString(selectedBody_);const EvaluatedBody* source=nullptr;
        for(const auto& b:document_->bodies())if(b.bodyId==id && b.occurrenceId.isEmpty())source=&b;
        if(!source || source->ownerId!=document_->authored()["documentId"].toString())return failure("body_unavailable","Choose one committed root solid body");
        try{bodyClipboard_={{"tabId",activeTabId_},{"documentId",document_->authored()["documentId"]},{"snapshot",BodyCopy::capture(*source)},{"name",source->name+" Copy"}};}
        catch(const std::exception& e){return failure("invalid_body_copy",QString::fromUtf8(e.what()));}
        execute("selection.set",{{"bodyId",id}});
        return QJsonObject{{"copied",true},{"snapshotSha256",bodyClipboard_["snapshot"].toObject()["sha256"]}};
    }});
    commands_.push_back({"body.paste","Paste Body","Commit an independent frozen body with explicit XYZ translation in millimeters.",schema,true,[this](const QJsonObject& a){
        QJsonObject candidate;try{candidate=bodyCopyCandidate(a);}catch(const std::exception& e){return failure("invalid_body_copy",QString::fromUtf8(e.what()));}
        const auto error=document_->commit(candidate,"Paste Body");if(!error.isEmpty())return failure("evaluation_failed",error);
        selectedBody_=candidate["features"].toArray().last().toObject()["bodyId"].toString();return document_->state();
    }});
    commands_.push_back({"body.pasteDialog","Paste","Preview an independent body snapshot and enter XYZ translation before OK or Cancel.",schema,true,[this](const QJsonObject& a){return beginBodyCopy(a);}});
}
QJsonObject AlloyWindow::bodyCopyCandidate(const QJsonObject& args) const {
    if(bodyClipboard_["tabId"]!=activeTabId_ || bodyClipboard_["documentId"]!=document_->authored()["documentId"] || bodyClipboard_["snapshot"].toObject().isEmpty())
        throw std::invalid_argument("Copy a body in this design before Paste");
    auto inputs=args;inputs["name"]=args.value("name").toString(bodyClipboard_["name"].toString());
    inputs=authoringArguments(inputs);
    if(inputs["ownerId"]!=document_->authored()["documentId"])throw std::invalid_argument("Activate the root component before Paste Body");
    auto snapshot=bodyClipboard_["snapshot"].toObject();
    if(session_.value("command")=="body.paste"){
        inputs["newFeatureId"]=session_["newFeatureId"];inputs["newBodyId"]=session_["newBodyId"];snapshot=session_["snapshot"].toObject();
    }
    return CadDocument::bodyCopyCandidate(document_->authored(),snapshot,inputs);
}
QJsonObject AlloyWindow::beginBodyCopy(const QJsonObject& args){
    if(!session_.isEmpty())return failure("active_command","Finish or Cancel before Paste");
    try{bodyCopyCandidate(args);}catch(const std::exception& e){return failure("invalid_body_copy",QString::fromUtf8(e.what()));}
    QJsonObject inputs{{"xMm",0},{"yMm",0},{"zMm",0},{"name",bodyClipboard_["name"]}};for(auto it=args.begin();it!=args.end();++it)inputs[it.key()]=it.value();
    session_={{"id",CadDocument::newId()},{"command","body.paste"},{"status","active"},{"inputs",inputs},
        {"newFeatureId",CadDocument::newId()},{"newBodyId",CadDocument::newId()},{"snapshot",bodyClipboard_["snapshot"]},
        {"baseDocumentRevision",static_cast<qint64>(document_->revision())},{"previewed",false}};
    if(bodyCopyDock_){canvasWindow_->removeDockWidget(bodyCopyDock_);bodyCopyDock_->deleteLater();}
    bodyCopyDock_=new QDockWidget("Paste Body",canvasWindow_);bodyCopyDock_->setObjectName("body.copy.dialog");bodyCopyDock_->setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);
    auto* panel=new QWidget(bodyCopyDock_);bodyCopyDock_->setWidget(panel);auto* layout=new QVBoxLayout(panel);
    auto* label=new QLabel("Independent body snapshot",panel);layout->addWidget(label);auto* form=new QFormLayout;layout->addLayout(form);
    auto update=[this](const QJsonObject& changes){execute("command.update",{{"session",session_["id"]},{"inputs",changes}});};
    auto* name=new QLineEdit(inputs["name"].toString(),panel);name->setObjectName("body.copy.name");form->addRow("Name",name);connect(name,&QLineEdit::textEdited,panel,[update](const QString& text){update({{"name",text}});});
    const QStringList keys{"xMm","yMm","zMm"},labels{"X Distance","Y Distance","Z Distance"};
    for(int i=0;i<3;++i){auto* spin=new QDoubleSpinBox(panel);spin->setObjectName("body.copy."+keys[i]);spin->setAccessibleName(labels[i]);spin->setRange(-100000,100000);spin->setDecimals(3);spin->setSuffix(" mm");spin->setValue(inputs[keys[i]].toDouble());form->addRow(labels[i],spin);connect(spin,&QDoubleSpinBox::valueChanged,panel,[update,key=keys[i]](double value){update({{key,value}});});}
    auto* status=new QLabel(panel);status->setObjectName("body.copy.status");status->setWordWrap(true);layout->addWidget(status);layout->addStretch(1);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,panel);buttons->button(QDialogButtonBox::Ok)->setObjectName("body.copy.ok");buttons->button(QDialogButtonBox::Cancel)->setObjectName("body.copy.cancel");layout->addWidget(buttons);
    connect(buttons,&QDialogButtonBox::accepted,panel,[this]{execute("command.commit",{{"session",session_["id"]}});});connect(buttons,&QDialogButtonBox::rejected,panel,[this]{execute("command.cancel",{{"session",session_["id"]}});});
    auto* escape=new QShortcut(QKeySequence(Qt::Key_Escape),panel);escape->setContext(Qt::WidgetWithChildrenShortcut);connect(escape,&QShortcut::activated,panel,[this]{execute("command.cancel",{{"session",session_["id"]}});});
    bodyCopyDock_->setMinimumWidth(300);canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,bodyCopyDock_);bodyCopyDock_->show();previewBodyCopy();return session_;
}
QJsonObject AlloyWindow::previewBodyCopy(){
    view_->clearPreview();view_->clearBodyStage();session_["previewed"]=false;session_.remove("geometry");session_.remove("previewError");session_.remove("documentDraft");
    QString message;QJsonObject candidate;
    if(session_["baseDocumentRevision"].toInteger()!=static_cast<qint64>(document_->revision()))message="The design changed; Cancel and restart Paste";
    if(message.isEmpty())try{candidate=bodyCopyCandidate(session_["inputs"].toObject());}catch(const std::exception& e){message=QString::fromUtf8(e.what());}
    if(message.isEmpty()){
        const auto evaluated=CadDocument::evaluateCandidate(candidate);if(!evaluated.ok)message=evaluated.error;
        else {view_->setBodyStage(evaluated.evaluation->bodies,"body-copy:"+session_["id"].toString());
            for(const auto& b:evaluated.evaluation->bodies)if(b.bodyId==session_["newBodyId"].toString()){view_->showMovePreview(b.shape,b.bodyId);session_["geometry"]=b.summary;}
            session_["documentDraft"]=candidate;session_["previewed"]=true;session_["geometryEvaluated"]=true;}
    }
    if(bodyCopyDock_)if(auto* status=bodyCopyDock_->findChild<QLabel*>("body.copy.status"))status->setText(message);
    if(!message.isEmpty()){session_["previewError"]=QJsonObject{{"code","invalid_body_copy"},{"message",message}};return failure("invalid_body_copy",message);}return session_;
}
