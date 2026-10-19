#include "AlloyWindow.h"
#include "LocalOccurrences.h"
#include <BRepBuilderAPI_Copy.hxx>
#include <QDockWidget>
#include <QTabBar>
#include <QStatusBar>
#include <QElapsedTimer>
#include <QDateTime>
#include <algorithm>

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
    if(document_->bodies().empty())return error("solid_required","Create or import a solid first");
    if(auto* existing=findChild<QDialog*>("mesh.export.dialog")){existing->raise();existing->activateWindow();return QJsonObject{{"visible",true}};}
    auto* dialog=new MeshExportDialog(this);dialog->setObjectName("mesh.export.dialog");dialog->setWindowTitle("Save As Mesh");
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
    if(linked && !bodyId.isEmpty() && bodies->findData(bodyId)<0)bodies->setCurrentIndex(-1);
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
    format->addItem("STL (Binary)","binary");format->addItem("STL (ASCII)","ascii");
    format->addItem("3MF","3mf");format->addItem("OBJ","obj");form->addRow("Format",format);
    auto* units=new QComboBox;units->setObjectName("mesh.export.units");
    units->addItem("Millimeters","mm");units->addItem("Inches","in");form->addRow("Unit Type",units);
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
            ? "3MF uses millimeters and keeps selected bodies as separate placed objects.\nComponent selections include visible bodies."
            : format->currentData()=="obj" ? "One file · separate body groups · no materials.\nZ-up world coordinates; OBJ does not store units. Use matching import units."
            : "Custom refinement · separate bodies retain their placement.\nComponent selections include visible bodies.\nSTL files do not store units. Use the same units when importing.");
    };
    connect(format,&QComboBox::currentIndexChanged,dialog,updateFormat);updateFormat();
    auto* status=new QLabel;status->setObjectName("mesh.export.status");status->setWordWrap(true);layout->addWidget(status);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Save|QDialogButtonBox::Cancel);layout->addWidget(buttons);
    connect(buttons,&QDialogButtonBox::rejected,dialog,&QDialog::reject);
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
        auto path=QFileDialog::getSaveFileName(dialog,"Save As Mesh",bodies->currentText()+suffix,
            threeMf ? "3MF mesh (*.3mf)" : obj ? "OBJ mesh (*.obj)" : "STL mesh (*.stl)");
        if(path.isEmpty())return;
        if(document_->authored().value("documentId").toString()!=documentId || document_->revision()!=revision) {
            status->setText("The design changed. Close this window and restart Save As Mesh.");return;
        }
        if(!path.endsWith(suffix,Qt::CaseInsensitive))path+=suffix;
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
    dialog->resize(430,380);dialog->show();
    QJsonObject state{{"visible",true}};
    if(bodies->currentData()=="visible")state["allVisible"]=true;
    else if(bodies->currentData()=="selected")state["bodyIds"]=bodyIds;
    else if(bodies->currentData().toString().startsWith("occurrence:"))state["occurrenceId"]=bodies->currentData().toString().sliced(11);
    else state["bodyId"]=bodies->currentData().toString();
    return state;
}

QJsonObject AlloyWindow::beginMeshExport(const std::vector<EvaluatedBody>& bodies,const QJsonObject& inputs,const QString& label,const QString& format) {
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
            task.exportResult=threeMf
                ? MeshExport::write3mf(task.exportBodies,task.exportTransforms,task.inputs.value("path").toString(),
                    task.inputs.value("surfaceDeviationMm").toDouble(),task.inputs.value("normalDeviationDeg").toDouble(),approve)
                : obj ? MeshExport::writeObj(task.exportBodies,task.inputs.value("path").toString(),task.inputs.value("units").toString(),
                    task.inputs.value("surfaceDeviationMm").toDouble(),task.inputs.value("normalDeviationDeg").toDouble(),approve)
                : MeshExport::writeStl(task.exportBodies,task.inputs.value("path").toString(),"mm",true,
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
        if(qobject_cast<QComboBox*>(widget) || qobject_cast<QListWidget*>(widget) || qobject_cast<QDoubleSpinBox*>(widget))widget->setEnabled(!running);
    auto* buttons=dialog->findChild<QDialogButtonBox*>();buttons->button(QDialogButtonBox::Save)->setEnabled(!running);
    buttons->button(QDialogButtonBox::Cancel)->setEnabled(!running || (pending && meshExportOperation_.value("evaluationPhase")!="publishing"));
    auto* label=dialog->findChild<QLabel*>("mesh.export.status");
    if(running){label->setText(pending ? (meshExportOperation_.value("evaluationPhase")=="publishing" ? "Publishing "+formatLabel+"…" : "Preparing "+formatLabel+"… You can Cancel before publication.") : "Finishing cancelled export…");}
    else {
        dialog->pendingCancel={};
        if(meshExportOperation_.value("evaluationStatus")=="completed")dialog->accept();
        else label->setText(meshExportOperation_.value("previewError").toObject().value("message").toString());
    }
}
