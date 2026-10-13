// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <QDockWidget>
#include <QFormLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QDialogButtonBox>
#include <QSignalBlocker>
#include <QShortcut>
#include <QHBoxLayout>
namespace {QJsonObject failure(const QString& code,const QString& message){return {{"ok",false},{"error",QJsonObject{{"code",code},{"message",message}}}};}}
QJsonObject AlloyWindow::revolveCandidate(const QJsonObject& inputs) const {
    auto args=inputs;if(!args.contains("featureId") && session_.value("command")=="solid.revolve"){args["newFeatureId"]=session_["newFeatureId"];args["newBodyId"]=session_["newBodyId"];}
    auto candidate=CadDocument::revolveCandidate(document_->authored(),authoringArguments(args));
    if(!inputs.contains("featureId")){auto features=candidate["features"].toArray();for(int i=0;i<features.size();++i){auto feature=features[i].toObject();if(feature["id"]==inputs["sketchId"]){feature["visible"]=false;features[i]=feature;}}candidate["features"]=features;}return candidate;
}
QJsonObject AlloyWindow::beginRevolve(const QJsonObject& overrides) {
    if(!session_.isEmpty())return failure("session_busy","Complete or cancel the active command first");
    QJsonObject inputs{{"extent","full"},{"angleDeg",360},{"name","Revolve"}};
    if(overrides.contains("featureId")){bool found=false;for(const auto& v:document_->authored()["features"].toArray()){const auto f=v.toObject();if(f["id"]!=overrides["featureId"])continue;if(f["type"]!="revolve")return failure("stale_entity","Revolve feature unavailable");inputs=f["parameters"].toObject();inputs["featureId"]=f["id"];inputs["name"]=f["name"];found=true;}if(!found)return failure("stale_entity","Revolve feature unavailable");}
    if(!overrides.contains("featureId") && !overrides.contains("sketchId")) {
        std::vector<const EvaluatedSketch*> choices;
        for(const auto& sketch:document_->sketches())if(sketch.visible && sketch.ownerId==document_->activeOwnerId() && sketch.profiles.size()==1 && (selectedSketch_.isEmpty() || selectedSketch_==sketch.featureId))choices.push_back(&sketch);
        if(choices.size()==1){const auto& sketch=*choices.front();QStringList axes;for(const auto& e:sketch.summary["entities"].toArray()){const auto line=e.toObject();if(line["type"]=="line" && line["centerline"].toBool())axes.append(line["id"].toString());}if(axes.size()==1){inputs["sketchId"]=sketch.featureId;inputs["profileId"]=sketch.profiles.front().profileId;inputs["axisEntityId"]=axes.front();}}
    }
    for(auto it=overrides.begin();it!=overrides.end();++it)inputs[it.key()]=it.value();const auto error=validateArguments(*findCommand("solid.revolveDialog"),inputs);if(!error.isEmpty())return failure("invalid_arguments",error);
    session_={{"id",CadDocument::newId()},{"command","solid.revolve"},{"status","active"},{"inputs",inputs},{"newFeatureId",CadDocument::newId()},{"newBodyId",CadDocument::newId()},{"baseDocumentRevision",static_cast<qint64>(document_->revision())},{"previewed",false},{"geometryEvaluated",false},{"inputSchema",findCommand("solid.revolve")->parameters},{"nativeControlsAvailable",true},{"selectionRole",inputs.contains("profileId") ? (inputs.contains("axisEntityId") ? "ready":"axis"):"profile"}};
    if(inputs.contains("featureId"))view_->setBodyStage(document_->evaluationBefore(inputs["featureId"].toString()).bodies,"revolve-source:"+session_.value("id").toString());showRevolve();previewRevolve();view_->setFocus();return session_;
}
QJsonObject AlloyWindow::previewRevolve() {
    if(session_.value("command")!="solid.revolve")return failure("invalid_session","Revolve is not active");
    session_["previewed"]=false;session_["geometryEvaluated"]=false;session_.remove("geometry");session_.remove("documentDraft");view_->clearPreview();
    auto reject=[this](const QString& code,const QString& message,const QJsonObject& diagnostics=QJsonObject{}){auto r=failure(code,message);auto e=r["error"].toObject();e["diagnostics"]=diagnostics;r["error"]=e;session_["previewError"]=e;refreshRevolveControls();return r;};
    if(session_["baseDocumentRevision"].toInteger()!=static_cast<qint64>(document_->revision()))return reject("stale_session","The document changed; cancel and restart Revolve");
    const auto inputs=session_["inputs"].toObject();view_->setEditDisplay(inputs["sketchId"].toString(),{});view_->selectProfile(inputs["sketchId"].toString(),inputs["profileId"].toString());
    if(!inputs.contains("profileId") || !inputs.contains("axisEntityId"))return reject("selection_required","Select a closed profile, then an authored straight line in the same sketch");
    QJsonObject candidate;try{candidate=revolveCandidate(session_["inputs"].toObject());}catch(const std::exception& e){return reject("invalid_arguments",QString::fromUtf8(e.what()));}
    auto evaluated=CadDocument::evaluateCandidate(candidate);if(!evaluated.ok)return reject("evaluation_failed",evaluated.error,evaluated.diagnostics);
    const auto id=session_["inputs"].toObject().value("featureId").toString(session_["newFeatureId"].toString());auto stage=evaluated.evaluation;
    if(session_["inputs"].toObject().contains("featureId")){QJsonArray prefix;for(const auto& v:candidate["features"].toArray()){prefix.append(v);if(v.toObject()["id"]==id)break;}auto early=candidate;early["features"]=prefix;auto result=CadDocument::evaluateCandidate(early);if(!result.ok)return reject("evaluation_failed",result.error,result.diagnostics);stage=result.evaluation;}
    for(const auto& body:stage->bodies)if(body.featureId==id){view_->showPreview(body.shape,session_["inputs"].toObject().contains("featureId") ? body.bodyId:QString{},true);session_["geometry"]=body.summary;session_["featureId"]=id;session_["bodyId"]=body.bodyId;for(const auto& sketch:stage->sketches)if(sketch.featureId==inputs["sketchId"].toString())for(const auto& profile:sketch.profiles)if(profile.profileId==inputs["profileId"].toString())view_->showRevolveCue(body.summary,profile);}
    session_["documentDraft"]=candidate;session_["previewed"]=true;session_["geometryEvaluated"]=true;session_.remove("previewError");refreshRevolveControls();return session_;
}

EvaluatedDocument AlloyWindow::revolveInputDocument() const {const auto id=session_["inputs"].toObject()["featureId"].toString();return id.isEmpty() ? document_->evaluation():document_->evaluationBefore(id);}
QJsonObject AlloyWindow::pickRevolve(double x,double y){
    auto inputs=session_["inputs"].toObject();
    if(session_["selectionRole"]=="axis"){
        const auto source=revolveInputDocument();for(const auto& sketch:source.sketches)if(sketch.featureId==inputs["sketchId"].toString()){
            QJsonObject feature;for(const auto& v:document_->authored()["features"].toArray())if(v.toObject()["id"]==sketch.featureId)feature=v.toObject();
            const auto hit=view_->pickSketch(feature,x,y,"line",sketch.resolvedFrame);if(hit.isEmpty() || hit["projected"].toBool())return failure("axis_required","Click an authored straight line in the selected profile sketch");
            inputs["axisEntityId"]=hit["entityId"];session_["inputs"]=inputs;session_["selectionRole"]="ready";previewRevolve();auto result=hit;result["sketchId"]=sketch.featureId;result["role"]="axis";result["inputs"]=inputs;return result;
        }return failure("source_unavailable","Select a current profile before its axis");
    }
    const auto hit=view_->pickFinishedProfile(x,y);if(hit.isEmpty())return failure("profile_required","Click a closed sketch profile");
    bool allowed=false;for(const auto& sketch:revolveInputDocument().sketches)if(sketch.featureId==hit["sketchId"].toString())for(const auto& profile:sketch.profiles)if(profile.profileId==hit["profileId"].toString())allowed=true;if(!allowed)return failure("profile_unavailable","Revolve profile must precede this feature");
    if(inputs["sketchId"]!=hit["sketchId"])inputs.remove("axisEntityId");inputs["sketchId"]=hit["sketchId"];inputs["profileId"]=hit["profileId"];session_["inputs"]=inputs;session_["selectionRole"]=inputs.contains("axisEntityId") ? "ready":"axis";previewRevolve();auto result=hit;result["role"]="profile";result["inputs"]=inputs;return result;
}
void AlloyWindow::refreshRevolveControls(){if(!revolveDock_ || session_.value("command")!="solid.revolve")return;const auto inputs=session_["inputs"].toObject();
    auto* profile=revolveDock_->findChild<QPushButton*>("revolve.profile");auto* axis=revolveDock_->findChild<QPushButton*>("revolve.axis");profile->setText(inputs.contains("profileId") ? "1 selected":"Select");axis->setText(inputs.contains("axisEntityId") ? "1 selected":"Select");profile->setToolTip(inputs["sketchId"].toString()+" / "+inputs["profileId"].toString());axis->setToolTip(inputs["axisEntityId"].toString());
    const QSignalBlocker a(profile),b(axis);profile->setChecked(session_["selectionRole"]=="profile");axis->setChecked(session_["selectionRole"]=="axis");
    auto* extent=revolveDock_->findChild<QComboBox*>("revolve.extent");const QSignalBlocker c(extent);extent->setCurrentIndex(extent->findData(inputs["extent"].toString()));auto* angle=revolveDock_->findChild<QDoubleSpinBox*>("revolve.angle");const QSignalBlocker d(angle);angle->setValue(inputs["angleDeg"].toDouble());angle->setVisible(inputs["extent"]=="partial");if(auto* form=qobject_cast<QFormLayout*>(revolveDock_->widget()->layout()))if(auto* label=form->labelForField(angle))label->setVisible(angle->isVisible());
    auto* error=revolveDock_->findChild<QLabel*>("revolve.error");error->setText(session_["previewError"].toObject()["message"].toString());revolveDock_->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->setEnabled(session_["previewed"].toBool());
}
void AlloyWindow::showRevolve(){
    const auto commandSession=session_.value("id").toString();
    if(revolveDock_){canvasWindow_->removeDockWidget(revolveDock_);delete revolveDock_;}revolveDock_=new QDockWidget("Revolve",canvasWindow_);revolveDock_->setObjectName("revolveDock");revolveDock_->setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);auto* panel=new QWidget(revolveDock_);revolveDock_->setWidget(panel);auto* form=new QFormLayout(panel);
    auto* name=new QLineEdit(session_["inputs"].toObject()["name"].toString());name->setObjectName("revolve.name");form->addRow("Name",name);connect(name,&QLineEdit::editingFinished,panel,[this,name,commandSession]{if(session_.value("command")!="solid.revolve" || session_.value("id").toString()!=commandSession)return;execute("command.update",{{"session",session_.value("id")},{"inputs",QJsonObject{{"name",name->text()}}}});});
    for(const auto& role:QStringList{"profile","axis"}){auto* row=new QWidget;auto* layout=new QHBoxLayout(row);layout->setContentsMargins(0,0,0,0);auto* button=new QPushButton("Select");button->setObjectName("revolve."+role);button->setCheckable(true);auto* clear=new QPushButton("×");clear->setObjectName("revolve."+role+".clear");clear->setMaximumWidth(28);clear->setToolTip("Clear "+role);layout->addWidget(button,1);layout->addWidget(clear);form->addRow(role=="profile" ? "Profile":"Axis",row);connect(button,&QPushButton::clicked,panel,[this,role,commandSession]{if(session_.value("id").toString()!=commandSession)return;execute("solid.revolveSelection",{{"role",role}});});connect(clear,&QPushButton::clicked,panel,[this,role,commandSession]{if(session_.value("id").toString()!=commandSession)return;execute("solid.revolveSelection",{{"role",role},{"clear",true}});});}
    auto* extent=new QComboBox;extent->setObjectName("revolve.extent");extent->addItem("Full","full");extent->addItem("Partial","partial");form->addRow("Extent Type",extent);connect(extent,&QComboBox::currentIndexChanged,panel,[this,extent,commandSession]{if(session_.value("command")!="solid.revolve" || session_.value("id").toString()!=commandSession)return;execute("command.update",{{"session",session_.value("id")},{"inputs",QJsonObject{{"extent",extent->currentData().toString()},{"angleDeg",extent->currentData()=="full" ? 360.:90.}}}});previewRevolve();});
    auto* angle=new QDoubleSpinBox;angle->setObjectName("revolve.angle");angle->setDecimals(3);angle->setRange(.001,359.999);angle->setSuffix(" °");angle->setKeyboardTracking(false);form->addRow("Angle",angle);connect(angle,&QDoubleSpinBox::valueChanged,panel,[this,angle,commandSession]{if(session_.value("command")!="solid.revolve" || session_.value("id").toString()!=commandSession)return;execute("command.update",{{"session",session_.value("id")},{"inputs",QJsonObject{{"angleDeg",angle->value()},{"extent","partial"}}}});previewRevolve();});
    auto* projection=new QCheckBox("Project Axis");projection->setObjectName("revolve.projectAxis");projection->setChecked(true);projection->setToolTip("The retained axis is already in the profile sketch plane. Projection is identity in this slice.");projection->setEnabled(false);form->addRow(projection);
    auto* operation=new QComboBox;operation->setObjectName("revolve.operation");operation->addItem("New Body","newBody");form->addRow("Operation",operation);
    auto* instruction=new QLabel("Select a profile and its sketch line. Drag the orange angular control to preview a Partial turn; rotate the view if the ring is edge-on.");instruction->setWordWrap(true);form->addRow(instruction);
    auto* error=new QLabel;error->setObjectName("revolve.error");error->setWordWrap(true);error->setStyleSheet("color:#b3261e");form->addRow(error);auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);form->addRow(buttons);connect(buttons,&QDialogButtonBox::accepted,panel,[this,commandSession]{if(session_.value("id").toString()!=commandSession)return;if(previewRevolve()["previewed"].toBool())execute("command.commit",{{"session",session_.value("id")}});});connect(buttons,&QDialogButtonBox::rejected,panel,[this,commandSession]{if(session_.value("id").toString()!=commandSession)return;execute("command.cancel",{{"session",session_.value("id")}});});canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,revolveDock_);revolveDock_->show();
    if(!revolveEscape_){revolveEscape_=new QShortcut(QKeySequence(Qt::Key_Escape),this);connect(revolveEscape_,&QShortcut::activated,this,[this]{if(session_.value("command")=="solid.revolve")execute("command.cancel",{{"session",session_.value("id")}});});}revolveEscape_->setEnabled(true);refreshRevolveControls();
}
