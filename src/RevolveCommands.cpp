// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "ReferenceView.h"
namespace {QJsonObject failure(const QString& code,const QString& message){return {{"ok",false},{"error",QJsonObject{{"code",code},{"message",message}}}};}}
QJsonObject AlloyWindow::revolveCandidate(const QJsonObject& inputs) const {
    auto args=inputs;if(!args.contains("featureId") && session_["command"]=="solid.revolve"){args["newFeatureId"]=session_["newFeatureId"];args["newBodyId"]=session_["newBodyId"];}
    return CadDocument::revolveCandidate(document_->authored(),authoringArguments(args));
}
QJsonObject AlloyWindow::beginRevolve(const QJsonObject& overrides) {
    if(!session_.isEmpty())return failure("session_busy","Complete or cancel the active command first");
    QJsonObject inputs{{"extent","full"},{"angleDeg",360},{"name","Revolve"}};
    if(overrides.contains("featureId")){bool found=false;for(const auto& v:document_->authored()["features"].toArray()){const auto f=v.toObject();if(f["id"]!=overrides["featureId"])continue;if(f["type"]!="revolve")return failure("stale_entity","Revolve feature unavailable");inputs=f["parameters"].toObject();inputs["featureId"]=f["id"];inputs["name"]=f["name"];found=true;}if(!found)return failure("stale_entity","Revolve feature unavailable");}
    for(auto it=overrides.begin();it!=overrides.end();++it)inputs[it.key()]=it.value();const auto error=validateArguments(*findCommand("solid.revolve"),inputs);if(!error.isEmpty())return failure("invalid_arguments",error);
    session_={{"id",CadDocument::newId()},{"command","solid.revolve"},{"status","active"},{"inputs",inputs},{"newFeatureId",CadDocument::newId()},{"newBodyId",CadDocument::newId()},{"baseDocumentRevision",static_cast<qint64>(document_->revision())},{"previewed",false},{"geometryEvaluated",false},{"inputSchema",findCommand("solid.revolve")->parameters},{"nativeControlsAvailable",false}};
    if(inputs.contains("featureId"))view_->setBodyStage(document_->evaluationBefore(inputs["featureId"].toString()).bodies,"revolve-source:"+session_["id"].toString());previewRevolve();return session_;
}
QJsonObject AlloyWindow::previewRevolve() {
    session_["previewed"]=false;session_["geometryEvaluated"]=false;session_.remove("geometry");session_.remove("documentDraft");view_->clearPreview();
    auto reject=[this](const QString& code,const QString& message,const QJsonObject& diagnostics=QJsonObject{}){auto r=failure(code,message);auto e=r["error"].toObject();e["diagnostics"]=diagnostics;r["error"]=e;session_["previewError"]=e;return r;};
    if(session_["baseDocumentRevision"].toInteger()!=static_cast<qint64>(document_->revision()))return reject("stale_session","The document changed; cancel and restart Revolve");
    QJsonObject candidate;try{candidate=revolveCandidate(session_["inputs"].toObject());}catch(const std::exception& e){return reject("invalid_arguments",QString::fromUtf8(e.what()));}
    auto evaluated=CadDocument::evaluateCandidate(candidate);if(!evaluated.ok)return reject("evaluation_failed",evaluated.error,evaluated.diagnostics);
    const auto id=session_["inputs"].toObject().value("featureId").toString(session_["newFeatureId"].toString());auto stage=evaluated.evaluation;
    if(session_["inputs"].toObject().contains("featureId")){QJsonArray prefix;for(const auto& v:candidate["features"].toArray()){prefix.append(v);if(v.toObject()["id"]==id)break;}auto early=candidate;early["features"]=prefix;auto result=CadDocument::evaluateCandidate(early);if(!result.ok)return reject("evaluation_failed",result.error,result.diagnostics);stage=result.evaluation;}
    for(const auto& body:stage->bodies)if(body.featureId==id){view_->showPreview(body.shape,session_["inputs"].toObject().contains("featureId") ? body.bodyId:QString{},true);session_["geometry"]=body.summary;session_["featureId"]=id;session_["bodyId"]=body.bodyId;}
    session_["documentDraft"]=candidate;session_["previewed"]=true;session_["geometryEvaluated"]=true;session_.remove("previewError");return session_;
}
