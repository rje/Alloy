// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "BoundaryFill.h"
#include "ReferenceView.h"
#include "SurfaceExtrude.h"
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QFormLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QShortcut>
#include <QSignalBlocker>
#include <algorithm>

namespace {
QJsonObject failure(const QString& code,const QString& message,const QJsonObject& diagnostics={}){
    return {{"ok",false},{"error",QJsonObject{{"code",code},{"message",message},{"diagnostics",diagnostics}}}};
}
QJsonArray canonicalTools(const QJsonArray& tools){
    std::vector<QJsonObject> rows;for(const auto& value:tools)rows.push_back(value.toObject());
    std::sort(rows.begin(),rows.end(),[](const auto& a,const auto& b){
        const auto ak=a.value("bodyId").toString()+"/"+a.value("producerFeatureId").toString();
        const auto bk=b.value("bodyId").toString()+"/"+b.value("producerFeatureId").toString();return ak<bk;});
    QJsonArray result;for(const auto& row:rows)result.append(row);return result;
}
}
struct AlloyWindow::BoundaryFillPreviewSource {QJsonObject key;EvaluatedDocument stage;};
struct AlloyWindow::BoundaryFillPreviewPartition {QJsonObject key;BoundaryFillPartition partition;};

void AlloyWindow::clearBoundaryFillState(){
    boundaryFillSource_.reset();boundaryFillPartition_.reset();
    for(auto& marker:boundaryFillMarkers_)if(marker){marker->hide();marker->setObjectName({});marker->deleteLater();}boundaryFillMarkers_.clear();
}
const EvaluatedDocument& AlloyWindow::boundaryFillSourceStage(){
    const auto edit=session_.value("inputs").toObject().value("featureId").toString();
    const QJsonObject key{{"sessionId",session_.value("id")},{"documentId",document_->authored().value("documentId")},
        {"revision",static_cast<qint64>(document_->revision())},{"featureId",edit},{"ownerId",session_.value("ownerId")}};
    if(!boundaryFillSource_ || boundaryFillSource_->key!=key){
        clearBoundaryFillState();boundaryFillSource_=std::make_shared<const BoundaryFillPreviewSource>(BoundaryFillPreviewSource{
            key,edit.isEmpty()?document_->evaluation():document_->evaluationBefore(edit)});
    }
    return boundaryFillSource_->stage;
}
QJsonObject AlloyWindow::boundaryFillFeature(const QJsonObject& inputs) const{
    auto parameters=inputs;for(const auto& key:{"featureId","name"})parameters.remove(key);
    return {{"id",inputs.value("featureId").toString(session_.value("newFeatureId").toString())},
        {"bodyId",session_.value("newBodyId")},{"ownerId",session_.value("ownerId")},{"type","boundaryFill"},
        {"name",inputs.value("name").toString("Boundary Fill")},{"visible",true},{"parameters",parameters}};
}
QJsonObject AlloyWindow::boundaryFillCandidate(const QJsonObject& inputs) const{
    auto args=inputs;
    if(args.contains("featureId"))args.remove("ownerId");
    else{
        args["ownerId"]=document_->activeOwnerId();
        if(session_.value("command")=="surface.boundaryFill"){
            args["ownerId"]=session_.value("ownerId");args["newFeatureId"]=session_.value("newFeatureId");args["newBodyId"]=session_.value("newBodyId");}
    }
    return CadDocument::boundaryFillCandidate(document_->authored(),args);
}
QJsonObject AlloyWindow::beginBoundaryFill(const QJsonObject& overrides){
    if(!session_.isEmpty())return failure("session_busy","Complete or cancel the active command first");
    clearBoundaryFillState();
    QJsonObject inputs{{"operation","newBody"},{"tools",QJsonArray{}},{"removeTools",false},{"name","Boundary Fill"}};
    QString owner=document_->activeOwnerId(),bodyId=CadDocument::newId();
    if(overrides.contains("featureId")){
        bool found=false;for(const auto& row:document_->authored().value("features").toArray()){
            const auto feature=row.toObject();if(feature.value("id")!=overrides.value("featureId"))continue;
            if(feature.value("type")!="boundaryFill")return failure("stale_entity","Boundary Fill feature unavailable");
            inputs=feature.value("parameters").toObject();inputs["featureId"]=feature.value("id");inputs["name"]=feature.value("name");
            owner=feature.value("ownerId").toString();bodyId=feature.value("bodyId").toString();found=true;break;}
        if(!found)return failure("stale_entity","Boundary Fill feature unavailable");
    }
    for(auto it=overrides.begin();it!=overrides.end();++it)inputs[it.key()]=it.value();
    const auto error=validateArguments(*findCommand("surface.boundaryFillDialog"),inputs);
    if(!error.isEmpty())return failure("invalid_arguments",error);
    session_={{"id",CadDocument::newId()},{"command","surface.boundaryFill"},{"status","active"},{"inputs",inputs},
        {"ownerId",owner},{"newFeatureId",CadDocument::newId()},{"newBodyId",bodyId},
        {"baseDocumentRevision",static_cast<qint64>(document_->revision())},{"previewed",false},{"geometryEvaluated",false},
        {"selectionRole",inputs.contains("featureId")?"cells":"tools"},{"inputSchema",findCommand("surface.boundaryFill")->parameters}};
    showBoundaryFill();previewBoundaryFill();view_->setFocus(Qt::OtherFocusReason);return session_;
}
QJsonObject AlloyWindow::previewBoundaryFill(){
    if(session_.value("command")!="surface.boundaryFill")return failure("invalid_session","Start Boundary Fill first");
    session_["previewed"]=false;session_["geometryEvaluated"]=false;session_["partitionReused"]=false;
    session_.remove("geometry");session_.remove("documentDraft");session_.remove("cells");view_->clearPreview();
    for(auto& marker:boundaryFillMarkers_)if(marker){marker->hide();marker->setObjectName({});marker->deleteLater();}boundaryFillMarkers_.clear();
    auto reject=[&](const QString& code,const QString& message,const QJsonObject& diagnostics=QJsonObject{}){
        const auto result=failure(code,message,diagnostics);session_["previewError"]=result.value("error");
        if(boundaryFillSource_ && session_.value("cells").toArray().isEmpty())
            view_->setBodyStage(boundaryFillSource_->stage.bodies,"boundary-fill-tools:"+session_.value("id").toString());
        refreshBoundaryFillControls();return result;};
    if(session_.value("baseDocumentRevision").toInteger()!=static_cast<qint64>(document_->revision())){
        clearBoundaryFillState();view_->clearBodyStage();return reject("stale_session","The document changed; cancel and restart Boundary Fill");}
    const auto inputs=session_.value("inputs").toObject();
    if(!inputs.contains("featureId") && session_.value("ownerId").toString()!=document_->activeOwnerId()){
        clearBoundaryFillState();view_->clearBodyStage();return reject("stale_session","The active modeling owner changed; restart Boundary Fill");}
    try{
        const auto owner=session_.value("ownerId").toString(),root=document_->authored().value("documentId").toString();
        if(owner!=root){int count=0;bool identity=false;
            for(const auto& row:document_->authored().value("occurrences").toArray()){const auto occurrence=row.toObject();
                if(occurrence.value("definitionId")==owner){++count;identity=occurrence.value("parentId")==root
                    && occurrence.value("placement")=="identity" && !occurrence.contains("parentOccurrenceId");}}
            if(count!=1 || !identity)return reject("cross_component_dependency","Boundary Fill requires a root or one identity component owner");}
        const auto& source=boundaryFillSourceStage();auto key=boundaryFillSource_->key;
        for(const auto& row:inputs.value("tools").toArray()){
            const auto ref=row.toObject();const EvaluatedBody* tool=nullptr;for(const auto& body:source.bodies)if(body.bodyId==ref.value("bodyId"))tool=&body;
            if(!tool || tool->featureId!=ref.value("producerFeatureId"))return reject("boundary_fill_tool_unavailable","Select each exact earlier tool body and producer");
            if(tool->ownerId!=owner)return reject("cross_component_dependency","Boundary Fill tools must belong to its modeling owner");}
        key["featureId"]=inputs.value("featureId").toString(session_.value("newFeatureId").toString());key["bodyId"]=session_.value("newBodyId");
        key["tools"]=canonicalTools(inputs.value("tools").toArray());
        if(boundaryFillPartition_ && boundaryFillPartition_->key!=key)boundaryFillPartition_.reset();
        if(session_.value("selectionRole")=="tools"){
            view_->setBodyStage(source.bodies,"boundary-fill-tools:"+session_.value("id").toString());
            session_.remove("previewError");refreshBoundaryFillControls();return session_;}
        if(inputs.value("tools").toArray().isEmpty())return reject("boundary_fill_tools_required","Select enclosing surface tools first");
        const bool reused=bool(boundaryFillPartition_);
        if(!reused){session_["partitionBuildCount"]=session_.value("partitionBuildCount").toInteger()+1;
            boundaryFillPartition_=std::make_shared<const BoundaryFillPreviewPartition>(BoundaryFillPreviewPartition{
                key,BoundaryFill::partition(boundaryFillFeature(inputs),source)});}
        session_["partitionReused"]=reused;const auto& partition=boundaryFillPartition_->partition;
        const auto rows=BoundaryFill::descriptors(partition);session_["cells"]=rows;
        session_["geometry"]=QJsonObject{{"cells",rows},{"diagnostics",partition.diagnostics}};session_["geometryEvaluated"]=true;
        if(rows.isEmpty())return reject("boundary_fill_no_cells","These finite tools do not enclose a bounded cell",partition.diagnostics);
        // Opaque actual cell geometry avoids coincident surface transparency
        // artifacts. This disposable stage never consumes authored tools.
        auto visible=source.bodies;for(auto& b:visible)if(partition.toolBodyIds.contains(b.bodyId))b.visible=false;
        view_->setBodyStage(visible,"boundary-fill-cells:"+session_.value("id").toString());
        TopoDS_Compound shapes;BRep_Builder builder;builder.MakeCompound(shapes);
        for(const auto& cell:partition.cells)builder.Add(shapes,cell.shape);view_->showBoundaryFillPreview(shapes);
        for(const auto& row:rows){const auto cell=row.toObject();const auto point=cell.value("selectionPointMm").toArray();
            if(point.size()!=3)continue;
            auto* marker=new QCheckBox(view_);marker->setObjectName("boundaryFill.cellMarker."+cell.value("cellId").toString());
            marker->setProperty("cellId",cell.value("cellId"));marker->setProperty("positionMm",point);
            marker->setToolTip("Select this bounded cell");marker->setChecked(inputs.value("cell")==cell.value("cell"));
            marker->setStyleSheet("QCheckBox { background: #ffffff; border: 1px solid #62776a; border-radius: 3px; padding: 3px; }");
            marker->setFixedSize(24,24);connect(marker,&QCheckBox::clicked,this,[this,id=cell.value("cellId").toString()]{selectBoundaryFillCell(id);view_->setFocus(Qt::OtherFocusReason);});
            boundaryFillMarkers_.push_back(marker);marker->show();}
        refreshBoundaryFillMarkers();
        if(!inputs.contains("cell"))return reject("boundary_fill_cell_required","Activate a cell checkbox to select the new body",partition.diagnostics);
        const auto selected=BoundaryFill::applyCell(boundaryFillFeature(inputs),partition);
        session_["documentDraft"]=boundaryFillCandidate(inputs);session_["geometry"]=QJsonObject{
            {"cells",rows},{"diagnostics",partition.diagnostics},{"result",selected.summary}};
        session_["previewed"]=true;session_.remove("previewError");refreshBoundaryFillControls();return session_;
    }catch(const GeometryKernel::EvaluationError& error){const auto d=error.diagnostics();return reject(d.value("code").toString("evaluation_failed"),d.value("message").toString(),d);}
    catch(const std::exception& error){return reject("evaluation_failed",QString::fromUtf8(error.what()));}
}
QJsonObject AlloyWindow::selectBoundaryFillTool(const QString& bodyId){
    if(session_.value("command")!="surface.boundaryFill")return failure("invalid_session","Start Boundary Fill first");
    if(session_.value("baseDocumentRevision").toInteger()!=static_cast<qint64>(document_->revision()))return previewBoundaryFill();
    const EvaluatedBody* selected=nullptr;for(const auto& b:boundaryFillSourceStage().bodies)if(b.bodyId==bodyId && b.visible && b.occurrenceId.isEmpty()
        && b.ownerId==session_.value("ownerId") && b.summary.value("type")=="surface")selected=&b;
    if(!selected)return failure("boundary_fill_tool_unavailable","Select a visible local tool in the active modeling owner");
    auto inputs=session_.value("inputs").toObject();auto tools=inputs.value("tools").toArray();bool removed=false;
    for(int i=0;i<tools.size();++i)if(tools[i].toObject().value("bodyId")==bodyId){tools.removeAt(i);removed=true;break;}
    if(!removed)tools.append(QJsonObject{{"bodyId",bodyId},{"producerFeatureId",selected->featureId}});
    inputs["tools"]=tools;inputs.remove("cell");session_["inputs"]=inputs;session_["selectionRole"]="tools";
    return previewBoundaryFill();
}
QJsonObject AlloyWindow::selectBoundaryFillCells(){
    if(session_.value("command")!="surface.boundaryFill")return failure("invalid_session","Start Boundary Fill first");
    session_["selectionRole"]="cells";previewBoundaryFill();return session_;
}
QJsonObject AlloyWindow::selectBoundaryFillCell(const QString& cellId){
    if(session_.value("command")!="surface.boundaryFill" || session_.value("selectionRole")!="cells")return failure("invalid_session","Activate Select Cells first");
    if(session_.value("baseDocumentRevision").toInteger()!=static_cast<qint64>(document_->revision()))return previewBoundaryFill();
    QJsonObject reference;for(const auto& row:session_.value("cells").toArray())if(row.toObject().value("cellId")==cellId)reference=row.toObject().value("cell").toObject();
    if(reference.isEmpty())return failure("boundary_fill_cell_unavailable","Select an exact current bounded cell");
    auto inputs=session_.value("inputs").toObject();if(inputs.value("cell")==reference)inputs.remove("cell");else inputs["cell"]=reference;
    session_["inputs"]=inputs;previewBoundaryFill();return session_;
}
QJsonObject AlloyWindow::pickBoundaryFill(double x,double y){
    if(session_.value("selectionRole")=="cells")return failure("boundary_fill_cell_required","Use the checkbox on the desired bounded cell");
    std::vector<EvaluatedBody> eligible;for(const auto& b:boundaryFillSourceStage().bodies)if(b.visible && b.occurrenceId.isEmpty()
        && b.ownerId==session_.value("ownerId"))eligible.push_back(b);
    const auto hit=view_->pickSupportFace(x,y,eligible);if(hit.isEmpty())return failure("boundary_fill_tool_required","Click an enclosing surface tool");
    return selectBoundaryFillTool(hit.value("bodyId").toString());
}
void AlloyWindow::refreshBoundaryFillMarkers(){
    for(auto& pointer:boundaryFillMarkers_)if(auto* marker=pointer.data()){
        const auto p=view_->projectWorldPoint(marker->property("positionMm").toJsonArray());
        const QRect bounds(QPoint(qRound(p.x())-12,qRound(p.y())-12),QSize(24,24));
        if(marker->geometry()!=bounds)marker->setGeometry(bounds);
        marker->setVisible(session_.value("command")=="surface.boundaryFill" && view_->rect().intersects(bounds));marker->raise();}
}
void AlloyWindow::refreshBoundaryFillControls(){
    if(!boundaryFillDock_ || session_.value("command")!="surface.boundaryFill")return;
    const auto inputs=session_.value("inputs").toObject();
    auto* list=boundaryFillDock_->findChild<QListWidget*>("boundaryFill.tools");
    {const QSignalBlocker blocker(list);std::vector<const EvaluatedBody*> eligible;
        if(boundaryFillSource_)for(const auto& body:boundaryFillSource_->stage.bodies)if(body.visible && body.occurrenceId.isEmpty()
            && body.ownerId==session_.value("ownerId") && body.summary.value("type")=="surface")eligible.push_back(&body);
        bool rebuild=list->count()!=int(eligible.size());
        for(int i=0;!rebuild && i<list->count();++i)rebuild=list->item(i)->data(Qt::UserRole).toString()!=eligible[i]->bodyId;
        if(rebuild)list->clear();
        QMap<QString,int> nameCounts;for(const auto* body:eligible)++nameCounts[body->name];
        for(int i=0;i<int(eligible.size());++i){const auto& body=*eligible[i];QString label=body.name;
            bool profileLabel=false;
            for(const auto& row:document_->authored().value("features").toArray()){const auto feature=row.toObject();
                if(feature.value("id")!=body.featureId || feature.value("type")!="surfaceExtrude")continue;
                const auto results=SurfaceExtrude::resultBodyIds(feature);const auto profile=results.indexOf(body.bodyId);const auto name=feature.value("name").toString();
                if(results.size()>1 && profile>=0 && (body.name==name || body.name==name+" · "+body.bodyId.left(8) || nameCounts.value(body.name)>1)){
                    label=name+QString(" · Profile %1").arg(profile+1);profileLabel=true;}break;}
            if(!profileLabel && nameCounts.value(body.name)>1)label+=QString(" · Tool %1").arg(i+1);
            auto* item=rebuild?new QListWidgetItem(label,list):list->item(i);item->setText(label);item->setFlags(item->flags()|Qt::ItemIsUserCheckable);item->setData(Qt::UserRole,body.bodyId);item->setToolTip(body.bodyId);
            bool checked=false;for(const auto& value:inputs.value("tools").toArray())if(value.toObject().value("bodyId")==body.bodyId)checked=true;
            item->setCheckState(checked?Qt::Checked:Qt::Unchecked);}}
    const bool cells=session_.value("selectionRole")=="cells";
    boundaryFillDock_->findChild<QPushButton*>("boundaryFill.selectCells")->setEnabled(!inputs.value("tools").toArray().isEmpty());
    boundaryFillDock_->findChild<QPushButton*>("boundaryFill.selectCells")->setText(cells?"Select Cells · "+QString::number(inputs.contains("cell")?1:0)+" selected":"Select Cells");
    auto* remove=boundaryFillDock_->findChild<QCheckBox*>("boundaryFill.removeTools");{const QSignalBlocker b(remove);remove->setChecked(inputs.value("removeTools").toBool());}
    QString warning;const auto diagnostics=session_.value("geometry").toObject().value("diagnostics").toObject();warning=diagnostics.value("warnings").toString();
    auto* warnings=boundaryFillDock_->findChild<QLabel*>("boundaryFill.warnings");warnings->setText(warning.contains("UnableToOrientTheShape")
        ? (session_.value("previewed").toBool()?"Some boundary fragments could not be oriented; the selected solid passed validation.":"Some boundary fragments could not be oriented. Inspect the available cells before selecting.")
        : warning.trimmed().isEmpty()?QString{}:QString("Geometry warnings were reported. Inspect the available cells before accepting."));
    warnings->setToolTip(warning.trimmed());warnings->setVisible(!warning.trimmed().isEmpty());
    auto* error=boundaryFillDock_->findChild<QLabel*>("boundaryFill.error");error->setText(session_.value("previewError").toObject().value("message").toString());
    boundaryFillDock_->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->setEnabled(session_.value("previewed").toBool());
}
void AlloyWindow::showBoundaryFill(){
    if(boundaryFillDock_){canvasWindow_->removeDockWidget(boundaryFillDock_);delete boundaryFillDock_;}
    boundaryFillDock_=new QDockWidget("Boundary Fill",canvasWindow_);boundaryFillDock_->setObjectName("boundaryFillDock");
    auto* panel=new QWidget(boundaryFillDock_);boundaryFillDock_->setWidget(panel);auto* form=new QFormLayout(panel);
    auto* hint=new QLabel("Select enclosing tools, activate Select Cells, then check the desired bounded cell.",panel);hint->setWordWrap(true);form->addRow(hint);
    auto* tools=new QListWidget(panel);tools->setObjectName("boundaryFill.tools");tools->setMinimumHeight(120);form->addRow("Tools",tools);
    connect(tools,&QListWidget::itemChanged,panel,[this](QListWidgetItem* item){const auto id=item->data(Qt::UserRole).toString();selectBoundaryFillTool(id);view_->setFocus(Qt::OtherFocusReason);});
    auto* cells=new QPushButton("Select Cells",panel);cells->setObjectName("boundaryFill.selectCells");form->addRow(cells);
    connect(cells,&QPushButton::clicked,panel,[this]{execute("surface.boundaryFillSelectCells");view_->setFocus(Qt::OtherFocusReason);});
    auto* operation=new QComboBox(panel);operation->setObjectName("boundaryFill.operation");operation->addItem("New Body","newBody");operation->setEnabled(false);form->addRow("Operation",operation);
    auto* remove=new QCheckBox("Remove Tools",panel);remove->setObjectName("boundaryFill.removeTools");form->addRow(remove);
    connect(remove,&QCheckBox::clicked,panel,[this,remove]{auto inputs=session_.value("inputs").toObject();inputs["removeTools"]=remove->isChecked();session_["inputs"]=inputs;previewBoundaryFill();view_->setFocus(Qt::OtherFocusReason);});
    auto* warnings=new QLabel(panel);warnings->setObjectName("boundaryFill.warnings");warnings->setWordWrap(true);warnings->setStyleSheet("color: #8b5b12;");form->addRow(warnings);
    auto* error=new QLabel(panel);error->setObjectName("boundaryFill.error");error->setWordWrap(true);form->addRow(error);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,panel);buttons->button(QDialogButtonBox::Ok)->setEnabled(false);form->addRow(buttons);
    connect(buttons,&QDialogButtonBox::accepted,panel,[this]{execute("command.commit",{{"session",session_.value("id")}});});
    auto cancel=[this]{if(session_.value("command")=="surface.boundaryFill")execute("command.cancel",{{"session",session_.value("id")}});};
    connect(buttons,&QDialogButtonBox::rejected,panel,cancel);auto* escape=new QShortcut(QKeySequence(Qt::Key_Escape),panel);escape->setContext(Qt::WidgetWithChildrenShortcut);connect(escape,&QShortcut::activated,panel,cancel);
    if(!boundaryFillEscape_){boundaryFillEscape_=new QShortcut(QKeySequence(Qt::Key_Escape),view_);boundaryFillEscape_->setContext(Qt::WidgetShortcut);connect(boundaryFillEscape_,&QShortcut::activated,this,cancel);}
    boundaryFillEscape_->setEnabled(true);canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,boundaryFillDock_);boundaryFillDock_->show();
}
QJsonObject AlloyWindow::finishBoundaryFill(const QString& id,bool commit){
    if(session_.value("id")!=id)return failure("invalid_session","Boundary Fill is no longer active");
    if(commit){
        if(session_.value("baseDocumentRevision").toInteger()!=static_cast<qint64>(document_->revision()))return previewBoundaryFill();
        if(!session_.value("previewed").toBool())return failure("boundary_fill_cell_required","Select a valid bounded cell before accepting");
        const auto result=execute("surface.boundaryFill",session_.value("inputs").toObject());if(!result.value("ok").toBool()){
            session_["previewed"]=false;session_["previewError"]=result.value("error");refreshBoundaryFillControls();return result;}
    }
    clearBoundaryFillState();session_={};if(boundaryFillDock_)boundaryFillDock_->hide();if(boundaryFillEscape_)boundaryFillEscape_->setEnabled(false);
    view_->clearPreview();view_->clearBodyStage();view_->setFocus(Qt::OtherFocusReason);
    return {{"sessionId",id},{"status",commit?"committed":"cancelled"}};
}

void AlloyWindow::registerBoundaryFillCommands(){
    auto add=[this](QString id,QString title,QString description,QJsonObject fields,std::function<QJsonObject(const QJsonObject&)> run){
        commands_.push_back({id,title,description,fields,true,std::move(run)});};
    auto field=[](const QString& type,const QString& description,bool required=true){return QJsonObject{{"type",type},{"description",description},{"required",required}};};
    auto operation=field("string","Create one selected bounded cell as a new body.",false);operation["enum"]=QJsonArray{"newBody"};
    QJsonObject schema{{"operation",operation},{"tools",field("array","Earlier local tool references: bodyId and producerFeatureId.")},
        {"cell",field("object","Exact returned cell reference with retained sourceFaces.")},{"removeTools",field("boolean","Consume tool bodies after creating the selected solid; default false.",false)},
        {"featureId",field("string","Earlier Boundary Fill feature UUID to edit.",false)},{"name",field("string","Boundary Fill feature name.",false)}};
    add("surface.boundaryFill","Boundary Fill","Create one exact selected bounded cell from retained local tools.",schema,[this](const auto& args){
        QJsonObject candidate;try{auto inputs=args;if(!inputs.contains("operation"))inputs["operation"]="newBody";if(!inputs.contains("removeTools"))inputs["removeTools"]=false;candidate=boundaryFillCandidate(inputs);}
        catch(const std::exception& error){return failure("invalid_arguments",QString::fromUtf8(error.what()));}
        const auto error=document_->commit(candidate,"Boundary Fill");if(!error.isEmpty())return failure("evaluation_failed",error,document_->lastEvaluationError());return document_->state();});
    auto dialog=schema;for(auto it=dialog.begin();it!=dialog.end();++it){auto spec=it.value().toObject();spec["required"]=false;it.value()=spec;}
    add("surface.boundaryFillDialog","Boundary Fill","Select enclosing tools, activate Select Cells and select an actual bounded cell.",dialog,[this](const auto& args){return beginBoundaryFill(args);});
    add("surface.boundaryFillTool","Select Boundary Fill tool","Toggle one earlier visible local tool in the active Boundary Fill command.",
        {{"bodyId",field("string","Exact current tool body UUID.")}},[this](const auto& args){return selectBoundaryFillTool(args.value("bodyId").toString());});
    add("surface.boundaryFillSelectCells","Select Cells","Enumerate actual bounded cells from the selected tools; no cell is selected automatically.",{},[this](const auto&){return selectBoundaryFillCells();});
    add("surface.boundaryFillCell","Select bounded cell","Toggle the exact returned cell identity in the active Boundary Fill command.",
        {{"cellId",field("string","Exact current cellId returned after Select Cells.")}},[this](const auto& args){return selectBoundaryFillCell(args.value("cellId").toString());});
}
