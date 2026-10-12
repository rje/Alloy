#include "AlloyWindow.h"
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
    for(const auto& body:document_->bodies())bodies->addItem(body.name+(body.visible ? "" : " (hidden)"),body.bodyId);
    if(const int index=bodies->findData(bodyId);index>=0)bodies->setCurrentIndex(index);
    form->addRow("Body",bodies);
    auto* units=new QComboBox;units->setObjectName("step.export.units");
    units->addItem("Millimeters (mm)","mm");units->addItem("Inches (in)","in");form->addRow("Unit Type",units);
    auto* note=new QLabel("Export one committed solid. Output units are stored in the STEP file.");
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
        const auto result=execute("document.exportStep",{{"path",QFileInfo(path).absoluteFilePath()},
            {"bodyId",bodies->currentData().toString()},{"units",units->currentData().toString()}});
        if(!result["ok"].toBool()){status->setText(result["error"].toObject()["message"].toString());return;}
        dialog->accept();
    });
    dialog->resize(430,210);dialog->show();
    return {{"visible",true},{"bodyId",bodies->currentData().toString()},{"units",units->currentData().toString()}};
}
