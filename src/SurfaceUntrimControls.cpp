// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include "SurfaceUntrim.h"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QShortcut>

namespace {
QJsonObject failure(const QString& code,const QString& message){return {{"ok",false},{"error",QJsonObject{{"code",code},{"message",message}}}};}
}
QJsonObject AlloyWindow::surfaceUntrimCandidate(const QJsonObject& inputs) const {
    auto args=inputs;
    if(!args.contains("featureId") && session_.value("command")=="surface.untrim")args["newFeatureId"]=session_.value("newFeatureId");
    args=authoringArguments(args);if(args.contains("featureId"))args.remove("ownerId");
    return CadDocument::surfaceUntrimCandidate(document_->authored(),args);
}
QJsonObject AlloyWindow::beginSurfaceUntrim(const QJsonObject& overrides) {
    if(!session_.isEmpty())return failure("session_busy","Complete or cancel the active command first");
    QJsonObject inputs{{"untrimType","externalEdges"},{"name","Untrim"}};
    if(overrides.contains("featureId")){
        bool found=false;for(const auto& value:document_->authored().value("features").toArray()){
            const auto f=value.toObject();if(f.value("id")!=overrides.value("featureId"))continue;
            if(f.value("type")!="surfaceUntrim")return failure("stale_entity","Untrim feature unavailable");
            const auto p=f.value("parameters").toObject();
            for(const auto& key:{"sourceFace","trimFeatureId","untrimType"})inputs[key]=p.value(key);
            inputs["featureId"]=f.value("id");inputs["name"]=f.value("name");found=true;break;
        }
        if(!found)return failure("stale_entity","Untrim feature unavailable");
    }
    for(auto i=overrides.begin();i!=overrides.end();++i)inputs[i.key()]=i.value();
    const auto error=validateArguments(*findCommand("surface.untrimDialog"),inputs);if(!error.isEmpty())return failure("invalid_arguments",error);
    surfaceUntrimPreviewSuppressed_=false;
    session_={{"id",CadDocument::newId()},{"command","surface.untrim"},{"status","active"},{"inputs",inputs},
        {"newFeatureId",CadDocument::newId()},{"baseDocumentRevision",static_cast<qint64>(document_->revision())},
        {"previewed",false},{"geometryEvaluated",false},{"previewSuppressed",false},
        {"inputSchema",findCommand("surface.untrim")->parameters},{"nativeControlsAvailable",false},
        {"canvasSelectionAvailable",true},{"canvasManipulatorsAvailable",false},
        {"supportedUntrimTypes",QJsonArray{"externalEdges"}}};
    showSurfaceUntrim();previewSurfaceUntrim();return session_;
}
QJsonObject AlloyWindow::previewSurfaceUntrim() {
    if(session_.value("command")!="surface.untrim")return failure("invalid_session","Untrim is not active");
    session_["previewed"]=false;session_["geometryEvaluated"]=false;session_.remove("geometry");session_.remove("documentDraft");view_->clearPreview();view_->selectSupportFace({},{});view_->clearSolidEdgeHighlights();
    auto reject=[&](const QString& code,const QString& message,const QJsonObject& details=QJsonObject{}){
        auto result=failure(code,message);auto error=result.value("error").toObject();error["diagnostics"]=details;
        result["error"]=error;session_["previewError"]=error;refreshSurfaceUntrimControls();return result;
    };
    if(session_.value("baseDocumentRevision").toInteger()!=static_cast<qint64>(document_->revision()))return reject("stale_session","The document changed; cancel and restart Untrim");
    const auto inputs=session_.value("inputs").toObject();const auto edit=inputs.value("featureId").toString();
    const auto source=edit.isEmpty()?document_->evaluation():document_->evaluationBefore(edit);
    if(!edit.isEmpty())view_->setBodyStage(source.bodies,"surface-untrim-source:"+session_.value("id").toString());
    const auto ref=inputs.value("sourceFace").toObject();
    if(ref.isEmpty())return reject("selection_required","Select one retained planar Trim-result surface face");
    try{SurfaceUntrim::resolveSource(source,ref,inputs.value("trimFeatureId").toString());}
    catch(const GeometryKernel::EvaluationError& e){return reject(e.diagnostics().value("code").toString(),QString::fromUtf8(e.what()),e.diagnostics());}
    for(const auto& body:source.bodies)if(body.bodyId==ref.value("bodyId").toString())for(const auto& face:body.supportFaces)
        if(face.faceId==ref.value("faceId").toString() && face.producerFeatureId==ref.value("producerFeatureId").toString()){
            view_->selectSupportFace(ref,face.face);QJsonArray edges;for(const auto& edge:face.boundaryEdges)edges.append(edge.edgeId);
            view_->highlightSolidEdges(source.bodies,body.bodyId,edges);
        }
    QJsonObject candidate;try{candidate=surfaceUntrimCandidate(inputs);}catch(const std::exception& e){return reject("invalid_arguments",QString::fromUtf8(e.what()));}
    const auto evaluated=CadDocument::evaluateCandidate(candidate);if(!evaluated.ok)return reject("evaluation_failed",evaluated.error,evaluated.diagnostics);
    auto stage=evaluated.evaluation;const auto id=edit.isEmpty()?session_.value("newFeatureId").toString():edit;
    if(!edit.isEmpty()){
        auto prefix=candidate;QJsonArray rows;for(const auto& value:candidate.value("features").toArray()){rows.append(value);if(value.toObject().value("id")==id)break;}prefix["features"]=rows;
        const auto early=CadDocument::evaluateCandidate(prefix);if(!early.ok)return reject("evaluation_failed",early.error,early.diagnostics);stage=early.evaluation;
    }
    bool shown=false;
    for(const auto& body:stage->bodies)if(body.bodyId==ref.value("bodyId").toString()){
        if(!surfaceUntrimPreviewSuppressed_)view_->showPreview(body.shape,body.bodyId,true);
        session_["geometry"]=body.summary;session_["featureId"]=id;session_["bodyId"]=body.bodyId;shown=true;
    }
    if(!shown)return reject("surface_source_unavailable","The restored surface body is unavailable");
    session_["documentDraft"]=candidate;session_["previewed"]=true;session_["geometryEvaluated"]=true;
    session_["previewSuppressed"]=surfaceUntrimPreviewSuppressed_;session_.remove("previewError");refreshSurfaceUntrimControls();return session_;
}
QJsonObject AlloyWindow::selectSurfaceUntrimFace(const QJsonObject& face){
    if(!face.value("occurrenceId").toString().isEmpty())return failure("unsupported_instance_edit","Select the retained source face");
    QJsonObject exact{{"bodyId",face.value("bodyId")},{"producerFeatureId",face.value("producerFeatureId")},{"faceId",face.value("faceId")}};
    selectedFace_=exact;selectedBody_.clear();selectedOccurrence_.clear();selectedEdge_={};selectedSketch_.clear();selectedProfile_.clear();selectedProfileIds_={};selectedPlane_.clear();
    view_->selectBody({});view_->selectProfile({},{});view_->clearSolidEdgeHighlights();
    return execute("command.update",{{"session",session_.value("id")},{"inputs",QJsonObject{{"sourceFace",exact},{"trimFeatureId",exact.value("producerFeatureId")}}}});
}
void AlloyWindow::refreshSurfaceUntrimControls(){
    if(!surfaceUntrimDock_ || session_.value("command")!="surface.untrim")return;
    const auto inputs=session_.value("inputs").toObject();
    surfaceUntrimDock_->findChild<QLabel*>("surfaceUntrim.selection")->setText(inputs.value("sourceFace").toObject().isEmpty()?"0 selected":"1 face selected");
    surfaceUntrimDock_->findChild<QLabel*>("surfaceUntrim.error")->setText(session_.value("previewError").toObject().value("message").toString());
    surfaceUntrimDock_->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->setEnabled(session_.value("previewed").toBool());
}
void AlloyWindow::showSurfaceUntrim(){
    if(surfaceUntrimDock_){canvasWindow_->removeDockWidget(surfaceUntrimDock_);delete surfaceUntrimDock_;}
    surfaceUntrimDock_=new QDockWidget("Untrim",canvasWindow_);surfaceUntrimDock_->setObjectName("surfaceUntrimDock");
    auto* panel=new QWidget;surfaceUntrimDock_->setWidget(panel);auto* form=new QFormLayout(panel);
    const auto sessionId=session_.value("id").toString();
    auto update=[this,sessionId](const QJsonObject& inputs){if(session_.value("id")==sessionId)execute("command.update",{{"session",sessionId},{"inputs",inputs}});};
    auto* name=new QLineEdit(session_.value("inputs").toObject().value("name").toString());name->setObjectName("surfaceUntrim.name");form->addRow("Name",name);
    connect(name,&QLineEdit::editingFinished,panel,[update,name]{update({{"name",name->text()}});});
    auto* selection=new QLabel;selection->setObjectName("surfaceUntrim.selection");form->addRow("Faces",selection);
    auto* clear=new QPushButton("Clear");clear->setObjectName("surfaceUntrim.clear");form->addRow(clear);
    connect(clear,&QPushButton::clicked,panel,[update]{update({{"sourceFace",QJsonObject{}},{"trimFeatureId",""}});});
    auto* type=new QComboBox;type->setObjectName("surfaceUntrim.untrimType");type->addItem("External Edges","externalEdges");type->setEnabled(false);form->addRow("Type",type);
    auto* hint=new QLabel("Select one planar Trim-result face to restore its retained finite pretrim boundary. All Edges, Internal Edges, Manual, extra extension and Keep Copy are not supported.");hint->setWordWrap(true);form->addRow(hint);
#ifdef Q_OS_MACOS
    auto* gesture=new QLabel("Hold Command while adjusting selection to suppress the preview.");
#else
    auto* gesture=new QLabel("Hold Ctrl while adjusting selection to suppress the preview.");
#endif
    gesture->setWordWrap(true);form->addRow(gesture);
    auto* error=new QLabel;error->setObjectName("surfaceUntrim.error");error->setWordWrap(true);error->setStyleSheet("color:#b3261e");form->addRow(error);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);form->addRow(buttons);
    connect(buttons,&QDialogButtonBox::accepted,panel,[this,sessionId]{execute("command.commit",{{"session",sessionId}});});
    connect(buttons,&QDialogButtonBox::rejected,panel,[this,sessionId]{execute("command.cancel",{{"session",sessionId}});});
    if(!surfaceUntrimEscape_){surfaceUntrimEscape_=new QShortcut(QKeySequence(Qt::Key_Escape),this);connect(surfaceUntrimEscape_,&QShortcut::activated,this,[this]{if(session_.value("command")=="surface.untrim")execute("command.cancel",{{"session",session_.value("id")}});});}
    surfaceUntrimEscape_->setEnabled(true);canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,surfaceUntrimDock_);surfaceUntrimDock_->show();refreshSurfaceUntrimControls();
}
