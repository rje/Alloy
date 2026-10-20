// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "ReferenceView.h"
QJsonObject AlloyWindow::beginAlign(const QJsonObject& inputs){
    if(!session_.isEmpty())return {{"ok",false},{"error",QJsonObject{{"code","session_busy"},{"message","Complete or cancel the active command first"}}}};
    const auto error=validateArguments(*findCommand("solid.align"),inputs);if(!error.isEmpty())return {{"ok",false},{"error",QJsonObject{{"code","invalid_arguments"},{"message",error}}}};
    session_={{"id",CadDocument::newId()},{"command","solid.align"},{"status","active"},{"inputs",inputs},{"newFeatureId",CadDocument::newId()},{"baseDocumentRevision",static_cast<qint64>(document_->revision())},{"previewed",false},{"inputSchema",findCommand("solid.align")->parameters}};
    return session_;
}
QJsonObject AlloyWindow::previewAlign(){
    view_->clearPreview();view_->clearBodyStage();session_["previewed"]=false;session_["geometryEvaluated"]=false;session_.remove("geometry");session_.remove("documentDraft");session_.remove("previewError");
    auto reject=[this](const QString& code,const QString& message){QJsonObject error{{"code",code},{"message",message}};session_["previewError"]=error;return QJsonObject{{"ok",false},{"completion","failed"},{"error",error}};};
    if(session_.value("baseDocumentRevision").toInteger()!=static_cast<qint64>(document_->revision()))return reject("stale_session","The document changed; cancel and restart Align");
    auto args=session_.value("inputs").toObject();if(!args.contains("featureId"))args["newFeatureId"]=session_.value("newFeatureId");
    QJsonObject candidate;try{candidate=CadDocument::alignCandidate(document_->authored(),args);}catch(const std::exception& e){return reject("invalid_align",QString::fromUtf8(e.what()));}
    const auto evaluated=CadDocument::evaluateCandidate(candidate);if(!evaluated.ok){const auto response=reject(evaluated.diagnostics.value("code").toString("evaluation_failed"),evaluated.error);session_["diagnostics"]=evaluated.diagnostics;return response;}
    auto stage=evaluated.evaluation;
    if(args.contains("featureId")){
        auto prefix=candidate;QJsonArray features;for(const auto& f:candidate.value("features").toArray()){features.append(f);if(f.toObject().value("id")==args.value("featureId"))break;}prefix["features"]=features;
        const auto beforeDownstream=CadDocument::evaluateCandidate(prefix);if(!beforeDownstream.ok)return reject("evaluation_failed",beforeDownstream.error);stage=beforeDownstream.evaluation;
    }
    const auto source=args.value("sourceFace").toObject().value("bodyId").toString();
    for(const auto& body:stage->bodies)if(body.bodyId==source){session_["geometry"]=body.summary;break;}
    view_->setBodyStage(stage->bodies,"align:"+session_.value("id").toString());session_["documentDraft"]=candidate;session_["previewed"]=true;session_["geometryEvaluated"]=true;return session_;
}
