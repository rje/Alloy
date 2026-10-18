// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <QFormLayout>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QShortcut>
#include <QSignalBlocker>
#include <QVBoxLayout>
#include <cmath>
namespace {
QJsonObject fail(const QString& code,const QString& message){return {{"ok",false},{"error",QJsonObject{{"code",code},{"message",message}}}};}
QJsonObject param(const QString& type,const QString& description,bool required=true){return {{"type",type},{"description",description},{"required",required}};}
}
void AlloyWindow::registerSketchChamferCommands(){
    auto value=param("number","Equal setback distance in supplied length units.",false);value["minimum"]=0.000001;value["maximum"]=100000;
    auto units=param("string","Length units (defaults to the sketch units).",false);units["enum"]=QJsonArray{"mm","in"};
    const QJsonObject schema{{"entityIds",param("array","Two incident retained source line IDs.",false)},{"value",value},{"units",units},{"chamferId",param("string","Retained Chamfer UUID for editing.",false)},{"session",param("string","Optional active sketch session UUID.",false)}};
    for(const auto& action:QStringList{"Preview","Accept","Cancel","Update",""}){
        const QString id="sketch.chamfer"+action;
        auto commandSchema=action=="Cancel"?QJsonObject{}:schema;
        if(action.isEmpty() || action=="Update"){
            auto requiredValue=value;requiredValue["required"]=true;commandSchema["value"]=requiredValue;
            const auto target=action=="Update"?QString("chamferId"):QString("entityIds");auto requiredTarget=commandSchema.value(target).toObject();requiredTarget["required"]=true;commandSchema[target]=requiredTarget;
        }
        commands_.push_back({id,action.isEmpty()?"Equal Distance Chamfer":action+" Equal Distance Chamfer","Retain equal setbacks on one connected line corner while preserving source dimensions and constraints.",commandSchema,true,[this,action](const QJsonObject& a){return sketchChamfer(a,action);}});
    }
}
void AlloyWindow::cancelChamfer(){
    if(!session_.contains("pendingChamfer"))return;
    session_.remove("pendingChamfer");view_->clearSketchCursor();
    if(session_.value("command")=="sketch.edit"){view_->showSketchDraft(evaluateSketchDraft(session_.value("draftSketch").toObject()));displayedSketchDraft_=session_.value("draftSketch").toObject();}
}
QJsonObject AlloyWindow::sketchChamfer(const QJsonObject& args,const QString& operation){
    if(session_.value("command")!="sketch.edit")return fail("no_active_sketch","Enter a sketch first");
    if(args.contains("session") && args.value("session")!=session_.value("id"))return fail("invalid_session","Session is no longer active");
    if(session_.value("baseDocumentRevision").toInteger()!=static_cast<qint64>(document_->revision()))return fail("stale_session","The document changed; cancel and restart this sketch");
    if(operation=="Cancel"){cancelChamfer();session_["tool"]="select";view_->setFocus(Qt::OtherFocusReason);return {{"authoredChange",false},{"cancelled",true}};}
    if((operation=="Preview" || operation=="Accept") && session_.value("tool")!="chamfer")return fail("chamfer_tool_inactive","Choose Equal Distance Chamfer first");
    auto pending=session_.value("pendingChamfer").toObject();
    auto canonical=QJsonObject{};
    if(args.contains("chamferId"))canonical["chamferId"]=args.value("chamferId");
    else if(pending.contains("chamferId"))canonical["chamferId"]=pending.value("chamferId");
    else canonical["entityIds"]=args.contains("entityIds")?args.value("entityIds"):pending.value("entityIds");
    if(operation=="Update" && !args.contains("chamferId"))return fail("invalid_arguments","Chamfer Update requires chamferId");
    const auto units=args.value("units").toString(session_.value("inputs").toObject().value("units").toString());
    const double d=args.contains("value")?CadDocument::millimeters(args.value("value").toDouble(),units):pending.value("distanceMm").toDouble(10);
    canonical["distanceMm"]=d;
    const auto solved=SketchGeometry::chamferCorner(session_.value("draftSketch").toObject(),canonical,session_.value("projectionContext").toObject());
    if(!solved.ok){
        if(operation=="Preview"){pending.remove("previewDraft");pending["previewError"]=solved.error;session_["pendingChamfer"]=pending;view_->showSketchDraft(evaluateSketchDraft(session_.value("draftSketch").toObject()));refreshSketchControls();refreshChamferControls();}
        return fail("sketch_chamfer_failed",solved.error);
    }
    if(operation=="Preview"){
        pending.remove("previewError");pending["distanceMm"]=d;pending["previewDraft"]=solved.feature;pending["diagnostics"]=solved.diagnostics;
        if(canonical.contains("chamferId"))pending["chamferId"]=canonical.value("chamferId");else pending["entityIds"]=canonical.value("entityIds");session_["pendingChamfer"]=pending;
        const auto evaluated=evaluateSketchDraft(solved.feature);view_->showSketchDraft(evaluated);auto selection=session_.value("selection").toObject();selection["tool"]="chamfer";selection["entityIds"]=QJsonArray{solved.diagnostics.value("createdLineId")};view_->decorateSketch(solved.feature,selection,session_.value("solverDiagnostics").toObject(),units);
        return {{"authoredChange",false},{"previewed",true},{"sketchGeometry",evaluated.summary},{"diagnostics",solved.diagnostics}};
    }
    const auto response=publishSketchSolve(solved);if(response.contains("error"))return response;
    session_.remove("pendingChamfer");session_["tool"]="select";view_->clearSketchCursor();view_->setFocus(Qt::OtherFocusReason);return response;
}
QJsonObject AlloyWindow::chamferGesture(const QJsonObject& pixel,bool commit){
    const auto plane=session_.value("inputs").toObject().value("plane").toString();
    const auto found=resolveFilletCorner(pixel.value("x").toDouble(),pixel.value("y").toDouble(),plane);
    if(!commit){if(!session_.contains("pendingChamfer"))view_->showSketchSnap(plane,found);return {{"snap",found},{"authoredChange",false}};}
    auto pending=session_.value("pendingChamfer").toObject();QJsonArray pair;
    if(found.value("status")=="acquired" && !pending.contains("firstSideId"))pair=found.value("target").toObject().value("entityIds").toArray();
    else {
        const auto hit=view_->pickSketch(session_.value("draftSketch").toObject(),pixel.value("x").toDouble(),pixel.value("y").toDouble(),"entity");
        if(hit.value("kind")!="entity")return fail("chamfer_target_required","Pick a corner or two adjacent normal lines");
        if(!pending.contains("firstSideId")){pending["firstSideId"]=hit.value("entityId");session_["pendingChamfer"]=pending;return {{"authoredChange",false},{"pendingChamfer",pending}};}
        pair={pending.value("firstSideId"),hit.value("entityId")};pending.remove("firstSideId");
    }
    pending["entityIds"]=pair;pending["distanceMm"]=10;session_["pendingChamfer"]=pending;
    const auto response=sketchChamfer({{"entityIds",pair},{"value",10},{"units","mm"}},"Preview");
    if(sketchChamferDistance_){const QSignalBlocker block(sketchChamferDistance_);sketchChamferDistance_->setText(QString::number(10.0/(session_.value("inputs").toObject().value("units")=="in"?25.4:1),'g',8));sketchChamferDistance_->setFocus();sketchChamferDistance_->selectAll();}
    return response;
}
void AlloyWindow::buildChamferControls(QVBoxLayout* layout){
    sketchChamferControls_=new QWidget;sketchChamferControls_->setObjectName("sketch.chamfer.controls");auto* form=new QFormLayout(sketchChamferControls_);form->setContentsMargins(0,3,0,3);
    auto* title=new QLabel("Equal Distance Chamfer");title->setStyleSheet("font-weight: 700; color: #176588;");form->addRow(title);
    auto* notice=new QLabel("Original source dimensions and constraints remain attached. Enter accepts; Escape cancels.");notice->setWordWrap(true);form->addRow(notice);
    sketchChamferDistance_=new QLineEdit("10");sketchChamferDistance_->setObjectName("sketch.chamfer.distance");form->addRow("Distance ("+session_.value("inputs").toObject().value("units").toString()+")",sketchChamferDistance_);
    auto* retained=new QComboBox;retained->setObjectName("sketch.chamfer.retained");form->addRow("Retained corner",retained);
    connect(retained,qOverload<int>(&QComboBox::currentIndexChanged),this,[this]{refreshChamferControls();});
    auto* edit=new QPushButton("Edit Equal Distance Chamfer");edit->setObjectName("sketch.chamfer.edit");form->addRow(edit);
    auto* accept=new QPushButton("OK");accept->setObjectName("sketch.chamfer.accept");auto* cancel=new QPushButton("Cancel");cancel->setObjectName("sketch.chamfer.cancel");form->addRow(accept,cancel);layout->addWidget(sketchChamferControls_);
    connect(edit,&QPushButton::clicked,this,[this]{const auto groups=session_.value("draftSketch").toObject().value("parameters").toObject().value("chamferGroups").toArray();if(groups.isEmpty())return;const auto* retained=sketchChamferControls_->findChild<QComboBox*>("sketch.chamfer.retained");const auto g=groups.at(retained?retained->currentIndex():0).toObject();execute("sketch.tool",{{"tool","chamfer"}});sketchResponse(execute("sketch.chamferPreview",{{"chamferId",g.value("id")},{"value",g.value("distanceMm")},{"units","mm"}}));sketchChamferDistance_->setFocus();sketchChamferDistance_->selectAll();});
    connect(sketchChamferDistance_,&QLineEdit::textEdited,this,[this](const QString& text){bool ok=false;const auto value=text.toDouble(&ok);if(ok && session_.contains("pendingChamfer"))sketchResponse(execute("sketch.chamferPreview",{{"value",value}}));});
    const auto apply=[this]{bool ok=false;const auto value=sketchChamferDistance_->text().toDouble(&ok);if(!ok){sketchResponse(fail("invalid_chamfer_distance","Enter a numeric equal setback distance"));return;}sketchResponse(execute("sketch.chamferAccept",{{"value",value}}));};
    connect(sketchChamferDistance_,&QLineEdit::returnPressed,this,apply);connect(accept,&QPushButton::clicked,this,apply);connect(cancel,&QPushButton::clicked,this,[this]{sketchResponse(execute("sketch.chamferCancel"));});
    auto* escape=new QShortcut(QKeySequence(Qt::Key_Escape),sketchChamferControls_);escape->setContext(Qt::WidgetWithChildrenShortcut);connect(escape,&QShortcut::activated,this,[this]{sketchResponse(execute("sketch.chamferCancel"));});
}
void AlloyWindow::refreshChamferControls(){
    if(!sketchChamferControls_)return;
    const bool active=session_.value("tool")=="chamfer";const auto groups=session_.value("draftSketch").toObject().value("parameters").toObject().value("chamferGroups").toArray();const auto pending=session_.value("pendingChamfer").toObject();
    sketchChamferControls_->setVisible(active || !groups.isEmpty());
    auto* retained=sketchChamferControls_->findChild<QComboBox*>("sketch.chamfer.retained");
    int selected=0;
    if(retained){
        const auto previous=retained->currentData().toString();const QSignalBlocker block(retained);retained->clear();
        for(int i=0;i<groups.size();++i){const auto g=groups[i].toObject();retained->addItem(QString("Chamfer %1").arg(i+1),g.value("id").toString());if(g.value("id").toString()==previous)selected=i;}
        retained->setCurrentIndex(selected);
        const bool showRetained=!active && groups.size()>1;
        retained->setVisible(showRetained);
        if(auto* form=qobject_cast<QFormLayout*>(sketchChamferControls_->layout()))
            if(auto* label=form->labelForField(retained))label->setVisible(showRetained);
    }
    if(auto* edit=sketchChamferControls_->findChild<QPushButton*>("sketch.chamfer.edit"))edit->setVisible(!active && !groups.isEmpty());
    if(auto* accept=sketchChamferControls_->findChild<QPushButton*>("sketch.chamfer.accept")){accept->setVisible(active);accept->setEnabled(pending.contains("previewDraft"));}
    if(auto* cancel=sketchChamferControls_->findChild<QPushButton*>("sketch.chamfer.cancel"))cancel->setVisible(active);
    sketchChamferDistance_->setReadOnly(!active);
    if(pending.contains("distanceMm") || !groups.isEmpty()){
        const double distance=(pending.contains("distanceMm")?pending.value("distanceMm").toDouble():groups.at(selected).toObject().value("distanceMm").toDouble())/(session_.value("inputs").toObject().value("units")=="in"?25.4:1);
        if(!sketchChamferDistance_->hasFocus()){const QSignalBlocker block(sketchChamferDistance_);sketchChamferDistance_->setText(QString::number(distance,'g',8));}
    }
    if(active && pending.contains("previewDraft")){const auto preview=pending.value("previewDraft").toObject();view_->showSketchDraft(evaluateSketchDraft(preview));auto selection=session_.value("selection").toObject();selection["tool"]="chamfer";selection["entityIds"]=QJsonArray{pending.value("diagnostics").toObject().value("createdLineId")};view_->decorateSketch(preview,selection,session_.value("solverDiagnostics").toObject(),session_.value("inputs").toObject().value("units").toString());}
}
