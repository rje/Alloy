#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <QDockWidget>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QShortcut>

namespace {
QJsonObject shellFailure(const QString& code,const QString& message,const QJsonObject& diagnostics={}) {
    return {{"ok",false},{"completion","failed"},{"error",QJsonObject{{"code",code},{"message",message},{"diagnostics",diagnostics}}}};
}
}
QJsonObject AlloyWindow::shellCandidate(const QJsonObject& inputs) const {
    auto args=inputs;
    if(!args.contains("featureId") && session_.value("command")=="solid.shell")args["newFeatureId"]=session_["newFeatureId"];
    return CadDocument::shellCandidate(document_->authored(),args);
}
QJsonObject AlloyWindow::beginShell(const QJsonObject& overrides) {
    if(!session_.isEmpty())return shellFailure("session_busy","Complete or cancel the active command first");
    const auto error=validateArguments(*findCommand("solid.shell"),overrides);
    if(!error.isEmpty())return shellFailure("invalid_arguments",error);
    QJsonObject inputs{{"targetBodyId",selectedFace_["bodyId"]},{"removedFace",selectedFace_},
        {"thicknessMm",0.0},{"name","Shell"}};
    if(overrides.contains("featureId")){
        bool found=false;
        for(const auto& entry:document_->authored()["features"].toArray())if(entry.toObject()["id"]==overrides["featureId"]){
            const auto feature=entry.toObject();if(feature["type"]!="shell")return shellFailure("stale_entity","Shell feature unavailable");
            inputs=feature["parameters"].toObject();inputs["name"]=feature["name"];inputs["featureId"]=feature["id"];found=true;break;
        }
        if(!found)return shellFailure("stale_entity","Shell feature unavailable");
    }
    for(auto it=overrides.begin();it!=overrides.end();++it)inputs[it.key()]=it.value();
    session_={{"id",CadDocument::newId()},{"command","solid.shell"},{"status","active"},
        {"inputs",inputs},{"newFeatureId",CadDocument::newId()},
        {"baseDocumentRevision",static_cast<qint64>(document_->revision())},
        {"previewed",false},{"geometryEvaluated",false},{"inputSchema",findCommand("solid.shell")->parameters}};
    const auto edit=inputs["featureId"].toString();
    if(!edit.isEmpty())view_->setBodyStage(document_->evaluationBefore(edit).bodies,"shell-source:"+session_["id"].toString());
    showShell();previewShell();view_->setFocus();return session_;
}
QJsonObject AlloyWindow::previewShell() {
    if(shellDock_)if(auto* box=shellDock_->findChild<QDialogButtonBox*>())box->button(QDialogButtonBox::Ok)->setEnabled(false);
    session_["previewed"]=false;session_["geometryEvaluated"]=false;session_.remove("geometry");session_.remove("documentDraft");session_.remove("previewError");view_->clearPreview();
    auto reject=[this](const QString& code,const QString& message,const QJsonObject& diagnostics=QJsonObject{}) {
        auto result=shellFailure(code,message,diagnostics);session_["previewError"]=result["error"];
        if(shellDock_)if(auto* label=shellDock_->findChild<QLabel*>("shellError"))label->setText(message);
        return result;
    };
    if(session_["baseDocumentRevision"].toInteger()!=static_cast<qint64>(document_->revision()))return reject("stale_session","The document changed; cancel and restart Shell");
    const auto inputs=session_["inputs"].toObject();
    if(inputs["removedFace"].toObject().isEmpty())return reject("face_required","Select a planar face to remove");
    if(inputs["thicknessMm"].toDouble()<=0)return reject("thickness_required","Enter an inside thickness");
    QJsonObject candidate;try{candidate=shellCandidate(inputs);}catch(const std::exception& error){return reject("invalid_arguments",QString::fromUtf8(error.what()));}
    const auto evaluated=CadDocument::evaluateCandidate(candidate);if(!evaluated.ok)return reject("evaluation_failed",evaluated.error,evaluated.diagnostics);
    auto stage=evaluated.evaluation;const auto edit=inputs["featureId"].toString();
    if(!edit.isEmpty()){
        QJsonArray features;for(const auto& entry:candidate["features"].toArray()){features.append(entry);if(entry.toObject()["id"]==edit)break;}
        auto prefix=candidate;prefix["features"]=features;const auto early=CadDocument::evaluateCandidate(prefix);
        if(!early.ok)return reject("evaluation_failed",early.error,early.diagnostics);stage=early.evaluation;
    }
    bool found=false;for(const auto& body:stage->bodies)if(body.bodyId==inputs["targetBodyId"].toString()){
        view_->showPreview(body.shape,body.bodyId,true);session_["geometry"]=body.summary;found=true;break;
    }
    if(!found)return reject("body_unavailable","Shell target body is unavailable");
    session_["documentDraft"]=candidate;session_["previewed"]=true;session_["geometryEvaluated"]=true;
    if(shellDock_){if(auto* box=shellDock_->findChild<QDialogButtonBox*>())box->button(QDialogButtonBox::Ok)->setEnabled(true);
        if(auto* label=shellDock_->findChild<QLabel*>("shellError"))label->clear();}
    return session_;
}
void AlloyWindow::showShell() {
    if(shellDock_){canvasWindow_->removeDockWidget(shellDock_);delete shellDock_;}
    shellDock_=new QDockWidget("Shell",canvasWindow_);shellDock_->setObjectName("shellDock");
    shellDock_->setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);
    auto* panel=new QWidget(shellDock_);shellDock_->setWidget(panel);auto* form=new QFormLayout(panel);
    form->addRow("Selection",new QLabel("1 Face",panel));
    form->addRow("Type",new QLabel("Sharp Offset",panel));form->addRow("Direction",new QLabel("Inside",panel));
    auto* thickness=new QDoubleSpinBox(panel);thickness->setObjectName("shellThickness");thickness->setAccessibleName("Inside Thickness");
    thickness->setKeyboardTracking(false);thickness->setDecimals(3);thickness->setRange(0,100000);thickness->setSuffix(" mm");
    thickness->setValue(session_["inputs"].toObject()["thicknessMm"].toDouble());form->addRow("Inside Thickness",thickness);
    auto* status=new QLabel(panel);status->setObjectName("shellError");status->setWordWrap(true);form->addRow(status);
    connect(thickness,&QDoubleSpinBox::valueChanged,panel,[this](double value){
        const auto response=execute("command.update",{{"session",session_["id"]},{"inputs",QJsonObject{{"thicknessMm",value}}}});
        if(response["ok"].toBool())previewShell();
    });
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,panel);form->addRow(buttons);
    buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
    connect(buttons,&QDialogButtonBox::accepted,panel,[this,status]{const auto response=execute("command.commit",{{"session",session_["id"]}});
        if(!response["ok"].toBool())status->setText(response["error"].toObject()["message"].toString());});
    connect(buttons,&QDialogButtonBox::rejected,panel,[this]{execute("command.cancel",{{"session",session_["id"]}});});
    auto cancel=[this]{if(session_.value("command")=="solid.shell")execute("command.cancel",{{"session",session_["id"]}});};
    auto* escape=new QShortcut(QKeySequence(Qt::Key_Escape),panel);escape->setContext(Qt::WidgetWithChildrenShortcut);connect(escape,&QShortcut::activated,panel,cancel);
    if(!shellEscape_){shellEscape_=new QShortcut(QKeySequence(Qt::Key_Escape),view_);shellEscape_->setContext(Qt::WidgetShortcut);connect(shellEscape_,&QShortcut::activated,this,cancel);}
    shellEscape_->setEnabled(true);canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,shellDock_);shellDock_->show();
}
