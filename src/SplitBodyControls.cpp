// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QLabel>
#include <QPushButton>
#include <QShortcut>
#include <QHBoxLayout>
#include <QVBoxLayout>

namespace {
QJsonObject failure(const QString& code,const QString& message,const QString& field={}) {
    QJsonObject error{{"code",code},{"message",message}};
    if(!field.isEmpty())error["field"]=field;
    return {{"ok",false},{"completion","failed"},{"error",error}};
}
}

QJsonObject AlloyWindow::beginSplitBody(const QJsonObject& overrides) {
    if(!session_.isEmpty())return failure("session_busy","Complete or cancel the active command first");
    QJsonObject inputs{{"targetBodyId",selectedBody_},{"planeId",selectedPlane_},{"name","Split Body"}};
    if(overrides.contains("featureId")) {
        bool found=false;
        for(const auto& entry:document_->authored()["features"].toArray()) {
            const auto feature=entry.toObject();
            if(feature["id"]!=overrides["featureId"])continue;
            if(feature["type"]!="splitBody")return failure("stale_entity","Split Body feature is unavailable","featureId");
            inputs=feature["parameters"].toObject();inputs["featureId"]=feature["id"];
            inputs["name"]=feature["name"];found=true;break;
        }
        if(!found)return failure("stale_entity","Split Body feature is unavailable","featureId");
    }
    for(auto it=overrides.begin();it!=overrides.end();++it)inputs[it.key()]=it.value();
    if(!inputs.contains("featureId"))inputs["newFeatureId"]=CadDocument::newId();
    session_={{"id",CadDocument::newId()},{"command","solid.splitBody"},{"status","active"},
        {"inputs",inputs},{"previewed",false},{"baseDocumentRevision",static_cast<qint64>(document_->revision())},
        {"inputSchema",findCommand("solid.splitBody")->parameters},{"selectionRole",inputs["targetBodyId"].toString().isEmpty()?"body":"plane"},
        {"previousBody",selectedBody_},{"previousPlane",selectedPlane_}};
    const auto edit=inputs["featureId"].toString();
    if(!edit.isEmpty())view_->setBodyStage(document_->evaluationBefore(edit).bodies,"split-source:"+session_["id"].toString());
    showSplitBody();previewSplitBody();return session_;
}

QJsonObject AlloyWindow::selectSplitBody(const QString& id) {
    if(session_.value("command")!="solid.splitBody")return failure("invalid_session","Split Body is not active");
    const auto edit=session_["inputs"].toObject()["featureId"].toString();
    const auto available=edit.isEmpty()?document_->bodies():document_->evaluationBefore(edit).bodies;
    bool found=false;
    for(const auto& body:available)if(body.bodyId==id && body.visible && body.ownerId==document_->activeOwnerId() && body.occurrenceId.isEmpty())found=true;
    if(!found)return failure("stale_entity","Choose an available local body at this history stage","targetBodyId");
    const auto response=execute("command.update",{{"session",session_["id"]},{"inputs",QJsonObject{{"targetBodyId",id}}}});
    if(!response["ok"].toBool()){
        const auto error=response["error"].toObject();return failure(error["code"].toString(),error["message"].toString());
    }
    session_["selectionRole"]="plane";return session_;
}

QJsonObject AlloyWindow::selectSplitPlane(const QString& id) {
    if(session_.value("command")!="solid.splitBody")return failure("invalid_session","Split Body is not active");
    const auto edit=session_["inputs"].toObject()["featureId"].toString();
    const auto available=edit.isEmpty()?document_->evaluation().constructionPlanes:document_->evaluationBefore(edit).constructionPlanes;
    bool found=false;
    for(const auto& plane:available)if(plane.featureId==id && plane.visible && plane.ownerId==document_->activeOwnerId())found=true;
    if(!found)return failure("plane_unavailable","Choose an available construction plane at this history stage","planeId");
    const auto response=execute("command.update",{{"session",session_["id"]},{"inputs",QJsonObject{{"planeId",id}}}});
    if(!response["ok"].toBool()){
        const auto error=response["error"].toObject();return failure(error["code"].toString(),error["message"].toString());
    }
    return session_;
}

QJsonObject AlloyWindow::previewSplitBody() {
    if(session_.value("command")!="solid.splitBody")return failure("invalid_session","Split Body is not active");
    auto* bodyLabel=splitBodyDock_?splitBodyDock_->findChild<QLabel*>("split.body"):nullptr;
    auto* planeLabel=splitBodyDock_?splitBodyDock_->findChild<QLabel*>("split.plane"):nullptr;
    auto* status=splitBodyDock_?splitBodyDock_->findChild<QLabel*>("split.status"):nullptr;
    auto* buttons=splitBodyDock_?splitBodyDock_->findChild<QDialogButtonBox*>("split.buttons"):nullptr;
    const auto inputs=session_["inputs"].toObject();
    if(bodyLabel)bodyLabel->setText(QString("Body to Split: ")+(inputs["targetBodyId"].toString().isEmpty()?"Select a body":"Selected"));
    if(planeLabel)planeLabel->setText(QString("Splitting Tool: ")+(inputs["planeId"].toString().isEmpty()?"Select a construction plane":"Selected"));
    session_["previewed"]=false;session_["geometryEvaluated"]=false;session_.remove("documentDraft");session_.remove("geometry");view_->clearPreview();
    if(buttons)buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
    if(inputs["targetBodyId"].toString().isEmpty() || inputs["planeId"].toString().isEmpty()) {
        if(status)status->setText("Select the body, then the construction plane.");return session_;
    }
    if(session_.value("baseDocumentRevision").toInteger()!=static_cast<qint64>(document_->revision()))
        return failure("stale_session","The document changed; cancel and restart Split Body");
    try {
        const auto candidate=CadDocument::splitBodyCandidate(document_->authored(),inputs);
        const auto prepared=CadDocument::evaluateCandidate(candidate);
        if(!prepared.ok)throw std::runtime_error(prepared.error.toStdString());
        const auto target=inputs["targetBodyId"].toString();QString positive;
        const auto featureId=inputs.contains("featureId")?inputs["featureId"]:inputs["newFeatureId"];
        for(const auto& entry:candidate["features"].toArray())if(entry.toObject()["id"]==featureId)positive=entry.toObject()["bodyId"].toString();
        TopoDS_Compound compound;BRep_Builder builder;builder.MakeCompound(compound);QJsonArray geometry;
        for(const auto& body:prepared.evaluation->bodies)if(body.bodyId==target || body.bodyId==positive){builder.Add(compound,body.shape);geometry.append(body.summary);}
        if(geometry.size()!=2)throw std::runtime_error("Split preview did not produce two bodies");
        view_->showPreview(compound,target,true);session_["previewed"]=true;session_["geometryEvaluated"]=true;
        session_["documentDraft"]=candidate;session_["geometry"]=geometry;
        if(status)status->setText("Two closed bodies previewed. OK commits both.");
        if(buttons)buttons->button(QDialogButtonBox::Ok)->setEnabled(true);
        return session_;
    }catch(const std::exception& error){
        if(status)status->setText(QString::fromUtf8(error.what()));
        return failure("evaluation_failed",QString::fromUtf8(error.what()));
    }
}

void AlloyWindow::showSplitBody() {
    if(splitBodyDock_)delete splitBodyDock_;
    splitBodyDock_=new QDockWidget("Split Body",canvasWindow_);splitBodyDock_->setObjectName("splitBodyDock");
    splitBodyDock_->setAllowedAreas(Qt::LeftDockWidgetArea|Qt::RightDockWidgetArea);
    auto* panel=new QWidget(splitBodyDock_);panel->setFixedWidth(284);splitBodyDock_->setWidget(panel);
    auto* layout=new QVBoxLayout(panel);layout->setContentsMargins(14,14,14,12);layout->setSpacing(12);
    auto* instructions=new QLabel("Select a solid body, then a retained construction plane. The plane extends through the body.",panel);
    instructions->setWordWrap(true);layout->addWidget(instructions);
    auto* body=new QLabel(panel);body->setObjectName("split.body");body->setWordWrap(true);layout->addWidget(body);
    auto* bodyActions=new QHBoxLayout;auto* chooseBody=new QPushButton("Select body",panel);chooseBody->setObjectName("split.selectBody");
    auto* clearBody=new QPushButton("Clear",panel);clearBody->setObjectName("split.clearBody");
    bodyActions->addWidget(chooseBody);bodyActions->addWidget(clearBody);layout->addLayout(bodyActions);
    auto* plane=new QLabel(panel);plane->setObjectName("split.plane");plane->setWordWrap(true);layout->addWidget(plane);
    auto* planeActions=new QHBoxLayout;auto* choosePlane=new QPushButton("Select plane",panel);choosePlane->setObjectName("split.selectPlane");
    auto* clearPlane=new QPushButton("Clear",panel);clearPlane->setObjectName("split.clearPlane");
    planeActions->addWidget(choosePlane);planeActions->addWidget(clearPlane);layout->addLayout(planeActions);
    connect(chooseBody,&QPushButton::clicked,this,[this]{session_["selectionRole"]="body";view_->setFocus(Qt::OtherFocusReason);});
    connect(choosePlane,&QPushButton::clicked,this,[this]{session_["selectionRole"]="plane";view_->setFocus(Qt::OtherFocusReason);});
    connect(clearBody,&QPushButton::clicked,this,[this]{
        execute("command.update",{{"session",session_["id"]},{"inputs",QJsonObject{{"targetBodyId",""}}}});session_["selectionRole"]="body";view_->setFocus(Qt::OtherFocusReason);
    });
    connect(clearPlane,&QPushButton::clicked,this,[this]{
        execute("command.update",{{"session",session_["id"]},{"inputs",QJsonObject{{"planeId",""}}}});session_["selectionRole"]="plane";view_->setFocus(Qt::OtherFocusReason);
    });
    auto* status=new QLabel(panel);status->setObjectName("split.status");status->setWordWrap(true);status->setMinimumHeight(48);layout->addWidget(status);
    layout->addStretch();
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,panel);buttons->setObjectName("split.buttons");
    buttons->button(QDialogButtonBox::Ok)->setEnabled(false);layout->addWidget(buttons);
    connect(buttons,&QDialogButtonBox::accepted,this,[this,status]{const auto response=execute("command.commit",{{"session",session_["id"]}});
        if(!response["ok"].toBool())status->setText(response["error"].toObject()["message"].toString());});
    auto cancel=[this]{if(session_.value("command")=="solid.splitBody")execute("command.cancel",{{"session",session_["id"]}});};
    connect(buttons,&QDialogButtonBox::rejected,this,cancel);
    auto* escape=new QShortcut(QKeySequence(Qt::Key_Escape),panel);escape->setContext(Qt::WidgetWithChildrenShortcut);connect(escape,&QShortcut::activated,this,cancel);
    canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,splitBodyDock_);splitBodyDock_->show();view_->setFocus(Qt::OtherFocusReason);
}
