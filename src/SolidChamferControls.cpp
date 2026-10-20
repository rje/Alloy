#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <QDockWidget>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QShortcut>
#include <QComboBox>
#include <QCheckBox>
#include <QSignalBlocker>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
namespace {
QJsonObject chamferFailure(const QString& code,const QString& message,const QJsonObject& diagnostics={}) {
    return {{"ok",false},{"completion","failed"},{"error",QJsonObject{{"code",code},{"message",message},{"diagnostics",diagnostics}}}};
}
}
std::vector<EvaluatedBody> AlloyWindow::chamferInputBodies() const {
    const auto edit=session_["inputs"].toObject()["featureId"].toString();
    return edit.isEmpty() ? document_->bodies() : document_->evaluationBefore(edit).bodies;
}
QJsonObject AlloyWindow::chamferCandidate(const QJsonObject& inputs) const {
    auto args=inputs;if(!args.contains("featureId") && session_.value("command")=="solid.chamfer")args["newFeatureId"]=session_["newFeatureId"];
    return CadDocument::chamferCandidate(document_->authored(),authoringArguments(args));
}
QJsonObject AlloyWindow::beginChamfer(const QJsonObject& overrides) {
    if(!session_.isEmpty())return chamferFailure("session_busy","Complete or cancel the active command first");
    auto error=validateArguments(*findCommand("solid.chamfer"),overrides);if(!error.isEmpty())return chamferFailure("invalid_arguments",error);
    QJsonObject inputs{{"targetBodyId",selectedBody_},{"edgeIds",QJsonArray{}},{"distanceMm",0.0},{"name","Chamfer"}};
    if(overrides.contains("featureId"))for(const auto& value:document_->authored()["features"].toArray())if(value.toObject()["id"]==overrides["featureId"]) {
        const auto feature=value.toObject();if(feature["type"]!="chamfer")return chamferFailure("stale_entity","Chamfer feature unavailable");inputs=feature["parameters"].toObject();inputs["name"]=feature["name"];inputs["featureId"]=feature["id"];
    }
    for(auto it=overrides.begin();it!=overrides.end();++it)inputs[it.key()]=it.value();
    if(inputs.value("mode")=="twoDistance"){if(!inputs.contains("distanceOneMm"))inputs["distanceOneMm"]=1.;if(!inputs.contains("distanceTwoMm"))inputs["distanceTwoMm"]=1.;if(!inputs.contains("flip"))inputs["flip"]=false;}
    session_={{"id",CadDocument::newId()},{"command","solid.chamfer"},{"status","active"},{"inputs",inputs},{"newFeatureId",CadDocument::newId()},{"baseDocumentRevision",static_cast<qint64>(document_->revision())},{"previewed",false},{"inputSchema",findCommand("solid.chamfer")->parameters}};
    const auto source=chamferInputBodies();view_->setBodyStage(source,"chamfer-source:"+session_["id"].toString());view_->selectBody({});selectedFace_={};view_->selectSupportFace({},{});showChamfer();previewChamfer();view_->setFocus();return session_;
}
QJsonObject AlloyWindow::previewChamfer() {
    if(chamferDock_)if(auto* box=chamferDock_->findChild<QDialogButtonBox*>())box->button(QDialogButtonBox::Ok)->setEnabled(false);
    session_["previewed"]=false;session_["geometryEvaluated"]=false;session_.remove("geometry");session_.remove("documentDraft");session_.remove("previewError");view_->clearPreview();
    auto reject=[this](const QString& code,const QString& message,const QJsonObject& diagnostics=QJsonObject{}){auto response=chamferFailure(code,message,diagnostics);session_["previewError"]=response["error"];if(chamferDock_)if(auto* label=chamferDock_->findChild<QLabel*>("chamferError"))label->setText(message);return response;};
    if(session_["baseDocumentRevision"].toInteger()!=static_cast<qint64>(document_->revision()))return reject("stale_session","The document changed; cancel and restart Chamfer");
    auto inputs=session_["inputs"].toObject();const auto source=chamferInputBodies();
    if(chamferDock_){const bool two=inputs.value("mode")=="twoDistance";if(auto* type=chamferDock_->findChild<QComboBox*>("chamferType")){const QSignalBlocker blocker(type);type->setCurrentIndex(two?1:0);}if(auto* fields=chamferDock_->findChild<QWidget*>("chamferTwoDistanceFields"))fields->setVisible(two);if(auto* field=chamferDock_->findChild<QDoubleSpinBox*>("chamferDistance"))field->setVisible(!two);if(auto* all=chamferDock_->findChild<QPushButton*>("chamferSelectAll"))all->setEnabled(!two);}
    // Alloy policy: establish and retain lexical adjacent-face order once.
    if(inputs.value("mode")=="twoDistance" && inputs["referenceFace"].toObject().isEmpty() && inputs["edgeIds"].toArray().size()==1) {
        std::vector<EvaluatedSupportFace> adjacent;
        for(const auto& body:source)if(body.bodyId==inputs["targetBodyId"].toString())for(const auto& edge:body.solidEdges)if(edge.edgeId==inputs["edgeIds"].toArray().first().toString() && !edge.edge.IsNull())for(const auto& face:body.supportFaces){TopTools_IndexedMapOfShape edges;TopExp::MapShapes(face.face,TopAbs_EDGE,edges);if(edges.Contains(edge.edge))adjacent.push_back(face);}
        std::sort(adjacent.begin(),adjacent.end(),[](const auto& a,const auto& b){return a.faceId<b.faceId;});
        if(adjacent.size()==2){const auto& f=adjacent.front();inputs["referenceFace"]=QJsonObject{{"bodyId",f.bodyId},{"producerFeatureId",f.producerFeatureId},{"faceId",f.faceId}};session_["inputs"]=inputs;}
    }
    if(chamferDock_)if(auto* label=chamferDock_->findChild<QLabel*>("chamferReferenceFace")){
        QString description="Select one retained edge";std::vector<EvaluatedSupportFace> faces;
        for(const auto& body:source)if(body.bodyId==inputs["targetBodyId"].toString())for(const auto& edge:body.solidEdges)if(inputs["edgeIds"].toArray().size()==1 && edge.edgeId==inputs["edgeIds"].toArray().first().toString() && !edge.edge.IsNull())for(const auto& f:body.supportFaces){TopTools_IndexedMapOfShape edges;TopExp::MapShapes(f.face,TopAbs_EDGE,edges);if(edges.Contains(edge.edge))faces.push_back(f);}
        if(faces.size()==2){if(faces[1].faceId==inputs["referenceFace"].toObject()["faceId"].toString())std::swap(faces[0],faces[1]);if(inputs["flip"].toBool())std::swap(faces[0],faces[1]);description="Distance 1: "+faces[0].role+"\n"+faces[0].faceId+"\nDistance 2: "+faces[1].role+"\n"+faces[1].faceId;}
        label->setText(description);
    }
    view_->highlightSolidEdges(source,inputs["targetBodyId"].toString(),inputs["edgeIds"].toArray());
    if(chamferDock_)if(auto* label=chamferDock_->findChild<QLabel*>("chamferSelection"))label->setText(QString::number(inputs["edgeIds"].toArray().size())+" selected");
    if(inputs["edgeIds"].toArray().isEmpty())return reject("edges_required","Select solid edges to chamfer");
    QJsonObject candidate;try{candidate=chamferCandidate(inputs);}catch(const std::exception& e){return reject("invalid_arguments",QString::fromUtf8(e.what()));}
    const auto evaluated=CadDocument::evaluateCandidate(candidate);if(!evaluated.ok)return reject("evaluation_failed",evaluated.error,evaluated.diagnostics);
    auto stage=evaluated.evaluation;const auto edit=inputs["featureId"].toString();
    if(!edit.isEmpty()){QJsonArray features;for(const auto& entry:candidate["features"].toArray()){features.append(entry);if(entry.toObject()["id"]==edit)break;}auto prefix=candidate;prefix["features"]=features;const auto early=CadDocument::evaluateCandidate(prefix);if(!early.ok)return reject("evaluation_failed",early.error,early.diagnostics);stage=early.evaluation;}
    for(const auto& body:stage->bodies)if(body.bodyId==inputs["targetBodyId"].toString()){view_->showPreview(body.shape,body.bodyId,true);session_["geometry"]=body.summary;break;}
    if(chamferDock_)if(auto* box=chamferDock_->findChild<QDialogButtonBox*>())box->button(QDialogButtonBox::Ok)->setEnabled(true);
    session_["documentDraft"]=candidate;session_["previewed"]=true;session_["geometryEvaluated"]=true;
    if(chamferDock_)if(auto* label=chamferDock_->findChild<QLabel*>("chamferError"))label->clear();return session_;
}
void AlloyWindow::showChamfer() {
    if(chamferDock_){canvasWindow_->removeDockWidget(chamferDock_);delete chamferDock_;}
    chamferDock_=new QDockWidget("Chamfer",canvasWindow_);chamferDock_->setObjectName("chamferDock");chamferDock_->setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);
    auto* panel=new QWidget(chamferDock_);chamferDock_->setWidget(panel);auto* form=new QFormLayout(panel);
    auto* selected=new QLabel(panel);selected->setObjectName("chamferSelection");form->addRow("Edges",selected);
    auto* clear=new QPushButton("Clear selection",panel);form->addRow(clear);connect(clear,&QPushButton::clicked,panel,[this]{execute("command.update",{{"session",session_["id"]},{"inputs",QJsonObject{{"edgeIds",QJsonArray{}}}}});previewChamfer();});
    auto* all=new QPushButton("Select all edges",panel);all->setObjectName("chamferSelectAll");form->addRow(all);connect(all,&QPushButton::clicked,panel,[this]{auto inputs=session_["inputs"].toObject();auto target=inputs["targetBodyId"].toString();const auto bodies=chamferInputBodies();if(target.isEmpty() && bodies.size()==1)target=bodies.front().bodyId;QJsonArray edges;for(const auto& body:bodies)if(body.bodyId==target)for(const auto& edge:body.solidEdges)if(!edge.edge.IsNull())edges.append(edge.edgeId);execute("command.update",{{"session",session_["id"]},{"inputs",QJsonObject{{"targetBodyId",target},{"edgeIds",edges}}}});previewChamfer();});
    auto* type=new QComboBox(panel);type->setObjectName("chamferType");type->addItem("Equal Distance","equalDistance");type->addItem("Two Distance","twoDistance");type->setCurrentIndex(session_["inputs"].toObject()["mode"]=="twoDistance" ? 1:0);form->addRow("Type",type);auto* distance=new QDoubleSpinBox(panel);distance->setObjectName("chamferDistance");distance->setAccessibleName("Chamfer Distance");distance->setKeyboardTracking(false);distance->setDecimals(3);distance->setRange(0,100000);distance->setSuffix(" mm");distance->setValue(session_["inputs"].toObject()["distanceMm"].toDouble());form->addRow("Distance",distance);
    auto* twoPanel=new QWidget(panel);twoPanel->setObjectName("chamferTwoDistanceFields");auto* twoForm=new QFormLayout(twoPanel);
    for(const auto& key:{"distanceOneMm","distanceTwoMm"}){auto* field=new QDoubleSpinBox(twoPanel);field->setObjectName(QString("chamfer")+key);field->setDecimals(3);field->setRange(0,100000);field->setSuffix(" mm");field->setKeyboardTracking(false);field->setValue(session_["inputs"].toObject().value(key).toDouble(1));twoForm->addRow(QString(key)=="distanceOneMm" ? "Distance 1":"Distance 2",field);connect(field,&QDoubleSpinBox::valueChanged,panel,[this,key](double value){execute("command.update",{{"session",session_["id"]},{"inputs",QJsonObject{{key,value}}}});previewChamfer();});}
    auto* flip=new QCheckBox("Flip",twoPanel);flip->setObjectName("chamferFlip");flip->setChecked(session_["inputs"].toObject()["flip"].toBool());twoForm->addRow(flip);connect(flip,&QCheckBox::toggled,panel,[this](bool value){execute("command.update",{{"session",session_["id"]},{"inputs",QJsonObject{{"flip",value}}}});previewChamfer();});
    auto* reference=new QLabel(twoPanel);reference->setWordWrap(true);reference->setObjectName("chamferReferenceFace");twoForm->addRow(reference);form->addRow(twoPanel);
    twoPanel->setVisible(type->currentIndex()==1);distance->setVisible(type->currentIndex()==0);all->setEnabled(type->currentIndex()==0);
    connect(type,&QComboBox::currentIndexChanged,panel,[this,type,twoPanel,distance,all](int index){twoPanel->setVisible(index==1);distance->setVisible(index==0);all->setEnabled(index==0);auto p=session_["inputs"].toObject();QJsonObject update{{"mode",type->currentData().toString()}};if(index==1){update["distanceOneMm"]=p.value("distanceOneMm").toDouble(1);update["distanceTwoMm"]=p.value("distanceTwoMm").toDouble(1);update["flip"]=p.value("flip").toBool();}execute("command.update",{{"session",session_["id"]},{"inputs",update}});previewChamfer();});
    auto* status=new QLabel(panel);status->setObjectName("chamferError");status->setWordWrap(true);form->addRow(status);
    connect(distance,&QDoubleSpinBox::valueChanged,panel,[this](double value){const auto response=execute("command.update",{{"session",session_["id"]},{"inputs",QJsonObject{{"distanceMm",value}}}});if(response["ok"].toBool())previewChamfer();});
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,panel);form->addRow(buttons);buttons->button(QDialogButtonBox::Ok)->setEnabled(false);connect(buttons,&QDialogButtonBox::accepted,panel,[this,status]{const auto response=execute("command.commit",{{"session",session_["id"]}});if(!response["ok"].toBool())status->setText(response["error"].toObject()["message"].toString());});connect(buttons,&QDialogButtonBox::rejected,panel,[this]{execute("command.cancel",{{"session",session_["id"]}});});
    auto cancel=[this]{if(session_.value("command")=="solid.chamfer")execute("command.cancel",{{"session",session_["id"]}});};auto* escape=new QShortcut(QKeySequence(Qt::Key_Escape),panel);escape->setContext(Qt::WidgetWithChildrenShortcut);connect(escape,&QShortcut::activated,panel,cancel);
    if(!chamferEscape_){chamferEscape_=new QShortcut(QKeySequence(Qt::Key_Escape),view_);chamferEscape_->setContext(Qt::WidgetShortcut);connect(chamferEscape_,&QShortcut::activated,this,cancel);}chamferEscape_->setEnabled(true);canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,chamferDock_);chamferDock_->show();
}
