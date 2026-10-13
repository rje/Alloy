// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "SketchConic.h"
#include "ReferenceView.h"
#include <QFrame>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QShortcut>
#include <QSignalBlocker>
#include <cmath>
#include <QJsonArray>

namespace {
QJsonObject failure(const QString& code,const QString& message) {
    return {{"ok",false},{"error",QJsonObject{{"code",code},{"message",message}}}};
}
}
QJsonObject AlloyWindow::conicCommand(const QString& operation,const QJsonObject& inputs) {
    if(session_.value("command")!="sketch.edit")
        return failure("no_active_sketch","Enter a sketch first");
    if(session_.value("baseDocumentRevision").toInteger()!=static_cast<qint64>(document_->revision()))
        return failure("stale_session","Restart the sketch after a document change");
    const auto draft=session_.value("draftSketch").toObject();
    const auto context=session_.value("projectionContext").toObject();
    if(operation=="cancel") {
        cancelConicInput();execute("sketch.tool",{{"tool","select"}});view_->setFocus();
        return {{"pending",false},{"authoredChange",false}};
    }
    if(operation=="preview" || operation=="accept") {
        auto pending=session_.value("pendingConic").toObject();
        if(pending["points"].toArray().size()!=3)return failure("conic_incomplete","Place start, end and tangent vertex first");
        if(pending["base"].toObject()!=draft) {cancelConicInput();return failure("stale_conic_preview","Sketch changed during preview; start the conic again");}
        if(operation=="accept") {
            if(!pending["valid"].toBool())return failure("invalid_conic_rho","Correct Rho before accepting the conic");
            const auto candidate=pending["previewDraft"].toObject();
            auto result=publishSketchSolve(SketchGeometry::solveDraft(candidate,context));
            if(!result.contains("error")) {cancelConicInput();execute("sketch.tool",{{"tool","select"}});view_->setFocus();}
            return result;
        }
        bool numeric=false;const double rho=inputs["text"].toString().toDouble(&numeric);
        const auto picks=pending["points"].toArray();
        const auto solved=SketchConic::add(draft,{{"start",picks[0]},{"end",picks[1]},{"vertex",picks[2]},{"rho",rho}},context);
        pending["valid"]=numeric&&std::isfinite(rho)&&solved.ok;
        pending["text"]=inputs["text"];
        if(pending["valid"].toBool()) {
            pending["rho"]=rho;pending["previewDraft"]=solved.feature;pending["diagnostics"]=solved.diagnostics;pending["entityId"]=solved.diagnostics["createdConicId"];
            view_->showSketchDraft(evaluateSketchDraft(solved.feature));
            auto selected=session_.value("selection").toObject();selected["tool"]="conic";
            selected["entityIds"]=QJsonArray{solved.diagnostics["createdConicId"]};
            view_->decorateSketch(solved.feature,selected,solved.diagnostics,session_.value("inputs").toObject()["units"].toString());
        }
        session_["pendingConic"]=pending;
        if(conicRhoInput_){QSignalBlocker blocker(conicRhoInput_);conicRhoInput_->setText(inputs["text"].toString());}
        if(conicAccept_)conicAccept_->setEnabled(pending["valid"].toBool());
        if(conicHint_)conicHint_->setText(pending["valid"].toBool()?"Rho · Enter or right-click OK": "Enter a number strictly between 0 and 1");
        return {{"pending",true},{"valid",pending["valid"]},{"authoredChange",false}};
    }
    SketchGeometry::SolveResult solved;
    if(operation=="add") solved=SketchConic::add(draft,inputs,context);
    else if(operation=="rho") solved=SketchConic::setRho(draft,inputs["entityId"].toString(),inputs["rho"].toDouble(),context);
    else return failure("unsupported_conic_operation","Unsupported conic command");
    auto result=publishSketchSolve(solved);
    if(solved.ok) {
        const auto id=operation=="add" ? solved.diagnostics["createdConicId"] : inputs["entityId"];
        setSketchSelection({{"entityIds",QJsonArray{id}}});
    }
    return result;
}

void AlloyWindow::cancelConicInput() {
    const bool pending=session_.contains("pendingConic");
    session_.remove("pendingConic");
    if(conicEditor_)conicEditor_->hide();
    if(pending && session_.value("command")=="sketch.edit") {
        view_->clearSketchCursor();
        view_->showSketchDraft(evaluateSketchDraft(session_.value("draftSketch").toObject()));
    }
}

void AlloyWindow::showConicEditor() {
    if(!conicEditor_) {
        conicEditor_=new QFrame(view_);conicEditor_->setObjectName("sketchConicEditor");
        conicEditor_->setAttribute(Qt::WA_NativeWindow);
        conicEditor_->setStyleSheet("#sketchConicEditor { background:#f7f8f9; border:1px solid #9aabb8; } QLabel { color:#203541; background:#f7f8f9; } QLineEdit { color:#20252a; background:white; padding:5px; } QPushButton { color:#203541; background:#e6eef2; padding:5px; }");
        auto* layout=new QVBoxLayout(conicEditor_);
        conicHint_=new QLabel("Rho · Enter or right-click OK");layout->addWidget(conicHint_);
        conicRhoInput_=new QLineEdit;conicRhoInput_->setObjectName("sketch.conic.rho");layout->addWidget(conicRhoInput_);
        conicAccept_=new QPushButton("OK");conicAccept_->setObjectName("sketch.conic.accept");layout->addWidget(conicAccept_);
        auto* cancel=new QPushButton("Cancel");layout->addWidget(cancel);
        connect(conicRhoInput_,&QLineEdit::textChanged,this,[this](const QString& text){execute("sketch.conicPreview",{{"text",text}});});
        connect(conicRhoInput_,&QLineEdit::returnPressed,this,[this]{sketchResponse(execute("sketch.conicAccept"));});
        connect(conicAccept_,&QPushButton::clicked,this,[this]{sketchResponse(execute("sketch.conicAccept"));});
        connect(cancel,&QPushButton::clicked,this,[this]{sketchResponse(execute("sketch.conicCancel"));});
        auto* escape=new QShortcut(QKeySequence(Qt::Key_Escape),conicEditor_);escape->setContext(Qt::WidgetWithChildrenShortcut);
        connect(escape,&QShortcut::activated,this,[this]{execute("sketch.conicCancel");});
        conicEditor_->setFixedWidth(245);
    }
    {QSignalBlocker blocker(conicRhoInput_);conicRhoInput_->setText(session_["pendingConic"].toObject()["text"].toString("0.5"));}
    conicEditor_->adjustSize();conicEditor_->move(12,12);conicEditor_->show();conicEditor_->raise();
}

QJsonObject AlloyWindow::conicPointer(const QJsonObject& inputs,bool place) {
    auto pending=session_.value("pendingConic").toObject();
    auto picks=pending["points"].toArray();
    if(picks.size()>=3)return {{"pending",true},{"authoredChange",false}};
    const auto draft=session_["draftSketch"].toObject();
    if(!pending.contains("base"))pending={{"points",QJsonArray{}},{"base",draft},{"rho",0.5},{"text","0.5"},{"valid",false}};
    const auto plane=session_["inputs"].toObject()["plane"].toString();
    const auto frame=session_["resolvedFrame"].toObject();
    const auto coordinate=view_->sketchPoint(inputs["x"].toDouble(),inputs["y"].toDouble(),plane,frame);
    QJsonObject point{{"uMm",coordinate["uMm"]},{"vMm",coordinate["vMm"]}};
    const auto hit=view_->pickSketch(draft,inputs["x"].toDouble(),inputs["y"].toDouble(),"point",frame);
    if(hit["kind"]=="point")for(const auto& value:draft["parameters"].toObject()["points"].toArray()) {
        const auto existing=value.toObject();if(existing["id"]==hit["pointId"])
            point={{"uMm",existing["uMm"]},{"vMm",existing["vMm"]},{"pointId",existing["id"]}};
    }
    if(place) {
        picks.append(point);pending["points"]=picks;session_["pendingConic"]=pending;
        if(picks.size()==3) {showConicEditor();return conicCommand("preview",{{"text",pending["text"]}});}
    } else if(picks.size()==2) {
        const auto preview=SketchConic::add(draft,{{"start",picks[0]},{"end",picks[1]},{"vertex",point},{"rho",0.5}},session_["projectionContext"].toObject());
        if(preview.ok)view_->showSketchCurveCursor(preview.feature);
    }
    return {{"pendingPoints",picks},{"authoredChange",false}};
}
