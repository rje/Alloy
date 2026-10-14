// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "LocalReference.h"
#include "ReferenceView.h"
#include "StepExchange.h"
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QCryptographicHash>
#include <QFile>
#include <limits>

namespace {
QJsonObject field(const QString& type,bool required=true){QJsonObject result{{"type",type},{"required",required}};
    if(type=="number"){result["minimum"]=-100000;result["maximum"]=100000;}return result;}
QJsonObject error(const QString& code,const QString& message){return {{"ok",false},{"completion","failed"},{"error",QJsonObject{{"code",code},{"message",message}}}};}
QJsonObject reference(const QJsonObject& authored,const QString& id){for(const auto& value:authored["features"].toArray()){
    const auto feature=value.toObject();if(feature["id"]==id && feature["type"]=="stepAssemblyImport")return feature["parameters"].toObject()["reference"].toObject();}return {};}
}
void AlloyWindow::registerLocalReferenceCommands(){
    commands_.push_back({"reference.insert","Insert Local Reference","Insert one embedded local STEP assembly with a retained source locator and separate outer placement.",
        {{"path",field("string")},{"name",field("string",false)},{"placement",field("object",false)}},true,[this](const QJsonObject& a){
        if(!session_.isEmpty())return error("session_busy","Finish or Cancel the active command first");
        try{const auto candidate=CadDocument::localReferenceCandidate(document_->authored(),a);
            const auto feature=candidate["features"].toArray().last().toObject();const auto owner=feature["ownerId"].toString();
            if(const auto problem=document_->commit(candidate,"Insert Local Reference");!problem.isEmpty())return error("evaluation_failed",problem);
            view_->fit(presentationBodies(),document_->sketches());
            return QJsonObject{{"featureId",feature["id"]},{"definitionId",owner},{"outerOccurrenceId",LocalReference::outerOccurrenceId(feature["id"].toString())},
                {"assembly",feature["parameters"].toObject()["assembly"]}};
        }catch(const StepExchange::Error& e){auto result=error(e.diagnostics()["code"].toString(),e.diagnostics()["message"].toString());result["error"]=e.diagnostics();return result;}
         catch(const std::exception& e){return error("invalid_reference",QString::fromUtf8(e.what()));}
    }});
    commands_.push_back({"reference.insertDialog","Insert Local Reference","Select a local STEP assembly and insert it as a component.",{},true,[this](const QJsonObject&){
        if(!session_.isEmpty())return error("session_busy","Finish or Cancel the active command first");
        const auto path=QFileDialog::getOpenFileName(this,"Insert Local Reference",{},"STEP files (*.step *.stp *.ste)");
        if(path.isEmpty())return QJsonObject{{"cancelled",true}};
        return execute("reference.insert",{{"path",path}});
    }});
    commands_.push_back({"reference.inspect","Inspect Local References","Read the embedded authority, outer placement and local source status.",{},true,[this](const QJsonObject&){
        QJsonArray rows;for(const auto& value:document_->authored()["features"].toArray()){
            const auto feature=value.toObject(),p=feature["parameters"].toObject(),r=p["reference"].toObject();if(r.isEmpty())continue;
            const auto path=r["path"].toString();QString status="missing";
            QFile file(path);if(file.open(QIODevice::ReadOnly)){
                const auto bytes=file.read(StepExchange::maximumSourceBytes+1);
                status=bytes.size()>StepExchange::maximumSourceBytes ? "changed" :
                    QString::fromLatin1(QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex())==r["lastSourceSha256"] ? "current" : "changed";
            }else if(QFileInfo::exists(path))status="unreadable";
            rows.append(QJsonObject{{"featureId",feature["id"]},{"definitionId",feature["ownerId"]},{"outerOccurrenceId",LocalReference::outerOccurrenceId(feature["id"].toString())},
                {"path",path},{"sourceStatus",status},{"outerPlacement",r["outerPlacement"]},{"pending",pendingReference_.value("featureId")==feature["id"]}});
        }
        return QJsonObject{{"references",rows}};
    }});
    commands_.push_back({"reference.update","Update Local Reference","Check the local source explicitly; byte-identical reload retains every imported identity.",
        {{"featureId",field("string")}},true,[this](const QJsonObject& a){
        if(!session_.isEmpty())return error("session_busy","Finish or Cancel the active command first");
        const auto r=reference(document_->authored(),a["featureId"].toString());if(r.isEmpty())return error("stale_entity","Local reference is unavailable");
        const auto path=r["path"].toString();QFile file(path);
        if(!file.open(QIODevice::ReadOnly))return QFileInfo::exists(path)
            ? error("source_unreadable","Local reference source cannot be read; embedded geometry remains available")
            : error("source_missing","Local reference source is missing; embedded geometry remains available");
        const auto bytes=file.read(StepExchange::maximumSourceBytes+1);if(bytes.size()>StepExchange::maximumSourceBytes)return error("source_size","Local source exceeds 32 MiB");
        if(QString::fromLatin1(QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex())!=r["lastSourceSha256"])
            return error("source_changed","Changed STEP geometry needs verified subshape correspondence before Update");
        return QJsonObject{{"featureId",a["featureId"]},{"status","current"},{"updated",false}};
    }});
    commands_.push_back({"reference.move","Move Local Reference","Preview an outer rigid placement; Capture Position persists it, Revert discards it.",
        {{"featureId",field("string")},{"placement",field("object")}},true,[this](const QJsonObject& a){
        if(!session_.isEmpty())return error("session_busy","Finish or Cancel the active command first");
        const auto id=a["featureId"].toString();if(reference(document_->authored(),id).isEmpty())return error("stale_entity","Local reference is unavailable");
        if(!pendingReference_.isEmpty() && pendingReference_["featureId"]!=id)return error("pending_position","Capture or Revert the other pending placement first");
        try{const auto candidate=CadDocument::localReferencePlacementCandidate(document_->authored(),id,a["placement"].toObject());
            const auto prepared=document_->prepareCandidate(candidate);if(!prepared.ok)return error("evaluation_failed",prepared.error);
            pendingReference_={{"featureId",id},{"placement",a["placement"]},{"baseRevision",static_cast<qint64>(document_->revision())}};
            pendingReferenceEvaluation_=prepared.evaluation;displayedDocumentRevision_=std::numeric_limits<quint64>::max();
            return QJsonObject{{"featureId",id},{"placement",a["placement"]},{"pending",true}};
        }catch(const std::exception& e){return error("invalid_placement",QString::fromUtf8(e.what()));}
    }});
    commands_.push_back({"reference.capture","Capture Position","Commit and undo the pending outer placement.",{},true,[this](const QJsonObject&){
        if(pendingReference_.isEmpty())return error("no_pending_position","No pending reference placement");
        if(pendingReference_["baseRevision"].toInteger()!=static_cast<qint64>(document_->revision()))return error("stale_position","Document changed; Revert the pending placement");
        const auto candidate=CadDocument::localReferencePlacementCandidate(document_->authored(),pendingReference_["featureId"].toString(),pendingReference_["placement"].toObject());
        if(const auto problem=document_->commit(candidate,"Capture Reference Position");!problem.isEmpty())return error("evaluation_failed",problem);
        pendingReference_={};pendingReferenceEvaluation_.reset();displayedDocumentRevision_=std::numeric_limits<quint64>::max();return document_->state();
    }});
    commands_.push_back({"reference.revert","Revert Position","Discard the pending placement and restore the captured position.",{},true,[this](const QJsonObject&){
        pendingReference_={};pendingReferenceEvaluation_.reset();displayedDocumentRevision_=std::numeric_limits<quint64>::max();return QJsonObject{{"pending",false}};
    }});
    commands_.push_back({"reference.moveDialog","Move Local Reference","Edit the outer placement with native preview and Cancel.",
        {{"featureId",field("string")}},true,[this](const QJsonObject& a){return showReferenceMove(a["featureId"].toString());}});
    commands_.push_back({"component.opacitySet","Component Opacity","Set local component presentation opacity (0 to 1).",
        {{"definitionId",field("string")},{"opacity",field("number")}},true,[this](const QJsonObject& a){
        if(!session_.isEmpty())return error("session_busy","Finish or Cancel the active command first");
        try{const auto candidate=CadDocument::componentOpacityCandidate(document_->authored(),a["definitionId"].toString(),a["opacity"].toDouble());
            if(const auto problem=document_->commit(candidate,"Component Opacity");!problem.isEmpty())return error("evaluation_failed",problem);
            return document_->state();}catch(const std::exception& e){return error("invalid_opacity",QString::fromUtf8(e.what()));}
    }});
}

QJsonObject AlloyWindow::showReferenceMove(const QString& featureId){
    const auto r=reference(document_->authored(),featureId);if(r.isEmpty())return error("stale_entity","Local reference is unavailable");
    if(!pendingReference_.isEmpty())return error("pending_position","Capture or Revert the current placement first");
    auto* dialog=new QDialog(this);dialog->setAttribute(Qt::WA_DeleteOnClose);dialog->setWindowTitle("Move/Copy");dialog->setObjectName("reference.move.dialog");
    auto* layout=new QVBoxLayout(dialog);auto* form=new QFormLayout;layout->addLayout(form);
    auto* axis=new QComboBox;axis->setObjectName("reference.move.axis");axis->addItems({"X","Y","Z"});
    const auto old=r["outerPlacement"].toObject();axis->setCurrentText(old["axis"].toString());form->addRow("Rotation Axis",axis);
    auto spin=[&](const QString& label,const QString& object,double value){auto* control=new QDoubleSpinBox;control->setObjectName(object);control->setRange(-100000,100000);control->setDecimals(3);control->setValue(value);form->addRow(label,control);return control;};
    auto* angle=spin("Angle (deg)","reference.move.angle",old["angleDeg"].toDouble());
    const auto t=old["translationMm"].toArray();auto* x=spin("X (mm)","reference.move.x",t[0].toDouble());
    auto* y=spin("Y (mm)","reference.move.y",t[1].toDouble());auto* z=spin("Z (mm)","reference.move.z",t[2].toDouble());
    auto* status=new QLabel("Move previews a pending position. Capture Position accepts it.");status->setWordWrap(true);layout->addWidget(status);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);buttons->button(QDialogButtonBox::Ok)->setText("Move");layout->addWidget(buttons);
    const auto preview=[=,this]{const QJsonObject placement{{"axis",axis->currentText()},{"angleDeg",angle->value()},{"translationMm",QJsonArray{x->value(),y->value(),z->value()}}};
        const auto result=execute("reference.move",{{"featureId",featureId},{"placement",placement}});
        status->setText(result["ok"].toBool() ? "Pending position. Capture Position to retain it, or Revert." : result["error"].toObject()["message"].toString());};
    connect(axis,&QComboBox::currentTextChanged,dialog,[=](const QString&){preview();});
    for(auto* control:{angle,x,y,z})connect(control,qOverload<double>(&QDoubleSpinBox::valueChanged),dialog,[=](double){preview();});
    connect(buttons,&QDialogButtonBox::accepted,dialog,[=,this]{preview();if(!pendingReference_.isEmpty())dialog->accept();});
    connect(buttons,&QDialogButtonBox::rejected,dialog,&QDialog::reject);
    connect(dialog,&QDialog::rejected,dialog,[this]{execute("reference.revert");});
    dialog->resize(340,280);dialog->show();return {{"visible",true},{"featureId",featureId}};
}
