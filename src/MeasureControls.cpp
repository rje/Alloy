// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "CadDocument.h"
#include "Measure.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QShortcut>
#include <QVBoxLayout>
#include <algorithm>
void AlloyWindow::registerMeasureCommands(){
    const QJsonObject sourceSchema{{"type","array"},{"description","One or two exact sources: kind edge/face with bodyId and edgeId/faceId and optional occurrenceId; sketchEdge with sketchId/entityId; sketchPoint with sketchId/pointId."},{"required",false}};
    commands_.push_back({"inspect.measureResult","Measure Result","Read exact current supported measurements without authored changes.",{{"sources",sourceSchema}},true,[this](const QJsonObject& args){
        return measureGeometry(document_->evaluation(),presentationBodies(),args.contains("sources")?args.value("sources").toArray():measureSources_,document_->authored().value("displayUnits").toString("mm"));
    }});
    commands_.push_back({"inspect.measureFilter","Measure Selection Filter","Choose the same Edge, Face or Point filter used by the native panel.",{{"filter",QJsonObject{{"type","string"},{"required",true},{"enum",QJsonArray{"Edge","Face","Point"}}}}},true,[this](const QJsonObject& args){
        if(!measureDialog_)return QJsonObject{{"ok",false},{"error",QJsonObject{{"code","measure_inactive"},{"message","Open Measure before choosing a filter"}}}};
        auto* filter=measureDialog_->findChild<QComboBox*>("measure.filter");filter->setCurrentText(args.value("filter").toString());return QJsonObject{{"filter",filter->currentText()}};
    }});
    commands_.push_back({"inspect.measureClear","Clear Selection","Clear only transient Measure picks.",{},true,[this](const QJsonObject&){measureSources_={};view_->clearSweepPath();view_->clearSolidEdgeHighlights();view_->selectSupportFace({},{});view_->selectFinishedPoint({});refreshMeasure();return QJsonObject{{"sources",measureSources_}};}});
    commands_.push_back({"inspect.measure","Measure","Open the native inspection panel; select an edge, face or sketch point.",{},true,[this](const QJsonObject&){
        if(!session_.isEmpty())return QJsonObject{{"ok",false},{"error",QJsonObject{{"code","active_command"},{"message","Finish the active command before measuring"}}}};
        if(measureDialog_){measureDialog_->raise();return QJsonObject{{"dialog","measure.dialog"}};}
        measureSources_={};measureDocument_=document_;auto* dialog=new QDialog(this);measureDialog_=dialog;
        dialog->setObjectName("measure.dialog");dialog->setWindowTitle("Measure");dialog->setAttribute(Qt::WA_DeleteOnClose);dialog->resize(340,320);
        auto* layout=new QVBoxLayout(dialog);layout->setSizeConstraint(QLayout::SetMinimumSize);layout->addWidget(new QLabel("Select geometry in the canvas",dialog));
        auto* filter=new QComboBox(dialog);filter->setObjectName("measure.filter");filter->addItems({"Edge","Face","Point"});layout->addWidget(filter);
        for(const auto* name:{"Selection 1","Selection 2","Results"}){layout->addWidget(new QLabel(name,dialog));auto* label=new QLabel(dialog);label->setWordWrap(true);label->setObjectName(QString("measure.")+name);layout->addWidget(label);}
        auto* clear=new QPushButton("Clear Selection",dialog);clear->setObjectName("measure.clear");layout->addWidget(clear);connect(clear,&QPushButton::clicked,this,[this]{execute("inspect.measureClear");});
        auto* buttons=new QDialogButtonBox(QDialogButtonBox::Close,dialog);layout->addWidget(buttons);connect(buttons,&QDialogButtonBox::rejected,dialog,&QDialog::close);
        auto* results=dialog->findChild<QLabel*>("measure.Results");
        results->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
        connect(results,&QLabel::linkActivated,this,[](const QString& value){QApplication::clipboard()->setText(value);});
        connect(dialog,&QDialog::finished,this,[this]{measureDialog_=nullptr;measureSources_={};measureDocument_=nullptr;view_->clearSweepPath();view_->clearSolidEdgeHighlights();view_->selectSupportFace({},{});view_->selectFinishedPoint({});
            // Closing the modeless inspector must return keyboard commands to
            // the canvas. A modeling session that closed it owns its own focus.
            if(session_.isEmpty()){activateWindow();view_->setFocus();}
        });refreshMeasure();dialog->show();dialog->move(mapToGlobal(QPoint(std::max(0,width()-dialog->width()-24),220)));view_->setFocus();return QJsonObject{{"dialog","measure.dialog"}};
    }});
    auto* shortcut=new QShortcut(QKeySequence("I"),this);connect(shortcut,&QShortcut::activated,this,[this]{if(session_.isEmpty())execute("inspect.measure");});
}
void AlloyWindow::refreshMeasure(){
    if(!measureDialog_)return;if(measureDocument_!=document_ || !session_.isEmpty()){measureDialog_->close();return;}
    for(int i=0;i<2;++i){const auto source=measureSources_.size()>i?measureSources_[i].toObject():QJsonObject{};QStringList ids;
        for(const auto* key:{"kind","bodyId","faceId","edgeId","sketchId","entityId","pointId","occurrenceId"})if(source.contains(key))ids.append(source.value(key).toString());
        QString title;
        const auto kind=source.value("kind").toString();
        if(kind=="sketchEdge" || kind=="sketchPoint"){
            title=kind=="sketchEdge" ? "Sketch Line" : "Sketch Point";
            for(const auto& sketch:document_->evaluation().sketches)if(sketch.featureId==source.value("sketchId"))title+=" — "+sketch.name;
        }else if(kind=="edge" || kind=="face"){
            title=kind=="edge" ? "Edge" : "Face";
            for(const auto& body:presentationBodies())if(body.bodyId==source.value("bodyId") && (!source.contains("occurrenceId") || body.occurrenceId==source.value("occurrenceId"))){title+=" — "+body.name;break;}
        }
        auto* label=measureDialog_->findChild<QLabel*>(QString("measure.Selection %1").arg(i+1));label->setText(title);label->setToolTip(ids.join("\n"));}
    const auto result=measureGeometry(document_->evaluation(),presentationBodies(),measureSources_,document_->authored().value("displayUnits").toString("mm"));QStringList lines;
    for(const auto& entry:result.value("measurements").toArray()){const auto m=entry.toObject();const auto text=QString::number(m.value("value").toDouble(),'f',4)+" "+m.value("unit").toString();lines.append(m.value("name").toString()+": <a href=\""+text.toHtmlEscaped()+"\">"+text.toHtmlEscaped()+"</a>");}
    if(lines.isEmpty())lines.append(result.value("error").toObject().value("message").toString());
    auto* label=measureDialog_->findChild<QLabel*>("measure.Results");label->setText(lines.join("<br>"));
    // Rich-text QLabel can keep its one-line height after a second result is
    // added. Reserve enough space for every displayed measurement before the
    // layout places the Clear and Close controls below it.
    label->setMinimumHeight(std::max(label->sizeHint().height(),label->fontMetrics().lineSpacing()*int(lines.size())+4));
    measureDialog_->layout()->activate();
}
QJsonObject AlloyWindow::pickMeasure(double x,double y){
    const auto filter=measureDialog_->findChild<QComboBox*>("measure.filter")->currentText();QJsonObject source;
    if(filter=="Face"){source=view_->pickSupportFace(x,y,presentationBodies());if(!source.isEmpty())source["kind"]="face";}
    else if(filter=="Point")source=view_->pickFinishedPoint(x,y);
    else {source=view_->pickMeasureSketchEdge(x,y);if(source.isEmpty())source=view_->pickSolidEdge(x,y,presentationBodies());if(!source.isEmpty() && !source.contains("kind"))source["kind"]="edge";}
    if(source.contains("error"))return source;
    if(source.isEmpty())return {{"ok",false},{"error",QJsonObject{{"code","measure_unavailable"},{"message","No supported geometry under the pointer"}}}};
    if(measureSources_.size()>=2)return {{"ok",false},{"error",QJsonObject{{"code","measure_selection_count"},{"message","Clear Selection before choosing more objects"}}}};
    auto candidate=measureSources_;candidate.append(source);
    const auto candidateResult=measureGeometry(document_->evaluation(),presentationBodies(),candidate,document_->authored().value("displayUnits").toString("mm"));
    if(candidateResult.value("error").toObject().value("code")=="measure_duplicate")return candidateResult;
    measureSources_=candidate;
    if(source.value("kind")=="edge")view_->highlightSolidEdges(presentationBodies(),source.value("bodyId").toString(),QJsonArray{source.value("edgeId")},source.value("occurrenceId").toString());
    else if(source.value("kind")=="face")for(const auto& body:presentationBodies())if(body.bodyId==source.value("bodyId") && (!source.contains("occurrenceId") || body.occurrenceId==source.value("occurrenceId")))for(const auto& face:body.supportFaces)if(face.faceId==source.value("faceId"))view_->selectSupportFace(source,face.face);
    if(source.value("kind")=="sketchPoint")view_->selectFinishedPoint(source);
    if(source.value("kind")=="sketchEdge")for(const auto& sketch:document_->evaluation().sketches)if(sketch.featureId==source.value("sketchId"))view_->showSweepPath(sketch,QJsonArray{source.value("entityId")});
    refreshMeasure();return measureGeometry(document_->evaluation(),presentationBodies(),measureSources_,document_->authored().value("displayUnits").toString("mm"));
}
