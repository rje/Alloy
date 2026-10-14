// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AssemblySlider.h"
#include "ReferenceView.h"
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
QString ownerOf(const QJsonObject& authored,const QString& occurrenceId){
    for(const auto& value:authored.value("occurrences").toArray())if(value.toObject()["id"]==occurrenceId)return value.toObject()["definitionId"].toString();return {};
}
}
std::vector<EvaluatedBody> AlloyWindow::presentationBodies() const {
    return AssemblySlider::worldBodies(sliderPreviewAuthored_.isEmpty() ? document_->authored() : sliderPreviewAuthored_,
        pendingReferenceEvaluation_ ? *pendingReferenceEvaluation_ : document_->evaluation(),transientSliderPosition_);
}
QJsonObject AlloyWindow::presentationAssembly() const {
    return AssemblySlider::inspect(sliderPreviewAuthored_.isEmpty() ? document_->authored() : sliderPreviewAuthored_,document_->evaluation(),transientSliderPosition_);
}
bool AlloyWindow::nonzeroAssemblyPose() const {
    const auto& authored=sliderPreviewAuthored_.isEmpty() ? document_->authored() : sliderPreviewAuthored_;
    return !authored.value("joints").toArray().isEmpty() && std::abs(AssemblySlider::position(authored,transientSliderPosition_))>1e-9;
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
    commands_.push_back({"joint.inspect","Inspect Slider","Read authored joint, one relative DOF, current pose and occurrence world bounds.",{},true,
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
