// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <QDockWidget>
#include <QFormLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <QSignalBlocker>
#include <QShortcut>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <cmath>
#include <QComboBox>
#include <BRepAdaptor_Curve.hxx>
namespace {QJsonObject failure(const QString& code,const QString& message){return {{"ok",false},{"error",QJsonObject{{"code",code},{"message",message}}}};}}
QJsonObject AlloyWindow::ribCandidate(const QJsonObject& inputs) const {
    auto args=inputs;if(!args.contains("featureId") && session_.value("command")=="solid.rib"){args["newFeatureId"]=session_.value("newFeatureId");}
    auto candidate=CadDocument::ribCandidate(document_->authored(),authoringArguments(args));
    return candidate;
}
QJsonObject AlloyWindow::beginRib(const QJsonObject& overrides) {
    if(!session_.isEmpty())return failure("dialog_busy","Finish or cancel the active command first");
    QJsonObject inputs{{"extent","toNext"},{"direction","symmetric"},{"operation","join"},{"name","Rib"},{"thicknessMm",2.},{"flip",false}};
    if(overrides.contains("featureId")){bool found=false;for(const auto& v:document_->authored()["features"].toArray()){const auto f=v.toObject();if(f["id"]!=overrides["featureId"] || f["type"]!="rib")continue;inputs=f["parameters"].toObject();inputs.remove("supportFaceIds");inputs["featureId"]=f["id"];inputs["name"]=f["name"];found=true;}if(!found)return failure("feature_unavailable","Rib feature unavailable");}
    for(auto it=overrides.begin();it!=overrides.end();++it)inputs[it.key()]=it.value();const auto error=validateArguments(*findCommand("solid.ribDialog"),inputs);if(!error.isEmpty())return failure("invalid_arguments",error);
    session_={{"id",CadDocument::newId()},{"command","solid.rib"},{"status","active"},{"inputs",inputs},{"newFeatureId",CadDocument::newId()},{"baseDocumentRevision",static_cast<qint64>(document_->revision())},{"previewed",false},{"geometryEvaluated",false},{"inputSchema",findCommand("solid.rib")->parameters},{"nativeControlsAvailable",true}};
    if(inputs.contains("featureId"))view_->setBodyStage(document_->evaluationBefore(inputs["featureId"].toString()).bodies,"rib-source:"+session_.value("id").toString());showRib();previewRib();view_->setFocus();return session_;
}
QJsonObject AlloyWindow::previewRib() {
    if(session_.value("command")!="solid.rib")return failure("invalid_session","Rib is not active");
    session_["previewed"]=false;session_["geometryEvaluated"]=false;session_.remove("geometry");session_.remove("documentDraft");view_->clearPreview();
    auto reject=[this](const QString& code,const QString& message,const QJsonObject& diagnostics=QJsonObject{}){auto r=failure(code,message);auto e=r["error"].toObject();e["diagnostics"]=diagnostics;r["error"]=e;session_["previewError"]=e;refreshRibControls();return r;};
    if(session_.value("baseDocumentRevision").toInteger()!=static_cast<qint64>(document_->revision()))return reject("stale_session","The document changed; cancel and restart Rib");
    const auto inputs=session_.value("inputs").toObject();if(!inputs.contains("sourceEntityId") || !inputs.contains("targetBodyId"))return reject("selection_required","Select an open Line Profile and solid Target");QJsonObject candidate;try{candidate=ribCandidate(inputs);}catch(const std::exception& e){return reject("invalid_arguments",QString::fromUtf8(e.what()));}
    auto evaluated=CadDocument::evaluateCandidate(candidate);if(!evaluated.ok)return reject("evaluation_failed",evaluated.error,evaluated.diagnostics);
    const auto id=inputs.value("featureId").toString(session_.value("newFeatureId").toString());auto stage=evaluated.evaluation;
    if(inputs.contains("featureId")){QJsonArray prefix;for(const auto& v:candidate["features"].toArray()){prefix.append(v);if(v.toObject()["id"]==id)break;}auto early=candidate;early["features"]=prefix;auto result=CadDocument::evaluateCandidate(early);if(!result.ok)return reject("evaluation_failed",result.error,result.diagnostics);stage=result.evaluation;}
    for(const auto& body:stage->bodies)if(body.bodyId==inputs["targetBodyId"]){view_->showPreview(body.shape,body.bodyId,true);session_["geometry"]=body.summary;session_["featureId"]=id;session_["bodyId"]=body.bodyId;}
    session_["documentDraft"]=candidate;session_["previewed"]=true;session_["geometryEvaluated"]=true;session_.remove("previewError");refreshRibControls();return session_;
}

void AlloyWindow::refreshRibControls(){
 if(!ribDock_ || session_.value("command")!="solid.rib")return;
 const auto inputs=session_.value("inputs").toObject();
 auto* error=ribDock_->findChild<QLabel*>("rib.error");error->setText(session_.value("previewError").toObject()["message"].toString());
 ribDock_->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->setEnabled(session_.value("previewed").toBool());
 for(const auto& key:{"thicknessMm","name"}){auto* edit=ribDock_->findChild<QLineEdit*>("rib."+QString(key));if(edit && !edit->hasFocus()){QSignalBlocker b(edit);edit->setText(QString(key)=="name"?inputs[key].toString():QString::number(inputs[key].toDouble(),'g',12));}}
 auto* flip=ribDock_->findChild<QCheckBox*>("rib.flip");{QSignalBlocker b(flip);flip->setChecked(inputs["flip"].toBool());}
 for(const auto& key:{"profile","target"}){auto* box=ribDock_->findChild<QComboBox*>("rib."+QString(key));QSignalBlocker b(box);int index=0;for(int i=1;i<box->count();++i){const auto data=box->itemData(i).toJsonObject();if(QString(key)=="profile" ? data["sourceEntityId"]==inputs["sourceEntityId"] && data["sketchId"]==inputs["sketchId"] : data["targetBodyId"]==inputs["targetBodyId"])index=i;}box->setCurrentIndex(index);}
 const auto available=sweepInputDocument();QSet<QString> visible;for(const auto& sketch:available.sketches)if(sketch.ownerId==document_->activeOwnerId()){visible.insert(sketch.featureId);if(sketch.featureId==inputs["sketchId"] && inputs.contains("sourceEntityId"))view_->showSweepPath(sketch,QJsonArray{inputs["sourceEntityId"]});}view_->setCommandSketches(visible);
}
void AlloyWindow::showRib(){
 const auto sid=session_.value("id").toString();if(ribDock_){canvasWindow_->removeDockWidget(ribDock_);delete ribDock_;}ribDock_=new QDockWidget("Rib",canvasWindow_);ribDock_->setObjectName("ribDock");ribDock_->setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);auto* panel=new QWidget;ribDock_->setWidget(panel);auto* form=new QFormLayout(panel);form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
 const auto update=[this,sid](const QJsonObject& inputs){if(session_.value("id").toString()!=sid)return;execute("command.update",{{"session",sid},{"inputs",inputs}});previewRib();};
 auto* name=new QLineEdit(session_.value("inputs").toObject()["name"].toString());name->setObjectName("rib.name");form->addRow("Name",name);connect(name,&QLineEdit::editingFinished,panel,[update,name]{update({{"name",name->text()}});});
 const auto available=sweepInputDocument();
 auto* profile=new QComboBox;profile->setObjectName("rib.profile");profile->addItem("Select open Line");for(const auto& sketch:available.sketches)if(sketch.ownerId==document_->activeOwnerId())for(const auto& row:sketch.summary["entities"].toArray()){const auto entity=row.toObject();const auto eid=entity["id"].toString();if(entity["type"]!="line" || entity["construction"].toBool())continue;bool closed=false;for(const auto& region:sketch.profiles)closed|=region.summary["sourceEntityIds"].toArray().contains(eid);if(!closed)profile->addItem(sketch.name+" / Line "+eid.left(8),QJsonObject{{"sketchId",sketch.featureId},{"sourceEntityId",eid}});}form->addRow("Profile",profile);connect(profile,&QComboBox::activated,panel,[update,profile](int i){if(i>0)update(profile->itemData(i).toJsonObject());});
 auto* target=new QComboBox;target->setObjectName("rib.target");target->addItem("Select solid Target");for(const auto& body:available.bodies)if((body.ownerId.isEmpty() || body.ownerId==document_->activeOwnerId()) && body.summary["closed"].toBool() && body.summary["topology"].toObject()["solids"]==1)target->addItem(body.name+" / "+body.bodyId.left(8),QJsonObject{{"targetBodyId",body.bodyId}});form->addRow("Target Body",target);connect(target,&QComboBox::activated,panel,[update,target](int i){if(i>0)update(target->itemData(i).toJsonObject());});
 form->addRow("Direction",new QLabel("Symmetric"));auto* thickness=new QLineEdit(QString::number(session_.value("inputs").toObject()["thicknessMm"].toDouble()));thickness->setObjectName("rib.thicknessMm");form->addRow("Thickness (mm)",thickness);connect(thickness,&QLineEdit::editingFinished,panel,[this,update,thickness]{bool ok=false;double value=thickness->text().toDouble(&ok);if(!ok || !std::isfinite(value) || value<.001 || value>100000){session_["previewed"]=false;session_["geometryEvaluated"]=false;session_.remove("documentDraft");session_["previewError"]=QJsonObject{{"code","invalid_number"},{"message","Enter Thickness from 0.001 to 100000 mm"}};view_->clearPreview();refreshRibControls();return;}update({{"thicknessMm",value}});});
 form->addRow("Extent Type",new QLabel("To Next"));auto* flip=new QCheckBox("Flip Direction");flip->setObjectName("rib.flip");form->addRow(flip);connect(flip,&QCheckBox::toggled,panel,[update](bool checked){update({{"flip",checked}});});form->addRow("Operation",new QLabel("Join"));
 auto* hint=new QLabel("One open straight Line, symmetric thickness, and an unobstructed span to one full-cover planar wall. Direction follows the sketch normal crossed with the directed Line; Flip reverses it.");hint->setWordWrap(true);form->addRow(hint);auto* error=new QLabel;error->setObjectName("rib.error");error->setWordWrap(true);error->setStyleSheet("color:#b3261e");form->addRow(error);auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);form->addRow(buttons);connect(buttons,&QDialogButtonBox::accepted,panel,[this,sid]{if(session_.value("id").toString()==sid && previewRib().value("previewed").toBool())execute("command.commit",{{"session",sid}});});connect(buttons,&QDialogButtonBox::rejected,panel,[this,sid]{if(session_.value("id").toString()==sid)execute("command.cancel",{{"session",sid}});});canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,ribDock_);ribDock_->show();
 if(!ribEscape_){ribEscape_=new QShortcut(QKeySequence(Qt::Key_Escape),this);connect(ribEscape_,&QShortcut::activated,this,[this]{if(session_.value("command")=="solid.rib")execute("command.cancel",{{"session",session_.value("id")}});});}ribEscape_->setEnabled(true);refreshRibControls();
}
