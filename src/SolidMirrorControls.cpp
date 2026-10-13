#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <QDockWidget>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QShortcut>

namespace {
QJsonObject mirrorFailure(const QString& code,const QString& message,const QJsonObject& diagnostics={}){
    return {{"ok",false},{"completion","failed"},{"error",QJsonObject{{"code",code},{"message",message},{"diagnostics",diagnostics}}}};
}
}
QJsonObject AlloyWindow::mirrorCandidate(const QJsonObject& inputs) const {
    auto args=inputs;
    if(!args.contains("featureId") && session_.value("command")=="solid.mirror"){
        args["newFeatureId"]=session_["newFeatureId"];args["newBodyId"]=session_["newBodyId"];
    }
    return CadDocument::mirrorCandidate(document_->authored(),args);
}
QJsonObject AlloyWindow::beginMirror(const QJsonObject& overrides){
    if(!session_.isEmpty())return mirrorFailure("session_busy","Complete or cancel the active command first");
    const auto error=validateArguments(*findCommand("solid.mirror"),overrides);
    if(!error.isEmpty())return mirrorFailure("invalid_arguments",error);
    QJsonObject inputs{{"sourceBodyId",selectedBody_},{"mirrorPlane",selectedFace_},{"name","Mirror"}};
    if(overrides.contains("featureId")){
        bool found=false;for(const auto& entry:document_->authored()["features"].toArray())if(entry.toObject()["id"]==overrides["featureId"]){
            const auto feature=entry.toObject();if(feature["type"]!="mirror")return mirrorFailure("stale_entity","Mirror feature unavailable");
            inputs=feature["parameters"].toObject();inputs["name"]=feature["name"];inputs["featureId"]=feature["id"];found=true;break;
        }
        if(!found)return mirrorFailure("stale_entity","Mirror feature unavailable");
    }
    for(auto it=overrides.begin();it!=overrides.end();++it)inputs[it.key()]=it.value();
    session_={{"id",CadDocument::newId()},{"command","solid.mirror"},{"status","active"},{"inputs",inputs},
        {"newFeatureId",CadDocument::newId()},{"newBodyId",CadDocument::newId()},
        {"selectionRole",inputs["sourceBodyId"].toString().isEmpty() ? "objects" : "mirrorPlane"},
        {"baseDocumentRevision",static_cast<qint64>(document_->revision())},{"previewed",false},
        {"geometryEvaluated",false},{"inputSchema",findCommand("solid.mirror")->parameters}};
    if(!inputs["featureId"].toString().isEmpty())view_->setBodyStage(document_->evaluationBefore(inputs["featureId"].toString()).bodies,"mirror-source:"+session_["id"].toString());
    showMirror();previewMirror();view_->setFocus();return session_;
}
QJsonObject AlloyWindow::previewMirror(){
    if(mirrorDock_)if(auto* buttons=mirrorDock_->findChild<QDialogButtonBox*>())buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
    session_["previewed"]=false;session_["geometryEvaluated"]=false;session_.remove("geometry");session_.remove("documentDraft");session_.remove("previewError");view_->clearPreview();
    const auto inputs=session_["inputs"].toObject();
    if(mirrorDock_){
        if(auto* label=mirrorDock_->findChild<QLabel*>("mirrorObject"))label->setText(inputs["sourceBodyId"].toString().isEmpty() ? "0 selected" : "1 selected");
        if(auto* label=mirrorDock_->findChild<QLabel*>("mirrorPlane"))label->setText(inputs["mirrorPlane"].toObject().isEmpty() ? "0 selected" : "1 selected");
    }
    auto reject=[this](const QString& code,const QString& message,const QJsonObject& diagnostics=QJsonObject{}){
        auto result=mirrorFailure(code,message,diagnostics);session_["previewError"]=result["error"];
        if(mirrorDock_)if(auto* label=mirrorDock_->findChild<QLabel*>("mirrorError"))label->setText(message);return result;
    };
    if(session_["baseDocumentRevision"].toInteger()!=static_cast<qint64>(document_->revision()))return reject("stale_session","The document changed; cancel and restart Mirror");
    if(inputs["sourceBodyId"].toString().isEmpty())return reject("object_required","Select one source body");
    if(inputs["mirrorPlane"].toObject().isEmpty())return reject("plane_required","Select a retained planar face as Mirror Plane");
    QJsonObject candidate;try{candidate=mirrorCandidate(inputs);}catch(const std::exception& error){return reject("invalid_arguments",QString::fromUtf8(error.what()));}
    const auto evaluated=CadDocument::evaluateCandidate(candidate);if(!evaluated.ok)return reject("evaluation_failed",evaluated.error,evaluated.diagnostics);
    auto stage=evaluated.evaluation;const auto edit=inputs["featureId"].toString();
    if(!edit.isEmpty()){
        QJsonArray features;for(const auto& entry:candidate["features"].toArray()){features.append(entry);if(entry.toObject()["id"]==edit)break;}
        auto prefix=candidate;prefix["features"]=features;const auto early=CadDocument::evaluateCandidate(prefix);
        if(!early.ok)return reject("evaluation_failed",early.error,early.diagnostics);stage=early.evaluation;
    }
    const auto featureId=inputs.value("featureId").toString(session_["newFeatureId"].toString());
    bool found=false;for(const auto& body:stage->bodies)if(body.featureId==featureId){
        view_->showPreview(body.shape,body.bodyId,true);session_["geometry"]=body.summary;session_["bodyId"]=body.bodyId;found=true;break;
    }
    if(!found)return reject("body_unavailable","Mirrored result is unavailable");
    view_->selectBody(inputs["sourceBodyId"].toString());
    session_["documentDraft"]=candidate;session_["previewed"]=true;session_["geometryEvaluated"]=true;
    if(mirrorDock_){if(auto* buttons=mirrorDock_->findChild<QDialogButtonBox*>())buttons->button(QDialogButtonBox::Ok)->setEnabled(true);
        if(auto* label=mirrorDock_->findChild<QLabel*>("mirrorError"))label->clear();}
    return session_;
}
void AlloyWindow::showMirror(){
    if(mirrorDock_){canvasWindow_->removeDockWidget(mirrorDock_);delete mirrorDock_;}
    mirrorDock_=new QDockWidget("Mirror",canvasWindow_);mirrorDock_->setObjectName("mirrorDock");
    mirrorDock_->setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);
    auto* panel=new QWidget(mirrorDock_);panel->setMinimumWidth(240);mirrorDock_->setWidget(panel);auto* form=new QFormLayout(panel);
    form->setRowWrapPolicy(QFormLayout::WrapLongRows);
    form->addRow("Pattern Type",new QLabel("Bodies",panel));
    auto* object=new QLabel(panel);object->setObjectName("mirrorObject");form->addRow("Objects",object);
    auto* chooseObject=new QPushButton("Select Object",panel);form->addRow(chooseObject);
    connect(chooseObject,&QPushButton::clicked,panel,[this]{session_["selectionRole"]="objects";view_->setFocus();});
    auto* plane=new QLabel(panel);plane->setObjectName("mirrorPlane");form->addRow("Mirror Plane",plane);
    auto* choosePlane=new QPushButton("Select Mirror Plane",panel);form->addRow(choosePlane);
    connect(choosePlane,&QPushButton::clicked,panel,[this]{session_["selectionRole"]="mirrorPlane";view_->setFocus();});
    auto* status=new QLabel(panel);status->setObjectName("mirrorError");status->setWordWrap(true);form->addRow(status);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,panel);form->addRow(buttons);
    buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
    connect(buttons,&QDialogButtonBox::accepted,panel,[this,status]{const auto response=execute("command.commit",{{"session",session_["id"]}});
        if(!response["ok"].toBool())status->setText(response["error"].toObject()["message"].toString());});
    connect(buttons,&QDialogButtonBox::rejected,panel,[this]{execute("command.cancel",{{"session",session_["id"]}});});
    auto cancel=[this]{if(session_.value("command")=="solid.mirror")execute("command.cancel",{{"session",session_["id"]}});};
    auto* escape=new QShortcut(QKeySequence(Qt::Key_Escape),panel);escape->setContext(Qt::WidgetWithChildrenShortcut);connect(escape,&QShortcut::activated,panel,cancel);
    if(!mirrorEscape_){mirrorEscape_=new QShortcut(QKeySequence(Qt::Key_Escape),view_);mirrorEscape_->setContext(Qt::WidgetShortcut);connect(mirrorEscape_,&QShortcut::activated,this,cancel);}
    mirrorEscape_->setEnabled(true);canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,mirrorDock_);mirrorDock_->show();
}
