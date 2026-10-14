#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <QDockWidget>
#include <QFormLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QLabel>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QShortcut>
#include <QTreeWidget>
#include <QSignalBlocker>
#include <QTimer>
#include <QSet>

namespace {
QJsonObject sectionError(const QString& code,const QString& message) {
    return {{"ok",false},{"completion","failed"},{"error",QJsonObject{{"code",code},{"message",message}}}};
}
QJsonObject sectionSchema(){return {{"analysisId",QJsonObject{{"type","string"},{"required",false}}},{"name",QJsonObject{{"type","string"},{"required",false}}},{"plane",QJsonObject{{"type","string"},{"enum",QJsonArray{"XY","XZ","YZ","face"}},{"required",false}}},{"support",QJsonObject{{"type","object"},{"required",false}}},{"offsetMm",QJsonObject{{"type","number"},{"minimum",-100000},{"maximum",100000},{"required",false}}},{"flip",QJsonObject{{"type","boolean"},{"required",false}}},{"hatch",QJsonObject{{"type","boolean"},{"required",false}}}};}
}
QJsonObject AlloyWindow::sectionPresentation(const QJsonObject& analysis) const {
    if(analysis["plane"]!="face") {
        const auto plane=analysis["plane"].toString();
        const QJsonArray source=plane=="XY" ? QJsonArray{0,0,1} : plane=="XZ" ? QJsonArray{0,1,0} : QJsonArray{1,0,0};
        QJsonArray origin,normal;for(int axis=0;axis<3;++axis){origin.append(analysis["offsetMm"].toDouble()*source[axis].toDouble());normal.append((analysis["flip"].toBool() ? -1.0 : 1.0)*source[axis].toDouble());}
        auto result=analysis;result["resolvedPlane"]=QJsonObject{{"origin",origin},{"normal",normal}};result["availability"]="available";return result;
    }
    try {
        // A Split can move an unchanged named face onto its new result body.
        // Follow only authored Split ancestry, never a nearest-plane match.
        const auto support=analysis["support"].toObject();
        QSet<QString> descendants{support["bodyId"].toString()};
        for(const auto& entry:document_->authored()["features"].toArray()) {
            const auto feature=entry.toObject();
            if(feature["type"]=="splitBody" && descendants.contains(feature["parameters"].toObject()["targetBodyId"].toString()))
                descendants.insert(feature["bodyId"].toString());
        }
        QJsonObject surviving;int matches=0;
        for(const auto& body:document_->evaluation().bodies)if(descendants.contains(body.bodyId))
            for(const auto& face:body.supportFaces)
                if(face.faceId==support["faceId"].toString() && face.producerFeatureId==support["producerFeatureId"].toString()) {
                    surviving=support;surviving["bodyId"]=body.bodyId;++matches;
                }
        if(matches!=1)throw std::runtime_error("Section support has no unique Split descendant");
        const auto resolved=GeometryKernel::resolveSketchSupport(document_->evaluation(),surviving);
        for(const auto& body:presentationBodies())if(body.bodyId==resolved.bodyId)
            for(const auto& face:body.supportFaces)if(face.faceId==resolved.faceId && face.producerFeatureId==resolved.producerFeatureId){
                auto result=analysis;result["resolvedFrame"]=face.frame;result["availability"]="available";
                const auto origin=face.frame["origin"].toArray(),normal=face.frame["normal"].toArray();QJsonArray moved,side;
                for(int axis=0;axis<3;++axis){moved.append(origin[axis].toDouble()+analysis["offsetMm"].toDouble()*normal[axis].toDouble());side.append((analysis["flip"].toBool() ? -1.0 : 1.0)*normal[axis].toDouble());}
                result["resolvedPlane"]=QJsonObject{{"origin",moved},{"normal",side}};return result;
            }
    }catch(const std::exception&){}
    auto result=analysis;result["visible"]=false;result["availability"]="unavailable";return result;
}
QJsonArray AlloyWindow::sectionInspection() const {
    QJsonArray rows;for(const auto& entry:document_->authored()["analyses"].toArray()){
        auto row=sectionPresentation(entry.toObject());row.remove("resolvedFrame");row["visible"]=entry.toObject()["visible"];
        if(!row.contains("availability"))row["availability"]="available";rows.append(row);
    }return rows;
}
void AlloyWindow::registerSectionCommands() {
    commands_.push_back({"inspect.section","Section Analysis","Retain one origin-plane section outside modeling history.",sectionSchema(),true,[this](const QJsonObject& args){
        try {
            const bool sketchActive=session_.value("command")=="sketch.edit";
            const bool ownDialog=session_.value("command")=="inspect.section";
            if(!session_.isEmpty() && !ownDialog && !(sketchActive && args.contains("analysisId")))
                return sectionError("session_busy","Complete or cancel the active command first");
            if(sketchActive && session_["baseDocumentRevision"].toInteger()!=static_cast<qint64>(document_->revision()))
                return sectionError("stale_session","The document changed; cancel and restart this sketch");
            if(!args.contains("plane") && !args.contains("analysisId"))return sectionError("plane_required","Select a cutting plane");
            auto inputs=args;inputs["visible"]=true;if(session_.value("command")=="inspect.section" && !inputs.contains("analysisId"))inputs["newAnalysisId"]=session_["newAnalysisId"];
            const auto candidate=CadDocument::sectionCandidate(document_->authored(),inputs);
            for(const auto& entry:candidate["analyses"].toArray())if(entry.toObject()["visible"].toBool() && sectionPresentation(entry.toObject())["availability"]=="unavailable")return sectionError("support_unavailable","Selected planar face is unavailable");
            const auto error=document_->commitAnalyses(candidate["analyses"].toArray(),args.contains("analysisId") ? "Edit Section Analysis" : "Create Section Analysis");
            if(!error.isEmpty())return sectionError("invalid_analysis",error);
            // Analyses change visual state, not evaluated sketch sources. Keep the
            // active authored draft and its optimistic revision in sync.
            if(sketchActive)session_["baseDocumentRevision"]=static_cast<qint64>(document_->revision());
            return QJsonObject{{"analyses",document_->authored()["analyses"]}};
        }catch(const std::exception& error){return sectionError("invalid_arguments",QString::fromUtf8(error.what()));}
    }});
    commands_.push_back({"inspect.sectionDialog","Section Analysis","Choose an origin plane and preview a retained section.",sectionSchema(),true,[this](const QJsonObject& args){return beginSection(args);}});
    commands_.push_back({"analysis.inspect","Inspect Analyses","Read retained inspection records and support availability.",{},true,[this](const QJsonObject&){return QJsonObject{{"analyses",sectionInspection()}};}});
    commands_.push_back({"analysis.visibility","Show / Hide Analysis","Only one section may be visible; showing replaces the active section atomically.",{{"analysisId",QJsonObject{{"type","string"}}},{"visible",QJsonObject{{"type","boolean"}}}},true,[this](const QJsonObject& args){
        const bool sketchActive=session_.value("command")=="sketch.edit";
        if(!session_.isEmpty() && !sketchActive)return sectionError("session_busy","Complete or cancel the active command first");
        if(sketchActive && session_["baseDocumentRevision"].toInteger()!=static_cast<qint64>(document_->revision()))
            return sectionError("stale_session","The document changed; cancel and restart this sketch");
        try{const auto candidate=CadDocument::sectionCandidate(document_->authored(),args);const auto error=document_->commitAnalyses(candidate["analyses"].toArray(),"Show / Hide Section Analysis");if(!error.isEmpty())return sectionError("invalid_analysis",error);if(sketchActive)session_["baseDocumentRevision"]=static_cast<qint64>(document_->revision());return QJsonObject{{"analyses",document_->authored()["analyses"]}};}
        catch(const std::exception& error){return sectionError("invalid_arguments",QString::fromUtf8(error.what()));}
    }});
}
QJsonObject AlloyWindow::beginSection(const QJsonObject& overrides) {
    const bool duringSketch=session_.value("command")=="sketch.edit";
    if(!sectionEdit_.isEmpty() || (!session_.isEmpty() && !duringSketch))return sectionError("session_busy","Complete or cancel the active command first");
    if(duringSketch && !overrides.contains("analysisId"))return sectionError("analysis_required","Choose an existing Section Analysis while sketching");
    if(duringSketch && overrides.size()!=1)return sectionError("unsupported_option","Choose the existing section first, then edit its settings in the dock");
    if(duringSketch && session_.value("baseDocumentRevision").toInteger()!=static_cast<qint64>(document_->revision()))
        return sectionError("stale_session","The document changed; cancel and restart this sketch");
    QJsonObject inputs{{"name","Section Analysis"},{"hatch",true},{"offsetMm",0.0},{"flip",false}};
    if(overrides.contains("analysisId")) {
        bool found=false;for(const auto& entry:document_->authored()["analyses"].toArray()){const auto row=entry.toObject();if(row["id"]==overrides["analysisId"]){inputs={{"analysisId",row["id"]},{"name",row["name"]},{"plane",row["plane"]},{"hatch",row["hatch"]},{"offsetMm",row.value("offsetMm").toDouble()},{"flip",row.value("flip").toBool()}};if(row["plane"]=="face")inputs["support"]=row["support"];found=true;break;}}
        if(!found)return sectionError("stale_entity","Section analysis is unavailable");
    }
    for(auto it=overrides.begin();it!=overrides.end();++it)inputs[it.key()]=it.value();
    if(!inputs.contains("plane") && !selectedFace_.isEmpty()){inputs["plane"]="face";inputs["support"]=selectedFace_;}
    QJsonObject edit{{"id",CadDocument::newId()},{"command","inspect.section"},{"status","active"},{"inputs",inputs},
        {"newAnalysisId",CadDocument::newId()},{"baseDocumentRevision",static_cast<qint64>(document_->revision())},
        {"previewed",false},{"inputSchema",sectionSchema()}};
    if(duringSketch){
        edit["sketchSessionId"]=session_.value("id");edit["tabId"]=activeTabId_;
        sectionReturnFocus_=sketchLastFocus_ ? sketchLastFocus_ : view_;
        sectionEdit_=edit;
        if(sketchDock_)sketchDock_->setEnabled(false);
    }else{
        session_=edit;selectedFace_={};view_->selectSupportFace({},{});
    }
    showSection();previewSection();return duringSketch ? sectionEdit_ : session_;
}
QJsonObject AlloyWindow::previewSection() {
    auto& edit=sectionEdit_.isEmpty() ? session_ : sectionEdit_;
    auto fail=[this,&edit](const QString& code,const QString& message){edit["previewed"]=false;edit.remove("documentDraft");refreshSection();if(sectionDock_){sectionDock_->findChild<QLabel*>("sectionError")->setText(message);sectionDock_->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->setEnabled(false);}return sectionError(code,message);};
    if(edit["baseDocumentRevision"].toInteger()!=static_cast<qint64>(document_->revision()))return fail("stale_session","The document changed; cancel and restart Section Analysis");
    if(!sectionEdit_.isEmpty() && (edit.value("tabId")!=activeTabId_ || edit.value("sketchSessionId")!=session_.value("id")))
        return fail("stale_session","Return to the original sketch before editing this section");
    const auto inputs=edit["inputs"].toObject();if(!inputs.contains("plane"))return fail("plane_required","Select a cutting plane");
    try{auto args=inputs;args["visible"]=true;args["newAnalysisId"]=edit["newAnalysisId"];const auto candidate=CadDocument::sectionCandidate(document_->authored(),args);edit["documentDraft"]=candidate;edit["previewed"]=true;
        for(const auto& entry:candidate["analyses"].toArray())if(entry.toObject()["visible"].toBool()){
            const auto presentation=sectionPresentation(entry.toObject());if(presentation["availability"]=="unavailable")return fail("support_unavailable","Selected planar face is unavailable");view_->setSectionAnalysis(presentation);}
        if(sectionDock_){sectionDock_->findChild<QLabel*>("sectionError")->clear();sectionDock_->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->setEnabled(true);}return edit;
    }catch(const std::exception& error){return fail("invalid_arguments",QString::fromUtf8(error.what()));}
}
QJsonObject AlloyWindow::updateSectionEdit(const QJsonObject& changes) {
    if(sectionEdit_.isEmpty())return sectionError("invalid_session","Section edit is no longer active");
    for(auto it=changes.begin();it!=changes.end();++it)
        if(it.key()!="name" && it.key()!="offsetMm" && it.key()!="flip" && it.key()!="hatch")
            return sectionError("unsupported_option","The section support and identity stay fixed while sketching");
    auto inputs=sectionEdit_.value("inputs").toObject();for(auto it=changes.begin();it!=changes.end();++it)inputs[it.key()]=it.value();
    sectionEdit_["inputs"]=inputs;return previewSection();
}
QJsonObject AlloyWindow::finishSectionEdit(const QString& id,bool commit) {
    if(sectionEdit_.isEmpty() || sectionEdit_.value("id")!=id)return sectionError("invalid_session","Section edit is no longer active");
    if(commit){
        if(sectionEdit_.value("tabId")!=activeTabId_ || sectionEdit_.value("sketchSessionId")!=session_.value("id")
            || sectionEdit_.value("baseDocumentRevision").toInteger()!=static_cast<qint64>(document_->revision())
            || session_.value("baseDocumentRevision").toInteger()!=static_cast<qint64>(document_->revision()))
            return sectionError("stale_session","The document or sketch changed; return to the sketch and retry");
        if(!sectionEdit_.value("previewed").toBool())return sectionError("invalid_analysis","Resolve the section preview before accepting");
        const auto error=document_->commitAnalyses(sectionEdit_.value("documentDraft").toObject().value("analyses").toArray(),"Edit Section Analysis");
        if(!error.isEmpty())return sectionError("invalid_analysis",error);
        session_["baseDocumentRevision"]=static_cast<qint64>(document_->revision());
    }
    const auto tabId=activeTabId_,sketchId=session_.value("id").toString();
    sectionEdit_={};if(sectionDock_)sectionDock_->hide();if(sketchDock_)sketchDock_->setEnabled(true);refreshSection();
    const QPointer<QWidget> focus=sectionReturnFocus_;sectionReturnFocus_.clear();
    QTimer::singleShot(0,this,[this,focus,tabId,sketchId]{
        if(focus && sectionEdit_.isEmpty() && activeTabId_==tabId && session_.value("command")=="sketch.edit"
            && session_.value("id")==sketchId && (focus==view_ || (sketchDock_ && (focus==sketchDock_ || sketchDock_->isAncestorOf(focus)))))
            focus->setFocus(Qt::OtherFocusReason);
    });
    return {{"sessionId",id},{"status",commit ? "committed" : "cancelled"},{"sketchSessionId",session_.value("id")}};
}
void AlloyWindow::showSection() {
    if(sectionDock_){canvasWindow_->removeDockWidget(sectionDock_);delete sectionDock_;}
    const auto& edit=sectionEdit_.isEmpty() ? session_ : sectionEdit_;
    const bool returningToSketch=!sectionEdit_.isEmpty();
    sectionDock_=new QDockWidget("Section Analysis",canvasWindow_);sectionDock_->setObjectName("sectionDock");sectionDock_->setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);
    auto* panel=new QWidget(sectionDock_);sectionDock_->setWidget(panel);panel->setMinimumWidth(240);auto* form=new QFormLayout(panel);
    auto* plane=new QComboBox(panel);plane->setObjectName("sectionPlane");plane->addItems({"Select cutting plane…","XY","XZ","YZ","Selected Face"});plane->setCurrentIndex(edit["inputs"].toObject()["plane"]=="face" ? 4 : plane->findText(edit["inputs"].toObject()["plane"].toString()));if(plane->currentIndex()<0)plane->setCurrentIndex(0);plane->setEnabled(!returningToSketch);form->addRow("Cutting Plane",plane);
    auto* pick=new QPushButton("Select Face on Canvas",panel);pick->setObjectName("sectionSelectFace");pick->setVisible(!returningToSketch);form->addRow(pick);
    auto* support=new QLabel(panel);support->setObjectName("sectionSupport");support->setWordWrap(true);form->addRow(support);
    auto* clear=new QPushButton("Clear Face",panel);clear->setObjectName("sectionClearFace");clear->setVisible(!returningToSketch);form->addRow(clear);
    auto* name=new QLineEdit(edit["inputs"].toObject()["name"].toString(),panel);name->setObjectName("sectionName");form->addRow("Name",name);
    auto* offset=new QDoubleSpinBox(panel);offset->setObjectName("sectionOffset");offset->setDecimals(4);offset->setKeyboardTracking(false);offset->setRange(-100000,100000);offset->setSuffix(" mm");offset->setValue(edit["inputs"].toObject()["offsetMm"].toDouble());form->addRow("Distance",offset);
    auto* flip=new QCheckBox("Flip",panel);flip->setObjectName("sectionFlip");flip->setChecked(edit["inputs"].toObject()["flip"].toBool());form->addRow(flip);
    form->addRow("Section Color",new QLabel("From Component",panel));
    auto* hatch=new QCheckBox("Show Hatching",panel);hatch->setObjectName("sectionHatch");hatch->setChecked(edit["inputs"].toObject()["hatch"].toBool());form->addRow(hatch);
    auto* status=new QLabel(panel);status->setObjectName("sectionError");status->setWordWrap(true);form->addRow(status);
    auto update=[this](const QJsonObject& inputs){execute("command.update",{{"session",sectionEdit_.isEmpty() ? session_["id"] : sectionEdit_["id"]},{"inputs",inputs}});};
    connect(plane,&QComboBox::currentIndexChanged,panel,[plane,update](int i){if(i>0 && i<4)update({{"plane",plane->currentText()}});else if(i==4)update({{"plane","face"}});else update({{"support",QJsonValue::Null}});});
    connect(pick,&QPushButton::clicked,panel,[this]{view_->setFocus();});
    connect(clear,&QPushButton::clicked,panel,[update]{update({{"support",QJsonValue::Null}});});
    connect(name,&QLineEdit::textEdited,panel,[update](const QString& value){update({{"name",value}});});
    connect(offset,&QDoubleSpinBox::valueChanged,panel,[update](double value){update({{"offsetMm",value}});});
    connect(flip,&QCheckBox::toggled,panel,[update](bool on){update({{"flip",on}});});
    connect(hatch,&QCheckBox::toggled,panel,[update](bool on){update({{"hatch",on}});});
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,panel);form->addRow(buttons);
    if(returningToSketch){buttons->button(QDialogButtonBox::Ok)->setText("OK");buttons->button(QDialogButtonBox::Cancel)->setText("Cancel");
        buttons->setToolTip("Commit or discard the section edit, then return to the active sketch");}
    connect(buttons,&QDialogButtonBox::accepted,panel,[this,status]{const auto result=execute("command.commit",{{"session",sectionEdit_.isEmpty() ? session_["id"] : sectionEdit_["id"]}});if(!result["ok"].toBool())status->setText(result["error"].toObject()["message"].toString());});
    auto cancel=[this]{if(!sectionEdit_.isEmpty() || session_.value("command")=="inspect.section")execute("command.cancel",{{"session",sectionEdit_.isEmpty() ? session_["id"] : sectionEdit_["id"]}});};connect(buttons,&QDialogButtonBox::rejected,panel,cancel);auto* escape=new QShortcut(QKeySequence(Qt::Key_Escape),panel);escape->setContext(Qt::WidgetWithChildrenShortcut);connect(escape,&QShortcut::activated,panel,cancel);
    canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,sectionDock_);sectionDock_->show();
    // Native menu dismissal restores its previous focus after the action callback.
    QTimer::singleShot(0,plane,[plane,offset,returningToSketch]{(returningToSketch ? static_cast<QWidget*>(offset) : static_cast<QWidget*>(plane))->setFocus(Qt::OtherFocusReason);});
}
void AlloyWindow::refreshSection() {
    QJsonObject active;
    const auto& edit=sectionEdit_.isEmpty() ? session_ : sectionEdit_;
    if(edit.value("command")=="inspect.section") {
        if(edit["previewed"].toBool()){
            for(const auto& entry:edit["documentDraft"].toObject()["analyses"].toArray())if(entry.toObject()["visible"].toBool())active=entry.toObject();
        }else{
            for(const auto& entry:document_->authored()["analyses"].toArray())if(entry.toObject()["visible"].toBool())active=entry.toObject();
        }
    }else for(const auto& entry:document_->authored()["analyses"].toArray())if(entry.toObject()["visible"].toBool())active=entry.toObject();
    view_->setSectionAnalysis(active.isEmpty() ? active : sectionPresentation(active));
    if(edit.value("command")=="inspect.section" && sectionDock_) {
        const auto inputs=edit.value("inputs").toObject();
        if(auto* plane=sectionDock_->findChild<QComboBox*>("sectionPlane")){const QSignalBlocker block(plane);plane->setCurrentIndex(inputs["plane"]=="face" ? 4 : qMax(0,plane->findText(inputs["plane"].toString())));}
        if(auto* support=sectionDock_->findChild<QLabel*>("sectionSupport"))support->setText(inputs["plane"]=="face" ? (inputs["support"].toObject().isEmpty() ? "Click a planar face in the canvas" : "Face: "+inputs["support"].toObject()["faceId"].toString().left(8)) : "Origin plane");
        if(auto* offset=sectionDock_->findChild<QDoubleSpinBox*>("sectionOffset")){const QSignalBlocker block(offset);offset->setValue(inputs["offsetMm"].toDouble());offset->setEnabled(inputs.contains("plane"));}
        if(auto* flip=sectionDock_->findChild<QCheckBox*>("sectionFlip")){const QSignalBlocker block(flip);flip->setChecked(inputs["flip"].toBool());flip->setEnabled(inputs.contains("plane"));}
        if(auto* name=sectionDock_->findChild<QLineEdit*>("sectionName")){const QSignalBlocker block(name);if(!name->hasFocus())name->setText(inputs["name"].toString());}
        if(auto* hatch=sectionDock_->findChild<QCheckBox*>("sectionHatch")){const QSignalBlocker block(hatch);hatch->setChecked(inputs["hatch"].toBool());}
    }
    if(!analysesNode_)return;const QSignalBlocker block(tree_);QSet<QString> live;
    for(const auto& entry:document_->authored()["analyses"].toArray()) {
        const auto row=entry.toObject();const auto id=row["id"].toString();live.insert(id);QTreeWidgetItem* item=nullptr;
        for(int i=0;i<analysesNode_->childCount();++i)if(analysesNode_->child(i)->data(0,Qt::UserRole+2)==id)item=analysesNode_->child(i);
        if(!item)item=new QTreeWidgetItem(analysesNode_);item->setData(0,Qt::UserRole+2,id);const bool unavailable=sectionPresentation(row)["availability"]=="unavailable";item->setText(0,row["name"].toString()+(unavailable ? " (unavailable)" : ""));item->setToolTip(0,unavailable ? "Selected planar face is unavailable" : "");item->setFlags(item->flags()|Qt::ItemIsUserCheckable);item->setCheckState(0,row["visible"].toBool() ? Qt::Checked : Qt::Unchecked);item->setHidden(false);
    }
    for(int i=0;i<analysesNode_->childCount();++i)if(!live.contains(analysesNode_->child(i)->data(0,Qt::UserRole+2).toString()))analysesNode_->child(i)->setHidden(true);
}
