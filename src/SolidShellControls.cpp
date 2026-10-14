#include "AlloyWindow.h"
#include "ReferenceView.h"
#include "LengthParameterSpinBox.h"
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
    if(args.contains("thicknessParameter")){
        const auto row=lengthParameter(args["thicknessParameter"].toString());
        if(row.isEmpty())throw std::invalid_argument("User length name is unavailable");
        args["thicknessParameterId"]=row["id"];args["thicknessMm"]=row["valueMm"];args.remove("thicknessParameter");
    }
    return CadDocument::shellCandidate(document_->authored(),authoringArguments(args));
}
QJsonObject AlloyWindow::beginShell(const QJsonObject& overrides) {
    if(!session_.isEmpty())return shellFailure("session_busy","Complete or cancel the active command first");
    const auto error=validateArguments(*findCommand("solid.shell"),overrides);
    if(!error.isEmpty())return shellFailure("invalid_arguments",error);
    QJsonObject inputs{{"targetBodyId",selectedFace_.isEmpty() ? QJsonValue(selectedBody_) : selectedFace_.value("bodyId")},{"removedFace",selectedFace_},
        {"thicknessMm",0.0},{"name","Shell"}};
    if(overrides.contains("featureId")){
        bool found=false;
        for(const auto& entry:document_->authored()["features"].toArray())if(entry.toObject()["id"]==overrides["featureId"]){
            const auto feature=entry.toObject();if(feature["type"]!="shell")return shellFailure("stale_entity","Shell feature unavailable");
            inputs=feature["parameters"].toObject();inputs["name"]=feature["name"];inputs["featureId"]=feature["id"];found=true;break;
        }
        if(!found)return shellFailure("stale_entity","Shell feature unavailable");
    }
    if(inputs.contains("thicknessParameterId")){
        for(const auto& entry:document_->authored()["userParameters"].toArray())if(entry.toObject()["id"]==inputs["thicknessParameterId"]){inputs["thicknessParameter"]=entry.toObject()["name"];break;}
        inputs.remove("thicknessParameterId");
    }
    for(auto it=overrides.begin();it!=overrides.end();++it)inputs[it.key()]=it.value();
    if(inputs.contains("thicknessParameter")){
        const auto row=lengthParameter(inputs["thicknessParameter"].toString());
        if(row.isEmpty())return shellFailure("unknown_parameter","User length name is unavailable");
        inputs["thicknessMm"]=row["valueMm"];
    }
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
    if(shellDock_)if(auto* label=shellDock_->findChild<QLabel*>("shellSelection"))
        label->setText(inputs["targetBodyId"].toString().isEmpty() ? "0 selected" : inputs["removedFace"].toObject().isEmpty() ? "1 body selected (closed cavity)" : "1 face selected");
    if(inputs["targetBodyId"].toString().isEmpty())return reject("body_required","Select a body in the Browser for a closed cavity, or a face to remove");
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
    form->addRow("Object",new QLabel("Faces / Body",panel));
    auto* selected=new QLabel(panel);selected->setObjectName("shellSelection");
    selected->setWordWrap(true);
    auto* help=new QLabel("Select a face in the canvas to remove it, or select the body in the Browser to keep every face.",panel);help->setWordWrap(true);form->addRow(help);
    form->addRow("Selection",selected);
    form->addRow("Type",new QLabel("Sharp Offset",panel));form->addRow("Direction",new QLabel("Inside",panel));
    auto* thickness=new LengthParameterSpinBox(panel);thickness->setObjectName("shellThickness");thickness->setAccessibleName("Inside Thickness");
    thickness->setKeyboardTracking(false);thickness->setDecimals(3);thickness->setRange(0,100000);thickness->setSuffix(" mm");
    thickness->namedValue=[this](const QString& name)->std::optional<double>{const auto row=lengthParameter(name);if(row.isEmpty())return {};return row["valueMm"].toDouble();};
    thickness->setValue(session_["inputs"].toObject()["thicknessMm"].toDouble());thickness->showBinding(session_["inputs"].toObject()["thicknessParameter"].toString());form->addRow("Inside Thickness",thickness);
    auto* status=new QLabel(panel);status->setObjectName("shellError");status->setWordWrap(true);form->addRow(status);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,panel);form->addRow(buttons);
    buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
    connect(thickness,&QDoubleSpinBox::valueChanged,panel,[this,thickness](double value){
        if(!thickness->enteredIdentifier().isEmpty())return;
        const auto response=execute("command.update",{{"session",session_["id"]},{"inputs",QJsonObject{{"thicknessMm",value}}}});
        if(response["ok"].toBool())previewShell();
    });
    connect(thickness,&QDoubleSpinBox::editingFinished,panel,[this,thickness,status,buttons]{
        const auto typed=thickness->enteredIdentifier();
        if(!typed.isEmpty() && lengthParameter(typed).isEmpty()){
            session_["invalidNamedInput"]=typed;session_["previewed"]=false;session_["geometryEvaluated"]=false;
            view_->clearPreview();buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
            status->setText("User length name is unavailable");return;
        }
        if(typed.isEmpty() && !thickness->literalEdited())return;
        session_.remove("invalidNamedInput");
        const auto update=typed.isEmpty() ? QJsonObject{{"thicknessMm",thickness->value()}} : QJsonObject{{"thicknessParameter",typed}};
        const auto response=execute("command.update",{{"session",session_["id"]},{"inputs",update}});
        if(response["ok"].toBool()){thickness->showBinding(typed);previewShell();}else status->setText(response["error"].toObject()["message"].toString());
    });
    connect(buttons,&QDialogButtonBox::accepted,panel,[this,status]{const auto response=execute("command.commit",{{"session",session_["id"]}});
        if(!response["ok"].toBool())status->setText(response["error"].toObject()["message"].toString());});
    connect(buttons,&QDialogButtonBox::rejected,panel,[this]{execute("command.cancel",{{"session",session_["id"]}});});
    auto cancel=[this]{if(session_.value("command")=="solid.shell")execute("command.cancel",{{"session",session_["id"]}});};
    auto* escape=new QShortcut(QKeySequence(Qt::Key_Escape),panel);escape->setContext(Qt::WidgetWithChildrenShortcut);connect(escape,&QShortcut::activated,panel,cancel);
    if(!shellEscape_){shellEscape_=new QShortcut(QKeySequence(Qt::Key_Escape),view_);shellEscape_->setContext(Qt::WidgetShortcut);connect(shellEscape_,&QShortcut::activated,this,cancel);}
    shellEscape_->setEnabled(true);canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,shellDock_);shellDock_->show();
}
