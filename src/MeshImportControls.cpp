// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "MeshImport.h"
#include "ReferenceView.h"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QFile>
#include <QRegularExpression>
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
#include <QProgressBar>
#include <QDateTime>
#include <QElapsedTimer>
#include <thread>

namespace {
QJsonObject parameter(const QString& type,const QString& description){return {{"type",type},{"description",description}};}
QJsonObject failure(const QString& code,const QString& message){return {{"ok",false},{"error",QJsonObject{{"code",code},{"message",message}}}};}
const QStringList phases{"starting","reading","preflight","decoding","checking decoder","retaining coordinates","quality analysis","scaling","validating","preview preparation","prepared"};
QJsonObject importFailure(const QString& message) {return {{"ok",false},{"error",QJsonObject{{"code","mesh_import_failed"},{"message",message}}}};}
}
QJsonObject AlloyWindow::meshImportState() const {
    if(!meshImportDock_)return {};
    auto state=meshImportOperation_;state["inputs"]=meshImportInputs_;state["authored"]=false;state["loadingCancellable"]=true;
    if(meshImportPrepared_)state["preview"]=MeshImport::summary(meshImportCandidate_.value("meshes").toArray().last().toObject());
    if(meshImportWork_) {state["phase"]=phases.value(meshImportWork_->phase.load());state["completed"]=meshImportWork_->completed.load();state["total"]=meshImportWork_->total.load();}
    return state;
}
void AlloyWindow::closeMeshImport() {
    ++meshImportGeneration_;
    if(meshImportOperation_.value("status")!="committed") {meshImportOperation_["status"]="cancelled";meshImportOperation_["phase"]="cancelled";meshImportOperation_["resultDiscarded"]=true;}
    if(meshImportWork_) {
        meshImportWork_->cancelled.store(true);meshImportDraining_=meshImportWork_;meshImportWork_.reset();
        meshImportOperation_["status"]="cancelled";meshImportOperation_["phase"]="cancelled";meshImportOperation_["resultDiscarded"]=true;
    }
    view_->setMeshExportPreview({});
    if(meshImportDock_){auto* dock=meshImportDock_.data();meshImportDock_=nullptr;canvasWindow_->removeDockWidget(dock);dock->hide();dock->deleteLater();}
    meshImportBase_={};meshImportCandidate_={};meshImportInputs_={};meshImportObjSource_.reset();meshImportPrepared_.reset();
    view_->setFocus(Qt::OtherFocusReason);
    QTimer::singleShot(0,this,[this]{if(QGuiApplication::applicationState()==Qt::ApplicationActive && !QApplication::activeModalWidget())activateWindow();});
}
QJsonObject AlloyWindow::updateMeshImport(const QString& units) {
    if(meshImportWork_)return failure("import_pending","Wait for preparation or Cancel before changing units");
    if(units!="mm" && units!="in") {
        meshImportInputs_["units"]=units;meshImportPrepared_.reset();
        meshImportDock_->findChild<QPushButton*>("mesh.import.ok")->setEnabled(false);
        meshImportDock_->findChild<QLabel*>("mesh.import.error")->setText("Choose mm or in source units");
        return importFailure("Choose mm or in source units");
    }
    if(const auto draining=meshImportDraining_.lock();draining && !draining->ready.load(std::memory_order_acquire))return failure("import_busy","Cancelled import is still stopping; retry when it finishes");
    auto work=std::make_shared<MeshImportWork>();work->generation=++meshImportGeneration_;work->revision=meshImportBaseRevision_;
    work->tabId=meshImportTabId_;work->target=document_;work->documentId=document_->authored().value("documentId").toString();
    work->base=document_->authored();work->original=meshImportBase_;work->objSource=meshImportObjSource_;work->units=units;
    meshImportInputs_["units"]=units;meshImportPrepared_.reset();meshImportCandidate_={};meshImportWork_=work;
    const auto now=QDateTime::currentMSecsSinceEpoch();meshImportOperation_={{"status","pending"},{"phase","starting"},{"generation",double(work->generation)},{"startedMs",double(now)},{"heartbeat",QJsonObject{{"lastMs",double(now)},{"maxGapMs",0},{"samples",0}}}};
    auto* field=meshImportDock_->findChild<QComboBox*>("mesh.import.units");{const QSignalBlocker blocker(field);field->setCurrentIndex(field->findData(units));}field->setEnabled(false);
    meshImportDock_->findChild<QPushButton*>("mesh.import.ok")->setEnabled(false);
    meshImportDock_->findChild<QLabel*>("mesh.import.error")->clear();
    meshImportDock_->findChild<QLabel*>("mesh.import.summary")->setText("Loading mesh…");
    auto* progress=meshImportDock_->findChild<QProgressBar*>("mesh.import.progress");progress->setRange(0,0);progress->show();
    const auto path=meshImportInputs_.value("path").toString();
    // Owned immutable snapshots only. A detached draining task never accesses
    // this, widgets or a document, and can outlive their destruction safely.
    std::thread([work,path]{
        QElapsedTimer timer;timer.start();
        const auto report=[work](const QString& phase,int completed=0,int total=0){
            if(work->cancelled.load())throw std::runtime_error("Import cancelled");
            work->phase.store(phases.indexOf(phase));work->completed.store(completed);work->total.store(total);
        };
        try {
            if(work->original.isEmpty() && !work->objSource) {
                // The selected suffix is authoritative. For extensionless/local
                // renamed sources, recognize a bounded first OBJ record only;
                // the strict reader still validates the complete original file.
                bool obj=QFileInfo(path).suffix().compare("obj",Qt::CaseInsensitive)==0;
                if(QFileInfo(path).suffix().compare("stl",Qt::CaseInsensitive)!=0 && !obj) {
                    QFile probe(path);if(probe.open(QIODevice::ReadOnly)) {
                        const auto rows=QString::fromUtf8(probe.peek(4096)).split('\n');
                        for(auto row:rows) {row=row.section('#',0,0).trimmed();if(row.isEmpty())continue;
                            obj=QStringList{"v","vt","vn","f","o","g","s","mtllib","usemtl","l","p"}.contains(row.section(QRegularExpression("\\s+"),0,0));break;
                        }
                    }
                }
                if(obj)work->objSource=MeshImport::readObjSource(path);
                else work->original=MeshImport::readStl(path,"mm",report);
            }
            report("scaling");
            work->mesh=work->objSource ? MeshImport::readObjData(*work->objSource,path,work->units,report)
                : (work->units=="mm" ? work->original : MeshImport::withUnits(work->original,work->units));
            report("validating");work->prepared=CadDocument::prepareMeshImport(work->base,work->mesh);
            report("preview preparation");const auto vertices=work->mesh.value("verticesMm").toArray();const auto triangles=work->mesh.value("triangles").toArray();
            work->facets.reserve(triangles.size()*3);
            for(const auto& row:triangles){if(work->facets.size()%3072==0)report("preview preparation",work->facets.size()/3,triangles.size());for(const auto& index:row.toArray()){const auto p=vertices[index.toInt()].toArray();work->facets.emplace_back(p[0].toDouble(),p[1].toDouble(),p[2].toDouble());}}
            report("prepared",1,1);
        }catch(const MeshImport::Error& error){work->error=error.details;}
        catch(const std::exception& error){work->error={{"code","mesh_import_failed"},{"message",QString::fromUtf8(error.what())},{"filename",QFileInfo(path).fileName()},{"remedy","Choose a complete bounded STL or OBJ with mm or in units and retry."}};}
        work->workerMs=timer.elapsed();work->ready.store(true,std::memory_order_release);
    }).detach();
    QTimer::singleShot(20,this,[this,generation=work->generation]{if(meshImportWork_ && meshImportWork_->generation==generation)pollMeshImport();});
    return {{"status","pending"},{"loadingCancellable",true},{"authored",false}};
}
void AlloyWindow::pollMeshImport() {
    const auto work=meshImportWork_;if(!work)return;
    const auto now=QDateTime::currentMSecsSinceEpoch();auto heartbeat=meshImportOperation_.value("heartbeat").toObject();
    heartbeat["maxGapMs"]=std::max(heartbeat.value("maxGapMs").toDouble(),double(now)-heartbeat.value("lastMs").toDouble());heartbeat["lastMs"]=double(now);heartbeat["samples"]=heartbeat.value("samples").toInt()+1;meshImportOperation_["heartbeat"]=heartbeat;
    if(!meshImportDock_ || work->generation!=meshImportGeneration_ || work->target!=document_ || work->tabId!=activeTabId_ || work->documentId!=document_->authored().value("documentId").toString() || work->revision!=document_->revision() || work->base!=document_->authored()) {closeMeshImport();return;}
    auto* summary=meshImportDock_->findChild<QLabel*>("mesh.import.summary");auto* progress=meshImportDock_->findChild<QProgressBar*>("mesh.import.progress");
    summary->setText("Loading mesh…\n"+phases.value(work->phase.load()));const int total=work->total.load();progress->setRange(0,total);progress->setValue(work->completed.load());
    if(!work->ready.load(std::memory_order_acquire)){QTimer::singleShot(20,this,[this,generation=work->generation]{if(meshImportWork_ && meshImportWork_->generation==generation)pollMeshImport();});return;}
    meshImportOperation_["workerMs"]=double(work->workerMs);meshImportOperation_["phase"]="prepared";
    auto* units=meshImportDock_->findChild<QComboBox*>("mesh.import.units");units->setEnabled(true);progress->hide();meshImportWork_.reset();
    meshImportObjSource_=work->objSource;
    if(!work->error.isEmpty() || work->cancelled.load()) {
        meshImportOperation_["status"]="failed";meshImportOperation_["phase"]="failed";meshImportOperation_["error"]=work->error;view_->setMeshExportPreview({});
        summary->setText("Mesh could not be loaded");meshImportDock_->findChild<QLabel*>("mesh.import.error")->setText(work->error.value("filename").toString()+"\n"+work->error.value("message").toString()+"\n"+work->error.value("remedy").toString());return;
    }
    meshImportBase_=work->original;meshImportPrepared_=work->prepared;meshImportCandidate_=work->prepared->candidate();
    QElapsedTimer publication;publication.start();view_->setMeshExportPreview(work->facets);view_->fitMeshImport(work->mesh.value("summary").toObject().value("boundsMm").toObject());
    meshImportOperation_["publicationMs"]=double(publication.elapsed());meshImportOperation_["status"]="ready";meshImportOperation_["wallMs"]=double(QDateTime::currentMSecsSinceEpoch())-meshImportOperation_.value("startedMs").toDouble();
    const auto info=MeshImport::summary(work->mesh),bounds=info.value("boundsMm").toObject();
    auto text=[](const QJsonArray& a){return QString("%1, %2, %3").arg(a[0].toDouble(),0,'g',10).arg(a[1].toDouble(),0,'g',10).arg(a[2].toDouble(),0,'g',10);};
    QString details=QString("%1 triangles · %2 vertices\nBounds (mm)\nMin: %3\nMax: %4").arg(info.value("triangleCount").toInt()).arg(info.value("vertexCount").toInt()).arg(text(bounds.value("min").toArray()),text(bounds.value("max").toArray()));
    if(info.value("source").toObject().value("format")=="obj") {
        const auto diagnostics=info.value("diagnostics").toObject();
        const auto ignored=diagnostics.value("ignoredRecords").toObject();QStringList counts;
        for(auto it=ignored.begin();it!=ignored.end();++it)counts.append(it.key()+": "+QString::number(it.value().toInt()));
        const auto labels=diagnostics.value("sourceLabels").toArray();QStringList names;
        for(int i=0;i<std::min(3,int(labels.size()));++i) {const auto label=labels[i].toObject();auto name=label.value("name").toString();if(name.size()>40)name=name.left(39)+"…";names.append(label.value("kind").toString()+": "+name);}
        details+="\nIgnored records: "+(counts.isEmpty()?QString("none"):counts.join(", "));
        details+=QString("\nSource labels: %1").arg(labels.size());if(!names.isEmpty())details+="\n"+names.join("\n");if(labels.size()>3)details+=QString("\n… %1 more retained").arg(labels.size()-3);
        details+="\nMaterials, UVs and source normals are not applied.";
    }
    summary->setText(details);
    meshImportDock_->findChild<QPushButton*>("mesh.import.ok")->setEnabled(true);units->setFocus(Qt::OtherFocusReason);
}
void AlloyWindow::registerMeshImportCommands() {
    auto add=[this](QString id,QString title,QString description,QJsonObject schema,std::function<QJsonObject(const QJsonObject&)> run){commands_.push_back({id,title,description,schema,true,std::move(run)});};
    add("document.importMeshDialog","Import Mesh…","Choose one local STL or OBJ, preview its explicit mm/in interpretation, then OK or Cancel.",{},[this](const auto&){
        if(!session_.isEmpty() || meshImportDock_ || document_->isRolledBack() || !pendingReference_.isEmpty() || nonzeroAssemblyPose() || !sliderPreviewAuthored_.isEmpty() || !jointGraphStage_.isEmpty() || transientSliderPosition_.has_value() || pendingReferenceEvaluation_)
            return failure("active_command","Finish the command and return to the committed design before importing");
        if(auto* existing=findChild<QFileDialog*>("mesh.import.chooser")){existing->raise();return QJsonObject{{"visible",true}};}
        auto* chooser=new QFileDialog(this,"Import Mesh",{},"Mesh files (*.stl *.obj);;STL mesh (*.stl);;OBJ mesh (*.obj)");chooser->setObjectName("mesh.import.chooser");chooser->setFileMode(QFileDialog::ExistingFile);chooser->setAcceptMode(QFileDialog::AcceptOpen);chooser->setAttribute(Qt::WA_DeleteOnClose);chooser->setWindowModality(Qt::WindowModal);
        // AppKit may close its sheet with no editor key window. Restore the
        // owning editor as part of chooser closure, only while Alloy is active.
        connect(chooser,&QFileDialog::finished,this,[this]{QTimer::singleShot(0,this,[this]{
            if(QGuiApplication::applicationState()==Qt::ApplicationActive && !QApplication::activeModalWidget())activateWindow();
        });});
        connect(chooser,&QFileDialog::fileSelected,this,[this](const QString& path){
            // The native chooser closes before the panel is created. No event
            // pumping or model mutation occurs before the owned worker starts.
            QTimer::singleShot(0,this,[this,path]{const auto result=execute("mesh.importPreview",{{"path",QFileInfo(path).absoluteFilePath()}});if(!result.value("ok").toBool()) {
                const auto error=result.value("error").toObject();auto* message=new QMessageBox(QMessageBox::Warning,"Import Mesh",error.value("message").toString(),QMessageBox::Ok,this);message->setObjectName("mesh.import.failure");message->setInformativeText(QFileInfo(path).fileName()+"\n"+error.value("remedy").toString());message->setAttribute(Qt::WA_DeleteOnClose);message->setWindowModality(Qt::WindowModal);
                connect(message,&QDialog::finished,this,[this]{QTimer::singleShot(0,this,[this]{if(QGuiApplication::applicationState()==Qt::ApplicationActive && !QApplication::activeModalWidget())activateWindow();});});message->open();
            }});
        });
        chooser->open();return QJsonObject{{"visible",true},{"loadingCancellable",true}};
    });
    add("mesh.importPreview","Preview mesh import","Prepare one STL or OBJ in a cancellable worker; poll app.state.meshImport until ready or failed.",{{"path",parameter("string","Absolute local STL or OBJ source path.")}},[this](const auto& a){
        if(!session_.isEmpty() || meshImportDock_ || document_->isRolledBack() || !pendingReference_.isEmpty() || nonzeroAssemblyPose() || !sliderPreviewAuthored_.isEmpty() || !jointGraphStage_.isEmpty() || transientSliderPosition_.has_value() || pendingReferenceEvaluation_)return failure("active_command","Finish the active command before importing");
        if(const auto draining=meshImportDraining_.lock();draining && !draining->ready.load(std::memory_order_acquire))return failure("import_busy","Cancelled import is still stopping; retry when it finishes");
        meshImportBase_={};meshImportTabId_=activeTabId_;
        meshImportBaseRevision_=document_->revision();
        meshImportInputs_={{"path",a.value("path")},{"units","mm"}};
        auto* dock=new QDockWidget("Import Mesh",canvasWindow_);meshImportDock_=dock;dock->setObjectName("mesh.import.panel");dock->setFeatures(QDockWidget::NoDockWidgetFeatures);dock->setMinimumWidth(310);
        auto* panel=new QWidget;auto* layout=new QVBoxLayout(panel);auto* name=new QLabel(QFileInfo(a.value("path").toString()).fileName());name->setTextFormat(Qt::PlainText);name->setWordWrap(true);layout->addWidget(name);
        auto* form=new QFormLayout;auto* units=new QComboBox;units->setObjectName("mesh.import.units");units->setFocusPolicy(Qt::StrongFocus);units->addItem("Millimeter (mm)","mm");units->addItem("Inch (in)","in");form->addRow("Unit Type",units);layout->addLayout(form);
        auto* summary=new QLabel;summary->setObjectName("mesh.import.summary");summary->setTextFormat(Qt::PlainText);summary->setWordWrap(true);summary->setMinimumHeight(4*summary->fontMetrics().lineSpacing());summary->setAlignment(Qt::AlignLeft|Qt::AlignTop);layout->addWidget(summary);auto* progress=new QProgressBar;progress->setObjectName("mesh.import.progress");progress->setRange(0,0);layout->addWidget(progress);auto* message=new QLabel("Source coordinates are preserved.\nA mesh body is not an editable solid.");message->setWordWrap(true);layout->addWidget(message);
        auto* errorLabel=new QLabel;errorLabel->setObjectName("mesh.import.error");errorLabel->setTextFormat(Qt::PlainText);errorLabel->setWordWrap(true);layout->addWidget(errorLabel);layout->addStretch();
        auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);buttons->button(QDialogButtonBox::Ok)->setObjectName("mesh.import.ok");buttons->button(QDialogButtonBox::Cancel)->setObjectName("mesh.import.cancel");layout->addWidget(buttons);dock->setWidget(panel);canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,dock);dock->show();
        connect(units,&QComboBox::currentIndexChanged,this,[this,units]{execute("mesh.importUnits",{{"units",units->currentData().toString()}});});
        connect(buttons,&QDialogButtonBox::accepted,this,[this,errorLabel]{const auto r=execute("mesh.importAccept");if(!r.value("ok").toBool())errorLabel->setText(r.value("error").toObject().value("message").toString());});
        connect(buttons,&QDialogButtonBox::rejected,this,[this]{execute("mesh.importCancel");});auto* escape=new QShortcut(QKeySequence(Qt::Key_Escape),panel);escape->setContext(Qt::WidgetWithChildrenShortcut);connect(escape,&QShortcut::activated,this,[this]{execute("mesh.importCancel");});
        const auto result=updateMeshImport("mm");buttons->button(QDialogButtonBox::Cancel)->setFocus();return result;
    });
    add("mesh.importUnits","Import Mesh units","Update the ready preview from the original prepared coordinates without authored mutation.",{{"units",parameter("string","mm or in source interpretation.")}},[this](const auto& a){if(!meshImportDock_)return failure("invalid_session","No import preview is active");return updateMeshImport(a.value("units").toString());});
    add("mesh.importCancel","Cancel Import Mesh","Discard the ready preview, preserving the document and selection.",{},[this](const auto&){if(!meshImportDock_)return failure("invalid_session","No import preview is active");closeMeshImport();return QJsonObject{{"cancelled",true}};});
    add("mesh.importAccept","Accept Import Mesh","Commit the exact prepared STL or OBJ preview through the shared document import authority.",{},[this](const auto&){
        if(!meshImportDock_)return failure("invalid_session","No import preview is active");
        if(meshImportWork_)return failure("import_pending","Wait for preparation or Cancel");
        if(!meshImportPrepared_)return failure("invalid_preview","Load a valid mesh before OK");
        if(activeTabId_!=meshImportTabId_ || document_->revision()!=meshImportBaseRevision_)return failure("stale_preview","The design changed; Cancel and select the mesh again");
        if(meshImportCandidate_.value("meshes").toArray().last().toObject().value("source").toObject().value("units")!=meshImportInputs_.value("units"))return failure("invalid_preview","Choose valid source units before OK");
        QElapsedTimer commitTimer;commitTimer.start();meshImportCommitting_=true;const auto reset=qScopeGuard([this]{meshImportCommitting_=false;});const auto result=execute(meshImportCandidate_.value("meshes").toArray().last().toObject().value("source").toObject().value("format")=="obj" ? "document.importObj" : "document.importStl",meshImportInputs_);
        if(!result.value("ok").toBool())return result;meshImportOperation_["commitMs"]=double(commitTimer.elapsed());meshImportOperation_["status"]="committed";meshImportOperation_["phase"]="committed";closeMeshImport();return result.value("result").toObject();
    });
}
