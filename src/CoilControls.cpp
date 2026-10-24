// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <BRepMesh_IncrementalMesh.hxx>
#include <Prs3d_Drawer.hxx>
#include <StdPrs_ToolTriangulatedShape.hxx>
#include <QDateTime>
#include <QElapsedTimer>
#include <QLabel>
#include <QStatusBar>
#include <cmath>

namespace {
QJsonObject coilFailure(const QString& code,const QString& message) {
    return {{"ok",false},{"completion","failed"},{"error",QJsonObject{{"code",code},{"message",message}}}};
}
}
QJsonObject AlloyWindow::previewCoil() {
    if(session_.contains("invalidNamedInput")){
        session_["coilCommitRequested"]=false;return coilFailure("unknown_parameter","Resolve the unavailable user length name before previewing Coil");
    }
    const auto inputs=session_.value("inputs").toObject();
    if(session_.value("requestedInputs")==inputs &&
        (session_.value("evaluationStatus")=="pending" || (session_.value("evaluationStatus")=="completed" && coilPrepared_)))return session_;
    ++geometryEvaluationGeneration_;geometryEvaluationQueued_.reset();coilPrepared_.reset();
    session_["previewed"]=false;session_["geometryEvaluated"]=false;session_.remove("geometry");session_.remove("documentDraft");
    view_->clearPreview();
    auto reject=[&](const QString& message){session_["evaluationStatus"]="failed";session_["coilCommitRequested"]=false;
        session_["previewError"]=QJsonObject{{"code","evaluation_failed"},{"message",message}};
        if(primitiveError_)primitiveError_->setText(message);return coilFailure("evaluation_failed",message);};
    if(session_.value("baseDocumentRevision").toInteger()!=static_cast<qint64>(document_->revision()))return reject("The document changed; cancel and restart Coil");
    auto task=std::make_shared<GeometryEvaluation>();task->command="solid.coil";
    task->document=document_;task->documentId=document_->authored().value("documentId").toString();task->revision=document_->revision();
    task->base=document_->authored();task->inputs=inputs;task->sessionId=session_.value("id").toString();task->generation=geometryEvaluationGeneration_;
    try{task->candidate=primitiveCandidate("solid.coil",inputs);}catch(const std::exception& e){return reject(QString::fromUtf8(e.what()));}
    const auto features=task->candidate.value("features").toArray();
    task->featureId=inputs.value("featureId").toString();
    if(task->featureId.isEmpty())task->featureId=features.last().toObject().value("id").toString();
    for(const auto& row:features)if(row.toObject().value("id")==task->featureId)task->feature=row.toObject();
    session_["requestedInputs"]=inputs;session_["evaluationStatus"]="pending";session_["previewGeneration"]=static_cast<qint64>(task->generation);
    session_["evaluationHeartbeat"]=QJsonObject{{"ticks",0},{"lastMs",QDateTime::currentMSecsSinceEpoch()},{"maxGapMs",0}};
    session_["cancellation"]="Discard result; running kernel evaluation continues";session_.remove("previewError");
    if(primitiveError_)primitiveError_->setText("Computing Coil preview… You can navigate or Cancel.");
    statusBar()->showMessage("Computing Coil preview… You can navigate or Cancel.");
    geometryEvaluationQueued_=task;startGeometryEvaluation();return session_;
}

void AlloyWindow::evaluateCoil(GeometryEvaluation& task) {
    QElapsedTimer phase;phase.start();task.full=CadDocument::evaluateCandidate(task.candidate);task.fullMs=phase.restart();
    task.stage=task.full;
    if(task.full.ok && task.candidate.value("features").toArray().last().toObject().value("id")!=task.featureId){
        auto prefix=task.candidate;QJsonArray through;
        for(const auto& row:prefix.value("features").toArray()){through.append(row);if(row.toObject().value("id")==task.featureId)break;}
        prefix["features"]=through;prefix.remove("objectOwners");prefix.remove("bodyNames");task.stage=CadDocument::evaluateCandidate(prefix);
    }
    task.stageMs=phase.restart();
    if(!task.full.ok || !task.stage.ok)return;
    const auto bodyId=task.feature.value("bodyId").toString();
    for(const auto& evaluation:{task.full.evaluation,task.stage.evaluation})for(const auto& body:evaluation->bodies)if(body.bodyId==bodyId){
        bool prepared=false;for(const auto& shape:task.preparedDisplayMeshes)prepared|=shape.IsSame(body.shape);if(prepared)continue;
        Handle(Prs3d_Drawer) drawer=new Prs3d_Drawer;drawer->SetDeviationCoefficient(.0005);drawer->SetDeviationAngle(.10);
        const double deflection=StdPrs_ToolTriangulatedShape::GetDeflection(body.shape,drawer);
        if(!std::isfinite(deflection) || deflection<=0)throw std::runtime_error("Coil display bounds are not finite");
        BRepMesh_IncrementalMesh mesh(body.shape,deflection,false,.10,false);
        if(!mesh.IsDone() || !StdPrs_ToolTriangulatedShape::IsTessellated(body.shape,drawer))throw std::runtime_error("Coil display tessellation is incomplete");
        task.preparedDisplayMeshes.push_back(body.shape);
    }
    task.sourceMs=phase.elapsed(); // Display-mesh phase; exposed with a clear Coil label below.
}

void AlloyWindow::completeCoilEvaluation(const std::shared_ptr<GeometryEvaluation>& task) {
    if(session_.contains("invalidNamedInput"))return;
    if(document_!=task->document || document_->revision()!=task->revision || document_->authored()!=task->base
        || session_.value("id")!=task->sessionId || session_.value("inputs")!=task->inputs || geometryEvaluationGeneration_!=task->generation)return;
    session_["evaluationTimingMs"]=QJsonObject{{"full",task->fullMs},{"historicalStage",task->stageMs},{"displayMesh",task->sourceMs}};
    if(!task->error.isEmpty() || !task->full.ok || !task->stage.ok){
        const auto message=!task->error.isEmpty() ? task->error : !task->full.ok ? task->full.error : task->stage.error;
        session_["evaluationStatus"]="failed";session_["coilCommitRequested"]=false;
        session_["previewError"]=QJsonObject{{"code","evaluation_failed"},{"message",message}};
        if(primitiveError_)primitiveError_->setText(message);view_->clearPreview();
        if(session_.value("coilDirectRequest").toBool()){
            const auto terminal=session_;finishSession(task->sessionId,false);geometryEvaluationTerminal_=terminal;
        }
        return;
    }
    QElapsedTimer publication;publication.start();
    coilPrepared_=task;view_->setPreparedThreadMeshes(task->preparedDisplayMeshes);
    session_["evaluationStatus"]="completed";session_["previewed"]=true;session_["geometryEvaluated"]=true;session_["documentDraft"]=task->candidate;
    for(const auto& body:task->stage.evaluation->bodies)if(body.bodyId==task->feature.value("bodyId")){
        view_->showPreview(body.shape,task->inputs.contains("featureId") ? body.bodyId : QString{},true);
        session_["geometry"]=body.summary;
        if(document_->bodies().empty() && !session_.value("coilPreviewFramed").toBool()){view_->fit({body});session_["coilPreviewFramed"]=true;}
        break;
    }
    if(primitiveError_)primitiveError_->clear();
    if(session_.value("coilCommitRequested").toBool()){
        const auto response=finishSession(task->sessionId,true);
        if(response.contains("ok") && !response.value("ok").toBool()){
            session_["evaluationStatus"]="failed";session_["previewError"]=response.value("error");
        }
    }
    refresh();const auto recoveryError=syncRecovery();
    auto timing=session_.value("id")==task->sessionId ? session_.value("evaluationTimingMs").toObject() : geometryEvaluationTerminal_.value("evaluationTimingMs").toObject();
    timing["publication"]=publication.elapsed();
    if(session_.value("id")==task->sessionId)session_["evaluationTimingMs"]=timing;
    if(geometryEvaluationTerminal_.value("id")==task->sessionId)geometryEvaluationTerminal_["evaluationTimingMs"]=timing;
    if(!geometryEvaluationTerminal_.isEmpty())geometryEvaluationTerminal_["recovery"]=QJsonObject{{"durable",recoveryError.isEmpty()},{"warning",recoveryError}};
}
