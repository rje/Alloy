// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include "SketchGeometry.h"
#include "SolidHole.h"
#include <BRepAlgoAPI_Common.hxx>
#include <QDockWidget>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QShortcut>
#include <QSignalBlocker>
namespace {
QJsonObject holeFailure(const QString& code,const QString& message,const QJsonObject& diagnostics={}) {
    return {{"ok",false},{"completion","failed"},{"error",QJsonObject{{"code",code},{"message",message},{"diagnostics",diagnostics}}}};
}
QJsonObject pointInputs(const QJsonObject& picked) {
    return {{"sourceSketchId",picked["sketchId"]},{"sourceEntityId",picked["entityId"]},{"sourcePointId",picked["pointId"]}};
}
}
EvaluatedDocument AlloyWindow::holeInputDocument() const {
    const auto edit=session_["inputs"].toObject()["featureId"].toString();
    return edit.isEmpty() ? document_->evaluation() : document_->evaluationBefore(edit);
}
QJsonObject AlloyWindow::holeCandidate(const QJsonObject& inputs) const {
    auto args=inputs;
    if(!args.contains("featureId") && session_.value("command")=="solid.hole")args["newFeatureId"]=session_["newFeatureId"];
    return CadDocument::holeCandidate(document_->authored(),authoringArguments(args));
}
QJsonObject AlloyWindow::beginHole(const QJsonObject& overrides) {
    if(!session_.isEmpty())return holeFailure("session_busy","Complete or cancel the active command first");
    const auto error=validateArguments(*findCommand("solid.hole"),overrides);if(!error.isEmpty())return holeFailure("invalid_arguments",error);
    QJsonObject inputs{{"placement","fromSketch"},{"holeType","countersink"},{"extent","all"},
        {"boreDiameterMm",4},{"sinkDiameterMm",10},{"includedAngleDeg",90},{"name","Hole"}};
    const auto picked=view_->finishedPointSelection();
    if(picked["kind"]=="sketchPoint") {
        const auto ids=pointInputs(picked);for(auto i=ids.begin();i!=ids.end();++i)inputs[i.key()]=i.value();
        const auto sketch=SketchGeometry::findFeature(document_->authored(),picked["sketchId"].toString());
        inputs["targetBodyId"]=sketch["parameters"].toObject()["support"].toObject()["bodyId"];
    }
    if(overrides.contains("featureId")) {
        bool found=false;for(const auto& entry:document_->authored()["features"].toArray())if(entry.toObject()["id"]==overrides["featureId"]) {
            const auto feature=entry.toObject();if(feature["type"]!="hole")return holeFailure("stale_entity","Hole feature unavailable");
            inputs=feature["parameters"].toObject();inputs["featureId"]=feature["id"];inputs["name"]=feature["name"];found=true;break;
        }
        if(!found)return holeFailure("stale_entity","Hole feature unavailable");
    }
    for(auto i=overrides.begin();i!=overrides.end();++i)inputs[i.key()]=i.value();
    for(const auto& field:{"sinkDiameterMm","includedAngleDeg","distanceMm"})if(!inputs.contains(field))
        inputs[field]=QString(field)=="distanceMm" ? 5 : QString(field)=="sinkDiameterMm" ? 10 : 90;
    session_={{"id",CadDocument::newId()},{"command","solid.hole"},{"status","active"},{"inputs",inputs},
        {"newFeatureId",CadDocument::newId()},{"baseDocumentRevision",static_cast<qint64>(document_->revision())},
        {"previewed",false},{"inputSchema",findCommand("solid.hole")->parameters},{"selectionRole",inputs.contains("sourcePointId") ? "ready" : "point"}};
    view_->setBodyStage(holeInputDocument().bodies,"hole-source:"+session_.value("id").toString());
    view_->setCommandSketches(inputs.contains("sourceSketchId") ? QSet<QString>{inputs["sourceSketchId"].toString()} : QSet<QString>{});
    selectedFace_={};view_->selectSupportFace({},{});view_->selectBody({});showHole();previewHole();view_->setFocus();return session_;
}
QJsonObject AlloyWindow::pickHole(double x,double y) {
    if(session_["selectionRole"]=="target") {
        const auto target=view_->pickBody(x,y);if(target.isEmpty())return holeFailure("target_required","Select an actual body to cut");
        const auto response=execute("command.update",{{"session",session_.value("id")},{"inputs",QJsonObject{{"targetBodyId",target}}}});
        if(!response["ok"].toBool())return response;
        session_["selectionRole"]="ready";previewHole();view_->setFocus();return QJsonObject{{"kind","body"},{"bodyId",target}};
    }
    const auto picked=view_->pickFinishedPoint(x,y);
    if(picked.isEmpty())return holeFailure("point_required","Select a visible standalone sketch Point");
    auto updates=pointInputs(picked);
    const auto sketch=SketchGeometry::findFeature(document_->authored(),picked["sketchId"].toString());
    // Selecting a point infers its exact support body only when no target was
    // selected. A later point or target pick never silently changes the other.
    if(!session_["inputs"].toObject().contains("targetBodyId"))updates["targetBodyId"]=sketch["parameters"].toObject()["support"].toObject()["bodyId"];
    const auto response=execute("command.update",{{"session",session_.value("id")},{"inputs",updates}});if(!response["ok"].toBool())return response;
    session_["selectionRole"]="ready";view_->setCommandSketches({picked["sketchId"].toString()});view_->selectFinishedPoint(picked);
    previewHole();view_->setFocus();return picked;
}
QJsonObject AlloyWindow::previewHole() {
    if(session_.value("command")!="solid.hole")return holeFailure("invalid_session","Start Hole before previewing");
    session_["previewed"]=false;session_["geometryEvaluated"]=false;session_.remove("geometry");session_.remove("documentDraft");session_.remove("previewError");view_->clearPreview();
    auto reject=[this](const QString& code,const QString& message,const QJsonObject& details=QJsonObject{}) {
        const auto r=holeFailure(code,message,details);session_["previewError"]=r["error"];refreshHoleControls();return r;
    };
    if(session_["baseDocumentRevision"].toInteger()!=static_cast<qint64>(document_->revision()))return reject("stale_session","The document changed; cancel and restart Hole");
    const auto inputs=session_["inputs"].toObject();
    if(!inputs.contains("sourceSketchId") || !inputs.contains("sourceEntityId") || !inputs.contains("sourcePointId"))return reject("point_required","Select one standalone sketch Point");
    if(!inputs.contains("targetBodyId"))return reject("target_required","Select one body to cut");
    try {
        const auto candidate=holeCandidate(inputs);const auto featureId=inputs.value("featureId").toString(session_["newFeatureId"].toString());
        QJsonObject feature;for(const auto& entry:candidate["features"].toArray())if(entry.toObject()["id"]==featureId)feature=entry.toObject();
        const auto source=SketchGeometry::findFeature(candidate,inputs["sourceSketchId"].toString());
        const auto evaluated=CadDocument::evaluateCandidate(candidate);if(!evaluated.ok)return reject("evaluation_failed",evaluated.error,evaluated.diagnostics);
        const auto prior=holeInputDocument();const auto cutter=SolidHole::cutter(prior,feature,source);
        auto stage=evaluated;
        if(candidate["features"].toArray().last().toObject()["id"]!=featureId) {
            QJsonArray features;for(const auto& entry:candidate["features"].toArray()){features.append(entry);if(entry.toObject()["id"]==featureId)break;}
            auto prefix=candidate;prefix["features"]=features;stage=CadDocument::evaluateCandidate(prefix);
            if(!stage.ok)return reject("evaluation_failed",stage.error,stage.diagnostics);
        }
        const EvaluatedBody* stock=nullptr;const EvaluatedBody* result=nullptr;
        for(const auto& b:prior.bodies)if(b.bodyId==inputs["targetBodyId"])stock=&b;
        for(const auto& b:stage.evaluation->bodies)if(b.bodyId==inputs["targetBodyId"])result=&b;
        if(!stock || !result)return reject("target_unavailable","The retained Hole target is unavailable");
        // Clip the disposable tool to stock solely for readable display. The
        // full through cutter and final result came from the shared authority.
        BRepAlgoAPI_Common removed(stock->shape,cutter);if(!removed.IsDone())return reject("preview_failed","Unable to display the actual removed material");
        view_->showHolePreview(*result,removed.Shape());
        session_["geometry"]=result->summary;session_["documentDraft"]=candidate;
        session_["previewed"]=true;session_["geometryEvaluated"]=true;refreshHoleControls();return session_;
    } catch(const GeometryKernel::EvaluationError& e){return reject("evaluation_failed",QString::fromUtf8(e.what()),e.diagnostics());}
      catch(const std::exception& e){return reject("invalid_arguments",QString::fromUtf8(e.what()));}
}
void AlloyWindow::refreshHoleControls() {
    if(!holeDock_ || session_.value("command")!="solid.hole")return;
    const auto inputs=session_["inputs"].toObject();
    for(const auto& field:{"boreDiameterMm","sinkDiameterMm","includedAngleDeg","distanceMm"})if(auto* spin=holeDock_->findChild<QDoubleSpinBox*>("hole."+QString(field))) {
        const QSignalBlocker block(spin);spin->setValue(inputs[field].toDouble());
    }
    const bool simple=inputs["holeType"]=="simple",distance=inputs["extent"]=="distance";
    for(const auto& entry:{qMakePair(QString("hole.type"),QString("holeType")),qMakePair(QString("hole.extent"),QString("extent"))})
        if(auto* combo=holeDock_->findChild<QComboBox*>(entry.first)) {
            const QSignalBlocker block(combo);combo->setCurrentIndex(combo->findData(inputs[entry.second].toString()));
        }
    if(auto* panel=holeDock_->widget())if(auto* form=qobject_cast<QFormLayout*>(panel->layout())) {
        for(const auto& field:{"sinkDiameterMm","includedAngleDeg","depth"})
            if(auto* widget=panel->findChild<QWidget*>("hole."+QString(field)))form->setRowVisible(widget,!simple);
        if(auto* widget=panel->findChild<QWidget*>("hole.distanceMm"))form->setRowVisible(widget,simple && distance);
    }
    if(auto* combo=holeDock_->findChild<QComboBox*>("hole.extent"))combo->setEnabled(simple);
    if(auto* label=holeDock_->findChild<QLabel*>("hole.point"))label->setText(inputs.contains("sourcePointId") ? "1 sketch point selected" : "Select one sketch point");
    if(auto* label=holeDock_->findChild<QLabel*>("hole.target")) {
        QString name;for(const auto& body:holeInputDocument().bodies)if(body.bodyId==inputs["targetBodyId"])name=body.name;
        label->setText(name.isEmpty() ? "Select one body" : name);
    }
    if(auto* label=holeDock_->findChild<QLabel*>("hole.error"))label->setText(session_["previewError"].toObject()["message"].toString());
    if(auto* label=holeDock_->findChild<QLabel*>("hole.depth"))label->setText(session_["previewed"].toBool() ? QString::number(session_["geometry"].toObject()["sinkDepthMm"].toDouble(),'g',6)+" mm" : "—");
    if(auto* buttons=holeDock_->findChild<QDialogButtonBox*>("hole.buttons"))buttons->button(QDialogButtonBox::Ok)->setEnabled(session_["previewed"].toBool());
}
void AlloyWindow::showHole() {
    if(holeDock_){canvasWindow_->removeDockWidget(holeDock_);delete holeDock_;}
    holeDock_=new QDockWidget("Hole",canvasWindow_);holeDock_->setObjectName("holeDock");holeDock_->setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);
    auto* panel=new QWidget(holeDock_);holeDock_->setWidget(panel);auto* form=new QFormLayout(panel);
    const QString sessionId=session_.value("id").toString();
    auto choice=[&](const QString& label,const QString& value,const QString& object) {auto* combo=new QComboBox(panel);combo->setObjectName(object);combo->addItem(value);combo->setEnabled(false);form->addRow(label,combo);};
    choice("Placement","From Sketch","hole.placement");
    auto* point=new QLabel(panel);point->setObjectName("hole.point");form->addRow("Sketch Point",point);
    auto* choosePoint=new QPushButton("Select Point",panel);choosePoint->setObjectName("hole.selectPoint");form->addRow(choosePoint);
    connect(choosePoint,&QPushButton::clicked,panel,[this,sessionId]{if(session_.value("id")!=sessionId)return;session_["selectionRole"]="point";view_->setFocus();});
    auto* target=new QLabel(panel);target->setObjectName("hole.target");form->addRow("Objects to Cut",target);
    auto* chooseTarget=new QPushButton("Select Target",panel);chooseTarget->setObjectName("hole.selectTarget");form->addRow(chooseTarget);
    connect(chooseTarget,&QPushButton::clicked,panel,[this,sessionId]{if(session_.value("id")!=sessionId)return;session_["selectionRole"]="target";session_["previewed"]=false;session_["geometryEvaluated"]=false;
        session_.remove("geometry");session_.remove("documentDraft");view_->clearPreview();refreshHoleControls();view_->setFocus();});
    auto* type=new QComboBox(panel);type->setObjectName("hole.type");type->addItem("Simple","simple");type->addItem("Countersink","countersink");form->addRow("Hole Type",type);
    choice("Tap Type","Simple","hole.tap");
    auto* extent=new QComboBox(panel);extent->setObjectName("hole.extent");extent->addItem("All","all");extent->addItem("Distance","distance");form->addRow("Extent",extent);
    choice("Drill Point","Flat","hole.drillPoint");
    connect(type,&QComboBox::currentIndexChanged,panel,[this,sessionId,type]{
        if(session_.value("id")!=sessionId)return;
        QJsonObject updates{{"holeType",type->currentData().toString()}};
        if(type->currentData()=="countersink")updates["extent"]="all";
        const auto r=execute("command.update",{{"session",sessionId},{"inputs",updates}});if(r["ok"].toBool())previewHole();
    });
    connect(extent,&QComboBox::currentIndexChanged,panel,[this,sessionId,extent]{
        if(session_.value("id")!=sessionId)return;
        const auto r=execute("command.update",{{"session",sessionId},{"inputs",QJsonObject{{"extent",extent->currentData().toString()}}}});if(r["ok"].toBool())previewHole();
    });
    auto number=[&](const QString& label,const QString& field,double minimum,double maximum,const QString& suffix) {
        auto* spin=new QDoubleSpinBox(panel);spin->setObjectName("hole."+field);spin->setAccessibleName(label);
        spin->setRange(minimum,maximum);spin->setDecimals(3);spin->setSuffix(suffix);spin->setKeyboardTracking(false);spin->setValue(session_["inputs"].toObject()[field].toDouble());form->addRow(label,spin);
        connect(spin,&QDoubleSpinBox::valueChanged,panel,[this,sessionId,field](double value){if(session_.value("id")!=sessionId)return;const auto r=execute("command.update",{{"session",sessionId},{"inputs",QJsonObject{{field,value}}}});if(r["ok"].toBool())previewHole();});return spin;
    };
    number("Bore Diameter","boreDiameterMm",.001,100000," mm");number("Depth","distanceMm",.001,100000," mm");number("Countersink Diameter","sinkDiameterMm",.001,100000," mm");number("Included Angle","includedAngleDeg",.001,179.999,"°");
    auto* depth=new QLabel(panel);depth->setObjectName("hole.depth");form->addRow("Derived Sink Depth",depth);
    auto* error=new QLabel(panel);error->setObjectName("hole.error");error->setWordWrap(true);form->addRow(error);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,panel);buttons->setObjectName("hole.buttons");form->addRow(buttons);
    connect(buttons,&QDialogButtonBox::accepted,panel,[this,sessionId,error]{if(session_.value("id")!=sessionId)return;for(auto* spin:holeDock_->findChildren<QDoubleSpinBox*>())spin->interpretText();const auto preview=previewHole();if(!preview["previewed"].toBool())return;const auto r=execute("command.commit",{{"session",sessionId}});if(!r["ok"].toBool())error->setText(r["error"].toObject()["message"].toString());});
    auto cancel=[this,sessionId]{if(session_.value("id")==sessionId)execute("command.cancel",{{"session",sessionId}});};
    connect(buttons,&QDialogButtonBox::rejected,panel,cancel);
    auto* escape=new QShortcut(QKeySequence(Qt::Key_Escape),panel);escape->setContext(Qt::WidgetWithChildrenShortcut);connect(escape,&QShortcut::activated,panel,cancel);
    if(holeEscape_)delete holeEscape_;holeEscape_=new QShortcut(QKeySequence(Qt::Key_Escape),view_);holeEscape_->setContext(Qt::WidgetShortcut);connect(holeEscape_,&QShortcut::activated,this,cancel);
    canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,holeDock_);holeDock_->show();refreshHoleControls();
}
