#include "AlloyWindow.h"
#include "ReferenceView.h"
#include "LocalOccurrences.h"
#include "ExportScope.h"
#include <QTimer>
#include <QSignalBlocker>
#include <cmath>
#include <thread>
#include <BRepBuilderAPI_Copy.hxx>
#include <QDockWidget>
#include <QTabBar>
#include <QStatusBar>
#include <QElapsedTimer>
#include <QDateTime>
#include <algorithm>

#include <QComboBox>
#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QLabel>
#include <QListWidget>
#include <QLineEdit>
#include <QDir>
#include <QPushButton>
#include <QHBoxLayout>
#include <QRegularExpression>
#include <QSet>
#include <QVBoxLayout>

namespace {
class MeshExportDialog final:public QDialog {
public:
    using QDialog::QDialog;
    std::function<void()> pendingCancel;
    void reject() override {if(pendingCancel)pendingCancel();else QDialog::reject();}
};
}

QJsonObject AlloyWindow::showMeshExport(const QString& bodyId,const QJsonArray& bodyIds,bool allVisible) {
    auto error=[](const QString& code,const QString& message){return QJsonObject{{"ok",false},{"error",QJsonObject{{"code",code},{"message",message}}}};};
    if(!session_.isEmpty())return error("active_command","Finish or Cancel the active command before exporting");
    if(document_->bodies().empty())return error("body_required","Create or import a body first");
    if(auto* existing=findChild<QDialog*>("mesh.export.dialog")){existing->raise();existing->activateWindow();return QJsonObject{{"visible",true}};}
    auto* dialog=new MeshExportDialog(this);dialog->setObjectName("mesh.export.dialog");dialog->setWindowTitle("Save As Mesh");
    dialog->setAttribute(Qt::WA_DeleteOnClose);dialog->setWindowModality(Qt::WindowModal);
    // Closing an attached macOS sheet can leave the active application without
    // a key window. Return ownership to its editor after the modal has closed;
    // do not steal activation if the user has switched to another application.
    connect(dialog,&QDialog::finished,this,[this]{
        if(QGuiApplication::applicationState()==Qt::ApplicationActive && !QApplication::activeModalWidget())activateWindow();
    });
    auto* layout=new QVBoxLayout(dialog);auto* form=new QFormLayout;layout->addLayout(form);
    auto* bodies=new QComboBox;bodies->setObjectName("mesh.export.body");
    // macOS gives noneditable combos TabFocus by default, which its usual
    // text-controls-only traversal skips. These are the sheet's input fields:
    // include them in native initial focus and Tab traversal on every platform.
    bodies->setFocusPolicy(Qt::StrongFocus);
    QSet<QString> definitions;bool linked=false;
    for(const auto& value:document_->authored()["occurrences"].toArray()){
        const auto definition=value.toObject()["definitionId"].toString();
        if(definitions.contains(definition))linked=true;definitions.insert(definition);
    }
    for(const auto& body:document_->bodies()){
        int instances=0;for(const auto& placed:presentationBodies())if(placed.bodyId==body.bodyId)++instances;
        if(instances==1)bodies->addItem(body.name+(body.summary.value("type")=="surface" ? " · Open surface — non-solid" : ""),body.bodyId);
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
    if(linked && !bodyId.isEmpty() && bodies->findData(bodyId)<0)bodies->setCurrentIndex(-1);
    form->addRow("Bodies / Component",bodies);
    auto* selected=new QListWidget(dialog);selected->setObjectName("mesh.export.bodyIds");
    for(const auto& body:document_->bodies()) {
        auto* item=new QListWidgetItem(body.name+(body.summary.value("type")=="surface" ? " · Open surface — non-solid" : "")+(body.visible ? QString{} : " (Hidden)"),selected);
        item->setData(Qt::UserRole,body.bodyId);item->setFlags(item->flags()|Qt::ItemIsUserCheckable);
        item->setCheckState(bodyIds.contains(body.bodyId) || body.bodyId==bodyId ? Qt::Checked : Qt::Unchecked);
    }
    selected->setVisible(bodies->currentData()=="selected");layout->addWidget(selected);
    connect(bodies,&QComboBox::currentIndexChanged,dialog,[=]{selected->setVisible(bodies->currentData()=="selected");});
    auto* format=new QComboBox;format->setObjectName("mesh.export.format");
    format->setFocusPolicy(Qt::StrongFocus);
    format->addItem("STL (Binary)","binary");format->addItem("STL (ASCII)","ascii");
    format->addItem("3MF","3mf");format->addItem("OBJ","obj");form->addRow("Format",format);
    auto* structure=new QComboBox;structure->setObjectName("mesh.export.structure");structure->setFocusPolicy(Qt::StrongFocus);
    structure->addItem("One File","oneFile");structure->addItem("One Body Per File","oneBodyPerFile");form->addRow("Structure",structure);
    auto* selectionSummary=new QLabel;selectionSummary->setObjectName("mesh.export.selectionCount");selectionSummary->setWordWrap(true);layout->addWidget(selectionSummary);
    auto* surfaceWarning=new QLabel;surfaceWarning->setObjectName("mesh.export.surfaceWarning");surfaceWarning->setWordWrap(true);
    auto* parentRow=new QWidget;auto* parentLayout=new QHBoxLayout(parentRow);parentLayout->setContentsMargins(0,0,0,0);
    auto* parentDirectory=new QLineEdit(QDir::homePath());parentDirectory->setObjectName("mesh.export.parentDirectory");
    auto* browse=new QPushButton("Browse…");browse->setObjectName("mesh.export.browse");browse->setFocusPolicy(Qt::StrongFocus);
    parentLayout->addWidget(parentDirectory,1);parentLayout->addWidget(browse);form->addRow("Parent Folder",parentRow);
    auto* folderName=new QLineEdit("mesh-batch");folderName->setObjectName("mesh.export.folderName");form->addRow("New Folder Name",folderName);
    auto* directory=new QLabel;directory->setObjectName("mesh.export.directory");directory->setWordWrap(true);
    directory->setTextInteractionFlags(Qt::TextSelectableByMouse|Qt::TextSelectableByKeyboard);form->addRow("Destination",directory);
    const auto destinationError=[=]() -> QString {
        const auto name=folderName->text();
        static const QRegularExpression forbidden(R"([<>:"/\\|?*\x{0000}-\x{001f}])");
        static const QRegularExpression reserved(R"(^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(?:\.|$))",QRegularExpression::CaseInsensitiveOption);
        if(name.isEmpty())return "Enter a new folder name.";
        if(name=="." || name==".." || name!=name.trimmed() || name.endsWith('.') || name.contains(forbidden) || reserved.match(name).hasMatch())
            return "Use a folder name without path separators, reserved characters/names, or leading/trailing spaces or trailing dots.";
        const QFileInfo parent(parentDirectory->text());
        if(!parent.isAbsolute() || !parent.exists() || !parent.isDir())return "Choose an existing parent folder using Browse, or enter its absolute path.";
        const QFileInfo target(directory->text());
        if(target.isSymLink() || target.exists())return "The destination already exists (file, folder or symlink). Enter a different new folder name.";
        return {};
    };
    connect(browse,&QPushButton::clicked,dialog,[=,this]{
        if(dialog->findChild<QFileDialog*>("mesh.export.parentChooser"))return;
        auto* chooser=new QFileDialog(dialog,"Choose Parent Folder",parentDirectory->text());
        chooser->setObjectName("mesh.export.parentChooser");chooser->setAttribute(Qt::WA_DeleteOnClose);
        chooser->setFileMode(QFileDialog::Directory);chooser->setOption(QFileDialog::ShowDirsOnly,true);
        chooser->setWindowModality(Qt::WindowModal);
        connect(chooser,&QFileDialog::fileSelected,dialog,[=](const QString& path){parentDirectory->setText(path);});
        connect(chooser,&QDialog::finished,dialog,[=,this]{
            if(dialog->isVisible() && findChild<QDialog*>("mesh.export.dialog")==dialog
                && QGuiApplication::applicationState()==Qt::ApplicationActive){
                dialog->raise();dialog->activateWindow();browse->setFocus(Qt::OtherFocusReason);
            }
        });
        chooser->open();
    });
    auto* names=new QLabel;names->setObjectName("mesh.export.filenames");names->setWordWrap(true);layout->addWidget(names);
    auto* units=new QComboBox;units->setObjectName("mesh.export.units");
    units->setFocusPolicy(Qt::StrongFocus);
    units->addItem("Millimeters","mm");units->addItem("Inches","in");form->addRow("Unit Type",units);
    auto* refinement=new QComboBox;refinement->setObjectName("mesh.export.refinement");refinement->setFocusPolicy(Qt::StrongFocus);
    refinement->addItem("Low (Coarse)","Low");refinement->addItem("Medium","Medium");refinement->addItem("High (Fine)","High");refinement->addItem("Custom","Custom");refinement->setCurrentIndex(1);form->addRow("Refinement",refinement);
    auto* surface=new QDoubleSpinBox;surface->setObjectName("mesh.export.surfaceDeviation");
    surface->setDecimals(3);surface->setRange(0.001,10);surface->setValue(0.1);surface->setSuffix(" mm");form->addRow("Surface Deviation",surface);
    auto* normal=new QDoubleSpinBox;normal->setObjectName("mesh.export.normalDeviation");
    normal->setRange(1,45);normal->setValue(10);normal->setSuffix("°");form->addRow("Normal Deviation",normal);
    auto* note=new QLabel;note->setObjectName("mesh.export.note");
    note->setWordWrap(true);layout->addWidget(note);
    const auto updateFormat=[=]{
        const bool threeMf=format->currentData()=="3mf";
        units->setVisible(!threeMf);
        if(auto* label=form->labelForField(units))label->setVisible(!threeMf);
        note->setText(threeMf
            ? "3MF uses millimeters. Each selected body retains its placement."
            : format->currentData()=="obj" ? "One file · separate body groups · no materials.\nZ-up world coordinates; OBJ does not store units. Use matching import units."
            : structure->currentData()=="oneBodyPerFile"
                ? "STL files do not store units. Use matching import units.\nEach selected body gets a separate STL file."
                : "STL files do not store units. Use matching import units.\nOne File combines the selected bodies into one triangle file.");
    };
    connect(format,&QComboBox::currentIndexChanged,dialog,updateFormat);updateFormat();
    auto* previewButton=new QPushButton("Preview");previewButton->setObjectName("mesh.export.preview");layout->addWidget(previewButton);
    auto* count=new QLabel("Triangle Count: —");count->setObjectName("mesh.export.triangleCount");layout->addWidget(count);
    auto* policy=new QLabel("Alloy presets: Low 0.5 mm / 25° · Medium 0.1 mm / 10° · High 0.02 mm / 5°.\nMaximum Edge Length and Aspect Ratio are not yet available.");policy->setWordWrap(true);layout->addWidget(policy);
    auto* status=new QLabel;status->setObjectName("mesh.export.status");status->setWordWrap(true);layout->addWidget(status);layout->addWidget(surfaceWarning);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Save|QDialogButtonBox::Cancel);layout->addWidget(buttons);
    connect(buttons,&QDialogButtonBox::rejected,dialog,&QDialog::reject);
    const auto settings=[=]{
        QJsonObject inputs{{"format",format->currentData().toString()},{"units",format->currentData()=="3mf" ? "mm" : units->currentData().toString()},
            {"preset",refinement->currentData().toString()},{"surfaceDeviationMm",surface->value()},{"normalDeviationDeg",normal->value()}};
        const auto source=bodies->currentData().toString();
        if(source=="visible")inputs["allVisible"]=true;
        else if(source=="selected"){QJsonArray ids;for(int i=0;i<selected->count();++i)if(selected->item(i)->checkState()==Qt::Checked)ids.append(selected->item(i)->data(Qt::UserRole).toString());inputs["bodyIds"]=ids;}
        else if(source.startsWith("occurrence:"))inputs["occurrenceId"]=source.sliced(11);
        else inputs["bodyId"]=source;
        return inputs;
    };
    const auto updateScope=[=,this]{
        const bool obj=format->currentData()=="obj";int size=0;QStringList bodyNames;std::vector<EvaluatedBody> chosen;
        try{chosen=ExportScope::select(document_->authored(),presentationBodies(),settings()).bodies;size=int(chosen.size());for(const auto& body:chosen)bodyNames.append(body.name);}
        catch(const GeometryKernel::EvaluationError& error){status->setText(error.diagnostics().value("message").toString());}
        const bool hasSurface=std::any_of(chosen.begin(),chosen.end(),[](const auto& body){return body.summary.value("type")=="surface";});
        const bool surfaceSupported=!hasSurface || (size==1 && (format->currentData()=="binary" || format->currentData()=="ascii"));
        surfaceWarning->setVisible(hasSurface);
        surfaceWarning->setText(surfaceSupported
            ? "Open surface — non-solid\nThis STL contains an open, zero-thickness surface. It is not a closed volume and is not a printable solid."
            : "Open surface — non-solid\nSelect exactly one open surface body and STL format. Mixed selections and surface OBJ/3MF export are not supported.");
        selectionSummary->setText(QString("Selected bodies: %1 · %2").arg(size).arg(bodyNames.join(", ")));
        structure->setEnabled(!obj && size>1);
        if((obj || size<=1) && structure->currentIndex()!=0){const QSignalBlocker blocker(structure);structure->setCurrentIndex(0);}
        updateFormat();
        const bool perBody=!obj && structure->currentData()=="oneBodyPerFile";
        const bool preview=surfaceSupported && !obj && size==1 && !perBody;
        dialog->setProperty("requiresMeshPreview",preview);
        previewButton->setVisible(preview);count->setVisible(preview);
        for(auto* field:QList<QWidget*>{parentRow,folderName,directory}){field->setVisible(perBody);if(auto* label=form->labelForField(field))label->setVisible(perBody);}
        directory->setText(QDir::cleanPath(QDir(parentDirectory->text()).absoluteFilePath(folderName->text())));
        names->setVisible(perBody);
        if(perBody){QStringList files;try{for(const auto& value:MeshExport::batchPlan(chosen,directory->text(),format->currentData().toString()))files.append(value.toObject().value("filename").toString());}catch(const GeometryKernel::EvaluationError&){}
            names->setText("New folder only; its parent must exist. Existing files, folders and symlinks are refused.\n"+files.join("\n"));}
        const auto invalidDestination=perBody ? destinationError() : QString{};
        if(perBody)status->setText(invalidDestination);
        buttons->button(QDialogButtonBox::Save)->setEnabled(surfaceSupported && size>0 && !preview && invalidDestination.isEmpty());
    };
    const auto invalidate=[=,this]{clearMeshPreview();dialog->setProperty("meshPreviewId",QString{});dialog->setProperty("meshPreviewPhase",QString{});count->setText("Triangle Count: —");updateScope();};
    connect(structure,&QComboBox::currentIndexChanged,dialog,invalidate);
    connect(parentDirectory,&QLineEdit::textChanged,dialog,updateScope);
    connect(folderName,&QLineEdit::textChanged,dialog,updateScope);
    connect(refinement,&QComboBox::currentIndexChanged,dialog,[=]{
        const auto preset=refinement->currentData().toString();
        if(preset!="Custom"){const QSignalBlocker bs(surface),bn(normal);surface->setValue(preset=="Low" ? .5 : preset=="High" ? .02 : .1);normal->setValue(preset=="Low" ? 25 : preset=="High" ? 5 : 10);}
        invalidate();
    });
    const auto custom=[=]{refinement->setCurrentIndex(refinement->findData("Custom"));invalidate();};
    connect(surface,&QDoubleSpinBox::valueChanged,dialog,custom);connect(normal,&QDoubleSpinBox::valueChanged,dialog,custom);
    connect(bodies,&QComboBox::currentIndexChanged,dialog,invalidate);connect(selected,&QListWidget::itemChanged,dialog,invalidate);
    connect(units,&QComboBox::currentIndexChanged,dialog,invalidate);
    connect(format,&QComboBox::currentIndexChanged,dialog,[=]{invalidate();const bool obj=format->currentData()=="obj";refinement->setVisible(!obj);if(auto* label=form->labelForField(refinement))label->setVisible(!obj);policy->setVisible(!obj);});
    connect(previewButton,&QPushButton::clicked,dialog,[=,this]{
        invalidate();const auto inputs=settings();
        // Resolve the sheet's legacy selectors to one exact instance at the shared boundary.
        QJsonObject exact=inputs;
        try{const auto chosen=ExportScope::select(document_->authored(),presentationBodies(),inputs);
            if(chosen.bodies.size()!=1){status->setText("Preview supports exactly one committed body instance.");return;}
            exact.remove("bodyIds");exact.remove("allVisible");exact["bodyId"]=chosen.bodies.front().bodyId;
            if(!chosen.bodies.front().occurrenceId.isEmpty())exact["occurrenceId"]=chosen.bodies.front().occurrenceId;
        }catch(const GeometryKernel::EvaluationError& error){status->setText(error.diagnostics().value("message").toString());return;}
        const auto response=execute("document.previewMesh",exact);
        if(!response.value("ok").toBool()){status->setText(response.value("error").toObject().value("message").toString());return;}
        dialog->setProperty("meshPreviewId",response.value("result").toObject().value("previewId").toString());status->setText("Preparing preview…");
    });
    auto* previewUi=new QTimer(dialog);previewUi->setInterval(50);
    connect(previewUi,&QTimer::timeout,dialog,[=,this]{
        if(!dialog->property("requiresMeshPreview").toBool() || !dialog->property("exportOperation").toString().isEmpty())return;
        const auto id=dialog->property("meshPreviewId").toString();if(id.isEmpty())return;
        const auto state=meshPreviewState();const auto phase=state.value("evaluationStatus").toString();
        const bool ready=phase=="completed" && state.value("previewId")==id;
        buttons->button(QDialogButtonBox::Save)->setEnabled(ready);
        count->setText(ready ? "Triangle Count: "+QString::number(state.value("triangles").toInteger()) : "Triangle Count: —");
        if(dialog->property("meshPreviewPhase").toString()!=phase){
            dialog->setProperty("meshPreviewPhase",phase);
            if(ready)status->setText("Preview ready.");
            else if(phase=="failed")status->setText(state.value("error").toObject().value("message").toString());
            else if(phase!="pending")status->setText("Preview invalidated. Preview the current body and settings again.");
        }
    });previewUi->start();
    connect(dialog,&QDialog::finished,this,[this]{clearMeshPreview();});
    invalidate();

    const auto documentId=document_->authored().value("documentId").toString();const auto revision=document_->revision();
    connect(buttons,&QDialogButtonBox::accepted,dialog,[=,this]{
        if(document_->authored().value("documentId").toString()!=documentId || document_->revision()!=revision) {
            status->setText("The design changed. Close this window and restart Save As Mesh.");return;
        }
        if(bodies->currentIndex()<0){status->setText("Choose a component or all visible bodies.");return;}
        QJsonObject selection;
        if(bodies->currentData()=="visible")selection["allVisible"]=true;
        else if(bodies->currentData()=="selected") {
            QJsonArray ids;for(int i=0;i<selected->count();++i)if(selected->item(i)->checkState()==Qt::Checked)ids.append(selected->item(i)->data(Qt::UserRole).toString());
            if(ids.isEmpty()){status->setText("Select at least one body.");return;}selection["bodyIds"]=ids;
        } else if(bodies->currentData().toString().startsWith("occurrence:"))selection["occurrenceId"]=bodies->currentData().toString().sliced(11);
        else selection["bodyId"]=bodies->currentData().toString();
        const bool threeMf=format->currentData()=="3mf";
        const bool obj=format->currentData()=="obj";
        const QString suffix=threeMf ? ".3mf" : obj ? ".obj" : ".stl";
        const auto previewId=dialog->property("meshPreviewId").toString();
        const bool requiresPreview=dialog->property("requiresMeshPreview").toBool();
        const bool perBody=structure->currentData()=="oneBodyPerFile";
        if(requiresPreview && (meshPreviewState().value("evaluationStatus")!="completed" || !meshPreviewWork_ || meshPreviewWork_->id!=previewId)){status->setText("Preview the current solid and settings before Save.");return;}
        const auto chosenSettings=settings();

        if(perBody){const auto message=destinationError();if(!message.isEmpty()){status->setText(message);return;}}
        auto path=perBody ? directory->text() : QFileDialog::getSaveFileName(dialog,"Save As Mesh",bodies->currentText()+suffix,
            threeMf ? "3MF mesh (*.3mf)" : obj ? "OBJ mesh (*.obj)" : "STL mesh (*.stl)");
        if(path.isEmpty())return;
        if(document_->authored().value("documentId").toString()!=documentId || document_->revision()!=revision) {
            status->setText("The design changed. Close this window and restart Save As Mesh.");return;
        }
        if(!perBody && !path.endsWith(suffix,Qt::CaseInsensitive))path+=suffix;
        if(requiresPreview){
            if(settings()!=chosenSettings || !meshPreviewWork_ || meshPreviewWork_->id!=previewId || !meshPreviewCurrent(*meshPreviewWork_)){invalidate();status->setText("The design, selection or settings changed. Preview again before Save.");return;}
            const auto result=execute("document.saveMeshPreview",{{"previewId",previewId},{"path",QFileInfo(path).absoluteFilePath()}});
            if(!result.value("ok").toBool()){status->setText(result.value("error").toObject().value("message").toString());return;}
            dialog->accept();return;
        }

        if(!obj)selection["structure"]=structure->currentData().toString();
        selection["path"]=QFileInfo(path).absoluteFilePath();
        if(!threeMf){selection["units"]=units->currentData().toString();if(!obj)selection["format"]=format->currentData().toString();}
        selection["surfaceDeviationMm"]=surface->value();selection["normalDeviationDeg"]=normal->value();
        const auto result=execute(threeMf ? "document.export3mf" : obj ? "document.exportObj" : "document.exportStl",selection);
        if(!result["ok"].toBool()){status->setText(result["error"].toObject()["message"].toString());return;}
        if(result.value("completion")=="pending"){
            dialog->setProperty("exportOperation",result.value("result").toObject().value("id").toString());
            dialog->pendingCancel=[this]{execute("command.cancel",{{"session",meshExportOperation_.value("id")}});};
            refreshMeshExportDialog();
        }else dialog->accept();
    });
    dialog->resize(470,560);dialog->show();
    QJsonObject state{{"visible",true}};
    if(bodies->currentData()=="visible")state["allVisible"]=true;
    else if(bodies->currentData()=="selected")state["bodyIds"]=bodyIds;
    else if(bodies->currentData().toString().startsWith("occurrence:"))state["occurrenceId"]=bodies->currentData().toString().sliced(11);
    else state["bodyId"]=bodies->currentData().toString();
    return state;
}

QJsonObject AlloyWindow::beginMeshExport(const std::vector<EvaluatedBody>& bodies,const QJsonObject& inputs,const QString& label,const QString& format) {
    const auto structure=inputs.value("structure").toString("oneFile");
    const bool hasSurface=std::any_of(bodies.begin(),bodies.end(),[](const auto& body){return body.summary.value("type")=="surface";});
    if(format=="stl" && hasSurface && (bodies.size()!=1 || inputs.value("bodyId").toString().isEmpty() || structure!="oneFile")){
        QJsonArray selected;for(const auto& body:bodies)selected.append(QJsonObject{{"bodyId",body.bodyId},{"bodyType",body.summary.value("type")}});
        return {{"ok",false},{"error",QJsonObject{{"code","surface_scope_unsupported"},{"message","Open-surface STL requires one explicit bodyId and One File; mixed selections and surface batches are not supported"},{"selectedBodies",selected}}}};
    }
    if(structure!="oneFile" && structure!="oneBodyPerFile")return {{"ok",false},{"error",QJsonObject{{"code","invalid_structure"},{"message","Structure must be oneFile or oneBodyPerFile"}}}};
    auto task=std::make_shared<GeometryEvaluation>();task->command=format=="3mf" ? "document.export3mf" : format=="obj" ? "document.exportObj" : "document.exportStl";task->phase="export";
    task->document=document_;task->documentId=document_->authored().value("documentId").toString();
    task->revision=document_->revision();task->base=document_->authored();task->retainedSession=session_;
    task->tabId=activeTabId_;task->sessionId=CadDocument::newId();task->inputs=inputs;
    task->generation=++geometryEvaluationGeneration_;task->exportPublication=std::make_shared<MeshExport::PublicationGate>();
    // Copy geometry while the GUI exclusively owns the document. Do not hand
    // worker copies of live support-face, edge, topology or triangulation caches.
    QElapsedTimer snapshot;snapshot.start();
    QSet<QString> localOccurrenceIds;
    for(const auto& value:task->base.value("occurrences").toArray())
        localOccurrenceIds.insert(value.toObject().value("id").toString());
    for(const auto& body:bodies){
        BRepBuilderAPI_Copy copy(body.shape,true,false);
        if(!copy.IsDone())return {{"ok",false},{"error",QJsonObject{{"code","export_failed"},{"message","Cannot detach export geometry"}}}};
        EvaluatedBody detached;detached.shape=copy.Shape();detached.bodyId=body.bodyId;
        detached.name=body.name;detached.occurrenceId=body.occurrenceId;detached.definitionId=body.definitionId;detached.summary=body.summary;
        gp_Trsf transform;
        if(localOccurrenceIds.contains(body.occurrenceId))
            transform=LocalOccurrences::worldTransform(task->base,body.occurrenceId);
        task->exportTransforms.push_back(transform);
        task->exportBodies.push_back(std::move(detached));
    }
    task->snapshotMs=snapshot.elapsed();
    meshExportOperation_={{"id",task->sessionId},{"command",task->command},{"scopeLabel",label},
        {"evaluationStatus","pending"},{"evaluationPhase","preparing"},{"previewGeneration",qint64(task->generation)},
        {"sourceDocumentRevision",qint64(task->revision)},{"cancellation","Cancel before publication approval preserves the destination"},
        {"evaluationTimingMs",QJsonObject{{"guiSnapshot",task->snapshotMs}}},
        {"evaluationHeartbeat",QJsonObject{{"ticks",0},{"lastMs",QDateTime::currentMSecsSinceEpoch()},{"maxGapMs",0}}}};
    documentOpenOperation_={};sketchEvaluationOperation_={};geometryEvaluationTerminal_={};
    // All mutations, including native tab/model panels, remain frozen through
    // drain. Approval cannot later be revoked by a normal document mutation.
    meshExportFrozenControls_.clear();
    auto freeze=[this](QWidget* widget){if(widget && widget->isEnabled()){meshExportFrozenControls_.append(widget);widget->setEnabled(false);}};
    for(auto* dock:findChildren<QDockWidget*>())freeze(dock);
    for(auto* dialog:findChildren<QDialog*>())if(dialog->objectName()!="mesh.export.dialog")freeze(dialog);
    freeze(documentTabBar_);freeze(findChild<QWidget*>("modelingRibbon"));
    meshExportCancel_=new QPushButton("Cancel Export",statusBar());meshExportCancel_->setObjectName("mesh.cancelExport");
    statusBar()->addPermanentWidget(meshExportCancel_);
    connect(meshExportCancel_,&QPushButton::clicked,this,[this]{execute("command.cancel",{{"session",meshExportOperation_.value("id")}});});
    status_->hide();statusBar()->showMessage("Preparing "+format.toUpper()+"… You can navigate or Cancel Export before publication.");
    geometryEvaluationQueued_=task;startGeometryEvaluation();return meshExportOperation_;
}

void AlloyWindow::evaluateMeshExport(GeometryEvaluation& task) {
    QElapsedTimer timer,publication;timer.start();
    try {
        const auto gate=task.exportPublication;
        const bool threeMf=task.command=="document.export3mf";
        const bool obj=task.command=="document.exportObj";
        if(gate->state()==MeshExport::PublicationGate::State::Denied)
            task.exportError={{"code","export_cancelled"},{"message","Mesh export cancelled; destination preserved"}};
        else {
            const auto approve=[&]{
                task.sourceMs=timer.elapsed();
                const bool approved=gate->request();
                if(approved)publication.start();
                return approved;
            };
            task.exportResult=task.inputs.value("structure")=="oneBodyPerFile"
                ? MeshExport::writeBatch(task.exportBodies,task.exportTransforms,task.inputs.value("path").toString(),
                    threeMf ? "3mf" : task.inputs.value("format").toString(),task.inputs.value("units").toString(),
                    task.inputs.value("surfaceDeviationMm").toDouble(),task.inputs.value("normalDeviationDeg").toDouble(),approve,
                    [&]{return gate->state()==MeshExport::PublicationGate::State::Denied;})
                : threeMf
                ? MeshExport::write3mf(task.exportBodies,task.exportTransforms,task.inputs.value("path").toString(),
                    task.inputs.value("surfaceDeviationMm").toDouble(),task.inputs.value("normalDeviationDeg").toDouble(),approve)
                : obj ? MeshExport::writeObj(task.exportBodies,task.inputs.value("path").toString(),task.inputs.value("units").toString(),
                    task.inputs.value("surfaceDeviationMm").toDouble(),task.inputs.value("normalDeviationDeg").toDouble(),approve)
                : MeshExport::writeStl(task.exportBodies,task.inputs.value("path").toString(),task.inputs.value("units").toString(),task.inputs.value("format")=="binary",
                    task.inputs.value("surfaceDeviationMm").toDouble(),task.inputs.value("normalDeviationDeg").toDouble(),approve);
        }
    }catch(const GeometryKernel::EvaluationError& error){task.exportError=error.diagnostics();}
    catch(...){task.exportPublication->deny();throw;}
    task.fullMs=timer.elapsed();task.stageMs=publication.isValid() ? publication.elapsed() : 0;
}

bool AlloyWindow::meshExportCurrent(const GeometryEvaluation& task) const {
    return document_==task.document && activeTabId_==task.tabId && document_->revision()==task.revision
        && document_->authored()==task.base && session_==task.retainedSession
        && meshExportOperation_.value("id")==task.sessionId && meshExportOperation_.value("evaluationStatus")=="pending"
        && geometryEvaluationGeneration_==task.generation;
}

QJsonObject AlloyWindow::cancelMeshExport() {
    const auto task=geometryEvaluationRunning_;
    if(!task || (task->command!="document.exportStl" && task->command!="document.export3mf" && task->command!="document.exportObj"))return {{"ok",false},{"error",QJsonObject{{"code","invalid_session"},{"message","Export is no longer running"}}}};
    const QString label=task->command=="document.export3mf" ? "3MF" : task->command=="document.exportObj" ? "OBJ" : "STL";
    if(!task->exportPublication->deny())return {{"ok",false},{"completion","failed"},{"error",QJsonObject{{"code","export_publication_started"},
        {"message",label+" publication was approved; wait for its final result"}}}};
    meshExportOperation_["evaluationStatus"]="failed";meshExportOperation_["evaluationPhase"]="draining";
    meshExportOperation_["previewError"]=QJsonObject{{"code","export_cancelled"},{"message",label+" export cancelled; destination preserved"}};
    meshExportOperation_["resultDiscarded"]=true;
    if(meshExportCancel_)meshExportCancel_->setEnabled(false);
    statusBar()->showMessage("Finishing background export; destination preserved. Modeling resumes when finished.");
    refreshMeshExportDialog();
    return {{"ok",true},{"completion","completed"},{"result",QJsonObject{{"resultDiscarded",true},{"evaluationStillRunning",true}}}};
}

void AlloyWindow::pollMeshExport() {
    const auto task=geometryEvaluationRunning_;if(!task || (task->command!="document.exportStl" && task->command!="document.export3mf" && task->command!="document.exportObj"))return;
    const QString label=task->command=="document.export3mf" ? "3MF" : task->command=="document.exportObj" ? "OBJ" : "STL";
    auto heartbeat=meshExportOperation_.value("evaluationHeartbeat").toObject();const auto now=QDateTime::currentMSecsSinceEpoch();
    heartbeat["ticks"]=heartbeat.value("ticks").toInteger()+1;
    heartbeat["maxGapMs"]=std::max(heartbeat.value("maxGapMs").toInteger(),now-heartbeat.value("lastMs").toInteger());
    heartbeat["lastMs"]=now;meshExportOperation_["evaluationHeartbeat"]=heartbeat;
    // Only value-only mailbox state is read until worker completion. Never read
    // its shapes/results while it is preparing or awaiting publication approval.
    if(task->exportPublication->state()==MeshExport::PublicationGate::State::AwaitingApproval){
        if(!meshExportCurrent(*task)){
            task->exportPublication->deny();
            if(meshExportOperation_.value("evaluationStatus")=="pending"){
                meshExportOperation_["evaluationStatus"]="failed";
                meshExportOperation_["previewError"]=QJsonObject{{"code","stale_export"},{"message","The design changed; "+label+" destination preserved"}};
                meshExportOperation_["resultDiscarded"]=true;
            }
        }else if(task->exportPublication->approve()){
            meshExportOperation_["evaluationPhase"]="publishing";
            meshExportOperation_["cancellation"]="Publication approved; wait for the final result";
            if(meshExportCancel_)meshExportCancel_->setEnabled(false);
            statusBar()->showMessage("Publishing "+label+"… Wait for the final save result.");
        }
    }
    refreshMeshExportDialog();
}

void AlloyWindow::completeMeshExport(const std::shared_ptr<GeometryEvaluation>& task) {
    // After approval the worker's actual atomic file commit decides success.
    // Before approval stale requests are denied, including on owner teardown.
    if(meshExportOperation_.value("id")==task->sessionId){
        const QString label=task->command=="document.export3mf" ? "3MF" : task->command=="document.exportObj" ? "OBJ" : "STL";
        auto timing=meshExportOperation_.value("evaluationTimingMs").toObject();timing["workerAndApprovalWait"]=task->fullMs;
        timing["workerPreparation"]=task->sourceMs;timing["workerPublication"]=task->stageMs;
        meshExportOperation_["evaluationTimingMs"]=timing;
        if(meshExportOperation_.value("evaluationStatus")=="pending"){
            auto error=task->exportError;
            if(!task->error.isEmpty())error={{"code","export_failed"},{"message",task->error}};
            if(!error.isEmpty() || task->exportResult.isEmpty()){
                meshExportOperation_["evaluationStatus"]="failed";
                meshExportOperation_["previewError"]=error.isEmpty() ? QJsonObject{{"code","export_failed"},{"message",label+" writer returned no result"}} : error;
            }else{
                for(auto it=task->exportResult.begin();it!=task->exportResult.end();++it)meshExportOperation_[it.key()]=it.value();
                meshExportOperation_["evaluationStatus"]="completed";
            }
        }
        meshExportOperation_["evaluationPhase"]="finished";
    }
    for(const auto& widget:meshExportFrozenControls_)if(widget)widget->setEnabled(true);meshExportFrozenControls_.clear();
    if(meshExportCancel_){statusBar()->removeWidget(meshExportCancel_);meshExportCancel_->deleteLater();meshExportCancel_=nullptr;}
    status_->show();statusBar()->showMessage(meshExportOperation_.value("evaluationStatus")=="completed"
        ? (task->command=="document.export3mf" ? "3MF saved." : task->command=="document.exportObj" ? "OBJ saved." : "STL saved.") :
        meshExportOperation_.value("previewError").toObject().value("message").toString(),6000);
    refreshMeshExportDialog();
}

void AlloyWindow::refreshMeshExportDialog() {
    auto* dialog=dynamic_cast<MeshExportDialog*>(findChild<QDialog*>("mesh.export.dialog"));
    if(!dialog || dialog->property("exportOperation").toString()!=meshExportOperation_.value("id").toString())return;
    const bool running=geometryEvaluationRunning_ &&
        (geometryEvaluationRunning_->command=="document.exportStl" || geometryEvaluationRunning_->command=="document.export3mf" || geometryEvaluationRunning_->command=="document.exportObj");
    const QString formatLabel=meshExportOperation_.value("command")=="document.export3mf" ? "3MF" : meshExportOperation_.value("command")=="document.exportObj" ? "OBJ" : "STL";
    const bool pending=meshExportOperation_.value("evaluationStatus")=="pending";
    for(auto* widget:dialog->findChildren<QWidget*>())
        if(qobject_cast<QComboBox*>(widget) || qobject_cast<QListWidget*>(widget) || qobject_cast<QDoubleSpinBox*>(widget) || qobject_cast<QLineEdit*>(widget))widget->setEnabled(!running);
    if(auto* browse=dialog->findChild<QPushButton*>("mesh.export.browse"))browse->setEnabled(!running);
    auto* buttons=dialog->findChild<QDialogButtonBox*>();buttons->button(QDialogButtonBox::Save)->setEnabled(!running);
    buttons->button(QDialogButtonBox::Cancel)->setEnabled(!running || (pending && meshExportOperation_.value("evaluationPhase")!="publishing"));
    auto* label=dialog->findChild<QLabel*>("mesh.export.status");
    if(running){label->setText(pending ? (meshExportOperation_.value("evaluationPhase")=="publishing" ? "Publishing "+formatLabel+"…" : "Preparing "+formatLabel+"… You can Cancel before publication.") : "Finishing cancelled export…");}
    else {
        dialog->pendingCancel={};dialog->setProperty("exportOperation",QString{});
        if(meshExportOperation_.value("evaluationStatus")=="completed")dialog->accept();
        else label->setText(meshExportOperation_.value("previewError").toObject().value("message").toString());
    }
}


bool AlloyWindow::meshPreviewCurrent(const MeshPreviewWork& work) const {
    if(document_!=work.document || activeTabId_!=work.tabId || document_->revision()!=work.revision
        || document_->authored().value("documentId").toString()!=work.documentId || document_->authored()!=work.base
        || work.generation!=meshPreviewGeneration_ || selectedOccurrence_+":"+selectedBody_!=work.selectionKey || !session_.isEmpty()
        || !sliderPreviewAuthored_.isEmpty() || !jointGraphStage_.isEmpty() || transientSliderPosition_.has_value() || pendingReferenceEvaluation_)return false;
    try {const auto selected=ExportScope::select(document_->authored(),presentationBodies(),work.settings);
        return selected.bodies.size()==1 && selected.bodies.front().occurrenceId+":"+selected.bodies.front().bodyId==work.instanceKey;
    }catch(...){return false;}
}
QJsonObject AlloyWindow::clearMeshPreview(const QString& id) {
    if(!id.isEmpty() && (!meshPreviewWork_ || meshPreviewWork_->id!=id))return {{"ok",false},{"error",QJsonObject{{"code","invalid_preview"},{"message","Mesh preview is no longer current"}}}};
    ++meshPreviewGeneration_;if(meshPreviewWork_ && !meshPreviewWork_->ready.load(std::memory_order_acquire))meshPreviewDraining_=meshPreviewWork_;meshPreviewWork_.reset();
    if(meshPreviewPoll_)meshPreviewPoll_->stop();view_->setMeshExportPreview({});view_->setProperty("meshPreviewToken",QString{});
    const auto draining=meshPreviewDraining_.lock();
    return {{"cleared",true},{"authoredChanged",false},{"evaluationStillRunning",draining && !draining->ready.load(std::memory_order_acquire)}};
}
QJsonObject AlloyWindow::previewMesh(const QJsonObject& args) {
    auto fail=[](const QString& code,const QString& message){return QJsonObject{{"ok",false},{"error",QJsonObject{{"code",code},{"message",message}}}};};
    if(!session_.isEmpty())return fail("active_command","Finish or Cancel the active command before Preview");
    if(!sliderPreviewAuthored_.isEmpty() || !jointGraphStage_.isEmpty() || transientSliderPosition_.has_value() || pendingReferenceEvaluation_)return fail("staged_pose","Finish or revert staged motion before Preview");
    if(const auto draining=meshPreviewDraining_.lock();draining && !draining->ready.load(std::memory_order_acquire))return fail("evaluation_busy","Cancelled mesh preparation is still draining; retry Preview when finished");
    if(meshPreviewWork_ && !meshPreviewWork_->ready.load(std::memory_order_acquire))return fail("evaluation_busy","Mesh preparation is running; Cancel before restarting Preview");
    clearMeshPreview();
    const auto format=args.value("format").toString(),units=args.value("units").toString();
    const double deviation=args.value("surfaceDeviationMm").toDouble(),angle=args.value("normalDeviationDeg").toDouble();
    if(format!="binary" && format!="ascii" && format!="3mf")return fail("invalid_format","Choose binary/ASCII STL or Core 3MF");
    if((format=="3mf" && units!="mm") || (units!="mm" && units!="in"))return fail("invalid_units","Core 3MF uses mm; STL supports mm or in");
    if(!std::isfinite(deviation) || deviation<.001 || deviation>10 || !std::isfinite(angle) || angle<1 || angle>45)return fail("invalid_mesh_settings","Surface deviation must be 0.001–10 mm; normal deviation 1–45 degrees");
    const auto preset=args.value("preset").toString("Custom");
    if(preset!="Low" && preset!="Medium" && preset!="High" && preset!="Custom")return fail("invalid_preset","Choose Low, Medium, High or Custom");
    const double presetDeviation=preset=="Low" ? .5 : preset=="High" ? .02 : .1;
    const double presetAngle=preset=="Low" ? 25 : preset=="High" ? 5 : 10;
    if(preset!="Custom" && (deviation!=presetDeviation || angle!=presetAngle))return fail("invalid_preset","Preset tolerances must match the explicit Alloy values");
    try {
        const auto selected=ExportScope::select(document_->authored(),presentationBodies(),args);
        if(selected.bodies.size()!=1)return fail("single_solid_required","Preview supports exactly one committed body instance");
        const auto& body=selected.bodies.front();
        if(body.summary.value("type")=="surface" && args.value("bodyId").toString().isEmpty())return fail("surface_scope_unsupported","Open-surface STL requires one explicit bodyId");
        BRepBuilderAPI_Copy copy(body.shape,true,false);
        if(!copy.IsDone())return fail("export_failed","Cannot detach preview geometry");
        EvaluatedBody detached;detached.shape=copy.Shape();detached.bodyId=body.bodyId;detached.name=body.name;detached.occurrenceId=body.occurrenceId;detached.definitionId=body.definitionId;detached.summary=body.summary;
        gp_Trsf placement;
        for(const auto& value:document_->authored().value("occurrences").toArray())if(value.toObject().value("id")==body.occurrenceId)placement=LocalOccurrences::worldTransform(document_->authored(),body.occurrenceId);
        auto work=std::make_shared<MeshPreviewWork>();
        work->document=document_;work->documentId=document_->authored().value("documentId").toString();work->tabId=activeTabId_;work->revision=document_->revision();work->base=document_->authored();
        work->selectionKey=selectedOccurrence_+":"+selectedBody_;work->generation=meshPreviewGeneration_;work->id=CadDocument::newId();work->instanceKey=body.occurrenceId+":"+body.bodyId;work->settings=args;work->settings["preset"]=preset;
        meshPreviewWork_=work;
        std::thread([work,detached=std::move(detached),placement,format,units,deviation,angle]{
            try{work->artifact=MeshExport::prepare(detached,placement,format,units,deviation,angle);}
            catch(const GeometryKernel::EvaluationError& error){work->error=error.diagnostics();}
            catch(const std::exception& error){work->error={{"code","export_failed"},{"message",QString::fromUtf8(error.what())}};}
            catch(...){work->error={{"code","export_failed"},{"message","Mesh preparation failed"}};}
            work->ready.store(true,std::memory_order_release);
        }).detach();
        if(!meshPreviewPoll_){meshPreviewPoll_=new QTimer(this);meshPreviewPoll_->setInterval(40);connect(meshPreviewPoll_,&QTimer::timeout,this,&AlloyWindow::pollMeshPreview);}
        meshPreviewPoll_->start();return meshPreviewState();
    }catch(const GeometryKernel::EvaluationError& error){return {{"ok",false},{"error",error.diagnostics()}};}
}
QJsonObject AlloyWindow::meshPreviewState(bool includeBytes) {
    if(!meshPreviewWork_){const auto draining=meshPreviewDraining_.lock();return {{"evaluationStatus","uncomputed"},{"evaluationStillRunning",draining && !draining->ready.load(std::memory_order_acquire)}};}
    const auto work=meshPreviewWork_;
    if(!meshPreviewCurrent(*work)){clearMeshPreview();return {{"evaluationStatus","invalidated"}};}
    QJsonObject state{{"previewId",work->id},{"id",work->id},{"previewGeneration",qint64(work->generation)},
        {"documentId",work->documentId},{"tabId",work->tabId},{"sourceDocumentRevision",qint64(work->revision)},
        {"instanceKey",work->instanceKey},{"settings",work->settings},{"presetPolicy","Alloy numeric presets; public preset values unknown"},
        {"validRanges",QJsonObject{{"surfaceDeviationMm",QJsonArray{.001,10}},{"normalDeviationDeg",QJsonArray{1,45}}}},
        {"presets",QJsonObject{{"Low",QJsonObject{{"surfaceDeviationMm",.5},{"normalDeviationDeg",25}}},{"Medium",QJsonObject{{"surfaceDeviationMm",.1},{"normalDeviationDeg",10}}},{"High",QJsonObject{{"surfaceDeviationMm",.02},{"normalDeviationDeg",5}}}}}};
    if(!work->ready.load(std::memory_order_acquire)){state["evaluationStatus"]="pending";return state;}
    if(!work->error.isEmpty()){state["evaluationStatus"]="failed";state["error"]=work->error;return state;}
    state["evaluationStatus"]="completed";
    if(view_->property("meshPreviewToken").toString()!=work->id){view_->setMeshExportPreview(work->artifact->facetsMm,work->instanceKey);view_->setProperty("meshPreviewToken",work->id);}
    for(auto it=work->artifact->metadata.begin();it!=work->artifact->metadata.end();++it)state[it.key()]=it.value();
    if(includeBytes)state["bytesBase64"]=QString::fromLatin1(work->artifact->bytes.toBase64());
    return state;
}
void AlloyWindow::pollMeshPreview() {
    const auto state=meshPreviewState();
    if(state.value("evaluationStatus")!="pending") {
        if(meshPreviewPoll_)meshPreviewPoll_->stop();

    }
}
QJsonObject AlloyWindow::saveMeshPreview(const QString& id,const QString& path) {
    const auto state=meshPreviewState();
    if(!meshPreviewWork_ || meshPreviewWork_->id!=id || state.value("evaluationStatus")!="completed")return {{"ok",false},{"error",QJsonObject{{"code","invalid_preview"},{"message","Preview the current solid and settings before Save"}}}};
    try {auto result=MeshExport::publish(*meshPreviewWork_->artifact,path);result["previewId"]=id;return result;}
    catch(const GeometryKernel::EvaluationError& error){return {{"ok",false},{"error",error.diagnostics()}};}
}
