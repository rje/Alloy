// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QShortcut>
#include <QSignalBlocker>

namespace {
QJsonObject refused(const QString& code,const QString& message,const QJsonObject& diagnostics={}){
    return {{"ok",false},{"completion","failed"},
        {"error",QJsonObject{{"code",code},{"message",message},{"diagnostics",diagnostics}}}};
}
}

QJsonObject AlloyWindow::beginFeatureMirror(const QJsonObject& overrides){
    if(!session_.isEmpty())return refused("session_busy","Complete or cancel the active command first");
    const auto error=validateArguments(*findCommand("solid.featureMirror"),overrides);
    if(!error.isEmpty())return refused("invalid_arguments",error);
    QJsonObject inputs{{"targetBodyId",selectedBody_},{"sourceFeatureIds",QJsonArray{}},
        {"mirrorPlane",QJsonObject{}},{"name","Mirror Features"}};
    if(overrides.contains("featureId")){
        bool found=false;
        for(const auto& entry:document_->authored()["features"].toArray()){
            const auto feature=entry.toObject();
            if(feature["id"]!=overrides["featureId"])continue;
            if(feature["type"]!="featureMirror")return refused("stale_entity","Feature Mirror is unavailable");
            inputs=feature["parameters"].toObject();inputs["featureId"]=feature["id"];
            inputs["name"]=feature["name"];found=true;break;
        }
        if(!found)return refused("stale_entity","Feature Mirror is unavailable");
    }
    for(auto it=overrides.begin();it!=overrides.end();++it)inputs[it.key()]=it.value();
    session_={{"id",CadDocument::newId()},{"command","solid.featureMirror"},{"status","active"},
        {"inputs",inputs},{"newFeatureId",CadDocument::newId()},
        {"baseDocumentRevision",static_cast<qint64>(document_->revision())},
        {"previewed",false},{"geometryEvaluated",false},
        {"inputSchema",findCommand("solid.featureMirror")->parameters}};
    if(!inputs["featureId"].toString().isEmpty())
        view_->setBodyStage(document_->evaluationBefore(inputs["featureId"].toString()).bodies,
            "feature-mirror-source:"+session_["id"].toString());
    showFeatureMirror();previewFeatureMirror();view_->setFocus(Qt::OtherFocusReason);
    return session_;
}

QJsonObject AlloyWindow::selectFeatureMirrorSource(const QString& featureId){
    if(session_.value("command")!="solid.featureMirror")return refused("invalid_session","Feature Mirror is not active");
    const auto inputs=session_["inputs"].toObject();
    const auto edit=inputs["featureId"].toString();
    bool beforeEdit=true,eligible=false;
    QString target;
    QJsonArray selected=inputs["sourceFeatureIds"].toArray();
    const auto features=document_->authored()["features"].toArray();
    QString mode;
    if(!selected.isEmpty())for(const auto& entry:features)if(entry.toObject()["id"]==selected.first()){
        const auto first=entry.toObject();
        mode=first["parameters"].toObject()["operation"]=="join" ? "snap" : "cut";
        break;
    }
    for(const auto& entry:features){
        const auto feature=entry.toObject();
        if(feature["id"]==edit)beforeEdit=false;
        if(feature["id"]!=featureId)continue;
        const auto p=feature["parameters"].toObject();const auto targets=p["targetBodyIds"].toArray();
        const bool cut=feature["type"]=="extrude" && p["operation"]=="cut" && targets.size()==1;
        const bool join=feature["type"]=="extrude" && p["operation"]=="join" && targets.size()==1;
        const bool finish=feature["type"]=="chamfer" || feature["type"]=="fillet";
        eligible=beforeEdit && feature["ownerId"]==document_->activeOwnerId()
            && ((mode=="cut" && cut) || (mode=="snap" && (join || finish))
                || (mode.isEmpty() && (cut || join)));
        if(eligible)target=finish ? p["targetBodyId"].toString() : targets.first().toString();
    }
    if(!eligible)return refused("mirror_source_unsupported",
        "Select an earlier Cut, or a Join followed by its three Chamfers and Fillet in the Timeline");
    if(!selected.isEmpty() && inputs["targetBodyId"]!=target)
        return refused("mirror_target_mismatch","Selected feature modifies a different target body");
    if(selected.contains(featureId)){
        for(int i=0;i<selected.size();++i)if(selected[i]==featureId){selected.removeAt(i);break;}
    }
    else selected.append(featureId);
    QJsonArray ordered;
    for(const auto& entry:features)
        if(selected.contains(entry.toObject()["id"]))ordered.append(entry.toObject()["id"]);
    auto changed=inputs;changed["sourceFeatureIds"]=ordered;
    changed["targetBodyId"]=ordered.isEmpty() ? QJsonValue{} : QJsonValue(target);
    session_["inputs"]=changed;
    return previewFeatureMirror();
}

QJsonObject AlloyWindow::previewFeatureMirror(){
    session_["previewed"]=false;session_["geometryEvaluated"]=false;
    session_.remove("geometry");session_.remove("documentDraft");view_->clearPreview();
    const auto inputs=session_["inputs"].toObject();
    if(featureMirrorDock_){
        if(auto* label=featureMirrorDock_->findChild<QLabel*>("featureMirror.objects"))
            label->setText(QString("%1 selected in Timeline").arg(inputs["sourceFeatureIds"].toArray().size()));
        if(auto* combo=featureMirrorDock_->findChild<QComboBox*>("featureMirror.plane")){
            const QSignalBlocker block(combo);
            const auto plane=inputs["mirrorPlane"].toObject()["plane"].toString();
            combo->setCurrentIndex(plane=="XZ" ? 1 : plane=="YZ" ? 2 : 0);
        }
        if(auto* buttons=featureMirrorDock_->findChild<QDialogButtonBox*>())
            buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
    auto reject=[this](const QString& code,const QString& message,const QJsonObject& details=QJsonObject{}){
        const auto response=refused(code,message,details);session_["previewError"]=response["error"];
        if(featureMirrorDock_)if(auto* label=featureMirrorDock_->findChild<QLabel*>("featureMirror.status"))label->setText(message);
        return response;
    };
    if(session_["baseDocumentRevision"].toInteger()!=static_cast<qint64>(document_->revision()))
        return reject("stale_session","The document changed; cancel and restart Mirror");
    if(inputs["sourceFeatureIds"].toArray().isEmpty())return reject("objects_required","Select Cut features or a Join and its finishes in the Timeline");
    const auto plane=inputs["mirrorPlane"].toObject()["plane"].toString();
    if(plane!="XZ" && plane!="YZ")return reject("plane_required","Select Origin XZ or YZ as Mirror Plane");
    auto args=inputs;if(!args.contains("featureId"))args["newFeatureId"]=session_["newFeatureId"];
    QJsonObject candidate;
    try{candidate=CadDocument::featureMirrorCandidate(document_->authored(),authoringArguments(args));}
    catch(const std::exception& error){return reject("invalid_arguments",QString::fromUtf8(error.what()));}
    const auto prepared=document_->prepareCandidate(candidate);
    if(!prepared.ok)return reject("evaluation_failed",prepared.error,prepared.diagnostics);
    auto staged=prepared.evaluation;
    const auto edit=inputs["featureId"].toString();
    if(!edit.isEmpty()){
        QJsonArray prefix;
        for(const auto& entry:candidate["features"].toArray()){
            prefix.append(entry);if(entry.toObject()["id"]==edit)break;
        }
        auto trimmed=candidate;trimmed["features"]=prefix;
        const auto result=CadDocument::evaluateCandidate(trimmed);
        if(!result.ok)return reject("evaluation_failed",result.error,result.diagnostics);
        staged=result.evaluation;
    }
    bool found=false;
    for(const auto& body:staged->bodies)if(body.bodyId==inputs["targetBodyId"]){
        view_->showPreview(body.shape,body.bodyId,true);session_["geometry"]=body.summary;found=true;break;
    }
    if(!found)return reject("body_unavailable","Mirrored target body is unavailable");
    session_["documentDraft"]=candidate;session_["previewed"]=true;session_["geometryEvaluated"]=true;
    session_.remove("previewError");
    if(featureMirrorDock_){
        if(auto* label=featureMirrorDock_->findChild<QLabel*>("featureMirror.status"))label->setText("Mirrored feature preview is valid");
        if(auto* buttons=featureMirrorDock_->findChild<QDialogButtonBox*>())
            buttons->button(QDialogButtonBox::Ok)->setEnabled(true);
    }
    return session_;
}

void AlloyWindow::showFeatureMirror(){
    if(featureMirrorDock_){canvasWindow_->removeDockWidget(featureMirrorDock_);delete featureMirrorDock_;}
    featureMirrorDock_=new QDockWidget("Mirror",canvasWindow_);
    featureMirrorDock_->setObjectName("featureMirrorDock");
    featureMirrorDock_->setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);
    auto* panel=new QWidget(featureMirrorDock_);panel->setMinimumWidth(240);featureMirrorDock_->setWidget(panel);
    auto* form=new QFormLayout(panel);form->setRowWrapPolicy(QFormLayout::WrapLongRows);
    form->addRow("Pattern Type",new QLabel("Features",panel));
    auto* objects=new QLabel("0 selected in Timeline",panel);objects->setObjectName("featureMirror.objects");
    objects->setWordWrap(true);form->addRow("Objects",objects);
    auto* plane=new QComboBox(panel);plane->setObjectName("featureMirror.plane");
    plane->addItem("Select Mirror Plane");plane->addItem("Origin XZ");plane->addItem("Origin YZ");form->addRow("Mirror Plane",plane);
    connect(plane,QOverload<int>::of(&QComboBox::currentIndexChanged),this,[this](int index){
        if(session_.value("command")!="solid.featureMirror")return;
        auto inputs=session_["inputs"].toObject();inputs["mirrorPlane"]=index==1
            ? QJsonObject{{"kind","origin"},{"plane","XZ"}} : index==2
                ? QJsonObject{{"kind","origin"},{"plane","YZ"}} : QJsonObject{};
        session_["inputs"]=inputs;previewFeatureMirror();
    });
    auto* status=new QLabel(panel);status->setObjectName("featureMirror.status");status->setWordWrap(true);form->addRow(status);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,panel);form->addRow(buttons);
    buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
    connect(buttons,&QDialogButtonBox::accepted,this,[this,status]{
        const auto response=execute("command.commit",{{"session",session_["id"]}});
        if(!response["ok"].toBool())status->setText(response["error"].toObject()["message"].toString());
    });
    auto cancel=[this]{if(session_.value("command")=="solid.featureMirror")
        execute("command.cancel",{{"session",session_["id"]}});};
    connect(buttons,&QDialogButtonBox::rejected,this,cancel);
    auto* escape=new QShortcut(QKeySequence(Qt::Key_Escape),panel);
    escape->setContext(Qt::WidgetWithChildrenShortcut);connect(escape,&QShortcut::activated,this,cancel);
    canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,featureMirrorDock_);featureMirrorDock_->show();
}
