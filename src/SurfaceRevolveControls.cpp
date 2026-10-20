// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QListWidget>
#include <BRepAdaptor_Curve.hxx>
#include <QLineEdit>
#include <QPushButton>
#include <QShortcut>
#include <QSignalBlocker>
#include <algorithm>

namespace {
QJsonObject failure(const QString& code,const QString& message){return {{"ok",false},{"error",QJsonObject{{"code",code},{"message",message}}}};}
}
QJsonObject AlloyWindow::surfaceRevolveCandidate(const QJsonObject& inputs) const {
    auto args=inputs;
    if(!args.contains("featureId") && session_.value("command")=="surface.revolve"){
        args["newFeatureId"]=session_.value("newFeatureId");args["newBodyId"]=session_.value("newBodyId");
    }
    args=authoringArguments(args);if(args.contains("featureId"))args.remove("ownerId");
    return CadDocument::surfaceRevolveCandidate(document_->authored(),args);
}
QJsonObject AlloyWindow::beginSurfaceRevolve(const QJsonObject& overrides) {
    if(!session_.isEmpty())return failure("session_busy","Complete or cancel the active command first");
    QJsonObject inputs{{"angleDeg",90.},{"reverseDirection",false},{"name","Revolve"}};
    if(overrides.contains("featureId")){
        bool found=false;for(const auto& value:document_->authored().value("features").toArray()){
            const auto f=value.toObject();if(f.value("id")!=overrides.value("featureId"))continue;
            if(f.value("type")!="surfaceRevolve")return failure("stale_entity","Surface Revolve feature unavailable");
            const auto p=f.value("parameters").toObject();
            for(const auto& key:{"sketchId","entityId","entityIds","axisEntityId","angleDeg","reverseDirection"})if(p.contains(key))inputs[key]=p.value(key);
            inputs["featureId"]=f.value("id");inputs["name"]=f.value("name");found=true;break;
        }
        if(!found)return failure("stale_entity","Surface Revolve feature unavailable");
    }
    for(auto i=overrides.begin();i!=overrides.end();++i)inputs[i.key()]=i.value();
    const auto error=validateArguments(*findCommand("surface.revolveDialog"),inputs);if(!error.isEmpty())return failure("invalid_arguments",error);
    session_={{"id",CadDocument::newId()},{"command","surface.revolve"},{"status","active"},{"inputs",inputs},
        {"newFeatureId",CadDocument::newId()},{"newBodyId",CadDocument::newId()},
        {"baseDocumentRevision",static_cast<qint64>(document_->revision())},{"previewed",false},{"geometryEvaluated",false},
        {"inputSchema",findCommand("surface.revolve")->parameters},{"nativeControlsAvailable",true}};
    showSurfaceRevolve();previewSurfaceRevolve();return session_;
}
QJsonObject AlloyWindow::previewSurfaceRevolve() {
    if(session_.value("command")!="surface.revolve")return failure("invalid_session","Surface Revolve is not active");
    session_["previewed"]=false;session_["geometryEvaluated"]=false;session_.remove("geometry");session_.remove("documentDraft");view_->clearPreview();view_->clearSweepPath();
    auto reject=[&](const QString& code,const QString& message,const QJsonObject& details=QJsonObject{}){
        auto result=failure(code,message);auto error=result.value("error").toObject();error["diagnostics"]=details;
        result["error"]=error;session_["previewError"]=error;refreshSurfaceRevolveControls();return result;
    };
    if(session_.value("baseDocumentRevision").toInteger()!=static_cast<qint64>(document_->revision()))return reject("stale_session","The document changed; cancel and restart Surface Revolve");
    const auto inputs=session_.value("inputs").toObject();const auto edit=inputs.value("featureId").toString();
    const auto source=edit.isEmpty() ? document_->evaluation() : document_->evaluationBefore(edit);
    if(!edit.isEmpty())view_->setBodyStage(source.bodies,"surface-revolve-source:"+session_.value("id").toString());
    const auto sketchId=inputs.value("sketchId").toString();view_->setCommandSketches(QSet<QString>{sketchId});
    for(const auto& sketch:source.sketches)if(sketch.featureId==sketchId)
        {auto selected=inputs.value("entityIds").toArray();if(selected.isEmpty())selected.append(inputs.value("entityId"));selected.append(inputs.value("axisEntityId"));view_->showSweepPath(sketch,selected);}
    if(sketchId.isEmpty() || (inputs.value("entityId").toString().isEmpty() && inputs.value("entityIds").toArray().isEmpty()) || inputs.value("axisEntityId").toString().isEmpty())
        return reject("selection_required","Select an earlier sketch, one ordinary Line and its construction Line axis");
    QJsonObject candidate;try{candidate=surfaceRevolveCandidate(inputs);}catch(const std::exception& e){return reject("invalid_arguments",QString::fromUtf8(e.what()));}
    const auto evaluated=CadDocument::evaluateCandidate(candidate);if(!evaluated.ok)return reject("evaluation_failed",evaluated.error,evaluated.diagnostics);
    auto stage=evaluated.evaluation;const auto id=edit.isEmpty() ? session_.value("newFeatureId").toString() : edit;
    if(!edit.isEmpty()){
        auto prefix=candidate;QJsonArray rows;for(const auto& value:candidate.value("features").toArray()){rows.append(value);if(value.toObject().value("id")==id)break;}prefix["features"]=rows;
        const auto early=CadDocument::evaluateCandidate(prefix);if(!early.ok)return reject("evaluation_failed",early.error,early.diagnostics);stage=early.evaluation;
    }
    for(const auto& body:stage->bodies)if(body.featureId==id){view_->showPreview(body.shape,edit.isEmpty() ? QString{}:body.bodyId,true);
        session_["geometry"]=body.summary;session_["featureId"]=id;session_["bodyId"]=body.bodyId;}
    session_["documentDraft"]=candidate;session_["previewed"]=true;session_["geometryEvaluated"]=true;
    session_.remove("previewError");refreshSurfaceRevolveControls();return session_;
}
void AlloyWindow::refreshSurfaceRevolveControls(){
    if(!surfaceRevolveDock_ || session_.value("command")!="surface.revolve")return;
    const auto inputs=session_.value("inputs").toObject();const auto id=inputs.value("featureId").toString();
    const auto source=id.isEmpty() ? document_->evaluation() : document_->evaluationBefore(id);
    for(const auto& role:{"sketchId","entityId","axisEntityId"}){
        auto* list=surfaceRevolveDock_->findChild<QComboBox*>("surfaceRevolve."+QString(role));const QSignalBlocker blocker(list);
        list->clear();list->addItem("Select",QString{});
        for(const auto& sketch:source.sketches){
            if(sketch.ownerId!=document_->activeOwnerId() && sketch.featureId!=inputs.value("sketchId").toString())continue;
            if(QString(role)=="sketchId")list->addItem(sketch.name+" · "+sketch.featureId.left(8),sketch.featureId);
            else if(sketch.featureId==inputs.value("sketchId").toString())for(const auto& value:sketch.summary.value("entities").toArray()){
                const auto e=value.toObject();if(e.value("type")!="line" || e.value("construction").toBool()!=(QString(role)=="axisEntityId"))continue;
                list->addItem((QString(role)=="axisEntityId" ? "Construction Line · ":"Line · ")+e.value("id").toString().left(8),e.value("id").toString());
            }
        }
        list->setCurrentIndex(std::max(0,list->findData(inputs.value(role).toString())));
    }
    auto* chain=surfaceRevolveDock_->findChild<QListWidget*>("surfaceRevolve.entityIds");const QSignalBlocker chainBlock(chain);chain->clear();
    for(const auto& sketch:source.sketches)if(sketch.featureId==inputs.value("sketchId").toString())for(const auto& value:sketch.summary.value("entities").toArray()){
        const auto e=value.toObject();if(e.value("type")!="line" || e.value("construction").toBool())continue;
        auto* item=new QListWidgetItem("Line · "+e.value("id").toString().left(8),chain);item->setData(Qt::UserRole,e.value("id").toString());item->setSelected(inputs.value("entityIds").toArray().contains(e.value("id")));}
    QStringList selected;for(const auto& value:inputs.value("entityIds").toArray())selected.append(value.toString().left(8));
    surfaceRevolveDock_->findChild<QLabel*>("surfaceRevolve.orderedSources")->setText(selected.join(" → "));
    auto* angle=surfaceRevolveDock_->findChild<QDoubleSpinBox*>("surfaceRevolve.angle");const QSignalBlocker a(angle);angle->setValue(inputs.value("angleDeg").toDouble());
    auto* reverse=surfaceRevolveDock_->findChild<QCheckBox*>("surfaceRevolve.reverse");const QSignalBlocker b(reverse);reverse->setChecked(inputs.value("reverseDirection").toBool());
    surfaceRevolveDock_->findChild<QLabel*>("surfaceRevolve.error")->setText(session_.value("previewError").toObject().value("message").toString());
    surfaceRevolveDock_->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->setEnabled(session_.value("previewed").toBool());
}
void AlloyWindow::showSurfaceRevolve(){
    if(surfaceRevolveDock_){canvasWindow_->removeDockWidget(surfaceRevolveDock_);delete surfaceRevolveDock_;}
    surfaceRevolveDock_=new QDockWidget("Revolve",canvasWindow_);surfaceRevolveDock_->setObjectName("surfaceRevolveDock");
    auto* panel=new QWidget(surfaceRevolveDock_);surfaceRevolveDock_->setWidget(panel);auto* form=new QFormLayout(panel);
    const auto sessionId=session_.value("id").toString();
    auto update=[this,sessionId](const QJsonObject& inputs){if(session_.value("id")!=sessionId)return;execute("command.update",{{"session",sessionId},{"inputs",inputs}});};
    auto* name=new QLineEdit(session_.value("inputs").toObject().value("name").toString());name->setObjectName("surfaceRevolve.name");form->addRow("Name",name);
    connect(name,&QLineEdit::editingFinished,panel,[update,name]{update({{"name",name->text()}});});
    for(const auto& role:{"sketchId","entityId","axisEntityId"}){
        auto* list=new QComboBox;list->setObjectName("surfaceRevolve."+QString(role));list->setMinimumContentsLength(22);list->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
        form->addRow(QString(role)=="sketchId" ? "Sketch" : QString(role)=="entityId" ? "Profile":"Axis",list);
        connect(list,&QComboBox::activated,panel,[this,list,role,update]{
            if(QString(role)=="sketchId"){auto inputs=session_.value("inputs").toObject();inputs.remove("entityId");inputs.remove("entityIds");inputs.remove("axisEntityId");session_["inputs"]=inputs;}
            if(QString(role)=="entityId"){auto inputs=session_.value("inputs").toObject();inputs.remove("entityIds");session_["inputs"]=inputs;}
            update({{role,list->currentData().toString()}});});
    }
    auto* chain=new QListWidget;chain->setObjectName("surfaceRevolve.entityIds");chain->setSelectionMode(QAbstractItemView::ExtendedSelection);chain->setMaximumHeight(110);form->addRow("Profile chain",chain);
    auto* ordered=new QLabel;ordered->setObjectName("surfaceRevolve.orderedSources");ordered->setWordWrap(true);form->addRow("Ordered sources",ordered);
    connect(chain,&QListWidget::itemSelectionChanged,panel,[this,chain,update]{
        auto inputs=session_.value("inputs").toObject();QJsonArray ids;for(auto* item:chain->selectedItems())ids.append(item->data(Qt::UserRole).toString());
        const auto edit=inputs.value("featureId").toString();const auto source=edit.isEmpty()?document_->evaluation():document_->evaluationBefore(edit);
        for(const auto& sketch:source.sketches)if(sketch.featureId==inputs.value("sketchId").toString() && ids.size()>1){
            auto endpoints=[&](const QJsonValue& id){BRepAdaptor_Curve c(sketch.entityEdges.value(id.toString()));return std::pair{c.Value(c.FirstParameter()),c.Value(c.LastParameter())};};
            QJsonArray ordered;gp_Pnt current;bool found=false;
            for(const auto& id:ids){auto [a,b]=endpoints(id);for(auto point:{a,b}){int count=0;for(const auto& other:ids){auto [u,v]=endpoints(other);if(point.Distance(u)<1e-10 || point.Distance(v)<1e-10)++count;}if(count==1){current=point;found=true;break;}}if(found)break;}
            if(found){auto remaining=ids;while(!remaining.isEmpty()){bool next=false;for(int i=0;i<remaining.size();++i){auto [a,b]=endpoints(remaining[i]);if(current.Distance(a)<1e-10 || current.Distance(b)<1e-10){current=current.Distance(a)<1e-10?b:a;ordered.append(remaining[i]);remaining.removeAt(i);next=true;break;}}if(!next)break;}if(remaining.isEmpty())ids=ordered;}
        }
        inputs.remove("entityId");session_["inputs"]=inputs;update({{"entityIds",ids}});
    });
    form->addRow("Extent Type",new QLabel("Partial"));form->addRow("Direction",new QLabel("One Side"));
    auto* angle=new QDoubleSpinBox;angle->setObjectName("surfaceRevolve.angle");angle->setDecimals(3);angle->setRange(.001,359.999);angle->setSuffix(" °");angle->setKeyboardTracking(false);form->addRow("Angle",angle);
    connect(angle,&QDoubleSpinBox::valueChanged,panel,[update,angle]{update({{"angleDeg",angle->value()}});});
    auto* reverse=new QCheckBox("Reverse");reverse->setObjectName("surfaceRevolve.reverse");form->addRow(reverse);
    connect(reverse,&QCheckBox::toggled,panel,[update](bool value){update({{"reverseDirection",value}});});
    form->addRow("Operation",new QLabel("New Body"));
    auto* hint=new QLabel("One open straight Line, or select two or more joined Lines in Profile chain (Command/Ctrl-click). A distinct construction Line is the axis. Direction follows the axis start to end. The sheet has no caps.");hint->setWordWrap(true);form->addRow(hint);
    auto* error=new QLabel;error->setObjectName("surfaceRevolve.error");error->setWordWrap(true);error->setStyleSheet("color:#b3261e");form->addRow(error);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);form->addRow(buttons);
    connect(buttons,&QDialogButtonBox::accepted,panel,[this,sessionId]{if(session_.value("id")==sessionId)execute("command.commit",{{"session",sessionId}});});
    connect(buttons,&QDialogButtonBox::rejected,panel,[this,sessionId]{if(session_.value("id")==sessionId)execute("command.cancel",{{"session",sessionId}});});
    if(!surfaceRevolveEscape_){surfaceRevolveEscape_=new QShortcut(QKeySequence(Qt::Key_Escape),this);connect(surfaceRevolveEscape_,&QShortcut::activated,this,[this]{if(session_.value("command")=="surface.revolve")execute("command.cancel",{{"session",session_.value("id")}});});}
    surfaceRevolveEscape_->setEnabled(true);canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,surfaceRevolveDock_);surfaceRevolveDock_->show();refreshSurfaceRevolveControls();
}
