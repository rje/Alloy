#include "AlloyWindow.h"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

QJsonObject AlloyWindow::showMeshExport(const QString& bodyId) {
    auto error=[](const QString& code,const QString& message){return QJsonObject{{"ok",false},{"error",QJsonObject{{"code",code},{"message",message}}}};};
    if(!session_.isEmpty())return error("active_command","Finish or Cancel the active command before exporting");
    if(document_->bodies().empty())return error("solid_required","Create or import a solid first");
    if(auto* existing=findChild<QDialog*>("mesh.export.dialog")){existing->raise();existing->activateWindow();return QJsonObject{{"visible",true}};}
    auto* dialog=new QDialog(this);dialog->setObjectName("mesh.export.dialog");dialog->setWindowTitle("Save As Mesh");
    dialog->setAttribute(Qt::WA_DeleteOnClose);dialog->setWindowModality(Qt::WindowModal);
    auto* layout=new QVBoxLayout(dialog);auto* form=new QFormLayout;layout->addLayout(form);
    auto* bodies=new QComboBox;bodies->setObjectName("mesh.export.body");
    for(const auto& body:document_->bodies())bodies->addItem(body.name,body.bodyId);
    if(const int index=bodies->findData(bodyId);index>=0)bodies->setCurrentIndex(index);
    form->addRow("Body",bodies);
    auto* format=new QComboBox;format->setObjectName("mesh.export.format");
    format->addItem("STL (Binary)","binary");format->addItem("STL (ASCII)","ascii");form->addRow("Format",format);
    auto* units=new QComboBox;units->setObjectName("mesh.export.units");
    units->addItem("Millimeters","mm");units->addItem("Inches","in");form->addRow("Unit Type",units);
    auto* surface=new QDoubleSpinBox;surface->setObjectName("mesh.export.surfaceDeviation");
    surface->setDecimals(3);surface->setRange(0.001,10);surface->setValue(0.1);surface->setSuffix(" mm");form->addRow("Surface Deviation",surface);
    auto* normal=new QDoubleSpinBox;normal->setObjectName("mesh.export.normalDeviation");
    normal->setRange(1,45);normal->setValue(10);normal->setSuffix("°");form->addRow("Normal Deviation",normal);
    auto* note=new QLabel("Custom refinement · one solid\nSTL files do not store units. Use the same units when importing.");
    note->setWordWrap(true);layout->addWidget(note);
    auto* status=new QLabel;status->setObjectName("mesh.export.status");status->setWordWrap(true);layout->addWidget(status);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Save|QDialogButtonBox::Cancel);layout->addWidget(buttons);
    connect(buttons,&QDialogButtonBox::rejected,dialog,&QDialog::reject);
    const auto documentId=document_->authored().value("documentId").toString();const auto revision=document_->revision();
    connect(buttons,&QDialogButtonBox::accepted,dialog,[=,this]{
        if(document_->authored().value("documentId").toString()!=documentId || document_->revision()!=revision) {
            status->setText("The design changed. Close this window and restart Save As Mesh.");return;
        }
        auto path=QFileDialog::getSaveFileName(dialog,"Save As Mesh",bodies->currentText()+".stl","STL mesh (*.stl)");
        if(path.isEmpty())return;
        if(document_->authored().value("documentId").toString()!=documentId || document_->revision()!=revision) {
            status->setText("The design changed. Close this window and restart Save As Mesh.");return;
        }
        if(!path.endsWith(".stl",Qt::CaseInsensitive))path+=".stl";
        const auto result=execute("document.exportStl",{{"path",QFileInfo(path).absoluteFilePath()},{"bodyId",bodies->currentData().toString()},
            {"units",units->currentData().toString()},{"format",format->currentData().toString()},{"surfaceDeviationMm",surface->value()},{"normalDeviationDeg",normal->value()}});
        if(!result["ok"].toBool()){status->setText(result["error"].toObject()["message"].toString());return;}
        dialog->accept();
    });
    dialog->resize(430,300);dialog->show();return {{"visible",true},{"bodyId",bodies->currentData().toString()}};
}
