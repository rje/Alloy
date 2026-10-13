// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "ReferenceView.h"
namespace {QJsonObject failure(const QString& code,const QString& message){return {{"ok",false},{"error",QJsonObject{{"code",code},{"message",message}}}};}}
QJsonObject AlloyWindow::sweepCandidate(const QJsonObject& inputs) const {
    auto args=inputs;if(!args.contains("featureId") && session_.value("command")=="solid.sweep"){args["newFeatureId"]=session_.value("newFeatureId");args["newBodyId"]=session_.value("newBodyId");}
    auto candidate=CadDocument::sweepCandidate(document_->authored(),authoringArguments(args));
    if(!inputs.contains("featureId")){auto features=candidate["features"].toArray();for(int i=0;i<features.size();++i){auto f=features[i].toObject();if(f["id"]==inputs["profileSketchId"] || f["id"]==inputs["pathSketchId"]){f["visible"]=false;features[i]=f;}}candidate["features"]=features;}return candidate;
}
QJsonObject AlloyWindow::beginSweep(const QJsonObject& overrides) {
    if(!session_.isEmpty())return failure("dialog_busy","Finish or cancel the active command first");
    QJsonObject inputs{{"extent","full"},{"orientation","perpendicular"},{"operation","newBody"},{"name","Sweep"}};
    if(overrides.contains("featureId")){bool found=false;for(const auto& v:document_->authored()["features"].toArray()){const auto f=v.toObject();if(f["id"]!=overrides["featureId"] || f["type"]!="sweep")continue;inputs=f["parameters"].toObject();inputs["featureId"]=f["id"];inputs["name"]=f["name"];found=true;}if(!found)return failure("feature_unavailable","Sweep feature unavailable");}
    for(auto it=overrides.begin();it!=overrides.end();++it)inputs[it.key()]=it.value();const auto error=validateArguments(*findCommand("solid.sweep"),inputs);if(!error.isEmpty())return failure("invalid_arguments",error);
    session_={{"id",CadDocument::newId()},{"command","solid.sweep"},{"status","active"},{"inputs",inputs},{"newFeatureId",CadDocument::newId()},{"newBodyId",CadDocument::newId()},{"baseDocumentRevision",static_cast<qint64>(document_->revision())},{"previewed",false},{"geometryEvaluated",false},{"inputSchema",findCommand("solid.sweep")->parameters},{"nativeControlsAvailable",false}};
    if(inputs.contains("featureId"))view_->setBodyStage(document_->evaluationBefore(inputs["featureId"].toString()).bodies,"sweep-source:"+session_.value("id").toString());return session_;
}
QJsonObject AlloyWindow::previewSweep() {
    if(session_.value("command")!="solid.sweep")return failure("invalid_session","Sweep is not active");
    session_["previewed"]=false;session_["geometryEvaluated"]=false;session_.remove("geometry");session_.remove("documentDraft");view_->clearPreview();
    auto reject=[this](const QString& code,const QString& message,const QJsonObject& diagnostics=QJsonObject{}){auto r=failure(code,message);auto e=r["error"].toObject();e["diagnostics"]=diagnostics;r["error"]=e;session_["previewError"]=e;return r;};
    if(session_.value("baseDocumentRevision").toInteger()!=static_cast<qint64>(document_->revision()))return reject("stale_session","The document changed; cancel and restart Sweep");
    const auto inputs=session_.value("inputs").toObject();QJsonObject candidate;try{candidate=sweepCandidate(inputs);}catch(const std::exception& e){return reject("invalid_arguments",QString::fromUtf8(e.what()));}
    auto evaluated=CadDocument::evaluateCandidate(candidate);if(!evaluated.ok)return reject("evaluation_failed",evaluated.error,evaluated.diagnostics);
    const auto id=inputs.value("featureId").toString(session_.value("newFeatureId").toString());auto stage=evaluated.evaluation;
    if(inputs.contains("featureId")){QJsonArray prefix;for(const auto& v:candidate["features"].toArray()){prefix.append(v);if(v.toObject()["id"]==id)break;}auto early=candidate;early["features"]=prefix;auto result=CadDocument::evaluateCandidate(early);if(!result.ok)return reject("evaluation_failed",result.error,result.diagnostics);stage=result.evaluation;}
    for(const auto& body:stage->bodies)if(body.featureId==id){view_->showPreview(body.shape,inputs.contains("featureId")?body.bodyId:QString{},true);session_["geometry"]=body.summary;session_["featureId"]=id;session_["bodyId"]=body.bodyId;}
    session_["documentDraft"]=candidate;session_["previewed"]=true;session_["geometryEvaluated"]=true;session_.remove("previewError");return session_;
}
