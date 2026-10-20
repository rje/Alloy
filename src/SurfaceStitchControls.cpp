// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include "SurfaceStitch.h"
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
QJsonObject AlloyWindow::surfaceStitchCandidate(const QJsonObject& inputs) const {
    auto args=inputs;
    if(!args.contains("featureId") && session_.value("command")=="surface.stitch"){
        args["newFeatureId"]=session_.value("newFeatureId");args["newBodyId"]=session_.value("newBodyId");
    }
    args=authoringArguments(args);if(args.contains("featureId"))args.remove("ownerId");
    return CadDocument::surfaceStitchCandidate(document_->authored(),args);
}
QJsonObject AlloyWindow::beginSurfaceStitch(const QJsonObject& overrides) {
    if(!session_.isEmpty())return failure("session_busy","Complete or cancel the active command first");
    QJsonObject inputs{{"sourceFaces",QJsonArray{}},{"toleranceMm",0.01},{"name","Stitch"}};
    if(overrides.contains("featureId")){
        bool found=false;for(const auto& value:document_->authored().value("features").toArray()){
            const auto f=value.toObject();if(f.value("id")!=overrides.value("featureId"))continue;
            if(f.value("type")!="surfaceStitch")return failure("stale_entity","Stitch feature unavailable");
            const auto p=f.value("parameters").toObject();
            for(const auto& key:{"sourceFaces","toleranceMm"})inputs[key]=p.value(key);
            inputs["featureId"]=f.value("id");inputs["name"]=f.value("name");found=true;break;
        }
        if(!found)return failure("stale_entity","Stitch feature unavailable");
    }
    for(auto i=overrides.begin();i!=overrides.end();++i)inputs[i.key()]=i.value();
    const auto error=validateArguments(*findCommand("surface.stitchDialog"),inputs);if(!error.isEmpty())return failure("invalid_arguments",error);
    session_={{"id",CadDocument::newId()},{"command","surface.stitch"},{"status","active"},{"inputs",inputs},
        {"newFeatureId",CadDocument::newId()},{"newBodyId",CadDocument::newId()},
        {"baseDocumentRevision",static_cast<qint64>(document_->revision())},{"previewed",false},{"geometryEvaluated",false},
        {"inputSchema",findCommand("surface.stitch")->parameters},{"nativeControlsAvailable",true},{"canvasSelectionAvailable",true},{"canvasManipulatorsAvailable",false}};
    showSurfaceStitch();previewSurfaceStitch();return session_;
}
QJsonObject AlloyWindow::previewSurfaceStitch() {
    if(session_.value("command")!="surface.stitch")return failure("invalid_session","Stitch is not active");
    session_["previewed"]=false;session_["geometryEvaluated"]=false;session_.remove("geometry");session_.remove("documentDraft");view_->clearPreview();view_->clearSweepPath();view_->clearBodyStage();
    auto reject=[&](const QString& code,const QString& message,const QJsonObject& details=QJsonObject{}){
        auto result=failure(code,message);auto error=result.value("error").toObject();error["diagnostics"]=details;
        result["error"]=error;session_["previewError"]=error;refreshSurfaceStitchControls();return result;
    };
    if(session_.value("baseDocumentRevision").toInteger()!=static_cast<qint64>(document_->revision()))return reject("stale_session","The document changed; cancel and restart Stitch");
    const auto inputs=session_.value("inputs").toObject();const auto edit=inputs.value("featureId").toString();
    const auto source=edit.isEmpty() ? document_->evaluation() : document_->evaluationBefore(edit);
    if(!edit.isEmpty())view_->setBodyStage(source.bodies,"surface-stitch-source:"+session_.value("id").toString());
    const auto refs=inputs.value("sourceFaces").toArray();
    if(refs.size()!=2)return reject("selection_required","Select two finite planar rectangular sheet faces");
    QJsonObject candidate;try{candidate=surfaceStitchCandidate(inputs);}catch(const std::exception& e){return reject("invalid_arguments",QString::fromUtf8(e.what()));}
    const auto evaluated=CadDocument::evaluateCandidate(candidate);if(!evaluated.ok)return reject("evaluation_failed",evaluated.error,evaluated.diagnostics);
    auto stage=evaluated.evaluation;const auto id=edit.isEmpty() ? session_.value("newFeatureId").toString() : edit;
    if(!edit.isEmpty()){
        auto prefix=candidate;QJsonArray rows;for(const auto& value:candidate.value("features").toArray()){rows.append(value);if(value.toObject().value("id")==id)break;}prefix["features"]=rows;
        const auto early=CadDocument::evaluateCandidate(prefix);if(!early.ok)return reject("evaluation_failed",early.error,early.diagnostics);stage=early.evaluation;
    }
    view_->setBodyStage(stage->bodies,"surface-stitch-preview:"+session_.value("id").toString());
    for(const auto& body:stage->bodies)if(body.featureId==id){view_->showPreview(body.shape,body.bodyId,true);
        session_["geometry"]=body.summary;session_["featureId"]=id;session_["bodyId"]=body.bodyId;
    }

    session_["documentDraft"]=candidate;session_["previewed"]=true;session_["geometryEvaluated"]=true;
    session_.remove("previewError");refreshSurfaceStitchControls();return session_;
}
QJsonObject AlloyWindow::selectSurfaceStitchFace(const QJsonObject& face){
    if(!face.value("occurrenceId").toString().isEmpty())return failure("unsupported_instance_edit","Select the retained source face");
    QJsonObject exact{{"bodyId",face.value("bodyId")},{"producerFeatureId",face.value("producerFeatureId")},{"faceId",face.value("faceId")}};
    selectedFace_=exact;selectedBody_.clear();selectedOccurrence_.clear();selectedEdge_={};selectedSketch_.clear();selectedProfile_.clear();selectedProfileIds_={};selectedPlane_.clear();
    view_->selectBody({});view_->selectProfile({},{});view_->clearSolidEdgeHighlights();
    auto refs=session_.value("inputs").toObject().value("sourceFaces").toArray();
    for(int i=0;i<refs.size();++i)if(refs[i].toObject().value("bodyId")==exact.value("bodyId")){refs.removeAt(i);return execute("command.update",{{"session",session_.value("id")},{"inputs",QJsonObject{{"sourceFaces",refs}}}});}
    if(refs.size()>=2)return failure("selection_full","Clear a selected face before choosing another");
    refs.append(exact);
    return execute("command.update",{{"session",session_.value("id")},{"inputs",QJsonObject{{"sourceFaces",refs}}}});
}
void AlloyWindow::refreshSurfaceStitchControls(){
    if(!surfaceStitchDock_ || session_.value("command")!="surface.stitch")return;
    const auto inputs=session_.value("inputs").toObject();
    surfaceStitchDock_->findChild<QLabel*>("surfaceStitch.selection")->setText(QString("%1 faces selected").arg(inputs.value("sourceFaces").toArray().size()));
    auto* distance=surfaceStitchDock_->findChild<QDoubleSpinBox*>("surfaceStitch.tolerance");const QSignalBlocker blocker(distance);
    distance->setValue(inputs.value("toleranceMm").toDouble()/(document_->authored().value("displayUnits")=="in"?25.4:1.));
    const auto geometry=session_.value("geometry").toObject();
    surfaceStitchDock_->findChild<QLabel*>("surfaceStitch.status")->setText(geometry.isEmpty()?QString{}:QString("Gap: %1 mm\nSewn edges: %2; free edges: %3\nResult: valid open shell\nB-rep tolerance: %4 → %5 mm").arg(geometry.value("measuredSeamGapMm").toDouble(),0,'g',8).arg(geometry.value("sewnEdgeCount").toInt()).arg(geometry.value("freeEdgeCount").toInt()).arg(geometry.value("maxInputToleranceMm").toDouble(),0,'g',8).arg(geometry.value("maxOutputToleranceMm").toDouble(),0,'g',8));
    surfaceStitchDock_->findChild<QLabel*>("surfaceStitch.error")->setText(session_.value("previewError").toObject().value("message").toString());
    surfaceStitchDock_->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->setEnabled(session_.value("previewed").toBool());
}
void AlloyWindow::showSurfaceStitch(){
    if(surfaceStitchDock_){canvasWindow_->removeDockWidget(surfaceStitchDock_);delete surfaceStitchDock_;}
    surfaceStitchDock_=new QDockWidget("Stitch",canvasWindow_);surfaceStitchDock_->setObjectName("surfaceStitchDock");
    auto* panel=new QWidget;surfaceStitchDock_->setWidget(panel);auto* form=new QFormLayout(panel);
    const auto sessionId=session_.value("id").toString();
    auto update=[this,sessionId](const QJsonObject& inputs){if(session_.value("id")==sessionId)execute("command.update",{{"session",sessionId},{"inputs",inputs}});};
    auto* name=new QLineEdit(session_.value("inputs").toObject().value("name").toString());name->setObjectName("surfaceStitch.name");form->addRow("Name",name);
    connect(name,&QLineEdit::editingFinished,panel,[update,name]{update({{"name",name->text()}});});
    auto* selection=new QLabel;selection->setObjectName("surfaceStitch.selection");form->addRow("Faces",selection);
    auto* clear=new QPushButton("Clear");form->addRow(clear);connect(clear,&QPushButton::clicked,panel,[update]{update({{"sourceFaces",QJsonArray{}}});});
    const double scale=document_->authored().value("displayUnits")=="in"?25.4:1.;
    auto* distance=new QDoubleSpinBox;distance->setObjectName("surfaceStitch.tolerance");distance->setDecimals(7);distance->setRange(1e-7/scale,1./scale);distance->setSuffix(scale==1.?" mm":" in");distance->setKeyboardTracking(false);form->addRow("Tolerance",distance);
    connect(distance,&QDoubleSpinBox::valueChanged,panel,[update,distance,scale]{update({{"toleranceMm",distance->value()*scale}});});
    auto* status=new QLabel;status->setObjectName("surfaceStitch.status");status->setWordWrap(true);form->addRow(status);
    auto* hint=new QLabel("Select two coplanar rectangular sheet faces with one matching full straight seam. Stitch consumes both sheets into one retained open shell. General networks and solid promotion are not supported.");hint->setWordWrap(true);form->addRow(hint);
    auto* error=new QLabel;error->setObjectName("surfaceStitch.error");error->setWordWrap(true);error->setStyleSheet("color:#b3261e");form->addRow(error);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);form->addRow(buttons);
    connect(buttons,&QDialogButtonBox::accepted,panel,[this,sessionId]{execute("command.commit",{{"session",sessionId}});});
    connect(buttons,&QDialogButtonBox::rejected,panel,[this,sessionId]{execute("command.cancel",{{"session",sessionId}});});
    if(!surfaceStitchEscape_){surfaceStitchEscape_=new QShortcut(QKeySequence(Qt::Key_Escape),this);connect(surfaceStitchEscape_,&QShortcut::activated,this,[this]{if(session_.value("command")=="surface.stitch")execute("command.cancel",{{"session",session_.value("id")}});});}
    surfaceStitchEscape_->setEnabled(true);canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,surfaceStitchDock_);surfaceStitchDock_->show();refreshSurfaceStitchControls();
}
