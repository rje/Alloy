// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include "SketchGeometry.h"
#include <QDockWidget>
#include <QFormLayout>
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
#include <limits>
namespace {
QJsonObject failure(const QString& code,const QString& message){return {{"ok",false},{"error",QJsonObject{{"code",code},{"message",message}}}};}
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
    auto inputs=QJsonObject{{"profiles",QJsonArray{}},{"operation","newBody"},{"name","Loft"}};
    if(overrides.contains("featureId")) {
        bool found=false;for(const auto& entry:document_->authored()["features"].toArray()) {const auto f=entry.toObject();if(f["id"]!=overrides["featureId"])continue;if(f["type"]!="loft")return failure("stale_entity","Loft feature unavailable");inputs=f["parameters"].toObject();inputs["name"]=f["name"];inputs["featureId"]=f["id"];if(inputs.value("operation")=="newComponent")for(const auto& component:document_->authored()["components"].toArray())if(component.toObject()["id"]==f["ownerId"])inputs["componentName"]=component.toObject()["name"];found=true;}
        if(!found)return failure("stale_entity","Loft feature unavailable");
    } else if(!selectedProfile_.isEmpty())inputs["profiles"]=QJsonArray{QJsonObject{{"sketchId",selectedSketch_},{"profileId",selectedProfile_}}};
    for(auto it=overrides.begin();it!=overrides.end();++it)inputs[it.key()]=it.value();
    if(inputs.value("operation")=="newComponent"){
        if(!inputs.contains("sections"))inputs["sections"]=QJsonArray{QJsonObject{},QJsonObject{}};
        if(!inputs.contains("componentName"))inputs["componentName"]="Component";
    }
    const auto error=validateArguments(*findCommand("solid.loft"),inputs);if(!error.isEmpty())return failure("invalid_arguments",error);
    session_={{"id",CadDocument::newId()},{"command","solid.loft"},{"status","active"},{"inputs",inputs},
        {"newFeatureId",CadDocument::newId()},{"newBodyId",CadDocument::newId()},
        {"newComponentId",CadDocument::newId()},{"newOccurrenceId",CadDocument::newId()},
        {"selectionRole","section1"},{"baseDocumentRevision",static_cast<qint64>(document_->revision())},{"previewed",false},{"geometryEvaluated",false},{"inputSchema",findCommand("solid.loft")->parameters}};
    if(inputs.contains("featureId"))view_->setBodyStage(document_->evaluationBefore(inputs["featureId"].toString()).bodies,"loft-source:"+session_["id"].toString());
    showLoft();previewLoft();view_->setFocus();return session_;
}
QJsonObject AlloyWindow::previewLoft() {
    const auto inputs=session_["inputs"].toObject();const bool surface=inputs.value("operation")=="newComponent";
    view_->selectLoftProfiles(surface ? QJsonArray{} : inputs["profiles"].toArray());
    view_->clearSweepPath();
    if(surface) {
        QSet<QString> visible;for(const auto& sketch:document_->sketches())if(sketch.ownerId==document_->authored()["documentId"].toString())visible.insert(sketch.featureId);
        view_->setCommandSketches(visible);
        const auto guide=inputs["centerline"].toObject();
        for(const auto& sketch:document_->sketches())if(sketch.featureId==guide["sketchId"])
            view_->showSweepPath(sketch,QJsonArray{guide["entityId"]});
    } else view_->setCommandSketches({});
    session_["previewed"]=false;session_["geometryEvaluated"]=false;session_.remove("geometry");session_.remove("documentDraft");view_->clearPreview();
    auto reject=[this](const QString& code,const QString& message,const QJsonObject& diagnostics=QJsonObject{}){auto r=failure(code,message);auto e=r["error"].toObject();e["diagnostics"]=diagnostics;r["error"]=e;session_["previewError"]=e;if(loftDock_)if(auto* label=loftDock_->findChild<QLabel*>("loftError"))label->setText(message);return r;};
    if(loftDock_) {
        if(auto* list=loftDock_->findChild<QListWidget*>("loftProfiles")){
            const QSignalBlocker blocker(list);list->clear();
            int index=0;for(const auto& value:inputs["profiles"].toArray()){const auto ref=value.toObject();QString name=ref["sketchId"].toString();for(const auto& s:document_->sketches())if(s.featureId==name){name=s.name;break;}list->addItem(QString::number(++index)+". "+name);}
        }
        if(surface)for(const auto& role:QStringList{"section1","section2","centerline"})if(auto* button=loftDock_->findChild<QPushButton*>("loft."+role)){
            const auto sections=inputs["sections"].toArray();
            const auto ref=role=="centerline" ? inputs["centerline"].toObject() : sections.size()==2 ? sections[role=="section1"?0:1].toObject() : QJsonObject{};
            QString sketchName;
            for(const auto& sketch:document_->sketches())if(sketch.featureId==ref["sketchId"]){sketchName=sketch.name;break;}
            button->setText(ref.isEmpty()?"Select":QString("1 selected · %1").arg(sketchName.isEmpty()?"Sketch":sketchName));
            button->setChecked(session_["selectionRole"]==role);
        }
        loftDock_->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
    if(session_["baseDocumentRevision"].toInteger()!=static_cast<qint64>(document_->revision()))return reject("stale_session","The document changed; cancel and restart Loft");
    if(surface){const auto sections=inputs["sections"].toArray();if(sections.size()!=2 || sections[0].toObject().isEmpty() || sections[1].toObject().isEmpty() || inputs["centerline"].toObject().isEmpty())
        return reject("selection_required","Select an open arc, an open line, then one fit-spline Centerline");}
    QJsonObject candidate;try{candidate=loftCandidate(inputs);}catch(const std::exception& error){return reject("invalid_arguments",QString::fromUtf8(error.what()));}
    const auto evaluated=CadDocument::evaluateCandidate(candidate);if(!evaluated.ok)return reject("evaluation_failed",evaluated.error,evaluated.diagnostics);
    const auto id=session_["inputs"].toObject().value("featureId").toString(session_["newFeatureId"].toString());
    auto stage=evaluated.evaluation;
    if(session_["inputs"].toObject().contains("featureId")){QJsonArray fs;for(const auto& f:candidate["features"].toArray()){fs.append(f);if(f.toObject()["id"]==id)break;}auto prefix=candidate;prefix["features"]=fs;const auto early=CadDocument::evaluateCandidate(prefix);if(!early.ok)return reject("evaluation_failed",early.error,early.diagnostics);stage=early.evaluation;}
    for(const auto& body:stage->bodies)if(body.featureId==id){view_->showPreview(body.shape,session_["inputs"].toObject().contains("featureId") ? body.bodyId : QString{},!surface);session_["geometry"]=body.summary;session_["featureId"]=body.featureId;session_["bodyId"]=body.bodyId;break;}
    session_["documentDraft"]=candidate;session_["previewed"]=true;session_["geometryEvaluated"]=true;session_.remove("previewError");
    if(loftDock_){loftDock_->findChild<QLabel*>("loftError")->clear();loftDock_->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->setEnabled(true);}
    return session_;
}
QJsonObject AlloyWindow::selectLoftProfile(const QString& sketchId,const QString& profileId) {
    auto inputs=session_["inputs"].toObject();auto refs=inputs["profiles"].toArray();QJsonArray next;bool removed=false;
    for(const auto& value:refs){const auto r=value.toObject();if(r["sketchId"]==sketchId && r["profileId"]==profileId){removed=true;continue;}next.append(r);}
    if(!removed)next.append(QJsonObject{{"sketchId",sketchId},{"profileId",profileId}});inputs["profiles"]=next;session_["inputs"]=inputs;previewLoft();return {{"kind","profile"},{"sketchId",sketchId},{"profileId",profileId},{"profiles",next}};
}
QJsonObject AlloyWindow::selectLoftEntity(const QString& role,const QString& sketchId,const QString& entityId) {
    if(session_.value("command")!="solid.loft" || session_.value("inputs").toObject().value("operation")!="newComponent")return failure("invalid_session","Start a surface Loft first");
    const auto required=role=="section1" ? "arc" : role=="section2" ? "line" : role=="centerline" ? "fitSpline" : "";
    if(!*required)return failure("invalid_arguments","Choose a Loft section or Centerline role");
    const auto earlier=session_["inputs"].toObject().value("featureId").toString();
    const auto stage=earlier.isEmpty() ? document_->evaluation() : document_->evaluationBefore(earlier);
    bool found=false;for(const auto& sketch:stage.sketches)if(sketch.featureId==sketchId && sketch.visible
        && sketch.ownerId==document_->authored()["documentId"].toString() && sketch.entityEdges.contains(entityId))
        for(const auto& row:sketch.summary["entities"].toArray())if(row.toObject()["id"]==entityId && row.toObject()["type"]==required
            && !row.toObject()["construction"].toBool())found=true;
    if(!found)return failure("loft_source_unavailable",QString("Select an ordinary retained %1 in a visible root sketch").arg(required));
    auto inputs=session_["inputs"].toObject();const QJsonObject ref{{"sketchId",sketchId},{"entityId",entityId}};
    if(role=="centerline")inputs["centerline"]=ref;
    else {auto sections=inputs["sections"].toArray();if(sections.size()!=2)sections=QJsonArray{QJsonObject{},QJsonObject{}};sections[role=="section1"?0:1]=ref;inputs["sections"]=sections;}
    session_["inputs"]=inputs;session_["selectionRole"]=role=="section1" ? "section2" : "centerline";
    const auto preview=previewLoft();return {{"role",role},{"sketchId",sketchId},{"entityId",entityId},{"previewed",preview.value("previewed")}};
}
QJsonObject AlloyWindow::pickLoft(double x,double y) {
    const auto role=session_.value("selectionRole").toString("section1");
    const auto required=role=="section1" ? "arc" : role=="section2" ? "line" : "fitSpline";
    const auto earlier=session_["inputs"].toObject().value("featureId").toString();
    const auto stage=earlier.isEmpty() ? document_->evaluation() : document_->evaluationBefore(earlier);
    QString sketchId,entityId;double nearest=7;bool ambiguous=false;
    for(const auto& sketch:stage.sketches)if(sketch.visible && sketch.ownerId==document_->authored()["documentId"].toString()) {
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
            if(row.toObject()["id"]==hit["entityId"] && row.toObject()["type"]==required
                && !row.toObject()["construction"].toBool())rightType=true;
        if(!rightType)continue;
        const auto distance=hit["distancePixels"].toDouble();
        if(distance<nearest-1e-6){nearest=distance;sketchId=sketch.featureId;entityId=hit["entityId"].toString();ambiguous=false;}
        else if(std::abs(distance-nearest)<1e-6)ambiguous=true;
    }
    if(ambiguous)return failure("ambiguous_loft_source","More than one sketch curve is under this point");
    if(sketchId.isEmpty())return failure("loft_source_required","Click a visible open section or fit-spline Centerline");
    return selectLoftEntity(role,sketchId,entityId);
}
void AlloyWindow::showLoft() {
    if(loftDock_){canvasWindow_->removeDockWidget(loftDock_);delete loftDock_;}
    loftDock_=new QDockWidget("Loft",canvasWindow_);loftDock_->setObjectName("loftDock");loftDock_->setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);
    auto* panel=new QWidget(loftDock_);loftDock_->setWidget(panel);auto* form=new QFormLayout(panel);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    auto* name=new QLineEdit(session_["inputs"].toObject()["name"].toString(),panel);name->setObjectName("loftName");form->addRow("Name",name);
    connect(name,&QLineEdit::editingFinished,panel,[this,name]{auto inputs=session_["inputs"].toObject();inputs["name"]=name->text();session_["inputs"]=inputs;previewLoft();});
    const bool surface=session_["inputs"].toObject().value("operation")=="newComponent";
    auto* operation=new QComboBox(panel);operation->setObjectName("loftOperation");operation->addItem("New Body","newBody");operation->addItem("New Component","newComponent");operation->setCurrentIndex(surface?1:0);
    operation->setEnabled(!session_["inputs"].toObject().contains("featureId") && !session_["surfaceOnly"].toBool());form->addRow("Operation",operation);
    connect(operation,qOverload<int>(&QComboBox::currentIndexChanged),panel,[this,operation](int index){
        auto inputs=session_["inputs"].toObject();if(inputs.contains("featureId"))return;
        const auto choice=operation->itemData(index).toString();inputs["operation"]=choice;
        if(choice=="newComponent"){
            inputs.remove("profiles");inputs["sections"]=QJsonArray{QJsonObject{},QJsonObject{}};inputs["componentName"]="Component";
            session_["selectionRole"]="section1";
        } else {inputs.remove("sections");inputs.remove("centerline");inputs.remove("componentName");inputs["profiles"]=QJsonArray{};}
        session_["inputs"]=inputs;QTimer::singleShot(0,this,[this]{if(session_.value("command")!="solid.loft")return;showLoft();previewLoft();view_->setFocus();});
    });
    if(surface) {
        auto* component=new QLineEdit(session_["inputs"].toObject()["componentName"].toString(),panel);
        component->setObjectName("loftComponentName");component->setEnabled(!session_["inputs"].toObject().contains("featureId"));
        form->addRow("Component",component);
        connect(component,&QLineEdit::editingFinished,panel,[this,component]{auto inputs=session_["inputs"].toObject();inputs["componentName"]=component->text();session_["inputs"]=inputs;previewLoft();});
        for(const auto& role:QStringList{"section1","section2","centerline"}) {
            auto* button=new QPushButton("Select",panel);button->setObjectName("loft."+role);button->setCheckable(true);
            form->addRow(role=="section1"?"Profile 1 · arc":role=="section2"?"Profile 2 · line":"Centerline",button);
            connect(button,&QPushButton::clicked,panel,[this,role]{execute("solid.loftSelection",{{"role",role}});previewLoft();});
        }
        auto* help=new QLabel("Click an open arc, then an open line on the endpoint planes. Select the retained fit-spline Centerline. The result is one sheet face.",panel);
        help->setWordWrap(true);form->addRow(help);
        form->addRow("Closed",new QLabel("Off · surface",panel));
    } else {
        auto* list=new QListWidget(panel);list->setObjectName("loftProfiles");list->setMinimumHeight(100);form->addRow("Profiles",list);
        auto* help=new QLabel("Click circular profiles in order. Click again to remove.",panel);help->setWordWrap(true);form->addRow(help);
        for(const auto& action:QStringList{"Move Up","Move Down","Remove"}){auto* button=new QPushButton(action,panel);form->addRow(button);connect(button,&QPushButton::clicked,panel,[this,list,action]{auto inputs=session_["inputs"].toObject();auto refs=inputs["profiles"].toArray();const auto index=list->currentRow();if(index<0 || index>=refs.size())return;int target=index;if(action=="Remove")refs.removeAt(index);else{target=index+(action=="Move Up" ? -1 : 1);if(target<0 || target>=refs.size())return;const QJsonValue temp=refs[index];refs[index]=refs[target];refs[target]=temp;}inputs["profiles"]=refs;session_["inputs"]=inputs;previewLoft();list->setCurrentRow(std::min(target,int(refs.size())-1));});}
        form->addRow("Start/End",new QLabel("Free",panel));form->addRow("Guide Type",new QLabel("Rails · none",panel));form->addRow("Chain Selection",new QLabel("On",panel));form->addRow("Closed",new QLabel("Off",panel));
    }
    auto* error=new QLabel(panel);error->setObjectName("loftError");error->setWordWrap(true);form->addRow(error);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,panel);form->addRow(buttons);
    connect(buttons,&QDialogButtonBox::accepted,panel,[this,error]{const auto response=execute("command.commit",{{"session",session_["id"]}});if(!response["ok"].toBool())error->setText(response["error"].toObject()["message"].toString());});connect(buttons,&QDialogButtonBox::rejected,panel,[this]{execute("command.cancel",{{"session",session_["id"]}});});
    auto cancel=[this]{if(session_.value("command")=="solid.loft")execute("command.cancel",{{"session",session_["id"]}});};auto* escape=new QShortcut(QKeySequence(Qt::Key_Escape),panel);escape->setContext(Qt::WidgetWithChildrenShortcut);connect(escape,&QShortcut::activated,panel,cancel);
    if(!loftEscape_){loftEscape_=new QShortcut(QKeySequence(Qt::Key_Escape),view_);loftEscape_->setContext(Qt::WidgetShortcut);connect(loftEscape_,&QShortcut::activated,this,cancel);}loftEscape_->setEnabled(true);
    canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,loftDock_);loftDock_->show();
}
