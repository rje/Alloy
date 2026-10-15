#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <QDockWidget>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QDoubleSpinBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QShortcut>
#include <QSignalBlocker>
#include <algorithm>
namespace {
QJsonObject filletFailure(const QString& code,const QString& message,const QJsonObject& diagnostics={}) {
    return {{"ok",false},{"completion","failed"},{"error",QJsonObject{{"code",code},{"message",message},{"diagnostics",diagnostics}}}};
}
QJsonArray filletGroups(const QJsonObject& inputs){
    if(inputs.contains("groups"))return inputs["groups"].toArray();
    return QJsonArray{QJsonObject{{"id","legacy"},{"edgeIds",inputs["edgeIds"]},
        {"radiusMm",inputs["radiusMm"]},{"tangentChain",true}}};
}
QJsonArray allFilletEdges(const QJsonArray& groups){QJsonArray edges;
    for(const auto& value:groups)for(const auto& id:value.toObject()["edgeIds"].toArray())edges.append(id);
    return edges;
}
}
std::vector<EvaluatedBody> AlloyWindow::filletInputBodies() const {
    const auto edit=session_["inputs"].toObject()["featureId"].toString();
    return edit.isEmpty() ? document_->bodies() : document_->evaluationBefore(edit).bodies;
}
QJsonObject AlloyWindow::filletCandidate(const QJsonObject& inputs) const {
    auto args=inputs;if(!args.contains("featureId") && session_.value("command")=="solid.fillet")args["newFeatureId"]=session_["newFeatureId"];
    return CadDocument::filletCandidate(document_->authored(),authoringArguments(args));
}
QJsonObject AlloyWindow::beginFillet(const QJsonObject& overrides) {
    if(!session_.isEmpty())return filletFailure("session_busy","Complete or cancel the active command first");
    auto error=validateArguments(*findCommand("solid.fillet"),overrides);if(!error.isEmpty())return filletFailure("invalid_arguments",error);
    if(overrides.contains("tangentChain") && !overrides["tangentChain"].toBool())return filletFailure("unsupported_option","Tangent Chain Off is outside this slice");
    QJsonObject inputs{{"targetBodyId",selectedBody_},{"edgeIds",QJsonArray{}},{"radiusMm",0.0},{"tangentChain",true},{"name","Fillet"}};
    if(overrides.contains("featureId"))for(const auto& value:document_->authored()["features"].toArray())if(value.toObject()["id"]==overrides["featureId"]) {
        const auto feature=value.toObject();if(feature["type"]!="fillet")return filletFailure("stale_entity","Fillet feature unavailable");inputs=feature["parameters"].toObject();inputs["name"]=feature["name"];inputs["featureId"]=feature["id"];
    }
    for(auto it=overrides.begin();it!=overrides.end();++it)inputs[it.key()]=it.value();
    if(overrides.contains("groups")){inputs.remove("edgeIds");inputs.remove("radiusMm");inputs.remove("tangentChain");}
    if(inputs.contains("groups") && (inputs.contains("edgeIds") || inputs.contains("radiusMm") || inputs.contains("tangentChain")))
        return filletFailure("invalid_arguments","Choose grouped or single-radius Fillet inputs");
    session_={{"id",CadDocument::newId()},{"command","solid.fillet"},{"status","active"},{"inputs",inputs},{"newFeatureId",CadDocument::newId()},{"baseDocumentRevision",static_cast<qint64>(document_->revision())},{"previewed",false},{"inputSchema",findCommand("solid.fillet")->parameters}};
    session_["filletActiveGroupIndex"]=0;
    const auto source=filletInputBodies();view_->setBodyStage(source,"fillet-source:"+session_["id"].toString());view_->selectBody({});selectedFace_={};view_->selectSupportFace({},{});showFillet();previewFillet();view_->setFocus();return session_;
}
QJsonObject AlloyWindow::previewFillet() {
    if(filletDock_)if(auto* box=filletDock_->findChild<QDialogButtonBox*>())box->button(QDialogButtonBox::Ok)->setEnabled(false);
    session_["previewed"]=false;session_["geometryEvaluated"]=false;session_.remove("geometry");session_.remove("documentDraft");session_.remove("previewError");view_->clearPreview();
    auto reject=[this](const QString& code,const QString& message,const QJsonObject& diagnostics=QJsonObject{}){auto response=filletFailure(code,message,diagnostics);session_["previewError"]=response["error"];if(filletDock_)if(auto* label=filletDock_->findChild<QLabel*>("filletError"))label->setText(message);return response;};
    if(session_["baseDocumentRevision"].toInteger()!=static_cast<qint64>(document_->revision()))return reject("stale_session","The document changed; cancel and restart Fillet");
    const auto inputs=session_["inputs"].toObject();const auto source=filletInputBodies();const auto groups=filletGroups(inputs);
    view_->highlightSolidEdges(source,inputs["targetBodyId"].toString(),allFilletEdges(groups));refreshFilletControls();
    if(groups.isEmpty() || allFilletEdges(groups).isEmpty())return reject("edges_required","Select solid edges to fillet");
    QJsonObject candidate;try{candidate=filletCandidate(inputs);}catch(const std::exception& e){return reject("invalid_arguments",QString::fromUtf8(e.what()));}
    const auto evaluated=CadDocument::evaluateCandidate(candidate);if(!evaluated.ok)return reject("evaluation_failed",evaluated.error,evaluated.diagnostics);
    auto stage=evaluated.evaluation;const auto edit=inputs["featureId"].toString();
    if(!edit.isEmpty()){QJsonArray features;for(const auto& entry:candidate["features"].toArray()){features.append(entry);if(entry.toObject()["id"]==edit)break;}auto prefix=candidate;prefix["features"]=features;const auto early=CadDocument::evaluateCandidate(prefix);if(!early.ok)return reject("evaluation_failed",early.error,early.diagnostics);stage=early.evaluation;}
    for(const auto& body:stage->bodies)if(body.bodyId==inputs["targetBodyId"].toString()){view_->showPreview(body.shape,body.bodyId,true);session_["geometry"]=body.summary;break;}
    if(filletDock_)if(auto* box=filletDock_->findChild<QDialogButtonBox*>())box->button(QDialogButtonBox::Ok)->setEnabled(true);
    session_["documentDraft"]=candidate;session_["previewed"]=true;session_["geometryEvaluated"]=true;
    if(filletDock_)if(auto* label=filletDock_->findChild<QLabel*>("filletError"))label->clear();return session_;
}
void AlloyWindow::showFillet() {
    if(filletDock_){canvasWindow_->removeDockWidget(filletDock_);delete filletDock_;}
    filletDock_=new QDockWidget("Fillet",canvasWindow_);filletDock_->setObjectName("filletDock");filletDock_->setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);
    auto* panel=new QWidget(filletDock_);filletDock_->setWidget(panel);auto* form=new QFormLayout(panel);form->setAlignment(Qt::AlignTop);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    auto* row=new QWidget(panel);row->setFixedHeight(82);auto* rowLayout=new QHBoxLayout(row);rowLayout->setContentsMargins(0,0,0,0);
    auto* groups=new QListWidget(row);groups->setObjectName("fillet.groups");groups->setAccessibleName("Fillet selection sets");
    groups->setFixedHeight(82);
    auto* add=new QPushButton("+",row);add->setObjectName("fillet.addGroup");add->setToolTip("Add selection set");
    auto* remove=new QPushButton("×",row);remove->setObjectName("fillet.removeGroup");remove->setToolTip("Remove active selection set");
    rowLayout->addWidget(groups,1);rowLayout->addWidget(add);rowLayout->addWidget(remove);form->addRow("Selection Sets",row);
    auto* selected=new QLabel(panel);selected->setObjectName("filletSelection");form->addRow("Edges",selected);
    auto* clear=new QPushButton("Clear active selection",panel);clear->setObjectName("fillet.clearGroup");form->addRow(clear);
    form->addRow("Type",new QLabel("Constant Radius",panel));auto* radius=new QDoubleSpinBox(panel);radius->setObjectName("filletRadius");radius->setAccessibleName("Fillet Radius");radius->setKeyboardTracking(false);radius->setDecimals(3);radius->setRange(0,100000);radius->setSuffix(" mm");form->addRow("Radius",radius);
    form->addRow("Tangent Chain",new QLabel("On",panel));form->addRow("Continuity",new QLabel("Tangent (G1)",panel));form->addRow("Corner Type",new QLabel("Rolling Ball",panel));
    auto* status=new QLabel(panel);status->setObjectName("filletError");status->setWordWrap(true);form->addRow(status);
    auto updateGroups=[this,status](const QJsonArray& values){const auto response=execute("command.update",{{"session",session_["id"]},{"inputs",QJsonObject{{"groups",values}}}});
        if(response["ok"].toBool())execute("command.preview",{{"session",session_.value("id")}});else status->setText(response["error"].toObject()["message"].toString());};
    connect(groups,&QListWidget::currentRowChanged,panel,[this](int index){if(index<0)return;session_["filletActiveGroupIndex"]=index;refreshFilletControls();});
    connect(add,&QPushButton::clicked,panel,[this,updateGroups]{auto values=filletGroups(session_["inputs"].toObject());
        if(values[0].toObject()["id"]=="legacy"){auto first=values[0].toObject();first["id"]=CadDocument::newId();values[0]=first;}
        values.append(QJsonObject{{"id",CadDocument::newId()},{"edgeIds",QJsonArray{}},{"radiusMm",0.0},{"tangentChain",true}});
        session_["filletActiveGroupIndex"]=values.size()-1;updateGroups(values);});
    connect(remove,&QPushButton::clicked,panel,[this,updateGroups]{auto values=filletGroups(session_["inputs"].toObject());
        if(values.size()<2)return;const int index=std::clamp(session_["filletActiveGroupIndex"].toInt(),0,int(values.size())-1);
        values.removeAt(index);session_["filletActiveGroupIndex"]=std::min(index,int(values.size())-1);updateGroups(values);});
    connect(clear,&QPushButton::clicked,panel,[this,updateGroups]{if(!session_["inputs"].toObject().contains("groups")){
            const auto response=execute("command.update",{{"session",session_["id"]},{"inputs",QJsonObject{{"edgeIds",QJsonArray{}}}}});
            if(response["ok"].toBool())execute("command.preview",{{"session",session_.value("id")}});return;}
        auto values=filletGroups(session_["inputs"].toObject());
        if(values.isEmpty())return;const int index=std::clamp(session_["filletActiveGroupIndex"].toInt(),0,int(values.size())-1);
        auto group=values[index].toObject();group["edgeIds"]=QJsonArray{};values[index]=group;updateGroups(values);});
    connect(radius,&QDoubleSpinBox::valueChanged,panel,[this,updateGroups](double value){if(!session_["inputs"].toObject().contains("groups")){
            const auto response=execute("command.update",{{"session",session_["id"]},{"inputs",QJsonObject{{"radiusMm",value}}}});
            if(response["ok"].toBool())execute("command.preview",{{"session",session_.value("id")}});return;}
        auto values=filletGroups(session_["inputs"].toObject());
        if(values.isEmpty())return;const int index=std::clamp(session_["filletActiveGroupIndex"].toInt(),0,int(values.size())-1);
        auto group=values[index].toObject();group["radiusMm"]=value;values[index]=group;updateGroups(values);});
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,panel);form->addRow(buttons);buttons->button(QDialogButtonBox::Ok)->setEnabled(false);connect(buttons,&QDialogButtonBox::accepted,panel,[this,status]{const auto response=execute("command.commit",{{"session",session_["id"]}});if(!response["ok"].toBool())status->setText(response["error"].toObject()["message"].toString());});connect(buttons,&QDialogButtonBox::rejected,panel,[this]{execute("command.cancel",{{"session",session_["id"]}});});
    auto cancel=[this]{if(session_.value("command")=="solid.fillet")execute("command.cancel",{{"session",session_["id"]}});};auto* escape=new QShortcut(QKeySequence(Qt::Key_Escape),panel);escape->setContext(Qt::WidgetWithChildrenShortcut);connect(escape,&QShortcut::activated,panel,cancel);
    if(!filletEscape_){filletEscape_=new QShortcut(QKeySequence(Qt::Key_Escape),view_);filletEscape_->setContext(Qt::WidgetShortcut);connect(filletEscape_,&QShortcut::activated,this,cancel);}filletEscape_->setEnabled(true);canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,filletDock_);filletDock_->show();refreshFilletControls();
}
void AlloyWindow::refreshFilletControls(){
    if(!filletDock_ || session_.value("command")!="solid.fillet")return;
    const auto groups=filletGroups(session_["inputs"].toObject());if(groups.isEmpty())return;
    const int active=std::clamp(session_["filletActiveGroupIndex"].toInt(),0,int(groups.size())-1);
    session_["filletActiveGroupIndex"]=active;
    if(auto* list=filletDock_->findChild<QListWidget*>("fillet.groups")){
        const QSignalBlocker block(list);list->clear();
        for(int i=0;i<groups.size();++i){const auto group=groups[i].toObject();
            const int count=group["edgeIds"].toArray().size();
            auto* item=new QListWidgetItem(QString("Set %1   %2 %3   R %4 mm").arg(i+1).arg(count).arg(count==1 ? "edge" : "edges").arg(group["radiusMm"].toDouble()),list);
            item->setData(Qt::UserRole,group["id"]);}
        list->setCurrentRow(active);
    }
    const auto group=groups[active].toObject();
    if(auto* selected=filletDock_->findChild<QLabel*>("filletSelection"))selected->setText(QString::number(group["edgeIds"].toArray().size())+" in active set");
    if(auto* radius=filletDock_->findChild<QDoubleSpinBox*>("filletRadius")){const QSignalBlocker block(radius);radius->setValue(group["radiusMm"].toDouble());}
    if(auto* remove=filletDock_->findChild<QPushButton*>("fillet.removeGroup"))remove->setEnabled(groups.size()>1);
}
