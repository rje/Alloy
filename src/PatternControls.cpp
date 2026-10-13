// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <BRepAdaptor_Curve.hxx>
#include <QDockWidget>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QShortcut>
#include <QSignalBlocker>
#include <QScrollArea>
namespace {
QJsonObject failure(const QString& code,const QString& message){return {{"ok",false},{"error",QJsonObject{{"code",code},{"message",message}}}};}
}
QJsonObject AlloyWindow::patternCandidate(const QJsonObject& inputs) const {
    auto args=inputs;if(!args.contains("featureId") && session_.value("command")=="solid.rectangularPattern")args["newFeatureId"]=session_["newFeatureId"];
    return CadDocument::patternCandidate(document_->authored(),authoringArguments(args));
}
std::vector<EvaluatedBody> AlloyWindow::patternInputBodies() const {
    const auto edit=session_["inputs"].toObject()["featureId"].toString();return edit.isEmpty() ? document_->bodies() : document_->evaluationBefore(edit).bodies;
}
QJsonObject AlloyWindow::beginPattern(const QJsonObject& overrides) {
    if(!session_.isEmpty())return failure("session_busy","Complete or cancel the active command first");
    QJsonObject inputs{{"sourceFeatureId",""},{"targetBodyId",""},{"axes",QJsonArray{}},{"name","Rectangular Pattern"}};
    if(overrides.contains("featureId")){bool found=false;for(const auto& entry:document_->authored()["features"].toArray()){const auto f=entry.toObject();if(f["id"]!=overrides["featureId"])continue;if(f["type"]!="rectangularPattern")return failure("stale_entity","Pattern feature unavailable");inputs=f["parameters"].toObject();inputs["name"]=f["name"];inputs["featureId"]=f["id"];found=true;}if(!found)return failure("stale_entity","Pattern feature unavailable");}
    for(auto it=overrides.begin();it!=overrides.end();++it)inputs[it.key()]=it.value();
    const auto error=validateArguments(*findCommand("solid.rectangularPattern"),inputs);if(!error.isEmpty())return failure("invalid_arguments",error);
    session_={{"id",CadDocument::newId()},{"command","solid.rectangularPattern"},{"status","active"},{"inputs",inputs},{"activeAxis",0},{"newFeatureId",CadDocument::newId()},{"baseDocumentRevision",static_cast<qint64>(document_->revision())},{"previewed",false},{"geometryEvaluated",false},{"inputSchema",findCommand("solid.rectangularPattern")->parameters}};
    if(inputs.contains("featureId"))view_->setBodyStage(patternInputBodies(),"pattern-source:"+session_["id"].toString());
    selectedBody_.clear();selectedSketch_.clear();selectedProfile_.clear();selectedProfileIds_={};selectedFace_={};selectedPlane_.clear();view_->selectBody({});view_->selectProfile({},{});view_->selectSupportFace({},{});
    showPattern();previewPattern();view_->setFocus();return session_;
}
QJsonObject AlloyWindow::selectPatternFeature(const QString& featureId) {
    if(session_.value("command")!="solid.rectangularPattern")return failure("no_active_pattern","Start Rectangular Pattern before choosing a feature");
    QJsonObject source;for(const auto& entry:document_->authored()["features"].toArray())if(entry.toObject()["id"]==featureId){source=entry.toObject();break;}
    if(source["type"]!="extrude" || source["parameters"].toObject()["operation"]!="join" || source["parameters"].toObject()["targetBodyIds"].toArray().size()!=1)return failure("unsupported_pattern_seed","Select an additive one-target Extrude Join from the timeline");
    const auto edit=session_["inputs"].toObject()["featureId"].toString();if(!edit.isEmpty()){bool earlier=false;for(const auto& entry:document_->authored()["features"].toArray()){if(entry.toObject()["id"]==edit)break;if(entry.toObject()["id"]==featureId)earlier=true;}if(!earlier)return failure("pattern_seed_unavailable","The seed must precede its pattern");}
    auto inputs=session_["inputs"].toObject();inputs["sourceFeatureId"]=featureId;inputs["targetBodyId"]=source["parameters"].toObject()["targetBodyIds"].toArray()[0];inputs["axes"]=QJsonArray{};session_["inputs"]=inputs;session_["activeAxis"]=0;previewPattern();return {{"featureId",featureId},{"kind","feature"},{"inputs",inputs}};
}
QJsonObject AlloyWindow::selectPatternAxis(const QString& edgeId,int index) {
    if(session_.value("command")!="solid.rectangularPattern")return failure("no_active_pattern","Start Rectangular Pattern before choosing an axis");
    auto inputs=session_["inputs"].toObject();auto axes=inputs["axes"].toArray();if(index<0 || index>1 || index>axes.size())return failure("axis_order","Select Axis1 before Axis2");
    const auto bodies=patternInputBodies();const EvaluatedSolidEdge* found=nullptr;
    for(const auto& body:bodies)if(body.bodyId==inputs["targetBodyId"].toString())for(const auto& edge:body.solidEdges)if(edge.edgeId==edgeId && !edge.edge.IsNull())found=&edge;
    if(!found)return failure("pattern_axis_unavailable","Choose a current retained edge on the target body");
    BRepAdaptor_Curve curve(found->edge);if(curve.GetType()!=GeomAbs_Line)return failure("unsupported_pattern_axis","Choose a straight model edge");
    const auto extent=curve.Value(curve.FirstParameter()).Distance(curve.Value(curve.LastParameter()));QJsonObject axis=index<axes.size() ? axes[index].toObject() : QJsonObject{{"quantity",3},{"extentMm",extent}};axis["edgeId"]=edgeId;
    if(index<axes.size())axes[index]=axis;else axes.append(axis);inputs["axes"]=axes;session_["inputs"]=inputs;session_["activeAxis"]=index;previewPattern();return {{"edgeId",edgeId},{"axis",index},{"axes",axes}};
}
QJsonObject AlloyWindow::previewPattern() {
    session_["previewed"]=false;session_["geometryEvaluated"]=false;session_.remove("geometry");session_.remove("documentDraft");view_->clearPreview();view_->clearPatternAxes();
    const auto inputs=session_["inputs"].toObject();const auto axes=inputs["axes"].toArray();QJsonArray edges;for(const auto& axis:axes)edges.append(axis.toObject()["edgeId"]);view_->highlightSolidEdges(patternInputBodies(),inputs["targetBodyId"].toString(),edges);
    if(patternDock_){QString name="Select an Extrude feature from the timeline";for(const auto& f:document_->authored()["features"].toArray())if(f.toObject()["id"]==inputs["sourceFeatureId"]){name=f.toObject()["name"].toString();break;}patternDock_->findChild<QLabel*>("patternSeed")->setText(name);patternDock_->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->setEnabled(false);
        for(int i=0;i<2;++i){auto* q=patternDock_->findChild<QSpinBox*>("patternQuantity"+QString::number(i));auto* e=patternDock_->findChild<QDoubleSpinBox*>("patternExtent"+QString::number(i));const QSignalBlocker qb(q),eb(e);q->setEnabled(i<axes.size());e->setEnabled(i<axes.size());q->setValue(i<axes.size() ? axes[i].toObject()["quantity"].toInt() : 3);e->setValue(i<axes.size() ? axes[i].toObject()["extentMm"].toDouble() : 0);patternDock_->findChild<QPushButton*>("patternAxis"+QString::number(i))->setText(i<axes.size() ? "Change Axis "+QString::number(i+1) : "Select Axis "+QString::number(i+1));}
    }
    auto reject=[this](const QString& code,const QString& message,const QJsonObject& details=QJsonObject{}){auto r=failure(code,message);auto error=r["error"].toObject();error["diagnostics"]=details;r["error"]=error;session_["previewError"]=error;if(patternDock_)patternDock_->findChild<QLabel*>("patternError")->setText(message);return r;};
    if(session_["baseDocumentRevision"].toInteger()!=static_cast<qint64>(document_->revision()))return reject("stale_session","The design changed; cancel and restart Pattern");
    auto candidate=patternCandidate(inputs);const auto evaluated=CadDocument::evaluateCandidate(candidate);if(!evaluated.ok)return reject("evaluation_failed",evaluated.error,evaluated.diagnostics);
    auto stage=evaluated.evaluation;const auto id=inputs.value("featureId").toString(session_["newFeatureId"].toString());
    if(inputs.contains("featureId")){QJsonArray fs;for(const auto& f:candidate["features"].toArray()){fs.append(f);if(f.toObject()["id"]==id)break;}auto prefix=candidate;prefix["features"]=fs;const auto early=CadDocument::evaluateCandidate(prefix);if(!early.ok)return reject("evaluation_failed",early.error,early.diagnostics);stage=early.evaluation;}
    for(const auto& body:stage->bodies)if(body.bodyId==inputs["targetBodyId"].toString()){view_->showPreview(body.shape,body.bodyId,true);view_->showPatternAxes(body.summary);session_["geometry"]=body.summary;session_["bodyId"]=body.bodyId;break;}
    session_["featureId"]=id;session_["documentDraft"]=candidate;session_["previewed"]=true;session_["geometryEvaluated"]=true;session_.remove("previewError");if(patternDock_){patternDock_->findChild<QLabel*>("patternError")->clear();patternDock_->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->setEnabled(true);}return session_;
}
void AlloyWindow::showPattern() {
    if(patternDock_){canvasWindow_->removeDockWidget(patternDock_);delete patternDock_;}patternDock_=new QDockWidget("Rectangular Pattern",canvasWindow_);patternDock_->setObjectName("patternDock");patternDock_->setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);
    auto* scroll=new QScrollArea(patternDock_);scroll->setWidgetResizable(true);scroll->setMinimumWidth(286);patternDock_->setWidget(scroll);auto* panel=new QWidget(scroll);scroll->setWidget(panel);auto* form=new QFormLayout(panel);form->addRow("Object Type",new QLabel("Features",panel));auto* seed=new QLabel(panel);seed->setObjectName("patternSeed");seed->setWordWrap(true);form->addRow("Objects",seed);
    auto* name=new QLineEdit(session_["inputs"].toObject()["name"].toString(),panel);name->setObjectName("patternName");form->addRow("Name",name);connect(name,&QLineEdit::editingFinished,panel,[this,name]{auto inputs=session_["inputs"].toObject();inputs["name"]=name->text();session_["inputs"]=inputs;});form->addRow("Distribution",new QLabel("Extent",panel));
    for(int i=0;i<2;++i){auto* axis=new QPushButton("Select Axis "+QString::number(i+1),panel);axis->setObjectName("patternAxis"+QString::number(i));form->addRow(axis);connect(axis,&QPushButton::clicked,panel,[this,i]{session_["activeAxis"]=i;view_->setFocus();});
        auto* quantity=new QSpinBox(panel);quantity->setObjectName("patternQuantity"+QString::number(i));quantity->setRange(2,100);form->addRow("Quantity "+QString::number(i+1),quantity);auto* extent=new QDoubleSpinBox(panel);extent->setObjectName("patternExtent"+QString::number(i));extent->setRange(-100000,100000);extent->setDecimals(3);extent->setSuffix(" mm");extent->setKeyboardTracking(false);form->addRow("Distance "+QString::number(i+1),extent);form->addRow("Direction "+QString::number(i+1),new QLabel("One Direction",panel));
        auto update=[this,i,quantity,extent]{auto inputs=session_["inputs"].toObject();auto axes=inputs["axes"].toArray();if(i>=axes.size())return;auto axis=axes[i].toObject();axis["quantity"]=quantity->value();axis["extentMm"]=extent->value();axes[i]=axis;const auto result=execute("command.update",{{"session",session_["id"]},{"inputs",QJsonObject{{"axes",axes}}}});if(result["ok"].toBool())previewPattern();};connect(quantity,&QSpinBox::valueChanged,panel,[update]{update();});connect(extent,&QDoubleSpinBox::valueChanged,panel,[update]{update();});
        auto* flip=new QPushButton("Flip Axis "+QString::number(i+1),panel);form->addRow(flip);connect(flip,&QPushButton::clicked,panel,[extent]{extent->setValue(-extent->value());});
    }
    auto* clear=new QPushButton("Remove Axis 2",panel);form->addRow(clear);connect(clear,&QPushButton::clicked,panel,[this]{auto inputs=session_["inputs"].toObject();auto axes=inputs["axes"].toArray();if(axes.size()>1)axes.removeLast();inputs["axes"]=axes;session_["inputs"]=inputs;session_["activeAxis"]=0;previewPattern();});
    form->addRow("Compute Type",new QLabel("Identical",panel));form->addRow("Suppression",new QLabel("Off",panel));auto* error=new QLabel(panel);error->setObjectName("patternError");error->setWordWrap(true);form->addRow(error);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,panel);form->addRow(buttons);connect(buttons,&QDialogButtonBox::accepted,panel,[this,error]{const auto response=execute("command.commit",{{"session",session_["id"]}});if(!response["ok"].toBool())error->setText(response["error"].toObject()["message"].toString());});connect(buttons,&QDialogButtonBox::rejected,panel,[this]{execute("command.cancel",{{"session",session_["id"]}});});auto cancel=[this]{if(session_.value("command")=="solid.rectangularPattern")execute("command.cancel",{{"session",session_["id"]}});};auto* escape=new QShortcut(QKeySequence(Qt::Key_Escape),panel);escape->setContext(Qt::WidgetWithChildrenShortcut);connect(escape,&QShortcut::activated,panel,cancel);if(!patternEscape_){patternEscape_=new QShortcut(QKeySequence(Qt::Key_Escape),view_);patternEscape_->setContext(Qt::WidgetShortcut);connect(patternEscape_,&QShortcut::activated,this,cancel);}patternEscape_->setEnabled(true);canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,patternDock_);patternDock_->show();
}
