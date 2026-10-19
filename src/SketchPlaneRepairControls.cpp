// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include "SketchPlaneRepair.h"
#include <BRepAdaptor_Surface.hxx>
#include <QCloseEvent>
#include <QDockWidget>
#include <QJsonDocument>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QShortcut>
#include <QSignalBlocker>
#include <QVBoxLayout>
namespace {
QJsonObject fail(const QString& code,const QString& message){return {{"ok",false},{"completion","failed"},{"error",QJsonObject{{"code",code},{"message",message}}}};}
QJsonObject parameter(const QString& type,const QString& description){return {{"type",type},{"description",description},{"required",true}};}
class PlaneRepairDock final : public QDockWidget {
public:using QDockWidget::QDockWidget;std::function<void()> onClose;
void closeEvent(QCloseEvent* event) override {if(onClose)onClose();QDockWidget::closeEvent(event);}
};
QString vectorText(const QJsonValue& v){QStringList parts;for(const auto& n:v.toArray())parts.append(QString::number(n.toDouble(),'g',7));return "["+parts.join(", ")+"]";}
}
void AlloyWindow::registerSketchPlaneRepairCommands() {
    commands_.push_back({"sketch.redefinePlaneBegin","Redefine Sketch Plane","Choose an earlier parallel planar reference for an ordinary sketch in failed-edit inspection.",{{"sketchId",parameter("string","Affected ordinary sketch UUID.")}},true,[this](const auto& a){return beginSketchPlaneRepair(a.value("sketchId").toString());}});
    commands_.push_back({"sketch.redefinePlanePreview","Preview Sketch Plane","Explicit replacement is exactly one of {support}, {planeId}, or {plane}; evaluates the upstream edit and downstream model together.",{{"replacement",parameter("object","Exact earlier planar reference.")}},true,[this](const auto& a){return previewSketchPlaneRepair(a.value("replacement").toObject());}});
    commands_.push_back({"sketch.redefinePlaneCancel","Cancel Redefine Sketch Plane","Return to the exact cached failed-edit inspection state without changing the document.",{},true,[this](const auto&){return cancelSketchPlaneRepair();}});
    commands_.push_back({"sketch.redefinePlaneCommit","Accept Redefine Sketch Plane","Commit the failed upstream edit and explicit support replacement as one validated undoable edit.",{},true,[this](const auto&){return commitSketchPlaneRepair();}});
}
QJsonObject AlloyWindow::beginSketchPlaneRepair(const QString& sketchId) {
    if(failedEditRepair_.isEmpty() || !failedEditPrefix_ || failedEditRepair_.value("diagnostics").toObject().value("code")!="support_unavailable"
        || !failedEditRepair_.value("impactedSketchIds").toArray().contains(sketchId))return fail("repair_unavailable","Open failed-edit inspection for this sketch first");
    if(!planeRepairSession_.isEmpty())return fail("session_busy","Redefine Sketch Plane is already active");
    const auto sketch=session_.value("draftSketch").toObject();
    if(sketch.value("id").toString()!=sketchId)return fail("repair_unavailable","Open the cached inspection for the requested sketch first");
    if(!SketchPlaneRepair::ordinary(sketch) || sketch.value("ownerId")!=document_->authored().value("documentId"))return fail("unsupported_sketch_repair","Choose an ordinary root sketch without linked or generated geometry");
    planeRepairSession_=session_;planeRepairCamera_=view_->cameraState();planeRepairPreview_={};
    QJsonArray choices;
    for(const auto& plane:QStringList{"XY","XZ","YZ"})choices.append(QJsonObject{{"label",plane+" Origin Plane"},{"replacement",QJsonObject{{"plane",plane}}}});
    for(const auto& plane:failedEditPrefix_->constructionPlanes)choices.append(QJsonObject{{"label",plane.name+" · Construction Plane"},{"replacement",QJsonObject{{"planeId",plane.featureId}}}});
    for(const auto& body:failedEditPrefix_->bodies)for(const auto& face:body.supportFaces)
        if(!face.face.IsNull() && BRepAdaptor_Surface(face.face).GetType()==GeomAbs_Plane)
            choices.append(QJsonObject{{"label",body.name+" · "+face.role+" · "+face.faceId.left(8)},
                {"replacement",QJsonObject{{"support",QJsonObject{{"bodyId",face.bodyId},{"producerFeatureId",face.producerFeatureId},{"faceId",face.faceId}}}}}});
    session_["planeRedefinition"]=QJsonObject{{"status","choosing"},{"framePolicy","preserveLocalCoordinatesParallelAxes"},{"choices",choices},{"previewed",false}};
    if(failedEditDock_)failedEditDock_->hide();if(planeRepairDock_)delete planeRepairDock_;
    auto* dock=new PlaneRepairDock("Redefine Sketch Plane",canvasWindow_);planeRepairDock_=dock;dock->setObjectName("sketch.redefinePlane.dock");
    auto* panel=new QWidget;panel->setMinimumWidth(300);panel->setMaximumWidth(370);dock->setWidget(panel);auto* layout=new QVBoxLayout(panel);
    auto* instruction=new QLabel("Choose an earlier parallel face or plane. Sketch coordinates stay relative to its origin. The failed upstream edit and all dependent features are previewed together.");instruction->setWordWrap(true);layout->addWidget(instruction);
    auto* list=new QListWidget;list->setObjectName("sketch.redefinePlane.choices");
    for(const auto& value:choices){const auto row=value.toObject();auto* item=new QListWidgetItem(row.value("label").toString(),list);item->setData(Qt::UserRole,row.value("replacement").toObject());item->setToolTip(QString::fromUtf8(QJsonDocument(row.value("replacement").toObject()).toJson()));}
    layout->addWidget(list);planeRepairInfo_=new QLabel("Select a replacement. Nothing is committed.");planeRepairInfo_->setObjectName("sketch.redefinePlane.frame");planeRepairInfo_->setWordWrap(true);layout->addWidget(planeRepairInfo_);
    planeRepairError_=new QLabel;planeRepairError_->setObjectName("sketch.redefinePlane.error");planeRepairError_->setWordWrap(true);planeRepairError_->setStyleSheet("color:#984800;");layout->addWidget(planeRepairError_);
    connect(list,&QListWidget::currentItemChanged,this,[this,list](QListWidgetItem* item,QListWidgetItem* previous){if(!item)return;const auto response=execute("sketch.redefinePlanePreview",{{"replacement",item->data(Qt::UserRole).toJsonObject()}});if(!response.value("ok").toBool()){const QSignalBlocker block(list);list->setCurrentItem(previous);}});
    planeRepairOk_=new QPushButton("OK");planeRepairOk_->setObjectName("sketch.redefinePlane.ok");planeRepairOk_->setEnabled(false);layout->addWidget(planeRepairOk_);
    connect(planeRepairOk_,&QPushButton::clicked,this,[this]{const auto response=execute("sketch.redefinePlaneCommit");if(!response.value("ok").toBool())planeRepairError_->setText(response.value("error").toObject().value("message").toString());});
    auto* cancel=new QPushButton("Cancel");cancel->setObjectName("sketch.redefinePlane.cancel");layout->addWidget(cancel);connect(cancel,&QPushButton::clicked,this,[this]{execute("sketch.redefinePlaneCancel");});
    dock->onClose=[this]{execute("sketch.redefinePlaneCancel");};auto* escape=new QShortcut(QKeySequence(Qt::Key_Escape),panel);escape->setContext(Qt::WidgetWithChildrenShortcut);connect(escape,&QShortcut::activated,cancel,&QPushButton::click);
    canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,dock);dock->show();return session_;
}
QJsonObject AlloyWindow::previewSketchPlaneRepair(const QJsonObject& replacement) {
    if(planeRepairSession_.isEmpty() || failedEditRepair_.isEmpty())return fail("invalid_session","Redefine Sketch Plane is not active");
    auto refuse=[this](const QString& code,const QString& message){if(planeRepairError_)planeRepairError_->setText(message);return fail(code,message);};
    const auto sketchId=planeRepairSession_.value("draftSketch").toObject().value("id").toString();
    try {
        const auto choice=SketchPlaneRepair::candidate(failedEditRepair_.value("candidateAuthored").toObject(),*failedEditPrefix_,sketchId,replacement,planeRepairSession_.value("resolvedFrame").toObject());
        auto evaluated=CadDocument::evaluateCandidate(choice.authored);if(!evaluated.ok)return refuse(evaluated.diagnostics.value("code").toString("evaluation_failed"),evaluated.error);
        QJsonArray geometry;for(const auto& body:evaluated.evaluation->bodies)geometry.append(QJsonObject{{"bodyId",body.bodyId},{"featureId",body.featureId},{"geometry",body.summary}});
        auto state=session_.value("planeRedefinition").toObject();state["status"]="preview";state["previewed"]=true;state["generation"]=state.value("generation").toInt()+1;state["replacement"]=replacement;for(const auto& row:state.value("choices").toArray())if(row.toObject().value("replacement").toObject()==replacement)state["referenceLabel"]=row.toObject().value("label");state["frame"]=choice.frame;state["geometry"]=geometry;state["documentDraft"]=choice.authored;
        planeRepairPreview_=std::move(evaluated);session_["planeRedefinition"]=state;planeRepairError_->clear();planeRepairOk_->setEnabled(true);
        planeRepairInfo_->setText("CANDIDATE PREVIEW — nothing committed\nReference: "+state.value("referenceLabel").toString()+"\nOrigin "+vectorText(choice.frame.value("origin"))+"\nU "+vectorText(choice.frame.value("uAxis"))+"\nV "+vectorText(choice.frame.value("vAxis"))+"\nNormal "+vectorText(choice.frame.value("normal"))+QString("\n%1 evaluated bodies; dependent geometry recomputed.").arg(geometry.size()));
        if(auto* list=planeRepairDock_->findChild<QListWidget*>("sketch.redefinePlane.choices")){const QSignalBlocker block(list);for(int i=0;i<list->count();++i)if(list->item(i)->data(Qt::UserRole).toJsonObject()==replacement){list->setCurrentItem(list->item(i));planeRepairDock_->widget()->layout()->activate();list->scrollToItem(list->item(i));break;}}
        presentSketchPlaneRepair();const auto candidateSketch=SketchGeometry::findFeature(planeRepairPreview_.authored,sketchId);
        view_->lookAtSketch(candidateSketch.value("parameters").toObject().value("plane").toString(),choice.frame);
        return state;
    } catch(const GeometryKernel::EvaluationError& error){return refuse(error.diagnostics().value("code").toString("invalid_replacement"),QString::fromUtf8(error.what()));}
}
void AlloyWindow::presentSketchPlaneRepair() {
    if(!planeRepairPreview_.ok || !planeRepairPreview_.evaluation)return;
    const auto sketchId=planeRepairSession_.value("draftSketch").toObject().value("id").toString();
    view_->setBodyStage(planeRepairPreview_.evaluation->bodies,"sketch-plane-repair:"+session_.value("id").toString()+":"+QString::number(session_.value("planeRedefinition").toObject().value("generation").toInt()));view_->setEditDisplay({},{});
    for(const auto& sketch:planeRepairPreview_.evaluation->sketches)if(sketch.featureId==sketchId){view_->setSketchPlane(sketch.plane,sketch.resolvedFrame);view_->showSketchDraft(sketch);const auto feature=SketchGeometry::findFeature(planeRepairPreview_.authored,sketchId);auto selected=session_.value("selection").toObject();selected["tool"]="select";view_->decorateSketch(feature,selected,{},document_->authored().value("displayUnits").toString());}
    const auto replacement=session_.value("planeRedefinition").toObject().value("replacement").toObject();
    if(replacement.contains("support")){const auto support=replacement.value("support").toObject();const auto face=GeometryKernel::resolveSketchSupport(*failedEditPrefix_,support);view_->selectSupportFace(support,face.face,true);}
    else {view_->selectSupportFace({},{});view_->selectConstructionPlane(replacement.value("planeId").toString());}
}
QJsonObject AlloyWindow::cancelSketchPlaneRepair() {
    if(planeRepairSession_.isEmpty())return fail("invalid_session","Redefine Sketch Plane is not active");
    session_=planeRepairSession_;planeRepairSession_={};planeRepairPreview_={};if(planeRepairDock_)planeRepairDock_->hide();if(failedEditDock_)failedEditDock_->show();
    view_->clearBodyStage();view_->clearPreview();view_->selectSupportFace({},{});view_->selectConstructionPlane({});displayedSketchDraft_={};view_->restoreCameraState(planeRepairCamera_);planeRepairCamera_={};
    return {{"status","cancelled"},{"authoredChange",false},{"repairSessionId",session_.value("id")}};
}
QJsonObject AlloyWindow::commitSketchPlaneRepair() {
    if(planeRepairSession_.isEmpty() || !planeRepairPreview_.ok)return fail("replacement_required","Select a valid replacement and inspect its preview first");
    const auto sketchId=planeRepairSession_.value("draftSketch").toObject().value("id").toString();
    const auto candidate=planeRepairPreview_.authored;
    if(const auto error=document_->commit(candidate,"Redefine Sketch Plane");!error.isEmpty())return fail("evaluation_failed",error);
    closeFailedEditRepair("repaired",false);selectedSketch_=sketchId;selectedBody_.clear();selectedProfile_.clear();selectedProfileIds_={};
    return {{"status","committed"},{"sketchId",sketchId},{"document",document_->state()}};
}
