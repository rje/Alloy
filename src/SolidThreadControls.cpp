#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <BRepAdaptor_Surface.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <Prs3d_Drawer.hxx>
#include <StdPrs_ToolTriangulatedShape.hxx>
#include <Standard_Failure.hxx>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QElapsedTimer>
#include <QDebug>
#include <QLabel>
#include <QPushButton>
#include <QShortcut>
#include <QSignalBlocker>
#include <QTimer>
#include <cmath>
#include <thread>

namespace {
QJsonObject threadFailure(const QString& code,const QString& message) {
    return {{"ok",false},{"completion","failed"},{"error",QJsonObject{{"code",code},{"message",message}}}};
}
void traceThreadPhase(const char* phase,const QElapsedTimer& timer) {
    if(qEnvironmentVariableIsSet("ALLOY_TRACE_THREAD"))qInfo().noquote()<<"thread_phase_ms"<<phase<<timer.nsecsElapsed()/1e6;
}
double coarsePitch(double d) {return d==6 ? 1 : d==10 ? 1.5 : 1.25;}
}
struct AlloyWindow::ThreadEvaluation {
    std::atomic<bool> ready{false};
    CadDocument* document=nullptr; // Identity only; the worker never accesses the document.
    QString sessionId;
    bool autoCommit=false;
    quint64 generation=0, revision=0;
    QJsonObject base, candidate, inputs;
    CadDocument::CandidateEvaluation full, stage, source;
    QString error;
    std::vector<TopoDS_Shape> preparedMeshes;
};

QJsonObject AlloyWindow::beginThread(const QJsonObject& overrides,bool direct) {
    if(!session_.isEmpty())return threadFailure("session_busy","Complete or cancel the active command first");
    QJsonObject inputs{{"name","Thread"},{"nominalDiameterMm",8.0},{"pitchMm",1.25},
        {"startOffsetMm",0.0},{"lengthMm",10.0},{"fullLength",true},{"direction","right"},
        {"modeled",true},{"threadSide","external"},{"standard","ISO_metric_coarse"}};
    if(overrides.contains("featureId")) {
        bool found=false;
        for(const auto& entry:document_->authored().value("features").toArray()) {
            const auto feature=entry.toObject();if(feature.value("id")!=overrides.value("featureId"))continue;
            if(feature.value("type")!="thread")return threadFailure("stale_entity","Thread feature is unavailable");
            inputs=feature.value("parameters").toObject();inputs.remove("geometryVersion");inputs["featureId"]=feature.value("id");inputs["name"]=feature.value("name");found=true;break;
        }
        if(!found)return threadFailure("stale_entity","Thread feature is unavailable");
    } else if(!selectedFace_.isEmpty())inputs["support"]=selectedFace_;
    for(auto it=overrides.begin();it!=overrides.end();++it)inputs[it.key()]=it.value();
    // Inspect only the existing analytic face; sweep/Boolean work stays on the worker.
    const auto support=inputs.value("support").toObject();
    for(const auto& body:document_->bodies())if(body.bodyId==support.value("bodyId"))
        for(const auto& face:body.supportFaces)if(face.faceId==support.value("faceId")) {
            BRepAdaptor_Surface cylinder(face.face);if(cylinder.GetType()!=GeomAbs_Cylinder)continue;
            const double diameter=2*cylinder.Cylinder().Radius();
            double nearest=6;for(double size:{8.0,10.0})if(std::abs(size-diameter)<std::abs(nearest-diameter))nearest=size;
            if(!overrides.contains("nominalDiameterMm") && !inputs.contains("featureId"))inputs["nominalDiameterMm"]=nearest;
            if(!overrides.contains("pitchMm") && !inputs.contains("featureId"))inputs["pitchMm"]=coarsePitch(inputs.value("nominalDiameterMm").toDouble());
            if(!overrides.contains("lengthMm") && !inputs.contains("featureId"))inputs["lengthMm"]=std::abs(cylinder.LastVParameter()-cylinder.FirstVParameter());
        }
    session_={{"id",CadDocument::newId()},{"command","solid.thread"},{"status","active"},{"inputs",inputs},
        {"newFeatureId",inputs.value("newFeatureId").toString(CadDocument::newId())},
        {"baseDocumentRevision",static_cast<qint64>(document_->revision())},{"previewed",false},{"geometryEvaluated",false},
        {"inputSchema",findCommand("solid.thread")->parameters},{"autoCommit",direct}};
    session_["featureId"]=inputs.contains("featureId") ? inputs.value("featureId") : session_.value("newFeatureId");
    if(overrides.contains("lengthMm") && !overrides.contains("fullLength")) {inputs["fullLength"]=false;session_["inputs"]=inputs;}
    if(!direct){showThread();view_->setFocus();}
    const auto result=previewThread();
    if(result.value("ok").isBool() && !result.value("ok").toBool()) {
        if(direct){session_={};return result;}
        return session_;
    }
    return result;
}
QJsonObject AlloyWindow::selectThreadFace(const QJsonObject& support) {
    if(session_.value("command")!="solid.thread")return threadFailure("invalid_session","Thread is not active");
    if(!support.value("occurrenceId").toString().isEmpty())return threadFailure("unsupported_instance_edit","Select the source body for Thread");
    for(const auto& body:document_->bodies())if(body.bodyId==support.value("bodyId"))
        for(const auto& face:body.supportFaces)if(face.faceId==support.value("faceId") && face.producerFeatureId==support.value("producerFeatureId")) {
            BRepAdaptor_Surface cylinder(face.face);if(cylinder.GetType()!=GeomAbs_Cylinder)return threadFailure("cylindrical_face_required","Select an external cylindrical boss face");
            const double diameter=2*cylinder.Cylinder().Radius();double size=6;
            for(double candidate:{8.0,10.0})if(std::abs(candidate-diameter)<std::abs(size-diameter))size=candidate;
            selectedFace_=support;view_->selectSupportFace(support,face.face);
            return execute("command.update",{{"session",session_.value("id")},{"inputs",QJsonObject{{"support",support},
                {"nominalDiameterMm",size},{"pitchMm",coarsePitch(size)},{"lengthMm",std::abs(cylinder.LastVParameter()-cylinder.FirstVParameter())},
                {"fullLength",session_.value("inputs").toObject().value("fullLength")}}}});
        }
    return threadFailure("support_unavailable","The selected cylindrical face is unavailable");
}
QJsonObject AlloyWindow::previewThread() {
    if(session_.value("command")!="solid.thread")return threadFailure("invalid_session","Thread is not active");
    const auto inputs=session_.value("inputs").toObject();
    if((session_.value("evaluationStatus")=="pending" || session_.value("evaluationStatus")=="completed")
        && session_.value("requestedInputs")==inputs)return session_;
    ++threadGeneration_;threadPrepared_.reset();threadQueued_.reset();
    session_["previewGeneration"]=static_cast<qint64>(threadGeneration_);session_["previewed"]=false;session_["geometryEvaluated"]=false;
    session_.remove("geometry");session_.remove("documentDraft");session_.remove("previewError");
    auto reject=[this](const QString& code,const QString& message){auto result=threadFailure(code,message);session_["evaluationStatus"]="failed";session_["previewError"]=result.value("error");refreshThreadControls();return result;};
    if(session_.value("baseDocumentRevision").toInteger()!=static_cast<qint64>(document_->revision()))return reject("stale_session","The document changed; cancel and restart Thread");
    if(inputs.value("support").toObject().isEmpty())return reject("face_required","Select one external cylindrical boss face");
    QJsonObject args=inputs;if(!args.contains("featureId"))args["newFeatureId"]=session_.value("newFeatureId");
    auto task=std::make_shared<ThreadEvaluation>();task->document=document_;task->sessionId=session_.value("id").toString();
    task->generation=threadGeneration_;task->revision=document_->revision();task->base=document_->authored();task->inputs=inputs;task->autoCommit=session_.value("autoCommit").toBool();
    try{task->candidate=CadDocument::threadCandidate(task->base,authoringArguments(args));}
    catch(const std::exception& error){return reject("invalid_arguments",QString::fromUtf8(error.what()));}
    session_["requestedInputs"]=inputs;session_["evaluationStatus"]="pending";session_["evaluationPhase"]="preview";
    threadQueued_=task;
    if(!threadPollTimer_){threadPollTimer_=new QTimer(this);threadPollTimer_->setInterval(30);connect(threadPollTimer_,&QTimer::timeout,this,[this]{pollThreadEvaluation();});}
    threadPollTimer_->start();refreshThreadControls();pollThreadEvaluation();return session_;
}
void AlloyWindow::pollThreadEvaluation() {
    if(threadRunning_ && threadRunning_->ready.load(std::memory_order_acquire)) {
        const auto task=threadRunning_;threadRunning_.reset();
        if(document_==task->document && document_->revision()==task->revision && session_.value("id")==task->sessionId
            && threadGeneration_==task->generation && session_.value("inputs")==task->inputs) {
            if(!task->error.isEmpty() || !task->full.ok || !task->stage.ok) {
                const auto message=!task->error.isEmpty() ? task->error : !task->full.ok ? task->full.error : task->stage.error;
                session_["evaluationStatus"]="failed";session_["previewError"]=QJsonObject{{"code","evaluation_failed"},{"message",message},
                    {"diagnostics",task->full.diagnostics}};
                if(session_.value("autoCommit").toBool()) {
                    auto terminal=session_;terminal["status"]="failed";
                    finishSession(task->sessionId,false);
                    geometryEvaluationTerminal_=terminal;
                }
            } else {
                threadPrepared_=task;session_["evaluationStatus"]="completed";session_["previewed"]=true;session_["geometryEvaluated"]=true;
                session_["documentDraft"]=task->candidate;
                view_->setPreparedThreadMeshes(task->preparedMeshes);
                view_->selectSupportFace({},{});
                QElapsedTimer phase;phase.start();
                if(!task->autoCommit && task->source.ok)view_->setBodyStage(task->source.evaluation->bodies,"thread-source:"+task->sessionId);
                traceThreadPhase("ui_set_body_stage",phase);phase.restart();
                const auto bodyId=task->inputs.value("support").toObject().value("bodyId").toString();
                for(const auto& body:task->stage.evaluation->bodies)if(body.bodyId==bodyId){
                    if(!task->autoCommit)view_->showPreview(body.shape,body.bodyId,true);
                    session_["geometry"]=body.summary;break;
                }
                traceThreadPhase("ui_show_preview",phase);phase.restart();
                if(session_.value("autoCommit").toBool()) {
                    auto terminal=session_;const auto response=execute("command.commit",{{"session",task->sessionId}});
                    terminal["status"]=response.value("ok").toBool() ? "committed" : "failed";
                    terminal["commandResult"]=response;
                    if(!response.value("ok").toBool()) {
                        terminal["evaluationStatus"]="failed";terminal["previewError"]=response.value("error");
                        terminal["previewed"]=false;terminal["geometryEvaluated"]=false;
                        terminal.remove("documentDraft");terminal.remove("geometry");
                        finishSession(task->sessionId,false);
                    }
                    geometryEvaluationTerminal_=terminal;
                    traceThreadPhase("ui_execute_commit_inclusive",phase);
                }
            }
            QElapsedTimer refreshTimer;refreshTimer.start();
            refreshThreadControls();refresh();
            traceThreadPhase("ui_final_refresh",refreshTimer);
        }
    }
    if(!threadRunning_ && threadQueued_) {
        auto task=threadQueued_;threadQueued_.reset();threadRunning_=task;
        std::thread([task]{
            try {
                QElapsedTimer phase;phase.start();
                task->full=CadDocument::evaluateCandidate(task->candidate);task->stage=task->full;
                traceThreadPhase("worker_full_evaluation",phase);
                const auto edit=task->inputs.value("featureId").toString();
                if(task->full.ok && !edit.isEmpty()) {
                    auto prefix=task->candidate;QJsonArray before,through;
                    for(const auto& value:task->candidate.value("features").toArray()) {through.append(value);if(value.toObject().value("id")==edit)break;before.append(value);}
                    prefix["features"]=before;phase.restart();task->source=CadDocument::evaluateCandidate(prefix);
                    traceThreadPhase("worker_source_evaluation",phase);
                    prefix["features"]=through;phase.restart();if(prefix!=task->candidate)task->stage=CadDocument::evaluateCandidate(prefix);
                    traceThreadPhase("worker_stage_evaluation",phase);
                }
                if(task->full.ok && task->stage.ok) {
                    phase.restart();
                    const auto bodyId=task->inputs.value("support").toObject().value("bodyId").toString();
                    // evaluateCandidate builds detached geometry, exclusively owned by
                    // this task until ready is published. Never mesh live document shapes.
                    for(const auto& evaluated:{task->full.evaluation,task->stage.evaluation})
                        for(const auto& body:evaluated->bodies)if(body.bodyId==bodyId) {
                            bool prepared=false;for(const auto& shape:task->preparedMeshes)prepared|=shape.IsSame(body.shape);
                            if(prepared)continue;
                            Handle(Prs3d_Drawer) drawer=new Prs3d_Drawer;
                            drawer->SetDeviationCoefficient(0.0005);drawer->SetDeviationAngle(0.10);
                            const double deflection=StdPrs_ToolTriangulatedShape::GetDeflection(body.shape,drawer);
                            if(!std::isfinite(deflection) || deflection<=0)throw std::runtime_error("Thread display bounds are not finite");
                            BRepMesh_IncrementalMesh mesh(body.shape,deflection,false,0.10,false);
                            if(!mesh.IsDone() || !StdPrs_ToolTriangulatedShape::IsTessellated(body.shape,drawer))
                                throw std::runtime_error("Thread display tessellation is incomplete");
                            task->preparedMeshes.push_back(body.shape);
                            if(qEnvironmentVariableIsSet("ALLOY_TRACE_THREAD"))qInfo()<<"thread_prepared_mesh_deflection_mm"<<deflection;
                        }
                    traceThreadPhase("worker_display_mesh",phase);
                }
            } catch(const Standard_Failure& error){task->error=QString::fromUtf8(error.GetMessageString());}
            catch(const std::exception& error){task->error=QString::fromUtf8(error.what());}
            task->ready.store(true,std::memory_order_release);
        }).detach();
    }
    if(!threadRunning_ && threadPollTimer_)threadPollTimer_->stop();
}
QString AlloyWindow::commitThreadPreview() {
    if(!threadPrepared_ || !session_.value("previewed").toBool() || session_.value("evaluationStatus")!="completed")return "Wait for a valid Thread preview before accepting";
    const auto task=threadPrepared_;
    if(task->document!=document_ || task->sessionId!=session_.value("id") || task->inputs!=session_.value("inputs") || task->generation!=threadGeneration_)return "Thread inputs changed; preview again before accepting";
    QElapsedTimer phase;phase.start();
    const auto error=document_->commitThreadEvaluation(task->full,task->candidate,task->base,task->revision,"Thread");
    traceThreadPhase("ui_prepared_commit",phase);return error;
}
void AlloyWindow::refreshThreadControls() {
    const bool active=session_.value("command")=="solid.thread";
    if(threadEscape_)threadEscape_->setEnabled(active);
    if(!active || !threadDock_)return;
    const auto inputs=session_.value("inputs").toObject();
    if(auto* label=threadDock_->findChild<QLabel*>("threadSelection"))label->setText(inputs.value("support").toObject().isEmpty() ? "0 selected" : "1 cylindrical face selected");
    if(auto* label=threadDock_->findChild<QLabel*>("threadError"))label->setText(session_.value("evaluationStatus")=="pending" ? "Computing modeled thread… Cancel discards the result." : session_.value("previewError").toObject().value("message").toString());
    if(auto* label=threadDock_->findChild<QLabel*>("threadDesignation"))label->setText(QString("M%1 × %2").arg(inputs.value("nominalDiameterMm").toDouble()).arg(inputs.value("pitchMm").toDouble()));
    for(const auto& key:{QString("lengthMm"),QString("startOffsetMm")})if(auto* field=threadDock_->findChild<QDoubleSpinBox*>("thread"+key)){const QSignalBlocker block(field);field->setValue(inputs.value(key).toDouble());field->setEnabled(!inputs.value("fullLength").toBool());}
    if(auto* size=threadDock_->findChild<QComboBox*>("threadSize")){const QSignalBlocker block(size);size->setCurrentIndex(size->findData(inputs.value("nominalDiameterMm").toDouble()));}
    if(auto* full=threadDock_->findChild<QCheckBox*>("threadFullLength")){const QSignalBlocker block(full);full->setChecked(inputs.value("fullLength").toBool());}
    if(auto* direction=threadDock_->findChild<QComboBox*>("threadDirection")){const QSignalBlocker block(direction);direction->setCurrentIndex(direction->findData(inputs.value("direction").toString()));}
    if(auto* box=threadDock_->findChild<QDialogButtonBox*>())box->button(QDialogButtonBox::Ok)->setEnabled(session_.value("previewed").toBool() && session_.value("evaluationStatus")=="completed");
}
void AlloyWindow::showThread() {
    if(threadDock_){canvasWindow_->removeDockWidget(threadDock_);delete threadDock_;}
    threadDock_=new QDockWidget("Thread",canvasWindow_);threadDock_->setObjectName("threadDock");
    threadDock_->setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);
    auto* panel=new QWidget(threadDock_);threadDock_->setWidget(panel);auto* form=new QFormLayout(panel);
    auto* selection=new QPushButton("Select cylindrical face",panel);selection->setObjectName("threadFacePicker");form->addRow("Faces",selection);
    connect(selection,&QPushButton::clicked,panel,[this]{view_->setFocus();});
    auto* selected=new QLabel(panel);selected->setObjectName("threadSelection");form->addRow("Selection",selected);
    auto* modeled=new QCheckBox("Modeled",panel);modeled->setObjectName("threadModeled");modeled->setChecked(true);modeled->setEnabled(false);form->addRow(modeled);
    form->addRow("Thread Type",new QLabel("ISO Metric Coarse — nominal",panel));
    auto* size=new QComboBox(panel);size->setObjectName("threadSize");for(double value:{6.0,8.0,10.0})size->addItem(QString::number(value)+" mm",value);form->addRow("Size",size);
    auto* designation=new QLabel(panel);designation->setObjectName("threadDesignation");form->addRow("Designation",designation);
    auto* full=new QCheckBox("Full Length",panel);full->setObjectName("threadFullLength");form->addRow(full);
    auto update=[this](const QJsonObject& changes){const auto response=execute("command.update",{{"session",session_.value("id")},{"inputs",changes}});if(!response.value("ok").toBool())if(auto* label=threadDock_->findChild<QLabel*>("threadError"))label->setText(response.value("error").toObject().value("message").toString());};
    for(const auto& key:{QString("startOffsetMm"),QString("lengthMm")}){
        auto* field=new QDoubleSpinBox(panel);field->setObjectName("thread"+key);field->setDecimals(3);field->setRange(key=="lengthMm" ? 0.001 : 0,100000);field->setSuffix(" mm");field->setKeyboardTracking(false);
        form->addRow(key=="lengthMm" ? "Length" : "Offset",field);connect(field,&QDoubleSpinBox::valueChanged,panel,[update,key](double value){update({{key,value},{"fullLength",false}});});
    }
    auto* direction=new QComboBox(panel);direction->setObjectName("threadDirection");direction->addItem("Right hand","right");direction->addItem("Left hand","left");form->addRow("Direction",direction);
    auto* note=new QLabel("External M6, M8 and M10 nominal profiles. No tolerance class or print clearance compensation.",panel);note->setWordWrap(true);form->addRow(note);
    auto* status=new QLabel(panel);status->setObjectName("threadError");status->setWordWrap(true);form->addRow(status);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,panel);form->addRow(buttons);
    refreshThreadControls();
    connect(size,&QComboBox::currentIndexChanged,panel,[update,size](int){const double d=size->currentData().toDouble();update({{"nominalDiameterMm",d},{"pitchMm",coarsePitch(d)}});});
    connect(full,&QCheckBox::toggled,panel,[update](bool value){
        auto changes=QJsonObject{{"fullLength",value}};
        if(value)changes["startOffsetMm"]=0.0;
        update(changes);
    });
    connect(direction,&QComboBox::currentIndexChanged,panel,[update,direction](int){update({{"direction",direction->currentData().toString()}});});
    connect(buttons,&QDialogButtonBox::accepted,panel,[this,status]{const auto result=execute("command.commit",{{"session",session_.value("id")}});if(!result.value("ok").toBool())status->setText(result.value("error").toObject().value("message").toString());});
    auto cancel=[this]{if(session_.value("command")=="solid.thread")execute("command.cancel",{{"session",session_.value("id")}});};
    connect(buttons,&QDialogButtonBox::rejected,panel,cancel);
    auto* escape=new QShortcut(QKeySequence(Qt::Key_Escape),panel);escape->setContext(Qt::WidgetWithChildrenShortcut);connect(escape,&QShortcut::activated,panel,cancel);
    if(!threadEscape_){threadEscape_=new QShortcut(QKeySequence(Qt::Key_Escape),view_);threadEscape_->setContext(Qt::WidgetShortcut);connect(threadEscape_,&QShortcut::activated,this,cancel);}
    threadEscape_->setEnabled(true);canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,threadDock_);threadDock_->show();
}
