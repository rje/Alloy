// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AssemblySlider.h"
#include "AssemblyRevolute.h"
#include "AssemblyGraph.h"
#include "LocalOccurrences.h"
#include "JointOrigin.h"
#include "ReferenceView.h"
#include <BRepAdaptor_Curve.hxx>
#include <GeomAbs_CurveType.hxx>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
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
QJsonObject graphOverride(const QJsonObject& authored,const QJsonObject& graphStage,
    std::optional<double> revoluteAngle){
    if(!graphStage.isEmpty()){
        auto override=graphStage;override.remove("documentRevision");return override;
    }
    if(revoluteAngle)for(const auto& value:authored.value("joints").toArray()){
        const auto joint=value.toObject();if(joint["type"]=="asBuiltRevolute")
            return {{"jointId",joint["id"]},{"angleDeg",*revoluteAngle}};
    }
    return {};
}
QString jointType(const QJsonObject& authored,const QString& jointId){
    for(const auto& value:authored.value("joints").toArray()){
        const auto joint=value.toObject();if(joint["id"]==jointId)return joint["type"].toString();
    }
    return {};
}
bool passiveGraphJoint(const QJsonObject& assembly,const QString& jointId){
    if(jointId.isEmpty())return false;
    for(const auto& value:assembly.value("joints").toArray()){
        const auto joint=value.toObject();
        if(joint["id"]==jointId)return !joint.value("coordinateEditable").toBool(true);
    }
    return false;
}
QString originLabel(const QString& kind,const QJsonArray& center,bool yAxis){
    if(center.size()!=3)return kind;
    return QString("%1 X%2 %3%4 · %5%6").arg(kind).arg(center[0].toDouble(),0,'f',1)
        .arg(yAxis ? "Z" : "Y").arg(center[yAxis ? 2 : 1].toDouble(),0,'f',1)
        .arg(yAxis ? "Y" : "Z").arg(center[yAxis ? 1 : 2].toDouble(),0,'f',1);
}
QString originTooltip(const QString& body,const QString& kind,const QJsonArray& center){
    if(center.size()!=3)return body+" · "+kind;
    return QString("%1 · %2 center (%3, %4, %5) mm").arg(body,kind)
        .arg(center[0].toDouble(),0,'f',2).arg(center[1].toDouble(),0,'f',2)
        .arg(center[2].toDouble(),0,'f',2);
}
QJsonObject positionData(const QVariant& value){
    return value.metaType().id()==QMetaType::QJsonValue ? value.toJsonValue().toObject() : value.toJsonObject();
}
}
std::shared_ptr<const AssemblyGraph::Evaluation> AlloyWindow::graphEvaluation(
    const QJsonObject& authored,const EvaluatedDocument& definitions,const QJsonObject& coordinateOverride) const {
    // A pending reference move has different definition inputs for bodies and
    // inspection. Keep those reads independent until that workflow is unified.
    // Only the document-owned canonical evaluation has a guaranteed lifetime
    // through this scope. Prepared noncanonical evaluations may be released and
    // their addresses reused before another nested command reads the cache.
    const bool scoped=graphEvaluationScopeDepth_>0 && !pendingReferenceEvaluation_
        && &definitions==&document_->evaluation();
    if(scoped && graphEvaluationCache_ && graphEvaluationCache_->tabId==activeTabId_
        && graphEvaluationCache_->documentRevision==document_->revision()
        && graphEvaluationCache_->definitions==&definitions
        && graphEvaluationCache_->authored==authored
        && graphEvaluationCache_->coordinateOverride==coordinateOverride)
        return graphEvaluationCache_->result;
    auto result=std::make_shared<const AssemblyGraph::Evaluation>(AssemblyGraph::evaluate(authored,definitions,coordinateOverride));
    if(scoped)graphEvaluationCache_=GraphEvaluationCache{activeTabId_,document_->revision(),&definitions,
        authored,coordinateOverride,result};
    return result;
}
std::vector<EvaluatedBody> AlloyWindow::presentationBodies() const {
    const auto& authored=sliderPreviewAuthored_.isEmpty() ? document_->authored() : sliderPreviewAuthored_;
    const auto& definitions=pendingReferenceEvaluation_ ? *pendingReferenceEvaluation_ : document_->evaluation();
    const auto joints=authored.value("joints").toArray();
    if(AssemblyGraph::isJointGraph(authored))return graphEvaluation(authored,definitions,
        graphOverride(authored,jointGraphStage_,transientSliderPosition_))->worldBodies;
    if(joints.isEmpty())return LocalOccurrences::worldBodies(authored,definitions);
    return AssemblySlider::worldBodies(authored,definitions,transientSliderPosition_);
}
QJsonObject AlloyWindow::presentationAssembly() const {
    const auto& authored=sliderPreviewAuthored_.isEmpty() ? document_->authored() : sliderPreviewAuthored_;
    const auto joints=authored.value("joints").toArray();
    if(AssemblyGraph::isJointGraph(authored))return graphEvaluation(authored,document_->evaluation(),
        graphOverride(authored,jointGraphStage_,transientSliderPosition_))->inspection;
    if(joints.isEmpty())return LocalOccurrences::inspect(authored,document_->evaluation());
    return AssemblySlider::inspect(authored,document_->evaluation(),transientSliderPosition_);
}
bool AlloyWindow::nonzeroAssemblyPose() const {
    const auto& authored=sliderPreviewAuthored_.isEmpty() ? document_->authored() : sliderPreviewAuthored_;
    const auto joints=authored.value("joints").toArray();
    if(joints.isEmpty())return false;
    if(AssemblyGraph::isJointGraph(authored)){
        if(!jointGraphStage_.isEmpty() || transientSliderPosition_.has_value())return true;
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
    commands_.push_back({"joint.asBuiltDialog","As-Built Joint",
        "Choose Revolute or Cylindrical, two exact component occurrences and an origin in one command.",
        {{"type",QJsonObject{{"type","string"},{"required",false},{"enum",QJsonArray{"revolute","cylindrical"}}}},
            {"jointId",field("string",false)},{"component1OccurrenceId",field("string",false)},
            {"component2OccurrenceId",field("string",false)}},true,[this](const QJsonObject& args){
            auto inputs=args;const auto explicitType=inputs.take("type").toString();
            QString type=explicitType.isEmpty() ? "revolute" : explicitType;
            if(!inputs["jointId"].toString().isEmpty()){
                const auto existing=jointType(document_->authored(),inputs["jointId"].toString());
                if(existing!="asBuiltRevolute" && existing!="asBuiltCylindrical")
                    return fail("stale_joint","Choose a current Revolute or Cylindrical joint");
                const auto retained=existing=="asBuiltRevolute" ? "revolute" : "cylindrical";
                if(!explicitType.isEmpty() && explicitType!=retained)
                    return fail("unsupported_edit","An existing joint cannot change Type");
                type=retained;
            }
            return type=="cylindrical" ? beginCylindrical(inputs) : beginRevolute(inputs);
        }});
    commands_.push_back({"joint.typeSet","Choose As-Built Joint Type",
        "Switch Revolute or Cylindrical while retaining both selected occurrence IDs.",
        {{"type",QJsonObject{{"type","string"},{"enum",QJsonArray{"revolute","cylindrical"}}}}},true,
        [this](const QJsonObject& args){return switchAsBuiltType(args["type"].toString());}});
    commands_.push_back({"joint.componentRole","Choose As-Built component slot",
        "Pick the next exact component occurrence in the Browser or viewport.",
        {{"slot",QJsonObject{{"type","string"},{"enum",QJsonArray{"component1","component2"}}}}},true,
        [this](const QJsonObject& args){
            if(session_.value("command")!="joint.revolute" && session_.value("command")!="joint.cylindrical")
                return fail("invalid_session","Start an As-Built Joint before picking components");
            if(!session_["inputs"].toObject()["jointId"].toString().isEmpty())
                return fail("unsupported_edit","Existing joint participants are retained");
            session_["selectionRole"]=args["slot"];
            view_->setJointOriginPicking(false);
            if(session_["command"]=="joint.revolute")refreshRevoluteControls();else refreshCylindricalControls();
            return QJsonObject{{"selectionRole",session_["selectionRole"]},{"sessionId",session_["id"]}};
        }});
    commands_.push_back({"joint.componentPick","Pick As-Built component occurrence",
        "Fill the active component slot with one visible leaf occurrence.",
        {{"occurrenceId",field("string")},{"slot",field("string",false)}},true,
        [this](const QJsonObject& args){
            const auto command=session_.value("command").toString();
            if(command!="joint.revolute" && command!="joint.cylindrical")
                return fail("invalid_session","Start an As-Built Joint before picking components");
            if(!session_["inputs"].toObject()["jointId"].toString().isEmpty())
                return fail("unsupported_edit","Existing joint participants are retained");
            const auto slot=args.value("slot").toString(session_.value("selectionRole").toString());
            if(slot!="component1" && slot!="component2")return fail("invalid_selection_role","Choose Component 1 or Component 2 first");
            const auto id=args["occurrenceId"].toString();bool visibleLeaf=false;
            for(const auto& value:LocalOccurrences::inspect(document_->authored(),document_->evaluation())["occurrences"].toArray()){
                const auto row=value.toObject();if(row["occurrenceId"]==id && !row["bodies"].toArray().isEmpty())visibleLeaf=true;
            }
            if(!visibleLeaf)return fail("invalid_component","Pick a visible leaf component occurrence");
            auto inputs=session_["inputs"].toObject();
            const auto other=slot=="component1" ? "component2OccurrenceId" : "component1OccurrenceId";
            if(inputs[other]==id)return fail("same_component","Choose two distinct component occurrences");
            inputs[slot+"OccurrenceId"]=id;inputs.remove("position");session_["inputs"]=inputs;
            session_["selectionRole"]=inputs["component1OccurrenceId"].toString().isEmpty() ? "component1"
                : inputs["component2OccurrenceId"].toString().isEmpty() ? "component2" : "position";
            if(command=="joint.revolute")previewRevolute();else previewCylindrical();
            return QJsonObject{{"occurrenceId",id},{"slot",slot},{"selectionRole",session_["selectionRole"]},
                {"previewed",session_["previewed"]}};
        }});
    commands_.push_back({"joint.originInspect","Inspect joint-origin inference","Read the transient hovered or held exact origin candidate and glyph.",{},true,
        [this](const QJsonObject&){auto state=view_->jointOriginState();state["originsVisible"]=jointOriginsShown_;return state;}});
    commands_.push_back({"joint.originPick","Choose inferred joint origin","Use the exact retained circle or finite-cylinder reference shown by the joint-origin glyph.",
        {{"position",field("object")}},true,[this](const QJsonObject& args){
            if(session_.value("command")!="joint.revolute" && session_.value("command")!="joint.cylindrical")
                return fail("invalid_session","Start an As-Built Revolute or Cylindrical joint first");
            const auto position=args["position"].toObject();
            const auto fixed=session_["inputs"].toObject()["component2OccurrenceId"].toString();
            if(fixed.isEmpty() || position["occurrenceId"]!=fixed)
                return fail("wrong_component","Choose an origin on Component 2");
            auto retained=position;retained.remove("occurrenceId");
            const auto referenceError=JointOrigin::validateReference(retained);
            if(!referenceError.isEmpty())return fail("invalid_origin",referenceError);
            const auto response=execute("command.update",{{"session",session_["id"]},
                {"inputs",QJsonObject{{"position",position}}}});
            return response["ok"].toBool() ? response["result"].toObject() : response;
        }});
    commands_.push_back({"joint.originsVisibility","Show/Hide Joint Origins","Show retained joint origin and axis glyphs in this viewport.",
        {{"visible",field("boolean")}},true,[this](const QJsonObject& args){
            jointOriginsShown_=args["visible"].toBool();
            view_->setJointOrigins(presentationAssembly()["joints"].toArray(),jointOriginsShown_);
            return QJsonObject{{"visible",jointOriginsShown_},{"authoredChanged",false}};
        }});
    commands_.push_back({"joint.revolute","As-Built Revolute","Retain a circular-origin Revolute between component leaves.",
        revoluteSchema(),true,[this](const QJsonObject& args){
            if(!session_.isEmpty() && session_["command"]!="joint.revolute")return fail("session_busy","Finish or cancel the active command");
            try {const auto candidate=AssemblyRevolute::candidate(document_->authored(),document_->evaluation(),args);
                const auto error=document_->commit(candidate,!args["jointId"].toString().isEmpty() ? "Edit Revolute" : "As-Built Revolute");
                if(!error.isEmpty())return fail("evaluation_failed",error);
                sliderPreviewAuthored_={};transientSliderPosition_.reset();jointGraphStage_={};displayedDocumentRevision_=std::numeric_limits<quint64>::max();
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
                transientSliderPosition_.reset();jointGraphStage_={};return presentationAssembly();
            }catch(const std::exception& e){return fail("invalid_revolute",QString::fromUtf8(e.what()));}
        }});
    commands_.push_back({"joint.previewAngle","Preview Revolute angle","Show a temporary exact Revolute pose without authored history.",
        {{"jointId",field("string",false)},{"angleDeg",QJsonObject{{"type","number"},{"minimum",-180},{"maximum",180}}}},true,[this](const QJsonObject& args){
            if(!session_.isEmpty() && session_["command"]!="joint.revolute")return fail("session_busy","Finish or cancel the active command");
            const auto& authored=sliderPreviewAuthored_.isEmpty()?document_->authored():sliderPreviewAuthored_;
            try {const double angle=args["angleDeg"].toDouble();
                QString target=args["jointId"].toString();if(target.isEmpty())for(const auto& value:authored.value("joints").toArray())
                    if(value.toObject()["type"]=="asBuiltRevolute"){target=value.toObject()["id"].toString();break;}
                if(jointType(authored,target)!="asBuiltRevolute")return fail("invalid_revolute","Preview requires a retained Revolute joint ID");
                const QJsonObject override{{"jointId",target},{"angleDeg",angle}};
                const auto resolved=graphEvaluation(authored,document_->evaluation(),override)->inspection;
                transientSliderPosition_.reset();jointGraphStage_=override;
                jointGraphStage_["documentRevision"]=static_cast<qint64>(document_->revision());
                displayedDocumentRevision_=std::numeric_limits<quint64>::max();return resolved;
            }catch(const std::exception& e){return fail("invalid_revolute",QString::fromUtf8(e.what()));}
        }});
    commands_.push_back({"joint.revoluteStop","Stop Revolute preview","Restore the retained angle without an authored edit.",{},true,
        [this](const QJsonObject&){transientSliderPosition_.reset();jointGraphStage_={};displayedDocumentRevision_=std::numeric_limits<quint64>::max();return presentationAssembly();}});
    commands_.push_back({"position.revert","Revert Position","Restore every displayed occurrence to the retained solved position without a history edit.",{},true,
        [this](const QJsonObject&){
            if(!session_.isEmpty())return fail("session_busy","Complete or cancel the active command before Revert Position");
            if(sliderTimer_)sliderTimer_->stop();transientSliderPosition_.reset();jointGraphStage_={};
            displayedDocumentRevision_=std::numeric_limits<quint64>::max();return presentationAssembly();
        }});
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
    const auto edit=overrides["jointId"].toString();
    if(!edit.isEmpty()){
        bool found=false;
        for(const auto& value:document_->authored().value("joints").toArray()){
            const auto record=value.toObject();if(record["id"]!=edit)continue;
            if(record["type"]!="asBuiltRevolute")return fail("stale_joint","Selected joint is not Revolute");
            inputs={{"jointId",edit},{"name",record["name"]},{"component1OccurrenceId",record["component1OccurrenceId"]},
                {"component2OccurrenceId",record["component2OccurrenceId"]},{"position",record["position"]},
                {"angleDeg",record["angleDeg"]}};
            found=true;break;
        }
        if(!found)return fail("stale_joint","Revolute joint is unavailable");
    }
    for(auto it=overrides.begin();it!=overrides.end();++it)
        if(!it.value().isNull() && !it.value().isUndefined())inputs[it.key()]=it.value();
    session_={{"id",CadDocument::newId()},{"command","joint.revolute"},{"status","active"},
        {"inputs",inputs},{"baseDocumentRevision",static_cast<qint64>(document_->revision())},
        {"previewed",false},{"inputSchema",revoluteSchema()},
        {"selectionRole",inputs["component1OccurrenceId"].toString().isEmpty() ? "component1"
            : inputs["component2OccurrenceId"].toString().isEmpty() ? "component2" : "position"}};
    showRevolute();previewRevolute();return session_;
}
QJsonObject AlloyWindow::switchAsBuiltType(const QString& type){
    const auto current=session_.value("command").toString();
    if(current!="joint.revolute" && current!="joint.cylindrical")
        return fail("invalid_session","Start an As-Built Joint before choosing Type");
    if(!session_["inputs"].toObject()["jointId"].toString().isEmpty())
        return fail("unsupported_edit","Changing an existing joint Type is unavailable");
    if(type!="revolute" && type!="cylindrical")return fail("unsupported_type","Choose Revolute or Cylindrical");
    const auto next=type=="revolute" ? "joint.revolute" : "joint.cylindrical";
    if(next==current)return session_;
    auto inputs=session_["inputs"].toObject();
    if(inputs["name"]==(current=="joint.revolute" ? "Revolute" : "Cylindrical"))inputs["name"]=type=="revolute" ? "Revolute" : "Cylindrical";
    inputs.remove("position");
    inputs.remove("angleDeg");inputs.remove("rotationDeg");inputs.remove("slideMm");
    if(type=="revolute")inputs["angleDeg"]=0;
    else {inputs["rotationDeg"]=0;inputs["slideMm"]=0;}
    session_["command"]=next;session_["inputs"]=inputs;
    session_["inputSchema"]=type=="revolute" ? revoluteSchema() : cylindricalSchema();
    session_["previewed"]=false;session_.remove("previewError");session_.remove("documentDraft");
    sliderPreviewAuthored_={};view_->clearSolidEdgeHighlights();
    displayedDocumentRevision_=std::numeric_limits<quint64>::max();
    if(type=="revolute"){showRevolute();previewRevolute();}
    else {showCylindrical();previewCylindrical();}
    return session_;
}
QJsonObject AlloyWindow::previewRevolute(){
    if(session_["command"]!="joint.revolute")return fail("invalid_session","Revolute dialog is not active");
    if(session_["baseDocumentRevision"].toInteger()!=static_cast<qint64>(document_->revision()))
        return fail("stale_session","Document changed; cancel and restart Revolute");
    try {const auto candidate=AssemblyRevolute::candidate(document_->authored(),document_->evaluation(),session_["inputs"].toObject());
        const auto prepared=document_->prepareCandidate(candidate);
        if(!prepared.ok)throw std::invalid_argument(prepared.error.toStdString());
        const auto assembly=graphEvaluation(prepared.authored,*prepared.evaluation,{})->inspection;
        sliderPreviewAuthored_=prepared.authored;session_["documentDraft"]=prepared.authored;
        session_["assembly"]=assembly;session_["previewed"]=true;session_.remove("previewError");
        displayedDocumentRevision_=std::numeric_limits<quint64>::max();refreshRevoluteControls();return session_;
    }catch(const std::exception& e){sliderPreviewAuthored_={};session_.remove("documentDraft");session_.remove("assembly");
        displayedDocumentRevision_=std::numeric_limits<quint64>::max();session_["previewed"]=false;
        session_["previewError"]=QString::fromUtf8(e.what());refreshRevoluteControls();return fail("invalid_revolute",QString::fromUtf8(e.what()));}
}
void AlloyWindow::showRevolute(){
    if(sliderDock_){sliderDock_->hide();sliderDock_->deleteLater();sliderDock_=nullptr;}
    auto* dock=new QDockWidget("As-Built Joint",canvasWindow_);sliderDock_=dock;dock->setObjectName("revoluteDock");
    auto* escape=new QShortcut(QKeySequence(Qt::Key_Escape),dock);escape->setContext(Qt::WidgetWithChildrenShortcut);
    connect(escape,&QShortcut::activated,this,[this]{if(session_["command"]=="joint.revolute")
        execute("command.cancel",{{"session",session_["id"]}});});
    auto* panel=new QWidget;panel->setObjectName("revolutePanel");panel->setFixedWidth(306);dock->setWidget(panel);
    auto* layout=new QVBoxLayout(panel);layout->setContentsMargins(14,14,14,12);layout->setSpacing(10);
    auto* title=new QLabel("As-Built Joint");title->setWordWrap(true);layout->addWidget(title);
    auto* form=new QFormLayout;form->setVerticalSpacing(10);
    auto* first=new QComboBox;first->setObjectName("revolute.component1");
    auto* second=new QComboBox;second->setObjectName("revolute.component2");
    const auto addComponent=[form,panel](const QString& title,QComboBox* combo,const QString& slot){
        auto* row=new QWidget(panel);auto* horizontal=new QHBoxLayout(row);
        horizontal->setContentsMargins(0,0,0,0);horizontal->setSpacing(4);
        horizontal->addWidget(combo,1);auto* pick=new QPushButton("Pick",row);
        pick->setObjectName("asBuilt.pick"+slot);pick->setToolTip("Pick this component in the Browser or viewport");
        horizontal->addWidget(pick);form->addRow(title,row);return pick;
    };
    auto* pickFirst=addComponent("Component 1",first,"Component1");
    auto* pickSecond=addComponent("Component 2",second,"Component2");
    auto* type=new QComboBox;type->setObjectName("asBuilt.type");
    type->addItem("Revolute","revolute");type->addItem("Cylindrical","cylindrical");form->addRow("Type",type);
    auto* position=new QComboBox;position->setObjectName("revolute.position");form->addRow("Position",position);
    auto* angle=new QDoubleSpinBox;angle->setObjectName("revolute.angleDeg");angle->setDecimals(3);
    angle->setRange(-180,180);angle->setSuffix(" °");form->addRow("Angle",angle);layout->addLayout(form);
    const bool editing=!session_["inputs"].toObject()["jointId"].toString().isEmpty();
    first->setEnabled(!editing);second->setEnabled(!editing);position->setEnabled(!editing);
    pickFirst->setEnabled(!editing);pickSecond->setEnabled(!editing);type->setEnabled(!editing);
    const bool passive=editing && passiveGraphJoint(presentationAssembly(),session_["inputs"].toObject()["jointId"].toString());
    angle->setEnabled(!passive);
    auto* hint=new QLabel(passive ? "This joint follows the assembly driver. Its measured angle is read-only."
        : editing ? "Edit the prescribed angle. To change components or Position, create a new as-built joint."
        : "Hover a Y-axis bore or complete circular edge on Component 2, hold Ctrl/Command, then click its center marker. Angle 0 retains the assembled pose.");
    hint->setObjectName("asBuilt.hint");hint->setWordWrap(true);layout->addWidget(hint);
    auto* error=new QLabel;error->setObjectName("revolute.error");error->setWordWrap(true);layout->addWidget(error);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);buttons->setObjectName("revolute.buttons");layout->addWidget(buttons);layout->addStretch(1);
    const auto update=[this](const QJsonObject& changes){execute("command.update",{{"session",session_["id"]},{"inputs",changes}});};
    connect(first,qOverload<int>(&QComboBox::currentIndexChanged),this,[this,first](int index){if(index>0)execute("joint.componentPick",{{"slot","component1"},{"occurrenceId",first->itemData(index).toString()}});});
    connect(second,qOverload<int>(&QComboBox::currentIndexChanged),this,[this,second](int index){if(index>0)execute("joint.componentPick",{{"slot","component2"},{"occurrenceId",second->itemData(index).toString()}});});
    connect(pickFirst,&QPushButton::clicked,this,[this]{execute("joint.componentRole",{{"slot","component1"}});view_->setFocus(Qt::OtherFocusReason);});
    connect(pickSecond,&QPushButton::clicked,this,[this]{execute("joint.componentRole",{{"slot","component2"}});view_->setFocus(Qt::OtherFocusReason);});
    connect(type,qOverload<int>(&QComboBox::activated),this,[this,type](int index){
        if(index>=0)QTimer::singleShot(0,this,[this,choice=type->itemData(index).toString()]{execute("joint.typeSet",{{"type",choice}});});});
    connect(position,qOverload<int>(&QComboBox::currentIndexChanged),this,[position,update](int index){
        if(index>=0)update({{"position",positionData(position->itemData(index))}});});
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
    {auto* type=sliderDock_->findChild<QComboBox*>("asBuilt.type");const QSignalBlocker blocked(type);
        type->setCurrentIndex(type->findData("revolute"));}
    QMap<QString,QString> names;for(const auto& value:LocalOccurrences::inspect(document_->authored(),document_->evaluation())["occurrences"].toArray())
        if(!value.toObject()["bodies"].toArray().isEmpty())
            names[value.toObject()["occurrenceId"].toString()]=value.toObject()["path"].toString();
    for(auto* combo:{first,second}){const QSignalBlocker blocked(combo);combo->clear();combo->addItem("Select component…",QString{});
        for(const auto& value:document_->authored()["occurrences"].toArray()){
            const auto occurrence=value.toObject();
            const auto id=occurrence["id"].toString();
            if(!names.contains(id))continue;
            const auto path=names[id];combo->addItem(path.section(" / ",-1),id);
            combo->setItemData(combo->count()-1,path,Qt::ToolTipRole);
        }
        combo->setCurrentIndex(std::max(0,combo->findData(inputs[combo==first?"component1OccurrenceId":"component2OccurrenceId"].toString())));
    }
    const auto fixed=inputs["component2OccurrenceId"].toString();
    if(inputs["jointId"].toString().isEmpty()){
        const auto role=session_["selectionRole"].toString();
        sliderDock_->findChild<QLabel*>("asBuilt.hint")->setText(role=="component1" || role=="component2"
            ? QString("Pick %1 in the Browser or viewport, or choose it above.").arg(role=="component1" ? "Component 1" : "Component 2")
            : "Hover a Y-axis bore or circle on Component 2, hold Ctrl/Command, then click its center marker.");
    }
    view_->setJointOriginPicking(inputs["jointId"].toString().isEmpty() && session_["selectionRole"]=="position",fixed,"Y");
    {const QSignalBlocker blocked(position);position->clear();position->addItem("Select bore or circular edge…",QJsonObject{});
        for(const auto& body:presentationBodies())if(body.occurrenceId==fixed)for(const auto& edge:body.solidEdges)if(!edge.edge.IsNull()){
            const BRepAdaptor_Curve curve(edge.edge);if(curve.GetType()!=GeomAbs_Circle)continue;
            if(std::abs(std::abs(curve.LastParameter()-curve.FirstParameter())-2*std::acos(-1.0))>1e-6)continue;
            const auto point=curve.Circle().Location();
            const QJsonArray center{point.X(),point.Y(),point.Z()};
            position->addItem(originLabel("Circle",center,true),QJsonObject{{"occurrenceId",fixed},{"bodyId",body.bodyId},{"edgeId",edge.edgeId}});
            position->setItemData(position->count()-1,originTooltip(body.name,"circular edge",center),Qt::ToolTipRole);
        }
        for(const auto& body:presentationBodies())if(body.occurrenceId==fixed)
            for(const auto& candidate:JointOrigin::cylindricalCandidates(body)){
                const auto axis=candidate["axis"].toArray(),center=candidate["centerMm"].toArray();
                if(axis.size()!=3 || std::abs(axis[1].toDouble())<1-1e-6)continue;
                position->addItem(originLabel("Bore",center,true),candidate["position"]);
                position->setItemData(position->count()-1,originTooltip(body.name,"bore-axis midpoint",center),Qt::ToolTipRole);
            }
        const auto current=inputs["position"].toObject();int chosen=position->findData(current);
        if(chosen<0)for(int i=1;i<position->count();++i){const auto ref=positionData(position->itemData(i));
            if(ref["bodyId"]==current["bodyId"] && ref["edgeId"]==current["edgeId"]
                && ref["rimEdgeIds"]==current["rimEdgeIds"]){chosen=i;break;}}
        position->setCurrentIndex(std::max(0,chosen));
    }
    {auto* angle=sliderDock_->findChild<QDoubleSpinBox*>("revolute.angleDeg");const QSignalBlocker blocked(angle);angle->setValue(inputs["angleDeg"].toDouble());}
    sliderDock_->findChild<QLabel*>("revolute.error")->setText(inputs["position"].toObject().isEmpty()
        || inputs["component1OccurrenceId"].toString().isEmpty() || inputs["component2OccurrenceId"].toString().isEmpty()
        ? QString{} : session_["previewError"].toString());
    const bool passive=passiveGraphJoint(presentationAssembly(),inputs["jointId"].toString());
    sliderDock_->findChild<QDialogButtonBox*>("revolute.buttons")->button(QDialogButtonBox::Ok)->setEnabled(session_["previewed"].toBool() && !passive);
    const auto ref=inputs["position"].toObject();if(!ref.isEmpty())view_->highlightSolidEdges(presentationBodies(),ref["bodyId"].toString(),
        ref.contains("rimEdgeIds") ? ref["rimEdgeIds"].toArray() : QJsonArray{ref["edgeId"]},fixed);
}
void AlloyWindow::registerCylindricalCommands(){
    commands_.push_back({"joint.cylindrical","As-Built Cylindrical",
        "Retain one Z-axis circular-origin Cylindrical relationship between component leaves.",
        cylindricalSchema(),true,[this](const QJsonObject& args){
            if(!session_.isEmpty() && session_["command"]!="joint.cylindrical")return fail("session_busy","Finish or cancel the active command");
            try{const auto candidate=AssemblyGraph::cylindricalCandidate(document_->authored(),document_->evaluation(),args);
                if(const auto error=document_->commit(candidate,!args["jointId"].toString().isEmpty() ? "Edit Cylindrical" : "As-Built Cylindrical");!error.isEmpty())
                    return fail("evaluation_failed",error);
                sliderPreviewAuthored_={};transientSliderPosition_.reset();jointGraphStage_={};displayedDocumentRevision_=std::numeric_limits<quint64>::max();
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
            if(jointType(authored,args["jointId"].toString())!="asBuiltCylindrical")
                return fail("invalid_cylindrical","Preview requires a retained Cylindrical joint ID");
            try{graphEvaluation(authored,document_->evaluation(),args);
                transientSliderPosition_.reset();jointGraphStage_=args;jointGraphStage_["documentRevision"]=static_cast<qint64>(document_->revision());
                displayedDocumentRevision_=std::numeric_limits<quint64>::max();return presentationAssembly();
            }catch(const std::exception& e){return fail("invalid_cylindrical",QString::fromUtf8(e.what()));}
        }});
    commands_.push_back({"joint.cylindricalStop","Stop Cylindrical preview",
        "Restore all persisted occurrence poses without an authored history edit.",{},true,[this](const QJsonObject&){
            jointGraphStage_={};displayedDocumentRevision_=std::numeric_limits<quint64>::max();return presentationAssembly();
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
    for(auto it=overrides.begin();it!=overrides.end();++it)
        if(!it.value().isNull() && !it.value().isUndefined())inputs[it.key()]=it.value();
    session_={{"id",CadDocument::newId()},{"command","joint.cylindrical"},{"status","active"},
        {"inputs",inputs},{"baseDocumentRevision",static_cast<qint64>(document_->revision())},
        {"previewed",false},{"inputSchema",cylindricalSchema()},
        {"selectionRole",inputs["component1OccurrenceId"].toString().isEmpty() ? "component1"
            : inputs["component2OccurrenceId"].toString().isEmpty() ? "component2" : "position"}};
    showCylindrical();previewCylindrical();return session_;
}
QJsonObject AlloyWindow::previewCylindrical(){
    if(session_["command"]!="joint.cylindrical")return fail("invalid_session","Cylindrical dialog is not active");
    if(session_["baseDocumentRevision"].toInteger()!=static_cast<qint64>(document_->revision()))
        return fail("stale_session","Document changed; cancel and restart Cylindrical");
    try{const auto candidate=AssemblyGraph::cylindricalCandidate(document_->authored(),document_->evaluation(),session_["inputs"].toObject());
        const auto prepared=document_->prepareCandidate(candidate);
        if(!prepared.ok)throw std::invalid_argument(prepared.error.toStdString());
        const auto assembly=graphEvaluation(prepared.authored,*prepared.evaluation,{})->inspection;
        sliderPreviewAuthored_=prepared.authored;session_["documentDraft"]=prepared.authored;
        session_["assembly"]=assembly;session_["previewed"]=true;session_.remove("previewError");
        displayedDocumentRevision_=std::numeric_limits<quint64>::max();refreshCylindricalControls();return session_;
    }catch(const std::exception& e){sliderPreviewAuthored_={};session_.remove("documentDraft");session_.remove("assembly");
        displayedDocumentRevision_=std::numeric_limits<quint64>::max();session_["previewed"]=false;
        session_["previewError"]=QString::fromUtf8(e.what());refreshCylindricalControls();
        return fail("invalid_cylindrical",QString::fromUtf8(e.what()));}
}
void AlloyWindow::showCylindrical(){
    if(sliderDock_){sliderDock_->hide();sliderDock_->deleteLater();sliderDock_=nullptr;}
    auto* dock=new QDockWidget("As-Built Joint",canvasWindow_);sliderDock_=dock;dock->setObjectName("cylindricalDock");
    auto* escape=new QShortcut(QKeySequence(Qt::Key_Escape),dock);escape->setContext(Qt::WidgetWithChildrenShortcut);
    connect(escape,&QShortcut::activated,this,[this]{if(session_["command"]=="joint.cylindrical")
        execute("command.cancel",{{"session",session_["id"]}});});
    auto* panel=new QWidget;panel->setObjectName("cylindricalPanel");panel->setFixedWidth(318);dock->setWidget(panel);
    auto* layout=new QVBoxLayout(panel);layout->setContentsMargins(14,14,14,12);layout->setSpacing(10);
    auto* title=new QLabel("As-Built Joint");title->setWordWrap(true);layout->addWidget(title);
    auto* form=new QFormLayout;form->setVerticalSpacing(10);
    auto* first=new QComboBox;first->setObjectName("cylindrical.component1");
    auto* second=new QComboBox;second->setObjectName("cylindrical.component2");
    const auto addComponent=[form,panel](const QString& title,QComboBox* combo,const QString& slot){
        auto* row=new QWidget(panel);auto* horizontal=new QHBoxLayout(row);
        horizontal->setContentsMargins(0,0,0,0);horizontal->setSpacing(4);
        horizontal->addWidget(combo,1);auto* pick=new QPushButton("Pick",row);
        pick->setObjectName("asBuilt.pick"+slot);pick->setToolTip("Pick this component in the Browser or viewport");
        horizontal->addWidget(pick);form->addRow(title,row);return pick;
    };
    auto* pickFirst=addComponent("Component 1",first,"Component1");
    auto* pickSecond=addComponent("Component 2",second,"Component2");
    auto* type=new QComboBox;type->setObjectName("asBuilt.type");
    type->addItem("Revolute","revolute");type->addItem("Cylindrical","cylindrical");form->addRow("Type",type);
    auto* position=new QComboBox;position->setObjectName("cylindrical.position");form->addRow("Position",position);
    auto* rotation=new QDoubleSpinBox;rotation->setObjectName("cylindrical.rotationDeg");rotation->setRange(-180,180);
    rotation->setDecimals(3);rotation->setSuffix(" °");form->addRow("Rotation",rotation);
    auto* slide=new QDoubleSpinBox;slide->setObjectName("cylindrical.slideMm");slide->setRange(-100000,100000);
    slide->setDecimals(3);slide->setSuffix(" mm");form->addRow("Slide",slide);layout->addLayout(form);
    const bool editing=!session_["inputs"].toObject()["jointId"].toString().isEmpty();
    first->setEnabled(!editing);second->setEnabled(!editing);position->setEnabled(!editing);
    pickFirst->setEnabled(!editing);pickSecond->setEnabled(!editing);type->setEnabled(!editing);
    const bool passive=editing && passiveGraphJoint(presentationAssembly(),session_["inputs"].toObject()["jointId"].toString());
    rotation->setEnabled(!passive);slide->setEnabled(!passive);
    auto* hint=new QLabel(passive ? "This joint is driven by another joint in the connected island. Its coordinates are read-only."
        : editing ? "Edit both coordinates. To change components or Position, create a new as-built joint."
        : "Hover a Z-axis tube bore or complete circular edge on Component 2, hold Ctrl/Command, then click its center marker. The assembled pose is retained.");
    hint->setObjectName("asBuilt.hint");
    hint->setWordWrap(true);layout->addWidget(hint);
    auto* error=new QLabel;error->setObjectName("cylindrical.error");error->setWordWrap(true);layout->addWidget(error);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);buttons->setObjectName("cylindrical.buttons");layout->addWidget(buttons);layout->addStretch(1);
    const auto update=[this](const QJsonObject& changes){execute("command.update",{{"session",session_["id"]},{"inputs",changes}});};
    connect(first,qOverload<int>(&QComboBox::currentIndexChanged),this,[this,first](int i){if(i>0)execute("joint.componentPick",{{"slot","component1"},{"occurrenceId",first->itemData(i).toString()}});});
    connect(second,qOverload<int>(&QComboBox::currentIndexChanged),this,[this,second](int i){if(i>0)execute("joint.componentPick",{{"slot","component2"},{"occurrenceId",second->itemData(i).toString()}});});
    connect(pickFirst,&QPushButton::clicked,this,[this]{execute("joint.componentRole",{{"slot","component1"}});view_->setFocus(Qt::OtherFocusReason);});
    connect(pickSecond,&QPushButton::clicked,this,[this]{execute("joint.componentRole",{{"slot","component2"}});view_->setFocus(Qt::OtherFocusReason);});
    connect(type,qOverload<int>(&QComboBox::activated),this,[this,type](int i){
        if(i>=0)QTimer::singleShot(0,this,[this,choice=type->itemData(i).toString()]{execute("joint.typeSet",{{"type",choice}});});});
    connect(position,qOverload<int>(&QComboBox::currentIndexChanged),this,[position,update](int i){
        if(i>=0)update({{"position",positionData(position->itemData(i))}});});
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
    {auto* type=sliderDock_->findChild<QComboBox*>("asBuilt.type");const QSignalBlocker blocked(type);
        type->setCurrentIndex(type->findData("cylindrical"));}
    QMap<QString,QString> names;for(const auto& value:LocalOccurrences::inspect(document_->authored(),document_->evaluation())["occurrences"].toArray())
        if(!value.toObject()["bodies"].toArray().isEmpty())
            names[value.toObject()["occurrenceId"].toString()]=value.toObject()["path"].toString();
    for(auto* combo:{first,second}){const QSignalBlocker blocked(combo);combo->clear();combo->addItem("Select component…",QString{});
        for(const auto& value:document_->authored()["occurrences"].toArray()){
            const auto occurrence=value.toObject();
            const auto id=occurrence["id"].toString();
            if(!names.contains(id))continue;
            const auto path=names[id];combo->addItem(path.section(" / ",-1),id);
            combo->setItemData(combo->count()-1,path,Qt::ToolTipRole);}
        combo->setCurrentIndex(std::max(0,combo->findData(inputs[combo==first?"component1OccurrenceId":"component2OccurrenceId"].toString())));
    }
    const auto fixed=inputs["component2OccurrenceId"].toString();
    if(inputs["jointId"].toString().isEmpty()){
        const auto role=session_["selectionRole"].toString();
        sliderDock_->findChild<QLabel*>("asBuilt.hint")->setText(role=="component1" || role=="component2"
            ? QString("Pick %1 in the Browser or viewport, or choose it above.").arg(role=="component1" ? "Component 1" : "Component 2")
            : "Hover a Z-axis bore or circle on Component 2, hold Ctrl/Command, then click its center marker.");
    }
    view_->setJointOriginPicking(inputs["jointId"].toString().isEmpty() && session_["selectionRole"]=="position",fixed,"Z");
    {const QSignalBlocker blocked(position);position->clear();position->addItem("Select Z-axis bore or circle…",QJsonObject{});
        for(const auto& body:presentationBodies())if(body.occurrenceId==fixed)for(const auto& edge:body.solidEdges)if(!edge.edge.IsNull()){
            const BRepAdaptor_Curve curve(edge.edge);if(curve.GetType()!=GeomAbs_Circle)continue;
            if(std::abs(std::abs(curve.LastParameter()-curve.FirstParameter())-2*std::acos(-1.0))>1e-6
                || std::abs(curve.Circle().Axis().Direction().Z())<1-1e-6)continue;
            const auto center=curve.Circle().Location();
            const QJsonArray point{center.X(),center.Y(),center.Z()};
            position->addItem(originLabel("Circle",point,false),
                QJsonObject{{"occurrenceId",fixed},{"bodyId",body.bodyId},{"edgeId",edge.edgeId}});
            position->setItemData(position->count()-1,originTooltip(body.name,"circular edge",point),Qt::ToolTipRole);
        }
        for(const auto& body:presentationBodies())if(body.occurrenceId==fixed)
            for(const auto& candidate:JointOrigin::cylindricalCandidates(body)){
                const auto axis=candidate["axis"].toArray(),center=candidate["centerMm"].toArray();
                if(axis.size()!=3 || std::abs(axis[2].toDouble())<1-1e-6)continue;
                position->addItem(originLabel("Bore",center,false),candidate["position"]);
                position->setItemData(position->count()-1,originTooltip(body.name,"bore-axis midpoint",center),Qt::ToolTipRole);
            }
        const auto current=inputs["position"].toObject();int chosen=position->findData(current);
        if(chosen<0)for(int i=1;i<position->count();++i){const auto ref=positionData(position->itemData(i));
            if(ref["bodyId"]==current["bodyId"] && ref["edgeId"]==current["edgeId"]
                && ref["rimEdgeIds"]==current["rimEdgeIds"]){chosen=i;break;}}
        position->setCurrentIndex(std::max(0,chosen));
    }
    for(const auto& key:{"rotationDeg","slideMm"}){auto* field=sliderDock_->findChild<QDoubleSpinBox*>("cylindrical."+QString(key));
        const QSignalBlocker blocked(field);field->setValue(inputs[key].toDouble());}
    sliderDock_->findChild<QLabel*>("cylindrical.error")->setText(inputs["position"].toObject().isEmpty()
        || inputs["component1OccurrenceId"].toString().isEmpty() || inputs["component2OccurrenceId"].toString().isEmpty()
        ? QString{} : session_["previewError"].toString());
    const bool passive=passiveGraphJoint(presentationAssembly(),inputs["jointId"].toString());
    sliderDock_->findChild<QDialogButtonBox*>("cylindrical.buttons")->button(QDialogButtonBox::Ok)->setEnabled(session_["previewed"].toBool() && !passive);
    const auto ref=inputs["position"].toObject();if(!ref.isEmpty())
        view_->highlightSolidEdges(presentationBodies(),ref["bodyId"].toString(),
            ref.contains("rimEdgeIds") ? ref["rimEdgeIds"].toArray() : QJsonArray{ref["edgeId"]},fixed);
}
