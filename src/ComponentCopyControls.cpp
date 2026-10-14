// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include "LocalOccurrences.h"
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include <QDockWidget>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QShortcut>
#include <QSignalBlocker>
#include <QToolButton>
#include <QVBoxLayout>
#include <cmath>
#include <limits>
namespace {
QJsonObject failure(const QString& code,const QString& message){return {{"ok",false},{"completion","failed"},{"error",QJsonObject{{"code",code},{"message",message}}}};}
QJsonObject field(const QString& type){QJsonObject result{{"type",type},{"required",true}};if(type=="number"){result["minimum"]=-100000;result["maximum"]=100000;}return result;}
QJsonObject placement(const QJsonObject& inputs){return {{"axis","Z"},{"angleDeg",inputs["angleDeg"]},{"translationMm",QJsonArray{inputs["xMm"],inputs["yMm"],inputs["zMm"]}}};}
}
void AlloyWindow::registerComponentCopyCommands(){
    commands_.push_back({"component.copyHandle","Drag Paste translation","Update one constrained translation coordinate of the pending linked copy.",{{"field",field("string")},{"value",field("number")}},true,[this](const QJsonObject& a){
        if(!QStringList{"xMm","yMm","zMm"}.contains(a["field"].toString()))return failure("invalid_arguments","Paste supports translation arrows");
        return updateComponentCopy({{a["field"].toString(),a["value"]}});
    }});
    commands_.push_back({"component.copyGestureEnd","Finish Paste drag","Retain the live placement preview without committing the copy.",{},true,[this](const QJsonObject&){
        if(session_.value("command")!="component.copyPlacement")return failure("invalid_session","Paste placement is not active");return session_;
    }});
}
QJsonObject AlloyWindow::beginComponentCopy(const QJsonObject& args){
    if(!session_.isEmpty())return failure("active_command","Finish or Cancel before copying a component");
    if(!jointGraphStage_.isEmpty()||transientSliderPosition_.has_value())return failure("staged_pose","Revert the previewed assembly position before copying a component");
    const bool clipboard=args["fromClipboard"].toBool();const auto id=args["occurrenceId"].toString();
    if(clipboard){const auto source=execute("component.clipboardInspect");if(!source["ok"].toBool())return source;if(source["result"].toObject()["occurrenceId"]!=id)return failure("stale_component_clipboard","The copied component changed; Copy it again");}
    QString path;for(const auto& v:LocalOccurrences::inspect(document_->authored(),document_->evaluation())["occurrences"].toArray())if(v.toObject()["occurrenceId"]==id)path=v.toObject()["path"].toString();
    if(path.isEmpty())return failure("invalid_component_copy","Choose a current local component occurrence");
    QJsonObject result;try{result=CadDocument::copyOccurrenceCandidate(document_->authored(),id,{{"axis","Z"},{"angleDeg",0},{"translationMm",QJsonArray{0,0,0}}});}catch(const std::exception& e){return failure("invalid_component_copy",QString::fromUtf8(e.what()));}
    view_->setAssemblyDragCue({});
    session_={{"id",CadDocument::newId()},{"command","component.copyPlacement"},{"status","active"},{"visible",true},{"sourceOccurrenceId",id},{"sourcePath",path},{"fromClipboard",clipboard},{"tabId",activeTabId_},{"baseDocumentRevision",static_cast<qint64>(document_->revision())},{"inputs",QJsonObject{{"xMm",0},{"yMm",0},{"zMm",0},{"angleDeg",0}}},{"copyResult",result},{"previewed",false}};
    if(componentCopyDock_){canvasWindow_->removeDockWidget(componentCopyDock_);componentCopyDock_->deleteLater();}
    componentCopyDock_=new QDockWidget(clipboard?"Paste Component":"Copy Component",canvasWindow_);componentCopyDock_->setObjectName("component.copy.dialog");componentCopyDock_->setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable|QDockWidget::DockWidgetClosable);
    auto* panel=new QWidget(componentCopyDock_);componentCopyDock_->setWidget(panel);auto* layout=new QVBoxLayout(panel);auto* intro=new QLabel("Linked copy of "+path+". Drag a translation arrow or enter a parent-local placement.",panel);intro->setWordWrap(true);layout->addWidget(intro);auto* form=new QFormLayout;layout->addLayout(form);
    const QStringList fields{"xMm","yMm","zMm","angleDeg"},objects{"component.copy.x","component.copy.y","component.copy.z","component.copy.angle"},labels{"X","Y","Z","Rotation Z"};
    for(int n=0;n<fields.size();++n){auto* spin=new QDoubleSpinBox(panel);spin->setObjectName(objects[n]);spin->setRange(-100000,100000);spin->setDecimals(3);spin->setSuffix(n==3?"°":" mm");form->addRow(labels[n],spin);connect(spin,&QDoubleSpinBox::valueChanged,panel,[this,key=fields[n]](double value){execute("command.update",{{"session",session_.value("id")},{"inputs",QJsonObject{{key,value}}}});});}
    auto* status=new QLabel(panel);status->setObjectName("component.copy.status");status->setWordWrap(true);layout->addWidget(status);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,panel);buttons->button(QDialogButtonBox::Ok)->setObjectName("component.copy.ok");buttons->button(QDialogButtonBox::Cancel)->setObjectName("component.copy.cancel");layout->addWidget(buttons);
    connect(buttons,&QDialogButtonBox::accepted,panel,[this]{execute("command.commit",{{"session",session_.value("id")}});});connect(buttons,&QDialogButtonBox::rejected,panel,[this]{execute("command.cancel",{{"session",session_.value("id")}});});
    for(auto* child:panel->findChildren<QWidget*>())child->installEventFilter(this);panel->installEventFilter(this);
    connect(componentCopyDock_,&QDockWidget::visibilityChanged,this,[this](bool visible){if(!visible&&session_.value("command")=="component.copyPlacement")execute("command.cancel",{{"session",session_.value("id")}});});
    if(!componentCopyEscape_){componentCopyEscape_=new QShortcut(QKeySequence(Qt::Key_Escape),view_);componentCopyEscape_->setContext(Qt::WidgetShortcut);connect(componentCopyEscape_,&QShortcut::activated,this,[this]{if(session_.value("command")=="component.copyPlacement")execute("command.cancel",{{"session",session_.value("id")}});});}componentCopyEscape_->setEnabled(true);
    componentCopyDock_->setMinimumWidth(300);canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,componentCopyDock_);componentCopyDock_->show();previewComponentCopy();view_->setFocus(Qt::OtherFocusReason);return session_;
}
QJsonObject AlloyWindow::previewComponentCopy(){
    if(session_.value("command")!="component.copyPlacement")return failure("invalid_session","Paste placement is not active");
    auto reject=[this](const QString& code,const QString& message){view_->clearMoveCue(true);view_->clearPreview();view_->clearBodyStage();session_["previewed"]=false;session_["previewError"]=QJsonObject{{"code",code},{"message",message}};if(componentCopyDock_)if(auto* label=componentCopyDock_->findChild<QLabel*>("component.copy.status"))label->setText(message);return failure(code,message);};
    if(session_["tabId"]!=activeTabId_||session_["baseDocumentRevision"].toInteger()!=static_cast<qint64>(document_->revision()))return reject("stale_session","The design changed; Cancel and restart Paste");
    if(session_["fromClipboard"].toBool()&&(componentClipboard_["tabId"]!=activeTabId_||componentClipboard_["documentId"]!=document_->authored()["documentId"]||componentClipboard_["occurrenceId"]!=session_["sourceOccurrenceId"]))return reject("stale_component_clipboard","The copied source changed; Copy it again");
    auto result=session_["copyResult"].toObject();auto candidate=result["authored"].toObject();auto rows=candidate["occurrences"].toArray();for(int n=0;n<rows.size();++n){auto r=rows[n].toObject();if(r["id"]==result["rootOccurrenceId"]){r["placement"]=placement(session_["inputs"].toObject());rows[n]=r;}}candidate["occurrences"]=rows;const auto error=LocalOccurrences::validate(candidate);if(!error.isEmpty())return reject("invalid_component_copy",error);
    const auto bodies=LocalOccurrences::worldBodies(candidate,document_->evaluation());BRep_Builder builder;TopoDS_Compound copy;builder.MakeCompound(copy);double lo[3]={1e20,1e20,1e20},hi[3]={-1e20,-1e20,-1e20};int count=0;
    const auto map=result["occurrenceMap"].toObject();for(const auto& b:bodies){bool copied=false;for(const auto& v:map)copied|=v==b.occurrenceId;if(!copied||!b.visible)continue;builder.Add(copy,b.shape);++count;const auto bounds=b.summary["boundsMm"].toObject();for(int n=0;n<3;++n){lo[n]=std::min(lo[n],bounds["min"].toArray()[n].toDouble());hi[n]=std::max(hi[n],bounds["max"].toArray()[n].toDouble());}}
    if(!count)return reject("invalid_component_copy","The copied subtree has no visible geometry");
    view_->clearPreview();view_->setBodyStage(bodies,"copy:"+session_.value("id").toString()+":"+QString::number(++componentCopyPreviewRevision_));view_->showMovePreview(copy,{});
    auto cue=session_["inputs"].toObject();cue.remove("angleDeg");for(int n=0;n<3;++n){const QString key=QStringList{"xMm","yMm","zMm"}[n];cue[QStringList{"pivotXMm","pivotYMm","pivotZMm"}[n]]=(lo[n]+hi[n])/2-cue[key].toDouble();}cue["translationOnly"]=true;cue["handleCommand"]="component.copyHandle";cue["releaseCommand"]="component.copyGestureEnd";
    const auto parent=LocalOccurrences::parentOccurrenceId(candidate,result["rootOccurrenceId"].toString());if(!parent.isEmpty())cue["axisFrame"]=LocalOccurrences::worldPose(candidate,parent);view_->showMoveCue(cue);
    result["authored"]=candidate;session_["copyResult"]=result;session_["documentDraft"]=candidate;session_["previewed"]=true;session_["definitionsReused"]=true;QJsonArray previewOccurrences;for(const auto& v:candidate["occurrences"].toArray()){const auto r=v.toObject();QJsonArray summaries;for(const auto& b:bodies)if(b.occurrenceId==r["id"])summaries.append(QJsonObject{{"bodyId",b.bodyId},{"boundsMm",b.summary["boundsMm"]}});previewOccurrences.append(QJsonObject{{"occurrenceId",r["id"]},{"definitionId",r["definitionId"]},{"worldPose",LocalOccurrences::worldPose(candidate,r["id"].toString())},{"bodies",summaries}});}session_["previewOccurrences"]=previewOccurrences;session_.remove("previewError");
    if(componentCopyDock_){if(auto* label=componentCopyDock_->findChild<QLabel*>("component.copy.status"))label->setText(QString("X %1 mm · Y %2 mm · Z %3 mm").arg(session_["inputs"].toObject()["xMm"].toDouble(),0,'f',2).arg(session_["inputs"].toObject()["yMm"].toDouble(),0,'f',2).arg(session_["inputs"].toObject()["zMm"].toDouble(),0,'f',2));const QStringList fields{"xMm","yMm","zMm","angleDeg"},objects{"component.copy.x","component.copy.y","component.copy.z","component.copy.angle"};for(int n=0;n<fields.size();++n)if(auto* spin=componentCopyDock_->findChild<QDoubleSpinBox*>(objects[n])){const double value=session_["inputs"].toObject()[fields[n]].toDouble();if(spin->value()!=value){const QSignalBlocker block(spin);spin->setValue(value);}}}
    return session_;
}
QJsonObject AlloyWindow::updateComponentCopy(const QJsonObject& changes){if(session_.value("command")!="component.copyPlacement")return failure("invalid_session","Paste placement is not active");for(auto it=changes.begin();it!=changes.end();++it)if(!QStringList{"xMm","yMm","zMm","angleDeg"}.contains(it.key())||!it.value().isDouble()||!std::isfinite(it.value().toDouble())||std::abs(it.value().toDouble())>100000)return failure("invalid_arguments","Paste requires finite translation/rotation values within 100000");auto inputs=session_["inputs"].toObject();for(auto it=changes.begin();it!=changes.end();++it)inputs[it.key()]=it.value();session_["inputs"]=inputs;return previewComponentCopy();}
QJsonObject AlloyWindow::finishComponentCopy(const QString& id,bool commit){if(session_.value("command")!="component.copyPlacement"||session_.value("id")!=id)return failure("invalid_session","Paste placement is no longer active");if(commit){const auto preview=previewComponentCopy();if(preview.contains("ok")&&!preview["ok"].toBool())return preview;const auto error=document_->commit(session_["documentDraft"].toObject(),"Paste Component");if(!error.isEmpty())return failure("invalid_component_copy",error);}auto result=session_["copyResult"].toObject();result.remove("authored");result["sessionId"]=id;result["status"]=commit?"committed":"cancelled";session_={};if(componentCopyEscape_)componentCopyEscape_->setEnabled(false);if(componentCopyDock_)componentCopyDock_->hide();view_->clearMoveCue(true);view_->clearPreview();view_->clearBodyStage();displayedDocumentRevision_=std::numeric_limits<quint64>::max();view_->setFocus(Qt::OtherFocusReason);return result;}
bool AlloyWindow::componentCopyKey(QObject* object,QEvent* event){if(session_.value("command")!="component.copyPlacement"||!componentCopyDock_||event->type()!=QEvent::KeyPress)return false;auto* widget=qobject_cast<QWidget*>(object);if(!widget||!componentCopyDock_->isAncestorOf(widget))return false;const auto* key=static_cast<QKeyEvent*>(event);if(key->key()==Qt::Key_Escape){execute("command.cancel",{{"session",session_.value("id")}});return true;}if(key->key()!=Qt::Key_Return&&key->key()!=Qt::Key_Enter)return false;for(auto* spin:componentCopyDock_->findChildren<QDoubleSpinBox*>())spin->interpretText();execute("command.commit",{{"session",session_.value("id")}});return true;}
void AlloyWindow::refreshPositionTab(){const bool staged=!jointGraphStage_.isEmpty()||transientSliderPosition_.has_value();if(auto* tab=findChild<QToolButton*>("design.tab.position"))tab->setVisible(staged);if(!staged&&designTab_=="position"){designTab_="solid";for(const auto& name:{"solid","surface","position"}){if(auto* panel=findChild<QWidget*>(QString("design.groups.")+name))panel->setVisible(QString(name)==designTab_);if(auto* tab=findChild<QToolButton*>(QString("design.tab.")+name))tab->setChecked(QString(name)==designTab_);}}}
