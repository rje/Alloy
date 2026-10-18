// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <QLineEdit>
#include <QJsonArray>
namespace {
QJsonObject failure(const QString& code,const QString& message){return {{"ok",false},{"error",QJsonObject{{"code",code},{"message",message}}}};}
}
void AlloyWindow::clearTwoTangentCircleInput(){
    session_.remove("tangentCircleLineIds");session_.remove("tangentCirclePointer");session_.remove("tangentCirclePreview");
    if(tangentCircleRadius_) {tangentCircleRadius_->hide();tangentCircleRadius_->clear();}
}
QJsonObject AlloyWindow::appendTwoTangentCircle(const QJsonObject& inputs){
    if(session_.value("command")!="sketch.edit")return failure("no_active_sketch","Enter a sketch first");
    if(inputs.contains("session") && inputs["session"]!=session_.value("id"))return failure("invalid_session","Session is no longer active");
    const auto units=inputs.value("units").toString(session_.value("inputs").toObject()["units"].toString());
    QJsonObject canonical{{"lineIds",inputs["lineIds"]},{"uMm",CadDocument::millimeters(inputs["x"].toDouble(),units)},{"vMm",CadDocument::millimeters(inputs["y"].toDouble(),units)}};
    if(inputs.contains("radius"))canonical["radiusMm"]=CadDocument::millimeters(inputs["radius"].toDouble(),units);
    const auto result=publishSketchSolve(SketchGeometry::addTwoTangentCircle(session_.value("draftSketch").toObject(),canonical,session_.value("projectionContext").toObject()));
    if(!result.contains("error"))clearTwoTangentCircleInput();return result;
}
QJsonObject AlloyWindow::twoTangentCircleGesture(const QJsonObject& pixels,bool commit){
    auto ids=session_.value("tangentCircleLineIds").toArray();const auto draft=session_.value("draftSketch").toObject();
    if(ids.size()<2){
        if(!commit)return {{"previewed",false},{"authoredChange",false}};
        const auto hit=view_->pickSketch(draft,pixels["x"].toDouble(),pixels["y"].toDouble(),"entity");
        const auto id=hit["entityId"];bool line=false;for(const auto& row:draft["parameters"].toObject()["entities"].toArray())if(row.toObject()["id"]==id)line=row.toObject()["type"]=="line";
        if(!line || ids.contains(id))return failure("invalid_tangent_line","Choose two distinct authored straight sketch lines");
        ids.append(id);session_["tangentCircleLineIds"]=ids;
        session_["selection"]=QJsonObject{{"pointIds",QJsonArray{}},{"entityIds",ids},{"profileIds",QJsonArray{}}};
        if(ids.size()==2){
            if(!tangentCircleRadius_){
                tangentCircleRadius_=new QLineEdit(view_);tangentCircleRadius_->setAttribute(Qt::WA_NativeWindow);tangentCircleRadius_->setAccessibleName("Radius");tangentCircleRadius_->setObjectName("sketch.twoTangentCircle.radius");tangentCircleRadius_->setPlaceholderText("Radius");tangentCircleRadius_->setToolTip("Radius · type a value, then click to place");tangentCircleRadius_->setFixedWidth(150);tangentCircleRadius_->move(24,24);tangentCircleRadius_->installEventFilter(this);
                connect(tangentCircleRadius_,&QLineEdit::textChanged,this,[this]{if(session_.value("tool")=="twoTangentCircle" && session_.contains("tangentCirclePointer"))twoTangentCircleGesture(session_.value("tangentCirclePointer").toObject(),false);});
                connect(tangentCircleRadius_,&QLineEdit::returnPressed,this,[this]{view_->setFocus(Qt::OtherFocusReason);});
            }
            tangentCircleRadius_->show();tangentCircleRadius_->raise();
        }
        return {{"lineIds",ids},{"authoredChange",false}};
    }
    session_["tangentCirclePointer"]=pixels;
    const auto pointer=view_->sketchPoint(pixels["x"].toDouble(),pixels["y"].toDouble(),session_.value("inputs").toObject()["plane"].toString());
    QJsonObject canonical{{"lineIds",ids},{"uMm",pointer["uMm"]},{"vMm",pointer["vMm"]}};
    if(tangentCircleRadius_ && !tangentCircleRadius_->text().isEmpty()){
        bool valid=false;const double radius=tangentCircleRadius_->text().toDouble(&valid);
        if(!valid){view_->clearSketchCursor();session_["tangentCirclePreview"]=false;return failure("invalid_radius","Enter a positive numeric radius");}
        canonical["radiusMm"]=CadDocument::millimeters(radius,session_.value("inputs").toObject()["units"].toString());
    }
    const auto result=SketchGeometry::addTwoTangentCircle(draft,canonical,session_.value("projectionContext").toObject());session_["tangentCirclePreview"]=result.ok;
    if(result.ok && tangentCircleRadius_ && tangentCircleRadius_->text().isEmpty()){
        const double radius=result.feature["parameters"].toObject()["entities"].toArray().last().toObject()["radiusMm"].toDouble();
        const auto units=session_.value("inputs").toObject()["units"].toString();
        tangentCircleRadius_->setPlaceholderText(QString("Radius %1 %2").arg(radius/(units=="in" ? 25.4 : 1.0),0,'g',6).arg(units));
    }
    if(commit){
        QJsonObject args{{"lineIds",ids},{"x",pointer["uMm"]},{"y",pointer["vMm"]},{"units","mm"}};if(canonical.contains("radiusMm"))args["radius"]=canonical["radiusMm"];
        const auto response=execute("sketch.twoTangentCircle",args);if(!response["ok"].toBool())return response;
        session_["selection"]=QJsonObject{{"pointIds",QJsonArray{}},{"entityIds",QJsonArray{}},{"profileIds",QJsonArray{}}};view_->setFocus(Qt::OtherFocusReason);return {{"placement",true},{"authoredChange",true}};
    }
    if(result.ok)view_->showSketchEdgesCursor(result.feature,QJsonArray{result.diagnostics["createdCircleId"]});else view_->clearSketchCursor();
    return {{"previewed",result.ok},{"authoredChange",false},{"diagnostics",result.diagnostics},{"message",result.error}};
}
