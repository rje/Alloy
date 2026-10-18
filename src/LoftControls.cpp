// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include "SketchGeometry.h"
#include "SurfaceClosedLoft.h"
#include <QDockWidget>
#include <QFormLayout>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QListWidget>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QShortcut>
#include <QSignalBlocker>
#include <QComboBox>
#include <QTimer>
#include <QMap>
#include <QDateTime>
#include <QStatusBar>
#include <limits>
namespace {
bool asynchronousLoft(const QJsonObject& inputs){return inputs.value("operation")=="newBody" && inputs.value("guideType")=="none";}
bool surfaceInputs(const QJsonObject& inputs){return inputs.value("operation")=="newComponent"
    || (inputs.value("operation")=="newBody" && inputs.value("guideType")=="none");}
bool sketchOwnerForLoft(const QString& owner,const CadDocument& document,const QJsonObject& inputs){
    const auto root=document.authored()["documentId"].toString();if(owner==root)return true;
    if(inputs.value("operation")!="newBody" || inputs.value("guideType")!="none")return false;
    auto active=document.activeOwnerId();
    for(const auto& value:document.authored()["features"].toArray())if(value.toObject()["id"]==inputs.value("featureId"))active=value.toObject()["ownerId"].toString();
    return owner==active;
}
QJsonObject failure(const QString& code,const QString& message){return {{"ok",false},{"error",QJsonObject{{"code",code},{"message",message}}}};}
}

struct AlloyWindow::LoftPreviewSource {
    QJsonObject key;
    EvaluatedDocument stage;
};
struct AlloyWindow::LoftPreviewEvaluation {
    QJsonObject key;
    std::shared_ptr<const EvaluatedDocument> stage;
    CadDocument::CandidateEvaluation prepared;
    QJsonObject base;
};

QJsonObject AlloyWindow::loftSourceKey() const {
    const auto edit=session_.value("inputs").toObject().value("featureId").toString();
    auto owner=document_->activeOwnerId();
    for(const auto& value:document_->authored()["features"].toArray())if(value.toObject()["id"]==edit)
        owner=value.toObject()["ownerId"].toString();
    return {{"sessionId",session_.value("id")},
        {"documentId",document_->authored().value("documentId")},
        {"revision",static_cast<qint64>(document_->revision())},{"featureId",edit},
        {"activeOwnerId",document_->activeOwnerId()},{"featureOwnerId",owner}};
}

const EvaluatedDocument& AlloyWindow::loftSourceStage(){
    const auto key=loftSourceKey();const auto edit=key.value("featureId").toString();
    if(!loftSource_ || loftSource_->key!=key){
        loftSource_.reset();loftPreview_.reset();
        if(!edit.isEmpty())session_["sourcePrefixEvaluationCount"]=session_.value("sourcePrefixEvaluationCount").toInteger()+1;
        loftSource_=std::make_shared<const LoftPreviewSource>(LoftPreviewSource{
            key,edit.isEmpty()?document_->evaluation():document_->evaluationBefore(edit)});
    }
    return loftSource_->stage;
}

QJsonObject AlloyWindow::loftCandidate(const QJsonObject& inputs) const {
    auto args=inputs;if(!args.contains("featureId") && session_.value("command")=="solid.loft") {
        args["newFeatureId"]=session_["newFeatureId"];args["newBodyId"]=session_["newBodyId"];
        if(args.value("operation")=="newComponent"){
            args["newComponentId"]=session_["newComponentId"];args["newOccurrenceId"]=session_["newOccurrenceId"];
        }
    }
    return CadDocument::loftCandidate(document_->authored(),authoringArguments(args));
}
QJsonObject AlloyWindow::beginLoft(const QJsonObject& overrides) {
    if(!session_.isEmpty())return failure("session_busy","Complete or cancel the active command first");
    loftSource_.reset();loftPreview_.reset();
    auto inputs=QJsonObject{{"profiles",QJsonArray{}},{"operation","newBody"},{"name","Loft"}};
    if(overrides.contains("featureId")) {
        bool found=false;for(const auto& entry:document_->authored()["features"].toArray()) {const auto f=entry.toObject();if(f["id"]!=overrides["featureId"])continue;if(f["type"]!="loft")return failure("stale_entity","Loft feature unavailable");inputs=f["parameters"].toObject();inputs["name"]=f["name"];inputs["featureId"]=f["id"];if(inputs.value("operation")=="newComponent")for(const auto& component:document_->authored()["components"].toArray())if(component.toObject()["id"]==f["ownerId"])inputs["componentName"]=component.toObject()["name"];found=true;}
        if(!found)return failure("stale_entity","Loft feature unavailable");
    } else if(!selectedProfile_.isEmpty())inputs["profiles"]=QJsonArray{QJsonObject{{"sketchId",selectedSketch_},{"profileId",selectedProfile_}}};
    for(auto it=overrides.begin();it!=overrides.end();++it)inputs[it.key()]=it.value();
    if(surfaceInputs(inputs)){
        inputs.remove("profiles");
        if(!inputs.contains("sections"))inputs["sections"]=inputs.value("guideType")=="none"
            ? QJsonArray{QJsonObject{},QJsonObject{},QJsonObject{}}
            : QJsonArray{QJsonObject{},QJsonObject{}};
        if(!inputs.contains("componentName"))inputs["componentName"]="Component";
    }
    const auto error=validateArguments(*findCommand("solid.loft"),inputs);if(!error.isEmpty())return failure("invalid_arguments",error);
    session_={{"id",CadDocument::newId()},{"command","solid.loft"},{"status","active"},{"inputs",inputs},
        {"newFeatureId",CadDocument::newId()},{"newBodyId",CadDocument::newId()},
        {"newComponentId",CadDocument::newId()},{"newOccurrenceId",CadDocument::newId()},
        {"selectionRole","section1"},{"baseDocumentRevision",static_cast<qint64>(document_->revision())},{"previewed",false},{"geometryEvaluated",false},{"inputSchema",findCommand("solid.loft")->parameters}};
    if(inputs.contains("featureId") && !asynchronousLoft(inputs))view_->setBodyStage(loftSourceStage().bodies,"loft-source:"+session_["id"].toString());
    showLoft();previewLoft();view_->setFocus();return session_;
}
QJsonObject AlloyWindow::previewLoft() {
    if(session_.value("evaluationStatus")=="pending")return failure("evaluation_busy","Loft evaluation is pending; wait or Cancel");
    session_.remove("evaluationStatus");
    const auto inputs=session_["inputs"].toObject();const bool surface=surfaceInputs(inputs);
    const bool railMode=surface && inputs.value("guideType")=="rail";
    const bool closedMode=surface && inputs.value("guideType")=="none";
    view_->selectLoftProfiles(surface ? QJsonArray{} : inputs["profiles"].toArray());
    view_->clearSweepPath();
    if(surface) {
        QSet<QString> visible;for(const auto& sketch:document_->sketches())if(sketchOwnerForLoft(sketch.ownerId,*document_,inputs))visible.insert(sketch.featureId);
        view_->setCommandSketches(visible);
        if(!closedMode) {
            const auto guide=inputs[railMode ? "rail" : "centerline"].toObject();
            for(const auto& sketch:document_->sketches())if(sketch.featureId==guide["sketchId"])
                view_->showSweepPath(sketch,QJsonArray{guide["entityId"]});
        }
    } else view_->setCommandSketches({});
    session_["previewed"]=false;session_["geometryEvaluated"]=false;session_["previewEvaluationReused"]=false;
    session_.remove("geometry");session_.remove("documentDraft");view_->clearPreview();
    auto reject=[this](const QString& code,const QString& message,const QJsonObject& diagnostics=QJsonObject{}){auto r=failure(code,message);auto e=r["error"].toObject();e["diagnostics"]=diagnostics;r["error"]=e;session_["previewError"]=e;if(loftDock_)if(auto* label=loftDock_->findChild<QLabel*>("loftError"))label->setText(message);return r;};
    if(loftDock_) {
        if(auto* list=loftDock_->findChild<QListWidget*>("loftProfiles")){
            const QSignalBlocker blocker(list);list->clear();
            int index=0;for(const auto& value:inputs["profiles"].toArray()){const auto ref=value.toObject();QString name=ref["sketchId"].toString();for(const auto& s:document_->sketches())if(s.featureId==name){name=s.name;break;}list->addItem(QString::number(++index)+". "+name);}
        }
        if(surface)for(const auto& role:(closedMode ? QStringList{"section1","section2","section3"}
            : QStringList{"section1","section2",railMode ? "rail" : "centerline"}))if(auto* button=loftDock_->findChild<QPushButton*>("loft."+role)){
            const auto sections=inputs["sections"].toArray();
            const auto sectionIndex=role=="section1" ? 0 : role=="section2" ? 1 : 2;
            const auto ref=role=="centerline" || role=="rail" ? inputs[role].toObject()
                : sections.size()>sectionIndex ? sections[sectionIndex].toObject() : QJsonObject{};
            QString sketchName;
            for(const auto& sketch:document_->sketches())if(sketch.featureId==ref["sketchId"]){sketchName=sketch.name;break;}
            const auto count=ref.contains("edgeIds") ? ref["edgeIds"].toArray().size()
                : ref.contains("entityIds") ? ref["entityIds"].toArray().size() : ref.isEmpty() ? 0 : 1;
            button->setText(count==0?"Select":QString("%1 selected").arg(count));
            button->setToolTip(sketchName.isEmpty()?QString{}:sketchName);
            button->setChecked(session_["selectionRole"]==role);
        }
        loftDock_->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
    if(session_["baseDocumentRevision"].toInteger()!=static_cast<qint64>(document_->revision())){
        loftSource_.reset();loftPreview_.reset();
        view_->clearBodyStage();view_->setCommandSketches({});
        return reject("stale_session","The document changed; cancel and restart Loft");
    }
    if(!asynchronousLoft(inputs))loftSourceStage();
    if(surface){const auto sections=inputs["sections"].toArray();
        if(closedMode){if(sections.size()!=3 || (sections[0].toObject()["entityIds"].toArray().size()!=2 && sections[0].toObject()["edgeIds"].toArray().size()!=2)
            || sections[1].toObject()["entityIds"].toArray().size()!=1
            || (sections[2].toObject()["entityIds"].toArray().size()!=2 && sections[2].toObject()["edgeIds"].toArray().size()!=2))
            return reject("selection_required","Select a closed two-curve section or inner Trim chain, one full ellipse, then the opposite section");}
        else if(railMode){if(sections.size()!=2 || sections[0].toObject()["entityIds"].toArray().size()!=1
            || sections[1].toObject()["entityIds"].toArray().size()!=2 || inputs["rail"].toObject().isEmpty())
            return reject("selection_required","Select one back arc, two lower lines or a mirrored fit-spline pair, then one rail");}
        else if(sections.size()!=2 || sections[0].toObject().isEmpty() || sections[1].toObject().isEmpty() || inputs["centerline"].toObject().isEmpty())
            return reject("selection_required","Select an open arc, an open line, then one fit-spline Centerline");}
    QJsonObject candidate;try{candidate=loftCandidate(inputs);}catch(const std::exception& error){return reject("invalid_arguments",QString::fromUtf8(error.what()));}
    const auto id=session_["inputs"].toObject().value("featureId").toString(session_["newFeatureId"].toString());
    auto key=loftSourceKey();key["inputs"]=inputs;key["candidate"]=candidate;
    if(loftPreview_ && loftPreview_->key!=key)loftPreview_.reset();
    if(!loftPreview_ && asynchronousLoft(inputs))return queueLoftEvaluation(candidate,key);
    loftSourceStage();
    if(!loftPreview_){
        const auto features=candidate["features"].toArray();
        const bool editing=inputs.contains("featureId");
        const bool tail=editing && !features.isEmpty() && features.last().toObject()["id"]==id;
        std::shared_ptr<const EvaluatedDocument> stage;
        if(tail && candidate==document_->authored()){
            // The complete accepted evaluation is also the exact through-feature
            // stage only for the unchanged final feature. Earlier edits may have
            // downstream modifiers, so retain their ordinary evaluation path.
            stage=std::make_shared<const EvaluatedDocument>(document_->evaluation());
            session_["previewEvaluationSource"]="accepted_tail";
        }else{
            session_["candidateEvaluationCount"]=session_.value("candidateEvaluationCount").toInteger()+1;
            const auto evaluated=CadDocument::evaluateCandidate(candidate);
            if(!evaluated.ok)return reject("evaluation_failed",evaluated.error,evaluated.diagnostics);
            stage=evaluated.evaluation;session_["previewEvaluationSource"]="candidate";
            if(editing && !tail){
                QJsonArray rows;for(const auto& row:features){rows.append(row);if(row.toObject()["id"]==id)break;}
                auto prefix=candidate;prefix["features"]=rows;
                session_["featurePrefixEvaluationCount"]=session_.value("featurePrefixEvaluationCount").toInteger()+1;
                const auto early=CadDocument::evaluateCandidate(prefix);
                if(!early.ok)return reject("evaluation_failed",early.error,early.diagnostics);
                stage=early.evaluation;
            }
        }
        loftPreview_=std::make_shared<const LoftPreviewEvaluation>(LoftPreviewEvaluation{key,std::move(stage),{}, {}});
    }else session_["previewEvaluationReused"]=true;
    const auto& stage=loftPreview_->stage;
    for(const auto& body:stage->bodies)if(body.featureId==id){view_->showPreview(body.shape,session_["inputs"].toObject().contains("featureId") ? body.bodyId : QString{},true);session_["geometry"]=body.summary;session_["featureId"]=body.featureId;session_["bodyId"]=body.bodyId;break;}
    session_["documentDraft"]=candidate;session_["previewed"]=true;session_["geometryEvaluated"]=true;session_.remove("previewError");
    if(asynchronousLoft(inputs))session_["evaluationStatus"]="completed";
    if(loftDock_){loftDock_->findChild<QLabel*>("loftError")->clear();loftDock_->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->setEnabled(true);}
    return session_;
}
QJsonObject AlloyWindow::selectLoftProfile(const QString& sketchId,const QString& profileId) {
    auto inputs=session_["inputs"].toObject();auto refs=inputs["profiles"].toArray();QJsonArray next;bool removed=false;
    for(const auto& value:refs){const auto r=value.toObject();if(r["sketchId"]==sketchId && r["profileId"]==profileId){removed=true;continue;}next.append(r);}
    if(!removed)next.append(QJsonObject{{"sketchId",sketchId},{"profileId",profileId}});inputs["profiles"]=next;session_["inputs"]=inputs;previewLoft();return {{"kind","profile"},{"sketchId",sketchId},{"profileId",profileId},{"profiles",next}};
}
QJsonObject AlloyWindow::selectLoftEntity(const QString& role,const QString& sketchId,const QString& entityId) {
    if(session_.value("command")!="solid.loft" || !surfaceInputs(session_.value("inputs").toObject()))return failure("invalid_session","Start a surface Loft first");
    if(session_["baseDocumentRevision"].toInteger()!=static_cast<qint64>(document_->revision()))return previewLoft();
    const auto guideType=session_["inputs"].toObject().value("guideType").toString("centerline");
    const auto railMode=guideType=="rail",closedMode=guideType=="none";
    if((closedMode && role!="section1" && role!="section2" && role!="section3")
        || (!closedMode && role=="section3")
        || (railMode && role=="centerline") || (!railMode && role=="rail"))
        return failure("invalid_arguments","Choose a selector in the active Loft mode");
    const auto required=closedMode ? (role=="section2" ? "ellipse" : "fitSpline")
        : role=="section1" ? "arc" : role=="section2" ? "line" : role=="centerline" || role=="rail" ? "fitSpline" : "";
    if(!*required)return failure("invalid_arguments","Choose a Loft section or Centerline role");
    const auto& stage=loftSourceStage();
    bool found=false;for(const auto& sketch:stage.sketches)if(sketch.featureId==sketchId && sketch.visible
        && sketchOwnerForLoft(sketch.ownerId,*document_,session_["inputs"].toObject()) && sketch.entityEdges.contains(entityId))
        for(const auto& row:sketch.summary["entities"].toArray())if(row.toObject()["id"]==entityId && (row.toObject()["type"]==required || (railMode && role=="section2" && row.toObject()["type"]=="fitSpline"))
            && !row.toObject()["construction"].toBool())found=true;
    if(!found)return failure("loft_source_unavailable",QString("Select an ordinary retained %1 in a visible permitted sketch").arg(required));
    auto inputs=session_["inputs"].toObject();const QJsonObject ref{{"sketchId",sketchId},{"entityId",entityId}};
    if(role=="centerline" || role=="rail")inputs[role]=ref;
    else {auto sections=inputs["sections"].toArray();
        if(sections.size()!=(closedMode ? 3 : 2))sections=closedMode
            ? QJsonArray{QJsonObject{},QJsonObject{},QJsonObject{}} : QJsonArray{QJsonObject{},QJsonObject{}};
        const int sectionIndex=role=="section1" ? 0 : role=="section2" ? 1 : 2;
        if(closedMode) {
            auto section=sections[sectionIndex].toObject();auto ids=section["entityIds"].toArray();
            if(section["sketchId"]!=sketchId)ids={};
            if(ids.contains(entityId)){for(int i=0;i<ids.size();++i)if(ids[i]==entityId){ids.removeAt(i);break;}}
            else if(ids.size()<(sectionIndex==1 ? 1 : 2))ids.append(entityId);
            else return failure("loft_section_full","This no-guide section already has its required curves");
            sections[sectionIndex]=QJsonObject{{"sketchId",sketchId},{"entityIds",ids}};
        } else if(railMode){
            auto section=sections[role=="section1"?0:1].toObject();auto ids=section["entityIds"].toArray();
            if(section["sketchId"]!=sketchId)ids={};
            if(role=="section1")ids=QJsonArray{entityId};
            else if(ids.contains(entityId)){for(int i=0;i<ids.size();++i)if(ids[i]==entityId){ids.removeAt(i);break;}}
            else if(ids.size()<2)ids.append(entityId);
            else return failure("loft_section_full","Rail Loft accepts two lower lines or a mirrored fit-spline pair");
            sections[role=="section1"?0:1]=QJsonObject{{"sketchId",sketchId},{"entityIds",ids}};
        }else sections[role=="section1"?0:1]=ref;
        inputs["sections"]=sections;}
    session_["inputs"]=inputs;
    if(closedMode) {
        const int sectionIndex=role=="section1" ? 0 : role=="section2" ? 1 : 2;
        const auto count=inputs["sections"].toArray()[sectionIndex].toObject()["entityIds"].toArray().size();
        session_["selectionRole"]=count==(sectionIndex==1 ? 1 : 2) && sectionIndex<2
            ? QString("section%1").arg(sectionIndex+2) : role;
    } else if(role=="section1")session_["selectionRole"]="section2";
    else if(role=="section2")session_["selectionRole"]=!railMode || inputs["sections"].toArray()[1].toObject()["entityIds"].toArray().size()==2
        ? (railMode ? "rail" : "centerline") : "section2";
    else session_["selectionRole"]=role;
    const auto preview=previewLoft();if(session_.value("evaluationStatus")=="pending")return session_;return {{"role",role},{"sketchId",sketchId},{"entityId",entityId},{"previewed",preview.value("previewed")}};
}
QJsonObject AlloyWindow::selectLoftBoundary(const QString& role,const QString& bodyId,const QString& edgeId){
    const auto inputs=session_["inputs"].toObject();
    if(session_.value("command")!="solid.loft" || !surfaceInputs(inputs) || inputs.value("guideType")!="none"
        || (role!="section1" && role!="section3"))return failure("invalid_session","Choose an outer no-guide surface Loft section first");
    if(session_["baseDocumentRevision"].toInteger()!=static_cast<qint64>(document_->revision()))return previewLoft();
    const auto& stage=loftSourceStage();
    QJsonObject ref;try{ref=SurfaceClosedLoft::boundaryReference(stage,bodyId,edgeId);}
    catch(const GeometryKernel::EvaluationError& error){return failure("loft_boundary_unavailable",QString::fromUtf8(error.what()));}
    bool visible=false;for(const auto& body:stage.bodies)if(body.bodyId==bodyId && body.visible)visible=true;
    if(!visible)return failure("loft_boundary_unavailable","Select an ordinary edge of a visible earlier Trim body");
    auto next=inputs;auto sections=next["sections"].toArray();if(sections.size()!=3)sections={QJsonObject{},QJsonObject{},QJsonObject{}};
    sections[role=="section1" ? 0 : 2]=ref;next["sections"]=sections;session_["inputs"]=next;
    session_["selectionRole"]=role=="section1" ? "section2" : "section3";
    const auto preview=previewLoft();if(session_.value("evaluationStatus")=="pending")return session_;return {{"role",role},{"boundary",ref},{"previewed",preview.value("previewed")}};
}
QJsonObject AlloyWindow::pickLoft(double x,double y) {
    if(session_["baseDocumentRevision"].toInteger()!=static_cast<qint64>(document_->revision()))return previewLoft();
    const auto role=session_.value("selectionRole").toString("section1");
    const bool closedMode=session_["inputs"].toObject().value("guideType")=="none";
    const bool railMode=session_["inputs"].toObject().value("guideType")=="rail";
    const auto required=closedMode ? (role=="section2" ? "ellipse" : "fitSpline")
        : role=="section1" ? "arc" : role=="section2" ? "line" : "fitSpline";
    const auto& stage=loftSourceStage();
    if(closedMode && role!="section2"){
        const auto hit=view_->pickSolidEdge(x,y,stage.bodies);
        if(!hit.isEmpty()){
            if(hit.contains("occurrenceId"))return failure("loft_boundary_unavailable","Placed occurrence boundary references are outside this Loft mode");
            return selectLoftBoundary(role,hit["bodyId"].toString(),hit["edgeId"].toString());
        }
    }
    QString sketchId,entityId;double nearest=7;bool ambiguous=false;
    for(const auto& sketch:stage.sketches)if(sketch.visible && sketchOwnerForLoft(sketch.ownerId,*document_,session_["inputs"].toObject())) {
        auto source=SketchGeometry::findFeature(document_->authored(),sketch.featureId);
        auto parameters=source["parameters"].toObject();auto points=parameters["points"].toArray();
        const auto frame=sketch.resolvedFrame;const auto origin=frame["origin"].toArray(),u=frame["uAxis"].toArray(),v=frame["vAxis"].toArray();
        QMap<QString,QJsonArray> world;for(const auto& row:sketch.summary["worldPoints"].toArray())world[row.toObject()["id"].toString()]=row.toObject()["positionMm"].toArray();
        if(origin.size()!=3 || u.size()!=3 || v.size()!=3)continue;
        for(int i=0;i<points.size();++i){auto point=points[i].toObject();const auto xyz=world.value(point["id"].toString());if(xyz.size()!=3)continue;
            double a=0,b=0;for(int j=0;j<3;++j){const auto delta=xyz[j].toDouble()-origin[j].toDouble();a+=delta*u[j].toDouble();b+=delta*v[j].toDouble();}
            point["uMm"]=a;point["vMm"]=b;points[i]=point;}
        parameters["points"]=points;source["parameters"]=parameters;
        const auto hit=view_->pickSketch(source,x,y,"entity",frame);
        if(hit["kind"]!="entity" || hit["projected"].toBool())continue;
        bool rightType=false;for(const auto& row:sketch.summary["entities"].toArray())
            if(row.toObject()["id"]==hit["entityId"] && (row.toObject()["type"]==required || (railMode && role=="section2" && row.toObject()["type"]=="fitSpline"))
                && !row.toObject()["construction"].toBool())rightType=true;
        if(!rightType)continue;
        const auto distance=hit["distancePixels"].toDouble();
        if(distance<nearest-1e-6){nearest=distance;sketchId=sketch.featureId;entityId=hit["entityId"].toString();ambiguous=false;}
        else if(std::abs(distance-nearest)<1e-6)ambiguous=true;
    }
    if(ambiguous)return failure("ambiguous_loft_source","More than one sketch curve is under this point");
    if(sketchId.isEmpty())return failure("loft_source_required",closedMode
        ? "Click a visible fit spline or full ellipse for this section"
        : "Click a visible open section or fit-spline Centerline");
    return selectLoftEntity(role,sketchId,entityId);
}
void AlloyWindow::showLoft() {
    if(loftDock_){canvasWindow_->removeDockWidget(loftDock_);delete loftDock_;}
    loftDock_=new QDockWidget("Loft",canvasWindow_);loftDock_->setObjectName("loftDock");loftDock_->setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);
    auto* panel=new QWidget(loftDock_);loftDock_->setWidget(panel);
    auto* stack=new QVBoxLayout(panel);stack->setContentsMargins(0,0,0,8);
    auto* scroll=new QScrollArea(panel);scroll->setObjectName("loft.formScroll");
    scroll->setWidgetResizable(true);scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto* contents=new QWidget(scroll);scroll->setWidget(contents);stack->addWidget(scroll,1);
    auto* form=new QFormLayout(contents);form->setRowWrapPolicy(QFormLayout::WrapLongRows);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    auto* name=new QLineEdit(session_["inputs"].toObject()["name"].toString(),panel);name->setObjectName("loftName");form->addRow("Name",name);
    connect(name,&QLineEdit::editingFinished,panel,[this,name]{auto inputs=session_["inputs"].toObject();inputs["name"]=name->text();session_["inputs"]=inputs;previewLoft();});
    const bool surface=surfaceInputs(session_["inputs"].toObject());
    auto* operation=new QComboBox(panel);operation->setObjectName("loftOperation");operation->addItem("New Body","newBody");operation->addItem("New Component","newComponent");operation->setCurrentIndex(session_["inputs"].toObject().value("operation")=="newComponent"?1:0);
    operation->setEnabled(!session_["inputs"].toObject().contains("featureId")
        && (!session_["surfaceOnly"].toBool() || session_["inputs"].toObject().value("guideType")=="none"));form->addRow("Operation",operation);
    connect(operation,qOverload<int>(&QComboBox::currentIndexChanged),panel,[this,operation](int index){
        auto inputs=session_["inputs"].toObject();if(inputs.contains("featureId"))return;
        const auto choice=operation->itemData(index).toString();inputs["operation"]=choice;
        if(inputs.value("guideType")=="none" && surfaceInputs(session_["inputs"].toObject())){
            inputs.remove("profiles");if(!inputs.contains("componentName"))inputs["componentName"]="Component";
        } else if(choice=="newComponent"){
            inputs.remove("profiles");inputs["sections"]=QJsonArray{QJsonObject{},QJsonObject{}};inputs["componentName"]="Component";
            session_["selectionRole"]="section1";
        } else {inputs.remove("sections");inputs.remove("centerline");inputs.remove("rail");inputs.remove("guideType");inputs.remove("componentName");inputs["profiles"]=QJsonArray{};}
        session_["inputs"]=inputs;QTimer::singleShot(0,this,[this]{if(session_.value("command")!="solid.loft")return;showLoft();previewLoft();view_->setFocus();});
    });
    if(surface) {
        const auto mode=session_["inputs"].toObject().value("guideType").toString("centerline");
        const bool railMode=mode=="rail",closedMode=mode=="none";
        auto* guideType=new QComboBox(panel);guideType->setObjectName("loftGuideType");
        guideType->addItem("Centerline","centerline");guideType->addItem("Rails","rail");
        guideType->addItem("None · three closed sections","none");
        guideType->setCurrentIndex(closedMode?2:railMode?1:0);
        guideType->setEnabled(!session_["inputs"].toObject().contains("featureId"));form->addRow("Guide Type",guideType);
        connect(guideType,qOverload<int>(&QComboBox::currentIndexChanged),panel,[this,guideType](int index){
            auto inputs=session_["inputs"].toObject();if(inputs.contains("featureId"))return;
            const auto choice=guideType->itemData(index).toString();
            inputs["guideType"]=choice;inputs.remove("centerline");inputs.remove("rail");
            if(choice!="none" && inputs.value("operation")=="newBody")inputs["operation"]="newComponent";
            inputs["sections"]=choice=="none" ? QJsonArray{QJsonObject{},QJsonObject{},QJsonObject{}}
                : QJsonArray{QJsonObject{},QJsonObject{}};
            session_["inputs"]=inputs;session_["selectionRole"]="section1";
            QTimer::singleShot(0,this,[this]{if(session_.value("command")!="solid.loft")return;showLoft();previewLoft();});
        });
        auto* component=new QLineEdit(session_["inputs"].toObject()["componentName"].toString(),panel);
        component->setObjectName("loftComponentName");component->setEnabled(!session_["inputs"].toObject().contains("featureId"));
        if(session_["inputs"].toObject().value("operation")=="newComponent")form->addRow("Component",component);
        else component->hide();
        connect(component,&QLineEdit::editingFinished,panel,[this,component]{auto inputs=session_["inputs"].toObject();inputs["componentName"]=component->text();session_["inputs"]=inputs;previewLoft();});
        for(const auto& role:(closedMode ? QStringList{"section1","section2","section3"}
            : QStringList{"section1","section2",railMode ? "rail" : "centerline"})) {
            auto* button=new QPushButton("Select",panel);button->setObjectName("loft."+role);button->setCheckable(true);
            form->addRow(closedMode ? (role=="section1" ? "Section 1"
                : role=="section2" ? "Section 2" : "Section 3")
                : role=="section1" ? "Profile 1 · arc"
                : role=="section2" ? (railMode?"Profile 2 · two curves":"Profile 2 · line")
                : (railMode?"Rail":"Centerline"),button);
            connect(button,&QPushButton::clicked,panel,[this,role]{execute("solid.loftSelection",{{"role",role}});previewLoft();});
        }
        auto* help=new QLabel(closedMode
            ? "Select a closed two-spline section or an inner Trim boundary chain, one full sketch ellipse, then the opposite section. Chain Selection collects both inner edges. No rails; the Loft sequence stays open."
            : railMode
            ? "Select the back arc, then two lower lines or a retained mirrored fit-spline pair, and the rail. Lines use endpoint contacts; mirrored splines use an interior back contact and common tip."
            : "Click an open arc, then an open line on the endpoint planes. Select the retained fit-spline Centerline. The result is one sheet face.",panel);
        help->setWordWrap(true);form->addRow(help);
        if(closedMode)form->addRow("Chain Selection",new QLabel("On · inner boundary loop",panel));
        form->addRow("Closed",new QLabel("Off · surface",panel));
    } else {
        auto* list=new QListWidget(panel);list->setObjectName("loftProfiles");list->setMinimumHeight(100);form->addRow("Profiles",list);
        auto* help=new QLabel("Click circular profiles in order. Click again to remove.",panel);help->setWordWrap(true);form->addRow(help);
        for(const auto& action:QStringList{"Move Up","Move Down","Remove"}){auto* button=new QPushButton(action,panel);form->addRow(button);connect(button,&QPushButton::clicked,panel,[this,list,action]{auto inputs=session_["inputs"].toObject();auto refs=inputs["profiles"].toArray();const auto index=list->currentRow();if(index<0 || index>=refs.size())return;int target=index;if(action=="Remove")refs.removeAt(index);else{target=index+(action=="Move Up" ? -1 : 1);if(target<0 || target>=refs.size())return;const QJsonValue temp=refs[index];refs[index]=refs[target];refs[target]=temp;}inputs["profiles"]=refs;session_["inputs"]=inputs;previewLoft();list->setCurrentRow(std::min(target,int(refs.size())-1));});}
        form->addRow("Start/End",new QLabel("Free",panel));form->addRow("Guide Type",new QLabel("Rails · none",panel));form->addRow("Chain Selection",new QLabel("On",panel));form->addRow("Closed",new QLabel("Off",panel));
    }
    auto* error=new QLabel(panel);error->setObjectName("loftError");error->setWordWrap(true);stack->addWidget(error);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,panel);stack->addWidget(buttons);
    connect(buttons,&QDialogButtonBox::accepted,panel,[this,error]{const auto response=execute("command.commit",{{"session",session_["id"]}});if(!response["ok"].toBool())error->setText(response["error"].toObject()["message"].toString());});connect(buttons,&QDialogButtonBox::rejected,panel,[this]{execute("command.cancel",{{"session",session_["id"]}});});
    auto cancel=[this]{if(session_.value("command")=="solid.loft")execute("command.cancel",{{"session",session_["id"]}});};auto* escape=new QShortcut(QKeySequence(Qt::Key_Escape),panel);escape->setContext(Qt::WidgetWithChildrenShortcut);connect(escape,&QShortcut::activated,panel,cancel);
    if(!loftEscape_){loftEscape_=new QShortcut(QKeySequence(Qt::Key_Escape),view_);loftEscape_->setContext(Qt::WidgetShortcut);connect(loftEscape_,&QShortcut::activated,this,cancel);}loftEscape_->setEnabled(true);
    canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,loftDock_);loftDock_->show();
    if(session_.value("evaluationStatus")=="pending")refreshLoftPending();
}

void AlloyWindow::refreshLoftPending(){
    const bool pending=session_.value("evaluationStatus")=="pending";
    if(!pending){for(const auto& control:loftFrozenControls_)if(control)control->setEnabled(true);loftFrozenControls_.clear();}
    if(loftDock_){
        auto freeze=[&](QWidget* control){if(pending && control->isEnabled()){loftFrozenControls_.append(control);control->setEnabled(false);}};
        for(auto* field:loftDock_->findChildren<QLineEdit*>())freeze(field);
        for(auto* field:loftDock_->findChildren<QComboBox*>())freeze(field);
        for(auto* field:loftDock_->findChildren<QListWidget*>())freeze(field);
        auto* buttons=loftDock_->findChild<QDialogButtonBox*>();
        for(auto* button:loftDock_->findChildren<QPushButton*>())if(!buttons || button!=buttons->button(QDialogButtonBox::Cancel))freeze(button);
        if(buttons)buttons->button(QDialogButtonBox::Ok)->setEnabled(!pending && session_.value("previewed").toBool());
        if(auto* label=loftDock_->findChild<QLabel*>("loftError"))label->setText(pending ? "Evaluating Loft… You can navigate or Cancel."
            : session_.value("previewError").toObject().value("message").toString());
    }
    if(pending){status_->hide();statusBar()->showMessage("Evaluating Loft… You can navigate or Cancel to discard the result.");}
    else {status_->show();if(statusBar()->currentMessage().startsWith("Evaluating Loft"))statusBar()->clearMessage();}
}

QJsonObject AlloyWindow::queueLoftEvaluation(const QJsonObject& candidate,const QJsonObject& key){
    if(geometryEvaluationRunning_ || geometryEvaluationQueued_)return failure("evaluation_busy","Background geometry is still finishing");
    const auto inputs=session_.value("inputs").toObject();
    if(!inputs.contains("featureId"))loftSourceStage(); // Cheap GUI cache only; never passed to the worker.
    auto task=std::make_shared<GeometryEvaluation>();task->command="solid.loft";task->phase="preview";
    task->document=document_;task->documentId=document_->authored().value("documentId").toString();task->tabId=activeTabId_;
    task->base=document_->authored();task->revision=document_->revision();task->sessionId=session_.value("id").toString();
    task->generation=++geometryEvaluationGeneration_;task->inputs=inputs;task->ownerId=document_->activeOwnerId();task->candidate=candidate;task->cacheKey=key;
    task->featureId=inputs.value("featureId").toString(session_.value("newFeatureId").toString());auto sourceKey=key;sourceKey.remove("inputs");sourceKey.remove("candidate");
    task->needsSource=inputs.contains("featureId") && (!loftSource_ || loftSource_->key!=sourceKey);
    for(const auto& row:candidate.value("features").toArray())if(row.toObject().value("id")==task->featureId){task->feature=row.toObject();break;}
    session_["candidateEvaluationCount"]=session_.value("candidateEvaluationCount").toInteger()+1;
    if(task->needsSource)session_["sourcePrefixEvaluationCount"]=session_.value("sourcePrefixEvaluationCount").toInteger()+1;
    if(inputs.contains("featureId") && candidate.value("features").toArray().last().toObject().value("id")!=task->featureId)
        session_["featurePrefixEvaluationCount"]=session_.value("featurePrefixEvaluationCount").toInteger()+1;
    geometryEvaluationTerminal_={};documentOpenOperation_={};sketchEvaluationOperation_={};geometryEvaluationQueued_=task;
    session_["evaluationPhase"]="preview";session_["evaluationStatus"]="pending";session_["previewGeneration"]=static_cast<qint64>(task->generation);
    session_["evaluationHeartbeat"]=QJsonObject{{"ticks",0},{"lastMs",QDateTime::currentMSecsSinceEpoch()},{"maxGapMs",0}};
    session_["cancellation"]="Discard result; running kernel evaluation continues";session_.remove("previewError");
    refreshLoftPending();startGeometryEvaluation();return session_;
}

void AlloyWindow::completeLoftEvaluation(const std::shared_ptr<GeometryEvaluation>& task){
    for(const auto& control:loftFrozenControls_)if(control)control->setEnabled(true);loftFrozenControls_.clear();
    status_->show();if(statusBar()->currentMessage().startsWith("Evaluating Loft"))statusBar()->clearMessage();
    if(session_.value("id")!=task->sessionId || session_.value("command")!=task->command || geometryEvaluationGeneration_!=task->generation)return;
    auto reject=[&](const QString& code,const QString& message,const QJsonObject& diagnostics=QJsonObject{}){
        session_["evaluationStatus"]="failed";session_["previewed"]=false;loftPreview_.reset();
        session_["previewError"]=QJsonObject{{"code",code},{"message",message},{"diagnostics",diagnostics}};refreshLoftPending();
    };
    if(document_!=task->document || activeTabId_!=task->tabId || document_->revision()!=task->revision || document_->authored()!=task->base
        || session_.value("inputs").toObject()!=task->inputs || document_->activeOwnerId()!=task->ownerId
        || loftSourceKey().value("featureOwnerId")!=task->cacheKey.value("featureOwnerId")
        || task->featureId!=task->inputs.value("featureId").toString(session_.value("newFeatureId").toString())
        || (!task->inputs.contains("featureId") && task->feature.value("bodyId")!=session_.value("newBodyId"))){
        reject("stale_session","The document or ordered Loft sections changed; cancel and restart");return;
    }
    session_["evaluationTimingMs"]=QJsonObject{{"source",task->sourceMs},{"full",task->fullMs},{"historicalStage",task->stageMs}};
    if(!task->error.isEmpty()){reject("evaluation_failed",task->error);return;}
    if(!task->full.ok){reject("evaluation_failed",task->full.error,task->full.diagnostics);return;}
    if(!task->stage.ok){reject("evaluation_failed",task->stage.error,task->stage.diagnostics);return;}
    if(task->source){
        auto sourceKey=task->cacheKey;sourceKey.remove("inputs");sourceKey.remove("candidate");
        loftSource_=std::make_shared<const LoftPreviewSource>(LoftPreviewSource{sourceKey,*task->source});
        view_->setBodyStage(task->source->bodies,"loft-source:"+task->sessionId);
    }
    loftPreview_=std::make_shared<const LoftPreviewEvaluation>(LoftPreviewEvaluation{task->cacheKey,task->stage.evaluation,task->full,task->base});
    session_.remove("evaluationStatus");const auto preview=previewLoft();
    if(preview.value("ok").isBool() && !preview.value("ok").toBool()){
        const auto error=preview.value("error").toObject();reject(error.value("code").toString(),error.value("message").toString(),error.value("diagnostics").toObject());return;
    }
    session_["previewEvaluationSource"]="candidate";session_["previewEvaluationReused"]=false;refreshLoftPending();refresh();
}

QString AlloyWindow::commitLoftPreview(){
    if(!loftPreview_ || !loftPreview_->prepared.ok || !session_.value("previewed").toBool() || session_.value("evaluationStatus")!="completed")
        return "Wait for a valid completed Loft preview before accepting";
    if(loftPreview_->key.value("sessionId")!=session_.value("id") || loftPreview_->key.value("inputs")!=session_.value("inputs")
        || loftPreview_->key.value("activeOwnerId")!=document_->activeOwnerId())return "The Loft selection changed; preview it again before accepting";
    return document_->commitLoftEvaluation(loftPreview_->prepared,loftPreview_->key.value("candidate").toObject(),loftPreview_->base,
        static_cast<quint64>(loftPreview_->key.value("revision").toInteger()));
}
