// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include "SolidSphere.h"
#include <QDockWidget>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QShortcut>
#include <QLineEdit>
#include <QTimer>
#include <QSignalBlocker>
namespace {
QJsonObject sphereFailure(const QString& code,const QString& message,const QJsonObject& diagnostics={}) {
    return {{"ok",false},{"completion","failed"},{"error",QJsonObject{{"code",code},{"message",message},{"diagnostics",diagnostics}}}};
}
}
EvaluatedDocument AlloyWindow::sphereInputDocument() const {
    const auto edit=session_["inputs"].toObject()["featureId"].toString();
    return edit.isEmpty() ? document_->evaluation() : document_->evaluationBefore(edit);
}
QJsonObject AlloyWindow::sphereCandidate(const QJsonObject& inputs) const {
    auto args=inputs;
    if(!args.contains("featureId") && session_.value("command")=="solid.sphere") {
        args["newFeatureId"]=session_["newFeatureId"];args["newBodyId"]=session_["newBodyId"];
    }
    return CadDocument::sphereCandidate(document_->authored(),args);
}
QJsonObject AlloyWindow::beginSphere(const QJsonObject& overrides) {
    if(!session_.isEmpty())return sphereFailure("session_busy","Complete or cancel the active command first");
    const auto error=validateArguments(*findCommand("solid.sphere"),overrides);if(!error.isEmpty())return sphereFailure("invalid_arguments",error);
    QJsonObject inputs{{"diameterMm",lastSphereDiameterMm_},{"operation","cut"},{"name","Sphere"}};
    if(!selectedFace_.isEmpty()){inputs["support"]=selectedFace_;inputs["targetBodyId"]=selectedFace_["bodyId"];}
    if(overrides.contains("featureId")) {
        bool found=false;for(const auto& entry:document_->authored()["features"].toArray())if(entry.toObject()["id"]==overrides["featureId"]){const auto feature=entry.toObject();if(feature["type"]!="sphere")return sphereFailure("stale_entity","Sphere feature unavailable");inputs=feature["parameters"].toObject();inputs["featureId"]=feature["id"];inputs["name"]=feature["name"];found=true;break;}
        if(!found)return sphereFailure("stale_entity","Sphere feature unavailable");
    }
    for(auto it=overrides.begin();it!=overrides.end();++it)inputs[it.key()]=it.value();
    if(inputs.contains("support") && (!inputs.contains("targetBodyId") || (overrides.contains("support") && !overrides.contains("targetBodyId"))) && inputs["operation"]=="cut")inputs["targetBodyId"]=inputs["support"].toObject()["bodyId"];
    session_={{"id",CadDocument::newId()},{"command","solid.sphere"},{"status","active"},{"inputs",inputs},{"newFeatureId",CadDocument::newId()},{"newBodyId",CadDocument::newId()},{"baseDocumentRevision",static_cast<qint64>(document_->revision())},{"previewed",false},{"inputSchema",findCommand("solid.sphere")->parameters},{"selectionRole",inputs.contains("support") ? "center" : "face"}};
    view_->selectSupportFace({},{});
    view_->setBodyStage(sphereInputDocument().bodies,"sphere-source:"+session_["id"].toString());view_->selectBody({});view_->selectProfile({},{});showSphere();previewSphere();view_->setFocus();return session_;
}
QJsonObject AlloyWindow::selectSphereFace(const QJsonObject& support) {
    try {
        const auto face=GeometryKernel::resolveSketchSupport(sphereInputDocument(),support);
        auto inputs=session_["inputs"].toObject();inputs["support"]=support;inputs["targetBodyId"]=face.bodyId;inputs.remove("uMm");inputs.remove("vMm");session_["inputs"]=inputs;session_["selectionRole"]="center";
        view_->selectSupportFace(support,face.face);previewSphere();view_->setFocus();return QJsonObject{{"kind","face"},{"support",support},{"frame",face.frame}};
    } catch(const GeometryKernel::EvaluationError& e){return sphereFailure("support_unavailable",QString::fromUtf8(e.what()),e.diagnostics());}
}
QJsonObject AlloyWindow::placeSphereCenter(double x,double y) {
    try {
        const auto inputs=session_["inputs"].toObject();const auto face=GeometryKernel::resolveSketchSupport(sphereInputDocument(),inputs["support"].toObject());const auto point=view_->sketchPoint(x,y,"face",face.frame);
        auto updated=inputs;updated["uMm"]=point["uMm"];updated["vMm"]=point["vMm"];
        // Validate actual trimmed-face placement before replacing a usable center.
        SolidSphere::tool(sphereInputDocument(),updated);
        session_["inputs"]=updated;session_["selectionRole"]="placed";const auto result=previewSphere();
        if(result["previewed"].toBool() && sphereDock_)if(auto* diameter=sphereDock_->findChild<QDoubleSpinBox*>("sphereDiameter")){diameter->setFocus();diameter->selectAll();}
        return result;
    } catch(const GeometryKernel::EvaluationError& e){return sphereFailure("placement_failed",QString::fromUtf8(e.what()),e.diagnostics());}
      catch(const std::exception& e){return sphereFailure("placement_failed",QString::fromUtf8(e.what()));}
}
QJsonObject AlloyWindow::previewSphere() {
    if(sphereDock_)if(auto* box=sphereDock_->findChild<QDialogButtonBox*>())box->button(QDialogButtonBox::Ok)->setEnabled(false);
    session_["previewed"]=false;session_["geometryEvaluated"]=false;session_.remove("geometry");session_.remove("documentDraft");session_.remove("previewError");view_->clearPreview();
    auto reject=[this](const QString& code,const QString& message,const QJsonObject& diagnostics=QJsonObject{}){const auto result=sphereFailure(code,message,diagnostics);session_["previewError"]=result["error"];if(sphereDock_)if(auto* label=sphereDock_->findChild<QLabel*>("sphereError"))label->setText(message);return result;};
    if(session_["baseDocumentRevision"].toInteger()!=static_cast<qint64>(document_->revision()))return reject("stale_session","The document changed; cancel and restart Sphere");
    const auto inputs=session_["inputs"].toObject();const auto support=inputs["support"].toObject();
    if(sphereDock_)if(auto* label=sphereDock_->findChild<QLabel*>("sphereFace"))label->setText(support.isEmpty() ? "Select planar face" : "1 selected");
    if(sphereDock_)if(auto* label=sphereDock_->findChild<QLabel*>("sphereCenter"))label->setText(inputs.contains("uMm") && inputs.contains("vMm") ? QString("%1, %2 mm").arg(inputs["uMm"].toDouble(),0,'g',6).arg(inputs["vMm"].toDouble(),0,'g',6) : "Click on selected face");
    if(support.isEmpty())return reject("face_required","Select an actual planar body face");
    try{const auto face=GeometryKernel::resolveSketchSupport(sphereInputDocument(),support);view_->selectSupportFace(support,face.face);}catch(const GeometryKernel::EvaluationError& e){return reject("support_unavailable",QString::fromUtf8(e.what()),e.diagnostics());}
    if(!inputs.contains("uMm") || !inputs.contains("vMm"))return reject("center_required","Click to place the sphere center on the selected face");
    try {
        const auto source=sphereInputDocument();const auto resolved=GeometryKernel::resolveSketchSupport(source,support);view_->selectSupportFace(support,resolved.face);
        const auto tool=SolidSphere::tool(source,inputs);const auto candidate=sphereCandidate(inputs);const auto evaluated=CadDocument::evaluateCandidate(candidate);if(!evaluated.ok)return reject("evaluation_failed",evaluated.error,evaluated.diagnostics);
        // Inspect full history; display disposable cutter and preserve the source stock.
        const auto featureId=inputs.value("featureId").toString(session_["newFeatureId"].toString());QJsonArray features;for(const auto& entry:candidate["features"].toArray()){features.append(entry);if(entry.toObject()["id"]==featureId)break;}auto prefix=candidate;prefix["features"]=features;const auto stage=CadDocument::evaluateCandidate(prefix);if(!stage.ok)return reject("evaluation_failed",stage.error,stage.diagnostics);
        const auto resultId=inputs["operation"]=="cut" ? inputs["targetBodyId"].toString() : inputs.contains("featureId") ? [&]{for(const auto& entry:candidate["features"].toArray())if(entry.toObject()["id"]==featureId)return entry.toObject()["bodyId"].toString();return QString{};}() : session_["newBodyId"].toString();
        for(const auto& body:stage.evaluation->bodies)if(body.bodyId==resultId){session_["geometry"]=body.summary;break;}
        view_->showSpherePreview(tool,inputs["operation"]=="cut");session_["documentDraft"]=candidate;session_["previewed"]=true;session_["geometryEvaluated"]=true;
        if(sphereDock_){if(auto* label=sphereDock_->findChild<QLabel*>("sphereError"))label->clear();if(auto* box=sphereDock_->findChild<QDialogButtonBox*>())box->button(QDialogButtonBox::Ok)->setEnabled(true);}return session_;
    } catch(const GeometryKernel::EvaluationError& e){return reject("evaluation_failed",QString::fromUtf8(e.what()),e.diagnostics());}
      catch(const std::exception& e){return reject("invalid_arguments",QString::fromUtf8(e.what()));}
}
void AlloyWindow::showSphere() {
    if(sphereDock_){canvasWindow_->removeDockWidget(sphereDock_);delete sphereDock_;}
    sphereDock_=new QDockWidget("Sphere",canvasWindow_);sphereDock_->setObjectName("sphereDock");sphereDock_->setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);
    auto* panel=new QWidget(sphereDock_);sphereDock_->setWidget(panel);auto* form=new QFormLayout(panel);
    auto* face=new QLabel(panel);face->setObjectName("sphereFace");form->addRow("Plane / Face",face);
    auto* choose=new QPushButton("Select another face",panel);choose->setObjectName("sphereChooseFace");form->addRow(choose);connect(choose,&QPushButton::clicked,panel,[this]{session_["selectionRole"]="face";view_->setFocus();});
    auto* center=new QLabel(panel);center->setObjectName("sphereCenter");form->addRow("Center",center);
    auto* diameter=new QDoubleSpinBox(panel);diameter->setObjectName("sphereDiameter");diameter->setAccessibleName("Sphere Diameter");diameter->setDecimals(3);diameter->setRange(.001,100000);diameter->setSuffix(" mm");diameter->setKeyboardTracking(false);diameter->setValue(session_["inputs"].toObject()["diameterMm"].toDouble());form->addRow("Diameter",diameter);
    auto* operation=new QComboBox(panel);operation->setObjectName("sphereOperation");operation->addItem("Cut","cut");operation->addItem("New Body","newBody");operation->setCurrentIndex(operation->findData(session_["inputs"].toObject()["operation"].toString()));operation->setEnabled(!session_["inputs"].toObject().contains("featureId"));form->addRow("Operation",operation);
    auto* status=new QLabel(panel);status->setObjectName("sphereError");status->setWordWrap(true);form->addRow(status);
    connect(diameter,&QDoubleSpinBox::valueChanged,panel,[this](double value){const auto r=execute("command.update",{{"session",session_["id"]},{"inputs",QJsonObject{{"diameterMm",value}}}});if(r["ok"].toBool())previewSphere();});
    connect(operation,&QComboBox::currentIndexChanged,panel,[this,operation](int){const auto r=execute("command.update",{{"session",session_["id"]},{"inputs",QJsonObject{{"operation",operation->currentData().toString()}}}});if(r["ok"].toBool())previewSphere();});
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,panel);form->addRow(buttons);buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
    auto commit=[this,status]{const auto preview=previewSphere();if(!preview["previewed"].toBool())return;const auto r=execute("command.commit",{{"session",session_["id"]}});if(!r["ok"].toBool())status->setText(r["error"].toObject()["message"].toString());};
    connect(buttons,&QDialogButtonBox::accepted,panel,commit);connect(buttons,&QDialogButtonBox::rejected,panel,[this]{execute("command.cancel",{{"session",session_["id"]}});});
    // Spinbox Enter interprets pending diameter first, then commits once.
    for(auto key:{Qt::Key_Return,Qt::Key_Enter}){auto* enter=new QShortcut(QKeySequence(key),panel);enter->setContext(Qt::WidgetWithChildrenShortcut);connect(enter,&QShortcut::activated,panel,[diameter,commit]{diameter->interpretText();commit();});}
    auto* escape=new QShortcut(QKeySequence(Qt::Key_Escape),panel);escape->setContext(Qt::WidgetWithChildrenShortcut);connect(escape,&QShortcut::activated,panel,[this]{execute("command.cancel",{{"session",session_["id"]}});});
    canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,sphereDock_);sphereDock_->show();
}
