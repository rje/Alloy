// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "MeshImport.h"
#include "ReferenceView.h"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QShortcut>
#include <QVBoxLayout>
#include <QTimer>
#include <QStatusBar>
#include <QMessageBox>
#include <QScopeGuard>
#include <QSignalBlocker>
#include <QApplication>

namespace {
QJsonObject parameter(const QString& type,const QString& description){return {{"type",type},{"description",description}};}
QJsonObject failure(const QString& code,const QString& message){return {{"ok",false},{"error",QJsonObject{{"code",code},{"message",message}}}};}
QJsonObject importFailure(const QString& message) {return {{"ok",false},{"error",QJsonObject{{"code","mesh_import_failed"},{"message",message}}}};}
}
void AlloyWindow::closeMeshImport() {
    view_->setMeshExportPreview({});
    if(meshImportDock_){auto* dock=meshImportDock_.data();meshImportDock_=nullptr;canvasWindow_->removeDockWidget(dock);dock->hide();dock->deleteLater();}
    meshImportBase_={};meshImportCandidate_={};meshImportInputs_={};
    view_->setFocus(Qt::OtherFocusReason);
    QTimer::singleShot(0,this,[this]{if(QGuiApplication::applicationState()==Qt::ApplicationActive && !QApplication::activeModalWidget())activateWindow();});
}
QJsonObject AlloyWindow::updateMeshImport(const QString& units) {
    meshImportInputs_["units"]=units;
    auto publish=[this,units](const QJsonObject& result){
        if(meshImportDock_) {
            auto* field=meshImportDock_->findChild<QComboBox*>("mesh.import.units");
            const QSignalBlocker blocker(field);const int index=field->findData(units);if(index>=0)field->setCurrentIndex(index);
            const bool ok=!(result.value("ok").isBool() && !result.value("ok").toBool());
            meshImportDock_->findChild<QPushButton*>("mesh.import.ok")->setEnabled(ok);
            meshImportDock_->findChild<QLabel*>("mesh.import.error")->setText(ok ? QString{} : result.value("error").toObject().value("message").toString());
        }
        return result;
    };
    try {
        const auto mesh=MeshImport::withUnits(meshImportBase_,units);
        auto candidate=document_->authored();auto meshes=candidate.value("meshes").toArray();meshes.append(mesh);candidate["meshes"]=meshes;
        const auto error=CadDocument::validate(candidate);if(!error.isEmpty())return publish(importFailure(error));
        std::vector<gp_Pnt> facets;const auto vertices=mesh.value("verticesMm").toArray();
        for(const auto& row:mesh.value("triangles").toArray())for(const auto& index:row.toArray()){const auto p=vertices[index.toInt()].toArray();facets.emplace_back(p[0].toDouble(),p[1].toDouble(),p[2].toDouble());}
        meshImportCandidate_=candidate;meshImportInputs_["units"]=units;
        view_->setMeshExportPreview(facets);view_->fitMeshImport(mesh.value("summary").toObject().value("boundsMm").toObject());
        const auto summary=MeshImport::summary(mesh),bounds=summary.value("boundsMm").toObject();
        auto text=[](const QJsonArray& a){return QString("%1, %2, %3").arg(a[0].toDouble(),0,'g',10).arg(a[1].toDouble(),0,'g',10).arg(a[2].toDouble(),0,'g',10);};
        if(meshImportDock_)meshImportDock_->findChild<QLabel*>("mesh.import.summary")->setText(QString("%1 triangles · %2 vertices\nBounds (mm)\nMin: %3\nMax: %4").arg(summary.value("triangleCount").toInt()).arg(summary.value("vertexCount").toInt()).arg(text(bounds.value("min").toArray()),text(bounds.value("max").toArray())));
        return publish({{"preview",summary},{"authored",false}});
    }catch(const std::exception& error){return publish(importFailure(QString::fromUtf8(error.what())));}
}
void AlloyWindow::registerMeshImportCommands() {
    auto add=[this](QString id,QString title,QString description,QJsonObject schema,std::function<QJsonObject(const QJsonObject&)> run){commands_.push_back({id,title,description,schema,true,std::move(run)});};
    add("document.importMeshDialog","Import Mesh…","Choose one local STL, preview its explicit mm/in interpretation, then OK or Cancel.",{},[this](const auto&){
        if(!session_.isEmpty() || meshImportDock_ || document_->isRolledBack() || !pendingReference_.isEmpty() || nonzeroAssemblyPose() || !sliderPreviewAuthored_.isEmpty() || !jointGraphStage_.isEmpty() || transientSliderPosition_.has_value() || pendingReferenceEvaluation_)
            return failure("active_command","Finish the command and return to the committed design before importing");
        if(auto* existing=findChild<QFileDialog*>("mesh.import.chooser")){existing->raise();return QJsonObject{{"visible",true}};}
        auto* chooser=new QFileDialog(this,"Import Mesh",{},"STL mesh (*.stl)");chooser->setObjectName("mesh.import.chooser");chooser->setFileMode(QFileDialog::ExistingFile);chooser->setAcceptMode(QFileDialog::AcceptOpen);chooser->setAttribute(Qt::WA_DeleteOnClose);chooser->setWindowModality(Qt::WindowModal);
        // AppKit may close its sheet with no editor key window. Restore the
        // owning editor as part of chooser closure, only while Alloy is active.
        connect(chooser,&QFileDialog::finished,this,[this]{QTimer::singleShot(0,this,[this]{
            if(QGuiApplication::applicationState()==Qt::ApplicationActive && !QApplication::activeModalWidget())activateWindow();
        });});
        connect(chooser,&QFileDialog::fileSelected,this,[this](const QString& path){
            // The native chooser closes before the panel is created. No event
            // pumping or model mutation occurs during the synchronous decoder.
            QTimer::singleShot(0,this,[this,path]{const auto result=execute("mesh.importPreview",{{"path",QFileInfo(path).absoluteFilePath()}});if(!result.value("ok").toBool()) {
                const auto error=result.value("error").toObject();auto* message=new QMessageBox(QMessageBox::Warning,"Import Mesh",error.value("message").toString(),QMessageBox::Ok,this);message->setObjectName("mesh.import.failure");message->setInformativeText(QFileInfo(path).fileName()+"\n"+error.value("remedy").toString());message->setAttribute(Qt::WA_DeleteOnClose);message->setWindowModality(Qt::WindowModal);
                connect(message,&QDialog::finished,this,[this]{QTimer::singleShot(0,this,[this]{if(QGuiApplication::applicationState()==Qt::ApplicationActive && !QApplication::activeModalWidget())activateWindow();});});message->open();
            }});
        });
        chooser->open();return QJsonObject{{"visible",true},{"loadingCancellable",false}};
    });
    add("mesh.importPreview","Preview STL import","Synchronously prepare one STL; ready preview is disposable and commits the prepared bytes only.",{{"path",parameter("string","Absolute local STL source path.")}},[this](const auto& a){
        if(!session_.isEmpty() || meshImportDock_ || document_->isRolledBack() || !pendingReference_.isEmpty() || nonzeroAssemblyPose() || !sliderPreviewAuthored_.isEmpty() || !jointGraphStage_.isEmpty() || transientSliderPosition_.has_value() || pendingReferenceEvaluation_)return failure("active_command","Finish the active command before importing");
        try {meshImportBase_=MeshImport::readStl(a.value("path").toString(),"mm");}catch(const MeshImport::Error& error){return QJsonObject{{"ok",false},{"error",error.details}};}
        meshImportBaseRevision_=document_->revision();
        meshImportInputs_={{"path",a.value("path")},{"units","mm"}};
        auto* dock=new QDockWidget("Import Mesh",canvasWindow_);meshImportDock_=dock;dock->setObjectName("mesh.import.panel");dock->setFeatures(QDockWidget::NoDockWidgetFeatures);dock->setMinimumWidth(310);
        auto* panel=new QWidget;auto* layout=new QVBoxLayout(panel);auto* name=new QLabel(meshImportBase_.value("name").toString());name->setWordWrap(true);layout->addWidget(name);
        auto* form=new QFormLayout;auto* units=new QComboBox;units->setObjectName("mesh.import.units");units->setFocusPolicy(Qt::StrongFocus);units->addItem("Millimeter (mm)","mm");units->addItem("Inch (in)","in");form->addRow("Unit Type",units);layout->addLayout(form);
        auto* summary=new QLabel;summary->setObjectName("mesh.import.summary");summary->setWordWrap(true);layout->addWidget(summary);auto* message=new QLabel("Source coordinates are preserved.\nA mesh body is not an editable solid.");message->setWordWrap(true);layout->addWidget(message);
        auto* errorLabel=new QLabel;errorLabel->setObjectName("mesh.import.error");errorLabel->setWordWrap(true);layout->addWidget(errorLabel);layout->addStretch();
        auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);buttons->button(QDialogButtonBox::Ok)->setObjectName("mesh.import.ok");buttons->button(QDialogButtonBox::Cancel)->setObjectName("mesh.import.cancel");layout->addWidget(buttons);dock->setWidget(panel);canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,dock);dock->show();
        connect(units,&QComboBox::currentIndexChanged,this,[this,units]{execute("mesh.importUnits",{{"units",units->currentData().toString()}});});
        connect(buttons,&QDialogButtonBox::accepted,this,[this,errorLabel]{const auto r=execute("mesh.importAccept");if(!r.value("ok").toBool())errorLabel->setText(r.value("error").toObject().value("message").toString());});
        connect(buttons,&QDialogButtonBox::rejected,this,[this]{execute("mesh.importCancel");});auto* escape=new QShortcut(QKeySequence(Qt::Key_Escape),panel);escape->setContext(Qt::WidgetWithChildrenShortcut);connect(escape,&QShortcut::activated,this,[this]{execute("mesh.importCancel");});
        const auto result=updateMeshImport("mm");if(result.value("ok").isBool() && !result.value("ok").toBool()){closeMeshImport();return result;}units->setFocus();return result;
    });
    add("mesh.importUnits","Import Mesh units","Update the ready preview from the original prepared coordinates without authored mutation.",{{"units",parameter("string","mm or in source interpretation.")}},[this](const auto& a){if(!meshImportDock_)return failure("invalid_session","No import preview is active");return updateMeshImport(a.value("units").toString());});
    add("mesh.importCancel","Cancel Import Mesh","Discard the ready preview, preserving the document and selection.",{},[this](const auto&){if(!meshImportDock_)return failure("invalid_session","No import preview is active");closeMeshImport();return QJsonObject{{"cancelled",true}};});
    add("mesh.importAccept","Accept Import Mesh","Commit the prepared preview through document.importStl.",{},[this](const auto&){
        if(!meshImportDock_)return failure("invalid_session","No import preview is active");
        if(document_->revision()!=meshImportBaseRevision_)return failure("stale_preview","The design changed; Cancel and select the STL again");
        if(meshImportCandidate_.value("meshes").toArray().last().toObject().value("source").toObject().value("units")!=meshImportInputs_.value("units"))return failure("invalid_preview","Choose valid source units before OK");
        meshImportCommitting_=true;const auto reset=qScopeGuard([this]{meshImportCommitting_=false;});const auto result=execute("document.importStl",meshImportInputs_);
        if(!result.value("ok").toBool())return result;closeMeshImport();return result.value("result").toObject();
    });
}
