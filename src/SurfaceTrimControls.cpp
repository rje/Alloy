// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include "SurfaceTrim.h"
#include "SketchGeometry.h"
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QFormLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QShortcut>
#include <QSignalBlocker>
#include <QDateTime>
#include <QElapsedTimer>
#include <QStatusBar>
#include <QMap>
#include <cmath>
#include <algorithm>

namespace {
QJsonObject failure(const QString& code,const QString& message){return {{"ok",false},{"error",QJsonObject{{"code",code},{"message",message}}}};}
}

struct AlloyWindow::SurfaceTrimPreviewSource {
    QJsonObject key;
    EvaluatedDocument stage;
};

struct AlloyWindow::SurfaceTrimPreviewPartition {
    QJsonObject key;
    SurfaceTrimSplit split;
};

const EvaluatedDocument& AlloyWindow::surfaceTrimSourceStage(){
    const QJsonObject key{{"sessionId",session_.value("id")},
        {"documentId",document_->authored().value("documentId")},
        {"revision",static_cast<qint64>(document_->revision())},
        {"featureId",session_.value("inputs").toObject().value("featureId")}};
    if(!surfaceTrimSource_ || surfaceTrimSource_->key!=key){
        surfaceTrimSource_.reset();surfaceTrimPartition_.reset();
        const auto edit=key.value("featureId").toString();
        surfaceTrimSource_=std::make_shared<const SurfaceTrimPreviewSource>(SurfaceTrimPreviewSource{
            key,edit.isEmpty()?document_->evaluation():document_->evaluationBefore(edit)});
    }
    return surfaceTrimSource_->stage;
}

QJsonObject AlloyWindow::surfaceTrimCandidate(const QJsonObject& inputs) const {
    auto args=inputs;
    args.remove("sourcePlane");args.remove("sourceNormal");args.remove("projectionMode");
    if(!args.contains("featureId") && session_.value("command")=="surface.trim")
        args["newFeatureId"]=session_["newFeatureId"];
    if(args.contains("featureId"))args.remove("ownerId");
    else {
        QString owner;
        for(const auto& body:document_->bodies())if(body.bodyId==args["targetBodyId"] && body.occurrenceId.isEmpty())
            owner=body.ownerId;
        if(owner.isEmpty())throw std::invalid_argument("Select one current local surface body");
        args["ownerId"]=owner;
    }
    return CadDocument::surfaceTrimCandidate(document_->authored(),args);
}

QJsonObject AlloyWindow::beginSurfaceTrim(const QJsonObject& overrides){
    if(!session_.isEmpty())return failure("session_busy","Complete or cancel the active command first");
    surfaceTrimSource_.reset();surfaceTrimPartition_.reset();
    QJsonObject inputs{{"toolEntityIds",QJsonArray{}},{"removedRegionIds",QJsonArray{}},
        {"throughMarginMm",10.0},{"name","Trim"}};
    if(overrides.contains("featureId")){
        bool found=false;
        for(const auto& value:document_->authored()["features"].toArray()){
            const auto feature=value.toObject();if(feature["id"]!=overrides["featureId"])continue;
            if(feature["type"]!="surfaceTrim")return failure("stale_entity","Surface Trim feature unavailable");
            inputs=feature["parameters"].toObject();inputs["featureId"]=feature["id"];
            inputs["name"]=feature["name"];found=true;break;
        }
        if(!found)return failure("stale_entity","Surface Trim feature unavailable");
    }
    for(auto it=overrides.begin();it!=overrides.end();++it)inputs[it.key()]=it.value();
    inputs.remove("sourcePlane");inputs.remove("sourceNormal");inputs.remove("projectionMode");
    if(!overrides.contains("featureId") && !overrides.contains("targetBodyId")){
        const auto selected=selectedBody_.isEmpty()?selectedFace_.value("bodyId").toString():selectedBody_;
        if(!selected.isEmpty())inputs["targetBodyId"]=selected;
    }
    if(inputs.value("targetBodyId").toString().isEmpty()){
        QString unique;
        for(const auto& body:document_->bodies())if(body.summary["type"]=="surface" &&
            body.summary["topology"].toObject()["solids"].toInt()==0 && body.supportFaces.size()==2 && body.occurrenceId.isEmpty()){
            if(!unique.isEmpty()){unique.clear();break;}
            unique=body.bodyId;
        }
        if(!unique.isEmpty())inputs["targetBodyId"]=unique;
    }
    const auto error=validateArguments(*findCommand("surface.trimDialog"),inputs);
    if(!error.isEmpty())return failure("invalid_arguments",error);
    session_={{"id",CadDocument::newId()},{"command","surface.trim"},{"status","active"},
        {"inputs",inputs},{"newFeatureId",CadDocument::newId()},
        {"baseDocumentRevision",static_cast<qint64>(document_->revision())},{"previewed",false},
        {"selectionRole",(inputs.value("toolEntityIds").toArray().size()==2 || !inputs.value("toolBodyId").toString().isEmpty())
            ?(inputs.value("targetBodyId").toString().isEmpty()?"target":"region"):"tool"},
        {"inputSchema",findCommand("surface.trim")->parameters}};
    showSurfaceTrim();previewSurfaceTrim(true);view_->setFocus(Qt::OtherFocusReason);return session_;
}

QJsonObject AlloyWindow::previewSurfaceTrim(bool asynchronous){
    if(session_.value("evaluationStatus")=="pending")return failure("evaluation_busy","Trim evaluation is pending; wait or Cancel");
    session_.remove("evaluationStatus");
    const auto inputs=session_["inputs"].toObject();
    session_["previewed"]=false;session_["geometryEvaluated"]=false;
    session_.remove("geometry");session_.remove("documentDraft");session_.remove("regions");surfaceTrimPickBodies_.clear();
    session_["partitionReused"]=false;
    view_->clearPreview();
    auto reject=[&](const QString& code,const QString& message,const QJsonObject& details=QJsonObject{}){
        auto response=failure(code,message);auto error=response["error"].toObject();error["diagnostics"]=details;
        response["error"]=error;session_["previewError"]=error;session_["evaluationStatus"]="failed";
        if(surfaceTrimDock_)if(auto* label=surfaceTrimDock_->findChild<QLabel*>("surfaceTrim.error"))label->setText(message);
        return response;
    };
    if(surfaceTrimDock_){
        if(auto* label=surfaceTrimDock_->findChild<QLabel*>("surfaceTrim.target")){
            QString name;for(const auto& b:document_->bodies())if(b.bodyId==inputs.value("targetBodyId"))name=b.name;
            label->setText(name.isEmpty()?"Select target surface":name);label->setToolTip(inputs.value("targetBodyId").toString());}
        if(auto* list=surfaceTrimDock_->findChild<QListWidget*>("surfaceTrim.tools")){
            const QSignalBlocker block(list);list->clear();
            QJsonObject source;
            for(const auto& row:document_->authored()["features"].toArray())
                if(row.toObject()["id"]==inputs["toolSketchId"]) {source=row.toObject();break;}
            const auto entities=source["parameters"].toObject()["entities"].toArray();
            if(!inputs["toolBodyId"].toString().isEmpty())
                for(const auto& body:document_->bodies())if(body.bodyId==inputs["toolBodyId"]){
                    list->addItem(body.name);list->item(0)->setToolTip(body.bodyId);break;}
            int selectedIndex=0;
            for(const auto& value:inputs["toolEntityIds"].toArray()){
                ++selectedIndex;int curveIndex=0;
                for(int i=0;i<entities.size();++i)if(entities[i].toObject()["id"]==value){curveIndex=i+1;break;}
                list->addItem(QString("Tool %1 · %2 · spline %3").arg(selectedIndex)
                    .arg(source["name"].toString("Sketch")).arg(curveIndex));
                list->item(list->count()-1)->setToolTip(value.toString());
            }
        }
        if(auto* list=surfaceTrimDock_->findChild<QListWidget*>("surfaceTrim.regions")){
            const QSignalBlocker block(list);list->clear();
        }
        if(auto* buttons=surfaceTrimDock_->findChild<QDialogButtonBox*>())buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
    if(session_["baseDocumentRevision"].toInteger()!=static_cast<qint64>(document_->revision())){
        surfaceTrimSource_.reset();surfaceTrimPartition_.reset();
        return reject("stale_session","The document changed; cancel and restart Trim");
    }
    const auto edit=inputs.value("featureId").toString();
    auto geometryInputs=inputs;geometryInputs.remove("removedRegionIds");geometryInputs.remove("name");
    const QJsonObject sourceKey{{"sessionId",session_.value("id")},{"documentId",document_->authored().value("documentId")},
        {"revision",static_cast<qint64>(document_->revision())},{"featureId",inputs.value("featureId")}};
    auto partitionKey=sourceKey;partitionKey["featureId"]=edit.isEmpty()?session_.value("newFeatureId"):QJsonValue(edit);
    partitionKey["inputs"]=geometryInputs;
    if(surfaceTrimPartition_ && surfaceTrimPartition_->key!=partitionKey)surfaceTrimPartition_.reset();
    const bool hasTools=inputs.contains("toolBodyId") ? !inputs.value("toolBodyId").toString().isEmpty()
        : inputs.value("toolEntityIds").toArray().size()==2 && !inputs.value("toolSketchId").toString().isEmpty();
    if(asynchronous && ((hasTools && !inputs.value("targetBodyId").toString().isEmpty() && !surfaceTrimPartition_)
        || (!edit.isEmpty() && (!surfaceTrimSource_ || surfaceTrimSource_->key!=sourceKey)))){
        QJsonObject feature;
        if(hasTools && !inputs.value("targetBodyId").toString().isEmpty())try{
            auto args=inputs;args["removedRegionIds"]=QJsonArray{CadDocument::newId()};
            const auto candidate=surfaceTrimCandidate(args);const auto fid=partitionKey.value("featureId");
            for(const auto& row:candidate.value("features").toArray())if(row.toObject().value("id")==fid){feature=row.toObject();break;}
        }catch(const std::exception& e){return reject("invalid_arguments",QString::fromUtf8(e.what()));}
        return queueSurfaceTrimEvaluation("partition",partitionKey,feature);
    }
    const auto& stage=surfaceTrimSourceStage();
    QSet<QString> visible;for(const auto& sketch:stage.sketches)if(sketch.visible)visible.insert(sketch.featureId);
    view_->setCommandSketches(visible);
    if(!edit.isEmpty())view_->setBodyStage(stage.bodies,"surface-trim-source:"+session_["id"].toString());
    if(inputs.contains("toolBodyId") ? inputs["toolBodyId"].toString().isEmpty()
        : (inputs["toolEntityIds"].toArray().size()!=2 || inputs["toolSketchId"].toString().isEmpty()))
        return reject("trim_tool_required","Select one crossing sheet tool or two connected fit splines, then pick a target region");
    if(inputs.value("targetBodyId").toString().isEmpty())
        return reject("trim_target_required","Click the target surface portion");
    QJsonObject candidate;try{
        auto args=inputs;args["removedRegionIds"]=QJsonArray{CadDocument::newId()};
        candidate=surfaceTrimCandidate(args);
    }catch(const std::exception& e){return reject("invalid_arguments",QString::fromUtf8(e.what()));}
    const auto featureId=edit.isEmpty()?session_["newFeatureId"].toString():edit;
    QJsonObject feature;for(const auto& row:candidate["features"].toArray())if(row.toObject()["id"]==featureId){feature=row.toObject();break;}
    const auto bodyId=inputs["targetBodyId"].toString();
    const EvaluatedBody* target=nullptr;
    for(const auto& body:stage.bodies)if(body.bodyId==bodyId){target=&body;break;}
    if(!target)return reject("trim_target_required","Select one current local surface body");
    try{
        const bool reused=bool(surfaceTrimPartition_);
        if(!reused){
            // Keep native target binding + region selection in one gesture.
            // Only dialog-open partitions and final commits use the worker.
            session_["partitionBuildCount"]=session_.value("partitionBuildCount").toInteger()+1;
            surfaceTrimPartition_=std::make_shared<const SurfaceTrimPreviewPartition>(SurfaceTrimPreviewPartition{
                partitionKey,SurfaceTrim::split(feature,*target,stage)});
        }
        session_["partitionReused"]=reused;
        const auto& split=surfaceTrimPartition_->split;
        auto staged=*target;staged.shape=split.splitShape;staged.supportFaces=split.faces;staged.solidEdges=split.edges;
        surfaceTrimPickBodies_.push_back(std::move(staged));
        const auto regionRows=SurfaceTrim::descriptors(split);session_["regions"]=regionRows;
        if(surfaceTrimDock_)if(auto* list=surfaceTrimDock_->findChild<QListWidget*>("surfaceTrim.regions")){
            int regionIndex=0;
            for(const auto& row:regionRows){const auto item=row.toObject();
                if(item["removable"].toBool()){
                    ++regionIndex;
                    list->addItem(QString("%1 region · %2 mm² · %3").arg(item["side"].toString().isEmpty()?QString("Inner %1").arg(regionIndex):item["side"]=="portion"?QString("Portion %1").arg(regionIndex):item["side"].toString())
                        .arg(item["areaMm2"].toDouble(),0,'f',2)
                        .arg(inputs["removedRegionIds"].toArray().contains(item["regionId"])?"remove":"keep"));
                    list->item(list->count()-1)->setToolTip(item["regionId"].toString());
                    list->item(list->count()-1)->setData(Qt::UserRole,item["regionId"]);
                }
            }
        }
        const auto selected=inputs["removedRegionIds"].toArray();
        std::vector<TopoDS_Face> removed;
        for(const auto& region:split.regions)if(selected.contains(region.regionId)){
            if(!region.removable())return reject("trim_region_unsupported","Select a removable split portion");
            removed.push_back(region.face);
        }
        view_->showSurfaceTrimPreview(split.splitShape,bodyId,removed);
        session_["geometry"]=QJsonObject{{"regions",regionRows},{"projection",split.projection}};
        session_["geometryEvaluated"]=true;
        if(selected.isEmpty())return reject("trim_region_required","Pick a removable region; selected regions preview red");
        candidate=surfaceTrimCandidate(inputs);
        for(const auto& row:candidate["features"].toArray())if(row.toObject()["id"]==featureId){feature=row.toObject();break;}
        auto selectedBody=*target;
        SurfaceTrim::applySelection(selectedBody,feature,split);
        // Preview validates this Trim's actual selected geometry. The ordinary
        // document transaction validates all downstream features once on OK.
        session_["geometry"]=QJsonObject{{"regions",regionRows},{"projection",split.projection},
            {"result",selectedBody.summary}};
        session_["documentDraft"]=candidate;session_["previewed"]=true;session_["evaluationStatus"]="completed";session_.remove("previewError");
        if(surfaceTrimDock_){surfaceTrimDock_->findChild<QLabel*>("surfaceTrim.error")->clear();
            surfaceTrimDock_->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->setEnabled(true);}
        return session_;
    }catch(const GeometryKernel::EvaluationError& e){
        const auto detail=e.diagnostics();return reject(detail["code"].toString("evaluation_failed"),detail["message"].toString(),detail);
    }catch(const std::exception& e){return reject("evaluation_failed",QString::fromUtf8(e.what()));}
}

QJsonObject AlloyWindow::selectSurfaceTrimEntity(const QString& sketchId,const QString& entityId){
    if(session_["command"]!="surface.trim")return failure("invalid_session","Start Trim first");
    if(session_["baseDocumentRevision"].toInteger()!=static_cast<qint64>(document_->revision()))return previewSurfaceTrim();
    auto inputs=session_["inputs"].toObject();
    const auto& stage=surfaceTrimSourceStage();
    bool found=false;for(const auto& sketch:stage.sketches)if(sketch.featureId==sketchId && sketch.visible && sketch.entityEdges.contains(entityId))
        for(const auto& value:sketch.summary["entities"].toArray())if(value.toObject()["id"]==entityId &&
            value.toObject()["type"]=="fitSpline" && !value.toObject()["construction"].toBool())found=true;
    if(!found)return failure("trim_tool_unavailable","Select a visible ordinary fit spline");
    if(!inputs["toolSketchId"].toString().isEmpty() && inputs["toolSketchId"]!=sketchId)
        return failure("trim_sketch_mismatch","Both tool splines must belong to one sketch");
    auto ids=inputs["toolEntityIds"].toArray();
    if(ids.contains(entityId)){for(int i=0;i<ids.size();++i)if(ids[i]==entityId){ids.removeAt(i);break;}}
    else {if(ids.size()==2)return failure("trim_tool_limit","Select exactly two connected fit splines");ids.append(entityId);}
    inputs.remove("toolBodyId");
    inputs["toolSketchId"]=sketchId;inputs["toolEntityIds"]=ids;inputs["removedRegionIds"]=QJsonArray{};
    session_["inputs"]=inputs;session_["selectionRole"]=ids.size()==2?(inputs.value("targetBodyId").toString().isEmpty()?"target":"region"):"tool";
    previewSurfaceTrim();return QJsonObject{{"toolSketchId",sketchId},{"toolEntityIds",ids},
        {"selectionRole",session_["selectionRole"]},{"previewError",session_.value("previewError")}};
}

QJsonObject AlloyWindow::selectSurfaceTrimBody(const QString& bodyId){
    if(session_["command"]!="surface.trim")return failure("invalid_session","Start Trim first");
    if(session_["baseDocumentRevision"].toInteger()!=static_cast<qint64>(document_->revision()))return previewSurfaceTrim();
    auto inputs=session_["inputs"].toObject();
    const auto& stage=surfaceTrimSourceStage();
    bool found=false;for(const auto& body:stage.bodies)if(body.bodyId==bodyId && body.visible
        && body.occurrenceId.isEmpty() && bodyId!=inputs["targetBodyId"].toString()
        && !body.supportFaces.empty() && body.summary["type"]=="surface"
        && std::all_of(body.supportFaces.begin(),body.supportFaces.end(),[](const auto& f){return f.role=="surfaceExtrude.sheet";}))found=true;
    if(!found)return failure("trim_tool_unavailable","Select a distinct visible earlier Surface Extrude sheet tool");
    inputs.remove("toolSketchId");inputs["toolEntityIds"]=QJsonArray{};inputs["toolBodyId"]=bodyId;
    inputs["removedRegionIds"]=QJsonArray{};session_["inputs"]=inputs;session_["selectionRole"]=inputs.value("targetBodyId").toString().isEmpty()?"target":"region";
    previewSurfaceTrim();return session_;
}

QJsonObject AlloyWindow::selectSurfaceTrimTarget(const QString& bodyId){
    if(session_["command"]!="surface.trim")return failure("invalid_session","Start Trim first");
    if(session_["baseDocumentRevision"].toInteger()!=static_cast<qint64>(document_->revision()))return previewSurfaceTrim();
    auto inputs=session_.value("inputs").toObject();const auto& stage=surfaceTrimSourceStage();
    const EvaluatedBody* target=nullptr;const EvaluatedBody* tool=nullptr;
    for(const auto& b:stage.bodies){if(b.bodyId==bodyId)target=&b;if(b.bodyId==inputs.value("toolBodyId"))tool=&b;}
    if(!target || !target->visible || !target->occurrenceId.isEmpty() || target->summary.value("type")!="surface"
        || target->bodyId==inputs.value("toolBodyId"))return failure("trim_target_required","Select a distinct visible local target surface");
    if(tool && tool->ownerId!=target->ownerId)return failure("cross_component_dependency","Tool and target must share a modeling owner");
    inputs["targetBodyId"]=bodyId;inputs["removedRegionIds"]=QJsonArray{};session_["inputs"]=inputs;
    session_["selectionRole"]=(tool || inputs.value("toolEntityIds").toArray().size()==2)?"region":"tool";
    previewSurfaceTrim();return session_;
}

QJsonObject AlloyWindow::selectSurfaceTrimRegion(const QString& id){
    if(session_["command"]!="surface.trim")return failure("invalid_session","Start Trim first");
    if(session_["baseDocumentRevision"].toInteger()!=static_cast<qint64>(document_->revision()))return previewSurfaceTrim();
    bool found=false;for(const auto& value:session_["regions"].toArray())if(value.toObject()["regionId"]==id && value.toObject()["removable"].toBool())found=true;
    if(!found)return failure("trim_region_unavailable","Pick a current removable region of the split sheet");
    auto inputs=session_["inputs"].toObject();auto ids=inputs["removedRegionIds"].toArray();
    if(ids.contains(id)){for(int i=0;i<ids.size();++i)if(ids[i]==id){ids.removeAt(i);break;}}
    else {if(inputs.contains("toolBodyId") && surfaceTrimPartition_ && surfaceTrimPartition_->split.projection.value("normal").isArray())ids=QJsonArray{};ids.append(id);}inputs["removedRegionIds"]=ids;session_["inputs"]=inputs;
    previewSurfaceTrim();return QJsonObject{{"removedRegionIds",ids},{"previewed",session_["previewed"]},
        {"previewError",session_.value("previewError")}};
}

QJsonObject AlloyWindow::pickSurfaceTrim(double x,double y){
    if(session_["baseDocumentRevision"].toInteger()!=static_cast<qint64>(document_->revision()))return previewSurfaceTrim();
    if(session_["selectionRole"]=="target"){
        std::vector<EvaluatedBody> candidates;const auto inputs=session_.value("inputs").toObject();
        for(const auto& b:surfaceTrimSourceStage().bodies)if(b.visible && b.occurrenceId.isEmpty()
            && b.summary.value("type")=="surface" && b.bodyId!=inputs.value("toolBodyId"))candidates.push_back(b);
        const auto hit=view_->pickSupportFace(x,y,candidates);
        if(hit.isEmpty())return failure("trim_target_required","Click the target surface portion");
        const auto bound=selectSurfaceTrimTarget(hit.value("bodyId").toString());
        if(!bound.value("ok").toBool(true))return bound;
        // Reuse the same real hit against the newly partitioned target.
        if(session_.value("regions").toArray().isEmpty())return session_;
        const auto boundary=view_->pickSolidEdge(x,y,surfaceTrimPickBodies_);
        if(!boundary.isEmpty() && boundary.value("distancePx").toDouble()<2)
            return failure("ambiguous_trim_region","Click inside a portion, away from its boundary");
        const auto region=view_->pickSupportFace(x,y,surfaceTrimPickBodies_);
        if(region.isEmpty())return failure("trim_region_required","Pick a removable split portion");
        return selectSurfaceTrimRegion(region.value("faceId").toString());
    }
    if(session_["selectionRole"]=="region"){
        const auto boundary=view_->pickSolidEdge(x,y,surfaceTrimPickBodies_);
        if(!boundary.isEmpty() && boundary.value("distancePx").toDouble()<2)
            return failure("ambiguous_trim_region","Click inside a portion, away from its boundary");
        const auto hit=view_->pickSupportFace(x,y,surfaceTrimPickBodies_);
        if(hit.isEmpty())return failure("trim_region_required","Click a removable portion on the bound target");
        return selectSurfaceTrimRegion(hit["faceId"].toString());
    }
    const auto inputs=session_["inputs"].toObject();
    const auto& stage=surfaceTrimSourceStage();
    QString sketchId,entityId;double nearest=7;bool ambiguous=false;
    for(const auto& sketch:stage.sketches)if(sketch.visible && (inputs["toolSketchId"].toString().isEmpty() || inputs["toolSketchId"]==sketch.featureId)){
        auto source=SketchGeometry::findFeature(document_->authored(),sketch.featureId);
        auto p=source["parameters"].toObject();auto points=p["points"].toArray();const auto frame=sketch.resolvedFrame;
        const auto origin=frame["origin"].toArray(),u=frame["uAxis"].toArray(),v=frame["vAxis"].toArray();
        if(origin.size()!=3||u.size()!=3||v.size()!=3)continue;
        QMap<QString,QJsonArray> world;for(const auto& row:sketch.summary["worldPoints"].toArray())world[row.toObject()["id"].toString()]=row.toObject()["positionMm"].toArray();
        for(int i=0;i<points.size();++i){auto point=points[i].toObject();const auto xyz=world.value(point["id"].toString());if(xyz.size()!=3)continue;
            double a=0,b=0;for(int j=0;j<3;++j){const auto d=xyz[j].toDouble()-origin[j].toDouble();a+=d*u[j].toDouble();b+=d*v[j].toDouble();}
            point["uMm"]=a;point["vMm"]=b;points[i]=point;}
        p["points"]=points;source["parameters"]=p;
        const auto hit=view_->pickSketch(source,x,y,"entity",frame);
        if(hit["kind"]!="entity" || hit["projected"].toBool())continue;
        bool supported=false;for(const auto& row:sketch.summary["entities"].toArray())if(row.toObject()["id"]==hit["entityId"] &&
            row.toObject()["type"]=="fitSpline" && !row.toObject()["construction"].toBool())supported=true;
        if(!supported)continue;const auto distance=hit["distancePixels"].toDouble();
        if(distance<nearest-1e-6){nearest=distance;sketchId=sketch.featureId;entityId=hit["entityId"].toString();ambiguous=false;}
        else if(std::abs(distance-nearest)<1e-6)ambiguous=true;
    }
    if(ambiguous)return failure("ambiguous_trim_tool","More than one spline is under this point");
    if(sketchId.isEmpty()){
        std::vector<EvaluatedBody> candidates;
        for(const auto& body:stage.bodies)if(body.visible && body.bodyId!=inputs["targetBodyId"] && body.occurrenceId.isEmpty())candidates.push_back(body);
        const auto hit=view_->pickSupportFace(x,y,candidates);
        if(hit.isEmpty())return failure("trim_tool_required","Click a visible fit spline or crossing sheet tool");
        return selectSurfaceTrimBody(hit["bodyId"].toString());
    }
    return selectSurfaceTrimEntity(sketchId,entityId);
}

void AlloyWindow::showSurfaceTrim(){
    if(surfaceTrimDock_){canvasWindow_->removeDockWidget(surfaceTrimDock_);delete surfaceTrimDock_;}
    surfaceTrimDock_=new QDockWidget("Trim",canvasWindow_);surfaceTrimDock_->setObjectName("surfaceTrimDock");
    auto* panel=new QWidget(surfaceTrimDock_);surfaceTrimDock_->setWidget(panel);
    auto* form=new QFormLayout(panel);form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    auto* hint=new QLabel("Select the tool, then click the target portion to remove. Sheet tools use one earlier Surface Extrude body.",panel);
    hint->setWordWrap(true);form->addRow(hint);
    auto* tools=new QListWidget(panel);tools->setObjectName("surfaceTrim.tools");tools->setMinimumHeight(72);form->addRow("Tools",tools);
    auto* targetLabel=new QLabel(panel);targetLabel->setObjectName("surfaceTrim.target");form->addRow("Target",targetLabel);
    auto* changeTarget=new QPushButton("Change Target",panel);form->addRow(changeTarget);
    connect(changeTarget,&QPushButton::clicked,panel,[this]{auto inputs=session_.value("inputs").toObject();inputs.remove("targetBodyId");
        inputs["removedRegionIds"]=QJsonArray{};session_["inputs"]=inputs;session_["selectionRole"]="target";previewSurfaceTrim();view_->setFocus(Qt::OtherFocusReason);});
    auto* regions=new QListWidget(panel);regions->setObjectName("surfaceTrim.regions");regions->setMinimumHeight(80);form->addRow("Regions",regions);
    connect(regions,&QListWidget::itemClicked,panel,[this](QListWidgetItem* item){
        const auto id=item->data(Qt::UserRole).toString();selectSurfaceTrimRegion(id);view_->setFocus(Qt::OtherFocusReason);});
    auto* change=new QPushButton("Change Tools",panel);form->addRow(change);
    connect(change,&QPushButton::clicked,panel,[this]{auto inputs=session_["inputs"].toObject();inputs.remove("toolSketchId");inputs["toolEntityIds"]=QJsonArray{};
        inputs.remove("toolBodyId");
        inputs["removedRegionIds"]=QJsonArray{};session_["inputs"]=inputs;session_["selectionRole"]="tool";previewSurfaceTrim();view_->setFocus(Qt::OtherFocusReason);});
    auto* error=new QLabel(panel);error->setObjectName("surfaceTrim.error");error->setWordWrap(true);form->addRow(error);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,panel);form->addRow(buttons);
    connect(buttons,&QDialogButtonBox::accepted,panel,[this,error]{const auto response=execute("command.commit",{{"session",session_["id"]}});
        if(!response["ok"].toBool())error->setText(response["error"].toObject()["message"].toString());});
    auto cancel=[this]{if(session_["command"]=="surface.trim")execute("command.cancel",{{"session",session_["id"]}});};
    connect(buttons,&QDialogButtonBox::rejected,panel,cancel);
    auto* escape=new QShortcut(QKeySequence(Qt::Key_Escape),panel);escape->setContext(Qt::WidgetWithChildrenShortcut);
    connect(escape,&QShortcut::activated,panel,cancel);
    if(!surfaceTrimEscape_){surfaceTrimEscape_=new QShortcut(QKeySequence(Qt::Key_Escape),view_);
        surfaceTrimEscape_->setContext(Qt::WidgetShortcut);connect(surfaceTrimEscape_,&QShortcut::activated,this,cancel);}
    surfaceTrimEscape_->setEnabled(true);
    canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,surfaceTrimDock_);surfaceTrimDock_->show();
}

void AlloyWindow::refreshSurfaceTrimPending(){
    const bool pending=session_.value("evaluationStatus")=="pending";
    if(surfaceTrimDock_){
        for(auto* list:surfaceTrimDock_->findChildren<QListWidget*>())list->setEnabled(!pending);
        auto* buttons=surfaceTrimDock_->findChild<QDialogButtonBox*>();
        for(auto* button:surfaceTrimDock_->findChildren<QPushButton*>())
            button->setEnabled(!pending || (buttons && button==buttons->button(QDialogButtonBox::Cancel)));
        if(buttons)buttons->button(QDialogButtonBox::Ok)->setEnabled(!pending && session_.value("previewed").toBool());
        if(auto* label=surfaceTrimDock_->findChild<QLabel*>("surfaceTrim.error"))label->setText(pending
            ? "Evaluating Trim… You can navigate or Cancel." : session_.value("previewError").toObject().value("message").toString());
    }
    if(pending){status_->hide();statusBar()->showMessage("Evaluating Trim… You can navigate or Cancel to discard the result.");}
    else {status_->show();if(statusBar()->currentMessage().startsWith("Evaluating Trim"))statusBar()->clearMessage();}
}

QJsonObject AlloyWindow::queueSurfaceTrimEvaluation(const QString& phase,const QJsonObject& key,const QJsonObject& feature){
    if(session_.value("evaluationStatus")=="pending")return phase=="commit" && session_.value("evaluationPhase")=="commit"
        ? session_ : failure("evaluation_pending","Wait for Trim regions before accepting");
    if(session_.value("baseDocumentRevision").toInteger()!=static_cast<qint64>(document_->revision()))
        return failure("stale_session","The document changed; cancel and restart Trim");
    if(phase=="commit" && !session_.value("previewed").toBool())return failure("trim_region_required","Select valid removable portions before accepting");
    if(geometryEvaluationRunning_ || geometryEvaluationQueued_)return failure("evaluation_busy","Background geometry is still finishing");
    auto task=std::make_shared<GeometryEvaluation>();task->command="surface.trim";task->phase=phase;
    task->document=document_;task->documentId=document_->authored().value("documentId").toString();task->tabId=activeTabId_;
    task->base=document_->authored();task->revision=document_->revision();task->sessionId=session_.value("id").toString();
    task->generation=++geometryEvaluationGeneration_;task->inputs=session_.value("inputs").toObject();task->ownerId=document_->activeOwnerId();
    task->featureId=task->inputs.value("featureId").toString(session_.value("newFeatureId").toString());task->feature=feature;task->cacheKey=key;
    if(phase=="commit")try{task->candidate=surfaceTrimCandidate(task->inputs);}catch(const std::exception& e){return failure("invalid_arguments",QString::fromUtf8(e.what()));}
    else if(!feature.isEmpty())session_["partitionBuildCount"]=session_.value("partitionBuildCount").toInteger()+1;
    geometryEvaluationTerminal_={};documentOpenOperation_={};sketchEvaluationOperation_={};geometryEvaluationQueued_=task;
    session_["evaluationPhase"]=phase;session_["evaluationStatus"]="pending";session_["previewGeneration"]=static_cast<qint64>(task->generation);
    session_["evaluationHeartbeat"]=QJsonObject{{"ticks",0},{"lastMs",QDateTime::currentMSecsSinceEpoch()},{"maxGapMs",0}};
    session_["cancellation"]="Discard result; running kernel evaluation continues";session_.remove("previewError");
    refreshSurfaceTrimPending();startGeometryEvaluation();return session_;
}

void AlloyWindow::evaluateSurfaceTrimPartition(GeometryEvaluation& task){
    // Serial worker only: source and partition have fresh independent topology.
    QElapsedTimer timer;timer.start();
    try{
        auto prefix=task.base;const auto edit=task.inputs.value("featureId").toString();
        if(!edit.isEmpty()){
            QJsonArray features;for(const auto& row:prefix.value("features").toArray()){
                if(row.toObject().value("id")==edit)break;features.append(row);}
            prefix["features"]=features;prefix.remove("objectOwners");prefix.remove("bodyNames");
        }
        const QJsonObject sourceKey{{"sessionId",task.sessionId},{"documentId",task.documentId},
            {"revision",static_cast<qint64>(task.revision)},{"featureId",task.inputs.value("featureId")}};
        auto source=std::make_shared<const SurfaceTrimPreviewSource>(SurfaceTrimPreviewSource{sourceKey,GeometryKernel::evaluateDocument(prefix)});
        task.sourceMs=timer.restart();
        if(!task.feature.isEmpty()){
            const EvaluatedBody* target=nullptr;for(const auto& body:source->stage.bodies)if(body.bodyId==task.inputs.value("targetBodyId"))target=&body;
            if(!target)throw GeometryKernel::EvaluationError({{"code","trim_target_required"},{"message","Select one current local surface body"}});
            task.trimPartition=std::make_shared<const SurfaceTrimPreviewPartition>(SurfaceTrimPreviewPartition{
                task.cacheKey,SurfaceTrim::split(task.feature,*target,source->stage)});
        }
        task.stageMs=timer.elapsed();task.trimSource=std::move(source);
    }catch(const GeometryKernel::EvaluationError& e){task.full.diagnostics=e.diagnostics();task.error=QString::fromUtf8(e.what());}
}

void AlloyWindow::completeSurfaceTrimEvaluation(const std::shared_ptr<GeometryEvaluation>& task){
    status_->show();if(statusBar()->currentMessage().startsWith("Evaluating Trim"))statusBar()->clearMessage();
    if(session_.value("id")!=task->sessionId || session_.value("command")!=task->command || geometryEvaluationGeneration_!=task->generation)return;
    auto reject=[&](const QString& code,const QString& message,const QJsonObject& diagnostics=QJsonObject{}){
        session_["evaluationStatus"]="failed";session_["previewed"]=false;
        session_["previewError"]=QJsonObject{{"code",code},{"message",message},{"diagnostics",diagnostics}};refreshSurfaceTrimPending();
    };
    if(document_!=task->document || activeTabId_!=task->tabId || document_->revision()!=task->revision || document_->authored()!=task->base
        || session_.value("inputs").toObject()!=task->inputs || document_->activeOwnerId()!=task->ownerId
        || task->featureId!=task->inputs.value("featureId").toString(session_.value("newFeatureId").toString())){
        reject("stale_session","The document or Trim selection changed; cancel and restart");return;
    }
    session_["evaluationTimingMs"]=QJsonObject{{"source",task->sourceMs},{"partition",task->stageMs},{"full",task->fullMs}};
    if(!task->error.isEmpty()){reject(task->full.diagnostics.value("code").toString("evaluation_failed"),task->error,task->full.diagnostics);return;}
    if(task->phase=="partition"){
        if(!task->trimSource){reject("evaluation_failed","Trim source is unavailable");return;}
        surfaceTrimSource_=task->trimSource;surfaceTrimPartition_=task->trimPartition;
        session_.remove("evaluationStatus");const auto preview=previewSurfaceTrim();
        session_["evaluationPhase"]="partition";session_["previewGeneration"]=static_cast<qint64>(task->generation);session_["partitionReused"]=false;
        const auto detail=preview.value("error").toObject();const auto code=detail.value("code").toString();
        const bool awaitingSelection=(code=="trim_region_required" && !session_.value("regions").toArray().isEmpty())
            || (task->feature.isEmpty() && (code=="trim_tool_required" || code=="trim_target_required"));
        if(preview.value("ok").isBool() && !preview.value("ok").toBool() && !awaitingSelection)
            reject(code,detail.value("message").toString(),detail.value("diagnostics").toObject());
        else session_["evaluationStatus"]="completed";
        refreshSurfaceTrimPending();refresh();return;
    }
    if(!task->full.ok){reject("evaluation_failed",task->full.error,task->full.diagnostics);return;}
    const auto error=document_->commitSurfaceTrimEvaluation(task->full,task->candidate,task->base,task->revision);
    if(!error.isEmpty()){reject("evaluation_failed",error,document_->lastEvaluationError());return;}
    QJsonObject terminal{{"id",task->sessionId},{"sessionId",task->sessionId},{"command",task->command},
        {"previewGeneration",static_cast<qint64>(task->generation)},{"evaluationStatus","completed"},{"status","committed"},
        {"evaluationTimingMs",session_.value("evaluationTimingMs")},{"evaluationHeartbeat",session_.value("evaluationHeartbeat")},
        {"commandResult",QJsonObject{{"ok",true},{"completion","completed"},{"result",document_->state()}}}};
    session_["evaluationStatus"]="completed";finishSession(task->sessionId,false);refresh();
    const auto recoveryError=syncRecovery();terminal["recovery"]=QJsonObject{{"durable",recoveryError.isEmpty()},{"warning",recoveryError}};
    geometryEvaluationTerminal_=terminal;
}
