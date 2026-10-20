// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <QCloseEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QShortcut>
#include <QSignalBlocker>
#include <QVBoxLayout>
namespace {
QJsonObject failure(const QString& code,const QString& message){return {{"ok",false},{"error",QJsonObject{{"code",code},{"message",message}}}};}
class ScaleDock final:public QDockWidget {
public: using QDockWidget::QDockWidget; std::function<void()> closed;
protected: void closeEvent(QCloseEvent* e)override{const auto callback=closed;if(callback)callback();QDockWidget::closeEvent(e);}
};
}
void AlloyWindow::closeSketchScaleControls(){
    session_.remove("pendingSketchScale");session_.remove("scaleInteraction");
    auto* panel=sketchScaleDialog_.data();sketchScaleDialog_=nullptr;
    if(panel){panel->hide();if(auto* dock=dynamic_cast<ScaleDock*>(panel->parentWidget())){dock->closed={};dock->hide();dock->deleteLater();}else panel->deleteLater();}
    if(session_["command"]=="sketch.edit"){
        view_->showSketchDraft(evaluateSketchDraft(session_["draftSketch"].toObject()));refreshSketchControls();
        if(sketchDock_)sketchDock_->raise();view_->setFocus(Qt::OtherFocusReason);
    }
}
void AlloyWindow::syncSketchScaleControls(const QString& message){
    if(!sketchScaleDialog_)return;
    const auto in=session_["scaleInteraction"].toObject();
    sketchScaleDialog_->findChild<QPushButton*>("sketch.scale.entities")->setText(QString("%1 / 4 Lines%2").arg(in["entityIds"].toArray().size()).arg(in.contains("groupId")?" · locked":" · select"));
    sketchScaleDialog_->findChild<QPushButton*>("sketch.scale.pivot")->setText(in["pivotPointId"].toString().isEmpty()?"Select Scale Point":"Scale Point selected");
    for(const auto& role:{QString("entities"),QString("pivot")}){
        auto* b=sketchScaleDialog_->findChild<QPushButton*>("sketch.scale."+role);QSignalBlocker block(b);b->setChecked(in["role"]==role);
    }
    sketchScaleDialog_->findChild<QLabel*>("sketch.scale.status")->setText(message.isEmpty() ? session_.contains("pendingSketchScale") ? "Preview · uniform scale from original source" : "Select four profile Lines and a separate Point" : message);
    sketchScaleDialog_->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->setEnabled(session_.contains("pendingSketchScale"));
}
void AlloyWindow::registerSketchScaleCommands(){
    auto add=[this](const QString& id,const QJsonObject& schema,std::function<QJsonObject(const QJsonObject&)> run){commands_.push_back({id,id,"Retained uniform Scale of one isolated XY four-Line profile about a separate Point",schema,true,std::move(run)});};
    const QJsonObject optionalId{{"type","string"},{"required",false}};
    const QJsonObject number{{"type","number"},{"minimum",-100000},{"maximum",100000}};
    const QJsonObject scaleFactor{{"type","number"},{"minimum",.000001},{"maximum",100000}};
    const auto active=[this]{return session_["command"]=="sketch.edit";};
    const auto busy=[this]{return session_.contains("pendingSketchMove") || session_.value("moveInteraction").toObject().value("active").toBool() || session_.contains("pasteInteraction") || session_.contains("axis3dInteraction") || session_.value("patternInteraction").toObject().value("active").toBool() || session_.contains("pendingSketchPattern");};
    add("sketch.scalePreview",{{"entityIds",QJsonObject{{"type","array"},{"required",false}}},{"pivotPointId",QJsonObject{{"type","string"}}},{"factor",scaleFactor},{"groupId",optionalId}},[this,active,busy](const auto& a){
        if(!active())return failure("no_active_sketch","Enter an XY sketch first");
        if(busy())return failure("pending_input_active","Accept or cancel the other input first");
        auto refuse=[this](const QString& code,const QString& message){session_.remove("pendingSketchScale");view_->showSketchDraft(evaluateSketchDraft(session_["draftSketch"].toObject()));syncSketchScaleControls(message);refreshSketchControls();return failure(code,message);};
        auto base=session_["draftSketch"].toObject(),inputs=a;const auto prior=session_["pendingSketchScale"].toObject();
        if(a["groupId"].toString().isEmpty() && prior["base"]==base){const auto groups=prior["draft"].toObject()["parameters"].toObject()["scaleGroups"].toArray();if(!groups.isEmpty()){base=prior["draft"].toObject();inputs["groupId"]=groups.first().toObject()["id"];}}
        const auto solved=SketchGeometry::retainedScale(base,inputs,session_["projectionContext"].toObject());
        if(!solved.ok)return refuse("sketch_scale_failed",solved.error);
        if(!session_["inputs"].toObject()["sketchId"].toString().isEmpty())try{GeometryKernel::evaluateDocument(SketchGeometry::replaceFeature(document_->authored(),solved.feature));}catch(const std::exception& e){return refuse("sketch_scale_dependency",QString::fromUtf8(e.what()));}
        const auto g=solved.feature["parameters"].toObject()["scaleGroups"].toArray().first().toObject();
        session_["pendingSketchScale"]=QJsonObject{{"base",session_["draftSketch"]},{"draft",solved.feature},{"diagnostics",solved.diagnostics},{"groupId",g["id"]}};
        if(sketchScaleDialog_){auto in=session_["scaleInteraction"].toObject();in["entityIds"]=g["entityIds"];in["pivotPointId"]=g["pivotPointId"];session_["scaleInteraction"]=in;auto* spin=sketchScaleDialog_->findChild<QDoubleSpinBox*>("sketch.scale.factor");QSignalBlocker block(spin);spin->setValue(g["factor"].toDouble());}
        view_->showSketchDraft(evaluateSketchDraft(solved.feature));refreshSketchControls();syncSketchScaleControls();
        return QJsonObject{{"pending",true},{"status","pending"},{"authoredChange",false},{"sessionId",session_["id"]},{"groupId",g["id"]},{"draft",solved.feature}};
    });
    add("sketch.scaleAccept",{},[this,active](const auto&){
        if(!active())return failure("no_active_sketch","Enter a sketch first");const auto p=session_["pendingSketchScale"].toObject();
        if(p.isEmpty())return failure("scale_preview_required","Preview a valid Scale first");if(p["base"]!=session_["draftSketch"])return failure("stale_scale_preview","The sketch changed; preview again");
        auto result=publishSketchSolve({true,p["draft"].toObject(),p["diagnostics"].toObject(),{}});
        if(!result.contains("error")){auto selection=session_["selection"].toObject();selection["entityIds"]=p["draft"].toObject()["parameters"].toObject()["scaleGroups"].toArray().first().toObject()["entityIds"];selection["pointIds"]=QJsonArray{};selection["profileIds"]=QJsonArray{};session_["selection"]=selection;closeSketchScaleControls();result["groupId"]=p["groupId"];result["sessionId"]=session_["id"];result["status"]="committed";}
        return result;
    });
    add("sketch.scaleCancel",{},[this](const auto&){closeSketchScaleControls();return QJsonObject{{"cancelled",true},{"status","cancelled"},{"authoredChange",false}};});
    add("sketch.scaleStart",{},[this,active,busy](const auto&){if(!active() || session_["draftSketch"].toObject()["parameters"].toObject()["plane"]!="XY")return failure("scale_plane_unsupported","Scale requires an active XY sketch");if(busy() || session_["tool"]!="select")return failure("pending_input_active","Accept or cancel the other input first");if(!sketchScaleDialog_)showSketchScaleControls();return QJsonObject{{"visible",true},{"sessionId",session_["id"]}};});
    add("sketch.scaleEdit",{{"groupId",optionalId}},[this,active](const auto& a){const auto groups=session_["draftSketch"].toObject()["parameters"].toObject()["scaleGroups"].toArray();if(!active() || groups.isEmpty() || (a.contains("groupId") && a["groupId"]!=groups.first().toObject()["id"]))return failure("scale_missing","Enter the sketch containing this Scale");return execute("sketch.scaleStart");});
    add("sketch.scalePick",{{"role",QJsonObject{{"type","string"},{"enum",QJsonArray{"entities","pivot"}}}},{"x",number},{"y",number}},[this](const auto& a){
        auto in=session_["scaleInteraction"].toObject();if(!sketchScaleDialog_ || !in["active"].toBool())return failure("no_scale_controls","Open Scale first");
        const auto visible=session_.contains("pendingSketchScale")?session_["pendingSketchScale"].toObject()["draft"].toObject():session_["draftSketch"].toObject();
        const bool point=a["role"]=="pivot";const auto hit=view_->pickSketch(visible,a["x"].toDouble(),a["y"].toDouble(),point?"point":"entity",session_["resolvedFrame"].toObject());
        if(point){const auto id=hit["pointId"].toString();bool standalone=false;for(const auto& v:visible["parameters"].toObject()["entities"].toArray()){const auto e=v.toObject();if(e["type"]=="point" && e["pointId"]==id)standalone=true;}if(id.isEmpty() || !standalone)return failure("scale_pick_unavailable","Click a separate authored Point");in["pivotPointId"]=id;in["role"]="entities";}
        else{const auto id=hit["entityId"].toString();QJsonObject edge;for(const auto& v:visible["parameters"].toObject()["entities"].toArray())if(v.toObject()["id"]==id)edge=v.toObject();if(edge["type"]!="line" || edge["construction"].toBool() || edge["centerline"].toBool())return failure("scale_pick_unavailable","Click an ordinary profile Line");auto selected=in["entityIds"].toArray();if(in.contains("groupId")){if(!selected.contains(id))return failure("scale_owned_selection","Edit Scale retains its four Lines");}else if(selected.contains(id)){QJsonArray keep;for(const auto& v:selected)if(v!=id)keep.append(v);selected=keep;}else selected.append(id);in["entityIds"]=selected;}
        session_["scaleInteraction"]=in;session_.remove("pendingSketchScale");
        QJsonObject args{{"entityIds",in["entityIds"]},{"pivotPointId",in["pivotPointId"].toString()},{"factor",sketchScaleDialog_->findChild<QDoubleSpinBox*>("sketch.scale.factor")->value()}};if(in.contains("groupId"))args["groupId"]=in["groupId"];
        if(in["entityIds"].toArray().size()==4 && !in["pivotPointId"].toString().isEmpty())execute("sketch.scalePreview",args);
        else view_->showSketchDraft(evaluateSketchDraft(session_["draftSketch"].toObject()));refreshSketchControls();syncSketchScaleControls();return QJsonObject{{"entityIds",in["entityIds"]},{"pivotPointId",in["pivotPointId"]}};
    });
}
void AlloyWindow::showSketchScaleControls(){
    const auto groups=session_["draftSketch"].toObject()["parameters"].toObject()["scaleGroups"].toArray();const auto g=groups.isEmpty()?QJsonObject{}:groups.first().toObject();
    auto* dock=new ScaleDock(g.isEmpty()?"Scale":"Edit Scale",canvasWindow_);dock->setObjectName("sketch.scale.dock");dock->setMinimumWidth(300);
    auto* panel=new QDialog(dock);panel->setWindowFlags(Qt::Widget);panel->setObjectName("sketch.scale.dialog");sketchScaleDialog_=panel;
    auto* layout=new QVBoxLayout(panel);auto* form=new QFormLayout;layout->addLayout(form);
    auto* entities=new QPushButton;entities->setObjectName("sketch.scale.entities");entities->setCheckable(true);form->addRow("Entities",entities);
    auto* pivot=new QPushButton;pivot->setObjectName("sketch.scale.pivot");pivot->setCheckable(true);form->addRow("Scale Point",pivot);
    auto* spin=new QDoubleSpinBox;spin->setObjectName("sketch.scale.factor");spin->setDecimals(6);spin->setRange(.000001,100000);spin->setValue(g.isEmpty()?1:g["factor"].toDouble());spin->setKeyboardTracking(false);form->addRow("Scale Factor",spin);
    auto* policy=new QLabel("Uniform · XY plane\nSeparate authored pivot Point\nRetained scale from original source");policy->setWordWrap(true);layout->addWidget(policy);
    auto* status=new QLabel;status->setObjectName("sketch.scale.status");status->setWordWrap(true);status->setMinimumHeight(72);layout->addWidget(status);layout->addStretch();
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);buttons->button(QDialogButtonBox::Ok)->setObjectName("sketch.scale.ok");buttons->button(QDialogButtonBox::Cancel)->setObjectName("sketch.scale.cancel");layout->addWidget(buttons);
    QJsonArray selected;if(g.isEmpty()){for(const auto& id:session_["selection"].toObject()["entityIds"].toArray())for(const auto& v:session_["draftSketch"].toObject()["parameters"].toObject()["entities"].toArray()){const auto e=v.toObject();if(e["id"]==id && e["type"]=="line" && !e["construction"].toBool() && !e["centerline"].toBool())selected.append(id);}}else selected=g["entityIds"].toArray();
    session_["scaleInteraction"]=QJsonObject{{"active",true},{"role","entities"},{"entityIds",selected},{"pivotPointId",g["pivotPointId"].toString()}};if(!g.isEmpty()){auto in=session_["scaleInteraction"].toObject();in["groupId"]=g["id"];session_["scaleInteraction"]=in;}
    auto preview=[this,spin]{const auto in=session_["scaleInteraction"].toObject();QJsonObject args{{"entityIds",in["entityIds"]},{"pivotPointId",in["pivotPointId"].toString()},{"factor",spin->value()}};if(in.contains("groupId"))args["groupId"]=in["groupId"];return execute("sketch.scalePreview",args);};
    connect(spin,&QDoubleSpinBox::valueChanged,panel,[preview]{preview();});
    for(const auto& role:{QString("entities"),QString("pivot")})connect(panel->findChild<QPushButton*>("sketch.scale."+role),&QPushButton::clicked,panel,[this,role]{auto in=session_["scaleInteraction"].toObject();in["role"]=role;session_["scaleInteraction"]=in;syncSketchScaleControls();refreshSketchControls();view_->setFocus(Qt::OtherFocusReason);});
    auto accept=[this,spin,preview]{
        spin->interpretText();const auto result=preview();
        if(!result.value("ok").toBool()){syncSketchScaleControls(result.value("error").toObject().value("message").toString());return;}
        const auto accepted=execute("sketch.scaleAccept");
        if(!accepted.value("ok").toBool())syncSketchScaleControls(accepted.value("error").toObject().value("message").toString());
    };
    connect(buttons,&QDialogButtonBox::accepted,panel,accept);connect(buttons,&QDialogButtonBox::rejected,panel,[this]{execute("sketch.scaleCancel");});
    connect(panel,&QDialog::rejected,panel,[this]{execute("sketch.scaleCancel");});
    for(const auto key:{Qt::Key_Return,Qt::Key_Enter}){auto* enter=new QShortcut(QKeySequence(key),panel);enter->setContext(Qt::WidgetWithChildrenShortcut);connect(enter,&QShortcut::activated,panel,accept);}
    auto* escape=new QShortcut(QKeySequence(Qt::Key_Escape),panel);escape->setContext(Qt::WidgetWithChildrenShortcut);connect(escape,&QShortcut::activated,panel,[this]{execute("sketch.scaleCancel");});
    dock->closed=[this]{execute("sketch.scaleCancel");};dock->setWidget(panel);canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,dock);if(sketchDock_)canvasWindow_->tabifyDockWidget(sketchDock_,dock);dock->show();dock->raise();panel->show();syncSketchScaleControls();if(!g.isEmpty())preview();refreshSketchControls();view_->setFocus(Qt::OtherFocusReason);
}
