// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QShortcut>
#include <QSignalBlocker>
#include <algorithm>

namespace {
QJsonObject failure(const QString& code,const QString& message){return {{"ok",false},{"error",QJsonObject{{"code",code},{"message",message}}}};}
}
QJsonObject AlloyWindow::surfacePatchCandidate(const QJsonObject& inputs) const {
    auto args=inputs;
    if(!args.contains("featureId") && session_.value("command")=="surface.patch"){
        args["newFeatureId"]=session_.value("newFeatureId");args["newBodyId"]=session_.value("newBodyId");
    }
    args=authoringArguments(args);if(args.contains("featureId"))args.remove("ownerId");
    return CadDocument::surfacePatchCandidate(document_->authored(),args);
}
QJsonObject AlloyWindow::beginSurfacePatch(const QJsonObject& overrides) {
    if(!session_.isEmpty())return failure("session_busy","Complete or cancel the active command first");
    QJsonObject inputs{{"name","Patch"}};
    if(overrides.contains("featureId")){
        bool found=false;for(const auto& value:document_->authored().value("features").toArray()){
            const auto f=value.toObject();if(f.value("id")!=overrides.value("featureId"))continue;
            if(f.value("type")!="surfacePatch")return failure("stale_entity","Patch feature unavailable");
            const auto p=f.value("parameters").toObject();
            for(const auto& key:{"sketchId","profileId"})inputs[key]=p.value(key);
            inputs["featureId"]=f.value("id");inputs["name"]=f.value("name");found=true;break;
        }
        if(!found)return failure("stale_entity","Patch feature unavailable");
    }
    for(auto i=overrides.begin();i!=overrides.end();++i)inputs[i.key()]=i.value();
    const auto error=validateArguments(*findCommand("surface.patchDialog"),inputs);if(!error.isEmpty())return failure("invalid_arguments",error);
    session_={{"id",CadDocument::newId()},{"command","surface.patch"},{"status","active"},{"inputs",inputs},
        {"newFeatureId",CadDocument::newId()},{"newBodyId",CadDocument::newId()},
        {"baseDocumentRevision",static_cast<qint64>(document_->revision())},{"previewed",false},{"geometryEvaluated",false},
        {"inputSchema",findCommand("surface.patch")->parameters},{"nativeControlsAvailable",true}};
    showSurfacePatch();previewSurfacePatch();return session_;
}
QJsonObject AlloyWindow::previewSurfacePatch() {
    if(session_.value("command")!="surface.patch")return failure("invalid_session","Patch is not active");
    session_["previewed"]=false;session_["geometryEvaluated"]=false;session_.remove("geometry");session_.remove("documentDraft");view_->clearPreview();view_->clearSweepPath();
    auto reject=[&](const QString& code,const QString& message,const QJsonObject& details=QJsonObject{}){
        auto result=failure(code,message);auto error=result.value("error").toObject();error["diagnostics"]=details;
        result["error"]=error;session_["previewError"]=error;refreshSurfacePatchControls();return result;
    };
    if(session_.value("baseDocumentRevision").toInteger()!=static_cast<qint64>(document_->revision()))return reject("stale_session","The document changed; cancel and restart Patch");
    const auto inputs=session_.value("inputs").toObject();const auto edit=inputs.value("featureId").toString();
    const auto source=edit.isEmpty() ? document_->evaluation() : document_->evaluationBefore(edit);
    if(!edit.isEmpty())view_->setBodyStage(source.bodies,"surface-patch-source:"+session_.value("id").toString());
    const auto sketchId=inputs.value("sketchId").toString();view_->setCommandSketches(QSet<QString>{sketchId});
    for(const auto& key:{"sketchId","profileId"})if(inputs.value(key).toString().isEmpty())return reject("selection_required","Select one closed planar profile");
    QJsonObject candidate;try{candidate=surfacePatchCandidate(inputs);}catch(const std::exception& e){return reject("invalid_arguments",QString::fromUtf8(e.what()));}
    const auto evaluated=CadDocument::evaluateCandidate(candidate);if(!evaluated.ok)return reject("evaluation_failed",evaluated.error,evaluated.diagnostics);
    auto stage=evaluated.evaluation;const auto id=edit.isEmpty() ? session_.value("newFeatureId").toString() : edit;
    if(!edit.isEmpty()){
        auto prefix=candidate;QJsonArray rows;for(const auto& value:candidate.value("features").toArray()){rows.append(value);if(value.toObject().value("id")==id)break;}prefix["features"]=rows;
        const auto early=CadDocument::evaluateCandidate(prefix);if(!early.ok)return reject("evaluation_failed",early.error,early.diagnostics);stage=early.evaluation;
    }
    for(const auto& body:stage->bodies)if(body.featureId==id){view_->showPreview(body.shape,edit.isEmpty() ? QString{}:body.bodyId,true);
        session_["geometry"]=body.summary;session_["featureId"]=id;session_["bodyId"]=body.bodyId;}
    session_["documentDraft"]=candidate;session_["previewed"]=true;session_["geometryEvaluated"]=true;
    session_.remove("previewError");refreshSurfacePatchControls();return session_;
}
void AlloyWindow::refreshSurfacePatchControls(){
    if(!surfacePatchDock_ || session_.value("command")!="surface.patch")return;
    const auto inputs=session_.value("inputs").toObject();const auto id=inputs.value("featureId").toString();
    const auto source=id.isEmpty() ? document_->evaluation() : document_->evaluationBefore(id);
    for(const auto& role:{"sketchId","profileId"}){
        auto* list=surfacePatchDock_->findChild<QComboBox*>("surfacePatch."+QString(role));const QSignalBlocker blocker(list);list->clear();list->addItem("Select",QString{});
        const QString r(role);const auto selectedSketch=inputs.value("sketchId").toString();
        for(const auto& sketch:source.sketches){
            if(sketch.ownerId!=document_->activeOwnerId() && sketch.featureId!=selectedSketch)continue;
            if(r=="sketchId")list->addItem(sketch.name+" · "+sketch.featureId.left(8),sketch.featureId);
            else if(sketch.featureId==selectedSketch)for(const auto& profile:sketch.profiles)
                list->addItem("Profile · "+profile.profileId.left(8),profile.profileId);
        }list->setCurrentIndex(std::max(0,list->findData(inputs.value(role).toString())));
    }
    surfacePatchDock_->findChild<QLabel*>("surfacePatch.error")->setText(session_.value("previewError").toObject().value("message").toString());
    surfacePatchDock_->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->setEnabled(session_.value("previewed").toBool());
}
void AlloyWindow::showSurfacePatch(){
    if(surfacePatchDock_){canvasWindow_->removeDockWidget(surfacePatchDock_);delete surfacePatchDock_;}
    surfacePatchDock_=new QDockWidget("Patch",canvasWindow_);surfacePatchDock_->setObjectName("surfacePatchDock");
    auto* panel=new QWidget(surfacePatchDock_);surfacePatchDock_->setWidget(panel);auto* form=new QFormLayout(panel);
    const auto sessionId=session_.value("id").toString();
    auto update=[this,sessionId](const QJsonObject& inputs){if(session_.value("id")!=sessionId)return;execute("command.update",{{"session",sessionId},{"inputs",inputs}});};
    auto* name=new QLineEdit(session_.value("inputs").toObject().value("name").toString());name->setObjectName("surfacePatch.name");form->addRow("Name",name);
    connect(name,&QLineEdit::editingFinished,panel,[update,name]{update({{"name",name->text()}});});
    for(const auto& role:{"sketchId","profileId"}){
        auto* list=new QComboBox;list->setObjectName("surfacePatch."+QString(role));list->setMinimumContentsLength(22);list->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
        const QString r(role);form->addRow(r=="sketchId"?"Sketch":"Boundary Profile",list);
        connect(list,&QComboBox::activated,panel,[this,list,r,update]{auto inputs=session_.value("inputs").toObject();if(r=="sketchId")inputs.remove("profileId");session_["inputs"]=inputs;update({{r,list->currentData().toString()}});});
    }
    form->addRow("Operation",new QLabel("New Body"));
    auto* hint=new QLabel("One simple closed planar ordinary-Line profile. Creates one surface face, without holes or solids.");hint->setWordWrap(true);form->addRow(hint);
    auto* error=new QLabel;error->setObjectName("surfacePatch.error");error->setWordWrap(true);error->setStyleSheet("color:#b3261e");form->addRow(error);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);form->addRow(buttons);
    connect(buttons,&QDialogButtonBox::accepted,panel,[this,sessionId]{if(session_.value("id")==sessionId)execute("command.commit",{{"session",sessionId}});});
    connect(buttons,&QDialogButtonBox::rejected,panel,[this,sessionId]{if(session_.value("id")==sessionId)execute("command.cancel",{{"session",sessionId}});});
    if(!surfacePatchEscape_){surfacePatchEscape_=new QShortcut(QKeySequence(Qt::Key_Escape),this);connect(surfacePatchEscape_,&QShortcut::activated,this,[this]{if(session_.value("command")=="surface.patch")execute("command.cancel",{{"session",session_.value("id")}});});}
    surfacePatchEscape_->setEnabled(true);canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,surfacePatchDock_);surfacePatchDock_->show();refreshSurfacePatchControls();
}
