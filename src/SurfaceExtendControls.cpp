// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include "SurfaceExtend.h"
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
QJsonObject AlloyWindow::surfaceExtendCandidate(const QJsonObject& inputs) const {
    auto args=inputs;
    if(!args.contains("featureId") && session_.value("command")=="surface.extend"){
        args["newFeatureId"]=session_.value("newFeatureId");
    }
    args=authoringArguments(args);if(args.contains("featureId"))args.remove("ownerId");
    return CadDocument::surfaceExtendCandidate(document_->authored(),args);
}
QJsonObject AlloyWindow::beginSurfaceExtend(const QJsonObject& overrides) {
    if(!session_.isEmpty())return failure("session_busy","Complete or cancel the active command first");
    QJsonObject inputs{{"distanceMm",20.},{"extendType","natural"},{"edgeAlignment","freeEdges"},{"chaining",false},{"name","Extend"}};
    if(overrides.contains("featureId")){
        bool found=false;for(const auto& value:document_->authored().value("features").toArray()){
            const auto f=value.toObject();if(f.value("id")!=overrides.value("featureId"))continue;
            if(f.value("type")!="surfaceExtend")return failure("stale_entity","Extend feature unavailable");
            const auto p=f.value("parameters").toObject();
            for(const auto& key:{"sourceEdge","distanceMm","extendType","edgeAlignment","chaining"})inputs[key]=p.value(key);
            inputs["featureId"]=f.value("id");inputs["name"]=f.value("name");found=true;break;
        }
        if(!found)return failure("stale_entity","Extend feature unavailable");
    }
    for(auto i=overrides.begin();i!=overrides.end();++i)inputs[i.key()]=i.value();
    const auto error=validateArguments(*findCommand("surface.extendDialog"),inputs);if(!error.isEmpty())return failure("invalid_arguments",error);
    session_={{"id",CadDocument::newId()},{"command","surface.extend"},{"status","active"},{"inputs",inputs},
        {"newFeatureId",CadDocument::newId()},
        {"baseDocumentRevision",static_cast<qint64>(document_->revision())},{"previewed",false},{"geometryEvaluated",false},
        {"inputSchema",findCommand("surface.extend")->parameters},{"nativeControlsAvailable",false},{"nativeNumericControlsAvailable",true},{"canvasSelectionAvailable",true},{"canvasManipulatorsAvailable",false}};
    showSurfaceExtend();previewSurfaceExtend();return session_;
}
QJsonObject AlloyWindow::previewSurfaceExtend() {
    if(session_.value("command")!="surface.extend")return failure("invalid_session","Extend is not active");
    session_["previewed"]=false;session_["geometryEvaluated"]=false;session_.remove("geometry");session_.remove("documentDraft");view_->clearPreview();view_->clearSweepPath();view_->clearSolidEdgeHighlights();
    auto reject=[&](const QString& code,const QString& message,const QJsonObject& details=QJsonObject{}){
        auto result=failure(code,message);auto error=result.value("error").toObject();error["diagnostics"]=details;
        result["error"]=error;session_["previewError"]=error;refreshSurfaceExtendControls();return result;
    };
    if(session_.value("baseDocumentRevision").toInteger()!=static_cast<qint64>(document_->revision()))return reject("stale_session","The document changed; cancel and restart Extend");
    const auto inputs=session_.value("inputs").toObject();const auto edit=inputs.value("featureId").toString();
    const auto source=edit.isEmpty() ? document_->evaluation() : document_->evaluationBefore(edit);
    if(!edit.isEmpty())view_->setBodyStage(source.bodies,"surface-extend-source:"+session_.value("id").toString());
    const auto ref=inputs.value("sourceEdge").toObject();
    if(ref.isEmpty())return reject("selection_required","Select one straight boundary edge on a rectangular planar Patch sheet");
    try{const auto retained=SurfaceExtend::resolveSource(source,ref);view_->selectSupportFace(ref,retained.face.face);view_->highlightSolidEdges(source.bodies,ref.value("bodyId").toString(),QJsonArray{ref.value("edgeId")});}
    catch(const GeometryKernel::EvaluationError& e){return reject(e.diagnostics().value("code").toString(),QString::fromUtf8(e.what()),e.diagnostics());}
    QJsonObject candidate;try{candidate=surfaceExtendCandidate(inputs);}catch(const std::exception& e){return reject("invalid_arguments",QString::fromUtf8(e.what()));}
    const auto evaluated=CadDocument::evaluateCandidate(candidate);if(!evaluated.ok)return reject("evaluation_failed",evaluated.error,evaluated.diagnostics);
    auto stage=evaluated.evaluation;const auto id=edit.isEmpty() ? session_.value("newFeatureId").toString() : edit;
    if(!edit.isEmpty()){
        auto prefix=candidate;QJsonArray rows;for(const auto& value:candidate.value("features").toArray()){rows.append(value);if(value.toObject().value("id")==id)break;}prefix["features"]=rows;
        const auto early=CadDocument::evaluateCandidate(prefix);if(!early.ok)return reject("evaluation_failed",early.error,early.diagnostics);stage=early.evaluation;
    }
    for(const auto& body:stage->bodies)if(body.bodyId==ref.value("bodyId").toString()){view_->showPreview(body.shape,body.bodyId,true);
        session_["geometry"]=body.summary;session_["featureId"]=id;session_["bodyId"]=body.bodyId;
    }
    session_["documentDraft"]=candidate;session_["previewed"]=true;session_["geometryEvaluated"]=true;
    session_.remove("previewError");refreshSurfaceExtendControls();return session_;
}
void AlloyWindow::refreshSurfaceExtendControls(){
    if(!surfaceExtendDock_ || session_.value("command")!="surface.extend")return;
    const auto inputs=session_.value("inputs").toObject();
    surfaceExtendDock_->findChild<QLabel*>("surfaceExtend.selection")->setText(inputs.value("sourceEdge").toObject().isEmpty()?"0 selected":"1 edge selected");
    auto* distance=surfaceExtendDock_->findChild<QDoubleSpinBox*>("surfaceExtend.distance");const QSignalBlocker blocker(distance);
    distance->setValue(inputs.value("distanceMm").toDouble()/(document_->authored().value("displayUnits")=="in"?25.4:1.));
    surfaceExtendDock_->findChild<QLabel*>("surfaceExtend.error")->setText(session_.value("previewError").toObject().value("message").toString());
    surfaceExtendDock_->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->setEnabled(session_.value("previewed").toBool());
}
void AlloyWindow::showSurfaceExtend(){
    if(surfaceExtendDock_){canvasWindow_->removeDockWidget(surfaceExtendDock_);delete surfaceExtendDock_;}
    surfaceExtendDock_=new QDockWidget("Extend",canvasWindow_);surfaceExtendDock_->setObjectName("surfaceExtendDock");
    auto* panel=new QWidget;surfaceExtendDock_->setWidget(panel);auto* form=new QFormLayout(panel);
    const auto sessionId=session_.value("id").toString();
    auto update=[this,sessionId](const QJsonObject& inputs){if(session_.value("id")==sessionId)execute("command.update",{{"session",sessionId},{"inputs",inputs}});};
    auto* name=new QLineEdit(session_.value("inputs").toObject().value("name").toString());name->setObjectName("surfaceExtend.name");form->addRow("Name",name);
    connect(name,&QLineEdit::editingFinished,panel,[update,name]{update({{"name",name->text()}});});
    auto* selection=new QLabel;selection->setObjectName("surfaceExtend.selection");form->addRow("Edges",selection);
    auto* clear=new QPushButton("Clear");form->addRow(clear);connect(clear,&QPushButton::clicked,panel,[update]{update({{"sourceEdge",QJsonObject{}}});});
    auto* chain=new QCheckBox("Enable Chaining");chain->setObjectName("surfaceExtend.chaining");chain->setChecked(false);chain->setEnabled(false);form->addRow(chain);
    auto* type=new QComboBox;type->setObjectName("surfaceExtend.extendType");type->addItem("Natural");type->setEnabled(false);form->addRow("Extend Type",type);
    auto* alignment=new QComboBox;alignment->setObjectName("surfaceExtend.edgeAlignment");alignment->addItem("Free Edges");alignment->setEnabled(false);form->addRow("Edge Alignment",alignment);
    const double scale=document_->authored().value("displayUnits")=="in"?25.4:1.;
    auto* distance=new QDoubleSpinBox;distance->setObjectName("surfaceExtend.distance");distance->setDecimals(4);distance->setRange(0.,1e100/scale);distance->setSuffix(scale==1.?" mm":" in");distance->setKeyboardTracking(false);form->addRow("Distance",distance);
    connect(distance,&QDoubleSpinBox::valueChanged,panel,[update,distance,scale]{update({{"distanceMm",distance->value()*scale}});});
    auto* hint=new QLabel("Select one straight outer edge of a rectangular planar Patch sheet. Positive Distance extends the sheet outward in its plane.");hint->setWordWrap(true);form->addRow(hint);
    auto* error=new QLabel;error->setObjectName("surfaceExtend.error");error->setWordWrap(true);error->setStyleSheet("color:#b3261e");form->addRow(error);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);form->addRow(buttons);
    connect(buttons,&QDialogButtonBox::accepted,panel,[this,sessionId]{execute("command.commit",{{"session",sessionId}});});
    connect(buttons,&QDialogButtonBox::rejected,panel,[this,sessionId]{execute("command.cancel",{{"session",sessionId}});});
    if(!surfaceExtendEscape_){surfaceExtendEscape_=new QShortcut(QKeySequence(Qt::Key_Escape),this);connect(surfaceExtendEscape_,&QShortcut::activated,this,[this]{if(session_.value("command")=="surface.extend")execute("command.cancel",{{"session",session_.value("id")}});});}
    surfaceExtendEscape_->setEnabled(true);canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,surfaceExtendDock_);surfaceExtendDock_->show();refreshSurfaceExtendControls();
}
