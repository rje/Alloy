// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QShortcut>

namespace {
QJsonObject failure(const QString& code,const QString& message,const QJsonObject& diagnostics={}){
    return {{"ok",false},{"error",QJsonObject{{"code",code},{"message",message},{"diagnostics",diagnostics}}}};
}
}

QJsonObject AlloyWindow::removeBodyCandidate(const QJsonObject& inputs) const{
    auto args=inputs;
    if(!args.contains("featureId")){
        if(session_.value("command")=="body.remove")args["newFeatureId"]=session_["newFeatureId"];
        QString producer;
        for(const auto& body:document_->bodies())if(body.bodyId==args["bodyId"] && body.occurrenceId.isEmpty()
            && body.summary["type"]=="surface")producer=body.featureId;
        if(producer.isEmpty())throw std::invalid_argument("Select one current local surface body");
        for(const auto& row:document_->authored()["features"].toArray())if(row.toObject()["id"]==producer)
            args["ownerId"]=row.toObject()["ownerId"];
    }
    return CadDocument::removeBodyCandidate(document_->authored(),args);
}

QJsonObject AlloyWindow::beginRemoveBody(const QJsonObject& overrides){
    if(!session_.isEmpty())return failure("session_busy","Complete or cancel the active command first");
    QJsonObject inputs{{"name","Remove"}};
    if(overrides.contains("featureId")){
        bool found=false;
        for(const auto& row:document_->authored()["features"].toArray())if(row.toObject()["id"]==overrides["featureId"]){
            const auto feature=row.toObject();if(feature["type"]!="removeBody")return failure("stale_entity","Remove feature unavailable");
            inputs=feature["parameters"].toObject();inputs["featureId"]=feature["id"];inputs["name"]=feature["name"];
            found=true;break;
        }
        if(!found)return failure("stale_entity","Remove feature unavailable");
    }else if(!selectedBody_.isEmpty())inputs["bodyId"]=selectedBody_;
    for(auto it=overrides.begin();it!=overrides.end();++it)inputs[it.key()]=it.value();
    const auto error=validateArguments(*findCommand("body.removeDialog"),inputs);
    if(!error.isEmpty())return failure("invalid_arguments",error);
    session_={{"id",CadDocument::newId()},{"command","body.remove"},{"status","active"},{"inputs",inputs},
        {"newFeatureId",CadDocument::newId()},{"baseDocumentRevision",static_cast<qint64>(document_->revision())},
        {"previewed",false},{"geometryEvaluated",false},{"inputSchema",findCommand("body.remove")->parameters}};
    showRemoveBody();previewRemoveBody();view_->setFocus(Qt::OtherFocusReason);return session_;
}

QJsonObject AlloyWindow::previewRemoveBody(){
    if(session_.value("command")!="body.remove")return failure("invalid_session","Start Remove first");
    session_["previewed"]=false;session_["geometryEvaluated"]=false;
    session_.remove("documentDraft");session_.remove("geometry");view_->clearBodyStage();
    if(removeBodyDock_)if(auto* buttons=removeBodyDock_->findChild<QDialogButtonBox*>())
        buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
    auto reject=[&](const QString& code,const QString& message,const QJsonObject& diagnostics=QJsonObject{}){
        const auto result=failure(code,message,diagnostics);session_["previewError"]=result["error"];
        if(removeBodyDock_)if(auto* label=removeBodyDock_->findChild<QLabel*>("removeBody.error"))label->setText(message);
        return result;
    };
    if(session_["baseDocumentRevision"].toInteger()!=static_cast<qint64>(document_->revision()))
        return reject("stale_session","The document changed; cancel and restart Remove");
    const auto inputs=session_["inputs"].toObject();
    if(inputs["bodyId"].toString().isEmpty())return reject("body_required","Select one current surface body");
    QJsonObject candidate;
    try{candidate=removeBodyCandidate(inputs);}catch(const std::exception& error){return reject("invalid_arguments",QString::fromUtf8(error.what()));}
    const auto result=document_->prepareCandidate(candidate);
    if(!result.ok)return reject("evaluation_failed",result.error,result.diagnostics);
    auto stage=result.evaluation;
    if(inputs.contains("featureId")){
        auto prefix=candidate;QJsonArray features;
        for(const auto& row:candidate["features"].toArray()){features.append(row);if(row.toObject()["id"]==inputs["featureId"])break;}
        prefix["features"]=features;const auto early=CadDocument::evaluateCandidate(prefix);
        if(!early.ok)return reject("evaluation_failed",early.error,early.diagnostics);stage=early.evaluation;
    }
    view_->setBodyStage(stage->bodies,"remove-preview:"+session_["id"].toString());
    session_["documentDraft"]=candidate;session_["previewed"]=true;session_["geometryEvaluated"]=true;
    session_["geometry"]=QJsonObject{{"remainingBodies",int(stage->bodies.size())},{"removedBodyId",inputs["bodyId"]}};
    session_.remove("previewError");
    if(removeBodyDock_){removeBodyDock_->findChild<QLabel*>("removeBody.error")->clear();
        removeBodyDock_->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->setEnabled(true);}
    return session_;
}

void AlloyWindow::showRemoveBody(){
    if(removeBodyDock_){canvasWindow_->removeDockWidget(removeBodyDock_);delete removeBodyDock_;}
    removeBodyDock_=new QDockWidget("Remove",canvasWindow_);removeBodyDock_->setObjectName("removeBodyDock");
    auto* panel=new QWidget(removeBodyDock_);removeBodyDock_->setWidget(panel);auto* form=new QFormLayout(panel);
    auto* selector=new QComboBox(panel);selector->setObjectName("removeBody.selector");
    selector->addItem("Select surface body",QString{});
    const auto edit=session_["inputs"].toObject()["featureId"].toString();
    const auto stage=edit.isEmpty()?document_->evaluation():document_->evaluationBefore(edit);
    for(const auto& body:stage.bodies)if(body.occurrenceId.isEmpty() && body.summary["type"]=="surface")
        selector->addItem(body.name,body.bodyId);
    selector->setCurrentIndex(std::max(0,selector->findData(session_["inputs"].toObject()["bodyId"].toString())));
    form->addRow("Bodies",selector);
    auto* note=new QLabel("Remove keeps source features in the Timeline. Undo or delete this Remove feature to restore the sheet.",panel);
    note->setWordWrap(true);form->addRow(note);
    auto* error=new QLabel(panel);error->setObjectName("removeBody.error");error->setWordWrap(true);form->addRow(error);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,panel);form->addRow(buttons);
    connect(selector,qOverload<int>(&QComboBox::currentIndexChanged),panel,[this,selector](int){
        auto inputs=session_["inputs"].toObject();inputs["bodyId"]=selector->currentData().toString();
        session_["inputs"]=inputs;previewRemoveBody();
    });
    connect(buttons,&QDialogButtonBox::accepted,panel,[this,error]{const auto response=execute("command.commit",{{"session",session_["id"]}});
        if(!response["ok"].toBool())error->setText(response["error"].toObject()["message"].toString());});
    auto cancel=[this]{if(session_.value("command")=="body.remove")execute("command.cancel",{{"session",session_["id"]}});};
    connect(buttons,&QDialogButtonBox::rejected,panel,cancel);
    auto* escape=new QShortcut(QKeySequence(Qt::Key_Escape),panel);escape->setContext(Qt::WidgetWithChildrenShortcut);
    connect(escape,&QShortcut::activated,panel,cancel);
    if(!removeBodyEscape_){removeBodyEscape_=new QShortcut(QKeySequence(Qt::Key_Escape),view_);
        removeBodyEscape_->setContext(Qt::WidgetShortcut);connect(removeBodyEscape_,&QShortcut::activated,this,cancel);}
    removeBodyEscape_->setEnabled(true);
    canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,removeBodyDock_);removeBodyDock_->show();
}
