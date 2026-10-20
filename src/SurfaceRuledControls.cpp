// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include "SurfaceRuled.h"
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
QJsonObject AlloyWindow::surfaceRuledCandidate(const QJsonObject& inputs) const {
    auto args=inputs;
    if(!args.contains("featureId") && session_.value("command")=="surface.ruled"){
        args["newFeatureId"]=session_.value("newFeatureId");args["newBodyId"]=session_.value("newBodyId");
    }
    args=authoringArguments(args);if(args.contains("featureId"))args.remove("ownerId");
    return CadDocument::surfaceRuledCandidate(document_->authored(),args);
}
QJsonObject AlloyWindow::beginSurfaceRuled(const QJsonObject& overrides) {
    if(!session_.isEmpty())return failure("session_busy","Complete or cancel the active command first");
    QJsonObject inputs{{"distanceMm",20.},{"angleDeg",0.},{"ruleType","normal"},{"operation","newBody"},{"isSolid",false},{"name","Ruled"}};
    if(overrides.contains("featureId")){
        bool found=false;for(const auto& value:document_->authored().value("features").toArray()){
            const auto f=value.toObject();if(f.value("id")!=overrides.value("featureId"))continue;
            if(f.value("type")!="surfaceRuled")return failure("stale_entity","Ruled feature unavailable");
            const auto p=f.value("parameters").toObject();
            for(const auto& key:{"sourceEdge","distanceMm","angleDeg","ruleType","operation","isSolid"})inputs[key]=p.value(key);
            inputs["featureId"]=f.value("id");inputs["name"]=f.value("name");found=true;break;
        }
        if(!found)return failure("stale_entity","Ruled feature unavailable");
    }
    for(auto i=overrides.begin();i!=overrides.end();++i)inputs[i.key()]=i.value();
    const auto error=validateArguments(*findCommand("surface.ruledDialog"),inputs);if(!error.isEmpty())return failure("invalid_arguments",error);
    session_={{"id",CadDocument::newId()},{"command","surface.ruled"},{"status","active"},{"inputs",inputs},
        {"newFeatureId",CadDocument::newId()},{"newBodyId",CadDocument::newId()},
        {"baseDocumentRevision",static_cast<qint64>(document_->revision())},{"previewed",false},{"geometryEvaluated",false},
        {"inputSchema",findCommand("surface.ruled")->parameters},{"nativeControlsAvailable",false},{"nativeNumericControlsAvailable",true},{"canvasSelectionAvailable",true},{"canvasManipulatorsAvailable",false}};
    showSurfaceRuled();previewSurfaceRuled();return session_;
}
QJsonObject AlloyWindow::previewSurfaceRuled() {
    if(session_.value("command")!="surface.ruled")return failure("invalid_session","Ruled is not active");
    session_["previewed"]=false;session_["geometryEvaluated"]=false;session_.remove("geometry");session_.remove("documentDraft");view_->clearPreview();view_->clearSweepPath();view_->clearSolidEdgeHighlights();
    auto reject=[&](const QString& code,const QString& message,const QJsonObject& details=QJsonObject{}){
        auto result=failure(code,message);auto error=result.value("error").toObject();error["diagnostics"]=details;
        result["error"]=error;session_["previewError"]=error;refreshSurfaceRuledControls();return result;
    };
    if(session_.value("baseDocumentRevision").toInteger()!=static_cast<qint64>(document_->revision()))return reject("stale_session","The document changed; cancel and restart Ruled");
    const auto inputs=session_.value("inputs").toObject();const auto edit=inputs.value("featureId").toString();
    const auto source=edit.isEmpty() ? document_->evaluation() : document_->evaluationBefore(edit);
    if(!edit.isEmpty())view_->setBodyStage(source.bodies,"surface-ruled-source:"+session_.value("id").toString());
    const auto ref=inputs.value("sourceEdge").toObject();
    if(ref.isEmpty())return reject("selection_required","Select one straight boundary edge on a planar face");
    try{const auto retained=SurfaceRuled::resolveSource(source,ref);view_->selectSupportFace(ref,retained.face.face);view_->highlightSolidEdges(source.bodies,ref.value("bodyId").toString(),QJsonArray{ref.value("edgeId")});}
    catch(const GeometryKernel::EvaluationError& e){return reject(e.diagnostics().value("code").toString(),QString::fromUtf8(e.what()),e.diagnostics());}
    QJsonObject candidate;try{candidate=surfaceRuledCandidate(inputs);}catch(const std::exception& e){return reject("invalid_arguments",QString::fromUtf8(e.what()));}
    const auto evaluated=CadDocument::evaluateCandidate(candidate);if(!evaluated.ok)return reject("evaluation_failed",evaluated.error,evaluated.diagnostics);
    auto stage=evaluated.evaluation;const auto id=edit.isEmpty() ? session_.value("newFeatureId").toString() : edit;
    if(!edit.isEmpty()){
        auto prefix=candidate;QJsonArray rows;for(const auto& value:candidate.value("features").toArray()){rows.append(value);if(value.toObject().value("id")==id)break;}prefix["features"]=rows;
        const auto early=CadDocument::evaluateCandidate(prefix);if(!early.ok)return reject("evaluation_failed",early.error,early.diagnostics);stage=early.evaluation;
    }
    for(const auto& body:stage->bodies)if(body.featureId==id){view_->showPreview(body.shape,edit.isEmpty() ? QString{}:body.bodyId,true);
        session_["geometry"]=body.summary;session_["featureId"]=id;session_["bodyId"]=body.bodyId;
    }
    session_["documentDraft"]=candidate;session_["previewed"]=true;session_["geometryEvaluated"]=true;
    session_.remove("previewError");refreshSurfaceRuledControls();return session_;
}
void AlloyWindow::refreshSurfaceRuledControls(){
    if(!surfaceRuledDock_ || session_.value("command")!="surface.ruled")return;
    const auto inputs=session_.value("inputs").toObject();
    surfaceRuledDock_->findChild<QLabel*>("surfaceRuled.selection")->setText(inputs.value("sourceEdge").toObject().isEmpty()?"0 selected":"1 edge selected");
    auto* distance=surfaceRuledDock_->findChild<QDoubleSpinBox*>("surfaceRuled.distance");const QSignalBlocker blocker(distance);
    distance->setValue(inputs.value("distanceMm").toDouble()/(document_->authored().value("displayUnits")=="in"?25.4:1.));
    auto* angle=surfaceRuledDock_->findChild<QDoubleSpinBox*>("surfaceRuled.angle");const QSignalBlocker angleBlocker(angle);angle->setValue(inputs.value("angleDeg").toDouble());
    surfaceRuledDock_->findChild<QLabel*>("surfaceRuled.error")->setText(session_.value("previewError").toObject().value("message").toString());
    surfaceRuledDock_->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->setEnabled(session_.value("previewed").toBool());
}
void AlloyWindow::showSurfaceRuled(){
    if(surfaceRuledDock_){canvasWindow_->removeDockWidget(surfaceRuledDock_);delete surfaceRuledDock_;}
    surfaceRuledDock_=new QDockWidget("Ruled",canvasWindow_);surfaceRuledDock_->setObjectName("surfaceRuledDock");
    auto* panel=new QWidget;surfaceRuledDock_->setWidget(panel);auto* form=new QFormLayout(panel);
    const auto sessionId=session_.value("id").toString();
    auto update=[this,sessionId](const QJsonObject& inputs){if(session_.value("id")==sessionId)execute("command.update",{{"session",sessionId},{"inputs",inputs}});};
    auto* name=new QLineEdit(session_.value("inputs").toObject().value("name").toString());name->setObjectName("surfaceRuled.name");form->addRow("Name",name);
    connect(name,&QLineEdit::editingFinished,panel,[update,name]{update({{"name",name->text()}});});
    auto* selection=new QLabel;selection->setObjectName("surfaceRuled.selection");form->addRow("Edges",selection);
    auto* clear=new QPushButton("Clear");form->addRow(clear);connect(clear,&QPushButton::clicked,panel,[update]{update({{"sourceEdge",QJsonObject{}}});});
    auto* chain=new QCheckBox("Chain Selection");chain->setChecked(false);chain->setEnabled(false);form->addRow(chain);
    auto* type=new QComboBox;type->addItem("Normal");type->setEnabled(false);form->addRow("Type",type);
    const double scale=document_->authored().value("displayUnits")=="in"?25.4:1.;
    auto* distance=new QDoubleSpinBox;distance->setObjectName("surfaceRuled.distance");distance->setDecimals(4);distance->setRange(0.,1e100/scale);distance->setSuffix(scale==1.?" mm":" in");distance->setKeyboardTracking(false);form->addRow("Distance",distance);
    connect(distance,&QDoubleSpinBox::valueChanged,panel,[update,distance,scale]{update({{"distanceMm",distance->value()*scale}});});
    auto* angle=new QDoubleSpinBox;angle->setObjectName("surfaceRuled.angle");angle->setDecimals(4);angle->setRange(-1e100,1e100);angle->setSuffix(" deg");angle->setKeyboardTracking(false);form->addRow("Angle",angle);
    connect(angle,&QDoubleSpinBox::valueChanged,panel,[update,angle]{update({{"angleDeg",angle->value()}});});
    form->addRow("Operation",new QLabel("New Body"));
    auto* hint=new QLabel("Select one straight edge on a planar face. Distance sets flange length; Angle rotates from the face normal. Enter a signed angle to preview either side.");hint->setWordWrap(true);form->addRow(hint);
    auto* error=new QLabel;error->setObjectName("surfaceRuled.error");error->setWordWrap(true);error->setStyleSheet("color:#b3261e");form->addRow(error);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);form->addRow(buttons);
    connect(buttons,&QDialogButtonBox::accepted,panel,[this,sessionId]{execute("command.commit",{{"session",sessionId}});});
    connect(buttons,&QDialogButtonBox::rejected,panel,[this,sessionId]{execute("command.cancel",{{"session",sessionId}});});
    if(!surfaceRuledEscape_){surfaceRuledEscape_=new QShortcut(QKeySequence(Qt::Key_Escape),this);connect(surfaceRuledEscape_,&QShortcut::activated,this,[this]{if(session_.value("command")=="surface.ruled")execute("command.cancel",{{"session",session_.value("id")}});});}
    surfaceRuledEscape_->setEnabled(true);canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,surfaceRuledDock_);surfaceRuledDock_->show();refreshSurfaceRuledControls();
}
