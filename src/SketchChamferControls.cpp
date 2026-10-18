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
    auto value=param("number","First setback in supplied length units; Equal Distance also drives the second side.",false);value["minimum"]=0.000001;value["maximum"]=100000;
    auto units=param("string","Length units (defaults to the sketch units).",false);units["enum"]=QJsonArray{"mm","in"};
    auto type=param("string","Chamfer mode; omitted edits retain the stored mode.",false);type["enum"]=QJsonArray{"equalDistance","twoDistance"};
    const QJsonObject schema{{"type",type},{"value2",value},{"entityIds",param("array","Two incident retained source line IDs.",false)},{"value",value},{"units",units},{"chamferId",param("string","Retained Chamfer UUID for editing.",false)},{"session",param("string","Optional active sketch session UUID.",false)}};
    auto requiredType=type;requiredType["required"]=true;
    commands_.push_back({"sketch.chamferMode","Chamfer Feature Options","Switch implemented Chamfer modes; pending corner retained and committed geometry unchanged until acceptance.",{{"type",requiredType}},true,[this](const auto& a){return chamferMode(a);}});
    auto side=param("integer","Two Distance side: 0 is the first source, 1 the second.",false);side["minimum"]=0;side["maximum"]=1;
    auto dragValue=param("number","Requested setback in millimeters",false);dragValue["minimum"]=-1000000;dragValue["maximum"]=1000000;
    commands_.push_back({"sketch.chamferHandle","Drag sketch Chamfer","Preview an equal setback and publish one draft transaction on release.",
        {{"side",side},{"phase",param("string","begin, move, end or cancel")},{"chamferId",param("string","Retained or preview corner UUID",false)},
         {"value",dragValue}},true,[this](const QJsonObject& a){return chamferHandle(a);}});
    for(const auto& action:QStringList{"Preview","Accept","Cancel","Update",""}){
        const QString id="sketch.chamfer"+action;
        auto commandSchema=action=="Cancel"?QJsonObject{}:schema;
        if(action.isEmpty() || action=="Update"){
            auto requiredValue=value;requiredValue["required"]=action.isEmpty();commandSchema["value"]=requiredValue;
            const auto target=action=="Update"?QString("chamferId"):QString("entityIds");auto requiredTarget=commandSchema.value(target).toObject();requiredTarget["required"]=true;commandSchema[target]=requiredTarget;
        }
        commands_.push_back({id,action.isEmpty()?"Sketch Chamfer":action+" Sketch Chamfer","Retain equal or two independent setbacks; sourceEntityIds order defines Distance 1 and Distance 2.",commandSchema,true,[this,action](const QJsonObject& a){return sketchChamfer(a,action);}});
    }
}
void AlloyWindow::cancelChamfer(){
    view_->clearChamferDrag();
    if(!session_.contains("pendingChamfer"))return;
    session_.remove("pendingChamfer");view_->clearSketchCursor();
    if(session_.value("command")=="sketch.edit"){view_->showSketchDraft(evaluateSketchDraft(session_.value("draftSketch").toObject()));displayedSketchDraft_=session_.value("draftSketch").toObject();}
}
QJsonObject AlloyWindow::sketchChamfer(const QJsonObject& args,const QString& operation){
    if(session_.value("command")!="sketch.edit")return fail("no_active_sketch","Enter a sketch first");
    if(args.contains("session") && args.value("session")!=session_.value("id"))return fail("invalid_session","Session is no longer active");
    if(session_.value("baseDocumentRevision").toInteger()!=static_cast<qint64>(document_->revision()))return fail("stale_session","The document changed; cancel and restart this sketch");
    if(operation=="Cancel"){cancelChamfer();session_["tool"]="select";view_->setFocus(Qt::OtherFocusReason);return {{"authoredChange",false},{"cancelled",true}};}
    if((operation=="Preview" || operation=="Accept") && session_.value("tool")!="chamfer")return fail("chamfer_tool_inactive","Choose a sketch Chamfer mode first");
    auto pending=session_.value("pendingChamfer").toObject();
    auto canonical=QJsonObject{};
    if(args.contains("chamferId"))canonical["chamferId"]=args.value("chamferId");
    else if(pending.contains("chamferId"))canonical["chamferId"]=pending.value("chamferId");
    else canonical["entityIds"]=args.contains("entityIds")?args.value("entityIds"):pending.value("entityIds");
    if(operation=="Update" && !args.contains("chamferId"))return fail("invalid_arguments","Chamfer Update requires chamferId");
    QJsonObject retained;
    for(const auto& row:session_.value("draftSketch").toObject().value("parameters").toObject().value("chamferGroups").toArray())
        if(row.toObject().value("id")==canonical.value("chamferId"))retained=row.toObject();
    const auto type=args.value("type").toString(pending.value("type").toString(retained.value("type").toString(session_.value("chamferMode").toString("equalDistance"))));
    const auto units=args.value("units").toString(session_.value("inputs").toObject().value("units").toString());
    const double d=args.contains("value")?CadDocument::millimeters(args.value("value").toDouble(),units):pending.value("distanceMm").toDouble(retained.value("distanceMm").toDouble(10));
    const double d2=args.contains("value2")?CadDocument::millimeters(args.value("value2").toDouble(),units):pending.value("distance2Mm").toDouble(retained.value("distance2Mm").toDouble(d));
    canonical["type"]=type;canonical["distanceMm"]=d;
    if(type=="twoDistance")canonical["distance2Mm"]=d2;
    else if(args.contains("value2"))return fail("invalid_arguments","Equal Distance has no independent second setback");
    pending["type"]=type;pending["distanceMm"]=d;
    if(type=="twoDistance")pending["distance2Mm"]=d2;else pending.remove("distance2Mm");
    if(canonical.contains("chamferId"))pending["chamferId"]=canonical.value("chamferId");else pending["entityIds"]=canonical.value("entityIds");
    if((args.contains("value2") && (!pending.value("dragActive").toBool() || pending.value("dragSide").toInt()==1)) || !retained.isEmpty())pending["secondIndependent"]=true;
    const auto solved=SketchGeometry::chamferCorner(session_.value("draftSketch").toObject(),canonical,session_.value("projectionContext").toObject());
    if(!solved.ok){
        if(operation=="Preview"){pending.remove("previewDraft");pending["distanceMm"]=d;pending["previewError"]=solved.error;session_["pendingChamfer"]=pending;view_->showSketchDraft(evaluateSketchDraft(session_.value("draftSketch").toObject()));refreshSketchControls();refreshChamferControls();}
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
QJsonObject AlloyWindow::chamferMode(const QJsonObject& args){
    if(session_.value("command")!="sketch.edit")return fail("no_active_sketch","Enter a sketch first");
    const auto type=args.value("type").toString();
    if(type!="equalDistance" && type!="twoDistance")return fail("invalid_chamfer_mode","Choose an implemented Chamfer mode");
    session_["chamferMode"]=type;
    if(session_.value("tool")!="chamfer"){
        const auto response=execute("sketch.tool",{{"tool","chamfer"}});if(!response.value("ok").toBool())return response;
    }
    const auto pending=session_.value("pendingChamfer").toObject();
    if(pending.contains("entityIds") || pending.contains("chamferId")){
        auto mutation=QJsonObject{{"type",type},{"value",pending.value("distanceMm")},{"units","mm"}};
        if(type=="twoDistance")mutation["value2"]=pending.value("distance2Mm").toDouble(pending.value("distanceMm").toDouble(10));
        const auto response=sketchChamfer(mutation,"Preview");
        if(sketchChamferDistance_){sketchChamferDistance_->setFocus();sketchChamferDistance_->selectAll();}
        return response;
    }
    view_->setFocus(Qt::OtherFocusReason);return {{"type",type},{"authoredChange",false}};
}
QJsonObject AlloyWindow::chamferHandle(const QJsonObject& args){
    if(session_.value("command")!="sketch.edit")return fail("no_active_sketch","Enter a sketch first");
    const auto phase=args.value("phase").toString();
    if(phase=="cancel")return sketchChamfer({},"Cancel");
    if(phase=="begin"){
        if(!session_.contains("pendingChamfer")){
            const auto id=args.value("chamferId");QJsonObject group;
            for(const auto& row:session_.value("draftSketch").toObject().value("parameters").toObject().value("chamferGroups").toArray())
                if(row.toObject().value("id")==id)group=row.toObject();
            if(group.isEmpty())return fail("chamfer_unavailable","Retained Chamfer is unavailable");
            const auto tool=execute("sketch.tool",{{"tool","chamfer"}});if(!tool.value("ok").toBool())return tool;
            QJsonObject input{{"chamferId",id},{"value",group.value("distanceMm")},{"type",group.value("type")},{"units","mm"}};
            if(group.value("type")=="twoDistance")input["value2"]=group.value("distance2Mm");
            const auto preview=sketchChamfer(input,"Preview");
            if(preview.contains("error"))return preview;
        }
        auto pending=session_.value("pendingChamfer").toObject();pending["dragActive"]=true;pending["dragSide"]=args.value("side").toInt();session_["pendingChamfer"]=pending;
        return {{"dragging",true},{"authoredChange",false}};
    }
    if((phase!="move" && phase!="end") || !session_.value("pendingChamfer").toObject().value("dragActive").toBool())
        return fail("chamfer_drag_inactive","Begin a Chamfer handle drag first");
    if(!args.value("value").isDouble())return fail("invalid_arguments","Drag needs a setback distance");
    const double distance=args.value("value").toDouble();
    const auto pending=session_.value("pendingChamfer").toObject();const bool second=pending.value("dragSide").toInt()==1;
    QJsonObject mutation{{second?"value2":"value",distance},{"units","mm"}};
    // Public help couples the first handle initially. Once side 2 is authored,
    // or an existing group is edited, each handle owns only its retained side.
    if(!second && pending.value("type")=="twoDistance" && !pending.value("secondIndependent").toBool())mutation["value2"]=distance;
    auto* field=second?sketchChamferDistance2_:sketchChamferDistance_;
    if(field){const QSignalBlocker block(field);field->setText(QString::number(distance/(session_.value("inputs").toObject().value("units")=="in"?25.4:1),'g',8));}
    const auto response=sketchChamfer(mutation,phase=="move"?"Preview":"Accept");
    if(phase=="end" && response.contains("error"))sketchChamfer({},"Cancel");
    return response;
}
QJsonObject AlloyWindow::chamferGesture(const QJsonObject& pixel,bool commit){
    const auto plane=session_.value("inputs").toObject().value("plane").toString();
    const auto found=resolveFilletCorner(pixel.value("x").toDouble(),pixel.value("y").toDouble(),plane);
    if(!commit){if(!session_.contains("pendingChamfer"))view_->showSketchSnap(plane,found);return {{"snap",found},{"authoredChange",false}};}
    auto pending=session_.value("pendingChamfer").toObject();QJsonArray pair;
    if(found.value("status")=="acquired" && !pending.contains("firstSideId")){
        const auto foundIds=found.value("target").toObject().value("entityIds").toArray();
        // Vertex selection has no pick order: use authored entity order.
        for(const auto& row:session_.value("draftSketch").toObject().value("parameters").toObject().value("entities").toArray())
            if(foundIds.contains(row.toObject().value("id")))pair.append(row.toObject().value("id"));
    }
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
    sketchChamferControls_=new QWidget;sketchChamferControls_->setObjectName("sketch.chamfer.controls");
    auto* form=new QFormLayout(sketchChamferControls_);form->setContentsMargins(0,3,0,3);
    auto* title=new QLabel("Sketch Chamfer");title->setStyleSheet("font-weight: 700; color: #176588;");form->addRow(title);
    auto* mode=new QComboBox;mode->setObjectName("sketch.chamfer.mode");mode->addItem("Equal Distance","equalDistance");mode->addItem("Two Distance","twoDistance");form->addRow("Feature Options",mode);
    connect(mode,qOverload<int>(&QComboBox::currentIndexChanged),this,[this,mode]{sketchResponse(execute("sketch.chamferMode",{{"type",mode->currentData().toString()}}));});
    auto* notice=new QLabel("Distance 1: first line. Distance 2: second line. Vertex picks follow sketch order. Enter accepts; Escape cancels.");notice->setWordWrap(true);form->addRow(notice);
    const auto units=session_.value("inputs").toObject().value("units").toString();
    sketchChamferDistance_=new QLineEdit("10");sketchChamferDistance_->setObjectName("sketch.chamfer.distance");form->addRow("Distance 1 ("+units+")",sketchChamferDistance_);
    sketchChamferDistance2_=new QLineEdit("10");sketchChamferDistance2_->setObjectName("sketch.chamfer.distance2");form->addRow("Distance 2 ("+units+")",sketchChamferDistance2_);
    auto* retained=new QComboBox;retained->setObjectName("sketch.chamfer.retained");form->addRow("Retained corner",retained);
    connect(retained,qOverload<int>(&QComboBox::currentIndexChanged),this,[this]{refreshChamferControls();});
    auto* edit=new QPushButton("Edit Sketch Chamfer");edit->setObjectName("sketch.chamfer.edit");form->addRow(edit);
    auto* accept=new QPushButton("OK");accept->setObjectName("sketch.chamfer.accept");auto* cancel=new QPushButton("Cancel");cancel->setObjectName("sketch.chamfer.cancel");form->addRow(accept,cancel);layout->addWidget(sketchChamferControls_);
    connect(edit,&QPushButton::clicked,this,[this]{
        const auto groups=session_.value("draftSketch").toObject().value("parameters").toObject().value("chamferGroups").toArray();if(groups.isEmpty())return;
        const auto* retained=sketchChamferControls_->findChild<QComboBox*>("sketch.chamfer.retained");const auto g=groups.at(retained?retained->currentIndex():0).toObject();
        execute("sketch.tool",{{"tool","chamfer"}});
        QJsonObject input{{"chamferId",g.value("id")},{"value",g.value("distanceMm")},{"type",g.value("type")},{"units","mm"}};
        if(g.value("type")=="twoDistance")input["value2"]=g.value("distance2Mm");
        sketchResponse(execute("sketch.chamferPreview",input));sketchChamferDistance_->setFocus();sketchChamferDistance_->selectAll();
    });
    for(int side=0;side<2;++side){auto* field=side?sketchChamferDistance2_:sketchChamferDistance_;
        connect(field,&QLineEdit::textEdited,this,[this,side](const QString& text){bool ok=false;const auto value=text.toDouble(&ok);if(ok && session_.contains("pendingChamfer"))sketchResponse(execute("sketch.chamferPreview",{{side?"value2":"value",value}}));});
        connect(field,&QLineEdit::returnPressed,this,[this,field,side]{bool ok=false;const auto value=field->text().toDouble(&ok);if(!ok){sketchResponse(fail("invalid_chamfer_distance","Enter a numeric setback distance"));return;}sketchResponse(execute("sketch.chamferAccept",{{side?"value2":"value",value}}));});
    }
    connect(accept,&QPushButton::clicked,this,[this]{sketchResponse(execute("sketch.chamferAccept"));});
    connect(cancel,&QPushButton::clicked,this,[this]{sketchResponse(execute("sketch.chamferCancel"));});
    auto* escape=new QShortcut(QKeySequence(Qt::Key_Escape),sketchChamferControls_);escape->setContext(Qt::WidgetWithChildrenShortcut);connect(escape,&QShortcut::activated,this,[this]{sketchResponse(execute("sketch.chamferCancel"));});
}
void AlloyWindow::refreshChamferControls(){
    if(!sketchChamferControls_)return;
    const bool active=session_.value("tool")=="chamfer";const auto groups=session_.value("draftSketch").toObject().value("parameters").toObject().value("chamferGroups").toArray();const auto pending=session_.value("pendingChamfer").toObject();
    sketchChamferControls_->setVisible(active || !groups.isEmpty());
    auto* retained=sketchChamferControls_->findChild<QComboBox*>("sketch.chamfer.retained");int selected=0;
    auto* form=qobject_cast<QFormLayout*>(sketchChamferControls_->layout());
    auto visible=[form](QWidget* field,bool show){field->setVisible(show);if(auto* label=form->labelForField(field))label->setVisible(show);};
    if(retained){
        const auto previous=retained->currentData().toString();const QSignalBlocker block(retained);retained->clear();
        for(int i=0;i<groups.size();++i){const auto g=groups[i].toObject();retained->addItem(QString("Chamfer %1").arg(i+1),g.value("id").toString());if(g.value("id").toString()==previous)selected=i;}
        retained->setCurrentIndex(selected);visible(retained,!active && groups.size()>1);
    }
    const auto group=groups.isEmpty()?QJsonObject{}:groups.at(selected).toObject();
    const auto type=active?pending.value("type").toString(session_.value("chamferMode").toString("equalDistance")):group.value("type").toString("equalDistance");
    const bool two=type=="twoDistance";
    if(auto* mode=sketchChamferControls_->findChild<QComboBox*>("sketch.chamfer.mode")){const QSignalBlocker block(mode);mode->setCurrentIndex(two?1:0);visible(mode,active);}
    visible(sketchChamferDistance2_,two);
    if(auto* edit=sketchChamferControls_->findChild<QPushButton*>("sketch.chamfer.edit"))edit->setVisible(!active && !groups.isEmpty());
    if(auto* accept=sketchChamferControls_->findChild<QPushButton*>("sketch.chamfer.accept")){accept->setVisible(active);accept->setEnabled(pending.contains("previewDraft"));}
    if(auto* cancel=sketchChamferControls_->findChild<QPushButton*>("sketch.chamfer.cancel"))cancel->setVisible(active);
    const double scale=session_.value("inputs").toObject().value("units")=="in"?25.4:1;
    for(int side=0;side<2;++side){auto* field=side?sketchChamferDistance2_:sketchChamferDistance_;field->setReadOnly(!active);
        const auto key=side?"distance2Mm":"distanceMm";
        if(!field->hasFocus()){const QSignalBlocker block(field);field->setText(QString::number(pending.value(key).toDouble(group.value(key).toDouble(10))/scale,'g',8));}
    }
    if(active && pending.contains("previewDraft")){const auto preview=pending.value("previewDraft").toObject();view_->showSketchDraft(evaluateSketchDraft(preview));auto selection=session_.value("selection").toObject();selection["tool"]="chamfer";selection["entityIds"]=QJsonArray{pending.value("diagnostics").toObject().value("createdLineId")};view_->decorateSketch(preview,selection,session_.value("solverDiagnostics").toObject(),session_.value("inputs").toObject().value("units").toString());}
}
