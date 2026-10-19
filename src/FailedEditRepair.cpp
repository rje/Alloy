// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include "RoundedLoopOffset.h"
#include "SketchPlaneRepair.h"
#include <QCloseEvent>
#include <QDockWidget>
#include <QJsonDocument>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QShortcut>
#include <QVBoxLayout>

namespace {
class RepairDock final : public QDockWidget {
public:
    using QDockWidget::QDockWidget;
    std::function<void()> onClose;
    void closeEvent(QCloseEvent* event) override {if(onClose)onClose();QDockWidget::closeEvent(event);}
};
}

QJsonObject AlloyWindow::failedEditSelection() const {
    return {{"bodyId",selectedBody_},{"occurrenceId",selectedOccurrence_},{"edge",selectedEdge_},
        {"face",selectedFace_},{"planeId",selectedPlane_},{"sketchId",selectedSketch_},
        {"profileId",selectedProfile_},{"profileIds",selectedProfileIds_},{"point",view_->finishedPointSelection()}};
}

bool AlloyWindow::canInspectFailedEdit(const QJsonObject& diagnostics) const {
    if(diagnostics.value("featureType")!="sketch")return false;
    const auto sketch=SketchGeometry::findFeature(document_->authored(),diagnostics.value("featureId").toString());
    const auto parameters=sketch.value("parameters").toObject();
    if(sketch.isEmpty() || parameters.value("plane")!="face")return false;
    if(diagnostics.value("code")=="support_unavailable")return true;
    if(diagnostics.value("code")!="ambiguous_inset_lineage" || !parameters.contains("loopOffsets")
        || !RoundedLoopOffset::validate(parameters).isEmpty())return false;
    for(const auto& row:parameters.value("loopOffsets").toArray())
        if(row.toObject().value("kind")!="planarInsetV1")return false;
    const auto support=parameters.value("support").toObject();
    for(const auto& key:{"bodyId","producerFeatureId","faceId"})
        if(diagnostics.value(key)!=support.value(key))return false;
    return true;
}

bool AlloyWindow::retainFailedEditRepair(const QString& command,const QJsonObject& arguments,
    const QJsonObject& candidate,const QJsonObject& diagnostics) {
    // Only an existing exact support or contextualized planar-inset boundary
    // failure has a meaningful accepted cache. No invalid document is accepted.
    if(!canInspectFailedEdit(diagnostics) || candidate==document_->authored() || !failedEditRepair_.isEmpty())return false;
    const auto sketchId=diagnostics.value("featureId").toString();
    const auto sketch=SketchGeometry::findFeature(document_->authored(),sketchId);
    if(diagnostics.value("code")=="ambiguous_inset_lineage"
        && (command!="solid.combine" || arguments.value("featureId").toString().isEmpty()
            || SketchGeometry::findFeature(candidate,sketchId)!=sketch))return false;
    auto prefix=candidate;QJsonArray preceding;bool found=false;
    for(const auto& row:candidate.value("features").toArray()) {
        if(row.toObject().value("id")==sketchId){found=true;break;}preceding.append(row);
    }
    if(!found)return false;
    prefix["features"]=preceding;prefix.remove("objectOwners");prefix.remove("bodyNames");
    const auto evaluated=CadDocument::evaluateCandidate(prefix);
    if(!evaluated.ok)return false;
    const auto accepted=document_->evaluationBefore(sketchId);
    const auto context=GeometryKernel::projectionContext(accepted,sketch);
    const auto solved=SketchGeometry::solveDraft(sketch,context);
    if(!solved.ok)return false;
    const auto frame=GeometryKernel::sketchFrame(accepted,sketch);
    const auto previous=session_.value("repairPreviousSelection").toObject(failedEditSelection());
    const auto camera=session_.value("repairPreviousCamera").toObject(view_->cameraState());
    if(!session_.isEmpty())finishSession(session_.value("id").toString(),false);
    failedEditPreviousSelection_=previous;failedEditPreviousCamera_=camera;
    failedEditDocument_=document_;failedEditPrefix_=evaluated.evaluation;
    QJsonArray featureIds,bodies,cachedProfiles,owners;
    for(const auto& row:preceding)featureIds.append(row.toObject().value("id"));
    for(const auto& body:failedEditPrefix_->bodies)bodies.append(QJsonObject{{"bodyId",body.bodyId},{"geometry",body.summary}});
    for(const auto& value:sketch.value("parameters").toObject().value("loopOffsets").toArray())owners.append(value.toObject().value("id"));
    for(const auto& cached:document_->sketches())if(cached.featureId==sketchId)
        for(const auto& profile:cached.profiles)cachedProfiles.append(profile.profileId);
    const auto sessionId=CadDocument::newId();
    failedEditRepair_={{"id",sessionId},{"candidateId",CadDocument::newId()},{"status","active"},
        {"mode","failedEditRepair"},{"policy","alloy_transient_failed_edit_inspection"},
        {"baseDocumentId",document_->authored().value("documentId")},{"baseDocumentRevision",static_cast<qint64>(document_->revision())},
        {"failedCommand",QJsonObject{{"command",command},{"arguments",arguments}}},{"candidateAuthored",candidate},
        {"diagnostics",diagnostics},{"impactedSketchIds",QJsonArray{sketchId}},{"cachedProfileIds",cachedProfiles},{"cachedOwnerIds",owners},
        {"presentation","last_accepted_sketch_cache"},{"candidateGeometryEvaluated",false},
        {"validPrefix",QJsonObject{{"authored",evaluated.authored},{"featureIds",featureIds},{"bodies",bodies}}}};
    // Share the ordinary sketch selection/inspection commands, with an explicit
    // read-only mode. The failed prefix is used only by exact-support Delete.
    session_={{"id",sessionId},{"command","sketch.edit"},{"status","active"},{"mode","failedEditRepair"},
        {"cachedGeometry",true},{"failedEditRepair",failedEditRepair_},{"repairCandidateId",failedEditRepair_.value("candidateId")},
        {"baseDocumentRevision",static_cast<qint64>(document_->revision())},
        {"inputs",QJsonObject{{"sketchId",sketchId},{"plane","face"},{"name",sketch.value("name")},{"units",document_->authored().value("displayUnits")}}},
        {"draftSketch",sketch},{"resolvedFrame",frame},{"projectionContext",context},{"solverDiagnostics",solved.diagnostics},
        {"tool","select"},{"selection",QJsonObject{{"pointIds",QJsonArray{}},{"entityIds",QJsonArray{}},{"profileIds",QJsonArray{}}}},
        {"displayOptions",QJsonObject{{"showProfile",true},{"showPoints",true},{"showConstructionGeometries",true},{"showProjectedGeometries",true},{"showConstraints",true},{"showDimensions",true}}}};
    sketchHistory_={{sketch,solved.diagnostics}};sketchHistoryIndex_=0;sketchHistoryGroup_.clear();updateSketchHistoryState();
    selectedBody_.clear();selectedOccurrence_.clear();selectedEdge_={};selectedFace_={};selectedPlane_.clear();
    selectedSketch_.clear();selectedProfile_.clear();selectedProfileIds_={};
    view_->clearPreview();view_->clearBodyStage();view_->setEditDisplay({},{});view_->selectProfiles({},{});view_->selectSupportFace({},{});view_->selectFinishedPoint({});view_->clearSolidEdgeHighlights();
    displayedSketchDraft_={};view_->lookAtSketch("face",frame);
    showFailedEditRepair();return true;
}

QJsonObject AlloyWindow::closeFailedEditRepair(const QString& status,bool restoreSelection) {
    auto terminal=failedEditRepair_;if(terminal.isEmpty())return {};
    planeRepairSession_={};planeRepairCamera_={};planeRepairPreview_={};if(planeRepairDock_)planeRepairDock_->hide();
    const auto sessionId=terminal.value("id").toString();
    // Clear the guard before normal sketch teardown; no history operation occurs.
    failedEditRepair_={};failedEditPrefix_.reset();failedEditDocument_=nullptr;
    if(session_.value("id")==sessionId)finishSession(sessionId,false);
    if(failedEditDock_)failedEditDock_->hide();
    if(restoreSelection) {
        const auto selection=failedEditPreviousSelection_;
        selectedBody_=selection.value("bodyId").toString();selectedOccurrence_=selection.value("occurrenceId").toString();
        selectedEdge_=selection.value("edge").toObject();selectedFace_=selection.value("face").toObject();selectedPlane_=selection.value("planeId").toString();
        selectedSketch_=selection.value("sketchId").toString();selectedProfile_=selection.value("profileId").toString();selectedProfileIds_=selection.value("profileIds").toArray();
        view_->selectBody(selectedBody_,selectedOccurrence_);view_->selectProfiles(selectedSketch_,selectedProfileIds_);view_->selectFinishedPoint(selection.value("point").toObject());
        view_->selectConstructionPlane(selectedPlane_);if(!selectedFace_.isEmpty())selectSupportFace(selectedFace_);
        view_->restoreCameraState(failedEditPreviousCamera_);
    }
    failedEditPreviousSelection_={};failedEditPreviousCamera_={};
    terminal["status"]=status;terminal["evaluationStatus"]="completed";geometryEvaluationTerminal_=terminal;
    return {{"sessionId",sessionId},{"candidateId",terminal.value("candidateId")},{"status",status},{"authoredChange",false}};
}

void AlloyWindow::showFailedEditRepair() {
    if(sketchDock_)sketchDock_->hide();
    if(failedEditDock_)delete failedEditDock_;
    auto* dock=new RepairDock("Failed Edit — Cached Sketch",canvasWindow_);failedEditDock_=dock;
    dock->setObjectName("failedEditRepair.dock");dock->setAllowedAreas(Qt::RightDockWidgetArea|Qt::LeftDockWidgetArea);
    auto* panel=new QWidget;panel->setMinimumWidth(290);panel->setMaximumWidth(360);dock->setWidget(panel);
    auto* layout=new QVBoxLayout(panel);
    auto label=[&](const QString& text){auto* row=new QLabel(text);row->setWordWrap(true);row->setTextInteractionFlags(Qt::TextSelectableByMouse);layout->addWidget(row);return row;};
    auto* banner=label("CACHED ACCEPTED SKETCH\nThe upstream edit failed. The document and solid are unchanged.");
    banner->setObjectName("failedEditRepair.banner");banner->setStyleSheet("background:#fff2d5; color:#684300; border:1px solid #d3ad58; padding:8px;");
    const auto diagnostic=failedEditRepair_.value("diagnostics").toObject();
    label(diagnostic.value("message").toString())->setObjectName("failedEditRepair.diagnostic");
    const auto sketchId=diagnostic.value("featureId").toString();
    const auto affected=SketchGeometry::findFeature(document_->authored(),sketchId);
    const bool ordinary=SketchPlaneRepair::ordinary(affected);
    auto* identity=label(QString("Sketch: %1\nSupport: %2\nSession: %3\nCandidate: %4\nBase revision: %5")
        .arg(sketchId,diagnostic.value("faceId").toString(),failedEditRepair_.value("id").toString(),failedEditRepair_.value("candidateId").toString())
        .arg(failedEditRepair_.value("baseDocumentRevision").toInteger()));
    identity->setObjectName("failedEditRepair.identity");
    identity->setToolTip(QString::fromUtf8(QJsonDocument(QJsonObject{{"failedCommand",failedEditRepair_.value("failedCommand")},
        {"cachedOwnerIds",failedEditRepair_.value("cachedOwnerIds")}}).toJson(QJsonDocument::Indented)));
    label(ordinary ? "Cached sketch geometry is shown for inspection. Choose Redefine Sketch Plane to select an earlier replacement. Nothing has been committed." : "Select a cached generated profile to inspect Delete. Cached geometry is not an evaluated result of the failed candidate.");
    auto* profiles=new QListWidget;profiles->setObjectName("failedEditRepair.profiles");
    for(const auto& id:failedEditRepair_.value("cachedProfileIds").toArray()) {
        auto* row=new QListWidgetItem("Cached profile · "+id.toString().left(8),profiles);row->setData(Qt::UserRole,id.toString());
        row->setToolTip("Profile: "+id.toString());
    }
    layout->addWidget(profiles);
    connect(profiles,&QListWidget::currentItemChanged,this,[this](QListWidgetItem* item){if(item)execute("sketch.selectionSet",{{"profileIds",QJsonArray{item->data(Qt::UserRole).toString()}}});});
    auto* remove=new QPushButton("Delete Selected Cached Profile");remove->setObjectName("failedEditRepair.delete");layout->addWidget(remove);remove->setVisible(!ordinary);
    failedEditActionError_=label({});failedEditActionError_->setObjectName("failedEditRepair.actionError");failedEditActionError_->setStyleSheet("color:#984800;");
    connect(remove,&QPushButton::clicked,this,[this]{const auto response=execute("sketch.deleteSelection");failedEditActionError_->setText(response.value("error").toObject().value("message").toString());failedEditActionError_->setMinimumHeight(failedEditActionError_->heightForWidth(failedEditActionError_->width()));});
    if(ordinary && affected.value("ownerId")==document_->authored().value("documentId")) {
        auto* redefine=new QPushButton("Redefine Sketch Plane");redefine->setObjectName("failedEditRepair.redefinePlane");layout->addWidget(redefine);
        connect(redefine,&QPushButton::clicked,this,[this,sketchId]{execute("sketch.redefinePlaneBegin",{{"sketchId",sketchId}});});
    }
    auto* cancel=new QPushButton("Cancel Failed Edit");cancel->setObjectName("failedEditRepair.cancel");layout->addWidget(cancel);
    connect(cancel,&QPushButton::clicked,this,[this]{execute("command.cancel",{{"session",session_.value("id")}});});
    dock->onClose=[this]{execute("command.cancel",{{"session",session_.value("id")}});};
    auto* escape=new QShortcut(QKeySequence(Qt::Key_Escape),panel);escape->setContext(Qt::WidgetWithChildrenShortcut);
    connect(escape,&QShortcut::activated,cancel,&QPushButton::click);
    canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,dock);dock->show();
}
