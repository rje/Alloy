#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <QFrame>
#include <QJsonArray>
#include <QLabel>
#include <QLineEdit>
#include <QShortcut>
#include <QSignalBlocker>
#include <QVBoxLayout>
#include <cmath>
#include <algorithm>
namespace {
QJsonObject failure(const QString& code,const QString& message){return {{"ok",false},{"error",QJsonObject{{"code",code},{"message",message}}}};}
}

QJsonObject AlloyWindow::appendThreePointRectangle(const QJsonObject& inputs) {
    if(session_.value("command")!="sketch.edit")return failure("no_active_sketch","Enter a sketch first");
    if(inputs.contains("session") && inputs["session"]!=session_.value("id"))return failure("invalid_session","Session is no longer active");
    if(session_.value("baseDocumentRevision").toInteger()!=static_cast<qint64>(document_->revision()))return failure("stale_session","The document changed; cancel and restart this sketch");
    const auto units=inputs.value("units").toString(session_.value("inputs").toObject()["units"].toString());
    QJsonObject canonical;
    for(const auto& item:{std::pair{"x1","u1Mm"},std::pair{"y1","v1Mm"},std::pair{"x2","u2Mm"},std::pair{"y2","v2Mm"},std::pair{"x3","u3Mm"},std::pair{"y3","v3Mm"},std::pair{"firstDistance","firstDistanceMm"},std::pair{"secondDistance","secondDistanceMm"}})
        if(inputs.contains(item.first))canonical[item.second]=CadDocument::millimeters(inputs[item.first].toDouble(),units);
    if(inputs.contains("startSnapTarget"))canonical["startSnapTarget"]=inputs["startSnapTarget"];
    return publishSketchSolve(SketchGeometry::addThreePointRectangle(session_.value("draftSketch").toObject(),canonical,session_.value("projectionContext").toObject()));
}

void AlloyWindow::clearThreePointRectangleInput() {
    session_.remove("pendingThreePointDistance");session_.remove("threePointFirstDistanceMm");
    if(threePointRectangleEditor_)threePointRectangleEditor_->hide();
}

QJsonObject AlloyWindow::threePointRectangleDistance(const QJsonObject& inputs,bool lock) {
    if(session_.value("command")!="sketch.edit" || session_.value("tool")!="threePointRectangle" || !session_.contains("firstPoint"))
        return failure("rectangle_distance_unavailable","Place the first rectangle corner before entering a distance");
    if(!threePointRectangleEditor_){
        auto* frame=new QFrame(view_);threePointRectangleEditor_=frame;frame->setObjectName("sketch.threePointRectangle.editor");
        frame->setAttribute(Qt::WA_NativeWindow);frame->setAutoFillBackground(true);frame->setFixedWidth(245);
        frame->setStyleSheet("#sketch\\.threePointRectangle\\.editor { background: #f7f8f9; border: 1px solid #9aabb8; } QLineEdit { background: white; color: #20252a; padding: 5px; }");
        auto* layout=new QVBoxLayout(frame);layout->setContentsMargins(7,7,7,7);layout->setSpacing(4);
        threePointRectangleLabel_=new QLabel;layout->addWidget(threePointRectangleLabel_);
        threePointRectangleDistance_=new QLineEdit;threePointRectangleDistance_->setObjectName("sketch.threePointRectangle.distance");layout->addWidget(threePointRectangleDistance_);threePointRectangleDistance_->installEventFilter(this);
        threePointRectangleError_=new QLabel;threePointRectangleError_->setWordWrap(true);layout->addWidget(threePointRectangleError_);
        connect(threePointRectangleDistance_,&QLineEdit::textChanged,this,[this](const QString& text){if(session_.value("tool")=="threePointRectangle")sketchResponse(execute("sketch.threePointRectangleDistancePreview",{{"text",text}}));});
        auto* escape=new QShortcut(QKeySequence(Qt::Key_Escape),frame);escape->setContext(Qt::WidgetWithChildrenShortcut);
        connect(escape,&QShortcut::activated,this,[this]{execute("sketch.tool",{{"tool","select"}});});
    }
    const auto text=inputs.value("text").toString();bool valid=false;const double raw=text.trimmed().toDouble(&valid);
    const auto units=session_.value("inputs").toObject()["units"].toString();const double length=CadDocument::millimeters(raw,units);
    valid=valid && std::isfinite(length) && length>=0.001 && length<=100000;
    const bool opening=!threePointRectangleEditor_->isVisible();
    QJsonObject pending{{"text",text},{"valid",valid},{"locked",lock && valid}};if(valid)pending["lengthMm"]=length;
    session_["pendingThreePointDistance"]=pending;
    {const QSignalBlocker block(threePointRectangleDistance_);threePointRectangleDistance_->setText(text);}
    threePointRectangleDistance_->setReadOnly(lock && valid);
    threePointRectangleLabel_->setText(session_.contains("secondPoint") ? "Second distance" : "First distance");
    threePointRectangleError_->setText(!valid ? "Enter a distance from 0.001 to 100000 mm" : lock ? "Distance locked · click to place · Esc cancels" : "Tab locks · pointer sets direction · Esc cancels");
    threePointRectangleEditor_->adjustSize();
    const auto pointer=session_.value("pointerPoint").toObject();const auto pixel=view_->projectSketchPoint(pointer["uMm"].toDouble(),pointer["vMm"].toDouble(),session_.value("inputs").toObject()["plane"].toString());
    threePointRectangleEditor_->move(std::clamp(qRound(pixel["x"].toDouble())+18,0,std::max(0,view_->width()-245)),std::clamp(qRound(pixel["y"].toDouble())+18,0,std::max(0,view_->height()-threePointRectangleEditor_->height())));
    threePointRectangleEditor_->show();threePointRectangleEditor_->raise();
    if(opening){threePointRectangleDistance_->setFocus();threePointRectangleDistance_->setCursorPosition(text.size());}
    if(valid && !pointer.isEmpty())threePointRectangleGesture(pointer,false);
    if(lock){if(!valid)return failure("invalid_rectangle_distance","Enter a positive rectangle distance");view_->setFocus(Qt::OtherFocusReason);}
    return {{"pendingDistance",pending},{"authoredChange",false}};
}

QJsonObject AlloyWindow::threePointRectangleGesture(const QJsonObject& pointer,bool commit) {
    auto point=pointer;session_["pointerPoint"]=point;const auto plane=session_.value("inputs").toObject()["plane"].toString();
    if(!session_.contains("firstPoint")){
        const auto pixel=view_->projectSketchPoint(point["uMm"].toDouble(),point["vMm"].toDouble(),plane);
        auto snap=resolveSketchSnap(pixel["x"].toDouble(),pixel["y"].toDouble(),plane,true);
        if(snap["status"]=="acquired"){
            const auto target=snap["target"].toObject();if(target["kind"]!="origin" && target["kind"]!="point" && target["kind"]!="midpoint")snap["status"]="none";
        }
        session_["snap"]=snap;view_->showSketchSnap(plane,snap);
        if(snap["status"]=="ambiguous")return failure("ambiguous_snap","Choose one unambiguous first corner");
        if(commit)session_["firstPoint"]=snap["status"]=="acquired" ? snap : point;
        return {{"placement",false},{"nextPoint","first-side endpoint"},{"authoredChange",false}};
    }
    const auto pending=session_.value("pendingThreePointDistance").toObject();
    if(!pending.isEmpty() && (!pending["valid"].toBool() || !pending["locked"].toBool())){
        if(commit)return failure("rectangle_distance_pending","Enter a valid distance and press Tab before clicking");
        if(!pending["valid"].toBool())return {{"previewed",false},{"authoredChange",false}};
    }
    const auto first=session_.value("firstPoint").toObject();const double ax=first["uMm"].toDouble(),ay=first["vMm"].toDouble();
    if(!session_.contains("secondPoint")){
        const double dx=point["uMm"].toDouble()-ax,dy=point["vMm"].toDouble()-ay,length=std::hypot(dx,dy);
        if(length<0.001){view_->clearSketchCursor();return commit ? failure("rectangle_side_required","Place the endpoint away from the first corner") : QJsonObject{{"previewed",false},{"authoredChange",false}};}
        const double size=pending.isEmpty() ? length : pending["lengthMm"].toDouble();point={{"uMm",ax+dx*size/length},{"vMm",ay+dy*size/length}};
        view_->showSketchCursor(plane,{ax,ay},{point["uMm"].toDouble(),point["vMm"].toDouble()},false);
        if(commit){session_["secondPoint"]=point;if(!pending.isEmpty())session_["threePointFirstDistanceMm"]=size;session_.remove("pendingThreePointDistance");if(threePointRectangleEditor_)threePointRectangleEditor_->hide();}
        return {{"placement",false},{"nextPoint","perpendicular second side"},{"authoredChange",false},{"previewed",true}};
    }
    const auto second=session_.value("secondPoint").toObject();const double bx=second["uMm"].toDouble(),by=second["vMm"].toDouble(),dx=bx-ax,dy=by-ay,length=std::hypot(dx,dy);
    double height=(dx*(point["vMm"].toDouble()-ay)-dy*(point["uMm"].toDouble()-ax))/length;
    if(!pending.isEmpty())height=std::copysign(pending["lengthMm"].toDouble(),height);
    const double cx=bx-dy*height/length,cy=by+dx*height/length;
    QJsonObject canonical{{"u1Mm",ax},{"v1Mm",ay},{"u2Mm",bx},{"v2Mm",by},{"u3Mm",cx},{"v3Mm",cy}};
    if(first["status"]=="acquired")canonical["startSnapTarget"]=first["target"];
    if(session_.contains("threePointFirstDistanceMm"))canonical["firstDistanceMm"]=session_.value("threePointFirstDistanceMm");
    if(!pending.isEmpty())canonical["secondDistanceMm"]=pending["lengthMm"];
    if(!commit){
        const auto result=SketchGeometry::addThreePointRectangle(session_.value("draftSketch").toObject(),canonical,session_.value("projectionContext").toObject());
        if(result.ok){const auto edges=result.feature["parameters"].toObject()["entities"].toArray();QJsonArray ids;for(int i=edges.size()-4;i<edges.size();++i)ids.append(edges[i].toObject()["id"]);view_->showSketchEdgesCursor(result.feature,ids);}else view_->clearSketchCursor();
        return {{"previewed",result.ok},{"authoredChange",false}};
    }
    // Native placement and local control use exactly the same authored command.
    QJsonObject inputs{{"x1",ax},{"y1",ay},{"x2",bx},{"y2",by},{"x3",cx},{"y3",cy},{"units","mm"}};
    if(canonical.contains("firstDistanceMm"))inputs["firstDistance"]=canonical["firstDistanceMm"];
    if(canonical.contains("secondDistanceMm"))inputs["secondDistance"]=canonical["secondDistanceMm"];
    if(canonical.contains("startSnapTarget"))inputs["startSnapTarget"]=canonical["startSnapTarget"];
    const auto published=execute("sketch.threePointRectangle",inputs);if(!published["ok"].toBool())return published;
    session_.remove("firstPoint");session_.remove("secondPoint");session_.remove("pointerPoint");clearThreePointRectangleInput();view_->clearSketchCursor();view_->setFocus(Qt::OtherFocusReason);
    return {{"placement",true},{"draftSketch",session_.value("draftSketch")}};
}
