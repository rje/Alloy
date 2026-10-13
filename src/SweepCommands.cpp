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
namespace {QJsonObject failure(const QString& code,const QString& message){return {{"ok",false},{"error",QJsonObject{{"code",code},{"message",message}}}};}}
QJsonObject AlloyWindow::sweepCandidate(const QJsonObject& inputs) const {
    auto args=inputs;if(!args.contains("featureId") && session_.value("command")=="solid.sweep"){args["newFeatureId"]=session_.value("newFeatureId");args["newBodyId"]=session_.value("newBodyId");}
    auto candidate=CadDocument::sweepCandidate(document_->authored(),authoringArguments(args));
    if(!inputs.contains("featureId")){auto features=candidate["features"].toArray();for(int i=0;i<features.size();++i){auto f=features[i].toObject();if(f["id"]==inputs["profileSketchId"] || f["id"]==inputs["pathSketchId"]){f["visible"]=false;features[i]=f;}}candidate["features"]=features;}return candidate;
}
QJsonObject AlloyWindow::beginSweep(const QJsonObject& overrides) {
    if(!session_.isEmpty())return failure("dialog_busy","Finish or cancel the active command first");
    QJsonObject inputs{{"extent","full"},{"orientation","perpendicular"},{"operation","newBody"},{"name","Sweep"}};
    if(overrides.contains("featureId")){bool found=false;for(const auto& v:document_->authored()["features"].toArray()){const auto f=v.toObject();if(f["id"]!=overrides["featureId"] || f["type"]!="sweep")continue;inputs=f["parameters"].toObject();inputs["featureId"]=f["id"];inputs["name"]=f["name"];found=true;}if(!found)return failure("feature_unavailable","Sweep feature unavailable");}
    for(auto it=overrides.begin();it!=overrides.end();++it)inputs[it.key()]=it.value();const auto error=validateArguments(*findCommand("solid.sweepDialog"),inputs);if(!error.isEmpty())return failure("invalid_arguments",error);
    session_={{"id",CadDocument::newId()},{"command","solid.sweep"},{"status","active"},{"inputs",inputs},{"newFeatureId",CadDocument::newId()},{"newBodyId",CadDocument::newId()},{"baseDocumentRevision",static_cast<qint64>(document_->revision())},{"previewed",false},{"geometryEvaluated",false},{"inputSchema",findCommand("solid.sweep")->parameters},{"nativeControlsAvailable",true},{"selectionRole",inputs.contains("profileId")?"path":"profile"},{"chainSelection",true},{"chainSelectionPolicy","connected unbranched component, tangent continuity validated by Sweep; no proximity healing"}};
    if(inputs.contains("featureId"))view_->setBodyStage(document_->evaluationBefore(inputs["featureId"].toString()).bodies,"sweep-source:"+session_.value("id").toString());showSweep();displaySweepSelection();previewSweep();view_->setFocus();return session_;
}
QJsonObject AlloyWindow::previewSweep() {
    if(session_.value("command")!="solid.sweep")return failure("invalid_session","Sweep is not active");
    session_["previewed"]=false;session_["geometryEvaluated"]=false;session_.remove("geometry");session_.remove("documentDraft");view_->clearPreview();
    auto reject=[this](const QString& code,const QString& message,const QJsonObject& diagnostics=QJsonObject{}){auto r=failure(code,message);auto e=r["error"].toObject();e["diagnostics"]=diagnostics;r["error"]=e;session_["previewError"]=e;displaySweepSelection();refreshSweepControls();return r;};
    if(session_.value("baseDocumentRevision").toInteger()!=static_cast<qint64>(document_->revision()))return reject("stale_session","The document changed; cancel and restart Sweep");
    const auto inputs=session_.value("inputs").toObject();if(!inputs.contains("profileId") || !inputs.contains("pathEntityIds"))return reject("selection_required","Select a circular Profile, then the connected open Path at its center");QJsonObject candidate;try{candidate=sweepCandidate(inputs);}catch(const std::exception& e){return reject("invalid_arguments",QString::fromUtf8(e.what()));}
    auto evaluated=CadDocument::evaluateCandidate(candidate);if(!evaluated.ok)return reject("evaluation_failed",evaluated.error,evaluated.diagnostics);
    const auto id=inputs.value("featureId").toString(session_.value("newFeatureId").toString());auto stage=evaluated.evaluation;
    if(inputs.contains("featureId")){QJsonArray prefix;for(const auto& v:candidate["features"].toArray()){prefix.append(v);if(v.toObject()["id"]==id)break;}auto early=candidate;early["features"]=prefix;auto result=CadDocument::evaluateCandidate(early);if(!result.ok)return reject("evaluation_failed",result.error,result.diagnostics);stage=result.evaluation;}
    for(const auto& body:stage->bodies)if(body.featureId==id){view_->showPreview(body.shape,inputs.contains("featureId")?body.bodyId:QString{},true);session_["geometry"]=body.summary;session_["featureId"]=id;session_["bodyId"]=body.bodyId;}
    session_["documentDraft"]=candidate;session_["previewed"]=true;session_["geometryEvaluated"]=true;session_.remove("previewError");displaySweepSelection();refreshSweepControls();return session_;
}

void AlloyWindow::displaySweepSelection(){
    if(session_.value("command")!="solid.sweep")return;const auto available=sweepInputDocument();QSet<QString> visible;for(const auto& s:available.sketches)if(s.ownerId==document_->activeOwnerId())visible.insert(s.featureId);view_->setCommandSketches(visible);
    const auto inputs=session_.value("inputs").toObject();view_->selectProfile(inputs["profileSketchId"].toString(),inputs["profileId"].toString());view_->clearSweepPath();
    for(const auto& s:available.sketches)if(s.featureId==inputs["pathSketchId"])view_->showSweepPath(s,inputs["pathEntityIds"].toArray());
    for(const auto& s:available.sketches)if(s.featureId==inputs["profileSketchId"])for(const auto& profile:s.profiles)if(profile.profileId==inputs["profileId"])view_->showSweepProfile(profile);
}
void AlloyWindow::refreshSweepControls(){
    if(!sweepDock_ || session_.value("command")!="solid.sweep")return;const auto inputs=session_.value("inputs").toObject();
    auto* profile=sweepDock_->findChild<QPushButton*>("sweep.profile");auto* path=sweepDock_->findChild<QPushButton*>("sweep.path");const QSignalBlocker a(profile),b(path);profile->setText(inputs.contains("profileId")?"1 selected":"Select");path->setText(inputs.contains("pathEntityIds")?QString("%1 selected").arg(inputs["pathEntityIds"].toArray().size()):"Select");profile->setChecked(session_.value("selectionRole")=="profile");path->setChecked(session_.value("selectionRole")=="path");path->setEnabled(inputs.contains("profileId"));profile->setToolTip(inputs["profileSketchId"].toString()+" / "+inputs["profileId"].toString());path->setToolTip(inputs["pathSketchId"].toString()+" / "+QString::fromUtf8(QJsonDocument(inputs["pathEntityIds"].toArray()).toJson(QJsonDocument::Compact)));
    auto* chain=sweepDock_->findChild<QCheckBox*>("sweep.chainSelection");const QSignalBlocker c(chain);chain->setChecked(session_.value("chainSelection").toBool());
    auto* error=sweepDock_->findChild<QLabel*>("sweep.error");error->setText((session_.contains("selectionError")?session_.value("selectionError"):session_.value("previewError")).toObject()["message"].toString());sweepDock_->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->setEnabled(session_.value("previewed").toBool() && !session_.contains("selectionError"));
}
void AlloyWindow::showSweep(){
    const auto commandSession=session_.value("id").toString();if(sweepDock_){canvasWindow_->removeDockWidget(sweepDock_);delete sweepDock_;}sweepDock_=new QDockWidget("Sweep",canvasWindow_);sweepDock_->setObjectName("sweepDock");sweepDock_->setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);auto* panel=new QWidget(sweepDock_);sweepDock_->setWidget(panel);auto* form=new QFormLayout(panel);form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    auto* name=new QLineEdit(session_.value("inputs").toObject()["name"].toString());name->setObjectName("sweep.name");form->addRow("Name",name);connect(name,&QLineEdit::editingFinished,panel,[this,name,commandSession]{if(session_.value("command")!="solid.sweep" || session_.value("id").toString()!=commandSession)return;execute("command.update",{{"session",commandSession},{"inputs",QJsonObject{{"name",name->text()}}}});previewSweep();});
    form->addRow("Type",new QLabel("Single Path"));
    for(const auto& role:QStringList{"profile","path"}){auto* row=new QWidget;auto* layout=new QHBoxLayout(row);layout->setContentsMargins(0,0,0,0);auto* button=new QPushButton("Select");button->setObjectName("sweep."+role);button->setCheckable(true);auto* clear=new QPushButton("×");clear->setObjectName("sweep."+role+".clear");clear->setMaximumWidth(28);clear->setToolTip("Clear "+role);layout->addWidget(button,1);layout->addWidget(clear);form->addRow(role=="profile"?"Profile":"Path",row);connect(button,&QPushButton::clicked,panel,[this,role,commandSession]{if(session_.value("id").toString()!=commandSession)return;execute("solid.sweepSelection",{{"role",role}});});connect(clear,&QPushButton::clicked,panel,[this,role,commandSession]{if(session_.value("id").toString()!=commandSession)return;execute("solid.sweepSelection",{{"role",role},{"clear",true}});});}
    auto* chain=new QCheckBox("Chain Selection");chain->setObjectName("sweep.chainSelection");chain->setToolTip("Alloy default: checked. Expand the connected unbranched line/arc component; non-tangent junctions are refused. Unchecked: add individual sections, beginning at the circle center. Existing selection is retained when toggled.");form->addRow(chain);connect(chain,&QCheckBox::toggled,panel,[this,chain,commandSession]{if(session_.value("id").toString()!=commandSession)return;execute("solid.sweepSelection",{{"chainSelection",chain->isChecked()}});});
    auto* distance=new QLabel("1.00 (Full)");distance->setObjectName("sweep.distance");form->addRow("Distance",distance);form->addRow("Orientation",new QLabel("Perpendicular"));form->addRow("Operation",new QLabel("New Body"));
    auto* instruction=new QLabel("Select a circle Profile, then a Path section. Chain Selection expands one connected unbranched tangent path. Full path only; additional Sweep modes are not available.");instruction->setWordWrap(true);form->addRow(instruction);auto* error=new QLabel;error->setObjectName("sweep.error");error->setWordWrap(true);error->setStyleSheet("color:#b3261e");form->addRow(error);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);form->addRow(buttons);connect(buttons,&QDialogButtonBox::accepted,panel,[this,commandSession]{if(session_.value("command")!="solid.sweep" || session_.value("id").toString()!=commandSession)return;if(previewSweep().value("previewed").toBool())execute("command.commit",{{"session",commandSession}});});connect(buttons,&QDialogButtonBox::rejected,panel,[this,commandSession]{if(session_.value("id").toString()!=commandSession)return;execute("command.cancel",{{"session",commandSession}});});canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,sweepDock_);sweepDock_->show();
    if(!sweepEscape_){sweepEscape_=new QShortcut(QKeySequence(Qt::Key_Escape),this);connect(sweepEscape_,&QShortcut::activated,this,[this]{if(session_.value("command")=="solid.sweep")execute("command.cancel",{{"session",session_.value("id")}});});}sweepEscape_->setEnabled(true);refreshSweepControls();
}
