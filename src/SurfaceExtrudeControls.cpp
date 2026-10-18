// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include "SketchGeometry.h"
#include "SurfaceExtrude.h"
#include <BRep_Builder.hxx>
#include <Standard_Failure.hxx>
#include <TopoDS_Compound.hxx>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDialog>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QShortcut>
#include <QSignalBlocker>
#include <QSet>
#include <QTimer>
#include <QElapsedTimer>
#include <QDateTime>
#include <QStatusBar>
#include <thread>
#include <cmath>
#include <algorithm>

namespace {
QJsonObject failure(const QString& code,const QString& message){return {{"ok",false},{"completion","failed"},{"error",QJsonObject{{"code",code},{"message",message}}}};}
}

QJsonObject AlloyWindow::surfaceExtrudeCandidate(const QJsonObject& inputs) const {
    auto args=inputs;
    if(!args.contains("featureId") && session_.value("command")=="surface.extrude"){
        args["newFeatureId"]=session_.value("newFeatureId");if(args["resultMode"]!="perProfile")args["newBodyId"]=session_.value("newBodyId");
    }
    args=authoringArguments(args);
    if(args.contains("featureId"))args.remove("ownerId");
    return CadDocument::surfaceExtrudeCandidate(document_->authored(),args);
}

QJsonObject AlloyWindow::beginSurfaceExtrude(const QJsonObject& overrides) {
    if(!session_.isEmpty())return failure("session_busy","Complete or cancel the active command first");
    QJsonObject inputs{{"resultMode","perProfile"},{"profiles",QJsonArray{}},{"sideOneMm",10.0},{"sideTwoMm",10.0},{"name","Extrude"}};
    if(overrides.contains("featureId")){
        bool found=false;
        for(const auto& entry:document_->authored()["features"].toArray()){
            const auto feature=entry.toObject();if(feature["id"]!=overrides["featureId"])continue;
            if(feature["type"]!="surfaceExtrude")return failure("stale_entity","Surface Extrude feature unavailable");
            inputs=feature["parameters"].toObject();inputs["featureId"]=feature["id"];inputs["name"]=feature["name"];found=true;break;
        }
        if(!found)return failure("stale_entity","Surface Extrude feature unavailable");
    }
    if(overrides.contains("entityIds") && !overrides.contains("featureId") && !overrides.contains("resultMode")){inputs.remove("resultMode");inputs.remove("profiles");}
    for(auto it=overrides.begin();it!=overrides.end();++it)inputs[it.key()]=it.value();
    const auto error=validateArguments(*findCommand("surface.extrudeDialog"),inputs);
    if(!error.isEmpty())return failure("invalid_arguments",error);
    session_={{"id",CadDocument::newId()},{"command","surface.extrude"},{"status","active"},{"inputs",inputs},
        {"newFeatureId",CadDocument::newId()},{"newBodyId",CadDocument::newId()},
        {"baseDocumentRevision",static_cast<qint64>(document_->revision())},{"previewed",false},
        {"geometryEvaluated",false},{"chaining",true},{"inputSchema",findCommand("surface.extrude")->parameters}};
    invalidateGeometryEvaluation();
    surfaceExtrudeBase_=document_->authored();
    showSurfaceExtrude();previewSurfaceExtrude();view_->setFocus(Qt::OtherFocusReason);return session_;
}

QJsonObject AlloyWindow::previewSurfaceExtrude() {
    const auto inputs=session_.value("inputs").toObject();
    const bool matchingJob=(geometryEvaluationQueued_ && geometryEvaluationQueued_->generation==geometryEvaluationGeneration_)
        || (geometryEvaluationRunning_ && geometryEvaluationRunning_->generation==geometryEvaluationGeneration_);
    if(surfaceExtrudeRequestedSession_==session_.value("id").toString() && surfaceExtrudeRequestedInputs_==inputs
        && session_.value("baseDocumentRevision").toInteger()==static_cast<qint64>(document_->revision())
        && (matchingJob || (session_.value("evaluationStatus")=="completed" && surfaceExtrudePrepared_.ok)))return session_;
    ++geometryEvaluationGeneration_;geometryEvaluationQueued_.reset();surfaceExtrudePrepared_={};
    surfaceExtrudeRequestedInputs_={};surfaceExtrudeRequestedSession_.clear();surfaceExtrudeRequestedCandidate_={};
    session_["previewed"]=false;session_["geometryEvaluated"]=false;
    session_.remove("geometry");session_.remove("documentDraft");session_.remove("bodyId");session_.remove("resultBodyIds");session_.remove("resultBodies");view_->clearPreview();view_->setEditDisplay({},{});
    QSet<QString> visible;
    for(const auto& sketch:document_->sketches())if(sketch.visible)visible.insert(sketch.featureId);
    view_->setCommandSketches(visible);
    if(surfaceExtrudeDock_){
        if(auto* list=surfaceExtrudeDock_->findChild<QListWidget*>("surfaceExtrude.entities")){
            const QSignalBlocker block(list);list->clear();
            if(inputs["resultMode"]=="perProfile"){
                int number=0;for(const auto& value:inputs["profiles"].toArray()){
                    const auto count=value.toObject()["entityIds"].toArray().size();
                    list->addItem(QString("Profile %1 · %2 %3").arg(++number).arg(count).arg(count==1 ? "curve" : "curves"));
                }
            }else for(const auto& value:inputs["entityIds"].toArray())list->addItem("Curve · "+value.toString().left(8));
        }
        for(const auto& key:{"sideOneMm","sideTwoMm"})if(auto* field=surfaceExtrudeDock_->findChild<QDoubleSpinBox*>(key)){
            const QSignalBlocker block(field);field->setValue(inputs[key].toDouble());
        }
        if(auto* buttons=surfaceExtrudeDock_->findChild<QDialogButtonBox*>())buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
    auto reject=[&](const QString& code,const QString& message,const QJsonObject& diagnostics=QJsonObject{}){
        auto result=failure(code,message);auto detail=result["error"].toObject();detail["diagnostics"]=diagnostics;
        result["error"]=detail;session_["previewError"]=detail;session_["evaluationStatus"]="failed";
        if(surfaceExtrudeDock_)if(auto* label=surfaceExtrudeDock_->findChild<QLabel*>("surfaceExtrude.error"))label->setText(message);
        return result;
    };
    if(session_.value("baseDocumentRevision").toInteger()!=static_cast<qint64>(document_->revision()))
        return reject("stale_session","The document changed; cancel and restart Surface Extrude");
    const bool selected=!inputs["sketchId"].toString().isEmpty() && !(inputs["resultMode"]=="perProfile" ? inputs["profiles"].toArray().isEmpty() : inputs["entityIds"].toArray().isEmpty());
    if(!selected && (!inputs.contains("featureId") || surfaceExtrudeSource_)) {
        session_["evaluationStatus"]="failed";
        return reject("selection_required","Select one or more nonbranching open profiles");
    }
    QJsonObject candidate;
    try{if(selected)candidate=surfaceExtrudeCandidate(inputs);}catch(const std::exception& error){return reject("invalid_arguments",QString::fromUtf8(error.what()));}
    auto task=std::make_shared<GeometryEvaluation>();
    task->document=document_;task->documentId=document_->authored()["documentId"].toString();
    task->sessionId=session_.value("id").toString();task->revision=document_->revision();
    task->generation=++geometryEvaluationGeneration_;task->base=surfaceExtrudeBase_;
    task->candidate=candidate;task->inputs=inputs;
    task->featureId=inputs.value("featureId").toString(session_.value("newFeatureId").toString());
    task->needsSource=inputs.contains("featureId") && !surfaceExtrudeSource_;
    surfaceExtrudeRequestedInputs_=inputs;surfaceExtrudeRequestedSession_=task->sessionId;surfaceExtrudeRequestedCandidate_=candidate;
    surfaceExtrudePrepared_={};geometryEvaluationQueued_=task;
    session_["evaluationHeartbeat"]=QJsonObject{{"ticks",0},{"lastMs",QDateTime::currentMSecsSinceEpoch()},{"maxGapMs",0}};
    session_["evaluationStatus"]="pending";session_["previewGeneration"]=static_cast<qint64>(task->generation);
    session_["cancellation"]="Discard result; running kernel evaluation continues";
    session_.remove("previewError");
    if(surfaceExtrudeDock_)if(auto* label=surfaceExtrudeDock_->findChild<QLabel*>("surfaceExtrude.error"))
        label->setText("Computing preview… You can navigate, change inputs or Cancel. Running kernel work cannot be interrupted.");
    startGeometryEvaluation();return session_;
}

QJsonObject AlloyWindow::beginDocumentOpen(const QString& path) {
    auto task=std::make_shared<GeometryEvaluation>();
    task->command="document.open";task->phase="open";task->document=document_;
    task->documentId=document_->authored().value("documentId").toString();task->revision=document_->revision();
    task->base=document_->authored();task->retainedSession=session_;task->tabId=activeTabId_;task->sessionId=CadDocument::newId();
    task->inputs={{"path",path}};task->generation=++geometryEvaluationGeneration_;
    documentOpenOperation_={{"id",task->sessionId},{"command",task->command},{"inputs",task->inputs},
        {"evaluationStatus","pending"},{"previewGeneration",static_cast<qint64>(task->generation)},
        {"cancellation","Discard result; running kernel evaluation continues"},
        {"evaluationHeartbeat",QJsonObject{{"ticks",0},{"lastMs",QDateTime::currentMSecsSinceEpoch()},{"maxGapMs",0}}}};
    geometryEvaluationTerminal_={};geometryEvaluationQueued_=task;
    documentOpenFrozenControls_.clear();
    for(auto* dock:findChildren<QDockWidget*>())if(dock->isEnabled()){
        documentOpenFrozenControls_.append(dock);dock->setEnabled(false);
    }
    for(auto* dialog:findChildren<QDialog*>())if(dialog->isEnabled()){
        documentOpenFrozenControls_.append(dialog);dialog->setEnabled(false);
    }
    documentOpenCancel_=new QPushButton("Cancel Open",statusBar());documentOpenCancel_->setObjectName("document.cancelOpen");
    statusBar()->addPermanentWidget(documentOpenCancel_);
    connect(documentOpenCancel_,&QPushButton::clicked,this,[this]{execute("command.cancel",{{"session",documentOpenOperation_.value("id")}});});
    statusBar()->showMessage("Opening design… You can navigate or Cancel Open. Running geometry work cannot be interrupted.");
    status_->hide(); // Keep normal status text from overlapping the Open message.
    startGeometryEvaluation();return documentOpenOperation_;
}

QJsonObject AlloyWindow::cancelDocumentOpen() {
    ++geometryEvaluationGeneration_;geometryEvaluationQueued_.reset();
    documentOpenOperation_["evaluationStatus"]="failed";
    documentOpenOperation_["previewError"]=QJsonObject{{"code","command_cancelled"},{"message","Open result discarded; original design preserved"}};
    documentOpenOperation_["resultDiscarded"]=true;
    if(documentOpenCancel_){statusBar()->removeWidget(documentOpenCancel_);documentOpenCancel_->deleteLater();documentOpenCancel_=nullptr;}
    statusBar()->showMessage("Finishing background geometry; new modeling commands will resume shortly.");
    return {{"ok",true},{"completion","completed"},{"result",QJsonObject{{"resultDiscarded",true},{"evaluationStillRunning",bool(geometryEvaluationRunning_)}}}};
}

void AlloyWindow::completeDocumentOpen(const std::shared_ptr<GeometryEvaluation>& task) {
    statusBar()->clearMessage();status_->show();
    for(const auto& control:documentOpenFrozenControls_)if(control)control->setEnabled(true);
    documentOpenFrozenControls_.clear();
    if(documentOpenCancel_){statusBar()->removeWidget(documentOpenCancel_);documentOpenCancel_->deleteLater();documentOpenCancel_=nullptr;}
    const bool current=document_==task->document && activeTabId_==task->tabId && document_->revision()==task->revision
        && document_->authored()==task->base && session_==task->retainedSession && documentOpenOperation_.value("id")==task->sessionId
        && documentOpenOperation_.value("evaluationStatus")=="pending" && geometryEvaluationGeneration_==task->generation;
    if(!current){
        if(documentOpenOperation_.value("id")==task->sessionId && documentOpenOperation_.value("evaluationStatus")=="pending"){
            documentOpenOperation_["evaluationStatus"]="failed";
            documentOpenOperation_["previewError"]=QJsonObject{{"code","stale_open"},{"message","The original design changed; Open result discarded"}};
        }
        return;
    }
    documentOpenOperation_["evaluationTimingMs"]=QJsonObject{{"full",task->fullMs}};
    auto error=task->error.isEmpty() ? task->full.error : task->error;
    if(error.isEmpty() && !task->full.ok)error="Native document evaluation unavailable";
    if(error.isEmpty()){
        // Only GUI-thread objects and presentation are changed after the worker's
        // complete private result has passed identity, request and revision guards.
        error=document_->adoptOpen(task->full,task->inputs.value("path").toString(),task->base,task->revision);
    }
    if(!error.isEmpty()){
        documentOpenOperation_["evaluationStatus"]="failed";
        documentOpenOperation_["previewError"]=QJsonObject{{"code","open_failed"},{"message",error},{"diagnostics",task->full.diagnostics}};
        status_->setText("Open failed; original design preserved: "+error);return;
    }
    if(!session_.isEmpty())finishSession(session_.value("id").toString(),false);
    view_->rebindDocument();refreshCanvasPresentation();displayedDocumentRevision_=std::numeric_limits<quint64>::max();
    selectedBody_.clear();selectedFace_={};selectedEdge_={};selectedPlane_.clear();view_->selectSupportFace({},{});
    view_->clearSolidEdgeHighlights();selectedSketch_.clear();selectedProfile_.clear();selectedProfileIds_={};
    view_->fit(presentationBodies(),nonzeroAssemblyPose() ? std::vector<EvaluatedSketch>{} : document_->sketches());
    const auto operation=documentOpenOperation_;documentOpenOperation_=document_->state();
    for(auto it=operation.begin();it!=operation.end();++it)documentOpenOperation_[it.key()]=it.value();
    documentOpenOperation_["evaluationStatus"]="completed";geometryEvaluationTerminal_={};
    refresh();const auto recoveryError=syncRecovery();
    documentOpenOperation_["recovery"]=QJsonObject{{"durable",recoveryError.isEmpty()},{"warning",recoveryError},{"scope","committed authored state only"}};

}

void AlloyWindow::invalidateGeometryEvaluation() {
    if(geometryEvaluationRunning_)statusBar()->showMessage("Finishing background geometry; new modeling commands will resume shortly.");
    ++geometryEvaluationGeneration_;geometryEvaluationQueued_.reset();surfaceExtrudeSource_.reset();
    surfaceExtrudePrepared_={};surfaceExtrudeRequestedInputs_={};surfaceExtrudeRequestedSession_.clear();surfaceExtrudeRequestedCandidate_={};
}

void AlloyWindow::startGeometryEvaluation() {
    if(geometryEvaluationRunning_ || !geometryEvaluationQueued_)return;
    geometryEvaluationRunning_=std::move(geometryEvaluationQueued_);
    if(!geometryEvaluationPoll_){geometryEvaluationPoll_=new QTimer(this);geometryEvaluationPoll_->setInterval(25);
        connect(geometryEvaluationPoll_,&QTimer::timeout,this,[this]{pollGeometryEvaluation();});}
    geometryEvaluationPoll_->start();
    const auto task=geometryEvaluationRunning_;
    // The detached worker owns only immutable JSON and its private fresh shapes.
    // No QObject, document, live topology or viewport cache is accessed here.
    std::thread([task]{
        try {
            QElapsedTimer timer;timer.start();
            if(task->command=="document.open"){task->full=CadDocument::prepareOpen(task->inputs.value("path").toString());task->fullMs=timer.elapsed();}
            else if(task->command=="surface.boundaryFill" && task->phase=="partition")evaluateBoundaryFillPartition(*task);
            else {
                if(task->needsSource){
                    auto prefix=task->base;QJsonArray features;
                    for(const auto& row:prefix["features"].toArray()){if(row.toObject()["id"]==task->featureId)break;features.append(row);}
                    prefix["features"]=features;prefix.remove("objectOwners");prefix.remove("bodyNames");
                    task->source=std::make_shared<const EvaluatedDocument>(GeometryKernel::evaluateDocument(prefix));
                }
                task->sourceMs=timer.restart();
                if(!task->candidate.isEmpty()){
                    task->full=CadDocument::evaluateCandidate(task->candidate);task->fullMs=timer.restart();
                    if(task->full.ok && task->command=="surface.extrude"){
                        const auto features=task->candidate["features"].toArray();
                        if(!features.isEmpty() && features.last().toObject()["id"]==task->featureId)task->stage=task->full;
                        else {
                            auto prefix=task->candidate;QJsonArray preceding;
                            for(const auto& row:features){preceding.append(row);if(row.toObject()["id"]==task->featureId)break;}
                            prefix["features"]=preceding;prefix.remove("objectOwners");prefix.remove("bodyNames");
                            task->stage=CadDocument::evaluateCandidate(prefix);
                        }
                        task->stageMs=timer.elapsed();
                    }
                }
            }
        }catch(const Standard_Failure& e){task->error=QString::fromUtf8(e.GetMessageString());}
         catch(const std::exception& e){task->error=QString::fromUtf8(e.what());}
         catch(...){task->error="Unexpected modeling evaluation failure";}
        task->ready.store(true,std::memory_order_release);
    }).detach();
}

void AlloyWindow::pollGeometryEvaluation() {
    if((session_.value("command")=="surface.extrude" || session_.value("command")=="surface.boundaryFill") && session_.value("evaluationStatus")=="pending"){
        auto heartbeat=session_.value("evaluationHeartbeat").toObject();const auto now=QDateTime::currentMSecsSinceEpoch();
        heartbeat["ticks"]=heartbeat["ticks"].toInteger()+1;
        heartbeat["maxGapMs"]=std::max(heartbeat["maxGapMs"].toInteger(),now-heartbeat["lastMs"].toInteger());
        heartbeat["lastMs"]=now;session_["evaluationHeartbeat"]=heartbeat;
    }
    if(documentOpenOperation_.value("evaluationStatus")=="pending"){
        auto heartbeat=documentOpenOperation_.value("evaluationHeartbeat").toObject();const auto now=QDateTime::currentMSecsSinceEpoch();
        heartbeat["ticks"]=heartbeat.value("ticks").toInteger()+1;
        heartbeat["maxGapMs"]=std::max(heartbeat.value("maxGapMs").toInteger(),now-heartbeat.value("lastMs").toInteger());
        heartbeat["lastMs"]=now;documentOpenOperation_["evaluationHeartbeat"]=heartbeat;
    }
    if(sketchEvaluationOperation_.value("evaluationStatus")=="pending"){
        auto heartbeat=sketchEvaluationOperation_.value("evaluationHeartbeat").toObject();const auto now=QDateTime::currentMSecsSinceEpoch();
        heartbeat["ticks"]=heartbeat.value("ticks").toInteger()+1;
        heartbeat["maxGapMs"]=std::max(heartbeat.value("maxGapMs").toInteger(),now-heartbeat.value("lastMs").toInteger());
        heartbeat["lastMs"]=now;sketchEvaluationOperation_["evaluationHeartbeat"]=heartbeat;
    }
    if(!geometryEvaluationRunning_ || !geometryEvaluationRunning_->ready.load(std::memory_order_acquire))return;
    const auto task=std::move(geometryEvaluationRunning_);
    const bool current=document_==task->document && document_->revision()==task->revision
        && document_->authored()["documentId"]==task->documentId && session_.value("id")==task->sessionId
        && session_.value("command")==task->command && geometryEvaluationGeneration_==task->generation;
    if(task->command=="sketch.edit" || task->command=="sketch.finish" || task->command=="feature.edit")completeSketchEvaluation(task);
    else if(task->command=="document.open")completeDocumentOpen(task);
    else if(task->command=="surface.boundaryFill"){
        if(task->phase=="partition")completeBoundaryFillPartition(task);else completeBoundaryFillCommit(task);
    }
    else if(current){
        if(task->source){surfaceExtrudeSource_=task->source;view_->setBodyStage(task->source->bodies,"surface-extrude-source:"+task->sessionId);}
        session_["evaluationTimingMs"]=QJsonObject{{"source",task->sourceMs},{"full",task->fullMs},{"historicalStage",task->stageMs}};
        auto reject=[&](const QString& code,const QString& message,const QJsonObject& diagnostics=QJsonObject{}){
            session_["evaluationStatus"]="failed";session_["previewError"]=QJsonObject{{"code",code},{"message",message},{"diagnostics",diagnostics}};
            if(surfaceExtrudeDock_)if(auto* label=surfaceExtrudeDock_->findChild<QLabel*>("surfaceExtrude.error"))label->setText(message);
        };
        if(!task->error.isEmpty())reject("evaluation_failed",task->error);
        else if(task->candidate.isEmpty())reject("selection_required","Select one or more nonbranching open profiles");
        else if(!task->full.ok)reject("evaluation_failed",task->full.error,task->full.diagnostics);
        else if(!task->stage.ok)reject("evaluation_failed",task->stage.error,task->stage.diagnostics);
        else {
            const auto inputs=task->inputs;const auto featureId=task->featureId;const auto stage=task->stage.evaluation;
            QJsonArray resultIds,resultBodies;TopoDS_Compound display;BRep_Builder builder;builder.MakeCompound(display);
            for(const auto& body:stage->bodies)if(body.featureId==featureId){
                builder.Add(display,body.shape);resultIds.append(body.bodyId);resultBodies.append(QJsonObject{{"bodyId",body.bodyId},{"geometry",body.summary}});
                if(inputs["resultMode"]!="perProfile"){session_["geometry"]=body.summary;session_["bodyId"]=body.bodyId;}
            }
            if(resultIds.isEmpty())reject("surface_result_unavailable","Surface Extrude produced no current result bodies");
            else {
                view_->showPreview(display,{},true);
                if(inputs.contains("featureId")){
                    QJsonArray hidden;for(const auto& body:document_->bodies())if(body.featureId==featureId)hidden.append(body.bodyId);
                    view_->setEditDisplay({},hidden);
                }
                session_["featureId"]=featureId;session_["resultBodyIds"]=resultIds;session_["resultBodies"]=resultBodies;
                session_["documentDraft"]=task->full.authored;session_["previewed"]=true;session_["geometryEvaluated"]=true;
                session_["evaluationStatus"]="completed";session_.remove("previewError");surfaceExtrudePrepared_=task->full;
                if(surfaceExtrudeDock_){
                    if(auto* label=surfaceExtrudeDock_->findChild<QLabel*>("surfaceExtrude.error"))label->clear();
                    surfaceExtrudeDock_->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->setEnabled(true);
                }
            }
        }
        refresh();
    }
    if(geometryEvaluationQueued_)startGeometryEvaluation();
    else {
        geometryEvaluationPoll_->stop();
        if(statusBar()->currentMessage()=="Finishing background geometry; new modeling commands will resume shortly.")
            statusBar()->clearMessage();
    }
}

QJsonObject AlloyWindow::selectSurfaceExtrudeEntity(const QString& sketchId,const QString& entityId){
    if(session_.value("command")!="surface.extrude")return failure("invalid_session","Start Surface Extrude first");
    const auto inputs=session_.value("inputs").toObject();
    const auto edit=inputs["featureId"].toString();
    if(!edit.isEmpty() && !surfaceExtrudeSource_)return failure("evaluation_pending","Source history is still being evaluated; retry when ready");
    const auto& stage=edit.isEmpty() ? document_->evaluation() : *surfaceExtrudeSource_;
    bool found=false;
    for(const auto& sketch:stage.sketches)if(sketch.featureId==sketchId && sketch.visible
        && sketch.entityEdges.contains(entityId))
        for(const auto& row:sketch.summary["entities"].toArray())if(row.toObject()["id"]==entityId
            && (row.toObject()["type"]=="line" || row.toObject()["type"]=="fitSpline")
            && !row.toObject()["construction"].toBool())found=true;
    if(!found)return failure("surface_source_unavailable","Select a visible ordinary open line or fit-spline curve");
    if(!inputs["sketchId"].toString().isEmpty() && inputs["sketchId"]!=sketchId)
        return failure("surface_sketch_mismatch","Selected curves must belong to one source sketch");
    if(inputs["resultMode"]=="perProfile"){
        for(const auto& profile:inputs["profiles"].toArray())if(profile.toObject()["entityIds"].toArray().contains(entityId))
            return selectSurfaceExtrudeProfile(sketchId,profile.toObject()["entityIds"].toArray());
        QJsonArray members{entityId};
        if(session_.value("chaining").toBool())for(const auto& sketch:stage.sketches)if(sketch.featureId==sketchId){
            try{members=SurfaceExtrude::openChain(sketch,entityId);}catch(const std::exception& error){return failure("surface_profile_topology",QString::fromUtf8(error.what()));}}
        return selectSurfaceExtrudeProfile(sketchId,members);
    }
    auto next=inputs;auto entities=next["entityIds"].toArray();
    if(entities.contains(entityId)){for(int i=0;i<entities.size();++i)if(entities[i]==entityId){entities.removeAt(i);break;}}
    else entities.append(entityId);
    next["sketchId"]=sketchId;next["entityIds"]=entities;session_["inputs"]=next;
    previewSurfaceExtrude();return {{"sketchId",sketchId},{"entityId",entityId},{"entityIds",entities},
        {"previewed",session_.value("previewed")},{"previewError",session_.value("previewError")},
        {"evaluationStatus",session_.value("evaluationStatus")},{"id",session_.value("id")},{"previewGeneration",session_.value("previewGeneration")}};
}

QJsonObject AlloyWindow::selectSurfaceExtrudeProfile(const QString& sketchId,const QJsonArray& entityIds){
    if(session_.value("command")!="surface.extrude" || session_.value("inputs").toObject()["resultMode"]!="perProfile")return failure("invalid_session","Start per-profile Surface Extrude first");
    auto inputs=session_.value("inputs").toObject();if(!inputs["sketchId"].toString().isEmpty() && inputs["sketchId"]!=sketchId)return failure("surface_sketch_mismatch","Profiles must belong to one source sketch");
    const auto edit=inputs.value("featureId").toString();if(!edit.isEmpty() && !surfaceExtrudeSource_)return failure("evaluation_pending","Source history is still being evaluated; retry when ready");
    const auto& stage=edit.isEmpty() ? document_->evaluation() : *surfaceExtrudeSource_;
    const EvaluatedSketch* source=nullptr;for(const auto& sketch:stage.sketches)if(sketch.featureId==sketchId && sketch.visible)source=&sketch;
    if(!source)return failure("surface_source_unavailable","Select a visible earlier solved sketch");
    // Validate retained topology through the shared JSON chain routine, without
    // constructing prisms or touching any live OCCT shape on this input path.
    if(entityIds.isEmpty())return failure("surface_profile_topology","Select an open profile");
    QSet<QString> unique;
    for(const auto& id:entityIds){if(!id.isString() || unique.contains(id.toString()) || !source->entityEdges.contains(id.toString()))
        return failure("surface_profile_topology","Selected profile contains unavailable or repeated curves");unique.insert(id.toString());}
    auto selected=*source;QJsonArray selectedEntities;
    for(const auto& row:source->summary["entities"].toArray())if(unique.contains(row.toObject()["id"].toString()))selectedEntities.append(row);
    selected.summary["entities"]=selectedEntities;
    try{if(SurfaceExtrude::openChain(selected,entityIds[0].toString()).size()!=entityIds.size())
        return failure("surface_profile_topology","Profile members are disconnected");}
    catch(const std::exception& error){return failure("surface_profile_topology",QString::fromUtf8(error.what()));}
    auto profiles=inputs["profiles"].toArray();const auto wanted=SurfaceExtrude::profileBodyId(session_.value("newFeatureId").toString(),sketchId,entityIds);int existing=-1;
    QSet<QString> members;for(const auto& v:entityIds)members.insert(v.toString());
    for(int i=0;i<profiles.size();++i){const auto ids=profiles[i].toObject()["entityIds"].toArray();
        if(SurfaceExtrude::profileBodyId(session_.value("newFeatureId").toString(),sketchId,ids)==wanted){existing=i;continue;}
        for(const auto& id:ids)if(members.contains(id.toString()))return failure("surface_profile_overlap","Selected chain overlaps an already authored profile; remove it explicitly first");}
    if(existing>=0)profiles.removeAt(existing);else profiles.append(QJsonObject{{"entityIds",entityIds}});
    inputs["sketchId"]=sketchId;inputs["profiles"]=profiles;session_["inputs"]=inputs;previewSurfaceExtrude();
    return {{"sketchId",sketchId},{"profiles",profiles},{"previewed",session_.value("previewed")},{"previewError",session_.value("previewError")},
        {"evaluationStatus",session_.value("evaluationStatus")},{"id",session_.value("id")},{"previewGeneration",session_.value("previewGeneration")}};

}

QJsonObject AlloyWindow::pickSurfaceExtrude(double x,double y){
    const auto current=session_.value("inputs").toObject();const auto edit=current["featureId"].toString();
    if(!edit.isEmpty() && !surfaceExtrudeSource_)return failure("evaluation_pending","Source history is still being evaluated; retry when ready");
    const auto& stage=edit.isEmpty() ? document_->evaluation() : *surfaceExtrudeSource_;
    QString sketchId,entityId;double nearest=7;bool ambiguous=false;
    for(const auto& sketch:stage.sketches)if(sketch.visible && (current["sketchId"].toString().isEmpty() || current["sketchId"]==sketch.featureId)){
        auto source=SketchGeometry::findFeature(document_->authored(),sketch.featureId);
        auto parameters=source["parameters"].toObject();auto points=parameters["points"].toArray();
        const auto frame=sketch.resolvedFrame;
        const auto origin=frame["origin"].toArray(),u=frame["uAxis"].toArray(),v=frame["vAxis"].toArray();
        QMap<QString,QJsonArray> world;
        for(const auto& row:sketch.summary["worldPoints"].toArray())world[row.toObject()["id"].toString()]=row.toObject()["positionMm"].toArray();
        if(origin.size()!=3 || u.size()!=3 || v.size()!=3)continue;
        for(int i=0;i<points.size();++i){auto point=points[i].toObject();const auto xyz=world.value(point["id"].toString());if(xyz.size()!=3)continue;
            double a=0,b=0;for(int j=0;j<3;++j){const auto delta=xyz[j].toDouble()-origin[j].toDouble();a+=delta*u[j].toDouble();b+=delta*v[j].toDouble();}
            point["uMm"]=a;point["vMm"]=b;points[i]=point;}
        parameters["points"]=points;source["parameters"]=parameters;
        const auto hit=view_->pickSketch(source,x,y,"entity",frame);
        if(hit["kind"]!="entity" || hit["projected"].toBool())continue;
        bool supported=false;
        for(const auto& row:sketch.summary["entities"].toArray())if(row.toObject()["id"]==hit["entityId"]
            && (row.toObject()["type"]=="line" || row.toObject()["type"]=="fitSpline")
            && !row.toObject()["construction"].toBool())supported=true;
        if(!supported)continue;
        const auto distance=hit["distancePixels"].toDouble();
        if(distance<nearest-1e-6){nearest=distance;sketchId=sketch.featureId;entityId=hit["entityId"].toString();ambiguous=false;}
        else if(std::abs(distance-nearest)<1e-6)ambiguous=true;
    }
    if(ambiguous)return failure("ambiguous_surface_source","More than one open curve is under this point");
    if(sketchId.isEmpty())return failure("surface_source_required","Click a visible open line or fit-spline curve");
    return selectSurfaceExtrudeEntity(sketchId,entityId);
}

void AlloyWindow::showSurfaceExtrude(){
    if(surfaceExtrudeDock_){canvasWindow_->removeDockWidget(surfaceExtrudeDock_);delete surfaceExtrudeDock_;}
    surfaceExtrudeDock_=new QDockWidget("Extrude",canvasWindow_);surfaceExtrudeDock_->setObjectName("surfaceExtrudeDock");
    auto* panel=new QWidget(surfaceExtrudeDock_);surfaceExtrudeDock_->setWidget(panel);
    auto* form=new QFormLayout(panel);form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    const bool perProfile=session_.value("inputs").toObject()["resultMode"]=="perProfile";
    auto* hint=new QLabel(perProfile
        ? "Select open profiles to create separate surface bodies. Chaining selects connected curves. Two Sides measures independently from the sketch plane."
        : "Select open sketch curves to create one compound surface body. Two Sides measures independently from the sketch plane.",panel);
    hint->setWordWrap(true);form->addRow(hint);
    auto* name=new QLineEdit(session_.value("inputs").toObject()["name"].toString(),panel);name->setObjectName("surfaceExtrude.name");form->addRow("Name",name);
    connect(name,&QLineEdit::editingFinished,panel,[this,name]{auto inputs=session_.value("inputs").toObject();inputs["name"]=name->text();session_["inputs"]=inputs;previewSurfaceExtrude();});
    auto* list=new QListWidget(panel);list->setObjectName("surfaceExtrude.entities");list->setMinimumHeight(100);form->addRow(session_.value("inputs").toObject()["resultMode"]=="perProfile" ? "Profiles" : "Curves",list);
    auto* remove=new QPushButton("Remove Selected",panel);remove->setObjectName("surfaceExtrude.remove");form->addRow(remove);
    connect(remove,&QPushButton::clicked,panel,[this,list]{const auto index=list->currentRow();auto inputs=session_.value("inputs").toObject();const auto key=inputs["resultMode"]=="perProfile" ? "profiles" : "entityIds";auto refs=inputs[key].toArray();
        if(index<0 || index>=refs.size())return;refs.removeAt(index);inputs[key]=refs;session_["inputs"]=inputs;previewSurfaceExtrude();});
    if(session_.value("inputs").toObject()["resultMode"]=="perProfile"){
        auto* chaining=new QCheckBox("Chaining",panel);chaining->setObjectName("surfaceExtrude.chaining");chaining->setChecked(session_.value("chaining").toBool());form->addRow(chaining);
        connect(chaining,&QCheckBox::toggled,panel,[this](bool checked){session_["chaining"]=checked;});
    }
    form->addRow("Direction",new QLabel("Two Sides",panel));form->addRow("Operation",new QLabel("New Body",panel));
    for(const auto& key:{"sideOneMm","sideTwoMm"}){
        auto* field=new QDoubleSpinBox(panel);field->setObjectName(key);field->setRange(.001,100000);field->setDecimals(3);field->setSuffix(" mm");
        field->setValue(session_.value("inputs").toObject()[key].toDouble());form->addRow(QString::fromLatin1(key)=="sideOneMm"?"Side 1":"Side 2",field);
        connect(field,qOverload<double>(&QDoubleSpinBox::valueChanged),panel,[this,key](double value){auto inputs=session_.value("inputs").toObject();inputs[key]=value;session_["inputs"]=inputs;previewSurfaceExtrude();});
    }
    auto* error=new QLabel(panel);error->setObjectName("surfaceExtrude.error");error->setWordWrap(true);form->addRow(error);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,panel);form->addRow(buttons);
    connect(buttons,&QDialogButtonBox::accepted,panel,[this,error]{const auto response=execute("command.commit",{{"session",session_.value("id")}});
        if(!response["ok"].toBool())error->setText(response["error"].toObject()["message"].toString());});
    auto cancel=[this]{if(session_.value("command")=="surface.extrude")execute("command.cancel",{{"session",session_.value("id")}});};
    connect(buttons,&QDialogButtonBox::rejected,panel,cancel);
    auto* escape=new QShortcut(QKeySequence(Qt::Key_Escape),panel);escape->setContext(Qt::WidgetWithChildrenShortcut);
    connect(escape,&QShortcut::activated,panel,cancel);
    if(!surfaceExtrudeEscape_){surfaceExtrudeEscape_=new QShortcut(QKeySequence(Qt::Key_Escape),view_);
        surfaceExtrudeEscape_->setContext(Qt::WidgetShortcut);connect(surfaceExtrudeEscape_,&QShortcut::activated,this,cancel);}
    surfaceExtrudeEscape_->setEnabled(true);
    canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,surfaceExtrudeDock_);surfaceExtrudeDock_->show();
}
