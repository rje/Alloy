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
QJsonObject AlloyWindow::surfaceSweepCandidate(const QJsonObject& inputs) const {
    auto args=inputs;
    if(!args.contains("featureId") && session_.value("command")=="surface.sweep"){
        args["newFeatureId"]=session_.value("newFeatureId");args["newBodyId"]=session_.value("newBodyId");
    }
    args=authoringArguments(args);if(args.contains("featureId"))args.remove("ownerId");
    return CadDocument::surfaceSweepCandidate(document_->authored(),args);
}
QJsonObject AlloyWindow::beginSurfaceSweep(const QJsonObject& overrides) {
    if(!session_.isEmpty())return failure("session_busy","Complete or cancel the active command first");
    QJsonObject inputs{{"name","Sweep"}};
    if(overrides.contains("featureId")){
        bool found=false;for(const auto& value:document_->authored().value("features").toArray()){
            const auto f=value.toObject();if(f.value("id")!=overrides.value("featureId"))continue;
            if(f.value("type")!="surfaceSweep")return failure("stale_entity","Surface Sweep feature unavailable");
            const auto p=f.value("parameters").toObject();
            for(const auto& key:{"sketchId","entityId","pathSketchId","pathEntityId","pathStartPointId"})inputs[key]=p.value(key);
            inputs["featureId"]=f.value("id");inputs["name"]=f.value("name");found=true;break;
        }
        if(!found)return failure("stale_entity","Surface Sweep feature unavailable");
    }
    for(auto i=overrides.begin();i!=overrides.end();++i)inputs[i.key()]=i.value();
    const auto error=validateArguments(*findCommand("surface.sweepDialog"),inputs);if(!error.isEmpty())return failure("invalid_arguments",error);
    session_={{"id",CadDocument::newId()},{"command","surface.sweep"},{"status","active"},{"inputs",inputs},
        {"newFeatureId",CadDocument::newId()},{"newBodyId",CadDocument::newId()},
        {"baseDocumentRevision",static_cast<qint64>(document_->revision())},{"previewed",false},{"geometryEvaluated",false},
        {"inputSchema",findCommand("surface.sweep")->parameters},{"nativeControlsAvailable",true}};
    showSurfaceSweep();previewSurfaceSweep();return session_;
}
QJsonObject AlloyWindow::previewSurfaceSweep() {
    if(session_.value("command")!="surface.sweep")return failure("invalid_session","Surface Sweep is not active");
    session_["previewed"]=false;session_["geometryEvaluated"]=false;session_.remove("geometry");session_.remove("documentDraft");view_->clearPreview();view_->clearSweepPath();
    auto reject=[&](const QString& code,const QString& message,const QJsonObject& details=QJsonObject{}){
        auto result=failure(code,message);auto error=result.value("error").toObject();error["diagnostics"]=details;
        result["error"]=error;session_["previewError"]=error;refreshSurfaceSweepControls();return result;
    };
    if(session_.value("baseDocumentRevision").toInteger()!=static_cast<qint64>(document_->revision()))return reject("stale_session","The document changed; cancel and restart Surface Sweep");
    const auto inputs=session_.value("inputs").toObject();const auto edit=inputs.value("featureId").toString();
    const auto source=edit.isEmpty() ? document_->evaluation() : document_->evaluationBefore(edit);
    if(!edit.isEmpty())view_->setBodyStage(source.bodies,"surface-sweep-source:"+session_.value("id").toString());
    const auto sketchId=inputs.value("sketchId").toString(),pathId=inputs.value("pathSketchId").toString();view_->setCommandSketches(QSet<QString>{sketchId,pathId});
    for(const auto& sketch:source.sketches){QJsonArray ids;if(sketch.featureId==sketchId)ids.append(inputs.value("entityId"));if(sketch.featureId==pathId)ids.append(inputs.value("pathEntityId"));if(!ids.isEmpty())view_->showSweepPath(sketch,ids);}
    for(const auto& key:{"sketchId","entityId","pathSketchId","pathEntityId","pathStartPointId"})if(inputs.value(key).toString().isEmpty())return reject("selection_required","Select one open Line section, one Line or Arc path and its start endpoint");
    QJsonObject candidate;try{candidate=surfaceSweepCandidate(inputs);}catch(const std::exception& e){return reject("invalid_arguments",QString::fromUtf8(e.what()));}
    const auto evaluated=CadDocument::evaluateCandidate(candidate);if(!evaluated.ok)return reject("evaluation_failed",evaluated.error,evaluated.diagnostics);
    auto stage=evaluated.evaluation;const auto id=edit.isEmpty() ? session_.value("newFeatureId").toString() : edit;
    if(!edit.isEmpty()){
        auto prefix=candidate;QJsonArray rows;for(const auto& value:candidate.value("features").toArray()){rows.append(value);if(value.toObject().value("id")==id)break;}prefix["features"]=rows;
        const auto early=CadDocument::evaluateCandidate(prefix);if(!early.ok)return reject("evaluation_failed",early.error,early.diagnostics);stage=early.evaluation;
    }
    for(const auto& body:stage->bodies)if(body.featureId==id){view_->showPreview(body.shape,edit.isEmpty() ? QString{}:body.bodyId,true);
        session_["geometry"]=body.summary;session_["featureId"]=id;session_["bodyId"]=body.bodyId;}
    session_["documentDraft"]=candidate;session_["previewed"]=true;session_["geometryEvaluated"]=true;
    session_.remove("previewError");refreshSurfaceSweepControls();return session_;
}
void AlloyWindow::refreshSurfaceSweepControls(){
    if(!surfaceSweepDock_ || session_.value("command")!="surface.sweep")return;
    const auto inputs=session_.value("inputs").toObject();const auto id=inputs.value("featureId").toString();
    const auto source=id.isEmpty() ? document_->evaluation() : document_->evaluationBefore(id);
    for(const auto& role:{"sketchId","entityId","pathSketchId","pathEntityId","pathStartPointId"}){
        auto* list=surfaceSweepDock_->findChild<QComboBox*>("surfaceSweep."+QString(role));const QSignalBlocker blocker(list);list->clear();list->addItem("Select",QString{});
        const QString r(role);const bool pathRole=r.startsWith("path");const auto selectedSketch=inputs.value(pathRole?"pathSketchId":"sketchId").toString();
        for(const auto& sketch:source.sketches){
            if(sketch.ownerId!=document_->activeOwnerId() && sketch.featureId!=selectedSketch)continue;
            if(r=="sketchId" || r=="pathSketchId")list->addItem(sketch.name+" · "+sketch.featureId.left(8),sketch.featureId);
            else if(sketch.featureId==selectedSketch)for(const auto& value:sketch.summary.value("entities").toArray()){
                const auto e=value.toObject();if(e.value("construction").toBool() || (e.value("type")!="line" && !(pathRole && e.value("type")=="arc")))continue;
                if(r=="pathStartPointId"){if(e.value("id")!=inputs.value("pathEntityId"))continue;list->addItem("Start · "+e.value("startPointId").toString().left(8),e.value("startPointId").toString());list->addItem("End · "+e.value("endPointId").toString().left(8),e.value("endPointId").toString());}
                else list->addItem(e.value("type").toString()+" · "+e.value("id").toString().left(8),e.value("id").toString());
            }
        }list->setCurrentIndex(std::max(0,list->findData(inputs.value(role).toString())));
    }
    surfaceSweepDock_->findChild<QLabel*>("surfaceSweep.error")->setText(session_.value("previewError").toObject().value("message").toString());
    surfaceSweepDock_->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->setEnabled(session_.value("previewed").toBool());
}
void AlloyWindow::showSurfaceSweep(){
    if(surfaceSweepDock_){canvasWindow_->removeDockWidget(surfaceSweepDock_);delete surfaceSweepDock_;}
    surfaceSweepDock_=new QDockWidget("Sweep",canvasWindow_);surfaceSweepDock_->setObjectName("surfaceSweepDock");
    auto* panel=new QWidget(surfaceSweepDock_);surfaceSweepDock_->setWidget(panel);auto* form=new QFormLayout(panel);
    const auto sessionId=session_.value("id").toString();
    auto update=[this,sessionId](const QJsonObject& inputs){if(session_.value("id")!=sessionId)return;execute("command.update",{{"session",sessionId},{"inputs",inputs}});};
    auto* name=new QLineEdit(session_.value("inputs").toObject().value("name").toString());name->setObjectName("surfaceSweep.name");form->addRow("Name",name);
    connect(name,&QLineEdit::editingFinished,panel,[update,name]{update({{"name",name->text()}});});
    for(const auto& role:{"sketchId","entityId","pathSketchId","pathEntityId","pathStartPointId"}){
        auto* list=new QComboBox;list->setObjectName("surfaceSweep."+QString(role));list->setMinimumContentsLength(22);list->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
        const QString r(role);form->addRow(r=="sketchId"?"Profile Sketch":r=="entityId"?"Profile":r=="pathSketchId"?"Path Sketch":r=="pathEntityId"?"Path":"Start Point",list);
        connect(list,&QComboBox::activated,panel,[this,list,r,update]{auto inputs=session_.value("inputs").toObject();if(r=="sketchId")inputs.remove("entityId");if(r=="pathSketchId"){inputs.remove("pathEntityId");inputs.remove("pathStartPointId");}if(r=="pathEntityId")inputs.remove("pathStartPointId");session_["inputs"]=inputs;update({{r,list->currentData().toString()}});});
    }
    form->addRow("Type",new QLabel("Single Path"));form->addRow("Distance",new QLabel("100 %"));form->addRow("Orientation",new QLabel("Perpendicular"));
    form->addRow("Operation",new QLabel("New Body"));
    auto* hint=new QLabel("One open Line centered at the retained path start, perpendicular to its tangent. One Line or Arc path. The sheet has no caps.");hint->setWordWrap(true);form->addRow(hint);
    auto* error=new QLabel;error->setObjectName("surfaceSweep.error");error->setWordWrap(true);error->setStyleSheet("color:#b3261e");form->addRow(error);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);form->addRow(buttons);
    connect(buttons,&QDialogButtonBox::accepted,panel,[this,sessionId]{if(session_.value("id")==sessionId)execute("command.commit",{{"session",sessionId}});});
    connect(buttons,&QDialogButtonBox::rejected,panel,[this,sessionId]{if(session_.value("id")==sessionId)execute("command.cancel",{{"session",sessionId}});});
    if(!surfaceSweepEscape_){surfaceSweepEscape_=new QShortcut(QKeySequence(Qt::Key_Escape),this);connect(surfaceSweepEscape_,&QShortcut::activated,this,[this]{if(session_.value("command")=="surface.sweep")execute("command.cancel",{{"session",session_.value("id")}});});}
    surfaceSweepEscape_->setEnabled(true);canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,surfaceSweepDock_);surfaceSweepDock_->show();refreshSurfaceSweepControls();
}
