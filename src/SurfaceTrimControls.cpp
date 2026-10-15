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
#include <QMap>
#include <cmath>

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
            if(!unique.isEmpty())return failure("trim_target_ambiguous","Choose the surface body before Trim; more than one is available");
            unique=body.bodyId;
        }
        if(unique.isEmpty())return failure("trim_target_required","Trim needs one current local two-face open sheet");
        inputs["targetBodyId"]=unique;
    }
    const auto error=validateArguments(*findCommand("surface.trimDialog"),inputs);
    if(!error.isEmpty())return failure("invalid_arguments",error);
    session_={{"id",CadDocument::newId()},{"command","surface.trim"},{"status","active"},
        {"inputs",inputs},{"newFeatureId",CadDocument::newId()},
        {"baseDocumentRevision",static_cast<qint64>(document_->revision())},{"previewed",false},
        {"selectionRole",(inputs.value("toolEntityIds").toArray().size()==2 || !inputs.value("toolBodyId").toString().isEmpty())?"region":"tool"},
        {"inputSchema",findCommand("surface.trim")->parameters}};
    showSurfaceTrim();previewSurfaceTrim();view_->setFocus(Qt::OtherFocusReason);return session_;
}

QJsonObject AlloyWindow::previewSurfaceTrim(){
    const auto inputs=session_["inputs"].toObject();
    session_["previewed"]=false;session_["geometryEvaluated"]=false;
    session_.remove("geometry");session_.remove("documentDraft");session_.remove("regions");surfaceTrimPickBodies_.clear();
    session_["partitionReused"]=false;
    view_->clearPreview();
    auto reject=[&](const QString& code,const QString& message,const QJsonObject& details=QJsonObject{}){
        auto response=failure(code,message);auto error=response["error"].toObject();error["diagnostics"]=details;
        response["error"]=error;session_["previewError"]=error;
        if(surfaceTrimDock_)if(auto* label=surfaceTrimDock_->findChild<QLabel*>("surfaceTrim.error"))label->setText(message);
        return response;
    };
    if(surfaceTrimDock_){
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
    const auto edit=inputs["featureId"].toString();
    const auto& stage=surfaceTrimSourceStage();
    // Selection and the display name cannot alter a partition. Everything
    // else is compared exactly, including ordered tool references and margin.
    auto geometryInputs=inputs;geometryInputs.remove("removedRegionIds");geometryInputs.remove("name");
    auto partitionKey=surfaceTrimSource_->key;
    partitionKey["featureId"]=edit.isEmpty()?session_.value("newFeatureId"):QJsonValue(edit);
    partitionKey["inputs"]=geometryInputs;
    if(surfaceTrimPartition_ && surfaceTrimPartition_->key!=partitionKey)surfaceTrimPartition_.reset();
    QSet<QString> visible;for(const auto& sketch:stage.sketches)if(sketch.visible)visible.insert(sketch.featureId);
    view_->setCommandSketches(visible);
    if(!edit.isEmpty())view_->setBodyStage(stage.bodies,"surface-trim-source:"+session_["id"].toString());
    if(inputs.contains("toolBodyId") ? inputs["toolBodyId"].toString().isEmpty()
        : (inputs["toolEntityIds"].toArray().size()!=2 || inputs["toolSketchId"].toString().isEmpty()))
        return reject("trim_tool_required","Select one crossing planar sheet tool or two connected fit splines, then pick regions");
    QJsonObject candidate;try{
        auto args=inputs;args["removedRegionIds"]=QJsonArray{CadDocument::newId()};
        candidate=surfaceTrimCandidate(args);
    }catch(const std::exception& e){return reject("invalid_arguments",QString::fromUtf8(e.what()));}
    const auto featureId=edit.isEmpty()?session_["newFeatureId"].toString():edit;
    QJsonObject feature;for(const auto& row:candidate["features"].toArray())if(row.toObject()["id"]==featureId){feature=row.toObject();break;}
    const auto bodyId=inputs["targetBodyId"].toString();
    const EvaluatedBody* target=nullptr;
    for(const auto& body:stage.bodies)if(body.bodyId==bodyId){target=&body;break;}
    if(!target)return reject("trim_target_required","Select one current two-face surface body");
    try{
        const bool reused=bool(surfaceTrimPartition_);
        if(!reused){
            session_["partitionBuildCount"]=session_.value("partitionBuildCount").toInteger()+1;
            surfaceTrimPartition_=std::make_shared<const SurfaceTrimPreviewPartition>(SurfaceTrimPreviewPartition{
                partitionKey,SurfaceTrim::split(feature,*target,stage)});
        }
        session_["partitionReused"]=reused;
        const auto& split=surfaceTrimPartition_->split;
        auto staged=*target;staged.shape=split.splitShape;staged.supportFaces=split.faces;
        surfaceTrimPickBodies_.push_back(std::move(staged));
        const auto regionRows=SurfaceTrim::descriptors(split);session_["regions"]=regionRows;
        if(surfaceTrimDock_)if(auto* list=surfaceTrimDock_->findChild<QListWidget*>("surfaceTrim.regions")){
            int regionIndex=0;
            for(const auto& row:regionRows){const auto item=row.toObject();
                if(item["removable"].toBool()){
                    ++regionIndex;
                    list->addItem(QString("%1 region · %2 mm² · %3").arg(item["side"].toString().isEmpty()?QString("Inner %1").arg(regionIndex):item["side"].toString())
                        .arg(item["areaMm2"].toDouble(),0,'f',2)
                        .arg(inputs["removedRegionIds"].toArray().contains(item["regionId"])?"remove":"keep"));
                    list->item(list->count()-1)->setToolTip(item["regionId"].toString());
                }
            }
        }
        const auto selected=inputs["removedRegionIds"].toArray();
        std::vector<TopoDS_Face> removed;
        for(const auto& region:split.regions)if(selected.contains(region.regionId)){
            if(!region.removable())return reject("trim_region_unsupported","Only inner regions can be removed");
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
        session_["documentDraft"]=candidate;session_["previewed"]=true;session_.remove("previewError");
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
    session_["inputs"]=inputs;session_["selectionRole"]=ids.size()==2?"region":"tool";
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
        && body.supportFaces.size()==1 && body.summary["type"]=="surface")found=true;
    if(!found)return failure("trim_tool_unavailable","Select a distinct visible earlier planar sheet tool");
    inputs.remove("toolSketchId");inputs["toolEntityIds"]=QJsonArray{};inputs["toolBodyId"]=bodyId;
    inputs["removedRegionIds"]=QJsonArray{};session_["inputs"]=inputs;session_["selectionRole"]="region";
    previewSurfaceTrim();return session_;
}

QJsonObject AlloyWindow::selectSurfaceTrimRegion(const QString& id){
    if(session_["command"]!="surface.trim")return failure("invalid_session","Start Trim first");
    if(session_["baseDocumentRevision"].toInteger()!=static_cast<qint64>(document_->revision()))return previewSurfaceTrim();
    bool found=false;for(const auto& value:session_["regions"].toArray())if(value.toObject()["regionId"]==id && value.toObject()["removable"].toBool())found=true;
    if(!found)return failure("trim_region_unavailable","Pick a current removable region of the split sheet");
    auto inputs=session_["inputs"].toObject();auto ids=inputs["removedRegionIds"].toArray();
    if(ids.contains(id)){for(int i=0;i<ids.size();++i)if(ids[i]==id){ids.removeAt(i);break;}}
    else {if(inputs.contains("toolBodyId"))ids=QJsonArray{};ids.append(id);}inputs["removedRegionIds"]=ids;session_["inputs"]=inputs;
    previewSurfaceTrim();return QJsonObject{{"removedRegionIds",ids},{"previewed",session_["previewed"]},
        {"previewError",session_.value("previewError")}};
}

QJsonObject AlloyWindow::pickSurfaceTrim(double x,double y){
    if(session_["baseDocumentRevision"].toInteger()!=static_cast<qint64>(document_->revision()))return previewSurfaceTrim();
    if(session_["selectionRole"]=="region"){
        const auto hit=view_->pickSupportFace(x,y,surfaceTrimPickBodies_);
        if(hit.isEmpty())return failure("trim_region_required","Click an inner split-surface region");
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
        if(hit.isEmpty())return failure("trim_tool_required","Click a visible fit spline or crossing planar sheet tool");
        return selectSurfaceTrimBody(hit["bodyId"].toString());
    }
    return selectSurfaceTrimEntity(sketchId,entityId);
}

void AlloyWindow::showSurfaceTrim(){
    if(surfaceTrimDock_){canvasWindow_->removeDockWidget(surfaceTrimDock_);delete surfaceTrimDock_;}
    surfaceTrimDock_=new QDockWidget("Trim",canvasWindow_);surfaceTrimDock_->setObjectName("surfaceTrimDock");
    auto* panel=new QWidget(surfaceTrimDock_);surfaceTrimDock_->setWidget(panel);
    auto* form=new QFormLayout(panel);form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    auto* hint=new QLabel("Select the tool, then click the target region to remove. For sheet tools, select one crossing planar surface body.",panel);
    hint->setWordWrap(true);form->addRow(hint);
    auto* tools=new QListWidget(panel);tools->setObjectName("surfaceTrim.tools");tools->setMinimumHeight(72);form->addRow("Tools",tools);
    auto* regions=new QListWidget(panel);regions->setObjectName("surfaceTrim.regions");regions->setMinimumHeight(80);form->addRow("Regions",regions);
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
