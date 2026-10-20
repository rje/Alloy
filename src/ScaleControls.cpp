// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "ReferenceView.h"
QJsonObject AlloyWindow::beginScale(const QJsonObject& overrides){
    auto inputs=overrides;if(!inputs.contains("factor"))inputs["factor"]=1.0;
    if(overrides.contains("featureId")){for(const auto& row:document_->authored().value("features").toArray())if(row.toObject().value("id")==overrides.value("featureId") && row.toObject().value("type")=="scale"){inputs=row.toObject().value("parameters").toObject();inputs["featureId"]=overrides.value("featureId");for(auto it=overrides.begin();it!=overrides.end();++it)inputs[it.key()]=it.value();break;}}
    if(!session_.isEmpty())return {{"ok",false},{"error",QJsonObject{{"code","session_busy"},{"message","Complete or cancel the active command first"}}}};
    const auto error=validateArguments(*findCommand("solid.scale"),inputs);if(!error.isEmpty())return {{"ok",false},{"error",QJsonObject{{"code","invalid_arguments"},{"message",error}}}};
    session_={{"id",CadDocument::newId()},{"command","solid.scale"},{"status","active"},{"inputs",inputs},{"newFeatureId",CadDocument::newId()},{"baseDocumentRevision",static_cast<qint64>(document_->revision())},{"previewed",false},{"inputSchema",findCommand("solid.scale")->parameters}};
    return session_;
}
QJsonObject AlloyWindow::previewScale(){
    view_->clearPreview();view_->clearBodyStage();session_["previewed"]=false;session_["geometryEvaluated"]=false;session_.remove("geometry");session_.remove("documentDraft");session_.remove("previewError");
    auto reject=[this](const QString& code,const QString& message){QJsonObject error{{"code",code},{"message",message}};session_["previewError"]=error;return QJsonObject{{"ok",false},{"completion","failed"},{"error",error}};};
    if(session_.value("baseDocumentRevision").toInteger()!=static_cast<qint64>(document_->revision()))return reject("stale_session","The document changed; cancel and restart Scale");
    auto args=session_.value("inputs").toObject();if(!args.contains("featureId"))args["newFeatureId"]=session_.value("newFeatureId");
    QJsonObject candidate;try{candidate=CadDocument::scaleCandidate(document_->authored(),args);}catch(const std::exception& e){return reject("invalid_scale",QString::fromUtf8(e.what()));}
    const auto evaluated=CadDocument::evaluateCandidate(candidate);if(!evaluated.ok){const auto response=reject(evaluated.diagnostics.value("code").toString("evaluation_failed"),evaluated.error);session_["diagnostics"]=evaluated.diagnostics;return response;}
    auto stage=evaluated.evaluation;
    if(args.contains("featureId")){
        auto prefix=candidate;QJsonArray features;for(const auto& f:candidate.value("features").toArray()){features.append(f);if(f.toObject().value("id")==args.value("featureId"))break;}prefix["features"]=features;
        const auto beforeDownstream=CadDocument::evaluateCandidate(prefix);if(!beforeDownstream.ok)return reject("evaluation_failed",beforeDownstream.error);stage=beforeDownstream.evaluation;
    }
    const auto source=args.value("targetBodyId").toString();
    for(const auto& body:stage->bodies)if(body.bodyId==source){session_["geometry"]=body.summary;break;}
    view_->setBodyStage(stage->bodies,"scale:"+session_.value("id").toString());session_["documentDraft"]=candidate;session_["previewed"]=true;session_["geometryEvaluated"]=true;return session_;
}
