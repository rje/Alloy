// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "CadDocument.h"
#include "SurfaceContinuity.h"
#include "ReferenceView.h"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QSignalBlocker>
#include <QVBoxLayout>
namespace {
QString faceLabel(const EvaluatedBody& body,const EvaluatedSupportFace& face){return "Face "+face.faceId.left(8)+(body.occurrenceId.isEmpty() ? QString{}:" · "+body.occurrenceId.left(8))+" · "+body.name+" · "+face.role;}
}
void AlloyWindow::registerSurfaceContinuityCommands(){
    const QJsonObject schema{{"faces",QJsonObject{{"type","array"},{"required",false},{"description","Exactly two retained face objects with bodyId, faceId, producerFeatureId, and optional exact occurrenceId. Omit to use the active inspector choices."}}},
        {"edgeId",QJsonObject{{"type","string"},{"required",false},{"description","One explicit retained straight edge shared topologically by both faces. Omit to use the active inspector edge."}}}};
    commands_.push_back({"inspect.surfaceContinuityResult","Surface Continuity Result","Read nine shared-edge samples of current planar faces; no model or history change.",schema,true,[this](const QJsonObject& args){
        const bool explicitInputs=args.contains("faces") || args.contains("edgeId");
        const bool current=surfaceContinuityDocument_==document_ && surfaceContinuityDocumentId_==document_->authored().value("documentId").toString();
        return SurfaceContinuity::evaluate(presentationBodies(),explicitInputs ? args.value("faces").toArray() : current ? surfaceContinuityFaces_:QJsonArray{},explicitInputs ? args.value("edgeId").toString() : current ? surfaceContinuityEdgeId_:QString{},document_->authored().value("displayUnits").toString("mm"));
    }});
    commands_.push_back({"inspect.surfaceContinuity","Surface Continuity","Open a transient sampled G0 / tangent-plane G1 inspector. Choose two planar faces and their exact shared straight edge. Fixed nine samples; no saved analysis or shortcut.",schema,true,[this](const QJsonObject& args){
        if(!session_.isEmpty())return QJsonObject{{"status","unavailable"},{"error",QJsonObject{{"code","active_command"},{"message","Finish the active command before inspecting continuity"}}}};
        if(surfaceContinuityDialog_ && (surfaceContinuityDocument_!=document_ || surfaceContinuityDocumentId_!=document_->authored().value("documentId").toString()))surfaceContinuityDialog_->close();
        if(!surfaceContinuityDialog_){surfaceContinuityFaces_={};surfaceContinuityEdgeId_.clear();surfaceContinuityDocument_=document_;surfaceContinuityDocumentId_=document_->authored().value("documentId").toString();
            auto* dialog=new QDialog(this);surfaceContinuityDialog_=dialog;dialog->setObjectName("surfaceContinuity.dialog");dialog->setWindowTitle("Surface Continuity");dialog->setAttribute(Qt::WA_DeleteOnClose);dialog->resize(430,360);
            auto* layout=new QVBoxLayout(dialog);auto* explanation=new QLabel("Choose two planar faces on one body and their shared straight edge.",dialog);explanation->setWordWrap(true);layout->addWidget(explanation);
            auto* form=new QFormLayout;layout->addLayout(form);
            for(int i=0;i<2;++i){auto* combo=new QComboBox(dialog);combo->setObjectName(QString("surfaceContinuity.face%1").arg(i+1));combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);combo->setMinimumContentsLength(24);form->addRow(QString("Face %1").arg(i+1),combo);
                connect(combo,&QComboBox::activated,this,[this,combo,i]{while(surfaceContinuityFaces_.size()<2)surfaceContinuityFaces_.append(QJsonObject{});surfaceContinuityFaces_[i]=combo->currentData().toJsonObject();surfaceContinuityEdgeId_.clear();refreshSurfaceContinuity();});}
            auto* edge=new QComboBox(dialog);edge->setObjectName("surfaceContinuity.edge");edge->setMinimumContentsLength(24);edge->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);form->addRow("Shared edge",edge);
            connect(edge,&QComboBox::activated,this,[this,edge]{surfaceContinuityEdgeId_=edge->currentData().toString();refreshSurfaceContinuity();});
            auto* result=new QLabel(dialog);result->setObjectName("surfaceContinuity.results");result->setWordWrap(true);result->setTextInteractionFlags(Qt::TextSelectableByMouse);layout->addWidget(result);
            auto* limits=new QLabel("9 uniform shared-edge parameter samples, including endpoints. Results apply only at these samples. Planar faces only; no G2. Alloy thresholds: 0.001 mm / 0.1°. Normal orientation is a separate diagnostic.",dialog);limits->setWordWrap(true);layout->addWidget(limits);
            auto* buttons=new QDialogButtonBox(QDialogButtonBox::Close,dialog);layout->addWidget(buttons);connect(buttons,&QDialogButtonBox::rejected,dialog,&QDialog::close);
            connect(dialog,&QDialog::finished,this,[this]{surfaceContinuityDialog_=nullptr;surfaceContinuityFaces_={};surfaceContinuityEdgeId_.clear();surfaceContinuityDocument_=nullptr;surfaceContinuityDocumentId_.clear();if(session_.isEmpty()){activateWindow();view_->setFocus();}});
        }
        if(args.contains("faces") || args.contains("edgeId")){surfaceContinuityFaces_=args.value("faces").toArray();surfaceContinuityEdgeId_=args.value("edgeId").toString();}
        refreshSurfaceContinuity();surfaceContinuityDialog_->show();surfaceContinuityDialog_->raise();
        return SurfaceContinuity::evaluate(presentationBodies(),surfaceContinuityFaces_,surfaceContinuityEdgeId_,document_->authored().value("displayUnits").toString("mm"));
    }});
}
void AlloyWindow::refreshSurfaceContinuity(){
    if(!surfaceContinuityDialog_)return;
    if(surfaceContinuityDocument_!=document_ || surfaceContinuityDocumentId_!=document_->authored().value("documentId").toString()){surfaceContinuityDialog_->close();return;}
    const auto bodies=presentationBodies();
    for(int i=0;i<2;++i){auto* combo=surfaceContinuityDialog_->findChild<QComboBox*>(QString("surfaceContinuity.face%1").arg(i+1));const QSignalBlocker blocker(combo);combo->clear();combo->addItem("Choose a face",QJsonObject{});int selected=0;
        const auto ref=surfaceContinuityFaces_.size()>i ? surfaceContinuityFaces_[i].toObject():QJsonObject{};
        for(const auto& body:bodies)if(body.visible)for(const auto& face:body.supportFaces){QJsonObject source{{"bodyId",body.bodyId},{"faceId",face.faceId},{"producerFeatureId",face.producerFeatureId}};if(!body.occurrenceId.isEmpty())source["occurrenceId"]=body.occurrenceId;
            combo->addItem(faceLabel(body,face),source);combo->setItemData(combo->count()-1,body.name+"\nFace: "+face.faceId+"\nBody: "+body.bodyId+"\nProducer: "+face.producerFeatureId+"\nOccurrence: "+body.occurrenceId,Qt::ToolTipRole);if(source==ref)selected=combo->count()-1;}
        combo->setCurrentIndex(selected);
    }
    auto* edgeCombo=surfaceContinuityDialog_->findChild<QComboBox*>("surfaceContinuity.edge");const QSignalBlocker blocker(edgeCombo);edgeCombo->clear();edgeCombo->addItem("Choose the shared edge",QString{});int selected=0;
    if(surfaceContinuityFaces_.size()==2){const auto a=surfaceContinuityFaces_[0].toObject(),b=surfaceContinuityFaces_[1].toObject();
        if(a.value("bodyId")==b.value("bodyId") && a.value("occurrenceId")==b.value("occurrenceId"))for(const auto& body:bodies)if(body.visible && body.bodyId==a.value("bodyId").toString() && body.occurrenceId==a.value("occurrenceId").toString()){
            const EvaluatedSupportFace* first=nullptr;const EvaluatedSupportFace* second=nullptr;
            for(const auto& face:body.supportFaces){if(face.faceId==a.value("faceId") && face.producerFeatureId==a.value("producerFeatureId"))first=&face;if(face.faceId==b.value("faceId") && face.producerFeatureId==b.value("producerFeatureId"))second=&face;}
            if(first && second && first!=second)for(const auto& e:first->boundaryEdges)for(const auto& f:second->boundaryEdges)if(e.edgeId==f.edgeId && e.edge.IsSame(f.edge)){
                edgeCombo->addItem("Edge · "+e.edgeId.left(8),e.edgeId);if(e.edgeId==surfaceContinuityEdgeId_)selected=edgeCombo->count()-1;}
        }
    }
    edgeCombo->setCurrentIndex(selected);
    const auto result=SurfaceContinuity::evaluate(bodies,surfaceContinuityFaces_,surfaceContinuityEdgeId_,document_->authored().value("displayUnits").toString("mm"));QString text;
    if(result.value("status")=="ready"){const auto display=result.value("display").toObject();
        text=QString("Sampled G0: %1\nMaximum gap: %2 %3\nSampled tangent-plane G1: %4\nMaximum angle: %5°\nOpposite normal hemisphere: %6 / 9 samples")
            .arg(result.value("sampledG0Pass").toBool() ? "Pass":"Fail").arg(display.value("maxGap").toDouble(),0,'g',8).arg(display.value("lengthUnit").toString())
            .arg(result.value("sampledG1Pass").toBool() ? "Pass":"Fail").arg(result.value("maxTangentPlaneAngleDeg").toDouble(),0,'g',8).arg(result.value("normalOrientation").toObject().value("oppositeHemisphereSampleCount").toInt());
    }else text=result.value("error").toObject().value("message").toString();
    surfaceContinuityDialog_->findChild<QLabel*>("surfaceContinuity.results")->setText(text);
}
