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
QJsonObject AlloyWindow::pipeCandidate(const QJsonObject& inputs) const {
    auto args=inputs;if(!args.contains("featureId") && session_.value("command")=="solid.pipe"){args["newFeatureId"]=session_.value("newFeatureId");args["newBodyId"]=session_.value("newBodyId");}
    auto candidate=CadDocument::pipeCandidate(document_->authored(),authoringArguments(args));
    if(!inputs.contains("featureId")){auto features=candidate["features"].toArray();for(int i=0;i<features.size();++i){auto f=features[i].toObject();if(f["id"]==inputs["pathSketchId"]){f["visible"]=false;features[i]=f;}}candidate["features"]=features;}return candidate;
}
QJsonObject AlloyWindow::beginPipe(const QJsonObject& overrides) {
    if(!session_.isEmpty())return failure("dialog_busy","Finish or cancel the active command first");
    QJsonObject inputs{{"extent","full"},{"orientation","perpendicular"},{"operation","newBody"},{"name","Pipe"},{"diameterMm",10.},{"wallThicknessMm",1.}};
    if(overrides.contains("featureId")){bool found=false;for(const auto& v:document_->authored()["features"].toArray()){const auto f=v.toObject();if(f["id"]!=overrides["featureId"] || f["type"]!="pipe")continue;inputs=f["parameters"].toObject();inputs["featureId"]=f["id"];inputs["name"]=f["name"];found=true;}if(!found)return failure("feature_unavailable","Pipe feature unavailable");}
    for(auto it=overrides.begin();it!=overrides.end();++it)inputs[it.key()]=it.value();const auto error=validateArguments(*findCommand("solid.pipeDialog"),inputs);if(!error.isEmpty())return failure("invalid_arguments",error);
    session_={{"id",CadDocument::newId()},{"command","solid.pipe"},{"status","active"},{"inputs",inputs},{"newFeatureId",CadDocument::newId()},{"newBodyId",CadDocument::newId()},{"baseDocumentRevision",static_cast<qint64>(document_->revision())},{"previewed",false},{"geometryEvaluated",false},{"inputSchema",findCommand("solid.pipe")->parameters},{"nativeControlsAvailable",true},{"selectionRole","path"},{"chainSelection",true},{"chainSelectionPolicy","connected unbranched component, tangent continuity validated by Pipe; no proximity healing"}};
    if(inputs.contains("featureId"))view_->setBodyStage(document_->evaluationBefore(inputs["featureId"].toString()).bodies,"pipe-source:"+session_.value("id").toString());showPipe();displayPipeSelection();previewPipe();view_->setFocus();return session_;
}
QJsonObject AlloyWindow::previewPipe() {
    if(session_.value("command")!="solid.pipe")return failure("invalid_session","Pipe is not active");
    session_["previewed"]=false;session_["geometryEvaluated"]=false;session_.remove("geometry");session_.remove("documentDraft");view_->clearPreview();
    auto reject=[this](const QString& code,const QString& message,const QJsonObject& diagnostics=QJsonObject{}){auto r=failure(code,message);auto e=r["error"].toObject();e["diagnostics"]=diagnostics;r["error"]=e;session_["previewError"]=e;displayPipeSelection();refreshPipeControls();return r;};
    if(session_.value("baseDocumentRevision").toInteger()!=static_cast<qint64>(document_->revision()))return reject("stale_session","The document changed; cancel and restart Pipe");
    const auto inputs=session_.value("inputs").toObject();if(!inputs.contains("pathEntityIds"))return reject("selection_required","Select one connected open tangent Path");QJsonObject candidate;try{candidate=pipeCandidate(inputs);}catch(const std::exception& e){return reject("invalid_arguments",QString::fromUtf8(e.what()));}
    auto evaluated=CadDocument::evaluateCandidate(candidate);if(!evaluated.ok)return reject("evaluation_failed",evaluated.error,evaluated.diagnostics);
    const auto id=inputs.value("featureId").toString(session_.value("newFeatureId").toString());auto stage=evaluated.evaluation;
    if(inputs.contains("featureId")){QJsonArray prefix;for(const auto& v:candidate["features"].toArray()){prefix.append(v);if(v.toObject()["id"]==id)break;}auto early=candidate;early["features"]=prefix;auto result=CadDocument::evaluateCandidate(early);if(!result.ok)return reject("evaluation_failed",result.error,result.diagnostics);stage=result.evaluation;}
    for(const auto& body:stage->bodies)if(body.featureId==id){view_->showPreview(body.shape,inputs.contains("featureId")?body.bodyId:QString{},true);session_["geometry"]=body.summary;session_["featureId"]=id;session_["bodyId"]=body.bodyId;}
    session_["documentDraft"]=candidate;session_["previewed"]=true;session_["geometryEvaluated"]=true;session_.remove("previewError");displayPipeSelection();refreshPipeControls();return session_;
}

void AlloyWindow::displayPipeSelection(){
    if(session_.value("command")!="solid.pipe")return;const auto available=sweepInputDocument();QSet<QString> visible;for(const auto& s:available.sketches)if(s.ownerId==document_->activeOwnerId())visible.insert(s.featureId);view_->setCommandSketches(visible);
    const auto inputs=session_.value("inputs").toObject();view_->clearSweepPath();
    for(const auto& sketch:available.sketches)if(sketch.featureId==inputs["pathSketchId"])view_->showSweepPath(sketch,inputs["pathEntityIds"].toArray());
}
void AlloyWindow::refreshPipeControls(){
    if(!pipeDock_ || session_.value("command")!="solid.pipe")return;const auto inputs=session_.value("inputs").toObject();
    auto* path=pipeDock_->findChild<QPushButton*>("pipe.path");const QSignalBlocker b(path);path->setText(inputs.contains("pathEntityIds")?QString("%1 selected").arg(inputs["pathEntityIds"].toArray().size()):"Select");path->setChecked(true);path->setToolTip(inputs["pathSketchId"].toString()+" / "+QString::fromUtf8(QJsonDocument(inputs["pathEntityIds"].toArray()).toJson(QJsonDocument::Compact)));
    for(const auto& field:{"diameterMm","wallThicknessMm"}){auto* edit=pipeDock_->findChild<QLineEdit*>("pipe."+QString(field));if(edit && !edit->hasFocus()){const QSignalBlocker block(edit);edit->setText(QString::number(inputs[field].toDouble(),'g',12));}}
    auto* chain=pipeDock_->findChild<QCheckBox*>("pipe.chainSelection");const QSignalBlocker c(chain);chain->setChecked(session_.value("chainSelection").toBool());
    auto* error=pipeDock_->findChild<QLabel*>("pipe.error");error->setText((session_.contains("selectionError")?session_.value("selectionError"):session_.value("previewError")).toObject()["message"].toString());pipeDock_->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->setEnabled(session_.value("previewed").toBool() && !session_.contains("selectionError"));
}
void AlloyWindow::showPipe(){
    const auto commandSession=session_.value("id").toString();if(pipeDock_){canvasWindow_->removeDockWidget(pipeDock_);delete pipeDock_;}pipeDock_=new QDockWidget("Pipe",canvasWindow_);pipeDock_->setObjectName("pipeDock");pipeDock_->setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);auto* panel=new QWidget(pipeDock_);pipeDock_->setWidget(panel);auto* form=new QFormLayout(panel);form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    auto* name=new QLineEdit(session_.value("inputs").toObject()["name"].toString());name->setObjectName("pipe.name");form->addRow("Name",name);connect(name,&QLineEdit::editingFinished,panel,[this,name,commandSession]{if(session_.value("command")!="solid.pipe" || session_.value("id").toString()!=commandSession)return;execute("command.update",{{"session",commandSession},{"inputs",QJsonObject{{"name",name->text()}}}});previewPipe();});
    form->addRow("Section",new QLabel("Circular"));form->addRow("Hollow",new QLabel("On"));
    for(const auto& role:QStringList{"path"}){auto* row=new QWidget;auto* layout=new QHBoxLayout(row);layout->setContentsMargins(0,0,0,0);auto* button=new QPushButton("Select");button->setObjectName("pipe."+role);button->setCheckable(true);auto* clear=new QPushButton("×");clear->setObjectName("pipe."+role+".clear");clear->setMaximumWidth(28);clear->setToolTip("Clear "+role);layout->addWidget(button,1);layout->addWidget(clear);form->addRow(role=="profile"?"Profile":"Path",row);connect(button,&QPushButton::clicked,panel,[this,role,commandSession]{if(session_.value("id").toString()!=commandSession)return;execute("solid.pipeSelection",{{"role",role}});});connect(clear,&QPushButton::clicked,panel,[this,role,commandSession]{if(session_.value("id").toString()!=commandSession)return;execute("solid.pipeSelection",{{"role",role},{"clear",true}});});}
    auto* chain=new QCheckBox("Chain Selection");chain->setObjectName("pipe.chainSelection");chain->setToolTip("Alloy default: checked. Expand the connected unbranched line/arc component; non-tangent junctions are refused. Unchecked: add individual sections. Existing selection is retained when toggled.");form->addRow(chain);connect(chain,&QCheckBox::toggled,panel,[this,chain,commandSession]{if(session_.value("id").toString()!=commandSession)return;execute("solid.pipeSelection",{{"chainSelection",chain->isChecked()}});});
    for(const auto& field:{"diameterMm","wallThicknessMm"}){auto* edit=new QLineEdit(QString::number(session_.value("inputs").toObject()[field].toDouble(),'g',12));edit->setObjectName("pipe."+QString(field));form->addRow(QString(field)=="diameterMm"?"Section Size (mm)":"Section Thickness (mm)",edit);connect(edit,&QLineEdit::editingFinished,panel,[this,edit,field=QString(field),commandSession]{if(session_.value("id").toString()!=commandSession)return;bool ok=false;const auto value=edit->text().toDouble(&ok);if(!ok){session_["previewError"]=QJsonObject{{"code","invalid_number"},{"message","Enter a finite dimension in millimeters"}};session_["previewed"]=false;view_->clearPreview();refreshPipeControls();return;}const auto result=execute("command.update",{{"session",commandSession},{"inputs",QJsonObject{{field,value}}}});if(result["ok"].toBool())previewPipe();});}
    auto* distance=new QLabel("1.00 (Full)");distance->setObjectName("pipe.distance");form->addRow("Distance",distance);form->addRow("Orientation",new QLabel("Perpendicular"));form->addRow("Operation",new QLabel("New Body"));
    auto* instruction=new QLabel("Select a sketch Path section. Chain Selection expands one connected unbranched tangent path. Circular Section Size is the outer diameter in mm; Section Thickness is the wall in mm. Full hollow New Body only.");instruction->setWordWrap(true);form->addRow(instruction);auto* error=new QLabel;error->setObjectName("pipe.error");error->setWordWrap(true);error->setStyleSheet("color:#b3261e");form->addRow(error);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);form->addRow(buttons);connect(buttons,&QDialogButtonBox::accepted,panel,[this,commandSession]{if(session_.value("command")!="solid.pipe" || session_.value("id").toString()!=commandSession)return;if(previewPipe().value("previewed").toBool())execute("command.commit",{{"session",commandSession}});});connect(buttons,&QDialogButtonBox::rejected,panel,[this,commandSession]{if(session_.value("id").toString()!=commandSession)return;execute("command.cancel",{{"session",commandSession}});});canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,pipeDock_);pipeDock_->show();
    if(!pipeEscape_){pipeEscape_=new QShortcut(QKeySequence(Qt::Key_Escape),this);connect(pipeEscape_,&QShortcut::activated,this,[this]{if(session_.value("command")=="solid.pipe")execute("command.cancel",{{"session",session_.value("id")}});});}pipeEscape_->setEnabled(true);refreshPipeControls();
}
