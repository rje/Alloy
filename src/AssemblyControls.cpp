// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AssemblySlider.h"
#include "AssemblyRevolute.h"
#include "AssemblyGraph.h"
#include "ReferenceView.h"
#include <BRepAdaptor_Curve.hxx>
#include <GeomAbs_CurveType.hxx>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QShortcut>
#include <QTimer>
#include <QVBoxLayout>
#include <cmath>
#include <limits>

namespace {
QJsonObject fail(const QString& code,const QString& message){return {{"ok",false},{"completion","failed"},{"error",QJsonObject{{"code",code},{"message",message}}}};}
QJsonObject field(const QString& type,bool required=true){QJsonObject result{{"type",type},{"required",required}};
    if(type=="number"){result["minimum"]=-100000;result["maximum"]=100000;}return result;}
QJsonObject sliderSchema(){return {{"jointId",field("string",false)},{"name",field("string",false)},
    {"component1OccurrenceId",field("string",false)},{"component2OccurrenceId",field("string",false)},
    {"position",field("object",false)},{"axis",QJsonObject{{"type","string"},{"required",false},{"enum",QJsonArray{"Y"}}}},
    {"reverseDirection",field("boolean",false)},
    {"minimumEnabled",field("boolean",false)},{"minimumMm",field("number",false)},
    {"maximumEnabled",field("boolean",false)},{"maximumMm",field("number",false)},
    {"positionMm",field("number",false)}};}
QJsonObject revoluteSchema(){return {{"jointId",field("string",false)},{"name",field("string",false)},
    {"component1OccurrenceId",field("string",false)},{"component2OccurrenceId",field("string",false)},
    {"position",field("object",false)},{"angleDeg",QJsonObject{{"type","number"},{"required",false},{"minimum",-180},{"maximum",180}}}};}
QJsonObject cylindricalSchema(){return {{"jointId",field("string",false)},{"name",field("string",false)},
    {"component1OccurrenceId",field("string",false)},{"component2OccurrenceId",field("string",false)},
    {"position",field("object",false)},
    {"rotationDeg",QJsonObject{{"type","number"},{"required",false},{"minimum",-180},{"maximum",180}}},
    {"slideMm",QJsonObject{{"type","number"},{"required",false},{"minimum",-100000},{"maximum",100000}}}};}
QString ownerOf(const QJsonObject& authored,const QString& occurrenceId){
    for(const auto& value:authored.value("occurrences").toArray())if(value.toObject()["id"]==occurrenceId)return value.toObject()["definitionId"].toString();return {};
}
QJsonObject graphOverride(const QJsonObject& authored,const QJsonObject& cylinderStage,
    std::optional<double> revoluteAngle){
    if(!cylinderStage.isEmpty()){
        auto override=cylinderStage;override.remove("documentRevision");return override;
    }
    if(revoluteAngle)for(const auto& value:authored.value("joints").toArray()){
        const auto joint=value.toObject();if(joint["type"]=="asBuiltRevolute")
            return {{"jointId",joint["id"]},{"angleDeg",*revoluteAngle}};
    }
    return {};
}
bool passiveCylinder(const QJsonObject& assembly,const QString& jointId){
    if(jointId.isEmpty())return false;
    for(const auto& value:assembly.value("joints").toArray()){
        const auto joint=value.toObject();
        if(joint["id"]==jointId)return !joint.value("coordinateEditable").toBool(true);
    }
    return false;
}
}
std::vector<EvaluatedBody> AlloyWindow::presentationBodies() const {
    const auto& authored=sliderPreviewAuthored_.isEmpty() ? document_->authored() : sliderPreviewAuthored_;
    const auto& definitions=pendingReferenceEvaluation_ ? *pendingReferenceEvaluation_ : document_->evaluation();
    const auto joints=authored.value("joints").toArray();
    if(AssemblyGraph::isJointGraph(authored))return AssemblyGraph::worldBodies(authored,definitions,
        graphOverride(authored,cylinderStage_,transientSliderPosition_));
    return AssemblySlider::worldBodies(authored,definitions,transientSliderPosition_);
}
QJsonObject AlloyWindow::presentationAssembly() const {
    const auto& authored=sliderPreviewAuthored_.isEmpty() ? document_->authored() : sliderPreviewAuthored_;
    const auto joints=authored.value("joints").toArray();
    if(AssemblyGraph::isJointGraph(authored))return AssemblyGraph::inspect(authored,document_->evaluation(),
        graphOverride(authored,cylinderStage_,transientSliderPosition_));
    return AssemblySlider::inspect(authored,document_->evaluation(),transientSliderPosition_);
}
bool AlloyWindow::nonzeroAssemblyPose() const {
    const auto& authored=sliderPreviewAuthored_.isEmpty() ? document_->authored() : sliderPreviewAuthored_;
    const auto joints=authored.value("joints").toArray();
    if(joints.isEmpty())return false;
    if(AssemblyGraph::isJointGraph(authored)){
        if(!cylinderStage_.isEmpty() || transientSliderPosition_.has_value())return true;
        for(const auto& value:joints){const auto joint=value.toObject();
            if(std::abs(joint["angleDeg"].toDouble())>1e-9 || std::abs(joint["rotationDeg"].toDouble())>1e-9
                || std::abs(joint["slideMm"].toDouble())>1e-9)return true;}
        return false;
    }
    return std::abs(AssemblySlider::position(authored,transientSliderPosition_))>1e-9;
}
void AlloyWindow::registerSliderCommands(){
    commands_.push_back({"joint.slider","As-Built Slider","Create or edit one root-sibling Slider joint without moving definition geometry.",sliderSchema(),true,[this](const QJsonObject& args){
        if(!session_.isEmpty() && session_.value("command")!="joint.slider")return fail("session_busy","Complete or cancel the active command first");
        try{
            const auto candidate=AssemblySlider::candidate(document_->authored(),args);
            const auto error=document_->commit(candidate,args.contains("jointId") ? "Edit Slider" : "As-Built Slider");
            if(!error.isEmpty())return fail("evaluation_failed",error);
            sliderPreviewAuthored_={};transientSliderPosition_.reset();displayedDocumentRevision_=std::numeric_limits<quint64>::max();
            return document_->state();
        }catch(const std::exception& e){return fail("invalid_slider",QString::fromUtf8(e.what()));}
    }});
    commands_.push_back({"joint.sliderDialog","As-Built Joint","Select two existing components, a ledge Position, Y axis and motion limits.",sliderSchema(),true,
        [this](const QJsonObject& args){return beginSlider(args);}});
    commands_.push_back({"joint.inspect","Inspect Joint","Read the retained joint, current pose and occurrence world bounds.",{},true,
        [this](const QJsonObject&){return presentationAssembly();}});
    commands_.push_back({"joint.positionSet","Set Slider position","Persist and undo a numeric Slider coordinate within its enabled limits.",
        {{"jointId",field("string")},{"positionMm",field("number")}},true,[this](const QJsonObject& args){
            if(!session_.isEmpty())return fail("session_busy","Finish or cancel the active command first");
            try{const auto candidate=AssemblySlider::candidate(document_->authored(),args);
                const auto error=document_->commit(candidate,"Drive Slider");if(!error.isEmpty())return fail("evaluation_failed",error);
                transientSliderPosition_.reset();displayedDocumentRevision_=std::numeric_limits<quint64>::max();return presentationAssembly();
            }catch(const std::exception& e){return fail("invalid_slider",QString::fromUtf8(e.what()));}
        }});
    commands_.push_back({"joint.previewPosition","Preview Slider position","Temporarily display a Slider coordinate without an authored transaction.",
        {{"positionMm",field("number")}},true,[this](const QJsonObject& args){
            if(!session_.isEmpty() && session_.value("command")!="joint.slider")return fail("session_busy","Finish or cancel the active command before Slider motion");
            try{const auto& authored=sliderPreviewAuthored_.isEmpty() ? document_->authored() : sliderPreviewAuthored_;
                if(authored.value("joints").toArray().isEmpty())return fail("joint_required","Create or preview a Slider first");
                const double coordinate=args["positionMm"].toDouble();AssemblySlider::inspect(authored,document_->evaluation(),coordinate);
                transientSliderPosition_=coordinate;displayedDocumentRevision_=std::numeric_limits<quint64>::max();return presentationAssembly();
            }catch(const std::exception& e){return fail("invalid_slider",QString::fromUtf8(e.what()));}
        }});
    commands_.push_back({"joint.animate","Animate Slider","Preview bounded motion without changing authored geometry or undo history.",{},true,[this](const QJsonObject&){
        if(!session_.isEmpty() && session_.value("command")!="joint.slider")return fail("session_busy","Finish or cancel the active command before Slider motion");
        const auto& authored=sliderPreviewAuthored_.isEmpty() ? document_->authored() : sliderPreviewAuthored_;
        const auto rows=authored.value("joints").toArray();if(rows.isEmpty())return fail("joint_required","Create a Slider first");
        const auto joint=rows[0].toObject();if(!joint["minimumEnabled"].toBool() || !joint["maximumEnabled"].toBool()
            || joint["maximumMm"].toDouble()<=joint["minimumMm"].toDouble())return fail("limits_required","Enable distinct finite Minimum and Maximum to animate");
        if(!sliderTimer_){sliderTimer_=new QTimer(this);sliderTimer_->setInterval(33);connect(sliderTimer_,&QTimer::timeout,this,[this]{
            const auto& authored=sliderPreviewAuthored_.isEmpty() ? document_->authored() : sliderPreviewAuthored_;
            const auto rows=authored.value("joints").toArray();if(rows.isEmpty()){sliderTimer_->stop();return;}
            const auto joint=rows[0].toObject();const double low=joint["minimumMm"].toDouble(),high=joint["maximumMm"].toDouble();
            const int phase=(++sliderAnimationFrame_)%120;const double fraction=phase<=60 ? phase/60.0 : (120-phase)/60.0;
            transientSliderPosition_=low+(high-low)*fraction;displayedDocumentRevision_=std::numeric_limits<quint64>::max();refresh();
        });}
        sliderAnimationFrame_=0;sliderTimer_->start();return QJsonObject{{"running",true},{"transient",true}};
    }});
    commands_.push_back({"joint.stop","Stop Slider preview","Restore the persisted position without a history edit.",{},true,[this](const QJsonObject&){
        if(sliderTimer_)sliderTimer_->stop();transientSliderPosition_.reset();displayedDocumentRevision_=std::numeric_limits<quint64>::max();return presentationAssembly();
    }});
}
QJsonObject AlloyWindow::beginSlider(const QJsonObject& overrides){
    if(!session_.isEmpty())return fail("session_busy","Complete or cancel the active command first");
    QJsonObject inputs{{"axis","Y"},{"reverseDirection",false},{"name","Slider"},{"minimumEnabled",false},{"minimumMm",0},
        {"maximumEnabled",false},{"maximumMm",0},{"positionMm",0}};
    const auto existing=document_->authored().value("joints").toArray();
    if(!existing.isEmpty()){
        const auto record=existing[0].toObject();if(overrides.contains("jointId") && overrides["jointId"]!=record["id"])return fail("stale_joint","Slider joint is unavailable");
        if(record["type"]!="asBuiltSlider")return fail("stale_joint","Existing joint is not a Slider");
        inputs=record;inputs.remove("id");inputs.remove("type");inputs["jointId"]=record["id"];
    }
    for(auto it=overrides.begin();it!=overrides.end();++it)inputs[it.key()]=it.value();
    session_={{"id",CadDocument::newId()},{"command","joint.slider"},{"status","active"},{"inputs",inputs},
        {"baseDocumentRevision",static_cast<qint64>(document_->revision())},{"previewed",false},{"inputSchema",sliderSchema()}};
    showSlider();previewSlider();return session_;
}
QJsonObject AlloyWindow::previewSlider(){
    if(session_.value("command")!="joint.slider")return fail("invalid_session","Slider dialog is not active");
    if(session_["baseDocumentRevision"].toInteger()!=static_cast<qint64>(document_->revision()))return fail("stale_session","The document changed; cancel and restart Slider");
    try{
        const auto candidate=AssemblySlider::candidate(document_->authored(),session_["inputs"].toObject());
        const auto prepared=document_->prepareCandidate(candidate);
        if(!prepared.ok){sliderPreviewAuthored_={};session_["previewed"]=false;refreshSliderControls();return fail("evaluation_failed",prepared.error);}
        const auto resolved=AssemblySlider::inspect(prepared.authored,*prepared.evaluation,transientSliderPosition_);
        sliderPreviewAuthored_=prepared.authored;session_["documentDraft"]=prepared.authored;session_["assembly"]=resolved;
        session_["previewed"]=true;displayedDocumentRevision_=std::numeric_limits<quint64>::max();refreshSliderControls();return session_;
    }catch(const std::exception& e){sliderPreviewAuthored_={};session_["previewed"]=false;refreshSliderControls();return fail("invalid_slider",QString::fromUtf8(e.what()));}
}
void AlloyWindow::showSlider(){
    if(sliderDock_)delete sliderDock_;
    auto* dock=new QDockWidget("As-Built Joint",canvasWindow_);sliderDock_=dock;dock->setObjectName("sliderDock");
    auto* escape=new QShortcut(QKeySequence(Qt::Key_Escape),dock);escape->setContext(Qt::WidgetWithChildrenShortcut);
    connect(escape,&QShortcut::activated,this,[this]{if(session_.value("command")=="joint.slider")execute("command.cancel",{{"session",session_["id"]}});});
    dock->setAllowedAreas(Qt::LeftDockWidgetArea|Qt::RightDockWidgetArea);
    auto* panel=new QWidget;panel->setObjectName("sliderPanel");panel->setFixedWidth(290);dock->setWidget(panel);
    auto* layout=new QVBoxLayout(panel);layout->setContentsMargins(14,14,14,12);layout->setSpacing(12);
    auto* form=new QFormLayout;form->setVerticalSpacing(10);
    auto* first=new QComboBox;first->setObjectName("slider.component1");form->addRow("Component 1",first);
    auto* second=new QComboBox;second->setObjectName("slider.component2");form->addRow("Component 2",second);
    auto* position=new QComboBox;position->setObjectName("slider.position");form->addRow("Position",position);
    auto* kind=new QLabel("Slider");form->addRow("Type",kind);
    auto* axis=new QLabel("Y Axis");form->addRow("Slide",axis);
    auto* reverse=new QCheckBox("Reverse Direction");reverse->setObjectName("slider.reverseDirection");
    reverse->setToolTip("Positive position moves Component 1 toward negative Y.");form->addRow("",reverse);
    auto* minimumEnabled=new QCheckBox("Minimum");minimumEnabled->setObjectName("slider.minimumEnabled");
    auto* minimum=new QDoubleSpinBox;minimum->setObjectName("slider.minimumMm");minimum->setRange(-100000,100000);minimum->setDecimals(4);minimum->setSuffix(" mm");form->addRow(minimumEnabled,minimum);
    auto* maximumEnabled=new QCheckBox("Maximum");maximumEnabled->setObjectName("slider.maximumEnabled");
    auto* maximum=new QDoubleSpinBox;maximum->setObjectName("slider.maximumMm");maximum->setRange(-100000,100000);maximum->setDecimals(4);maximum->setSuffix(" mm");form->addRow(maximumEnabled,maximum);
    auto* coordinate=new QDoubleSpinBox;coordinate->setObjectName("slider.positionMm");coordinate->setRange(-100000,100000);coordinate->setDecimals(4);coordinate->setSuffix(" mm");form->addRow("Position value",coordinate);
    layout->addLayout(form);
    auto* error=new QLabel;error->setObjectName("slider.error");error->setWordWrap(true);error->setStyleSheet("color:#a63832;");layout->addWidget(error);
    auto* animate=new QPushButton("Preview Motion");animate->setObjectName("slider.animate");layout->addWidget(animate);
    auto* help=new QLabel("Pick a retained straight ledge edge on Component 2. Component 1 slides along Y; Reverse Direction moves it toward negative Y. Component 2 is the assembly reference.");help->setWordWrap(true);layout->addWidget(help);
    layout->addStretch();auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);buttons->setObjectName("slider.buttons");layout->addWidget(buttons);
    auto update=[this,error](const QJsonObject& changes){const auto response=execute("command.update",{{"session",session_["id"]},{"inputs",changes}});
        error->setText(response["ok"].toBool() ? QString{} : response["error"].toObject()["message"].toString());};
    connect(first,qOverload<int>(&QComboBox::currentIndexChanged),this,[first,update](int i){if(i>=0)update({{"component1OccurrenceId",first->itemData(i).toString()}});});
    connect(second,qOverload<int>(&QComboBox::currentIndexChanged),this,[second,update](int i){if(i>=0)update({{"component2OccurrenceId",second->itemData(i).toString()}});});
    connect(position,qOverload<int>(&QComboBox::currentIndexChanged),this,[position,update](int i){if(i>=0)update({{"position",position->itemData(i).toJsonObject()}});});
    connect(reverse,&QCheckBox::toggled,this,[update](bool enabled){update({{"reverseDirection",enabled}});});
    connect(minimumEnabled,&QCheckBox::toggled,this,[update](bool enabled){update({{"minimumEnabled",enabled}});});
    connect(maximumEnabled,&QCheckBox::toggled,this,[update](bool enabled){update({{"maximumEnabled",enabled}});});
    connect(minimum,&QDoubleSpinBox::editingFinished,this,[minimum,update]{update({{"minimumMm",minimum->value()}});});
    connect(maximum,&QDoubleSpinBox::editingFinished,this,[maximum,update]{update({{"maximumMm",maximum->value()}});});
    connect(coordinate,&QDoubleSpinBox::editingFinished,this,[coordinate,update]{update({{"positionMm",coordinate->value()}});});
    connect(animate,&QPushButton::clicked,this,[this,animate,error]{const auto running=sliderTimer_ && sliderTimer_->isActive();
        const auto response=execute(running ? "joint.stop" : "joint.animate");error->setText(response["ok"].toBool() ? QString{} : response["error"].toObject()["message"].toString());
        animate->setText(running ? "Preview Motion" : "Stop Motion");});
    connect(buttons,&QDialogButtonBox::accepted,this,[this,error]{const auto response=execute("command.commit",{{"session",session_["id"]}});
        if(!response["ok"].toBool())error->setText(response["error"].toObject()["message"].toString());});
    connect(buttons,&QDialogButtonBox::rejected,this,[this]{execute("command.cancel",{{"session",session_["id"]}});});
    canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,dock);dock->show();refreshSliderControls();
}
void AlloyWindow::refreshSliderControls(){
    if(!sliderDock_ || session_.value("command")!="joint.slider")return;
    const auto inputs=session_["inputs"].toObject();
    auto* first=sliderDock_->findChild<QComboBox*>("slider.component1");auto* second=sliderDock_->findChild<QComboBox*>("slider.component2");
    auto* position=sliderDock_->findChild<QComboBox*>("slider.position");
    QMap<QString,QString> names;for(const auto& value:document_->authored()["components"].toArray())names[value.toObject()["id"].toString()]=value.toObject()["name"].toString();
    for(auto* combo:{first,second}){const QSignalBlocker block(combo);combo->clear();combo->addItem("Select component…",QString{});
        for(const auto& value:document_->authored()["occurrences"].toArray()){const auto occurrence=value.toObject();combo->addItem(names[occurrence["definitionId"].toString()],occurrence["id"].toString());}
        combo->setCurrentIndex(std::max(0,combo->findData(inputs[combo==first ? "component1OccurrenceId" : "component2OccurrenceId"].toString())));
    }
    const auto fixed=ownerOf(document_->authored(),inputs["component2OccurrenceId"].toString());
    {const QSignalBlocker block(position);position->clear();position->addItem("Select ledge edge…",QJsonObject{});
        for(const auto& body:document_->bodies())if(body.ownerId==fixed)for(const auto& edge:body.solidEdges)if(!edge.edge.IsNull()){
            const auto label=body.name+" · "+edge.provenance.value("boxEdge").toString(edge.provenance.value("role").toString("straight edge"));
            position->addItem(label,QJsonObject{{"bodyId",body.bodyId},{"edgeId",edge.edgeId}});
        }
        position->setCurrentIndex(std::max(0,position->findData(inputs["position"].toObject())));
    }
    for(const auto& key:{"reverseDirection","minimumEnabled","maximumEnabled"}){auto* check=sliderDock_->findChild<QCheckBox*>("slider."+QString(key));const QSignalBlocker block(check);check->setChecked(inputs[key].toBool());}
    for(const auto& key:{"minimumMm","maximumMm","positionMm"}){auto* field=sliderDock_->findChild<QDoubleSpinBox*>("slider."+QString(key));const QSignalBlocker block(field);field->setValue(inputs[key].toDouble());}
    sliderDock_->findChild<QDoubleSpinBox*>("slider.minimumMm")->setEnabled(inputs["minimumEnabled"].toBool());
    sliderDock_->findChild<QDoubleSpinBox*>("slider.maximumMm")->setEnabled(inputs["maximumEnabled"].toBool());
    sliderDock_->findChild<QDialogButtonBox*>("slider.buttons")->button(QDialogButtonBox::Ok)->setEnabled(session_["previewed"].toBool());
    const auto choice=inputs["position"].toObject();if(!choice.isEmpty())view_->highlightSolidEdges(presentationBodies(),choice["bodyId"].toString(),QJsonArray{choice["edgeId"]});
}
void AlloyWindow::registerRevoluteCommands(){
    commands_.push_back({"joint.revolute","As-Built Revolute","Retain one circular-origin Revolute between root-level leaf components.",
        revoluteSchema(),true,[this](const QJsonObject& args){
            if(!session_.isEmpty() && session_["command"]!="joint.revolute")return fail("session_busy","Finish or cancel the active command");
            try {const auto candidate=AssemblyRevolute::candidate(document_->authored(),document_->evaluation(),args);
                const auto error=document_->commit(candidate,args.contains("jointId") ? "Edit Revolute" : "As-Built Revolute");
                if(!error.isEmpty())return fail("evaluation_failed",error);
                sliderPreviewAuthored_={};transientSliderPosition_.reset();displayedDocumentRevision_=std::numeric_limits<quint64>::max();
                return presentationAssembly();
            }catch(const std::exception& e){return fail("invalid_revolute",QString::fromUtf8(e.what()));}
        }});
    commands_.push_back({"joint.revoluteDialog","As-Built Joint","Select two components and a retained circular Position for Revolute.",
        revoluteSchema(),true,[this](const QJsonObject& args){return beginRevolute(args);}});
    commands_.push_back({"joint.angleSet","Set Revolute angle","Persist an exact prescribed Revolute angle in one undoable transaction.",
        {{"jointId",field("string")},{"angleDeg",QJsonObject{{"type","number"},{"minimum",-180},{"maximum",180}}}},true,[this](const QJsonObject& args){
            if(!session_.isEmpty())return fail("session_busy","Finish or cancel the active command");
            try {const auto candidate=AssemblyRevolute::candidate(document_->authored(),document_->evaluation(),args);
                if(const auto error=document_->commit(candidate,"Drive Revolute");!error.isEmpty())return fail("evaluation_failed",error);
                transientSliderPosition_.reset();return presentationAssembly();
            }catch(const std::exception& e){return fail("invalid_revolute",QString::fromUtf8(e.what()));}
        }});
    commands_.push_back({"joint.previewAngle","Preview Revolute angle","Show a temporary exact Revolute pose without authored history.",
        {{"angleDeg",QJsonObject{{"type","number"},{"minimum",-180},{"maximum",180}}}},true,[this](const QJsonObject& args){
            if(!session_.isEmpty() && session_["command"]!="joint.revolute")return fail("session_busy","Finish or cancel the active command");
            const auto& authored=sliderPreviewAuthored_.isEmpty()?document_->authored():sliderPreviewAuthored_;
            try {const double angle=args["angleDeg"].toDouble();
                const auto resolved=AssemblyRevolute::inspect(authored,document_->evaluation(),angle);
                cylinderStage_={};transientSliderPosition_=angle;displayedDocumentRevision_=std::numeric_limits<quint64>::max();return resolved;
            }catch(const std::exception& e){return fail("invalid_revolute",QString::fromUtf8(e.what()));}
        }});
    commands_.push_back({"joint.revoluteStop","Stop Revolute preview","Restore the retained angle without an authored edit.",{},true,
        [this](const QJsonObject&){transientSliderPosition_.reset();displayedDocumentRevision_=std::numeric_limits<quint64>::max();return presentationAssembly();}});
    commands_.push_back({"joint.pin","Pin Component","Retain the current full root pose of one identity leaf as a Timeline feature.",
        {{"occurrenceId",field("string")}},true,[this](const QJsonObject& args){
            if(!session_.isEmpty())return fail("session_busy","Finish or cancel the active command");
            try {const auto candidate=CadDocument::pinOccurrenceCandidate(document_->authored(),args["occurrenceId"].toString());
                if(const auto error=document_->commit(candidate,"Pin Component");!error.isEmpty())return fail("evaluation_failed",error);
                return QJsonObject{{"feature",candidate["features"].toArray().last()},{"assembly",presentationAssembly()}};
            }catch(const std::exception& e){return fail("invalid_pin",QString::fromUtf8(e.what()));}
        }});
}
QJsonObject AlloyWindow::beginRevolute(const QJsonObject& overrides){
    if(!session_.isEmpty())return fail("session_busy","Complete or cancel the active command");
    QJsonObject inputs{{"name","Revolute"},{"angleDeg",0}};
    const auto old=document_->authored().value("joints").toArray();
    if(!old.isEmpty()){
        const auto record=old[0].toObject();
        if(record["type"]!="asBuiltRevolute" || (overrides.contains("jointId") && overrides["jointId"]!=record["id"]))
            return fail("stale_joint","Revolute joint is unavailable");
        inputs=record;inputs.remove("id");inputs.remove("type");inputs.remove("movingMarkerLocal");inputs["jointId"]=record["id"];
    }
    for(auto it=overrides.begin();it!=overrides.end();++it)inputs[it.key()]=it.value();
    session_={{"id",CadDocument::newId()},{"command","joint.revolute"},{"status","active"},
        {"inputs",inputs},{"baseDocumentRevision",static_cast<qint64>(document_->revision())},
        {"previewed",false},{"inputSchema",revoluteSchema()}};
    showRevolute();previewRevolute();return session_;
}
QJsonObject AlloyWindow::previewRevolute(){
    if(session_["command"]!="joint.revolute")return fail("invalid_session","Revolute dialog is not active");
    if(session_["baseDocumentRevision"].toInteger()!=static_cast<qint64>(document_->revision()))
        return fail("stale_session","Document changed; cancel and restart Revolute");
    try {const auto candidate=AssemblyRevolute::candidate(document_->authored(),document_->evaluation(),session_["inputs"].toObject());
        const auto prepared=document_->prepareCandidate(candidate);
        if(!prepared.ok)throw std::invalid_argument(prepared.error.toStdString());
        const auto assembly=AssemblyRevolute::inspect(prepared.authored,*prepared.evaluation);
        sliderPreviewAuthored_=prepared.authored;session_["documentDraft"]=prepared.authored;
        session_["assembly"]=assembly;session_["previewed"]=true;session_.remove("previewError");
        displayedDocumentRevision_=std::numeric_limits<quint64>::max();refreshRevoluteControls();return session_;
    }catch(const std::exception& e){sliderPreviewAuthored_={};session_["previewed"]=false;
        session_["previewError"]=QString::fromUtf8(e.what());refreshRevoluteControls();return fail("invalid_revolute",QString::fromUtf8(e.what()));}
}
void AlloyWindow::showRevolute(){
    if(sliderDock_)delete sliderDock_;
    auto* dock=new QDockWidget("As-Built Joint",canvasWindow_);sliderDock_=dock;dock->setObjectName("revoluteDock");
    auto* escape=new QShortcut(QKeySequence(Qt::Key_Escape),dock);escape->setContext(Qt::WidgetWithChildrenShortcut);
    connect(escape,&QShortcut::activated,this,[this]{if(session_["command"]=="joint.revolute")
        execute("command.cancel",{{"session",session_["id"]}});});
    auto* panel=new QWidget;panel->setObjectName("revolutePanel");panel->setFixedWidth(306);dock->setWidget(panel);
    auto* layout=new QVBoxLayout(panel);layout->setContentsMargins(14,14,14,12);layout->setSpacing(10);
    auto* title=new QLabel("As-Built Joint · Revolute");title->setWordWrap(true);layout->addWidget(title);
    auto* form=new QFormLayout;form->setVerticalSpacing(10);
    auto* first=new QComboBox;first->setObjectName("revolute.component1");form->addRow("Component 1",first);
    auto* second=new QComboBox;second->setObjectName("revolute.component2");form->addRow("Component 2",second);
    auto* position=new QComboBox;position->setObjectName("revolute.position");form->addRow("Position",position);
    auto* angle=new QDoubleSpinBox;angle->setObjectName("revolute.angleDeg");angle->setDecimals(3);
    angle->setRange(-180,180);angle->setSuffix(" °");form->addRow("Angle",angle);layout->addLayout(form);
    const bool editing=session_["inputs"].toObject().contains("jointId");
    first->setEnabled(!editing);second->setEnabled(!editing);position->setEnabled(!editing);
    auto* hint=new QLabel(editing ? "Edit the prescribed angle. To change components or Position, create a new as-built joint."
        : "Choose a full circular edge on Component 2. Angle 0 retains the assembled pose.");hint->setWordWrap(true);layout->addWidget(hint);
    auto* error=new QLabel;error->setObjectName("revolute.error");error->setWordWrap(true);layout->addWidget(error);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);buttons->setObjectName("revolute.buttons");layout->addWidget(buttons);
    const auto update=[this](const QJsonObject& changes){execute("command.update",{{"session",session_["id"]},{"inputs",changes}});};
    connect(first,qOverload<int>(&QComboBox::currentIndexChanged),this,[first,update](int index){if(index>=0)update({{"component1OccurrenceId",first->itemData(index).toString()}});});
    connect(second,qOverload<int>(&QComboBox::currentIndexChanged),this,[second,update](int index){if(index>=0)update({{"component2OccurrenceId",second->itemData(index).toString()}});});
    connect(position,qOverload<int>(&QComboBox::currentIndexChanged),this,[position,update](int index){if(index>=0)update({{"position",position->itemData(index).toJsonObject()}});});
    connect(angle,&QDoubleSpinBox::editingFinished,this,[angle,update]{update({{"angleDeg",angle->value()}});});
    connect(buttons,&QDialogButtonBox::accepted,this,[this,error]{const auto response=execute("command.commit",{{"session",session_["id"]}});
        if(!response["ok"].toBool())error->setText(response["error"].toObject()["message"].toString());});
    connect(buttons,&QDialogButtonBox::rejected,this,[this]{execute("command.cancel",{{"session",session_["id"]}});});
    canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,dock);dock->show();refreshRevoluteControls();
}
void AlloyWindow::refreshRevoluteControls(){
    if(!sliderDock_ || session_["command"]!="joint.revolute")return;
    const auto inputs=session_["inputs"].toObject();
    auto* first=sliderDock_->findChild<QComboBox*>("revolute.component1");
    auto* second=sliderDock_->findChild<QComboBox*>("revolute.component2");
    auto* position=sliderDock_->findChild<QComboBox*>("revolute.position");
    QMap<QString,QString> names;for(const auto& value:document_->authored()["components"].toArray())
        names[value.toObject()["id"].toString()]=value.toObject()["name"].toString();
    for(auto* combo:{first,second}){const QSignalBlocker blocked(combo);combo->clear();combo->addItem("Select component…",QString{});
        for(const auto& value:document_->authored()["occurrences"].toArray()){
            const auto occurrence=value.toObject();combo->addItem(names[occurrence["definitionId"].toString()],occurrence["id"].toString());
        }
        combo->setCurrentIndex(std::max(0,combo->findData(inputs[combo==first?"component1OccurrenceId":"component2OccurrenceId"].toString())));
    }
    const auto fixed=ownerOf(document_->authored(),inputs["component2OccurrenceId"].toString());
    {const QSignalBlocker blocked(position);position->clear();position->addItem("Select circular edge…",QJsonObject{});
        for(const auto& body:document_->bodies())if(body.ownerId==fixed)for(const auto& edge:body.solidEdges)if(!edge.edge.IsNull()){
            const BRepAdaptor_Curve curve(edge.edge);if(curve.GetType()!=GeomAbs_Circle)continue;
            if(std::abs(std::abs(curve.LastParameter()-curve.FirstParameter())-2*std::acos(-1.0))>1e-6)continue;
            const auto point=curve.Circle().Location();
            const auto label=body.name+QString(" · circle Y %1").arg(point.Y(),0,'f',1);
            position->addItem(label,QJsonObject{{"bodyId",body.bodyId},{"edgeId",edge.edgeId}});
            position->setItemData(position->count()-1,QString("Center (%1, %2, %3) mm")
                .arg(point.X(),0,'f',1).arg(point.Y(),0,'f',1).arg(point.Z(),0,'f',1),Qt::ToolTipRole);
        }
        position->setCurrentIndex(std::max(0,position->findData(inputs["position"].toObject())));
    }
    {auto* angle=sliderDock_->findChild<QDoubleSpinBox*>("revolute.angleDeg");const QSignalBlocker blocked(angle);angle->setValue(inputs["angleDeg"].toDouble());}
    sliderDock_->findChild<QLabel*>("revolute.error")->setText(session_["previewError"].toString());
    sliderDock_->findChild<QDialogButtonBox*>("revolute.buttons")->button(QDialogButtonBox::Ok)->setEnabled(session_["previewed"].toBool());
    const auto ref=inputs["position"].toObject();if(!ref.isEmpty())view_->highlightSolidEdges(presentationBodies(),ref["bodyId"].toString(),QJsonArray{ref["edgeId"]});
}
void AlloyWindow::registerCylindricalCommands(){
    commands_.push_back({"joint.cylindrical","As-Built Cylindrical",
        "Retain one Z-axis circular-origin Cylindrical relationship between component leaves.",
        cylindricalSchema(),true,[this](const QJsonObject& args){
            if(!session_.isEmpty() && session_["command"]!="joint.cylindrical")return fail("session_busy","Finish or cancel the active command");
            try{const auto candidate=AssemblyGraph::cylindricalCandidate(document_->authored(),document_->evaluation(),args);
                if(const auto error=document_->commit(candidate,args.contains("jointId") ? "Edit Cylindrical" : "As-Built Cylindrical");!error.isEmpty())
                    return fail("evaluation_failed",error);
                sliderPreviewAuthored_={};transientSliderPosition_.reset();cylinderStage_={};displayedDocumentRevision_=std::numeric_limits<quint64>::max();
                return presentationAssembly();
            }catch(const std::exception& e){return fail("invalid_cylindrical",QString::fromUtf8(e.what()));}
        }});
    commands_.push_back({"joint.cylindricalDialog","As-Built Joint",
        "Select two components and a retained Z-axis circular Position for Cylindrical.",
        cylindricalSchema(),true,[this](const QJsonObject& args){return beginCylindrical(args);}});
    commands_.push_back({"joint.previewCylindrical","Preview Cylindrical coordinates",
        "Stage exact rotation and axial slide without changing the authored joint.",
        {{"jointId",field("string")},{"rotationDeg",QJsonObject{{"type","number"},{"minimum",-180},{"maximum",180}}},
            {"slideMm",field("number")}},true,[this](const QJsonObject& args){
            if(!session_.isEmpty() && session_["command"]!="joint.cylindrical")return fail("session_busy","Finish or cancel the active command");
            const auto& authored=sliderPreviewAuthored_.isEmpty()?document_->authored():sliderPreviewAuthored_;
            try{AssemblyGraph::inspect(authored,document_->evaluation(),args);
                transientSliderPosition_.reset();cylinderStage_=args;cylinderStage_["documentRevision"]=static_cast<qint64>(document_->revision());
                displayedDocumentRevision_=std::numeric_limits<quint64>::max();return presentationAssembly();
            }catch(const std::exception& e){return fail("invalid_cylindrical",QString::fromUtf8(e.what()));}
        }});
    commands_.push_back({"joint.cylindricalStop","Stop Cylindrical preview",
        "Restore all persisted occurrence poses without an authored history edit.",{},true,[this](const QJsonObject&){
            cylinderStage_={};displayedDocumentRevision_=std::numeric_limits<quint64>::max();return presentationAssembly();
        }});
}
QJsonObject AlloyWindow::beginCylindrical(const QJsonObject& overrides){
    if(!session_.isEmpty())return fail("session_busy","Finish or cancel the active command first");
    QJsonObject inputs{{"name","Cylindrical"},{"rotationDeg",0},{"slideMm",0}};
    const auto edit=overrides["jointId"].toString();
    if(!edit.isEmpty()){
        bool found=false;for(const auto& value:document_->authored().value("joints").toArray()){
            const auto joint=value.toObject();if(joint["id"]!=edit)continue;
            if(joint["type"]!="asBuiltCylindrical")return fail("stale_joint","Selected joint is not Cylindrical");
            inputs={{"jointId",edit},{"name",joint["name"]},{"component1OccurrenceId",joint["component1OccurrenceId"]},
                {"component2OccurrenceId",joint["component2OccurrenceId"]},{"position",joint["position"]},
                {"rotationDeg",joint["rotationDeg"]},{"slideMm",joint["slideMm"]}};
            found=true;break;
        }
        if(!found)return fail("stale_joint","Cylindrical joint is unavailable");
    }
    for(auto it=overrides.begin();it!=overrides.end();++it)inputs[it.key()]=it.value();
    session_={{"id",CadDocument::newId()},{"command","joint.cylindrical"},{"status","active"},
        {"inputs",inputs},{"baseDocumentRevision",static_cast<qint64>(document_->revision())},
        {"previewed",false},{"inputSchema",cylindricalSchema()}};
    showCylindrical();previewCylindrical();return session_;
}
QJsonObject AlloyWindow::previewCylindrical(){
    if(session_["command"]!="joint.cylindrical")return fail("invalid_session","Cylindrical dialog is not active");
    if(session_["baseDocumentRevision"].toInteger()!=static_cast<qint64>(document_->revision()))
        return fail("stale_session","Document changed; cancel and restart Cylindrical");
    try{const auto candidate=AssemblyGraph::cylindricalCandidate(document_->authored(),document_->evaluation(),session_["inputs"].toObject());
        const auto prepared=document_->prepareCandidate(candidate);
        if(!prepared.ok)throw std::invalid_argument(prepared.error.toStdString());
        const auto assembly=AssemblyGraph::inspect(prepared.authored,*prepared.evaluation);
        sliderPreviewAuthored_=prepared.authored;session_["documentDraft"]=prepared.authored;
        session_["assembly"]=assembly;session_["previewed"]=true;session_.remove("previewError");
        displayedDocumentRevision_=std::numeric_limits<quint64>::max();refreshCylindricalControls();return session_;
    }catch(const std::exception& e){sliderPreviewAuthored_={};session_["previewed"]=false;
        session_["previewError"]=QString::fromUtf8(e.what());refreshCylindricalControls();
        return fail("invalid_cylindrical",QString::fromUtf8(e.what()));}
}
void AlloyWindow::showCylindrical(){
    if(sliderDock_)delete sliderDock_;
    auto* dock=new QDockWidget("As-Built Joint",canvasWindow_);sliderDock_=dock;dock->setObjectName("cylindricalDock");
    auto* escape=new QShortcut(QKeySequence(Qt::Key_Escape),dock);escape->setContext(Qt::WidgetWithChildrenShortcut);
    connect(escape,&QShortcut::activated,this,[this]{if(session_["command"]=="joint.cylindrical")
        execute("command.cancel",{{"session",session_["id"]}});});
    auto* panel=new QWidget;panel->setObjectName("cylindricalPanel");panel->setFixedWidth(318);dock->setWidget(panel);
    auto* layout=new QVBoxLayout(panel);layout->setContentsMargins(14,14,14,12);layout->setSpacing(10);
    auto* title=new QLabel("As-Built Joint · Cylindrical");title->setWordWrap(true);layout->addWidget(title);
    auto* form=new QFormLayout;form->setVerticalSpacing(10);
    auto* first=new QComboBox;first->setObjectName("cylindrical.component1");form->addRow("Component 1",first);
    auto* second=new QComboBox;second->setObjectName("cylindrical.component2");form->addRow("Component 2",second);
    auto* position=new QComboBox;position->setObjectName("cylindrical.position");form->addRow("Position",position);
    form->addRow("Type",new QLabel("Cylindrical · rotate + slide"));
    auto* rotation=new QDoubleSpinBox;rotation->setObjectName("cylindrical.rotationDeg");rotation->setRange(-180,180);
    rotation->setDecimals(3);rotation->setSuffix(" °");form->addRow("Rotation",rotation);
    auto* slide=new QDoubleSpinBox;slide->setObjectName("cylindrical.slideMm");slide->setRange(-100000,100000);
    slide->setDecimals(3);slide->setSuffix(" mm");form->addRow("Slide",slide);layout->addLayout(form);
    const bool editing=session_["inputs"].toObject().contains("jointId");
    first->setEnabled(!editing);second->setEnabled(!editing);position->setEnabled(!editing);
    const bool passive=editing && passiveCylinder(presentationAssembly(),session_["inputs"].toObject()["jointId"].toString());
    rotation->setEnabled(!passive);slide->setEnabled(!passive);
    auto* hint=new QLabel(passive ? "This joint is driven by another joint in the connected island. Its coordinates are read-only."
        : editing ? "Edit both coordinates. To change components or Position, create a new as-built joint."
        : "Choose a complete circular edge on Component 2. This slice uses its Z axis and retains the assembled pose.");
    hint->setObjectName("cylindrical.hint");
    hint->setWordWrap(true);layout->addWidget(hint);
    auto* error=new QLabel;error->setObjectName("cylindrical.error");error->setWordWrap(true);layout->addWidget(error);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);buttons->setObjectName("cylindrical.buttons");layout->addWidget(buttons);
    const auto update=[this](const QJsonObject& changes){execute("command.update",{{"session",session_["id"]},{"inputs",changes}});};
    connect(first,qOverload<int>(&QComboBox::currentIndexChanged),this,[first,update](int i){if(i>=0)update({{"component1OccurrenceId",first->itemData(i).toString()}});});
    connect(second,qOverload<int>(&QComboBox::currentIndexChanged),this,[second,update](int i){if(i>=0)update({{"component2OccurrenceId",second->itemData(i).toString()}});});
    connect(position,qOverload<int>(&QComboBox::currentIndexChanged),this,[position,update](int i){if(i>=0)update({{"position",position->itemData(i).toJsonObject()}});});
    connect(rotation,&QDoubleSpinBox::editingFinished,this,[rotation,update]{update({{"rotationDeg",rotation->value()}});});
    connect(slide,&QDoubleSpinBox::editingFinished,this,[slide,update]{update({{"slideMm",slide->value()}});});
    connect(buttons,&QDialogButtonBox::accepted,this,[this,error]{const auto response=execute("command.commit",{{"session",session_["id"]}});
        if(!response["ok"].toBool())error->setText(response["error"].toObject()["message"].toString());});
    connect(buttons,&QDialogButtonBox::rejected,this,[this]{execute("command.cancel",{{"session",session_["id"]}});});
    canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,dock);dock->show();refreshCylindricalControls();
}
void AlloyWindow::refreshCylindricalControls(){
    if(!sliderDock_ || session_["command"]!="joint.cylindrical")return;
    const auto inputs=session_["inputs"].toObject();
    auto* first=sliderDock_->findChild<QComboBox*>("cylindrical.component1");
    auto* second=sliderDock_->findChild<QComboBox*>("cylindrical.component2");
    auto* position=sliderDock_->findChild<QComboBox*>("cylindrical.position");
    QMap<QString,QString> names;for(const auto& value:document_->authored()["components"].toArray())
        names[value.toObject()["id"].toString()]=value.toObject()["name"].toString();
    for(auto* combo:{first,second}){const QSignalBlocker blocked(combo);combo->clear();combo->addItem("Select component…",QString{});
        for(const auto& value:document_->authored()["occurrences"].toArray()){
            const auto occurrence=value.toObject();combo->addItem(names[occurrence["definitionId"].toString()],occurrence["id"].toString());}
        combo->setCurrentIndex(std::max(0,combo->findData(inputs[combo==first?"component1OccurrenceId":"component2OccurrenceId"].toString())));
    }
    const auto fixed=ownerOf(document_->authored(),inputs["component2OccurrenceId"].toString());
    {const QSignalBlocker blocked(position);position->clear();position->addItem("Select Z-axis circle…",QJsonObject{});
        for(const auto& body:document_->bodies())if(body.ownerId==fixed)for(const auto& edge:body.solidEdges)if(!edge.edge.IsNull()){
            const BRepAdaptor_Curve curve(edge.edge);if(curve.GetType()!=GeomAbs_Circle)continue;
            if(std::abs(std::abs(curve.LastParameter()-curve.FirstParameter())-2*std::acos(-1.0))>1e-6
                || std::abs(curve.Circle().Axis().Direction().Z())<1-1e-6)continue;
            const auto center=curve.Circle().Location();
            position->addItem(body.name+QString(" · circle Z %1").arg(center.Z(),0,'f',1),
                QJsonObject{{"bodyId",body.bodyId},{"edgeId",edge.edgeId}});
            position->setItemData(position->count()-1,QString("Center (%1, %2, %3) mm")
                .arg(center.X(),0,'f',1).arg(center.Y(),0,'f',1).arg(center.Z(),0,'f',1),Qt::ToolTipRole);
        }
        position->setCurrentIndex(std::max(0,position->findData(inputs["position"].toObject())));
    }
    for(const auto& key:{"rotationDeg","slideMm"}){auto* field=sliderDock_->findChild<QDoubleSpinBox*>("cylindrical."+QString(key));
        const QSignalBlocker blocked(field);field->setValue(inputs[key].toDouble());}
    sliderDock_->findChild<QLabel*>("cylindrical.error")->setText(session_["previewError"].toString());
    const bool passive=passiveCylinder(presentationAssembly(),inputs["jointId"].toString());
    sliderDock_->findChild<QDialogButtonBox*>("cylindrical.buttons")->button(QDialogButtonBox::Ok)->setEnabled(session_["previewed"].toBool() && !passive);
    const auto ref=inputs["position"].toObject();if(!ref.isEmpty())
        view_->highlightSolidEdges(presentationBodies(),ref["bodyId"].toString(),QJsonArray{ref["edgeId"]});
}
