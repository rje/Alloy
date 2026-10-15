// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include "SketchGeometry.h"
#include "SurfaceExtrude.h"
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QShortcut>
#include <QSignalBlocker>
#include <QSet>
#include <cmath>

namespace {
QJsonObject failure(const QString& code,const QString& message){return {{"ok",false},{"error",QJsonObject{{"code",code},{"message",message}}}};}
}

QJsonObject AlloyWindow::surfaceExtrudeCandidate(const QJsonObject& inputs) const {
    auto args=inputs;
    if(!args.contains("featureId") && session_.value("command")=="surface.extrude"){
        args["newFeatureId"]=session_["newFeatureId"];if(args["resultMode"]!="perProfile")args["newBodyId"]=session_["newBodyId"];
    }
    args=authoringArguments(args);
    if(args.contains("featureId"))args.remove("ownerId");
    return CadDocument::surfaceExtrudeCandidate(document_->authored(),args);
}

QJsonObject AlloyWindow::beginSurfaceExtrude(const QJsonObject& overrides) {
    if(!session_.isEmpty())return failure("session_busy","Complete or cancel the active command first");
    QJsonObject inputs{{"resultMode","perProfile"},{"profiles",QJsonArray{}},{"sideOneMm",10.0},{"sideTwoMm",10.0},{"name","Extrude"}};
    if(overrides.contains("featureId")){
        bool found=false;
        for(const auto& entry:document_->authored()["features"].toArray()){
            const auto feature=entry.toObject();if(feature["id"]!=overrides["featureId"])continue;
            if(feature["type"]!="surfaceExtrude")return failure("stale_entity","Surface Extrude feature unavailable");
            inputs=feature["parameters"].toObject();inputs["featureId"]=feature["id"];inputs["name"]=feature["name"];found=true;break;
        }
        if(!found)return failure("stale_entity","Surface Extrude feature unavailable");
    }
    if(overrides.contains("entityIds") && !overrides.contains("featureId") && !overrides.contains("resultMode")){inputs.remove("resultMode");inputs.remove("profiles");}
    for(auto it=overrides.begin();it!=overrides.end();++it)inputs[it.key()]=it.value();
    const auto error=validateArguments(*findCommand("surface.extrudeDialog"),inputs);
    if(!error.isEmpty())return failure("invalid_arguments",error);
    session_={{"id",CadDocument::newId()},{"command","surface.extrude"},{"status","active"},{"inputs",inputs},
        {"newFeatureId",CadDocument::newId()},{"newBodyId",CadDocument::newId()},
        {"baseDocumentRevision",static_cast<qint64>(document_->revision())},{"previewed",false},
        {"geometryEvaluated",false},{"chaining",true},{"inputSchema",findCommand("surface.extrude")->parameters}};
    if(inputs.contains("featureId"))view_->setBodyStage(document_->evaluationBefore(inputs["featureId"].toString()).bodies,
        "surface-extrude-source:"+session_["id"].toString());
    showSurfaceExtrude();previewSurfaceExtrude();view_->setFocus(Qt::OtherFocusReason);return session_;
}

QJsonObject AlloyWindow::previewSurfaceExtrude() {
    const auto inputs=session_["inputs"].toObject();
    session_["previewed"]=false;session_["geometryEvaluated"]=false;
    session_.remove("geometry");session_.remove("documentDraft");session_.remove("bodyId");session_.remove("resultBodyIds");session_.remove("resultBodies");view_->clearPreview();view_->setEditDisplay({},{});
    QSet<QString> visible;
    for(const auto& sketch:document_->sketches())if(sketch.visible)visible.insert(sketch.featureId);
    view_->setCommandSketches(visible);
    if(surfaceExtrudeDock_){
        if(auto* list=surfaceExtrudeDock_->findChild<QListWidget*>("surfaceExtrude.entities")){
            const QSignalBlocker block(list);list->clear();
            if(inputs["resultMode"]=="perProfile"){
                for(const auto& value:inputs["profiles"].toArray()){
                    const auto members=value.toObject()["entityIds"].toArray();QStringList labels;for(const auto& id:members)labels.append(id.toString().left(8));
                    list->addItem(QString("Open profile · %1 curve(s) · %2").arg(members.size()).arg(labels.join(", ")));
                }
            }else for(const auto& value:inputs["entityIds"].toArray())list->addItem("Curve · "+value.toString().left(8));
        }
        for(const auto& key:{"sideOneMm","sideTwoMm"})if(auto* field=surfaceExtrudeDock_->findChild<QDoubleSpinBox*>(key)){
            const QSignalBlocker block(field);field->setValue(inputs[key].toDouble());
        }
        if(auto* buttons=surfaceExtrudeDock_->findChild<QDialogButtonBox*>())buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
    auto reject=[&](const QString& code,const QString& message,const QJsonObject& diagnostics=QJsonObject{}){
        auto result=failure(code,message);auto detail=result["error"].toObject();detail["diagnostics"]=diagnostics;
        result["error"]=detail;session_["previewError"]=detail;
        if(surfaceExtrudeDock_)if(auto* label=surfaceExtrudeDock_->findChild<QLabel*>("surfaceExtrude.error"))label->setText(message);
        return result;
    };
    if(session_["baseDocumentRevision"].toInteger()!=static_cast<qint64>(document_->revision()))
        return reject("stale_session","The document changed; cancel and restart Surface Extrude");
    if(inputs["sketchId"].toString().isEmpty() || (inputs["resultMode"]=="perProfile" ? inputs["profiles"].toArray().isEmpty() : inputs["entityIds"].toArray().isEmpty()))
        return reject("selection_required","Select one or more nonbranching open profiles");
    QJsonObject candidate;
    try{candidate=surfaceExtrudeCandidate(inputs);}catch(const std::exception& error){return reject("invalid_arguments",QString::fromUtf8(error.what()));}
    const auto evaluated=document_->prepareCandidate(candidate);
    if(!evaluated.ok)return reject("evaluation_failed",evaluated.error,evaluated.diagnostics);
    const auto featureId=inputs.value("featureId").toString(session_["newFeatureId"].toString());
    auto stage=evaluated.evaluation;
    if(inputs.contains("featureId")){
        auto prefix=candidate;QJsonArray features;
        for(const auto& row:candidate["features"].toArray()){features.append(row);if(row.toObject()["id"]==featureId)break;}
        prefix["features"]=features;const auto earlier=CadDocument::evaluateCandidate(prefix);
        if(!earlier.ok)return reject("evaluation_failed",earlier.error,earlier.diagnostics);
        stage=earlier.evaluation;
    }
    QJsonArray resultIds,resultBodies;TopoDS_Compound display;BRep_Builder builder;builder.MakeCompound(display);
    for(const auto& body:stage->bodies)if(body.featureId==featureId){
        builder.Add(display,body.shape);resultIds.append(body.bodyId);resultBodies.append(QJsonObject{{"bodyId",body.bodyId},{"geometry",body.summary}});
        if(inputs["resultMode"]!="perProfile"){session_["geometry"]=body.summary;session_["bodyId"]=body.bodyId;}
    }
    if(resultIds.isEmpty())return reject("surface_result_unavailable","Surface Extrude produced no current result bodies");
    // Display-only compound. Evaluated result bodies remain separately addressable.
    view_->showPreview(display,{},true);
    if(inputs.contains("featureId")){
        QJsonArray hidden;for(const auto& body:document_->bodies())if(body.featureId==featureId)hidden.append(body.bodyId);
        view_->setEditDisplay({},hidden);
    }
    session_["featureId"]=featureId;session_["resultBodyIds"]=resultIds;session_["resultBodies"]=resultBodies;
    session_["documentDraft"]=candidate;session_["previewed"]=true;session_["geometryEvaluated"]=true;
    session_.remove("previewError");
    if(surfaceExtrudeDock_){
        if(auto* label=surfaceExtrudeDock_->findChild<QLabel*>("surfaceExtrude.error"))label->clear();
        surfaceExtrudeDock_->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->setEnabled(true);
    }
    return session_;
}

QJsonObject AlloyWindow::selectSurfaceExtrudeEntity(const QString& sketchId,const QString& entityId){
    if(session_.value("command")!="surface.extrude")return failure("invalid_session","Start Surface Extrude first");
    const auto inputs=session_["inputs"].toObject();
    const auto edit=inputs["featureId"].toString();
    const auto stage=edit.isEmpty() ? document_->evaluation() : document_->evaluationBefore(edit);
    bool found=false;
    for(const auto& sketch:stage.sketches)if(sketch.featureId==sketchId && sketch.visible
        && sketch.entityEdges.contains(entityId))
        for(const auto& row:sketch.summary["entities"].toArray())if(row.toObject()["id"]==entityId
            && (row.toObject()["type"]=="line" || row.toObject()["type"]=="fitSpline")
            && !row.toObject()["construction"].toBool())found=true;
    if(!found)return failure("surface_source_unavailable","Select a visible ordinary open line or fit-spline curve");
    if(!inputs["sketchId"].toString().isEmpty() && inputs["sketchId"]!=sketchId)
        return failure("surface_sketch_mismatch","Selected curves must belong to one source sketch");
    if(inputs["resultMode"]=="perProfile"){
        for(const auto& profile:inputs["profiles"].toArray())if(profile.toObject()["entityIds"].toArray().contains(entityId))
            return selectSurfaceExtrudeProfile(sketchId,profile.toObject()["entityIds"].toArray());
        QJsonArray members{entityId};
        if(session_["chaining"].toBool())for(const auto& sketch:stage.sketches)if(sketch.featureId==sketchId){
            try{members=SurfaceExtrude::openChain(sketch,entityId);}catch(const std::exception& error){return failure("surface_profile_topology",QString::fromUtf8(error.what()));}}
        return selectSurfaceExtrudeProfile(sketchId,members);
    }
    auto next=inputs;auto entities=next["entityIds"].toArray();
    if(entities.contains(entityId)){for(int i=0;i<entities.size();++i)if(entities[i]==entityId){entities.removeAt(i);break;}}
    else entities.append(entityId);
    next["sketchId"]=sketchId;next["entityIds"]=entities;session_["inputs"]=next;
    previewSurfaceExtrude();return {{"sketchId",sketchId},{"entityId",entityId},{"entityIds",entities},
        {"previewed",session_["previewed"]},{"previewError",session_.value("previewError")}};
}

QJsonObject AlloyWindow::selectSurfaceExtrudeProfile(const QString& sketchId,const QJsonArray& entityIds){
    if(session_.value("command")!="surface.extrude" || session_["inputs"].toObject()["resultMode"]!="perProfile")return failure("invalid_session","Start per-profile Surface Extrude first");
    auto inputs=session_["inputs"].toObject();if(!inputs["sketchId"].toString().isEmpty() && inputs["sketchId"]!=sketchId)return failure("surface_sketch_mismatch","Profiles must belong to one source sketch");
    const auto edit=inputs["featureId"].toString();const auto stage=edit.isEmpty() ? document_->evaluation() : document_->evaluationBefore(edit);
    const EvaluatedSketch* source=nullptr;for(const auto& sketch:stage.sketches)if(sketch.featureId==sketchId && sketch.visible)source=&sketch;
    if(!source)return failure("surface_source_unavailable","Select a visible earlier solved sketch");
    // Validate exact authored group through the same kernel before changing the selection.
    auto probe=inputs;probe["sketchId"]=sketchId;probe["profiles"]=QJsonArray{QJsonObject{{"entityIds",entityIds}}};
    try{auto candidate=surfaceExtrudeCandidate(probe);auto feature=SketchGeometry::findFeature(candidate,probe.value("featureId").toString(session_["newFeatureId"].toString()));
        SurfaceExtrude::evaluateResults(feature,stage);
    }catch(const std::exception& error){return failure("surface_profile_topology",QString::fromUtf8(error.what()));}
    auto profiles=inputs["profiles"].toArray();const auto wanted=SurfaceExtrude::profileBodyId(session_["newFeatureId"].toString(),sketchId,entityIds);int existing=-1;
    QSet<QString> members;for(const auto& v:entityIds)members.insert(v.toString());
    for(int i=0;i<profiles.size();++i){const auto ids=profiles[i].toObject()["entityIds"].toArray();
        if(SurfaceExtrude::profileBodyId(session_["newFeatureId"].toString(),sketchId,ids)==wanted){existing=i;continue;}
        for(const auto& id:ids)if(members.contains(id.toString()))return failure("surface_profile_overlap","Selected chain overlaps an already authored profile; remove it explicitly first");}
    if(existing>=0)profiles.removeAt(existing);else profiles.append(QJsonObject{{"entityIds",entityIds}});
    inputs["sketchId"]=sketchId;inputs["profiles"]=profiles;session_["inputs"]=inputs;previewSurfaceExtrude();
    return {{"sketchId",sketchId},{"profiles",profiles},{"previewed",session_["previewed"]},{"previewError",session_.value("previewError")}};

}

QJsonObject AlloyWindow::pickSurfaceExtrude(double x,double y){
    const auto current=session_["inputs"].toObject();const auto edit=current["featureId"].toString();
    const auto stage=edit.isEmpty() ? document_->evaluation() : document_->evaluationBefore(edit);
    QString sketchId,entityId;double nearest=7;bool ambiguous=false;
    for(const auto& sketch:stage.sketches)if(sketch.visible && (current["sketchId"].toString().isEmpty() || current["sketchId"]==sketch.featureId)){
        auto source=SketchGeometry::findFeature(document_->authored(),sketch.featureId);
        auto parameters=source["parameters"].toObject();auto points=parameters["points"].toArray();
        const auto frame=sketch.resolvedFrame;
        const auto origin=frame["origin"].toArray(),u=frame["uAxis"].toArray(),v=frame["vAxis"].toArray();
        QMap<QString,QJsonArray> world;
        for(const auto& row:sketch.summary["worldPoints"].toArray())world[row.toObject()["id"].toString()]=row.toObject()["positionMm"].toArray();
        if(origin.size()!=3 || u.size()!=3 || v.size()!=3)continue;
        for(int i=0;i<points.size();++i){auto point=points[i].toObject();const auto xyz=world.value(point["id"].toString());if(xyz.size()!=3)continue;
            double a=0,b=0;for(int j=0;j<3;++j){const auto delta=xyz[j].toDouble()-origin[j].toDouble();a+=delta*u[j].toDouble();b+=delta*v[j].toDouble();}
            point["uMm"]=a;point["vMm"]=b;points[i]=point;}
        parameters["points"]=points;source["parameters"]=parameters;
        const auto hit=view_->pickSketch(source,x,y,"entity",frame);
        if(hit["kind"]!="entity" || hit["projected"].toBool())continue;
        bool supported=false;
        for(const auto& row:sketch.summary["entities"].toArray())if(row.toObject()["id"]==hit["entityId"]
            && (row.toObject()["type"]=="line" || row.toObject()["type"]=="fitSpline")
            && !row.toObject()["construction"].toBool())supported=true;
        if(!supported)continue;
        const auto distance=hit["distancePixels"].toDouble();
        if(distance<nearest-1e-6){nearest=distance;sketchId=sketch.featureId;entityId=hit["entityId"].toString();ambiguous=false;}
        else if(std::abs(distance-nearest)<1e-6)ambiguous=true;
    }
    if(ambiguous)return failure("ambiguous_surface_source","More than one open curve is under this point");
    if(sketchId.isEmpty())return failure("surface_source_required","Click a visible open line or fit-spline curve");
    return selectSurfaceExtrudeEntity(sketchId,entityId);
}

void AlloyWindow::showSurfaceExtrude(){
    if(surfaceExtrudeDock_){canvasWindow_->removeDockWidget(surfaceExtrudeDock_);delete surfaceExtrudeDock_;}
    surfaceExtrudeDock_=new QDockWidget("Extrude",canvasWindow_);surfaceExtrudeDock_->setObjectName("surfaceExtrudeDock");
    auto* panel=new QWidget(surfaceExtrudeDock_);surfaceExtrudeDock_->setWidget(panel);
    auto* form=new QFormLayout(panel);form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    const bool perProfile=session_["inputs"].toObject()["resultMode"]=="perProfile";
    auto* hint=new QLabel(perProfile
        ? "Select open profiles to create separate surface bodies. Chaining follows retained endpoint contacts. Two Sides measures independently from the sketch plane."
        : "Select open sketch curves to create one compound surface body. Two Sides measures independently from the sketch plane.",panel);
    hint->setWordWrap(true);form->addRow(hint);
    auto* name=new QLineEdit(session_["inputs"].toObject()["name"].toString(),panel);name->setObjectName("surfaceExtrude.name");form->addRow("Name",name);
    connect(name,&QLineEdit::editingFinished,panel,[this,name]{auto inputs=session_["inputs"].toObject();inputs["name"]=name->text();session_["inputs"]=inputs;previewSurfaceExtrude();});
    auto* list=new QListWidget(panel);list->setObjectName("surfaceExtrude.entities");list->setMinimumHeight(100);form->addRow(session_["inputs"].toObject()["resultMode"]=="perProfile" ? "Profiles" : "Curves",list);
    auto* remove=new QPushButton("Remove Selected",panel);remove->setObjectName("surfaceExtrude.remove");form->addRow(remove);
    connect(remove,&QPushButton::clicked,panel,[this,list]{const auto index=list->currentRow();auto inputs=session_["inputs"].toObject();const auto key=inputs["resultMode"]=="perProfile" ? "profiles" : "entityIds";auto refs=inputs[key].toArray();
        if(index<0 || index>=refs.size())return;refs.removeAt(index);inputs[key]=refs;session_["inputs"]=inputs;previewSurfaceExtrude();});
    if(session_["inputs"].toObject()["resultMode"]=="perProfile"){
        auto* chaining=new QCheckBox("Chaining",panel);chaining->setObjectName("surfaceExtrude.chaining");chaining->setChecked(session_["chaining"].toBool());form->addRow(chaining);
        connect(chaining,&QCheckBox::toggled,panel,[this](bool checked){session_["chaining"]=checked;});
    }
    form->addRow("Direction",new QLabel("Two Sides",panel));form->addRow("Operation",new QLabel("New Body",panel));
    for(const auto& key:{"sideOneMm","sideTwoMm"}){
        auto* field=new QDoubleSpinBox(panel);field->setObjectName(key);field->setRange(.001,100000);field->setDecimals(3);field->setSuffix(" mm");
        field->setValue(session_["inputs"].toObject()[key].toDouble());form->addRow(QString::fromLatin1(key)=="sideOneMm"?"Side 1 · −normal":"Side 2 · +normal",field);
        connect(field,qOverload<double>(&QDoubleSpinBox::valueChanged),panel,[this,key](double value){auto inputs=session_["inputs"].toObject();inputs[key]=value;session_["inputs"]=inputs;previewSurfaceExtrude();});
    }
    auto* error=new QLabel(panel);error->setObjectName("surfaceExtrude.error");error->setWordWrap(true);form->addRow(error);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,panel);form->addRow(buttons);
    connect(buttons,&QDialogButtonBox::accepted,panel,[this,error]{const auto response=execute("command.commit",{{"session",session_["id"]}});
        if(!response["ok"].toBool())error->setText(response["error"].toObject()["message"].toString());});
    auto cancel=[this]{if(session_["command"]=="surface.extrude")execute("command.cancel",{{"session",session_["id"]}});};
    connect(buttons,&QDialogButtonBox::rejected,panel,cancel);
    auto* escape=new QShortcut(QKeySequence(Qt::Key_Escape),panel);escape->setContext(Qt::WidgetWithChildrenShortcut);
    connect(escape,&QShortcut::activated,panel,cancel);
    if(!surfaceExtrudeEscape_){surfaceExtrudeEscape_=new QShortcut(QKeySequence(Qt::Key_Escape),view_);
        surfaceExtrudeEscape_->setContext(Qt::WidgetShortcut);connect(surfaceExtrudeEscape_,&QShortcut::activated,this,cancel);}
    surfaceExtrudeEscape_->setEnabled(true);
    canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,surfaceExtrudeDock_);surfaceExtrudeDock_->show();
}
