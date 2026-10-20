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
QJsonObject AlloyWindow::surfaceOffsetCandidate(const QJsonObject& inputs) const {
    auto args=inputs;
    if(!args.contains("featureId") && session_.value("command")=="surface.offset"){
        args["newFeatureId"]=session_.value("newFeatureId");args["newBodyId"]=session_.value("newBodyId");
    }
    args=authoringArguments(args);if(args.contains("featureId"))args.remove("ownerId");
    return CadDocument::surfaceOffsetCandidate(document_->authored(),args);
}
QJsonObject AlloyWindow::beginSurfaceOffset(const QJsonObject& overrides) {
    if(!session_.isEmpty())return failure("session_busy","Complete or cancel the active command first");
    QJsonObject inputs{{"distanceMm",0.},{"name","Offset"}};
    if(overrides.contains("featureId")){
        bool found=false;for(const auto& value:document_->authored().value("features").toArray()){
            const auto f=value.toObject();if(f.value("id")!=overrides.value("featureId"))continue;
            if(f.value("type")!="surfaceOffset")return failure("stale_entity","Surface Offset feature unavailable");
            const auto p=f.value("parameters").toObject();
            for(const auto& key:{"sourceFace","distanceMm"})inputs[key]=p.value(key);
            inputs["featureId"]=f.value("id");inputs["name"]=f.value("name");found=true;break;
        }
        if(!found)return failure("stale_entity","Surface Offset feature unavailable");
    }
    for(auto i=overrides.begin();i!=overrides.end();++i)inputs[i.key()]=i.value();
    const auto error=validateArguments(*findCommand("surface.offsetDialog"),inputs);if(!error.isEmpty())return failure("invalid_arguments",error);
    session_={{"id",CadDocument::newId()},{"command","surface.offset"},{"status","active"},{"inputs",inputs},
        {"newFeatureId",CadDocument::newId()},{"newBodyId",CadDocument::newId()},
        {"baseDocumentRevision",static_cast<qint64>(document_->revision())},{"previewed",false},{"geometryEvaluated",false},
        {"inputSchema",findCommand("surface.offset")->parameters},{"nativeControlsAvailable",true}};
    showSurfaceOffset();previewSurfaceOffset();return session_;
}
QJsonObject AlloyWindow::previewSurfaceOffset() {
    if(session_.value("command")!="surface.offset")return failure("invalid_session","Surface Offset is not active");
    session_["previewed"]=false;session_["geometryEvaluated"]=false;session_.remove("geometry");session_.remove("documentDraft");view_->clearPreview();view_->clearSweepPath();
    auto reject=[&](const QString& code,const QString& message,const QJsonObject& details=QJsonObject{}){
        auto result=failure(code,message);auto error=result.value("error").toObject();error["diagnostics"]=details;
        result["error"]=error;session_["previewError"]=error;refreshSurfaceOffsetControls();return result;
    };
    if(session_.value("baseDocumentRevision").toInteger()!=static_cast<qint64>(document_->revision()))return reject("stale_session","The document changed; cancel and restart Surface Offset");
    const auto inputs=session_.value("inputs").toObject();const auto edit=inputs.value("featureId").toString();
    const auto source=edit.isEmpty() ? document_->evaluation() : document_->evaluationBefore(edit);
    if(!edit.isEmpty())view_->setBodyStage(source.bodies,"surface-offset-source:"+session_.value("id").toString());
    const auto ref=inputs.value("sourceFace").toObject();
    if(ref.isEmpty())return reject("selection_required","Select one retained planar face in the canvas");
    try{const auto face=GeometryKernel::resolveSketchSupport(source,ref);view_->selectSupportFace(ref,face.face);}
    catch(const std::exception& e){return reject("surface_source_unavailable",QString::fromUtf8(e.what()),{{"sourceFace",ref},{"faceId",ref.value("faceId")}});}
    QJsonObject candidate;try{candidate=surfaceOffsetCandidate(inputs);}catch(const std::exception& e){return reject("invalid_arguments",QString::fromUtf8(e.what()));}
    const auto evaluated=CadDocument::evaluateCandidate(candidate);if(!evaluated.ok)return reject("evaluation_failed",evaluated.error,evaluated.diagnostics);
    auto stage=evaluated.evaluation;const auto id=edit.isEmpty() ? session_.value("newFeatureId").toString() : edit;
    if(!edit.isEmpty()){
        auto prefix=candidate;QJsonArray rows;for(const auto& value:candidate.value("features").toArray()){rows.append(value);if(value.toObject().value("id")==id)break;}prefix["features"]=rows;
        const auto early=CadDocument::evaluateCandidate(prefix);if(!early.ok)return reject("evaluation_failed",early.error,early.diagnostics);stage=early.evaluation;
    }
    for(const auto& body:stage->bodies)if(body.featureId==id){view_->showPreview(body.shape,edit.isEmpty() ? QString{}:body.bodyId,true);
        session_["geometry"]=body.summary;session_["featureId"]=id;session_["bodyId"]=body.bodyId;
        try{const auto face=GeometryKernel::resolveSketchSupport(source,ref);view_->showFaceOffsetCue(face,inputs.value("distanceMm").toDouble(),document_->authored().value("displayUnits").toString(),"surface.offset");}catch(const std::exception&){} }
    session_["documentDraft"]=candidate;session_["previewed"]=true;session_["geometryEvaluated"]=true;
    session_.remove("previewError");refreshSurfaceOffsetControls();return session_;
}
QJsonObject AlloyWindow::selectSurfaceOffsetFace(const QJsonObject& face){
    if(!face.value("occurrenceId").toString().isEmpty())return failure("unsupported_instance_edit","Select the retained source face");
    QJsonObject exact{{"bodyId",face.value("bodyId")},{"producerFeatureId",face.value("producerFeatureId")},{"faceId",face.value("faceId")}};
    selectedFace_=exact;selectedBody_.clear();selectedOccurrence_.clear();selectedEdge_={};selectedSketch_.clear();selectedProfile_.clear();selectedProfileIds_={};selectedPlane_.clear();
    view_->selectBody({});view_->selectProfile({},{});view_->clearSolidEdgeHighlights();
    return execute("command.update",{{"session",session_.value("id")},{"inputs",QJsonObject{{"sourceFace",exact}}}});
}
void AlloyWindow::refreshSurfaceOffsetControls(){
    if(!surfaceOffsetDock_ || session_.value("command")!="surface.offset")return;
    const auto inputs=session_.value("inputs").toObject();
    surfaceOffsetDock_->findChild<QLabel*>("surfaceOffset.selection")->setText(inputs.value("sourceFace").toObject().isEmpty()?"0 selected":"1 face selected");
    auto* distance=surfaceOffsetDock_->findChild<QDoubleSpinBox*>("surfaceOffset.distance");const QSignalBlocker blocker(distance);
    distance->setValue(inputs.value("distanceMm").toDouble()/(document_->authored().value("displayUnits")=="in"?25.4:1.));
    surfaceOffsetDock_->findChild<QLabel*>("surfaceOffset.error")->setText(session_.value("previewError").toObject().value("message").toString());
    surfaceOffsetDock_->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->setEnabled(session_.value("previewed").toBool());
}
void AlloyWindow::showSurfaceOffset(){
    if(surfaceOffsetDock_){canvasWindow_->removeDockWidget(surfaceOffsetDock_);delete surfaceOffsetDock_;}
    surfaceOffsetDock_=new QDockWidget("Offset",canvasWindow_);surfaceOffsetDock_->setObjectName("surfaceOffsetDock");
    auto* panel=new QWidget;surfaceOffsetDock_->setWidget(panel);auto* form=new QFormLayout(panel);
    const auto sessionId=session_.value("id").toString();
    auto update=[this,sessionId](const QJsonObject& inputs){if(session_.value("id")==sessionId)execute("command.update",{{"session",sessionId},{"inputs",inputs}});};
    auto* name=new QLineEdit(session_.value("inputs").toObject().value("name").toString());name->setObjectName("surfaceOffset.name");form->addRow("Name",name);
    connect(name,&QLineEdit::editingFinished,panel,[update,name]{update({{"name",name->text()}});});
    auto* selection=new QLabel;selection->setObjectName("surfaceOffset.selection");form->addRow("Faces",selection);
    auto* clear=new QPushButton("Clear");form->addRow(clear);connect(clear,&QPushButton::clicked,panel,[update]{update({{"sourceFace",QJsonObject{}}});});
    auto* chain=new QCheckBox("Chain Selection");chain->setChecked(false);chain->setEnabled(false);form->addRow(chain);
    form->addRow("Type",new QLabel("Sharp"));
    const double scale=document_->authored().value("displayUnits")=="in"?25.4:1.;
    auto* distance=new QDoubleSpinBox;distance->setObjectName("surfaceOffset.distance");distance->setDecimals(4);distance->setRange(-100000/scale,100000/scale);distance->setSuffix(scale==1.?" mm":" in");distance->setKeyboardTracking(false);form->addRow("Distance",distance);
    connect(distance,&QDoubleSpinBox::valueChanged,panel,[update,distance,scale]{update({{"distanceMm",distance->value()*scale}});});
    form->addRow("Operation",new QLabel("New Body"));
    auto* hint=new QLabel("Select one planar face. Positive follows the normal arrow; negative reverses it. Zero makes a coincident sheet.");hint->setWordWrap(true);form->addRow(hint);
    auto* error=new QLabel;error->setObjectName("surfaceOffset.error");error->setWordWrap(true);error->setStyleSheet("color:#b3261e");form->addRow(error);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);form->addRow(buttons);
    connect(buttons,&QDialogButtonBox::accepted,panel,[this,sessionId]{execute("command.commit",{{"session",sessionId}});});
    connect(buttons,&QDialogButtonBox::rejected,panel,[this,sessionId]{execute("command.cancel",{{"session",sessionId}});});
    if(!surfaceOffsetEscape_){surfaceOffsetEscape_=new QShortcut(QKeySequence(Qt::Key_Escape),this);connect(surfaceOffsetEscape_,&QShortcut::activated,this,[this]{if(session_.value("command")=="surface.offset")execute("command.cancel",{{"session",session_.value("id")}});});}
    surfaceOffsetEscape_->setEnabled(true);canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,surfaceOffsetDock_);surfaceOffsetDock_->show();refreshSurfaceOffsetControls();
}
