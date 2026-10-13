#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <QDockWidget>
#include <QFormLayout>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QShortcut>
#include <QTimer>

namespace {
QJsonObject moveFailure(const QString& message){return {{"ok",false},{"completion","failed"},{"error",QJsonObject{{"code","evaluation_failed"},{"message",message}}}};}
}
QJsonObject AlloyWindow::moveCandidate(const QJsonObject& inputs) const {
    auto args=inputs;
    if(!args.contains("featureId") && session_.value("command")=="solid.move")args["newFeatureId"]=session_["newFeatureId"];
    return CadDocument::moveCandidate(document_->authored(),args);
}
QJsonObject AlloyWindow::beginMove(const QJsonObject& overrides) {
    if(!session_.isEmpty())return moveFailure("Complete or cancel the active command first");
    auto error=validateArguments(*findCommand("solid.move"),overrides);if(!error.isEmpty())return moveFailure(error);
    QJsonObject inputs{{"targetBodyId",selectedBody_},{"name","Move"}};
    for(const auto& key:{"xMm","yMm","zMm","xDeg","yDeg","zDeg","pivotXMm","pivotYMm","pivotZMm"})inputs[key]=0;
    for(auto it=overrides.begin();it!=overrides.end();++it)inputs[it.key()]=it.value();
    if(!overrides.contains("pivotXMm") && !overrides.contains("featureId"))for(const auto& body:document_->bodies())if(body.bodyId==inputs["targetBodyId"].toString()){
        auto bounds=body.summary["boundsMm"].toObject();auto low=bounds["min"].toArray(),high=bounds["max"].toArray();inputs["pivotXMm"]=(low[0].toDouble()+high[0].toDouble())/2;inputs["pivotYMm"]=(low[1].toDouble()+high[1].toDouble())/2;inputs["pivotZMm"]=low[2];}
    session_={{"id",CadDocument::newId()},{"command","solid.move"},{"status","active"},{"inputs",inputs},{"newFeatureId",CadDocument::newId()},
        {"baseDocumentRevision",static_cast<qint64>(document_->revision())},{"previewed",false},{"inputSchema",findCommand("solid.move")->parameters}};
    showMove();previewMove();return session_;
}
QJsonObject AlloyWindow::previewMove() {
    view_->clearPreview();view_->clearMoveCue();session_["previewed"]=false;session_["geometryEvaluated"]=false;session_.remove("documentDraft");session_.remove("geometry");session_.remove("previewError");
    auto reject=[this](const QString& message){view_->clearBodyStage();auto response=moveFailure(message);session_["previewError"]=response["error"];return response;};
    if(session_["baseDocumentRevision"].toInteger()!=static_cast<qint64>(document_->revision()))return reject("Stale Move session");
    QJsonObject candidate;try{candidate=moveCandidate(session_["inputs"].toObject());}catch(const std::exception& e){return reject(QString::fromUtf8(e.what()));}
    const auto evaluated=CadDocument::evaluateCandidate(candidate);if(!evaluated.ok)return reject(evaluated.error);
    auto stage=evaluated.evaluation;
    const auto featureId=session_["inputs"].toObject()["featureId"].toString();
    if(!featureId.isEmpty()){QJsonArray features;for(const auto& f:candidate["features"].toArray()){features.append(f);if(f.toObject()["id"]==featureId)break;}candidate["features"]=features;stage=CadDocument::evaluateCandidate(candidate).evaluation;}
    const auto inputs=session_["inputs"].toObject();const auto target=inputs["targetBodyId"].toString();
    for(const auto& body:stage->bodies)if(body.bodyId==target){view_->setBodyStage(stage->bodies,"move:"+session_["id"].toString());view_->showMovePreview(body.shape,target);view_->showMoveCue(inputs);session_["geometry"]=body.summary;break;}
    session_["previewed"]=true;session_["geometryEvaluated"]=true;return session_;
}
void AlloyWindow::showMove() {
    if(moveDock_){canvasWindow_->removeDockWidget(moveDock_);moveDock_->deleteLater();}
    moveDock_=new QDockWidget("Move/Copy",canvasWindow_);moveDock_->setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);
    auto* panel=new QWidget(moveDock_);moveDock_->setWidget(panel);auto* form=new QFormLayout(panel);
    form->addRow("Move Object",new QLabel("Bodies",panel));form->addRow("Move Type",new QLabel("Free Move",panel));
    auto* bodies=new QComboBox(panel);bodies->setAccessibleName("Move body");bodies->addItem("Select body…",QString{});
    auto available=document_->bodies();const auto edit=session_["inputs"].toObject()["featureId"].toString();if(!edit.isEmpty())available=document_->evaluationBefore(edit).bodies;
    for(const auto& body:available)bodies->addItem(body.name,body.bodyId);bodies->setCurrentIndex(bodies->findData(session_["inputs"].toObject()["targetBodyId"].toString()));form->addRow("Selection",bodies);
    auto* status=new QLabel(panel);status->setWordWrap(true);
    auto update=[this,status](const QJsonObject& changes){auto response=execute("command.update",{{"session",session_["id"]},{"inputs",changes}});if(response["ok"].toBool())response=previewMove();status->setText(response["ok"].toBool(true) ? QString{} : response["error"].toObject()["message"].toString());};
    connect(bodies,&QComboBox::currentIndexChanged,panel,[this,bodies,status](int){const auto response=execute("selection.set",{{"bodyId",bodies->currentData().toString()}});status->setText(response["ok"].toBool() ? QString{} : response["error"].toObject()["message"].toString());});
    const QStringList keys{"xMm","yMm","zMm","xDeg","yDeg","zDeg","pivotXMm","pivotYMm","pivotZMm"};
    const QStringList labels{"X Distance","Y Distance","Z Distance","X Angle","Y Angle","Z Angle","Pivot X","Pivot Y","Pivot Z"};
    for(int i=0;i<keys.size();++i){auto* field=new QDoubleSpinBox(panel);field->setObjectName(keys[i]);field->setAccessibleName(labels[i]);field->setRange(-100000,100000);field->setDecimals(3);field->setSuffix(i>=3 && i<6 ? " °" : " mm");field->setValue(session_["inputs"].toObject()[keys[i]].toDouble());form->addRow(labels[i],field);connect(field,&QDoubleSpinBox::valueChanged,panel,[update,key=keys[i]](double v){update({{key,v}});});if(i<6)connect(field,&QDoubleSpinBox::editingFinished,panel,[this]{if(session_.value("command")=="solid.move")execute("solid.moveApplyGesture");});}
    form->addRow(status);auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,panel);form->addRow(buttons);
    connect(buttons,&QDialogButtonBox::accepted,panel,[this,status]{auto response=execute("command.commit",{{"session",session_["id"]}});if(!response["ok"].toBool())status->setText(response["error"].toObject()["message"].toString());});
    connect(buttons,&QDialogButtonBox::rejected,panel,[this]{execute("command.cancel",{{"session",session_["id"]}});});
    auto cancel=[this]{if(session_.value("command")=="solid.move")execute("command.cancel",{{"session",session_["id"]}});};
    auto* escape=new QShortcut(QKeySequence(Qt::Key_Escape),panel);escape->setContext(Qt::WidgetWithChildrenShortcut);connect(escape,&QShortcut::activated,panel,cancel);
    if(!moveEscape_){moveEscape_=new QShortcut(QKeySequence(Qt::Key_Escape),view_);moveEscape_->setContext(Qt::WidgetShortcut);connect(moveEscape_,&QShortcut::activated,this,cancel);}
    moveEscape_->setEnabled(true);
    canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,moveDock_);moveDock_->show();
}
