// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QDateTime>
#include <QDialog>
#include <QDockWidget>
#include <QLabel>
#include <QPushButton>
#include <QStatusBar>
#include <QSet>

QJsonObject AlloyWindow::preflightSketchDelete(const QJsonObject& candidate) const {
    const auto sketchId=session_.value("inputs").toObject().value("sketchId").toString();
    if(sketchId.isEmpty())return {}; // New sketches have no committed consumers.
    QSet<QString> remaining,removed;
    for(const auto& row:candidate.value("parameters").toObject().value("entities").toArray())
        remaining.insert(row.toObject().value("id").toString());
    for(const auto& row:session_.value("draftSketch").toObject().value("parameters").toObject().value("entities").toArray()) {
        const auto id=row.toObject().value("id").toString();if(!remaining.contains(id))removed.insert(id);
    }
    // Resolve the solved removal, including every owner-generated member, to
    // stable committed profile membership. No spatial matching or re-targeting.
    QJsonArray consumers;bool afterSketch=false;
    for(const auto& row:document_->authored().value("features").toArray()) {
        const auto feature=row.toObject();
        if(feature.value("id")==sketchId){afterSketch=true;continue;}
        const auto p=feature.value("parameters").toObject();
        if(!afterSketch || feature.value("type")!="extrude" || p.value("sketchId")!=sketchId)continue;
        auto ids=p.value("profileIds").toArray();if(ids.isEmpty() && p.contains("profileId"))ids.append(p.value("profileId"));
        for(const auto& sketch:document_->evaluation().sketches)if(sketch.featureId==sketchId)
            for(const auto& profile:sketch.profiles)if(ids.contains(profile.profileId)) {
                QJsonArray affected;
                for(const auto& id:profile.summary.value("sourceEntityIds").toArray())if(removed.contains(id.toString()))affected.append(id);
                if(!affected.isEmpty())consumers.append(QJsonObject{{"featureId",feature.value("id")},{"featureName",feature.value("name")},
                    {"profileId",profile.profileId},{"sketchId",sketchId},{"removedEntityIds",affected}});
            }
    }
    if(consumers.isEmpty())return {};
    // Evaluate only the affected sketch, never the downstream solid/history.
    const auto evaluated=evaluateSketchDraft(candidate);QSet<QString> available;
    for(const auto& profile:evaluated.profiles)available.insert(profile.profileId);
    QJsonArray blocked;for(const auto& row:consumers)if(!available.contains(row.toObject().value("profileId").toString()))blocked.append(row);
    if(blocked.isEmpty())return {};
    const auto owner=blocked.first().toObject();
    const auto name=owner.value("featureName").toString().isEmpty() ? QStringLiteral("Extrude") : owner.value("featureName").toString();
    const auto message=QString("This curve is used by Extrude ‘%1’. Keep the curve, or cancel this sketch edit and edit that Extrude to choose a current profile.").arg(name);
    QJsonObject error{{"code","downstream_profile_in_use"},{"message",message}};
    auto diagnostics=owner;diagnostics["consumers"]=blocked;diagnostics["policy"]="alloy_pre_delete_refusal";
    diagnostics["recovery"]="Keep the curve, or cancel sketch editing and edit the named Extrude to choose a current profile";
    error["diagnostics"]=diagnostics;return {{"ok",false},{"error",error}};
}

EvaluatedDocument AlloyWindow::sketchHistoryStage(const QString& featureId) const {
    if(sketchEvaluationSource_ && sketchEvaluationDocument_==document_
        && sketchEvaluationRevision_==document_->revision() && sketchEvaluationFeature_==featureId)
        return *sketchEvaluationSource_;
    return document_->evaluationBefore(featureId);
}

QJsonObject AlloyWindow::queueSketchEvaluation(const QString& command,const QString& sketchId,const QJsonObject& candidate) {
    // A new modeling request ends prior completed Open/commit status lifetimes.
    // The serial-worker gate prevents invalidating a still-pending caller here.
    documentOpenOperation_={};geometryEvaluationTerminal_={};meshExportOperation_={};
    auto task=std::make_shared<GeometryEvaluation>();task->command=command;task->phase=command=="sketch.edit" ? "open" : command=="feature.edit" ? "edit" : "finish";
    task->document=document_;task->documentId=document_->authored().value("documentId").toString();
    task->tabId=activeTabId_;task->revision=document_->revision();task->base=document_->authored();task->retainedSession=session_;
    task->sessionId=CadDocument::newId();task->generation=++geometryEvaluationGeneration_;
    task->featureId=sketchId;task->candidate=candidate;task->needsSource=command=="sketch.edit";
    sketchEvaluationOperation_={{"id",task->sessionId},{"command",command},{"evaluationPhase",task->phase},
        {"evaluationStatus","pending"},{"previewGeneration",static_cast<qint64>(task->generation)},
        {"cancellation","Discard result; running kernel evaluation continues"},
        {"evaluationHeartbeat",QJsonObject{{"ticks",0},{"lastMs",QDateTime::currentMSecsSinceEpoch()},{"maxGapMs",0}}}};
    if(command=="sketch.finish" && sketchError_)sketchError_->clear();
    sketchEvaluationFocus_=QApplication::focusWidget();sketchEvaluationFrozenControls_.clear();
    for(auto* dock:findChildren<QDockWidget*>())if(dock->isEnabled()){
        sketchEvaluationFrozenControls_.append(dock);dock->setEnabled(false);
    }
    for(auto* dialog:findChildren<QDialog*>())if(dialog->isEnabled()){
        sketchEvaluationFrozenControls_.append(dialog);dialog->setEnabled(false);
    }
    sketchEvaluationCancel_=new QPushButton("Cancel Evaluation",statusBar());sketchEvaluationCancel_->setObjectName("sketch.cancelEvaluation");
    sketchEvaluationCancel_->setToolTip("Discard the background result; the original sketch draft remains. Running kernel work continues.");
    statusBar()->addPermanentWidget(sketchEvaluationCancel_);
    connect(sketchEvaluationCancel_,&QPushButton::clicked,this,[this]{execute("command.cancel",{{"session",sketchEvaluationOperation_.value("id")}});});
    statusBar()->showMessage(command=="sketch.edit" ? "Opening sketch… You can navigate or Cancel Evaluation." : command=="feature.edit" ? "Updating sketch and downstream geometry… You can navigate or Cancel Evaluation." : "Finishing sketch… You can navigate or Cancel Evaluation.");
    status_->hide();geometryEvaluationQueued_=task;startGeometryEvaluation();return sketchEvaluationOperation_;
}

QJsonObject AlloyWindow::cancelSketchEvaluation() {
    ++geometryEvaluationGeneration_;geometryEvaluationQueued_.reset();
    sketchEvaluationOperation_["evaluationStatus"]="failed";sketchEvaluationOperation_["resultDiscarded"]=true;
    sketchEvaluationOperation_["previewError"]=QJsonObject{{"code","command_cancelled"},{"message","Background result discarded; the original sketch draft is preserved"}};
    if(sketchEvaluationCancel_){statusBar()->removeWidget(sketchEvaluationCancel_);sketchEvaluationCancel_->deleteLater();sketchEvaluationCancel_=nullptr;}
    statusBar()->showMessage("Finishing background geometry; new modeling commands will resume shortly.");
    return {{"ok",true},{"completion","completed"},{"result",QJsonObject{{"resultDiscarded",true},{"evaluationStillRunning",bool(geometryEvaluationRunning_)}}}};
}

void AlloyWindow::completeSketchEvaluation(const std::shared_ptr<GeometryEvaluation>& task) {
    statusBar()->clearMessage();status_->show();
    for(const auto& control:sketchEvaluationFrozenControls_)if(control)control->setEnabled(true);
    sketchEvaluationFrozenControls_.clear();
    if(sketchEvaluationCancel_){statusBar()->removeWidget(sketchEvaluationCancel_);sketchEvaluationCancel_->deleteLater();sketchEvaluationCancel_=nullptr;}
    const bool current=document_==task->document && activeTabId_==task->tabId && document_->revision()==task->revision
        && document_->authored()==task->base && session_==task->retainedSession
        && sketchEvaluationOperation_.value("id")==task->sessionId && sketchEvaluationOperation_.value("evaluationStatus")=="pending"
        && geometryEvaluationGeneration_==task->generation;
    auto reject=[this](const QString& code,const QString& message,const QJsonObject& diagnostics=QJsonObject{}){
        sketchEvaluationOperation_["evaluationStatus"]="failed";
        sketchEvaluationOperation_["previewError"]=QJsonObject{{"code",code},{"message",message},{"diagnostics",diagnostics}};
        status_->setText(message);
    };
    if(!current){
        if(sketchEvaluationOperation_.value("id")==task->sessionId && sketchEvaluationOperation_.value("evaluationStatus")=="pending")
            reject("stale_session","The sketch or design changed; background result discarded");
        if(sketchEvaluationFocus_)sketchEvaluationFocus_->setFocus(Qt::OtherFocusReason);
        return;
    }
    sketchEvaluationOperation_["evaluationTimingMs"]=QJsonObject{{"source",task->sourceMs},{"full",task->fullMs}};
    if(!task->error.isEmpty())reject("evaluation_failed",task->error);
    else if(task->command=="sketch.edit"){
        sketchEvaluationSource_=task->source;sketchEvaluationDocument_=document_;sketchEvaluationRevision_=task->revision;sketchEvaluationFeature_=task->featureId;
        const auto feature=SketchGeometry::findFeature(task->base,task->featureId);
        // Keep the opaque request out of the editing session until its private
        // historical source is complete. Reuse its ID for the actual session.
        session_={{"id",task->sessionId},{"command","sketch.plane"}};
        const auto opened=beginSketch(feature.value("parameters").toObject().value("plane").toString(),feature.value("name").toString(),task->featureId);
        if(opened.value("ok").isBool() && !opened.value("ok").toBool()){
            session_=task->retainedSession;reject(opened.value("error").toObject().value("code").toString(),opened.value("error").toObject().value("message").toString());
        }else{
            const auto operation=sketchEvaluationOperation_;sketchEvaluationOperation_=session_;
            for(auto it=operation.begin();it!=operation.end();++it)sketchEvaluationOperation_[it.key()]=it.value();
            sketchEvaluationOperation_["evaluationStatus"]="completed";
        }
    }else if(!task->full.ok)reject("evaluation_failed",task->full.error,task->full.diagnostics);
    else {
        const auto error=document_->commitSketchEvaluation(task->full,task->candidate,task->base,task->revision,task->command=="feature.edit" ? "Edit feature" : "Edit sketch");
        if(!error.isEmpty())reject("evaluation_failed",error);
        else {
            if(task->command=="feature.edit"){
                const auto operation=sketchEvaluationOperation_;sketchEvaluationOperation_=document_->state();
                for(auto it=operation.begin();it!=operation.end();++it)sketchEvaluationOperation_[it.key()]=it.value();
                sketchEvaluationOperation_["evaluationStatus"]="completed";
            }else {
            const auto priorSession=session_.value("id");finishSession(priorSession.toString(),false);
            sketchEvaluationOperation_["sessionId"]=priorSession;sketchEvaluationOperation_["status"]="committed";
            sketchEvaluationOperation_["document"]=document_->state();sketchEvaluationOperation_["evaluationStatus"]="completed";
            }
        }
    }
    refresh();const auto recoveryError=syncRecovery();
    sketchEvaluationOperation_["recovery"]=QJsonObject{{"durable",recoveryError.isEmpty()},{"warning",recoveryError},{"scope","committed authored state only"}};
    if(sketchEvaluationOperation_.value("evaluationStatus")=="failed"){
        const auto message=sketchEvaluationOperation_.value("previewError").toObject().value("message").toString();
        status_->setText(message);
        if(session_.value("command")=="sketch.edit" && sketchError_)sketchError_->setText(message);
        if(sketchEvaluationFocus_)sketchEvaluationFocus_->setFocus(Qt::OtherFocusReason);
    }
}
