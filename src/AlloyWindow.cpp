#include "AlloyWindow.h"
#include "ReferenceView.h"
#include "GeometryKernel.h"
#include <QApplication>
#include <QCryptographicHash>
#include <QDialog>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QDir>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QSaveFile>
#include <QSignalBlocker>
#include <QShortcut>
#include <QStatusBar>
#include <QTimer>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <Standard_Failure.hxx>
#include <algorithm>
#include <cmath>

namespace {
QJsonObject failure(const QString& code, const QString& message, const QString& field = {}) {
    QJsonObject error{{"code", code}, {"message", message}};
    if (!field.isEmpty()) error["field"] = field;
    return {{"ok", false}, {"completion", "failed"},
        {"error", error}};
}
QJsonObject parameter(const QString& type, const QString& description) {
    return {{"type", type}, {"description", description}, {"required", true}};
}
QWidget* divider() {
    auto* line = new QFrame;
    line->setFrameShape(QFrame::VLine);
    line->setStyleSheet("color: #d1d4d6;");
    return line;
}
}

AlloyWindow::AlloyWindow() {
    setWindowTitle("Untitled — Alloy");
    resize(1440, 900);
    setMinimumSize(1040, 640);
    setStyleSheet(R"(
        QMainWindow, QWidget { color: #323c44; font-size: 12px; }
        QMainWindow { background: #f4f5f6; }
        QPushButton, QToolButton { border: 1px solid transparent; border-radius: 3px; padding: 6px 9px; background: transparent; }
        QPushButton:hover, QToolButton:hover { background: #e3edf3; border-color: #bfd0dd; }
        QPushButton:pressed, QToolButton:pressed { background: #d3e5f0; }
        QPushButton:disabled, QToolButton:disabled { color: #939ca3; }
        QMenu { background: #fff; border: 1px solid #b9c1c7; padding: 5px; }
        QMenu::item { padding: 6px 22px; } QMenu::item:selected { background: #deedf5; }
        QDialog { background: #f7f8f9; }
        QComboBox { background: white; color: #323c44; border: 1px solid #9aabb8; padding: 6px; }
        QComboBox QAbstractItemView { background: white; color: #323c44; selection-background-color: #deedf5; }
        QDoubleSpinBox { background: white; color: #323c44; border: 1px solid #9aabb8; padding: 6px; }
        QTreeWidget { border: 0; background: #fafbfc; outline: 0; }
        QTreeWidget::item { height: 27px; } QTreeWidget::item:selected { background: #deedf5; color: #263944; }
        QLineEdit { border: 1px solid #9aabb8; border-radius: 3px; padding: 8px; background: white; }
        QListWidget { background: white; border: 1px solid #d4dade; } QListWidget::item { padding: 8px; }
        QStatusBar { background: #edf0f2; border-top: 1px solid #d6dce1; color: #67757e; }
    )");
    view_ = new ReferenceView;
    view_->dispatch = [this](const QString& command, const QJsonObject& args) { execute(command, args); };
    registerCommands();
    auto* root = new QWidget;
    auto* layout = new QVBoxLayout(root);
    layout->setContentsMargins(0, 0, 0, 0); layout->setSpacing(0);
    setCentralWidget(root);

    auto* appBar = new QWidget;
    appBar->setFixedHeight(43);
    appBar->setStyleSheet("background: #e8ebed; border-bottom: 1px solid #cdd3d8;");
    auto* top = new QHBoxLayout(appBar); top->setContentsMargins(12, 2, 12, 2); top->setSpacing(4);
    auto* brand = new QLabel("ALLOY");
    brand->setStyleSheet("font-size: 16px; font-weight: 700; letter-spacing: 2px; color: #235d79; border: 0; padding-right: 12px;");
    top->addWidget(brand);
    auto* file = new QPushButton("File");
    auto* fileMenu = new QMenu(file);
    fileMenu->addAction("New",QKeySequence::New,this,[this] { if(confirmDiscard()) execute("document.new",{{"discard",true}}); });
    fileMenu->addAction("Open…",QKeySequence::Open,this,[this] { execute("document.openDialog"); });
    fileMenu->addAction("Save",QKeySequence::Save,this,[this] { execute("document.saveDialog"); });
    fileMenu->addAction("Save As…",QKeySequence::SaveAs,this,[this] { execute("document.saveAsDialog"); });
    fileMenu->addAction("Document Settings…", this, [this] { execute("document.settings"); });
    fileMenu->addSeparator();
    fileMenu->addAction("Quit", this, [this] { close(); });
    file->setMenu(fileMenu); top->addWidget(file);
    top->addWidget(commandButton("document.saveDialog", "Save"));
    top->addWidget(commandButton("history.undo", "↶"));
    top->addWidget(commandButton("history.redo", "↷"));
    top->addStretch();
    auto* title = new QLabel("◇   Untitled"); documentTitle_=title; title->setMinimumWidth(280); title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("background: #f7f8f9; border: 0; border-radius: 4px; padding: 7px;");
    top->addWidget(title); top->addStretch();
    top->addWidget(commandButton("commands.search", "Search  S"));
    auto* local = new QLabel("●  LOCAL"); local->setStyleSheet("border: 0; color: #51796b; padding: 8px;");
    top->addWidget(local);
    layout->addWidget(appBar);

    auto* ribbon = new QWidget;
    ribbon->setFixedHeight(107);
    ribbon->setStyleSheet("background: #f7f8f9;");
    auto* ribbonLayout = new QHBoxLayout(ribbon); ribbonLayout->setContentsMargins(14, 0, 12, 6);
    auto* workspace = new QLabel("DESIGN"); workspace->setAlignment(Qt::AlignCenter); workspace->setFixedWidth(110);
    workspace->setStyleSheet("font-weight: 600; font-size: 13px; border: 1px solid #c7cfd5; border-radius: 4px; padding: 22px 8px;");
    ribbonLayout->addWidget(workspace);
    auto* ribbonRight = new QVBoxLayout; ribbonRight->setSpacing(0);
    auto* tabs = new QHBoxLayout; tabs->setSpacing(18);
    for (const QString& tab : {"SOLID", "SURFACE", "MESH", "SHEET METAL", "TOOLS"}) {
        auto* label = new QLabel(tab); label->setFixedHeight(30);
        label->setStyleSheet(tab == "SOLID" ? "font-weight: 700; border-bottom: 2px solid #2b83ae; padding: 0 10px;" : "color: #87929a; padding: 0 10px;");
        if (tab != "SOLID") label->setToolTip("Workspace tools are not implemented yet");
        tabs->addWidget(label);
    }
    tabs->addStretch(); ribbonRight->addLayout(tabs);
    auto* groups = new QHBoxLayout; groups->setSpacing(14);
    auto group = [&](const QString& caption, const QList<QPair<QString, QString>>& entries) {
        auto* box = new QVBoxLayout; box->setSpacing(0);
        auto* row = new QHBoxLayout; row->setSpacing(0);
        for (const auto& [id, glyph] : entries) row->addWidget(commandButton(id, glyph, true));
        box->addLayout(row);
        auto* label = new QLabel(caption); label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("font-size: 10px; color: #7c8790; padding-bottom: 2px;");
        box->addWidget(label); groups->addLayout(box); groups->addWidget(divider());
    };
    group("CREATE", {{"primitive.box", "▣"}, {"primitive.cylinder", "◉"}, {"sketch.create", "▧"}, {"solid.extrude", "▣"}});
    group("MODIFY", {{"solid.fillet", "◩"}, {"solid.shell", "▱"}, {"solid.move", "✥"}});
    group("CONSTRUCT", {{"construction.plane", "▰"}});
    group("INSPECT", {{"inspect.measure", "↔"}});
    groups->addStretch();
    auto* stage = new QLabel("LOCAL DESIGN\nBox · Cylinder");
    stage->setStyleSheet("font-size: 11px; color: #7e898f; padding: 8px;"); groups->addWidget(stage);
    ribbonRight->addLayout(groups); ribbonLayout->addLayout(ribbonRight, 1); layout->addWidget(ribbon);

    auto* body = new QHBoxLayout; body->setContentsMargins(0, 0, 0, 0); body->setSpacing(0);
    browser_ = new QWidget; browser_->setFixedWidth(252);
    browser_->setStyleSheet("background: #fafbfc; border-right: 1px solid #d1d8dd;");
    auto* browserLayout = new QVBoxLayout(browser_); browserLayout->setContentsMargins(8, 0, 8, 8); browserLayout->setSpacing(3);
    auto* browserTitle = new QLabel("BROWSER"); browserTitle->setFixedHeight(33);
    browserTitle->setStyleSheet("font-size: 11px; letter-spacing: 1px; border-right: 0; border-bottom: 1px solid #d7dde1;");
    browserLayout->addWidget(browserTitle);
    tree_ = new QTreeWidget; tree_->setHeaderHidden(true); tree_->setIndentation(17);
    auto* document = new QTreeWidgetItem(tree_, {"◇  Untitled"});
    auto* settings = new QTreeWidgetItem(document, {"Document Settings"});
    new QTreeWidgetItem(settings, {"Units: mm"});
    auto* views = new QTreeWidgetItem(document, {"Named Views"});
    for (const QString& name : {"isometric", "top", "front", "right"}) {
        auto* item = new QTreeWidgetItem(views, {name.first(1).toUpper() + name.sliced(1)});
        item->setData(0, Qt::UserRole, name);
    }
    new QTreeWidgetItem(document, {"Origin"});
    bodiesNode_=new QTreeWidgetItem(document,{"Bodies"}); bodiesNode_->setExpanded(true);
    document->setExpanded(true); views->setExpanded(true);
    connect(tree_, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem* item, int) {
        const auto name = item->data(0, Qt::UserRole).toString();
        if (!name.isEmpty()) execute("view.orientation", {{"name", name}});
        const auto body=item->data(0,Qt::UserRole+1).toString();
        if(!body.isEmpty()) execute("selection.set",{{"bodyId",body}});
    });
    connect(tree_,&QTreeWidget::itemDoubleClicked,this,[this](QTreeWidgetItem* item,int) {
        const auto feature=item->data(0,Qt::UserRole+2).toString();
        if(!feature.isEmpty()) execute("feature.editDialog",{{"featureId",feature}});
    });
    browserLayout->addWidget(tree_, 1);
    auto* browserNote = new QLabel("Create a box or cylinder.\nDouble-click a body or Timeline\nfeature to edit its dimensions."); browserNote_=browserNote;
    browserNote->setStyleSheet("color: #89939b; font-size: 11px; border: 0; padding: 8px 4px;");
    browserLayout->addWidget(browserNote);
    body->addWidget(browser_);
    auto* canvas = new QVBoxLayout; canvas->setContentsMargins(0, 0, 0, 0); canvas->setSpacing(0);
    canvas->addWidget(view_, 1);
    navigation_ = new QWidget; navigation_->setStyleSheet("background: #f0f2f3;");
    auto* nav = new QHBoxLayout(navigation_); nav->setContentsMargins(8, 2, 8, 5); nav->setSpacing(2);
    nav->addStretch(); nav->addWidget(commandButton("view.home", "Home"));
    for (const QString& orientation : {"top", "front", "right"}) {
        auto* button = new QPushButton(orientation.first(1).toUpper() + orientation.sliced(1));
        connect(button, &QPushButton::clicked, this, [this, orientation] { execute("view.orientation", {{"name", orientation}}); });
        nav->addWidget(button);
    }
    auto* gridButton = new QPushButton("Grid");
    connect(gridButton, &QPushButton::clicked, this, [this] { execute("view.grid", {{"visible", !view_->grid}}); });
    nav->addWidget(gridButton); nav->addStretch(); canvas->addWidget(navigation_);
    body->addLayout(canvas, 1); layout->addLayout(body, 1);

    timeline_ = new QWidget; timeline_->setFixedHeight(54);
    timeline_->setStyleSheet("background: #f7f8f9; border-top: 1px solid #cdd5db;");
    timelineLayout_=new QHBoxLayout(timeline_); timelineLayout_->setContentsMargins(14,6,14,6);
    layout->addWidget(timeline_);
    status_ = new QLabel; statusBar()->addWidget(status_, 1);
    auto* shell = new QLabel("Alloy 0.1 · Primitive preview"); statusBar()->addPermanentWidget(shell);

    auto* viewMenu = menuBar()->addMenu("View");
    auto* editMenu=menuBar()->addMenu("Edit");
    editMenu->addAction("Undo",QKeySequence::Undo,this,[this]{execute("history.undo");});
    editMenu->addAction("Redo",QKeySequence::Redo,this,[this]{execute("history.redo");});
    editMenu->addAction("Delete Selected Body",QKeySequence::Delete,this,[this]{
        for(const auto& body:document_.bodies()) if(body.bodyId==selectedBody_) { execute("feature.delete",{{"featureId",body.featureId}}); break; }
    });
    viewMenu->addAction("Show / Hide Browser", QKeySequence("Ctrl+Alt+B"), this, [this] { execute("layout.browser", {{"visible", !browserVisible_}}); });
    viewMenu->addAction("Show / Hide Navigation Bar", QKeySequence("Ctrl+Alt+N"), this, [this] { execute("layout.navigation", {{"visible", !navigationVisible_}}); });
    viewMenu->addAction("Show / Hide Timeline", this, [this] { execute("layout.timeline", {{"visible", !timelineVisible_}}); });
    viewMenu->addAction("Reset to Default Layout", QKeySequence("Ctrl+Alt+R"), this, [this] { execute("layout.reset"); });
    auto* searchShortcut = new QShortcut(QKeySequence("S"), this);
    connect(searchShortcut, &QShortcut::activated, this, [this] {
        if (!qobject_cast<QLineEdit*>(QApplication::focusWidget())) execute("commands.search");
    });
    refresh();
}

const AlloyWindow::Command* AlloyWindow::findCommand(const QString& id) const {
    for (const auto& command : commands_) if (command.id == id) return &command;
    return nullptr;
}

QWidget* AlloyWindow::commandButton(const QString& id, const QString& glyph, bool large) {
    const auto* command = findCommand(id);
    auto* button = new QToolButton;
    button->setText(large ? glyph + "\n" + command->title : glyph);
    button->setEnabled(command->available);
    button->setToolTip(command->title + "\n" + command->description);
    if (large) {
        button->setMinimumWidth(67); button->setFixedHeight(49);
        button->setStyleSheet("QToolButton { font-size: 11px; padding: 1px 7px; }");
    }
    connect(button, &QToolButton::clicked, this, [this, id] { execute(id); });
    return button;
}

void AlloyWindow::registerCommands() {
    auto add = [this](QString id, QString title, QString description, QJsonObject parameters,
                     std::function<QJsonObject(const QJsonObject&)> run) {
        commands_.push_back({id, title, description, parameters, true, std::move(run)});
    };
    add("app.state", "Inspect state", "Read authoritative real-window shell state.", {}, [this](const auto&) { return state(); });
    add("app.identity", "Instance identity", "Protocol, process, window and unique instance identity.", {}, [this](const auto&) { return identity(); });
    auto kernelLength=parameter("number","Primitive length in millimeters.");
    kernelLength["minimum"]=0.001; kernelLength["maximum"]=100000;
    add("kernel.probe", "Kernel box check", "Evaluate a real diagnostic box without adding document geometry.",
        {{"width",kernelLength},{"depth",kernelLength},{"height",kernelLength}}, [](const auto& a) {
            const auto shape=GeometryKernel::primitive("box",{{"widthMm",a["width"]},{"depthMm",a["depth"]},
                {"heightMm",a["height"]},{"xMm",0},{"yMm",0},{"zMm",0}});
            return GeometryKernel::summarize(shape);
        });
    add("commands.list", "List commands", "Discover supported and unavailable commands with argument schemas.", {}, [this](const auto&) { return QJsonObject{{"commands", commands()}}; });
    add("commands.search", "Command search", "Open a discoverable native search session (S).", {}, [this](const auto&) {
        if (session_.value("command").toString() == "commands.search") { showSearch(); return session_; }
        return beginSession("commands.search", {});
    });
    add("commands.close", "Close command search", "Dismiss the native search session.", {}, [this](const auto&) {
        if (session_.value("command").toString() == "commands.search") return finishSession(session_.value("id").toString(), false);
        if (search_) search_->hide(); return QJsonObject{};
    });
    auto units = parameter("string", "Display units; authored lengths remain canonical millimeters.");
    units.insert("enum", QJsonArray{"mm", "in"});
    add("document.units", "Display units", "Change display units without rescaling authored geometry.", {{"units", units}}, [this](const auto& a) {
        auto candidate = document_.authored(); candidate["displayUnits"] = a.value("units");
        const auto error = document_.commit(candidate, "Change display units");
        return error.isEmpty() ? document_.state() : failure("invalid_document", error);
    });
    add("document.inspect", "Inspect document", "Read authoritative authored IDs, units and ownership.", {}, [this](const auto&) { return document_.state(); });
    add("document.settings", "Document Settings", "Open a shared display-unit editing session.", {}, [this](const auto&) { return beginSession("document.units", {}); });
    add("document.new", "New document", "Create a fresh authored document; dirty replacement requires explicit discard.", {{"discard", parameter("boolean", "Explicitly discard unsaved edits.")}}, [this](const auto& a) {
        if (!document_.history().isClean() && !a.value("discard").toBool()) return failure("unsaved_changes", "Save or explicitly discard the current document");
        document_.reset(); return document_.state();
    });
    auto optional=[](QJsonObject value) { value["required"]=false; return value; };
    auto path=parameter("string","Absolute native .alloy file path.");
    auto discard=optional(parameter("boolean","Explicitly discard unsaved edits."));
    add("document.save","Save","Atomically save authored intent; mark clean only after success.",{{"path",path}},[this](const auto& a){
        const auto error=document_.save(a["path"].toString());
        return error.isEmpty() ? document_.state() : failure("save_failed",error,"path");
    });
    add("document.open","Open","Validate and evaluate before replacing the current document.",{{"path",path},{"discard",discard}},[this](const auto& a){
        if(!document_.history().isClean() && !a.value("discard").toBool()) return failure("unsaved_changes","Save or explicitly discard the current document");
        const auto error=document_.open(a["path"].toString());
        if(error.isEmpty())view_->fit(document_.bodies());
        return error.isEmpty() ? document_.state() : failure("open_failed",error,"path");
    });
    for(const auto& id:{"document.saveDialog","document.saveAsDialog"}) {
        add(id,QString(id).endsWith("AsDialog") ? "Save As…" : "Save","Choose a local file and use the shared save command.",{},[this,id](const auto&){
            auto path=document_.path();
            if(path.isEmpty() || QString(id).endsWith("AsDialog")) path=QFileDialog::getSaveFileName(this,"Save Design",path.isEmpty() ? "Untitled.alloy" : path,"Alloy Design (*.alloy)");
            if(path.isEmpty()) return QJsonObject{{"cancelled",true}};
            if(!path.endsWith(".alloy",Qt::CaseInsensitive)) path+=".alloy";
            const auto response=execute("document.save",{{"path",QFileInfo(path).absoluteFilePath()}});
            return response["ok"].toBool() ? response["result"].toObject() : response;
        });
    }
    add("document.openDialog","Open…","Choose a local design and use the shared open command.",{},[this](const auto&){
        const auto path=QFileDialog::getOpenFileName(this,"Open Design",{},"Alloy Design (*.alloy)");
        if(path.isEmpty() || !confirmDiscard()) return QJsonObject{{"cancelled",true}};
        const auto response=execute("document.open",{{"path",path},{"discard",true}});
        return response["ok"].toBool() ? response["result"].toObject() : response;
    });
    add("history.undo","Undo","Undo one committed authored edit.",{},[this](const auto&){document_.history().undo();return document_.state();});
    add("history.redo","Redo","Redo one committed authored edit.",{},[this](const auto&){document_.history().redo();return document_.state();});
    auto length=parameter("number","Length in the selected display units; converted to canonical millimeters.");
    length["minimum"]=0.000001; length["maximum"]=100000;
    auto position=optional(parameter("number","Placement in the selected display units; default zero."));
    position["minimum"]=-100000; position["maximum"]=100000;
    auto name=optional(parameter("string","Body/feature name; defaults to Box or Cylinder."));
    auto featureId=parameter("string","Stable authored feature UUID.");
    for(const QString& type:{"box","cylinder"}) {
        QJsonObject schema{{"height",length},{"x",position},{"y",position},{"z",position},
            {"units",optional(units)},{"name",name},{"featureId",optional(featureId)}};
        if(type=="box") {schema["width"]=length;schema["depth"]=length;} else schema["radius"]=length;
        const auto id="solid."+type;
        add(id,type=="box" ? "Box" : "Cylinder","Create or edit a real primitive as a New Body.",schema,[this,id](const auto& a){
            const auto candidate=primitiveCandidate(id,a);
            const auto error=document_.commit(candidate,a.value("featureId").toString().isEmpty() ? "Create "+id.sliced(6) : "Edit "+id.sliced(6));
            if(!error.isEmpty()) return failure("evaluation_failed",error);
            view_->fit(document_.bodies()); return document_.state();
        });
        add("primitive."+type,type=="box" ? "Box" : "Cylinder","Open a shared primitive dimension dialog.",{},[this,id](const auto&){return beginSession(id,{});});
    }
    add("feature.edit","Edit feature","Edit canonical millimeter parameters; validate/evaluate before commit.",
        {{"featureId",featureId},{"parameters",parameter("object","Partial authored millimeter parameters.")}},[this](const auto& a){
            auto candidate=document_.authored();auto features=candidate["features"].toArray();bool found=false;
            for(int i=0;i<features.size();++i) {auto feature=features[i].toObject();if(feature["id"]!=a["featureId"]) continue;
                auto parameters=feature["parameters"].toObject();const auto edits=a["parameters"].toObject();
                for(auto it=edits.begin();it!=edits.end();++it) parameters[it.key()]=it.value();
                feature["parameters"]=parameters;features[i]=feature;found=true;break;
            }
            if(!found)return failure("stale_entity","Feature does not exist","featureId");
            candidate["features"]=features;const auto error=document_.commit(candidate,"Edit feature");
            return error.isEmpty() ? document_.state() : failure("evaluation_failed",error,"parameters");
        });
    add("feature.delete","Delete feature","Delete a primitive in one undoable transaction.",{{"featureId",featureId}},[this](const auto& a){
        auto candidate=document_.authored();auto features=candidate["features"].toArray();bool found=false;
        for(int i=0;i<features.size();++i) if(features[i].toObject()["id"]==a["featureId"]) {features.removeAt(i);found=true;break;}
        if(!found)return failure("stale_entity","Feature does not exist","featureId");
        candidate["features"]=features;const auto error=document_.commit(candidate,"Delete feature");
        return error.isEmpty() ? document_.state() : failure("evaluation_failed",error);
    });
    add("feature.editDialog","Edit feature…","Open the same primitive command with its saved dimensions.",{{"featureId",featureId}},[this](const auto& a){
        for(const auto& entry:document_.authored()["features"].toArray()) {const auto feature=entry.toObject();if(feature["id"]!=a["featureId"])continue;
            const auto p=feature["parameters"].toObject();const auto unit=document_.authored()["displayUnits"].toString();const double scale=unit=="in" ? 25.4 : 1;
            QJsonObject inputs{{"featureId",feature["id"]},{"name",feature["name"]},{"units",unit}};
            for(auto it=p.begin();it!=p.end();++it) inputs[it.key().chopped(2)]=it.value().toDouble()/scale;
            return beginSession("solid."+feature["type"].toString(),inputs);
        }
        return failure("stale_entity","Feature does not exist","featureId");
    });
    add("geometry.inspect","Inspect geometry","Current evaluated body summaries from OCCT, keyed to authored identities.",{},[this](const auto&){
        return document_.state()["evaluation"].toObject();
    });
    auto bodyId=parameter("string","Stable body UUID; empty string clears selection.");
    add("selection.set","Select body","Select a visible body through the same path as the Browser.",{{"bodyId",bodyId}},[this](const auto& a){
        const auto id=a["bodyId"].toString();bool found=id.isEmpty();
        for(const auto& body:document_.bodies()) found=found || (body.bodyId==id && body.visible);
        if(!found)return failure("stale_entity","Visible body does not exist","bodyId");
        selectedBody_=id;view_->selectBody(id);return QJsonObject{{"bodyId",id},{"kind","body"}};
    });
    auto point=parameter("number","Viewport-local logical pixel coordinate.");point["minimum"]=0;point["maximum"]=10000;
    add("selection.pick","Pick body","Pick actual OCCT geometry at viewport-local coordinates.",{{"x",point},{"y",point}},[this](const auto& a){
        selectedBody_=view_->pickBody(a["x"].toInt(),a["y"].toInt());view_->selectBody(selectedBody_);
        return QJsonObject{{"bodyId",selectedBody_},{"kind","body"}};
    });
    add("view.projectBody","Project body center","Return actual OCCT projection for a body's center, in logical pixels.",{{"bodyId",bodyId}},[this](const auto& a){
        for(const auto& body:document_.bodies()) if(body.bodyId==a["bodyId"].toString())return view_->projectBody(body);
        return failure("stale_entity","Body does not exist");
    });
    auto sessionTarget = parameter("string", "Implemented session target; modeling targets follow alongside their features.");
    sessionTarget["enum"] = QJsonArray{"document.units", "commands.search","solid.box","solid.cylinder"};
    add("command.begin", "Begin command session", "Begin a real editable dialog with disposable inputs.",
        {{"command", sessionTarget}, {"inputs", parameter("object", "Initial input overrides.")}},
        [this](const auto& a) { return beginSession(a["command"].toString(), a["inputs"].toObject()); });
    const QJsonObject sessionId{{"session", parameter("string", "Unique active session ID.")}};
    auto updateSchema = sessionId; updateSchema["inputs"] = parameter("object", "Partial input changes; unknown fields are rejected.");
    add("command.update", "Update command inputs", "Validate and update the actual dialog draft without committing intent.", updateSchema, [this](const auto& a) {
        if (session_.isEmpty() || session_.value("id") != a["session"]) return failure("invalid_session", "Session is no longer active");
        auto inputs = session_.value("inputs").toObject();
        const auto updates = a["inputs"].toObject();
        for (auto it = updates.begin(); it != updates.end(); ++it) inputs[it.key()] = it.value();
        const auto target = session_.value("command").toString();
        if (target == "commands.search") {
            if (inputs.size() != 1 || !inputs["query"].isString()) return failure("invalid_arguments", "Search input requires a query string");
        } else {
            QString field;
            const auto error = validateArguments(*findCommand(target), inputs, &field);
            if (!error.isEmpty()) return failure("invalid_arguments", error, field);
        }
        session_["inputs"] = inputs; session_["previewed"] = false;view_->clearPreview();return session_;
    });
    add("command.preview", "Inspect command preview", "Inspect disposable authored draft; no evaluated geometry is implied.", sessionId, [this](const auto& a) {
        if (session_.isEmpty() || session_.value("id") != a["session"]) return failure("invalid_session", "Session is no longer active");
        session_["previewed"]=false;view_->clearPreview();
        if (session_.value("command").toString() != "commands.search"
            && session_.value("baseDocumentRevision").toInteger() != static_cast<qint64>(document_.revision()))
            return failure("stale_session", "The document changed; cancel and restart this edit");
        auto result = session_;
        if (session_.value("command").toString() == "document.units") {
            auto draft = document_.authored(); draft["displayUnits"] = session_.value("inputs").toObject()["units"];
            result["authoredDraft"] = draft;
        }
        if(session_.value("command").toString().startsWith("solid.")) {
            const auto candidate=primitiveCandidate(session_.value("command").toString(),session_.value("inputs").toObject());
            const auto features=candidate["features"].toArray();
            const auto id=session_.value("inputs").toObject()["featureId"].toString();
            for(const auto& entry:features) {const auto feature=entry.toObject();if(!id.isEmpty() && feature["id"]!=id)continue;
                if(id.isEmpty() && feature!=features.last().toObject())continue;
                const auto shape=GeometryKernel::primitive(feature["type"].toString(),feature["parameters"].toObject());
                view_->showPreview(shape);result["geometry"]=GeometryKernel::summarize(shape);break;
            }
            result["geometryEvaluated"]=true;
        } else result["geometryEvaluated"] = false;
        session_["previewed"]=true;result["previewed"]=true;return result;
    });
    add("command.commit", "Commit command session", "Commit the validated draft through the same authoritative command.", sessionId,
        [this](const auto& a) { return finishSession(a["session"].toString(), true); });
    add("command.cancel", "Cancel command session", "Discard the real dialog draft without an undo entry.", sessionId,
        [this](const auto& a) { return finishSession(a["session"].toString(), false); });
    const QJsonObject visible{{"visible", parameter("boolean", "Whether this region is visible.")}};
    add("layout.browser", "Show / hide Browser", "Control the left Browser region.", visible, [this](const auto& a) { browserVisible_ = a.value("visible").toBool(); return QJsonObject{}; });
    add("layout.timeline", "Show / hide Timeline", "Control the empty feature history region.", visible, [this](const auto& a) { timelineVisible_ = a.value("visible").toBool(); return QJsonObject{}; });
    add("layout.navigation", "Show / hide Navigation Bar", "Control the bottom reference-view controls.", visible, [this](const auto& a) { navigationVisible_ = a.value("visible").toBool(); return QJsonObject{}; });
    add("layout.reset", "Reset to Default Layout", "Restore Browser, navigation bar, and Timeline.", {}, [this](const auto&) { browserVisible_ = timelineVisible_ = navigationVisible_ = true; return QJsonObject{}; });
    add("view.grid", "Show / hide Grid", "Show or hide the reference grid; no model geometry is created.", visible, [this](const auto& a) { view_->grid = a.value("visible").toBool(); return QJsonObject{}; });
    add("view.origin", "Show / hide Origin", "Show or hide reference axes.", visible, [this](const auto& a) { view_->origin = a.value("visible").toBool(); return QJsonObject{}; });
    auto orientation = parameter("string", "Orthographic orientation of the reference grid.");
    orientation.insert("enum", QJsonArray{"isometric", "top", "front", "right"});
    add("view.orientation", "Set view orientation", "Select a named view of the reference grid.", {{"name", orientation}}, [this](const auto& a) { view_->orientation = a.value("name").toString(); return QJsonObject{}; });
    auto factor = parameter("number", "Relative scale factor (0.1 to 10); resulting zoom clamps to 0.1 to 10."); factor.insert("minimum", 0.1); factor.insert("maximum", 10);
    add("view.zoom", "Zoom reference view", "Scale the reference grid; mouse wheel uses this command.", {{"factor", factor}}, [this](const auto& a) { view_->zoom = std::clamp(view_->zoom * a.value("factor").toDouble(), 0.1, 10.0); return QJsonObject{}; });
    auto offset = parameter("number", "Relative pan in logical pixels; each delta limited to +/-10000."); offset.insert("minimum", -10000); offset.insert("maximum", 10000);
    add("view.pan", "Pan reference view", "Translate the grid; middle-mouse drag uses this command.", {{"x", offset}, {"y", offset}}, [this](const auto& a) { view_->pan += QPointF(a.value("x").toDouble(), a.value("y").toDouble()); return QJsonObject{}; });
    auto dimension = parameter("number", "Logical window size; width 1040..4096, height 640..4096.");
    dimension.insert("minimum", 640); dimension.insert("maximum", 4096);
    add("window.resize", "Resize window", "Resize the real native window for layout and framebuffer inspection.", {{"width", dimension}, {"height", dimension}}, [this](const auto& a) {
        if (a.value("width").toInt() < 1040) return failure("invalid_arguments", "Minimum width is 1040");
        resize(a.value("width").toInt(), a.value("height").toInt()); return QJsonObject{};
    });
    add("view.home", "Home view", "Fit visible bodies in the isometric view.", {}, [this](const auto&) { view_->orientation = "isometric"; view_->fit(document_.bodies()); return QJsonObject{}; });
    add("capture.screenshot", "Capture screenshot", "Save a PNG of the actual main window, excluding OS decorations and separate dialogs.", {{"path", parameter("string", "Absolute destination PNG path; parent directory must exist.")}}, [this](const auto& a) {
        const QString path = a.value("path").toString();
        if (!QDir::isAbsolutePath(path) || !path.endsWith(".png", Qt::CaseInsensitive)) return failure("invalid_arguments", "Screenshot requires an absolute .png path");
        auto pixels = grab();
        const auto framebuffer = view_->framebuffer();
        if (framebuffer.isNull()) return failure("capture_failed", "OCCT framebuffer capture failed");
        { QPainter composition(&pixels); composition.drawImage(view_->mapTo(this, QPoint(0, 0)), framebuffer); }
        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly) || !pixels.save(&file, "PNG") || !file.commit()) return failure("capture_failed", "Could not write screenshot");
        return QJsonObject{{"path", path}, {"pixelWidth", pixels.width()}, {"pixelHeight", pixels.height()}, {"devicePixelRatio", pixels.devicePixelRatio()}, {"source", "QWidget chrome + OCCT OpenGL framebuffer"}, {"viewportPixelWidth", framebuffer.width()}, {"viewportPixelHeight", framebuffer.height()}, {"viewportSha256", QString::fromLatin1(QCryptographicHash::hash(QByteArrayView(reinterpret_cast<const char*>(framebuffer.constBits()), framebuffer.sizeInBytes()), QCryptographicHash::Sha256).toHex())}};
    });
    add("capture.dialog", "Capture active dialog", "Capture the real search/settings QWidget; no geometry or OS decoration claim.", {{"path", parameter("string", "Absolute destination PNG path.")}}, [this](const auto& a) {
        const auto path = a["path"].toString();
        if (!QDir::isAbsolutePath(path) || !path.endsWith(".png", Qt::CaseInsensitive)) return failure("invalid_arguments", "Requires an absolute PNG path", "path");
        auto* dialog = session_.value("command").toString() == "commands.search" ? search_
            : session_.value("command").toString().startsWith("solid.") ? primitiveDialog_ : unitsDialog_;
        if (session_.isEmpty() || !dialog || !dialog->isVisible()) return failure("no_active_dialog", "No active dialog to capture");
        const auto pixels = dialog->grab(); QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly) || !pixels.save(&file, "PNG") || !file.commit()) return failure("capture_failed", "Could not save active dialog");
        return QJsonObject{{"path", path}, {"source", "active QWidget dialog"}, {"sessionId", session_.value("id")},
            {"pixelWidth", pixels.width()}, {"pixelHeight", pixels.height()}};
    });
    add("app.quit", "Quit Alloy", "Complete the request, then exit; dirty designs require explicit discard.", {{"discard",discard}}, [this](const auto& a) {
        if(!document_.history().isClean() && !a.value("discard").toBool())return failure("unsaved_changes","Save or explicitly discard the design before quitting");
        authorizedQuit_=true;
        QTimer::singleShot(0,qApp,&QApplication::quit);return QJsonObject{{"quitting",true}};
    });
    for (const auto& pair : QList<QPair<QString, QString>>{{"sketch.create", "Create Sketch"}, {"solid.extrude", "Extrude"}, {"solid.revolve", "Revolve"}, {"solid.fillet", "Fillet"}, {"solid.shell", "Shell"}, {"solid.move", "Move"}, {"construction.plane", "Plane"}, {"inspect.measure", "Measure"}}) {
        commands_.push_back({pair.first, pair.second, "Not implemented in the UI shell.", {}, false, {}});
    }
}

QJsonArray AlloyWindow::commands() const {
    QJsonArray result;
    for (const auto& command : commands_) result.append(QJsonObject{{"id", command.id}, {"title", command.title},
        {"description", command.description}, {"available", command.available}, {"parameters", command.parameters}});
    return result;
}

QJsonObject AlloyWindow::execute(const QString& id, const QJsonObject& args) {
    const auto* command = findCommand(id);
    if (!command) return failure("unknown_command", "No command named " + id);
    if (!command->available) return failure("not_implemented", command->title + " is not implemented in the UI shell");
    QString field;
    const auto argumentError = validateArguments(*command, args, &field);
    if (!argumentError.isEmpty()) return failure("invalid_arguments", argumentError, field);
    QJsonObject result;
    try { result=command->run(args); }
    catch(const Standard_Failure& error) { return failure("kernel_failure",QString::fromUtf8(error.GetMessageString())); }
    catch(const std::exception& error) { return failure("command_failed",QString::fromUtf8(error.what())); }
    if (result.value("ok").isBool() && !result.value("ok").toBool()) return result;
    const bool changesState = id == "window.resize" || id.startsWith("layout.") || id.startsWith("view.") || (id == "commands.search" || id == "commands.close");
    if (changesState) ++revision_;
    refresh();
    return {{"ok", true}, {"completion", "completed"}, {"command", id}, {"revision", static_cast<qint64>(revision_)},
        {"documentRevision", static_cast<qint64>(document_.revision())}, {"result", result}};
}

QString AlloyWindow::validateArguments(const Command& command, const QJsonObject& args, QString* field) const {
    for (auto it = args.begin(); it != args.end(); ++it) if (!command.parameters.contains(it.key())) {
        if (field) *field = it.key(); return "Unknown argument: " + it.key();
    }
    for (auto it = command.parameters.begin(); it != command.parameters.end(); ++it) {
        if (field) *field = it.key();
        const auto spec = it.value().toObject();
        const auto value = args.value(it.key());
        const QString type = spec.value("type").toString();
        if(value.isUndefined() && !spec.value("required").toBool(true))continue;
        if (value.isUndefined() || (type == "boolean" && !value.isBool()) || (type == "string" && !value.isString()) || (type == "number" && !value.isDouble()) || (type == "object" && !value.isObject())) return "Argument " + it.key() + " must be " + type;
        if (spec.contains("enum") && !spec.value("enum").toArray().contains(value)) return "Unsupported value for " + it.key();
        if (type == "number" && (!std::isfinite(value.toDouble()) || value.toDouble() < spec.value("minimum").toDouble() || value.toDouble() > spec.value("maximum").toDouble())) return "Argument out of range: " + it.key();
    }
    return {};
}

QJsonObject AlloyWindow::state() const {
    // Report the current native drawable, not a previous offscreen capture FBO.
    view_->sync();
    const auto target=session_.value("command").toString();
    const bool dialogVisible=target=="commands.search" ? bool(search_ && search_->isVisible())
        : target.startsWith("solid.") ? bool(primitiveDialog_ && primitiveDialog_->isVisible()) : bool(unitsDialog_ && unitsDialog_->isVisible());
    const auto dialogInputs=session_.isEmpty() ? QJsonObject{} : target.startsWith("solid.") ? primitiveWidgetInputs()
        : target=="commands.search" ? QJsonObject{{"query",searchInput_ ? searchInput_->text() : QString{}}}
        : QJsonObject{{"units",unitsInput_ ? unitsInput_->currentText() : QString{}}};
    return {{"application", "Alloy"}, {"version", "0.1.0"}, {"stage", "primitive-preview"},
        {"runtime", QJsonObject{{"qtVersion", qVersion()}, {"occtBuildVersion", ALLOY_OCCT_VERSION}}},
        {"revision", static_cast<qint64>(revision_)},
        {"identity", identity()}, {"activeSession", session_},
        {"document", document_.state()},
        {"selection",QJsonObject{{"kind","body"},{"bodyId",selectedBody_}}},
        {"kernel", QJsonObject{{"candidate", "Open CASCADE"}, {"discoveredVersion", ALLOY_OCCT_VERSION}, {"integrated", true}, {"visualizationIntegrated", true}, {"policy",GeometryKernel::policy()}}},
        {"ui", QJsonObject{{"workspace", "design"}, {"tab", "solid"}, {"browserVisible", browserVisible_}, {"timelineVisible", timelineVisible_}, {"navigationVisible", navigationVisible_}, {"commandSearchVisible", search_ && search_->isVisible()}, {"windowWidth", width()}, {"windowHeight", height()}, {"devicePixelRatio", devicePixelRatioF()},
            {"activeDialog", QJsonObject{{"command", session_.value("command")},
                {"visible",dialogVisible},{"inputs",dialogInputs}}}}},
        {"viewport", QJsonObject{{"renderer", "OCCT OpenGL"}, {"gpu", view_->diagnostics()}, {"orientation", view_->orientation}, {"gridVisible", view_->grid}, {"originVisible", view_->origin}, {"zoom", view_->zoom}, {"pan", QJsonObject{{"x", view_->pan.x()}, {"y", view_->pan.y()}}}, {"width", view_->width()}, {"height", view_->height()}}}};
}

void AlloyWindow::refresh() {
    if (!browser_) return;
    browser_->setVisible(browserVisible_); timeline_->setVisible(timelineVisible_); navigation_->setVisible(navigationVisible_);
    tree_->topLevelItem(0)->child(0)->child(0)->setText(0, "Units: " + document_.authored()["displayUnits"].toString());
    const auto label=document_.state()["label"].toString();
    const auto title=label+(document_.history().isClean() ? "" : " *");
    setWindowTitle(title+" — Alloy");documentTitle_->setText("◇   "+title);tree_->topLevelItem(0)->setText(0,"◇  "+label);
    if(!session_.isEmpty() && session_.value("command").toString()!="commands.search"
        && session_.value("baseDocumentRevision").toInteger()!=static_cast<qint64>(document_.revision())) {
        view_->clearPreview();session_["previewed"]=false;
    }
    if(displayedDocumentRevision_!=document_.revision()) {
        const QSignalBlocker block(tree_);
        qDeleteAll(bodiesNode_->takeChildren());
        while(auto* item=timelineLayout_->takeAt(0)){delete item->widget();delete item;}
        timelineLayout_->addWidget(new QLabel("TIMELINE"));
        bool selectedExists=false;
        for(const auto& body:document_.bodies()) {
            auto* item=new QTreeWidgetItem(bodiesNode_,{body.name});item->setData(0,Qt::UserRole+1,body.bodyId);item->setData(0,Qt::UserRole+2,body.featureId);
            selectedExists=selectedExists || (body.bodyId==selectedBody_ && body.visible);
            auto* button=new QToolButton;button->setText(body.name);button->setToolTip("Edit "+body.name);
            button->setProperty("featureId",body.featureId);
            connect(button,&QToolButton::clicked,this,[this,id=body.featureId]{execute("feature.editDialog",{{"featureId",id}});});
            auto* menu=new QMenu(button);menu->addAction("Delete",this,[this,id=body.featureId]{execute("feature.delete",{{"featureId",id}});});
            button->setMenu(menu);button->setPopupMode(QToolButton::MenuButtonPopup);timelineLayout_->addWidget(button);
        }
        if(!selectedExists)selectedBody_.clear();
        timelineLayout_->addStretch();bodiesNode_->setExpanded(true);
        view_->setBodies(document_.bodies(),document_.revision());
        displayedDocumentRevision_=document_.revision();
    }
    for(int i=0;i<bodiesNode_->childCount();++i)bodiesNode_->child(i)->setSelected(bodiesNode_->child(i)->data(0,Qt::UserRole+1).toString()==selectedBody_);
    view_->selectBody(selectedBody_);
    if (session_.value("command").toString() == "document.units" && unitsInput_) {
        const QSignalBlocker block(unitsInput_);
        unitsInput_->setCurrentText(session_.value("inputs").toObject()["units"].toString());
    }
    if (session_.value("command").toString() == "commands.search" && searchInput_) {
        searchInput_->setText(session_.value("inputs").toObject()["query"].toString());
    }
    if(session_.value("command").toString().startsWith("solid.") && primitiveDialog_) {
        const auto inputs=session_.value("inputs").toObject();
        for(auto it=primitiveInputs_.begin();it!=primitiveInputs_.end();++it){const QSignalBlocker block(it.value());it.value()->setValue(inputs[it.key()].toDouble());it.value()->setSuffix(" "+inputs["units"].toString());}
        const QSignalBlocker block(primitiveName_);primitiveName_->setText(inputs["name"].toString());
    }
    view_->sync();
    const auto gpu = view_->diagnostics();
    status_->setText(QString("  %3 · %4 bodies · %1 · Zoom %2%    |    Click: select · Middle mouse: pan · Wheel: zoom · S: commands")
        .arg(view_->orientation).arg(qRound(view_->zoom * 100)).arg(gpu.value("hardwareAccelerated").toBool() ? "OCCT OpenGL" : "GPU UNQUALIFIED: " + gpu.value("qualification").toString()).arg(document_.bodies().size()));
}

void AlloyWindow::showSearch() {
    if (search_) { search_->show(); search_->raise(); search_->activateWindow(); return; }
    search_ = new QDialog(this);
    search_->setWindowTitle("Alloy — Commands"); search_->resize(510, 400);
    connect(search_, &QDialog::rejected, this, [this] { execute("commands.close"); });
    auto* layout = new QVBoxLayout(search_);
    auto* input = new QLineEdit; searchInput_ = input;
    input->setPlaceholderText("Search supported commands…"); layout->addWidget(input);
    auto* list = new QListWidget; layout->addWidget(list);
    auto populate = [this, list](const QString& query) {
        list->clear();
        for (const auto& command : commands_) {
            if (!command.available || !command.parameters.isEmpty() || command.id == "commands.search" || command.id == "commands.close" || command.id == "commands.list" || command.id == "app.state") continue;
            if (!(command.title + command.id).contains(query, Qt::CaseInsensitive)) continue;
            auto* item = new QListWidgetItem(command.title + "   ·   " + command.id, list); item->setData(Qt::UserRole, command.id);
        }
        if (list->count()) list->setCurrentRow(0);
    };
    populate("");
    connect(input, &QLineEdit::textChanged, search_, populate);
    connect(input, &QLineEdit::textChanged, search_, [this](const QString& query) {
        if (session_.value("command").toString() == "commands.search" && session_.value("inputs").toObject()["query"].toString() != query)
            execute("command.update", {{"session", session_.value("id")}, {"inputs", QJsonObject{{"query", query}}}});
    });
    auto activate = [this, list] {
        if (const auto* item = list->currentItem()) { const auto id = item->data(Qt::UserRole).toString(); execute("commands.close"); execute(id); }
    };
    connect(input, &QLineEdit::returnPressed, search_, activate);
    connect(list, &QListWidget::itemActivated, search_, [activate](QListWidgetItem*) { activate(); });
    search_->show(); input->setFocus();
}

QJsonObject AlloyWindow::identity() const {
    return {{"application", "Alloy"}, {"protocol", "alloy-control/1"}, {"instanceId", instanceId_},
        {"pid", QCoreApplication::applicationPid()}, {"windowId", QString::number(static_cast<qulonglong>(winId()), 16)},
        {"capabilities", QJsonArray{"shell", "authored-document", "command-sessions"}}};
}

QJsonObject AlloyWindow::beginSession(const QString& command, const QJsonObject& inputs) {
    if (!session_.isEmpty()) return failure("session_busy", "Complete or cancel the active command first");
    QJsonObject values = command == "commands.search" ? QJsonObject{{"query", ""}}
        : QJsonObject{{"units", document_.authored()["displayUnits"]}};
    if(command.startsWith("solid.")) {
        const auto units=document_.authored()["displayUnits"].toString();const double scale=units=="in" ? 25.4 : 1;
        values={{"height",10/scale},{"x",0},{"y",0},{"z",0},{"units",units},{"name",command=="solid.box" ? "Box" : "Cylinder"}};
        if(command=="solid.box"){values["width"]=20/scale;values["depth"]=30/scale;} else values["radius"]=5/scale;
    }
    for (auto it = inputs.begin(); it != inputs.end(); ++it) values[it.key()] = it.value();
    if (command == "commands.search") {
        if (values.size() != 1 || !values["query"].isString()) return failure("invalid_arguments", "Search requires a query string");
    } else {
        const auto error = validateArguments(*findCommand(command), values);
        if (!error.isEmpty()) return failure("invalid_arguments", error);
    }
    session_ = {{"id", CadDocument::newId()}, {"command", command}, {"status", "active"},
        {"inputs", values}, {"previewed", false}, {"baseDocumentRevision", static_cast<qint64>(document_.revision())},
        {"inputSchema", command == "commands.search" ? QJsonObject{{"query", parameter("string", "Search text")}} : findCommand(command)->parameters}};
    if (command == "commands.search") showSearch(); else if(command.startsWith("solid.")) showPrimitive(); else showUnits();
    return session_;
}

QJsonObject AlloyWindow::finishSession(const QString& id, bool commit) {
    if (session_.isEmpty() || session_.value("id").toString() != id) return failure("invalid_session", "Session is no longer active");
    const auto target = session_.value("command").toString();
    QJsonObject result{{"sessionId", id}, {"status", commit ? "committed" : "cancelled"}};
    if (commit && target != "commands.search") {
        if (session_.value("baseDocumentRevision").toInteger() != static_cast<qint64>(document_.revision()))
            return failure("stale_session", "The document changed; cancel and restart this edit");
        auto response = execute(target, session_.value("inputs").toObject());
        if (!response["ok"].toBool()) return response;
        result["commandResult"] = response;
    }
    session_ = {};
    if (target == "commands.search" && search_) search_->hide();
    if (target == "document.units" && unitsDialog_) unitsDialog_->hide();
    if(target.startsWith("solid.")) {if(primitiveDialog_)primitiveDialog_->hide();view_->clearPreview();}
    return result;
}

void AlloyWindow::showUnits() {
    if (!unitsDialog_) {
        unitsDialog_ = new QDialog(this); unitsDialog_->setWindowTitle("Document Settings");
        unitsDialog_->setWindowModality(Qt::WindowModal);
        auto* layout = new QVBoxLayout(unitsDialog_);
        layout->addWidget(new QLabel("Units — changing units preserves the model's size"));
        unitsInput_ = new QComboBox; unitsInput_->addItems({"mm", "in"});
        unitsInput_->setObjectName("document.units.input"); layout->addWidget(unitsInput_);
        auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel); layout->addWidget(buttons);
        connect(unitsInput_, &QComboBox::currentTextChanged, this, [this](const QString& units) {
            execute("command.update", {{"session", session_.value("id")}, {"inputs", QJsonObject{{"units", units}}}});
        });
        connect(buttons, &QDialogButtonBox::accepted, this, [this] { execute("command.commit", {{"session", session_.value("id")}}); });
        connect(buttons, &QDialogButtonBox::rejected, unitsDialog_, &QDialog::reject);
        connect(unitsDialog_, &QDialog::rejected, this, [this] { execute("command.cancel", {{"session", session_.value("id")}}); });
    }
    { const QSignalBlocker block(unitsInput_); unitsInput_->setCurrentText(session_.value("inputs").toObject()["units"].toString()); }
    unitsDialog_->show(); unitsDialog_->raise();
}

QJsonObject AlloyWindow::primitiveCandidate(const QString& command,const QJsonObject& inputs) const {
    auto candidate=document_.authored();auto features=candidate["features"].toArray();
    const auto type=command.sliced(6);const auto id=inputs.value("featureId").toString();
    int index=-1;QJsonObject feature;
    if(!id.isEmpty()) {
        for(int i=0;i<features.size();++i)if(features[i].toObject()["id"].toString()==id){index=i;feature=features[i].toObject();break;}
        if(index<0 || feature["type"].toString()!=type)throw std::invalid_argument("Feature identity/type is no longer available");
    } else feature={{"id",CadDocument::newId()},{"bodyId",CadDocument::newId()},{"ownerId",candidate["documentId"]},
        {"type",type},{"visible",true}};
    const auto units=inputs.value("units").toString(candidate["displayUnits"].toString());
    QJsonObject parameters;
    const QStringList dimensions=type=="box" ? QStringList{"width","depth","height"} : QStringList{"radius","height"};
    for(const auto& name:dimensions)parameters[name+"Mm"]=CadDocument::millimeters(inputs.value(name).toDouble(),units);
    for(const auto& name:{"x","y","z"})parameters[QString(name)+"Mm"]=CadDocument::millimeters(inputs.value(name).toDouble(),units);
    feature["parameters"]=parameters;feature["name"]=inputs.value("name").toString(type=="box" ? "Box" : "Cylinder");
    if(index>=0)features[index]=feature;else features.append(feature);
    candidate["features"]=features;
    const auto error=CadDocument::validate(candidate);if(!error.isEmpty())throw std::invalid_argument(error.toStdString());
    return candidate;
}

QJsonObject AlloyWindow::primitiveWidgetInputs() const {
    QJsonObject inputs;
    for(auto it=primitiveInputs_.begin();it!=primitiveInputs_.end();++it)inputs[it.key()]=it.value()->value();
    if(primitiveName_)inputs["name"]=primitiveName_->text();
    inputs["units"]=session_.value("inputs").toObject()["units"];
    return inputs;
}
void AlloyWindow::showPrimitive() {
    if(primitiveDialog_)delete primitiveDialog_;
    primitiveInputs_.clear();primitiveDialog_=new QDialog(this);
    const auto command=session_.value("command").toString();const auto inputs=session_.value("inputs").toObject();
    primitiveDialog_->setWindowTitle(command=="solid.box" ? "Box" : "Cylinder");
    primitiveDialog_->setWindowModality(Qt::WindowModal);
    auto* layout=new QVBoxLayout(primitiveDialog_);auto* form=new QFormLayout;
    auto* plane=new QLabel(inputs.value("featureId").toString().isEmpty() ? "XY plane · New Body" : "XY plane · Edit Body");layout->addWidget(plane);
    primitiveName_=new QLineEdit(inputs["name"].toString());primitiveName_->setObjectName("primitive.name");form->addRow("Name",primitiveName_);
    const QStringList fields=command=="solid.box" ? QStringList{"width","depth","height","x","y","z"}
        : QStringList{"radius","height","x","y","z"};
    for(const auto& field:fields) {
        auto* input=new QDoubleSpinBox;input->setKeyboardTracking(false);input->setDecimals(6);input->setRange(field.size()==1 ? -100000 : 0.000001,100000);
        input->setSuffix(" "+inputs["units"].toString());input->setValue(inputs[field].toDouble());input->setObjectName("primitive."+field);
        primitiveInputs_[field]=input;form->addRow(field.first(1).toUpper()+field.sliced(1),input);
        connect(input,qOverload<double>(&QDoubleSpinBox::valueChanged),this,[this,field](double value){
            const auto response=execute("command.update",{{"session",session_.value("id")},{"inputs",QJsonObject{{field,value}}}});
            if(!response["ok"].toBool())primitiveError_->setText(response["error"].toObject()["message"].toString());
        });
    }
    connect(primitiveName_,&QLineEdit::textChanged,this,[this](const QString& value){
        execute("command.update",{{"session",session_.value("id")},{"inputs",QJsonObject{{"name",value}}}});
    });
    layout->addLayout(form);primitiveError_=new QLabel;primitiveError_->setWordWrap(true);primitiveError_->setMaximumWidth(400);
    primitiveError_->setStyleSheet("color: #a63832;");layout->addWidget(primitiveError_);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    auto* preview=buttons->addButton("Preview",QDialogButtonBox::ActionRole);
    connect(preview,&QPushButton::clicked,this,[this]{
        const auto response=execute("command.preview",{{"session",session_.value("id")}});
        primitiveError_->setText(response["ok"].toBool() ? "" : response["error"].toObject()["message"].toString());
    });
    connect(buttons,&QDialogButtonBox::accepted,this,[this]{
        const auto response=execute("command.commit",{{"session",session_.value("id")}});
        if(!response["ok"].toBool())primitiveError_->setText(response["error"].toObject()["message"].toString());
    });
    connect(buttons,&QDialogButtonBox::rejected,this,[this]{execute("command.cancel",{{"session",session_.value("id")}});});
    connect(primitiveDialog_,&QDialog::rejected,this,[this]{execute("command.cancel",{{"session",session_.value("id")}});});
    layout->addWidget(buttons);primitiveDialog_->show();primitiveDialog_->raise();
}

bool AlloyWindow::confirmDiscard() {
    if(document_.history().isClean())return true;
    const auto choice=QMessageBox::warning(this,"Unsaved Design","Save changes to this design?",
        QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel,QMessageBox::Save);
    if(choice==QMessageBox::Cancel)return false;
    if(choice==QMessageBox::Discard)return true;
    execute("document.saveDialog");return document_.history().isClean();
}
void AlloyWindow::closeEvent(QCloseEvent* event) {
    if(authorizedQuit_ || confirmDiscard())event->accept();else event->ignore();
}
