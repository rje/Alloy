#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QScrollArea>
#include <QSet>
#include <QShortcut>
#include <QSignalBlocker>
#include <QTimer>
#include <QUuid>
#include <QVBoxLayout>
#include <algorithm>
#include <stdexcept>

namespace {
QJsonObject failure(const QString& code, const QString& message, const QString& field = {}) {
    QJsonObject error{{"code",code},{"message",message}};
    if (!field.isEmpty()) error["field"] = field;
    return {{"ok",false},{"completion","failed"},{"error",error}};
}
class CombineDock final : public QDockWidget {
public:
    using QDockWidget::QDockWidget;
    std::function<void()> onClose;
protected:
    void closeEvent(QCloseEvent* event) override {
        if (onClose) onClose();
        QDockWidget::closeEvent(event);
    }
};
QString bodyLabel(const EvaluatedBody& body) {
    return body.name+" · "+body.bodyId.left(8)+(body.visible ? QString{} : " (hidden)");
}
}

QString AlloyWindow::validateCombineInputs(const QJsonObject& inputs) const {
    const auto error = validateArguments(*findCommand("solid.combine"),inputs);
    if (!error.isEmpty()) return error;
    if (inputs.contains("name") && inputs["name"].toString().trimmed().isEmpty()) return "Combine name must be nonempty";
    auto validId = [](const QString& id) {return !QUuid(id).isNull() && QUuid(id).toString(QUuid::WithoutBraces) == id;};
    const auto target = inputs["targetBodyId"].toString();
    if (!target.isEmpty() && !validId(target)) return "Target Body requires a stable body UUID";
    if (inputs.contains("featureId") && !inputs["featureId"].toString().isEmpty() && !validId(inputs["featureId"].toString()))
        return "Combine edit requires a stable feature UUID";
    QSet<QString> selected;
    if (!target.isEmpty()) selected.insert(target);
    for (const auto& item : inputs["toolBodyIds"].toArray()) {
        if (!item.isString() || !validId(item.toString())) return "Tool Bodies require stable body UUIDs";
        if (selected.contains(item.toString())) return "Target Body and Tool Bodies must be distinct; duplicate tools are unsupported";
        selected.insert(item.toString());
    }
    return {};
}

QJsonObject AlloyWindow::combineCandidate(const QJsonObject& inputs) const {
    QJsonObject arguments{{"operation",inputs["operation"]},{"targetBodyId",inputs["targetBodyId"]},
        {"toolBodyIds",inputs["toolBodyIds"]},{"keepTools",inputs.value("keepTools").toBool(false)}};
    if (inputs.contains("name")) arguments["name"] = inputs["name"];
    const auto featureId = inputs.value("featureId").toString();
    if (!featureId.isEmpty()) arguments["featureId"] = featureId;
    else if (session_.value("command") == "solid.combine") arguments["newFeatureId"] = session_["newFeatureId"];
    return CadDocument::booleanCandidate(document_->authored(),arguments);
}

std::vector<EvaluatedBody> AlloyWindow::combineInputBodies() const {
    const auto featureId = session_.value("inputs").toObject().value("featureId").toString();
    if (featureId.isEmpty()) return document_->bodies();
    auto prefix = document_->authored();QJsonArray features;bool found = false;
    for (const auto& entry : prefix["features"].toArray()) {
        if (entry.toObject()["id"] == featureId) {found = true;break;}
        features.append(entry);
    }
    if (!found) throw std::runtime_error("The edited Combine feature is no longer available");
    prefix["features"] = features;
    const auto evaluated = CadDocument::evaluateCandidate(prefix);
    if (!evaluated.ok) throw std::runtime_error(evaluated.error.toStdString());
    return evaluated.evaluation->bodies;
}

QJsonObject AlloyWindow::beginCombine(const QJsonObject& overrides) {
    if (!session_.isEmpty()) return failure("session_busy","Complete or cancel the active command first");
    QJsonObject inputs{{"targetBodyId",""},{"toolBodyIds",QJsonArray{}},{"operation","join"},{"keepTools",false},{"name","Combine"}};
    for (auto it = overrides.begin(); it != overrides.end(); ++it) inputs[it.key()] = it.value();
    const auto error = validateCombineInputs(inputs);
    if (!error.isEmpty()) return failure("invalid_arguments",error);
    const auto featureId = inputs.value("featureId").toString();
    if (!featureId.isEmpty()) {
        bool found = false;
        for (const auto& entry : document_->authored()["features"].toArray()) {
            const auto feature = entry.toObject();
            found = found || (feature["id"] == featureId && feature["type"] == "boolean");
        }
        if (!found) return failure("stale_entity","Combine feature does not exist","featureId");
    }
    session_ = {{"id",CadDocument::newId()},{"command","solid.combine"},{"status","active"},{"inputs",inputs},
        {"previewed",false},{"geometryEvaluated",false},{"baseDocumentRevision",static_cast<qint64>(document_->revision())},
        {"inputSchema",findCommand("solid.combine")->parameters},{"selectionRole","target"}};
    if (featureId.isEmpty()) session_["newFeatureId"] = CadDocument::newId();
    if (inputs["targetBodyId"].toString().isEmpty() && !selectedBody_.isEmpty()) {
        const auto candidates = combineInputBodies();
        if (std::any_of(candidates.begin(),candidates.end(),[&](const auto& body){return body.bodyId == selectedBody_;})
            && !inputs["toolBodyIds"].toArray().contains(selectedBody_)) {
            inputs["targetBodyId"] = selectedBody_;session_["inputs"] = inputs;
        }
    }
    if (!inputs["targetBodyId"].toString().isEmpty()) session_["selectionRole"] = "tools";
    selectedSketch_.clear();selectedProfile_.clear();view_->selectProfile({},{});view_->selectBody({});
    showCombine();previewCombine();refreshCombineControls();
    return session_;
}

QJsonObject AlloyWindow::previewCombine() {
    session_["previewed"] = false;session_["geometryEvaluated"] = false;
    session_.remove("geometry");session_.remove("documentDraft");session_.remove("previewError");session_.remove("finalBodyCount");
    view_->clearPreview();view_->setEditDisplay({},{});
    auto reject = [this](QJsonObject response) {
        view_->clearPreview();view_->setEditDisplay({},{});
        session_["previewError"] = response["error"];refreshCombineControls();return response;
    };
    if (session_.value("command") != "solid.combine") return failure("invalid_session","Combine is not active");
    if (session_.value("baseDocumentRevision").toInteger() != static_cast<qint64>(document_->revision()))
        return reject(failure("stale_session","The document changed; cancel and restart Combine"));
    const auto inputs = session_.value("inputs").toObject();
    if (inputs["targetBodyId"].toString().isEmpty()) return reject(failure("target_required","Select Target Body","targetBodyId"));
    if (inputs["toolBodyIds"].toArray().isEmpty()) return reject(failure("tools_required","Select at least one Tool Body","toolBodyIds"));
    QJsonObject candidate;
    try {candidate = combineCandidate(inputs);}
    catch (const std::exception& error) {return reject(failure("evaluation_failed",QString::fromUtf8(error.what())));}
    const auto evaluated = CadDocument::evaluateCandidate(candidate);
    if (!evaluated.ok) {
        auto response = failure("evaluation_failed",evaluated.error);auto error = response["error"].toObject();
        error["diagnostics"] = evaluated.diagnostics;response["error"] = error;return reject(response);
    }
    const auto featureId = inputs.value("featureId").toString(session_.value("newFeatureId").toString());
    auto displayEvaluation = evaluated.evaluation;
    // Edit previews show this operation's history stage. Full downstream
    // evaluation above still determines whether the entire candidate is valid.
    if (!inputs.value("featureId").toString().isEmpty()) {
        auto prefix = evaluated.authored;QJsonArray features;
        for (const auto& entry : prefix["features"].toArray()) {
            features.append(entry);if (entry.toObject()["id"] == featureId) break;
        }
        prefix["features"] = features;
        const auto stage = CadDocument::evaluateCandidate(prefix);
        if (!stage.ok) return reject(failure("evaluation_failed",stage.error));
        displayEvaluation = stage.evaluation;
    }
    const auto targetId = inputs["targetBodyId"].toString();
    const auto found = std::find_if(displayEvaluation->bodies.begin(),displayEvaluation->bodies.end(),
        [&](const auto& body){return body.bodyId == targetId;});
    if (found == displayEvaluation->bodies.end()) return reject(failure("evaluation_failed","The evaluated Combine target result is unavailable"));
    const auto stageKey = "combine:"+QString::number(document_->revision())+":"+featureId+":result:"
        +QString::fromUtf8(QJsonDocument(inputs).toJson(QJsonDocument::Compact));
    view_->setBodyStage(displayEvaluation->bodies,stageKey);
    view_->setEditDisplay({},inputs["toolBodyIds"].toArray());view_->showPreview(found->shape,targetId,true);
    session_["previewed"] = true;session_["geometryEvaluated"] = true;session_["geometry"] = found->summary;
    session_["featureId"] = featureId;session_["bodyId"] = targetId;session_["documentDraft"] = evaluated.authored;
    session_["previewStage"] = "through_feature";
    session_["presentationStage"] = "through_feature";
    session_["finalBodyCount"] = static_cast<int>(evaluated.evaluation->bodies.size());
    refreshCombineControls();return session_;
}

void AlloyWindow::showCombine() {
    if (combineEscape_) {delete combineEscape_;combineEscape_ = nullptr;}
    if (combineDock_) delete combineDock_;
    auto* dock = new CombineDock("Combine",canvasWindow_);combineDock_ = dock;dock->setObjectName("combineDock");
    dock->setAllowedAreas(Qt::LeftDockWidgetArea|Qt::RightDockWidgetArea);
    combinePanel_ = new QWidget;combinePanel_->setObjectName("combinePanel");combinePanel_->setFixedWidth(312);
    combinePanel_->setStyleSheet("#combinePanel { background:#f7f8f9; color:#323c44; border-left:1px solid #cdd3d8; }");dock->setWidget(combinePanel_);
    dock->onClose = [this]{if (session_.value("command") == "solid.combine") execute("command.cancel",{{"session",session_["id"]}});};
    auto* panelLayout = new QVBoxLayout(combinePanel_);panelLayout->setContentsMargins(0,0,0,12);panelLayout->setSpacing(12);
    auto* scroll = new QScrollArea;scroll->setWidgetResizable(true);scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto* contents = new QWidget;scroll->setWidget(contents);panelLayout->addWidget(scroll,1);
    auto* layout = new QVBoxLayout(contents);layout->setContentsMargins(14,14,14,0);layout->setSpacing(12);
    auto* form = new QFormLayout;form->setVerticalSpacing(12);form->setRowWrapPolicy(QFormLayout::WrapAllRows);
    combineTarget_ = new QComboBox;combineTarget_->setObjectName("combine.targetBody");form->addRow("Target Body",combineTarget_);
    combinePickTarget_ = new QPushButton("Select Target Body");combinePickTarget_->setCheckable(true);combinePickTarget_->setObjectName("combine.pickTarget");
    form->addRow(combinePickTarget_);
    combineTools_ = new QListWidget;combineTools_->setObjectName("combine.toolBodies");combineTools_->setMinimumHeight(125);combineTools_->setMaximumHeight(205);
    form->addRow("Tool Bodies",combineTools_);
    combinePickTools_ = new QPushButton("Select Tool Bodies");combinePickTools_->setCheckable(true);combinePickTools_->setObjectName("combine.pickTools");form->addRow(combinePickTools_);
    combineOperation_ = new QComboBox;combineOperation_->setObjectName("combine.operation");
    combineOperation_->addItem("Join","join");combineOperation_->addItem("Cut","cut");combineOperation_->addItem("Intersect","intersect");
    form->addRow("Operation",combineOperation_);
    combineKeepTools_ = new QCheckBox("Keep Tools");combineKeepTools_->setObjectName("combine.keepTools");form->addRow(combineKeepTools_);layout->addLayout(form);
    combineError_ = new QLabel;combineError_->setObjectName("combine.error");combineError_->setWordWrap(true);combineError_->setStyleSheet("color:#a63832;");layout->addWidget(combineError_);
    auto* hint = new QLabel("Choose a target, then one or more tools.\nTools are hidden during preview. Keep Tools retains them after OK.");
    hint->setWordWrap(true);hint->setStyleSheet("color:#687780;");layout->addWidget(hint);layout->addStretch();
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);combineOk_ = buttons->button(QDialogButtonBox::Ok);
    buttons->setContentsMargins(14,0,14,0);panelLayout->addWidget(buttons);
    combineOk_->setObjectName("combine.ok");buttons->button(QDialogButtonBox::Cancel)->setObjectName("combine.cancel");
    connect(combineTarget_,qOverload<int>(&QComboBox::currentIndexChanged),this,[this](int index){
        const auto target = combineTarget_->itemData(index).toString();auto tools = session_["inputs"].toObject()["toolBodyIds"].toArray();
        for (int i = tools.size()-1; i >= 0; --i) if (tools[i] == target) tools.removeAt(i);
        updateCombine({{"targetBodyId",target},{"toolBodyIds",tools}});
    });
    connect(combineTools_,&QListWidget::itemChanged,this,[this](QListWidgetItem*){
        QJsonArray tools;for (int i = 0; i < combineTools_->count(); ++i) {
            const auto* item = combineTools_->item(i);if (item->checkState() == Qt::Checked) tools.append(item->data(Qt::UserRole).toString());
        }
        // Refresh rebuilds this list. Let Qt finish the checkbox event before
        // deleting its item, and discard work from a session already closed.
        const auto sessionId = session_.value("id").toString();
        QTimer::singleShot(0,this,[this,tools,sessionId]{
            if (session_.value("command") == "solid.combine" && session_.value("id").toString() == sessionId)
                updateCombine({{"toolBodyIds",tools}});
        });
    });
    connect(combineOperation_,qOverload<int>(&QComboBox::currentIndexChanged),this,[this](int index){updateCombine({{"operation",combineOperation_->itemData(index).toString()}});});
    connect(combineKeepTools_,&QCheckBox::toggled,this,[this](bool keep){updateCombine({{"keepTools",keep}});});
    // Accessibility exposes these checkable buttons as toggles. Qt's toggle
    // action emits toggled, not clicked; both inputs must change the same role.
    connect(combinePickTarget_,&QPushButton::toggled,this,[this](bool checked){if(checked)execute("solid.combineSelectionMode",{{"role","target"}});refreshCombineControls();});
    connect(combinePickTools_,&QPushButton::toggled,this,[this](bool checked){if(checked)execute("solid.combineSelectionMode",{{"role","tools"}});refreshCombineControls();});
    connect(buttons,&QDialogButtonBox::accepted,this,[this]{
        const auto response = execute("command.commit",{{"session",session_["id"]}});
        if (!response["ok"].toBool()) {
            session_["previewed"] = false;session_["geometryEvaluated"] = false;session_["previewError"] = response["error"];
            session_.remove("geometry");session_.remove("documentDraft");session_.remove("finalBodyCount");
            view_->clearPreview();view_->setEditDisplay({},{});refreshCombineControls();
        }
    });
    connect(buttons,&QDialogButtonBox::rejected,this,[this]{execute("command.cancel",{{"session",session_["id"]}});});
    auto* escape = new QShortcut(QKeySequence(Qt::Key_Escape),combinePanel_);escape->setContext(Qt::WidgetWithChildrenShortcut);
    connect(escape,&QShortcut::activated,this,[this]{execute("command.cancel",{{"session",session_["id"]}});});
    combineEscape_ = new QShortcut(QKeySequence(Qt::Key_Escape),view_);combineEscape_->setContext(Qt::WidgetShortcut);
    connect(combineEscape_,&QShortcut::activated,this,[this]{if (session_["command"] == "solid.combine") execute("command.cancel",{{"session",session_["id"]}});});
    canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,dock);dock->show();refreshCombineControls();
}

void AlloyWindow::refreshCombineControls() {
    if (!combinePanel_ || session_.value("command") != "solid.combine") return;
    const auto inputs = session_.value("inputs").toObject();const auto targetId = inputs["targetBodyId"].toString();const auto tools = inputs["toolBodyIds"].toArray();
    std::vector<EvaluatedBody> available;bool inputsAvailable = true;
    try {available = combineInputBodies();}
    catch (const std::exception& error) {
        inputsAvailable = false;
        session_["previewed"] = false;session_["geometryEvaluated"] = false;
        session_.remove("geometry");session_.remove("documentDraft");session_.remove("finalBodyCount");
        session_["previewError"] = QJsonObject{{"message",QString::fromUtf8(error.what())}};
        view_->clearPreview();view_->setEditDisplay({},{});view_->clearBodyStage();
    }
    {
        const QSignalBlocker block(combineTarget_);combineTarget_->clear();combineTarget_->addItem("Select Target Body…",QString{});int selected = 0;
        for (const auto& body : available) {
            combineTarget_->addItem(bodyLabel(body),body.bodyId);
            if (body.bodyId == targetId) selected = combineTarget_->count()-1;
        }
        if (!targetId.isEmpty() && selected == 0) {combineTarget_->addItem("Unavailable · "+targetId.left(8),targetId);selected = combineTarget_->count()-1;}
        combineTarget_->setCurrentIndex(selected);
    }
    {
        const QSignalBlocker block(combineTools_);combineTools_->clear();QSet<QString> listed;
        for (const auto& body : available) {
            auto* item = new QListWidgetItem(bodyLabel(body),combineTools_);item->setData(Qt::UserRole,body.bodyId);listed.insert(body.bodyId);
            item->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable|Qt::ItemIsUserCheckable);
            item->setCheckState(tools.contains(body.bodyId) ? Qt::Checked : Qt::Unchecked);
            if (body.bodyId == targetId) {item->setFlags(Qt::NoItemFlags);item->setToolTip("Target Body cannot also be a Tool Body");}
        }
        for (const auto& tool : tools) if (!listed.contains(tool.toString())) {
            auto* item = new QListWidgetItem("Unavailable · "+tool.toString().left(8),combineTools_);item->setData(Qt::UserRole,tool.toString());
            item->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable|Qt::ItemIsUserCheckable);item->setCheckState(Qt::Checked);
        }
    }
    {const QSignalBlocker block(combineOperation_);combineOperation_->setCurrentIndex(combineOperation_->findData(inputs["operation"].toString()));}
    {const QSignalBlocker block(combineKeepTools_);combineKeepTools_->setChecked(inputs["keepTools"].toBool());}
    const auto role = session_.value("selectionRole").toString();
    {const QSignalBlocker block(combinePickTarget_);combinePickTarget_->setChecked(role == "target");}
    {const QSignalBlocker block(combinePickTools_);combinePickTools_->setChecked(role == "tools");}
    combinePickTools_->setText(QString("Select Tool Bodies (%1)").arg(tools.size()));
    combineError_->setText(session_.value("previewError").toObject().value("message").toString());
    const bool current = session_.value("baseDocumentRevision").toInteger() == static_cast<qint64>(document_->revision());
    combineOk_->setEnabled(session_.value("previewed").toBool() && current);
    if (current && inputsAvailable && !session_.value("previewed").toBool()) {
        const auto featureId = inputs.value("featureId").toString(session_.value("newFeatureId").toString());
        const auto stageKey = "combine:"+QString::number(document_->revision())+":"+featureId+":inputs";
        view_->setBodyStage(available,stageKey);session_["presentationStage"] = "before_feature";
    } else if (!current || !inputsAvailable) {
        view_->clearBodyStage();session_["presentationStage"] = "current_document";
    }
    view_->highlightCombineInputs(current && inputsAvailable ? targetId : QString{},current && inputsAvailable ? tools : QJsonArray{});
}

void AlloyWindow::updateCombine(const QJsonObject& inputs) {
    auto response = execute("command.update",{{"session",session_["id"]},{"inputs",inputs}});
    if (response["ok"].toBool()) response = execute("command.preview",{{"session",session_["id"]}});
    if (!response["ok"].toBool()) {
        session_["previewed"] = false;session_["geometryEvaluated"] = false;
        session_.remove("geometry");session_.remove("documentDraft");session_.remove("finalBodyCount");
        session_["previewError"] = response["error"];view_->clearPreview();view_->setEditDisplay({},{});
    }
    refreshCombineControls();
}

void AlloyWindow::selectCombineBody(const QString& bodyId) {
    if (session_.value("command") != "solid.combine" || bodyId.isEmpty()) return;
    const auto available = combineInputBodies();
    if (!std::any_of(available.begin(),available.end(),[&](const auto& body){return body.bodyId == bodyId;})) {
        session_["previewError"] = QJsonObject{{"message","This body is unavailable at the edited Combine history stage"}};
        session_["previewed"] = false;session_["geometryEvaluated"] = false;
        session_.remove("geometry");session_.remove("documentDraft");session_.remove("finalBodyCount");
        view_->clearPreview();view_->setEditDisplay({},{});refreshCombineControls();return;
    }
    const auto inputs = session_["inputs"].toObject();auto tools = inputs["toolBodyIds"].toArray();
    if (session_.value("selectionRole") == "target") {
        for (int i = tools.size()-1; i >= 0; --i) if (tools[i] == bodyId) tools.removeAt(i);
        session_["selectionRole"] = "tools";updateCombine({{"targetBodyId",bodyId},{"toolBodyIds",tools}});
    } else {
        if (inputs["targetBodyId"] == bodyId) {
            session_["previewError"] = QJsonObject{{"message","Target Body cannot also be a Tool Body"}};
            session_["previewed"] = false;session_["geometryEvaluated"] = false;
            session_.remove("geometry");session_.remove("documentDraft");session_.remove("finalBodyCount");
            view_->clearPreview();view_->setEditDisplay({},{});refreshCombineControls();return;
        }
        const auto index = std::find(tools.begin(),tools.end(),QJsonValue(bodyId));
        if (index != tools.end()) tools.erase(index);else tools.append(bodyId);
        updateCombine({{"toolBodyIds",tools}});
    }
}
