// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <QJsonArray>
namespace {
QJsonObject failure(const QString& code,const QString& message){return {{"ok",false},{"error",QJsonObject{{"code",code},{"message",message}}}};}
}
void AlloyWindow::clearThreeTangentCircleInput(){
    session_.remove("threeTangentCircleLineIds");session_.remove("threeTangentCirclePreview");session_.remove("threeTangentCircleHoverLineId");
}
QJsonObject AlloyWindow::appendThreeTangentCircle(const QJsonObject& inputs){
    if(session_.value("command")!="sketch.edit")return failure("no_active_sketch","Enter a sketch first");
    if(inputs.contains("session") && inputs["session"]!=session_.value("id"))return failure("invalid_session","Session is no longer active");
    const auto result=publishSketchSolve(SketchGeometry::addThreeTangentCircle(session_.value("draftSketch").toObject(),{{"lineIds",inputs["lineIds"]}},session_.value("projectionContext").toObject()));
    if(!result.contains("error"))clearThreeTangentCircleInput();return result;
}
QJsonObject AlloyWindow::threeTangentCircleGesture(const QJsonObject& pixels,bool commit){
    auto ids=session_.value("threeTangentCircleLineIds").toArray();const auto draft=session_.value("draftSketch").toObject();
    const auto hit=view_->pickSketch(draft,pixels["x"].toDouble(),pixels["y"].toDouble(),"entity");
    const auto id=hit["entityId"];bool line=false;for(const auto& row:draft["parameters"].toObject()["entities"].toArray())if(row.toObject()["id"]==id)line=row.toObject()["type"]=="line";
    if(!line || ids.contains(id)){
        session_.remove("threeTangentCircleHoverLineId");session_["threeTangentCirclePreview"]=false;view_->clearSketchCursor();
        return commit ? failure("invalid_tangent_line","Choose three distinct authored straight sketch lines") : QJsonObject{{"previewed",false},{"authoredChange",false}};
    }
    if(ids.size()<2){
        if(commit){ids.append(id);session_["threeTangentCircleLineIds"]=ids;session_["selection"]=QJsonObject{{"pointIds",QJsonArray{}},{"entityIds",ids},{"profileIds",QJsonArray{}}};}
        return {{"lineIds",ids},{"authoredChange",false}};
    }
    ids.append(id);session_["threeTangentCircleHoverLineId"]=id;
    if(commit){
        const auto response=execute("sketch.threeTangentCircle",{{"lineIds",ids}});if(!response["ok"].toBool())return response;
        session_["selection"]=QJsonObject{{"pointIds",QJsonArray{}},{"entityIds",QJsonArray{}},{"profileIds",QJsonArray{}}};view_->setFocus(Qt::OtherFocusReason);return {{"placement",true},{"authoredChange",true}};
    }
    const auto result=SketchGeometry::addThreeTangentCircle(draft,{{"lineIds",ids}},session_.value("projectionContext").toObject());session_["threeTangentCirclePreview"]=result.ok;
    if(result.ok)view_->showSketchEdgesCursor(result.feature,QJsonArray{result.diagnostics["createdCircleId"]});else view_->clearSketchCursor();
    return {{"previewed",result.ok},{"authoredChange",false},{"message",result.error}};
}
