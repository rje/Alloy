#include "AlloyWindow.h"
#include "LocalOccurrences.h"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QLabel>
#include <QVBoxLayout>

QJsonObject AlloyWindow::showStepExport(const QString& bodyId) {
    auto error=[](const QString& code,const QString& message){return QJsonObject{{"ok",false},{"error",QJsonObject{{"code",code},{"message",message}}}};};
    if(!session_.isEmpty())return error("active_command","Finish or Cancel the active command before exporting");
    if(document_->bodies().empty())return error("solid_required","Create or import a solid first");
    if(auto* existing=findChild<QDialog*>("step.export.dialog")){existing->raise();existing->activateWindow();return {{"visible",true}};}
    auto* dialog=new QDialog(this);dialog->setObjectName("step.export.dialog");dialog->setWindowTitle("Export STEP");
    dialog->setAttribute(Qt::WA_DeleteOnClose);dialog->setWindowModality(Qt::WindowModal);
    auto* layout=new QVBoxLayout(dialog);auto* form=new QFormLayout;layout->addLayout(form);
    auto* bodies=new QComboBox;bodies->setObjectName("step.export.body");
    const auto instances=LocalOccurrences::inspect(document_->authored(),document_->evaluation())["occurrences"].toArray();
    for(const auto& body:presentationBodies()){
        QString label=body.name;for(const auto& value:instances)if(value.toObject()["occurrenceId"]==body.occurrenceId){label=value.toObject()["path"].toString()+" / "+body.name;break;}
        if(!body.visible)label+=" (hidden)";
        bodies->addItem(label,QJsonObject{{"bodyId",body.bodyId},{"occurrenceId",body.occurrenceId}});
    }
    for(const auto& value:instances){const auto row=value.toObject();
        bodies->addItem("Component: "+row["path"].toString()+" (visible bodies)",QJsonObject{{"occurrenceId",row["occurrenceId"]}});
    }
    bodies->addItem("All visible bodies",QJsonObject{{"allVisible",true}});
    for(int i=0;i<bodies->count();++i)if(bodies->itemData(i).toJsonObject()["bodyId"]==bodyId){bodies->setCurrentIndex(i);break;}
    if(bodyId.isEmpty() && document_->bodies().size()>1)bodies->setCurrentIndex(bodies->count()-1);
    form->addRow("Bodies / Component",bodies);
    auto* units=new QComboBox;units->setObjectName("step.export.units");
    units->addItem("Millimeters (mm)","mm");units->addItem("Inches (in)","in");form->addRow("Unit Type",units);
    auto* note=new QLabel("Visible component and whole-design exports include each placed solid instance. Output units are stored in STEP.");
    note->setWordWrap(true);layout->addWidget(note);
    auto* status=new QLabel;status->setObjectName("step.export.status");status->setWordWrap(true);layout->addWidget(status);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Save|QDialogButtonBox::Cancel);layout->addWidget(buttons);
    connect(buttons,&QDialogButtonBox::rejected,dialog,&QDialog::reject);
    const auto tabId=activeTabId_,documentId=document_->authored().value("documentId").toString();
    const auto revision=document_->revision();
    auto current=[=,this]{return activeTabId_==tabId && document_->authored().value("documentId").toString()==documentId && document_->revision()==revision && session_.isEmpty();};
    connect(buttons,&QDialogButtonBox::accepted,dialog,[=,this]{
        if(!current()){status->setText("The design changed. Close this window and restart Export STEP.");return;}
        QString path=QFileDialog::getSaveFileName(dialog,"Export STEP",bodies->currentText()+".step","STEP solid (*.step *.stp)");
        if(path.isEmpty())return;
        if(!current()){status->setText("The design changed. Close this window and restart Export STEP.");return;}
        if(!path.endsWith(".step",Qt::CaseInsensitive) && !path.endsWith(".stp",Qt::CaseInsensitive))path+=".step";
        auto args=bodies->currentData().toJsonObject();args["path"]=QFileInfo(path).absoluteFilePath();args["units"]=units->currentData().toString();
        const auto result=execute("document.exportStep",args);
        if(!result["ok"].toBool()){status->setText(result["error"].toObject()["message"].toString());return;}
        dialog->accept();
    });
    dialog->resize(430,210);dialog->show();
    auto state=bodies->currentData().toJsonObject();state["visible"]=true;state["units"]=units->currentData().toString();return state;
}
