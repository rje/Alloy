// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <QDockWidget>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QShortcut>
#include <QLineEdit>

namespace {
QJsonObject failure(const QString& code,const QString& message,const QJsonObject& diagnostics={}){
    return {{"ok",false},{"error",QJsonObject{{"code",code},{"message",message},{"diagnostics",diagnostics}}}};
}
}
QJsonObject AlloyWindow::thickenCandidate(const QJsonObject& inputs) const{
    auto args=inputs;if(!args.contains("featureId") && session_.value("command")=="solid.thicken"){
        args["newFeatureId"]=session_["newFeatureId"];args["newBodyId"]=session_["newBodyId"];
    }
    if(!args.contains("featureId") && !args.contains("ownerId"))args["ownerId"]=document_->activeOwnerId();
    return CadDocument::thickenCandidate(document_->authored(),args);
}
QJsonObject AlloyWindow::beginThicken(const QJsonObject& overrides){
    if(!session_.isEmpty())return failure("session_busy","Complete or cancel the active command first");
    const auto error=validateArguments(*findCommand("solid.thicken"),overrides);
    if(!error.isEmpty())return failure("invalid_arguments",error);
    QJsonObject inputs{{"signedThicknessMm",3.0},{"name","Thicken"}};
    if(!selectedFace_.isEmpty())inputs["sourceFace"]=selectedFace_;
    if(overrides.contains("featureId")){
        bool found=false;for(const auto& row:document_->authored()["features"].toArray())if(row.toObject()["id"]==overrides["featureId"]){
            const auto feature=row.toObject();if(feature["type"]!="thicken")return failure("stale_entity","Thicken feature unavailable");
            inputs=feature["parameters"].toObject();inputs["featureId"]=feature["id"];inputs["name"]=feature["name"];found=true;break;
        }
        if(!found)return failure("stale_entity","Thicken feature unavailable");
    }
    for(auto it=overrides.begin();it!=overrides.end();++it)inputs[it.key()]=it.value();
    session_={{"id",CadDocument::newId()},{"command","solid.thicken"},{"status","active"},{"inputs",inputs},
        {"newFeatureId",CadDocument::newId()},{"newBodyId",CadDocument::newId()},
        {"baseDocumentRevision",static_cast<qint64>(document_->revision())},{"previewed",false},
        {"geometryEvaluated",false},{"inputSchema",findCommand("solid.thicken")->parameters}};
    const auto edit=inputs["featureId"].toString();if(!edit.isEmpty())
        view_->setBodyStage(document_->evaluationBefore(edit).bodies,"thicken-source:"+session_["id"].toString());
    showThicken();
    if(inputs.contains("sourceFace")){
        const auto selected=selectThickenFace(inputs["sourceFace"].toObject());
        if(selected.contains("ok") && !selected["ok"].toBool())previewThicken();
    }else previewThicken();
    view_->setFocus();return session_;
}
QJsonObject AlloyWindow::selectThickenFace(const QJsonObject& ref){
    if(session_.value("command")!="solid.thicken")return failure("invalid_session","Start Thicken first");
    const auto edit=session_["inputs"].toObject()["featureId"].toString();
    const auto& stage=edit.isEmpty()?document_->evaluation():document_->evaluationBefore(edit);
    bool found=false;QString name;
    for(const auto& body:stage.bodies)if(body.bodyId==ref["bodyId"].toString() && body.visible
        && body.summary["type"]=="surface" && body.supportFaces.size()==1){
        const auto& face=body.supportFaces[0];
        found=face.faceId==ref["faceId"].toString() && face.producerFeatureId==ref["producerFeatureId"].toString();
        name=body.name;
    }
    if(!found)return failure("surface_unavailable","Select the exact visible Loft surface face");
    auto inputs=session_["inputs"].toObject();inputs["sourceFace"]=ref;session_["inputs"]=inputs;
    session_["selectedSurfaceName"]=name;
    return previewThicken();
}
QJsonObject AlloyWindow::previewThicken(){
    if(thickenDock_)if(auto* buttons=thickenDock_->findChild<QDialogButtonBox*>())buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
    session_["previewed"]=false;session_["geometryEvaluated"]=false;session_.remove("geometry");session_.remove("documentDraft");view_->clearPreview();
    auto reject=[this](const QString& code,const QString& message,const QJsonObject& diagnostics=QJsonObject{}){
        auto result=failure(code,message,diagnostics);session_["previewError"]=result["error"];
        if(thickenDock_)if(auto* label=thickenDock_->findChild<QLabel*>("thickenError"))label->setText(message);return result;
    };
    if(session_["baseDocumentRevision"].toInteger()!=static_cast<qint64>(document_->revision()))
        return reject("stale_session","The document changed; cancel and restart Thicken");
    const auto inputs=session_["inputs"].toObject();
    if(thickenDock_)if(auto* button=thickenDock_->findChild<QPushButton*>("thickenFace"))
        button->setText(inputs["sourceFace"].toObject().isEmpty()?"Select face":session_.value("selectedSurfaceName").toString("1 surface selected"));
    if(inputs["sourceFace"].toObject().isEmpty())return reject("surface_required","Select one Loft surface face");
    if(std::abs(inputs["signedThicknessMm"].toDouble())<0.001)return reject("thickness_required","Enter a nonzero one-side thickness");
    QJsonObject candidate;try{candidate=thickenCandidate(inputs);}catch(const std::exception& error){return reject("invalid_arguments",QString::fromUtf8(error.what()));}
    const auto evaluated=CadDocument::evaluateCandidate(candidate);
    if(!evaluated.ok)return reject("evaluation_failed",evaluated.error,evaluated.diagnostics);
    auto stage=evaluated.evaluation;const auto edit=inputs["featureId"].toString();
    if(!edit.isEmpty()){
        QJsonArray features;for(const auto& row:candidate["features"].toArray()){features.append(row);if(row.toObject()["id"]==edit)break;}
        auto prefix=candidate;prefix["features"]=features;const auto early=CadDocument::evaluateCandidate(prefix);
        if(!early.ok)return reject("evaluation_failed",early.error,early.diagnostics);stage=early.evaluation;
    }
    const auto target=edit.isEmpty()?session_["newBodyId"].toString():[&]{for(const auto& row:candidate["features"].toArray())
        if(row.toObject()["id"]==edit)return row.toObject()["bodyId"].toString();return QString{};}();
    for(const auto& body:stage->bodies)if(body.bodyId==target){
        // The committed feature consumes this sheet; hide it during preview too.
        view_->showPreview(body.shape,inputs["sourceFace"].toObject()["bodyId"].toString(),true);
        session_["geometry"]=body.summary;session_["documentDraft"]=candidate;session_["previewed"]=true;session_["geometryEvaluated"]=true;session_.remove("previewError");
        if(thickenDock_){thickenDock_->findChild<QLabel*>("thickenError")->clear();thickenDock_->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->setEnabled(true);}
        return session_;
    }
    return reject("body_unavailable","Thicken result body is unavailable");
}
void AlloyWindow::showThicken(){
    if(thickenDock_){canvasWindow_->removeDockWidget(thickenDock_);delete thickenDock_;}
    thickenDock_=new QDockWidget("Thicken",canvasWindow_);thickenDock_->setObjectName("thickenDock");
    thickenDock_->setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);
    auto* panel=new QWidget(thickenDock_);thickenDock_->setWidget(panel);auto* form=new QFormLayout(panel);
    auto* face=new QPushButton("Select face",panel);face->setObjectName("thickenFace");form->addRow("Faces",face);
    connect(face,&QPushButton::clicked,panel,[this]{view_->setFocus();});
    form->addRow("Direction",new QLabel("One Side",panel));
    auto* amount=new QDoubleSpinBox(panel);amount->setObjectName("thickenThickness");amount->setAccessibleName("Signed Thickness");
    amount->setKeyboardTracking(false);amount->setDecimals(3);amount->setRange(-1000,1000);amount->setSuffix(" mm");
    amount->setValue(session_["inputs"].toObject()["signedThicknessMm"].toDouble());form->addRow("Thickness",amount);
    connect(amount,&QDoubleSpinBox::valueChanged,panel,[this](double value){
        const auto response=execute("command.update",{{"session",session_["id"]},{"inputs",QJsonObject{{"signedThicknessMm",value}}}});
        if(response["ok"].toBool())previewThicken();
    });
    auto* flip=new QPushButton("Flip Side",panel);flip->setObjectName("thickenFlip");form->addRow("Side",flip);
    connect(flip,&QPushButton::clicked,panel,[amount]{amount->setValue(-amount->value());});
    form->addRow("Operation",new QLabel("New Body · hide source sheet",panel));
    auto* help=new QLabel("Click the visible Loft sheet. A signed thickness follows the selected face normal; Flip Side reverses it.",panel);
    help->setWordWrap(true);form->addRow(help);
    auto* error=new QLabel(panel);error->setObjectName("thickenError");error->setWordWrap(true);form->addRow(error);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,panel);form->addRow(buttons);
    connect(buttons,&QDialogButtonBox::accepted,panel,[this,error]{const auto response=execute("command.commit",{{"session",session_["id"]}});
        if(!response["ok"].toBool())error->setText(response["error"].toObject()["message"].toString());});
    connect(buttons,&QDialogButtonBox::rejected,panel,[this]{execute("command.cancel",{{"session",session_["id"]}});});
    auto cancel=[this]{if(session_.value("command")=="solid.thicken")execute("command.cancel",{{"session",session_["id"]}});};
    auto* escape=new QShortcut(QKeySequence(Qt::Key_Escape),panel);escape->setContext(Qt::WidgetWithChildrenShortcut);connect(escape,&QShortcut::activated,panel,cancel);
    if(!thickenEscape_){thickenEscape_=new QShortcut(QKeySequence(Qt::Key_Escape),view_);thickenEscape_->setContext(Qt::WidgetShortcut);connect(thickenEscape_,&QShortcut::activated,this,cancel);}
    thickenEscape_->setEnabled(true);canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,thickenDock_);thickenDock_->show();
}
