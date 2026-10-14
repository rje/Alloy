#include "AlloyWindow.h"
#include "LocalOccurrences.h"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSet>
#include <QVBoxLayout>

QJsonObject AlloyWindow::showMeshExport(const QString& bodyId,const QJsonArray& bodyIds,bool allVisible) {
    auto error=[](const QString& code,const QString& message){return QJsonObject{{"ok",false},{"error",QJsonObject{{"code",code},{"message",message}}}};};
    if(!session_.isEmpty())return error("active_command","Finish or Cancel the active command before exporting");
    if(document_->bodies().empty())return error("solid_required","Create or import a solid first");
    if(auto* existing=findChild<QDialog*>("mesh.export.dialog")){existing->raise();existing->activateWindow();return QJsonObject{{"visible",true}};}
    auto* dialog=new QDialog(this);dialog->setObjectName("mesh.export.dialog");dialog->setWindowTitle("Save As Mesh");
    dialog->setAttribute(Qt::WA_DeleteOnClose);dialog->setWindowModality(Qt::WindowModal);
    auto* layout=new QVBoxLayout(dialog);auto* form=new QFormLayout;layout->addLayout(form);
    auto* bodies=new QComboBox;bodies->setObjectName("mesh.export.body");
    QSet<QString> definitions;bool linked=false;
    for(const auto& value:document_->authored()["occurrences"].toArray()){
        const auto definition=value.toObject()["definitionId"].toString();
        if(definitions.contains(definition))linked=true;definitions.insert(definition);
    }
    for(const auto& body:document_->bodies()){
        int instances=0;for(const auto& placed:presentationBodies())if(placed.bodyId==body.bodyId)++instances;
        if(instances==1)bodies->addItem(body.name,body.bodyId);
    }
    if(const int index=bodies->findData(bodyId);index>=0)bodies->setCurrentIndex(index);
    const auto occurrences=LocalOccurrences::inspect(document_->authored(),document_->evaluation())["occurrences"].toArray();
    for(const auto& value:occurrences){const auto row=value.toObject();
        bodies->addItem("Component: "+row["path"].toString()+" (visible bodies)","occurrence:"+row["occurrenceId"].toString());
    }
    if(!linked)bodies->addItem("Selected bodies…","selected");
    bodies->addItem("All visible bodies","visible");
    if(!linked && !bodyIds.isEmpty())bodies->setCurrentIndex(bodies->findData("selected"));
    if(allVisible || (bodyId.isEmpty() && bodyIds.isEmpty() && document_->bodies().size()>1))bodies->setCurrentIndex(bodies->findData("visible"));
    if(bodyId.isEmpty() && bodyIds.isEmpty() && !allVisible && document_->activeOwnerId()!=document_->authored()["documentId"].toString())
        for(const auto& value:document_->authored()["occurrences"].toArray()){const auto occurrence=value.toObject();if(occurrence["definitionId"]==document_->activeOwnerId())bodies->setCurrentIndex(bodies->findData("occurrence:"+occurrence["id"].toString()));}
    form->addRow("Bodies / Component",bodies);
    auto* selected=new QListWidget(dialog);selected->setObjectName("mesh.export.bodyIds");
    for(const auto& body:document_->bodies()) {
        auto* item=new QListWidgetItem(body.name+(body.visible ? QString{} : " (Hidden)"),selected);
        item->setData(Qt::UserRole,body.bodyId);item->setFlags(item->flags()|Qt::ItemIsUserCheckable);
        item->setCheckState(bodyIds.contains(body.bodyId) || body.bodyId==bodyId ? Qt::Checked : Qt::Unchecked);
    }
    selected->setVisible(bodies->currentData()=="selected");layout->addWidget(selected);
    connect(bodies,&QComboBox::currentIndexChanged,dialog,[=]{selected->setVisible(bodies->currentData()=="selected");});
    auto* format=new QComboBox;format->setObjectName("mesh.export.format");
    format->addItem("STL (Binary)","binary");format->addItem("STL (ASCII)","ascii");form->addRow("Format",format);
    auto* units=new QComboBox;units->setObjectName("mesh.export.units");
    units->addItem("Millimeters","mm");units->addItem("Inches","in");form->addRow("Unit Type",units);
    auto* surface=new QDoubleSpinBox;surface->setObjectName("mesh.export.surfaceDeviation");
    surface->setDecimals(3);surface->setRange(0.001,10);surface->setValue(0.1);surface->setSuffix(" mm");form->addRow("Surface Deviation",surface);
    auto* normal=new QDoubleSpinBox;normal->setObjectName("mesh.export.normalDeviation");
    normal->setRange(1,45);normal->setValue(10);normal->setSuffix("°");form->addRow("Normal Deviation",normal);
    auto* note=new QLabel("Custom refinement · separate bodies retain their placement.\nComponent selections include visible bodies.\nSTL files do not store units. Use the same units when importing.");
    note->setWordWrap(true);layout->addWidget(note);
    auto* status=new QLabel;status->setObjectName("mesh.export.status");status->setWordWrap(true);layout->addWidget(status);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Save|QDialogButtonBox::Cancel);layout->addWidget(buttons);
    connect(buttons,&QDialogButtonBox::rejected,dialog,&QDialog::reject);
    const auto documentId=document_->authored().value("documentId").toString();const auto revision=document_->revision();
    connect(buttons,&QDialogButtonBox::accepted,dialog,[=,this]{
        if(document_->authored().value("documentId").toString()!=documentId || document_->revision()!=revision) {
            status->setText("The design changed. Close this window and restart Save As Mesh.");return;
        }
        QJsonObject selection;
        if(bodies->currentData()=="visible")selection["allVisible"]=true;
        else if(bodies->currentData()=="selected") {
            QJsonArray ids;for(int i=0;i<selected->count();++i)if(selected->item(i)->checkState()==Qt::Checked)ids.append(selected->item(i)->data(Qt::UserRole).toString());
            if(ids.isEmpty()){status->setText("Select at least one body.");return;}selection["bodyIds"]=ids;
        } else if(bodies->currentData().toString().startsWith("occurrence:"))selection["occurrenceId"]=bodies->currentData().toString().sliced(11);
        else selection["bodyId"]=bodies->currentData().toString();
        auto path=QFileDialog::getSaveFileName(dialog,"Save As Mesh",bodies->currentText()+".stl","STL mesh (*.stl)");
        if(path.isEmpty())return;
        if(document_->authored().value("documentId").toString()!=documentId || document_->revision()!=revision) {
            status->setText("The design changed. Close this window and restart Save As Mesh.");return;
        }
        if(!path.endsWith(".stl",Qt::CaseInsensitive))path+=".stl";
        selection["path"]=QFileInfo(path).absoluteFilePath();selection["units"]=units->currentData().toString();selection["format"]=format->currentData().toString();
        selection["surfaceDeviationMm"]=surface->value();selection["normalDeviationDeg"]=normal->value();
        const auto result=execute("document.exportStl",selection);
        if(!result["ok"].toBool()){status->setText(result["error"].toObject()["message"].toString());return;}
        dialog->accept();
    });
    dialog->resize(430,380);dialog->show();
    QJsonObject state{{"visible",true}};
    if(bodies->currentData()=="visible")state["allVisible"]=true;
    else if(bodies->currentData()=="selected")state["bodyIds"]=bodyIds;
    else if(bodies->currentData().toString().startsWith("occurrence:"))state["occurrenceId"]=bodies->currentData().toString().sliced(11);
    else state["bodyId"]=bodies->currentData().toString();
    return state;
}
