#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <QDockWidget>
#include <QFormLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QShortcut>
#include <QTreeWidget>
#include <QSignalBlocker>
#include <QTimer>

namespace {
QJsonObject sectionError(const QString& code,const QString& message) {
    return {{"ok",false},{"completion","failed"},{"error",QJsonObject{{"code",code},{"message",message}}}};
}
QJsonObject sectionSchema(){return {{"analysisId",QJsonObject{{"type","string"},{"required",false}}},{"name",QJsonObject{{"type","string"},{"required",false}}},{"plane",QJsonObject{{"type","string"},{"enum",QJsonArray{"XY","XZ","YZ"}},{"required",false}}},{"hatch",QJsonObject{{"type","boolean"},{"required",false}}}};}
}
void AlloyWindow::registerSectionCommands() {
    commands_.push_back({"inspect.section","Section Analysis","Retain one origin-plane section outside modeling history.",sectionSchema(),true,[this](const QJsonObject& args){
        try {
            if(!args.contains("plane"))return sectionError("plane_required","Select an origin plane");
            auto inputs=args;inputs["visible"]=true;if(session_.value("command")=="inspect.section" && !inputs.contains("analysisId"))inputs["newAnalysisId"]=session_["newAnalysisId"];
            const auto candidate=CadDocument::sectionCandidate(document_->authored(),inputs);
            const auto error=document_->commitAnalyses(candidate["analyses"].toArray(),args.contains("analysisId") ? "Edit Section Analysis" : "Create Section Analysis");
            if(!error.isEmpty())return sectionError("invalid_analysis",error);
            return QJsonObject{{"analyses",document_->authored()["analyses"]}};
        }catch(const std::exception& error){return sectionError("invalid_arguments",QString::fromUtf8(error.what()));}
    }});
    commands_.push_back({"inspect.sectionDialog","Section Analysis","Choose an origin plane and preview a retained section.",sectionSchema(),true,[this](const QJsonObject& args){return beginSection(args);}});
    commands_.push_back({"analysis.inspect","Inspect Analyses","Read retained inspection records.",{},true,[this](const QJsonObject&){return QJsonObject{{"analyses",document_->authored()["analyses"].toArray()}};}});
    commands_.push_back({"analysis.visibility","Show / Hide Analysis","Only one section may be visible; showing replaces the active section atomically.",{{"analysisId",QJsonObject{{"type","string"}}},{"visible",QJsonObject{{"type","boolean"}}}},true,[this](const QJsonObject& args){
        if(!session_.isEmpty())return sectionError("session_busy","Complete or cancel the active command first");
        try{const auto candidate=CadDocument::sectionCandidate(document_->authored(),args);const auto error=document_->commitAnalyses(candidate["analyses"].toArray(),"Show / Hide Section Analysis");if(!error.isEmpty())return sectionError("invalid_analysis",error);return QJsonObject{{"analyses",document_->authored()["analyses"]}};}
        catch(const std::exception& error){return sectionError("invalid_arguments",QString::fromUtf8(error.what()));}
    }});
}
QJsonObject AlloyWindow::beginSection(const QJsonObject& overrides) {
    if(!session_.isEmpty())return sectionError("session_busy","Complete or cancel the active command first");
    QJsonObject inputs{{"name","Section Analysis"},{"hatch",true}};
    if(overrides.contains("analysisId")) {
        bool found=false;for(const auto& entry:document_->authored()["analyses"].toArray()){const auto row=entry.toObject();if(row["id"]==overrides["analysisId"]){inputs={{"analysisId",row["id"]},{"name",row["name"]},{"plane",row["plane"]},{"hatch",row["hatch"]}};found=true;break;}}
        if(!found)return sectionError("stale_entity","Section analysis is unavailable");
    }
    for(auto it=overrides.begin();it!=overrides.end();++it)inputs[it.key()]=it.value();
    session_={{"id",CadDocument::newId()},{"command","inspect.section"},{"status","active"},{"inputs",inputs},{"newAnalysisId",CadDocument::newId()},{"baseDocumentRevision",static_cast<qint64>(document_->revision())},{"previewed",false},{"inputSchema",sectionSchema()}};
    showSection();previewSection();return session_;
}
QJsonObject AlloyWindow::previewSection() {
    auto fail=[this](const QString& code,const QString& message){session_["previewed"]=false;view_->setSectionAnalysis({});if(sectionDock_){sectionDock_->findChild<QLabel*>("sectionError")->setText(message);sectionDock_->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->setEnabled(false);}return sectionError(code,message);};
    if(session_["baseDocumentRevision"].toInteger()!=static_cast<qint64>(document_->revision()))return fail("stale_session","The document changed; cancel and restart Section Analysis");
    const auto inputs=session_["inputs"].toObject();if(!inputs.contains("plane"))return fail("plane_required","Select an origin plane");
    try{auto args=inputs;args["visible"]=true;args["newAnalysisId"]=session_["newAnalysisId"];const auto candidate=CadDocument::sectionCandidate(document_->authored(),args);session_["documentDraft"]=candidate;session_["previewed"]=true;
        for(const auto& entry:candidate["analyses"].toArray())if(entry.toObject()["visible"].toBool())view_->setSectionAnalysis(entry.toObject());
        if(sectionDock_){sectionDock_->findChild<QLabel*>("sectionError")->clear();sectionDock_->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->setEnabled(true);}return session_;
    }catch(const std::exception& error){return fail("invalid_arguments",QString::fromUtf8(error.what()));}
}
void AlloyWindow::showSection() {
    if(sectionDock_){canvasWindow_->removeDockWidget(sectionDock_);delete sectionDock_;}
    sectionDock_=new QDockWidget("Section Analysis",canvasWindow_);sectionDock_->setObjectName("sectionDock");sectionDock_->setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);
    auto* panel=new QWidget(sectionDock_);sectionDock_->setWidget(panel);panel->setMinimumWidth(240);auto* form=new QFormLayout(panel);
    auto* plane=new QComboBox(panel);plane->setObjectName("sectionPlane");plane->addItems({"Select origin plane…","XY","XZ","YZ"});plane->setCurrentIndex(plane->findText(session_["inputs"].toObject()["plane"].toString()));if(plane->currentIndex()<0)plane->setCurrentIndex(0);form->addRow("Cutting Plane",plane);
    form->addRow("Section Color",new QLabel("From Component",panel));
    auto* hatch=new QCheckBox("Show Hatching",panel);hatch->setObjectName("sectionHatch");hatch->setChecked(session_["inputs"].toObject()["hatch"].toBool());form->addRow(hatch);
    auto* status=new QLabel(panel);status->setObjectName("sectionError");status->setWordWrap(true);form->addRow(status);
    auto update=[this](const QJsonObject& inputs){execute("command.update",{{"session",session_["id"]},{"inputs",inputs}});};
    connect(plane,&QComboBox::currentIndexChanged,panel,[plane,update](int i){if(i>0)update({{"plane",plane->currentText()}});});connect(hatch,&QCheckBox::toggled,panel,[update](bool on){update({{"hatch",on}});});
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,panel);form->addRow(buttons);
    connect(buttons,&QDialogButtonBox::accepted,panel,[this,status]{const auto result=execute("command.commit",{{"session",session_["id"]}});if(!result["ok"].toBool())status->setText(result["error"].toObject()["message"].toString());});
    auto cancel=[this]{if(session_.value("command")=="inspect.section")execute("command.cancel",{{"session",session_["id"]}});};connect(buttons,&QDialogButtonBox::rejected,panel,cancel);auto* escape=new QShortcut(QKeySequence(Qt::Key_Escape),panel);escape->setContext(Qt::WidgetWithChildrenShortcut);connect(escape,&QShortcut::activated,panel,cancel);
    canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,sectionDock_);sectionDock_->show();
    // Native menu dismissal restores its previous focus after the action callback.
    QTimer::singleShot(0,plane,[plane]{plane->setFocus(Qt::OtherFocusReason);});
}
void AlloyWindow::refreshSection() {
    QJsonObject active;
    if(session_.value("command")=="inspect.section") {
        if(session_["previewed"].toBool())for(const auto& entry:session_["documentDraft"].toObject()["analyses"].toArray())if(entry.toObject()["visible"].toBool())active=entry.toObject();
    }else for(const auto& entry:document_->authored()["analyses"].toArray())if(entry.toObject()["visible"].toBool())active=entry.toObject();
    view_->setSectionAnalysis(active);
    if(session_.value("command")=="inspect.section" && sectionDock_) {
        const auto inputs=session_.value("inputs").toObject();
        if(auto* plane=sectionDock_->findChild<QComboBox*>("sectionPlane")){const QSignalBlocker block(plane);plane->setCurrentIndex(qMax(0,plane->findText(inputs["plane"].toString())));}
        if(auto* hatch=sectionDock_->findChild<QCheckBox*>("sectionHatch")){const QSignalBlocker block(hatch);hatch->setChecked(inputs["hatch"].toBool());}
    }
    if(!analysesNode_)return;const QSignalBlocker block(tree_);QSet<QString> live;
    for(const auto& entry:document_->authored()["analyses"].toArray()) {
        const auto row=entry.toObject();const auto id=row["id"].toString();live.insert(id);QTreeWidgetItem* item=nullptr;
        for(int i=0;i<analysesNode_->childCount();++i)if(analysesNode_->child(i)->data(0,Qt::UserRole+2)==id)item=analysesNode_->child(i);
        if(!item)item=new QTreeWidgetItem(analysesNode_);item->setData(0,Qt::UserRole+2,id);item->setText(0,row["name"].toString());item->setFlags(item->flags()|Qt::ItemIsUserCheckable);item->setCheckState(0,row["visible"].toBool() ? Qt::Checked : Qt::Unchecked);item->setHidden(false);
    }
    for(int i=0;i<analysesNode_->childCount();++i)if(!live.contains(analysesNode_->child(i)->data(0,Qt::UserRole+2).toString()))analysesNode_->child(i)->setHidden(true);
}
