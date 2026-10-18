// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <QJsonArray>
#include <cmath>
namespace {
QJsonObject failure(const QString& code,const QString& message){return {{"ok",false},{"error",QJsonObject{{"code",code},{"message",message}}}};}
}
QJsonObject AlloyWindow::appendThreePointCircle(const QJsonObject& inputs) {
    if(session_.value("command")!="sketch.edit")return failure("no_active_sketch","Enter a sketch first");
    if(inputs.contains("session") && inputs["session"]!=session_.value("id"))return failure("invalid_session","Session is no longer active");
    if(session_.value("baseDocumentRevision").toInteger()!=static_cast<qint64>(document_->revision()))return failure("stale_session","The document changed; cancel and restart this sketch");
    const auto units=inputs.value("units").toString(session_.value("inputs").toObject()["units"].toString());QJsonObject canonical;
    for(int i=1;i<=3;++i){canonical[QString("u%1Mm").arg(i)]=CadDocument::millimeters(inputs[QString("x%1").arg(i)].toDouble(),units);canonical[QString("v%1Mm").arg(i)]=CadDocument::millimeters(inputs[QString("y%1").arg(i)].toDouble(),units);}
    for(const auto& key:{"firstSnapTarget","secondSnapTarget","thirdSnapTarget"})if(inputs.contains(key))canonical[key]=inputs[key];
    return publishSketchSolve(SketchGeometry::addThreePointCircle(session_.value("draftSketch").toObject(),canonical,session_.value("projectionContext").toObject()));
}
QJsonObject AlloyWindow::threePointCircleGesture(const QJsonObject& pointer,bool commit) {
    const auto plane=session_.value("inputs").toObject()["plane"].toString();const auto pixel=view_->projectSketchPoint(pointer["uMm"].toDouble(),pointer["vMm"].toDouble(),plane);
    auto snap=resolveSketchSnap(pixel["x"].toDouble(),pixel["y"].toDouble(),plane,false);
    if(snap["status"]=="acquired"){
        const auto kind=snap["target"].toObject()["kind"].toString();if(kind!="origin" && kind!="point")snap["status"]="none";
    }
    session_["snap"]=snap;view_->showSketchSnap(plane,snap);
    if(snap["status"]=="ambiguous"){view_->clearSketchCursor();view_->showSketchSnap(plane,snap);return failure("ambiguous_snap","Choose one unambiguous circumference point");}
    const auto point=snap["status"]=="acquired" ? snap : pointer;session_["pointerPoint"]=point;
    if(!session_.contains("firstPoint")){
        if(commit)session_["firstPoint"]=point;
        return {{"placement",false},{"nextPoint","second circumference point"},{"snap",snap},{"authoredChange",false}};
    }
    const auto first=session_.value("firstPoint").toObject();
    if(!session_.contains("secondPoint")){
        if(std::hypot(point["uMm"].toDouble()-first["uMm"].toDouble(),point["vMm"].toDouble()-first["vMm"].toDouble())<0.001){
            view_->clearSketchCursor();view_->showSketchSnap(plane,snap);
            return commit ? failure("circle_points_required","Choose distinct circumference points") : QJsonObject{{"previewed",false},{"authoredChange",false}};
        }
        view_->showSketchCursor(plane,{first["uMm"].toDouble(),first["vMm"].toDouble()},{point["uMm"].toDouble(),point["vMm"].toDouble()},false);view_->showSketchSnap(plane,snap);
        if(commit)session_["secondPoint"]=point;
        return {{"placement",false},{"nextPoint","third circumference point"},{"snap",snap},{"authoredChange",false}};
    }
    const auto second=session_.value("secondPoint").toObject();QJsonObject inputs{{"x1",first["uMm"]},{"y1",first["vMm"]},{"x2",second["uMm"]},{"y2",second["vMm"]},{"x3",point["uMm"]},{"y3",point["vMm"]},{"units","mm"}};
    if(first["status"]=="acquired")inputs["firstSnapTarget"]=first["target"];
    if(second["status"]=="acquired")inputs["secondSnapTarget"]=second["target"];
    if(point["status"]=="acquired")inputs["thirdSnapTarget"]=point["target"];
    if(commit){const auto response=execute("sketch.threePointCircle",inputs);if(!response["ok"].toBool())return response;view_->setFocus(Qt::OtherFocusReason);return {{"placement",true},{"draftSketch",session_.value("draftSketch")}};}
    QJsonObject canonical{{"u1Mm",first["uMm"]},{"v1Mm",first["vMm"]},{"u2Mm",second["uMm"]},{"v2Mm",second["vMm"]},{"u3Mm",point["uMm"]},{"v3Mm",point["vMm"]}};
    for(const auto& key:{"firstSnapTarget","secondSnapTarget","thirdSnapTarget"})if(inputs.contains(key))canonical[key]=inputs[key];
    const auto result=SketchGeometry::addThreePointCircle(session_.value("draftSketch").toObject(),canonical,session_.value("projectionContext").toObject());
    if(result.ok)view_->showSketchEdgesCursor(result.feature,QJsonArray{result.feature["parameters"].toObject()["entities"].toArray().last().toObject()["id"]});else view_->clearSketchCursor();
    view_->showSketchSnap(plane,snap);
    return {{"previewed",result.ok},{"authoredChange",false},{"snap",snap}};
}
