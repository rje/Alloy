// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "Interference.h"
#include "ReferenceView.h"
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
namespace {
QJsonObject refusal(const QString& code,const QString& message){return {{"ok",false},{"error",QJsonObject{{"code",code},{"message",message}}}};}
}
QJsonObject AlloyWindow::interferenceResult(const QJsonArray& sources,bool feedback){
    auto result=Interference::evaluate(presentationBodies(),sources,document_->authored().value("displayUnits").toString("mm"));
    for(const auto* key:{"first","second"})if(result.json.contains(key)){
        auto identity=result.json.value(key).toObject();QString name="Root";
        for(const auto& entry:document_->authored().value("components").toArray())if(entry.toObject().value("id")==identity.value("componentId"))name=entry.toObject().value("name").toString();
        identity["componentName"]=name;result.json[key]=identity;
    }
    if(feedback)view_->setInterferenceFeedback(result.sources,result.feedback);
    return result.json;
}
void AlloyWindow::registerInterferenceCommands(){
    const QJsonObject reference{{"type","object"},{"required",false},{"description","One committed visible closed solid: bodyId and optional exact occurrenceId."}};
    commands_.push_back({"inspect.interferenceResult","Compute Interference","Inspect exactly two current committed solids without authored changes.",{{"first",reference},{"second",reference},{"sources",QJsonObject{{"type","array"},{"required",false}}}},true,[this](const QJsonObject& args){
        if(!session_.isEmpty())return refusal("active_command","Finish the active modeling command before inspecting committed solids");
        QJsonArray sources=interferenceSources_;
        if(args.contains("sources"))sources=args.value("sources").toArray();
        else if(args.contains("first") || args.contains("second"))sources={args.value("first"),args.value("second")};
        const auto result=interferenceResult(sources,bool(interferenceDialog_));
        if(interferenceDialog_){interferenceSources_=sources;interferenceComputed_=result;interferenceRevision_=document_->revision();refreshInterference();}
        return result;
    }});
    commands_.push_back({"inspect.interferenceClear","Clear Interference Selection","Clear transient picks and feedback.",{},true,[this](const QJsonObject&){interferenceSources_={};interferenceComputed_={};view_->setInterferenceFeedback({},{});refreshInterference();return QJsonObject{{"sources",interferenceSources_}};}});
    commands_.push_back({"inspect.interferenceState","Interference Selection","Read transient native pair and computed result.",{},true,[this](const QJsonObject&){return QJsonObject{{"active",bool(interferenceDialog_)},{"sources",interferenceSources_},{"result",interferenceComputed_}};}});
    commands_.push_back({"inspect.interference","Interference","Open a transient two-solid Interference inspection.",{},true,[this](const QJsonObject&){
        if(!session_.isEmpty())return refusal("active_command","Finish the active modeling command before inspecting solids");
        if(interferenceDialog_){interferenceDialog_->raise();return QJsonObject{{"dialog","interference.dialog"}};}
        if(measureDialog_)measureDialog_->close();
        interferenceSources_={};interferenceComputed_={};interferenceDocument_=document_;interferenceRevision_=document_->revision();
        auto* dialog=new QDialog(this);interferenceDialog_=dialog;dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->setObjectName("interference.dialog");dialog->setWindowTitle("Interference");dialog->resize(350,310);
        auto* layout=new QVBoxLayout(dialog);layout->setSizeConstraint(QLayout::SetMinimumSize);
        auto* instructions=new QLabel("Select two solid bodies in the canvas or Browser, then Compute.",dialog);instructions->setWordWrap(true);layout->addWidget(instructions);
        for(int i=0;i<2;++i){auto* label=new QLabel(dialog);label->setWordWrap(true);label->setObjectName(QString("interference.selection%1").arg(i+1));layout->addWidget(label);}
        auto* result=new QLabel(dialog);result->setObjectName("interference.result");result->setWordWrap(true);result->setTextInteractionFlags(Qt::TextSelectableByMouse);layout->addWidget(result);
        auto* compute=new QPushButton("Compute",dialog);compute->setObjectName("interference.compute");layout->addWidget(compute);connect(compute,&QPushButton::clicked,this,[this]{execute("inspect.interferenceResult");});
        auto* clear=new QPushButton("Clear Selection",dialog);clear->setObjectName("interference.clear");layout->addWidget(clear);connect(clear,&QPushButton::clicked,this,[this]{execute("inspect.interferenceClear");});
        auto* buttons=new QDialogButtonBox(QDialogButtonBox::Close,dialog);layout->addWidget(buttons);connect(buttons,&QDialogButtonBox::rejected,dialog,&QDialog::close);
        connect(dialog,&QDialog::finished,this,[this]{interferenceDialog_=nullptr;interferenceDocument_=nullptr;interferenceSources_={};interferenceComputed_={};view_->setInterferenceFeedback({},{});if(session_.isEmpty()){activateWindow();view_->setFocus();}});
        refreshInterference();dialog->show();dialog->move(mapToGlobal(QPoint(std::max(0,width()-dialog->width()-24),220)));view_->setFocus();
        return QJsonObject{{"dialog","interference.dialog"}};
    }});
}
QJsonObject AlloyWindow::selectInterference(const QJsonObject& source){
    const auto resolved=Interference::resolve(presentationBodies(),source);if(resolved.contains("error"))return resolved;
    if(interferenceSources_.size()>=2)return refusal("interference_selection_count","Clear Selection before choosing another pair");
    for(const auto& entry:interferenceSources_)if(entry.toObject().value("bodyId")==resolved.value("bodyId") && entry.toObject().value("occurrenceId")==resolved.value("occurrenceId"))return refusal("interference_duplicate","Choose a different body or occurrence for the second selection");
    QJsonObject reference{{"bodyId",resolved.value("bodyId")}};if(resolved.contains("occurrenceId"))reference["occurrenceId"]=resolved.value("occurrenceId");
    interferenceSources_.append(reference);interferenceComputed_={};
    std::vector<TopoDS_Shape> shapes;for(const auto& ref:interferenceSources_)for(const auto& body:presentationBodies())if(body.bodyId==ref.toObject().value("bodyId") && body.occurrenceId==ref.toObject().value("occurrenceId").toString())shapes.push_back(body.shape);
    view_->setInterferenceFeedback(shapes,{});refreshInterference();return {{"sources",interferenceSources_}};
}
void AlloyWindow::refreshInterference(){
    if(!interferenceDialog_)return;
    if(interferenceDocument_!=document_ || !session_.isEmpty()){interferenceDialog_->close();return;}
    if(interferenceRevision_!=document_->revision()){interferenceComputed_={};interferenceRevision_=document_->revision();view_->setInterferenceFeedback({},{});}
    for(int i=0;i<2;++i){QString text=QString("Selection %1: choose a solid").arg(i+1);
        if(i<interferenceSources_.size()){
            const auto identity=Interference::resolve(presentationBodies(),interferenceSources_[i].toObject());
            if(identity.contains("error"))text=identity.value("error").toObject().value("message").toString();
            else {text=QString("Selection %1: %2").arg(i+1).arg(identity.value("bodyName").toString());
                QString componentName="Root";for(const auto& entry:document_->authored().value("components").toArray())if(entry.toObject().value("id")==identity.value("componentId"))componentName=entry.toObject().value("name").toString();
                text+="\nComponent: "+componentName;
                if(identity.contains("occurrenceId")){QString occurrenceName=identity.value("occurrenceId").toString().left(8);for(const auto& entry:document_->authored().value("occurrences").toArray())if(entry.toObject().value("id")==identity.value("occurrenceId") && !entry.toObject().value("name").toString().isEmpty())occurrenceName=entry.toObject().value("name").toString();text+="\nOccurrence: "+occurrenceName;}
                interferenceDialog_->findChild<QLabel*>(QString("interference.selection%1").arg(i+1))->setToolTip(QString("Body: %1\nComponent: %2\nOccurrence: %3").arg(identity.value("bodyId").toString(),identity.value("componentId").toString(),identity.value("occurrenceId").toString("Root")));}
        }
        interferenceDialog_->findChild<QLabel*>(QString("interference.selection%1").arg(i+1))->setText(text);
    }
    QString text="Compute to inspect the current pair.";
    if(interferenceComputed_.contains("error"))text=interferenceComputed_.value("error").toObject().value("message").toString();
    else if(!interferenceComputed_.isEmpty()){
        const auto display=interferenceComputed_.value("display").toObject();const auto kind=interferenceComputed_.value("kind").toString();
        text=kind=="overlap"?QString("Interference volume: %1 %2").arg(display.value("volume").toDouble(),0,'g',8).arg(display.value("volumeUnit").toString())
            :kind=="touching"?QString("Touching — clearance: 0 %1").arg(display.value("lengthUnit").toString())
            :QString("Minimum clearance: %1 %2").arg(display.value("length").toDouble(),0,'g',8).arg(display.value("lengthUnit").toString());
    }
    interferenceDialog_->findChild<QLabel*>("interference.result")->setText(text);
}
