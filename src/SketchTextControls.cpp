// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include "SketchText.h"
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QJsonArray>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QVBoxLayout>
#include <cmath>
#include <numbers>

namespace {
QJsonObject rejected(const QString& code,const QString& message){
    return {{"ok",false},{"completion","failed"},{"error",QJsonObject{{"code",code},{"message",message}}}};
}
QJsonObject withText(QJsonObject sketch,const QJsonObject& text){
    auto p=sketch["parameters"].toObject();p["texts"]=QJsonArray{text};sketch["parameters"]=p;return sketch;
}
}

QJsonObject AlloyWindow::beginSketchText(){
    if(session_.value("command")!="sketch.edit")return rejected("no_active_sketch","Enter a sketch first");
    const auto draft=session_["draftSketch"].toObject(),p=draft["parameters"].toObject();
    const auto texts=p["texts"].toArray();
    if(!p["entities"].toArray().isEmpty() || texts.size()>1)
        return rejected("text_only_sketch","Sketch Text requires an otherwise empty sketch");
    if(!session_.value("pendingText").toObject().isEmpty()){showSketchTextDialog();return {{"pendingText",session_["pendingText"]},{"previewed",true}};}
    const auto text=texts.size()==1 ? texts.first().toObject()
        : SketchText::create({{"text","D"},{"heightMm",10.},{"uMm",0.},{"vMm",0.},{"angleDeg",0.}});
    session_["pendingText"]=text;
    session_["pendingTextOriginal"]=texts.size()==1 ? texts.first().toObject() : QJsonObject{};
    showSketchTextDialog();
    return previewSketchText({});
}

QJsonObject AlloyWindow::previewSketchText(const QJsonObject& values){
    if(session_.value("command")!="sketch.edit" || session_.value("pendingText").toObject().isEmpty())
        return rejected("text_dialog_inactive","Open Sketch Text controls first");
    if(session_.value("baseDocumentRevision").toInteger()!=static_cast<qint64>(document_->revision()))
        return rejected("stale_session","The document changed; cancel this text edit");
    auto text=session_["pendingText"].toObject();
    for(const auto& key:{"text","fontId","heightMm","uMm","vMm","angleDeg"})if(values.contains(key))text[key]=values[key];
    if(values.contains("fontId")){
        const auto id=text["fontId"].toString();
        if(id!=SketchText::fontId() && id!="dejavu-sans-2.37-bold")return rejected("unsupported_font","Choose a bundled text font");
        text["fontSha256"]=SketchText::fontSha256(id);text["style"]=id.endsWith("-bold") ? "bold" : "regular";
    }
    session_["pendingText"]=text;
    const auto original=session_["pendingTextOriginal"].toObject();
    QString error;
    if(!original.isEmpty() && text["text"].toString().size()!=original["text"].toString().size())
        error="Editing retained text requires the same number of characters so existing profile references remain stable";
    QSet<QString> ids;
    if(error.isEmpty())error=SketchText::validate(text,ids);
    auto draft=withText(session_["draftSketch"].toObject(),text);
    if(error.isEmpty())try {
        const auto solved=SketchGeometry::solveDraft(draft,session_["projectionContext"].toObject());
        if(!solved.ok)error=solved.error;
        else {
            const auto evaluated=SketchGeometry::evaluateFeature(solved.feature,session_["resolvedFrame"].toObject(),session_["projectionContext"].toObject());
            session_["pendingTextDraft"]=solved.feature;session_.remove("pendingTextError");
            view_->showSketchDraft(evaluated);
            displayedSketchDraft_=session_["draftSketch"].toObject();
            const auto p=session_["draftSketch"].toObject()["parameters"].toObject();
            view_->showSketchTextHandles(text,p["plane"].toString(),session_["resolvedFrame"].toObject());
            syncSketchTextDialog();
            return {{"authoredChange",false},{"previewed",true},{"pendingText",text},{"sketchGeometry",evaluated.summary}};
        }
    }catch(const std::exception& e){error=QString::fromUtf8(e.what());}
    session_.remove("pendingTextDraft");session_["pendingTextError"]=error;
    view_->clearSketchTextHandles();
    const auto base=session_["draftSketch"].toObject();
    view_->showSketchDraft(SketchGeometry::evaluateFeature(base,session_["resolvedFrame"].toObject(),session_["projectionContext"].toObject()));
    displayedSketchDraft_=base;syncSketchTextDialog();
    return rejected("invalid_text_preview",error);
}

QJsonObject AlloyWindow::moveSketchTextHandle(const QJsonObject& values){
    if(session_.value("pendingTextDraft").toObject().isEmpty())return rejected("invalid_text_preview","Enter valid text first");
    const auto mode=values["mode"].toString(),phase=values["phase"].toString();
    if((mode!="move" && mode!="rotate") || (phase!="begin" && phase!="move" && phase!="end"))
        return rejected("invalid_handle","Choose a move or rotation handle and a pointer phase");
    auto drag=session_["pendingTextDrag"].toObject();
    const auto p=session_["draftSketch"].toObject()["parameters"].toObject();
    const auto plane=p["plane"].toString();const auto frame=session_["resolvedFrame"].toObject();
    if(phase=="begin"){
        drag={{"mode",mode},{"startX",values["startX"]},{"startY",values["startY"]},{"initial",session_["pendingText"]}};
        session_["pendingTextDrag"]=drag;return {{"dragging",true}};
    }
    if(drag.isEmpty() || drag["mode"]!=mode)return rejected("handle_inactive","Begin dragging a text handle first");
    const auto a=view_->sketchPoint(drag["startX"].toDouble(),drag["startY"].toDouble(),plane,frame);
    const auto b=view_->sketchPoint(values["x"].toDouble(),values["y"].toDouble(),plane,frame);
    const auto initial=drag["initial"].toObject();
    QJsonObject change;
    if(mode=="move"){
        change["uMm"]=initial["uMm"].toDouble()+b["uMm"].toDouble()-a["uMm"].toDouble();
        change["vMm"]=initial["vMm"].toDouble()+b["vMm"].toDouble()-a["vMm"].toDouble();
    }else{
        const double x=initial["uMm"].toDouble(),y=initial["vMm"].toDouble();
        const double before=std::atan2(a["vMm"].toDouble()-y,a["uMm"].toDouble()-x);
        const double after=std::atan2(b["vMm"].toDouble()-y,b["uMm"].toDouble()-x);
        change["angleDeg"]=initial["angleDeg"].toDouble()+std::remainder(after-before,2*std::numbers::pi)*180/std::numbers::pi;
    }
    const auto result=previewSketchText(change);
    if(phase=="end")session_.remove("pendingTextDrag");
    return result;
}

QJsonObject AlloyWindow::acceptSketchText(){
    if(session_.value("pendingTextDraft").toObject().isEmpty())return rejected("invalid_text_preview","Resolve the text preview error before accepting");
    const auto text=session_["pendingText"].toObject();
    const auto result=publishSketchSolve(SketchGeometry::solveDraft(session_["pendingTextDraft"].toObject(),session_["projectionContext"].toObject()));
    if(result.value("ok").isBool() && !result["ok"].toBool())return result;
    session_.remove("pendingText");session_.remove("pendingTextOriginal");session_.remove("pendingTextDraft");session_.remove("pendingTextError");session_.remove("pendingTextDrag");
    view_->clearSketchTextHandles();if(sketchTextDialog_)sketchTextDialog_->hide();
    auto answer=result;answer["textId"]=text["id"];return answer;
}

QJsonObject AlloyWindow::cancelSketchText(){
    if(session_.value("pendingText").toObject().isEmpty())return rejected("text_dialog_inactive","No text edit is active");
    session_.remove("pendingText");session_.remove("pendingTextOriginal");session_.remove("pendingTextDraft");session_.remove("pendingTextError");session_.remove("pendingTextDrag");
    view_->clearSketchTextHandles();
    const auto base=session_["draftSketch"].toObject();
    view_->showSketchDraft(SketchGeometry::evaluateFeature(base,session_["resolvedFrame"].toObject(),session_["projectionContext"].toObject()));
    displayedSketchDraft_=base;if(sketchTextDialog_)sketchTextDialog_->hide();
    return {{"canceled",true},{"authoredChange",false}};
}

void AlloyWindow::showSketchTextDialog(){
    if(!sketchTextDialog_){
        auto* dialog=new QDialog(this);sketchTextDialog_=dialog;dialog->setObjectName("sketch.text.dialog");
        dialog->setWindowTitle("Sketch Text");dialog->setWindowModality(Qt::NonModal);
        auto* outer=new QVBoxLayout(dialog);auto* form=new QFormLayout;outer->addLayout(form);
        auto* content=new QLineEdit(dialog);content->setObjectName("sketch.text.content");content->setMaxLength(16);form->addRow("Text",content);
        auto* font=new QComboBox(dialog);font->setObjectName("sketch.text.font");
        for(const auto& entry:SketchText::fonts()){const auto item=entry.toObject();font->addItem(item["label"].toString(),item["id"].toString());}
        form->addRow("Font",font);
        auto number=[&](const char* name,const QString& label,double lo,double hi,int decimals,const QString& suffix){
            auto* field=new QDoubleSpinBox(dialog);field->setObjectName(name);field->setRange(lo,hi);field->setDecimals(decimals);field->setSuffix(suffix);form->addRow(label,field);return field;
        };
        auto* height=number("sketch.text.height","Height",.1,1000,2," mm");
        auto* x=number("sketch.text.x","X",-100000,100000,2," mm");
        auto* y=number("sketch.text.y","Y",-100000,100000,2," mm");
        auto* angle=number("sketch.text.angle","Angle",-360000,360000,1,"°");
        auto* hint=new QLabel("Drag the center handle to move; drag the outer handle to rotate.",dialog);hint->setWordWrap(true);outer->addWidget(hint);
        auto* error=new QLabel(dialog);error->setObjectName("sketch.text.error");error->setWordWrap(true);outer->addWidget(error);
        auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,dialog);outer->addWidget(buttons);
        connect(content,&QLineEdit::textChanged,this,[this](const QString& value){execute("sketch.textPreview",{{"text",value}});});
        connect(font,&QComboBox::currentIndexChanged,this,[this,font](int){execute("sketch.textPreview",{{"fontId",font->currentData().toString()}});});
        connect(height,&QDoubleSpinBox::valueChanged,this,[this](double value){execute("sketch.textPreview",{{"heightMm",value}});});
        connect(x,&QDoubleSpinBox::valueChanged,this,[this](double value){execute("sketch.textPreview",{{"uMm",value}});});
        connect(y,&QDoubleSpinBox::valueChanged,this,[this](double value){execute("sketch.textPreview",{{"vMm",value}});});
        connect(angle,&QDoubleSpinBox::valueChanged,this,[this](double value){execute("sketch.textPreview",{{"angleDeg",value}});});
        connect(buttons,&QDialogButtonBox::accepted,this,[this]{execute("sketch.textAccept");});
        connect(buttons,&QDialogButtonBox::rejected,this,[this]{execute("sketch.textCancel");});
        connect(dialog,&QDialog::rejected,this,[this]{if(!session_.value("pendingText").toObject().isEmpty())execute("sketch.textCancel");});
        dialog->resize(330,260);
        dialog->move(mapToGlobal(QPoint(browser_->width()+12,180)));
    }
    syncSketchTextDialog();sketchTextDialog_->show();sketchTextDialog_->raise();
    sketchTextDialog_->activateWindow();
    QApplication::setActiveWindow(sketchTextDialog_);
    sketchTextDialog_->findChild<QLineEdit*>("sketch.text.content")->setFocus(Qt::OtherFocusReason);
}

void AlloyWindow::syncSketchTextDialog(){
    if(!sketchTextDialog_)return;
    const auto text=session_["pendingText"].toObject();if(text.isEmpty())return;
    auto* content=sketchTextDialog_->findChild<QLineEdit*>("sketch.text.content");
    auto* font=sketchTextDialog_->findChild<QComboBox*>("sketch.text.font");
    const QSignalBlocker a(content),b(font);
    content->setText(text["text"].toString());font->setCurrentIndex(font->findData(text["fontId"].toString()));
    for(const auto pair:{std::pair{"sketch.text.height","heightMm"},std::pair{"sketch.text.x","uMm"},
        std::pair{"sketch.text.y","vMm"},std::pair{"sketch.text.angle","angleDeg"}}){
        auto* field=sketchTextDialog_->findChild<QDoubleSpinBox*>(pair.first);
        const QSignalBlocker block(field);field->setValue(text[pair.second].toDouble());
    }
    const auto error=session_["pendingTextError"].toString();
    auto* label=sketchTextDialog_->findChild<QLabel*>("sketch.text.error");
    label->setText(error);label->setVisible(!error.isEmpty());
    sketchTextDialog_->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->setEnabled(error.isEmpty() && !session_["pendingTextDraft"].toObject().isEmpty());
}
