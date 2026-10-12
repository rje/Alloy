#include "AlloyWindow.h"
#include "ReferenceView.h"
#include "GeometryKernel.h"
#include "SketchGeometry.h"
#include <QApplication>
#include <QCryptographicHash>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QDir>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QGridLayout>
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
#include <QCheckBox>
#include <QScrollArea>
#include <QRegularExpression>
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
class CommandDock final : public QDockWidget {
public:
    using QDockWidget::QDockWidget;
    std::function<void()> onClose;
protected:
    void closeEvent(QCloseEvent* event) override {
        if(onClose)onClose();
        QDockWidget::closeEvent(event);
    }
};
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
    view_->dispatch = [this](const QString& command, const QJsonObject& args) {
        const auto response=execute(command,args);
        if(command.startsWith("sketch."))sketchResponse(response);
    };
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
    group("CREATE", {{"primitive.box", "▣"}, {"primitive.cylinder", "◉"}, {"sketch.create", "▧"}, {"solid.extrudeDialog", "▣"}});
    group("MODIFY", {{"solid.fillet", "◩"}, {"solid.shell", "▱"}, {"solid.move", "✥"}});
    group("CONSTRUCT", {{"construction.plane", "▰"}});
    group("INSPECT", {{"inspect.measure", "↔"}});
    groups->addStretch();
    auto* stage = new QLabel("LOCAL DESIGN\nSketch · Box · Cylinder");
    stage->setStyleSheet("font-size: 11px; color: #7e898f; padding: 8px;"); groups->addWidget(stage);
    ribbonRight->addLayout(groups); ribbonLayout->addLayout(ribbonRight, 1); layout->addWidget(ribbon);

    sketchToolbar_=new QWidget;sketchToolbar_->setObjectName("sketchToolbar");
    sketchToolbar_->setStyleSheet("#sketchToolbar { background: #e1f2f8; border-bottom: 1px solid #9fc7db; }");
    auto* sketchTools=new QHBoxLayout(sketchToolbar_);sketchTools->setContentsMargins(14,5,14,5);
    auto* sketchTitle=new QLabel("SKETCH");sketchTitle->setStyleSheet("font-weight: 700; color: #176588;");sketchTools->addWidget(sketchTitle);
    for(const auto& tool:QStringList{"line","rectangle","select","dimension"}) {
        auto* button=new QPushButton(tool=="line" ? "Line  L" : tool=="rectangle" ? "Rectangle  R" : tool=="dimension" ? "Dimension  D" : "Select");
        button->setObjectName("sketch.tool."+tool);sketchTools->addWidget(button);
        connect(button,&QPushButton::clicked,this,[this,tool]{sketchResponse(execute("sketch.tool",{{"tool",tool}}));});
    }
    sketchTools->addWidget(commandButton("sketch.lookAt","Look At"));sketchTools->addStretch();
    auto* finish=new QPushButton("Finish Sketch");finish->setObjectName("sketch.finish");
    finish->setStyleSheet("background: #bce5d4; border: 1px solid #77b79b; font-weight: 600;");sketchTools->addWidget(finish);
    connect(finish,&QPushButton::clicked,this,[this]{sketchResponse(execute("sketch.finish"));});
    auto* cancel=new QPushButton("Cancel");sketchTools->addWidget(cancel);
    connect(cancel,&QPushButton::clicked,this,[this]{execute("sketch.cancel");});
    layout->addWidget(sketchToolbar_);sketchToolbar_->hide();

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
    sketchesNode_=new QTreeWidgetItem(document,{"Sketches"});sketchesNode_->setExpanded(true);
    document->setExpanded(true); views->setExpanded(true);
    connect(tree_, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem* item, int) {
        const auto name = item->data(0, Qt::UserRole).toString();
        if (!name.isEmpty()) execute("view.orientation", {{"name", name}});
        const auto body=item->data(0,Qt::UserRole+1).toString();
        if(!body.isEmpty()) execute("selection.set",{{"bodyId",body}});
        else if(item->parent()==sketchesNode_)execute("selection.sketchSet",{{"sketchId",item->data(0,Qt::UserRole+2).toString()}});
    });
    connect(tree_,&QTreeWidget::itemChanged,this,[this](QTreeWidgetItem* item,int column){
        if(column==0 && (item->parent()==sketchesNode_ || item->parent()==bodiesNode_)) {
            const auto id=item->data(0,Qt::UserRole+2).toString();const auto response=execute("feature.visibility",{{"featureId",id},{"visible",item->checkState(0)==Qt::Checked}});
            if(!response["ok"].toBool()) {const QSignalBlocker block(tree_);for(const auto& entry:document_.authored()["features"].toArray()) {
                const auto feature=entry.toObject();if(feature["id"]==id){item->setCheckState(0,feature["visible"].toBool() ? Qt::Checked : Qt::Unchecked);break;}
            }}
        }
    });
    connect(tree_,&QTreeWidget::itemDoubleClicked,this,[this](QTreeWidgetItem* item,int) {
        const auto feature=item->data(0,Qt::UserRole+2).toString();
        if(!feature.isEmpty()) execute("feature.editDialog",{{"featureId",feature}});
    });
    browserLayout->addWidget(tree_, 1);
    auto* browserNote = new QLabel("Create a sketch, box or cylinder.\nDouble-click a Browser or Timeline\nfeature to edit it."); browserNote_=browserNote;
    browserNote->setStyleSheet("color: #89939b; font-size: 11px; border: 0; padding: 8px 4px;");
    browserLayout->addWidget(browserNote);
    body->addWidget(browser_);
    auto* canvas = new QVBoxLayout; canvas->setContentsMargins(0, 0, 0, 0); canvas->setSpacing(0);
    // Scope Qt docking to the canvas, preserving the toolbar and Timeline width.
    canvasWindow_=new QMainWindow;canvasWindow_->setWindowFlags(Qt::Widget);
    canvasWindow_->setCentralWidget(view_);canvas->addWidget(canvasWindow_,1);
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
    for(const auto& tool:QStringList{"line","rectangle","dimension"}) {
        auto* shortcut=new QShortcut(QKeySequence(tool=="line" ? "L" : tool=="rectangle" ? "R" : "D"),this);
        connect(shortcut,&QShortcut::activated,this,[this,tool]{
            if(session_.value("command").toString()=="sketch.edit" && !qobject_cast<QLineEdit*>(QApplication::focusWidget()))execute("sketch.tool",{{"tool",tool}});
        });
    }
    auto* extrudeShortcut=new QShortcut(QKeySequence("E"),this);
    connect(extrudeShortcut,&QShortcut::activated,this,[this]{
        if(session_.isEmpty() && !qobject_cast<QLineEdit*>(QApplication::focusWidget()))execute("solid.extrudeDialog");
    });
    sketchEscape_=new QShortcut(QKeySequence(Qt::Key_Escape),view_);
    sketchEscape_->setContext(Qt::WidgetShortcut);sketchEscape_->setEnabled(false);
    connect(sketchEscape_,&QShortcut::activated,this,[this]{
        if(session_.value("command").toString()=="sketch.edit") {
            if(session_.contains("pendingDimension"))execute("sketch.dimensionCancel");
            else execute("sketch.tool",{{"tool","select"}});
        }
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
        if(error.isEmpty())view_->fit(document_.bodies(),document_.sketches());
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
            view_->fit(document_.bodies(),document_.sketches()); return document_.state();
        });
        add("primitive."+type,type=="box" ? "Box" : "Cylinder","Open a shared primitive dimension dialog.",{},[this,id](const auto&){return beginSession(id,{});});
    }
    auto extrusionDistance=parameter("number","Signed one-side distance in display units along the profile normal; zero is invalid.");
    extrusionDistance["minimum"]=-100000;extrusionDistance["maximum"]=100000;
    auto sketchIdentity=parameter("string","Stable source sketch UUID.");
    auto profileIdentity=parameter("string","Stable accepted closed-profile UUID.");
    QJsonObject extrudeSchema{{"sketchId",sketchIdentity},{"profileId",profileIdentity},{"distance",extrusionDistance},
        {"units",optional(units)},{"name",optional(parameter("string","Extrusion feature name."))},{"featureId",optional(featureId)}};
    add("solid.extrude","Create or edit extrusion","Commit a one-side distance New Body extrusion of an accepted profile.",extrudeSchema,[this](const auto& a){
        QJsonObject candidate;try {candidate=extrusionCandidate(a,true);}catch(const std::exception& error){return failure("evaluation_failed",QString::fromUtf8(error.what()));}
        const auto error=document_.commit(candidate,a.value("featureId").toString().isEmpty() ? "Extrude" : "Edit extrusion");
        if(!error.isEmpty()){auto result=failure("evaluation_failed",error);auto details=result["error"].toObject();details["diagnostics"]=document_.lastEvaluationError();result["error"]=details;return result;}
        selectedSketch_.clear();selectedProfile_.clear();view_->selectProfile({},{});
        const auto target=a.value("featureId").toString(candidate["features"].toArray().last().toObject()["id"].toString());
        for(const auto& body:document_.bodies())if(body.featureId==target)selectedBody_=body.bodyId;
        return document_.state();
    });
    add("solid.extrudeDialog","Extrude","Open a docked profile extrusion with disposable live preview (E).",
        {{"sketchId",optional(sketchIdentity)},{"profileId",optional(profileIdentity)}},[this](const auto& a){return beginSession("solid.extrude",a);});
    add("feature.visibility","Show or hide feature","Change actual authored visibility in one undoable transaction.",
        {{"featureId",featureId},{"visible",parameter("boolean","Whether the feature is visible.")}},[this](const auto& a){
            auto candidate=document_.authored();auto features=candidate["features"].toArray();bool found=false;
            for(int i=0;i<features.size();++i){auto feature=features[i].toObject();if(feature["id"]!=a["featureId"])continue;feature["visible"]=a["visible"];features[i]=feature;found=true;break;}
            if(!found)return failure("stale_entity","Feature does not exist","featureId");candidate["features"]=features;
            const auto error=document_.commit(candidate,"Change visibility");return error.isEmpty() ? document_.state() : failure("evaluation_failed",error);
        });
    auto sketchPlane=parameter("string","Origin plane of the authored two-dimensional sketch.");
    sketchPlane["enum"]=QJsonArray{"XY","XZ","YZ"};
    auto sketchId=parameter("string","Stable authored sketch feature UUID.");
    QJsonObject sketchBeginSchema{{"plane",sketchPlane},{"name",name},{"units",optional(units)}};
    auto sketchCreateSchema=sketchBeginSchema;sketchCreateSchema["plane"]=optional(sketchPlane);
    add("sketch.create","Create Sketch","Choose an origin plane, then author retained sketch geometry.",sketchCreateSchema,[this](const auto& a){
        if(a.contains("plane")) {
            auto result=beginSketch(a["plane"].toString(),a.value("name").toString("Sketch"));
            if(!result["ok"].isBool() && a.contains("units")){auto inputs=session_.value("inputs").toObject();inputs["units"]=a["units"];session_["inputs"]=inputs;result=session_;}
            return result;
        }
        if(!session_.isEmpty())return failure("session_busy","Complete or cancel the active command first");
        session_={{"id",CadDocument::newId()},{"command","sketch.plane"},{"status","active"},{"inputs",QJsonObject{{"name",a.value("name").toString("Sketch")},{"units",a.value("units").toString(document_.authored()["displayUnits"].toString())}}},
            {"baseDocumentRevision",static_cast<qint64>(document_.revision())},{"previewed",false}};
        showSketch();return session_;
    });
    add("sketch.begin","Choose sketch plane","Enter origin-plane sketch editing with a disposable authored draft.",sketchBeginSchema,[this](const auto& a){
        auto result=beginSketch(a["plane"].toString(),a.value("name").toString(session_.value("inputs").toObject().value("name").toString("Sketch")));
        if(!result["ok"].isBool() && a.contains("units")){auto inputs=session_.value("inputs").toObject();inputs["units"]=a["units"];session_["inputs"]=inputs;result=session_;}
        return result;
    });
    add("sketch.edit","Edit Sketch","Re-enter an authored sketch without changing it until Finish.",{{"sketchId",sketchId}},[this](const auto& a){
        const auto feature=SketchGeometry::findFeature(document_.authored(),a["sketchId"].toString());
        return beginSketch(feature["parameters"].toObject()["plane"].toString(),feature["name"].toString(),feature["id"].toString());
    });
    auto coordinate=parameter("number","Sketch coordinate in the supplied units or the active display units.");
    coordinate["minimum"]=-100000;coordinate["maximum"]=100000;
    QJsonObject drawSchema{{"x1",coordinate},{"y1",coordinate},{"x2",coordinate},{"y2",coordinate},
        {"units",optional(units)},{"session",optional(parameter("string","Optional active sketch session identity."))}};
    for(const auto& tool:QStringList{"line","rectangle"})add("sketch."+tool,tool=="line" ? "Line" : "Two-Point Rectangle",
        "Append stable authored points and edges to the active sketch draft.",drawSchema,[this,tool](const auto& a){return appendSketchEntity(tool,a);});
    auto sketchTool=parameter("string","Drawing tool in the active sketch context.");sketchTool["enum"]=QJsonArray{"select","line","rectangle","dimension"};
    add("sketch.tool","Sketch drawing tool","Choose a tool and clear its pending first point.",{{"tool",sketchTool}},[this](const auto& a){
        if(session_.value("command").toString()!="sketch.edit")return failure("no_active_sketch","Enter a sketch first");
        cancelDimension();
        session_["tool"]=a["tool"];session_.remove("firstPoint");session_.remove("pointerPoint");view_->clearSketchCursor();
        view_->setFocus();
        return session_;
    });
    auto sketchPixel=parameter("number","Viewport-local logical pixel coordinate.");sketchPixel["minimum"]=0;sketchPixel["maximum"]=10000;
    const QJsonObject sketchPixels{{"x",sketchPixel},{"y",sketchPixel}};
    auto sketchPickSchema=sketchPixels;sketchPickSchema["additive"]=optional(parameter("boolean","Add or toggle a target in the current sketch selection."));
    auto ids=optional(parameter("array","Authored point or line IDs in this sketch; omitted arrays default empty."));
    add("sketch.selectionSet","Select sketch geometry","Set shared authored-point, line or evaluated-profile selection without changing geometry.",{{"pointIds",ids},{"entityIds",ids},{"profileIds",ids}},[this](const auto& a){
        return setSketchSelection({{"pointIds",a.value("pointIds").toArray()},{"entityIds",a.value("entityIds").toArray()},{"profileIds",a.value("profileIds").toArray()}});
    });
    add("sketch.selectionPick","Pick sketch geometry","Pick a visible projected authored point or line in logical pixels.",sketchPickSchema,[this](const auto& a){
        if(session_.value("command").toString()!="sketch.edit")return failure("no_active_sketch","Enter a sketch first");
        const auto picked=view_->pickSketch(session_.value("draftSketch").toObject(),a["x"].toDouble(),a["y"].toDouble());
        auto selection=a.value("additive").toBool() ? session_.value("selection").toObject() : QJsonObject{{"pointIds",QJsonArray{}},{"entityIds",QJsonArray{}}};
        if(!picked.isEmpty()) {
            const auto field=picked["kind"]=="point" ? QString("pointIds") : picked["kind"]=="profile" ? QString("profileIds") : QString("entityIds");
            const auto id=picked[picked["kind"]=="point" ? "pointId" : picked["kind"]=="profile" ? "profileId" : "entityId"];
            auto selected=selection.value(field).toArray();const int index=selected.contains(id) ? [&]{for(int i=0;i<selected.size();++i)if(selected[i]==id)return i;return -1;}() : -1;
            if(index>=0 && a.value("additive").toBool())selected.removeAt(index);else if(index<0)selected.append(id);
            selection[field]=selected;
        }
        auto result=setSketchSelection(selection);if(!result["ok"].isBool())result["picked"]=picked;return result;
    });
    add("sketch.placePoint","Place sketch point","Resolve an actual viewport point on the active plane; two clicks author the chosen entity.",sketchPickSchema,[this](const auto& a){
        if(session_.value("command").toString()!="sketch.edit")return failure("no_active_sketch","Enter a sketch first");
        if(session_.value("baseDocumentRevision").toInteger()!=static_cast<qint64>(document_.revision()))return failure("stale_session","The document changed; cancel and restart this sketch");
        const auto tool=session_.value("tool").toString();
        if(tool=="dimension" && session_.value("pendingDimension").toObject().value("phase")=="placing") {
            const auto response=execute("sketch.dimensionPlace",{{"x",a["x"]},{"y",a["y"]}});return response["ok"].toBool() ? response["result"].toObject() : response;
        }
        if(tool=="select" || tool=="dimension") {
            auto args=a;if(tool=="dimension" && !session_.value("selection").toObject()["pointIds"].toArray().isEmpty())args["additive"]=true;
            const auto response=execute("sketch.selectionPick",args);return response["ok"].toBool() ? response["result"].toObject() : response;
        }
        const auto point=view_->sketchPoint(a["x"].toDouble(),a["y"].toDouble(),session_.value("inputs").toObject()["plane"].toString());
        if(!session_.contains("firstPoint")){session_["firstPoint"]=point;return QJsonObject{{"placement",false},{"firstPoint",point}};}
        const auto first=session_.value("firstPoint").toObject();
        const auto response=appendSketchEntity(tool,{{"x1",first["uMm"]},{"y1",first["vMm"]},{"x2",point["uMm"]},{"y2",point["vMm"]},{"units","mm"}});
        if(response["ok"].isBool() && !response["ok"].toBool())return response;
        if(tool=="line")session_["firstPoint"]=point;
        return QJsonObject{{"placement",true},{"point",point},{"draftSketch",session_.value("draftSketch")}};
    });
    add("sketch.pointerMove","Sketch pointer preview","Preview a pending line or rectangle on the GPU without authored changes.",sketchPixels,[this](const auto& a){
        if(session_.value("command")=="sketch.edit" && session_.value("tool")=="dimension") {
            if(session_.value("pendingDimension").toObject().value("phase")=="editing")return QJsonObject{};
            try{return previewDimension(a);}catch(const std::invalid_argument&){return QJsonObject{};}
        }
        if(session_.value("command").toString()!="sketch.edit" || !session_.contains("firstPoint"))return QJsonObject{};
        if(session_.value("baseDocumentRevision").toInteger()!=static_cast<qint64>(document_.revision())){view_->clearSketchCursor();return QJsonObject{};}
        const auto plane=session_.value("inputs").toObject()["plane"].toString();
        const auto point=view_->sketchPoint(a["x"].toDouble(),a["y"].toDouble(),plane),first=session_.value("firstPoint").toObject();
        session_["pointerPoint"]=point;
        view_->showSketchCursor(plane,{first["uMm"].toDouble(),first["vMm"].toDouble()},{point["uMm"].toDouble(),point["vMm"].toDouble()},session_.value("tool").toString()=="rectangle");
        return QJsonObject{{"point",point},{"authoredChange",false}};
    });
    add("sketch.lookAt","Look At sketch","Orient the view normal to the active sketch plane.",{},[this](const auto&){
        if(session_.value("command").toString()!="sketch.edit")return failure("no_active_sketch","Enter a sketch first");
        view_->lookAtSketch(session_.value("inputs").toObject()["plane"].toString());return QJsonObject{};
    });
    add("sketch.finish","Finish Sketch","Commit the active sketch draft through the shared undo authority.",{},[this](const auto&){
        if(!session_.value("command").toString().startsWith("sketch."))return failure("no_active_sketch","Enter a sketch first");
        return finishSession(session_.value("id").toString(),true);
    });
    add("sketch.cancel","Cancel Sketch","Discard the active sketch draft without an undo entry.",{},[this](const auto&){
        if(!session_.value("command").toString().startsWith("sketch."))return failure("no_active_sketch","Enter a sketch first");
        return finishSession(session_.value("id").toString(),false);
    });
    add("view.projectSketchPoint","Project sketch coordinate","Project canonical sketch millimeters into viewport-local logical pixels.",{{"plane",sketchPlane},{"uMm",coordinate},{"vMm",coordinate}},[this](const auto& a){
        return view_->projectSketchPoint(a["uMm"].toDouble(),a["vMm"].toDouble(),a["plane"].toString());
    });
    auto optionalSession=optional(parameter("string","Optional active sketch session identity."));
    auto constraintType=parameter("string","Geometric relation to apply to the supplied or selected sketch geometry.");
    constraintType["enum"]=QJsonArray{"coincident","horizontal","vertical","parallel","perpendicular","fixed"};
    add("sketch.constraint","Constrain sketch geometry","Solve a geometric relation before replacing the usable sketch draft.",
        {{"type",constraintType},{"pointIds",ids},{"entityIds",ids},{"session",optionalSession}},[this](const auto& a){
            if(session_.value("command").toString()!="sketch.edit")return failure("no_active_sketch","Enter a sketch first");
            if(a.contains("session") && a["session"]!=session_.value("id"))return failure("invalid_session","Session is no longer active");
            QJsonObject args{{"type",a["type"]}};const auto field=a["type"]=="coincident" || a["type"]=="fixed" ? "pointIds" : "entityIds";
            args[field]=a.contains(field) ? a[field] : session_.value("selection").toObject().value(field);
            return publishSketchSolve(SketchGeometry::addConstraint(session_.value("draftSketch").toObject(),args));
        });
    auto kind=parameter("string","Dimension measurement kind.");kind["enum"]=QJsonArray{"linear","horizontal","vertical"};
    auto dimValue=coordinate;dimValue["minimum"]=0.000001;
    add("sketch.dimension","Add sketch dimension","Create a driving dimension or a driven measurement on two points or one line.",
        {{"kind",kind},{"pointIds",ids},{"entityId",optional(parameter("string","Selected line shorthand for its two endpoints."))},
            {"value",optional(dimValue)},{"units",optional(units)},{"annotation",optional(parameter("object","Optional canonical placement {uMm,vMm}."))},{"driving",optional(parameter("boolean","True drives geometry; false measures it."))},{"session",optionalSession}},[this](const auto& a){
            if(session_.value("command").toString()!="sketch.edit")return failure("no_active_sketch","Enter a sketch first");
            if(a.contains("session") && a["session"]!=session_.value("id"))return failure("invalid_session","Session is no longer active");
            auto args=sketchDimensionTargets(a);args["kind"]=a["kind"];args["driving"]=a.value("driving").toBool(true);
            if(a.contains("annotation"))args["annotation"]=a["annotation"];
            if(args["driving"].toBool()) {
                const auto units=a.value("units").toString(session_.value("inputs").toObject()["units"].toString());
                if(a.contains("value"))args["valueMm"]=CadDocument::millimeters(a["value"].toDouble(),units);
                else {
                    QMap<QString,QJsonObject> points;for(const auto& entry:session_.value("draftSketch").toObject()["parameters"].toObject()["points"].toArray()){const auto point=entry.toObject();points[point["id"].toString()]=point;}
                    const auto selected=args["pointIds"].toArray();const auto p=points.value(selected[0].toString()),q=points.value(selected[1].toString());
                    const double u=std::abs(q["uMm"].toDouble()-p["uMm"].toDouble()),v=std::abs(q["vMm"].toDouble()-p["vMm"].toDouble());
                    args["valueMm"]=a["kind"]=="horizontal" ? u : a["kind"]=="vertical" ? v : std::hypot(u,v);
                }
            }
            return publishSketchSolve(SketchGeometry::addDimension(session_.value("draftSketch").toObject(),args));
        });
    add("sketch.dimensionUpdate","Edit driving dimension","Solve the new dimension value before replacing the usable draft.",
        {{"dimensionId",parameter("string","Stable authored dimension UUID.")},{"value",dimValue},{"units",optional(units)},{"session",optionalSession}},[this](const auto& a){
            if(session_.value("command").toString()!="sketch.edit")return failure("no_active_sketch","Enter a sketch first");
            if(a.contains("session") && a["session"]!=session_.value("id"))return failure("invalid_session","Session is no longer active");
            const auto units=a.value("units").toString(session_.value("inputs").toObject()["units"].toString());
            return publishSketchSolve(SketchGeometry::updateDimension(session_.value("draftSketch").toObject(),{{"dimensionId",a["dimensionId"]},{"valueMm",CadDocument::millimeters(a["value"].toDouble(),units)}}));
        });
    auto previewKind=optional(kind);previewKind["enum"]=QJsonArray{"auto","linear","horizontal","vertical"};
    auto previewSchema=sketchPixels;previewSchema["kind"]=previewKind;previewSchema["driving"]=optional(parameter("boolean","False previews a driven measurement."));previewSchema["session"]=optionalSession;
    add("sketch.dimensionPreview","Preview dimension placement","Preview a complete GPU annotation on selected targets without changing intent.",previewSchema,[this](const auto& a){return previewDimension(a);});
    auto placeSchema=sketchPixels;placeSchema["session"]=optionalSession;
    add("sketch.dimensionPlace","Place dimension annotation","Fix preview placement and open the focused inline numeric editor.",placeSchema,[this](const auto& a){
        auto response=previewDimension(a);if(response.value("ok").isBool() && !response.value("ok").toBool())return response;
        auto pending=session_.value("pendingDimension").toObject();pending["phase"]="editing";session_["pendingDimension"]=pending;showDimensionEditor();return pending;
    });
    add("sketch.dimensionAccept","Accept dimension value","Atomically add the placed dimension or solve an existing driving edit; mm/in suffixes supported.",
        {{"text",optional(parameter("string","Number with optional mm/in suffix; expressions unsupported."))},{"value",optional(dimValue)},{"units",optional(units)},{"session",optionalSession}},[this](const auto& a){return acceptDimension(a);});
    add("sketch.dimensionCancel","Cancel pending dimension","Discard unaccepted placement or editing, retaining the active dimension tool.",{{"session",optionalSession}},[this](const auto& a){
        if(session_.value("command")!="sketch.edit")return failure("no_active_sketch","Enter a sketch first");
        if(a.contains("session") && a["session"]!=session_.value("id"))return failure("invalid_session","Session is no longer active");
        cancelDimension();view_->setFocus();return QJsonObject{{"pending",false},{"authoredChange",false}};
    });
    const auto dimensionId=parameter("string","Stable authored dimension UUID.");
    add("sketch.annotationPick","Pick dimension annotation","Resolve a rotated logical-pixel label hit to the same stable dimension selection.",sketchPixels,[this](const auto& a){
        if(session_.value("command")!="sketch.edit")return failure("no_active_sketch","Enter a sketch first");
        const auto picked=view_->pickAnnotation(a["x"].toDouble(),a["y"].toDouble());
        if(!picked.isEmpty()){auto selection=session_.value("selection").toObject();selection["dimensionId"]=picked.value("dimensionId");session_["selection"]=selection;}
        return picked;
    });
    add("sketch.dimensionEdit","Edit canvas dimension","Open a driving value editor or read-only driven measurement.",{{"dimensionId",dimensionId},{"session",optionalSession}},[this](const auto& a){
        if(a.contains("session") && a["session"]!=session_.value("id"))return failure("invalid_session","Session is no longer active");return editDimension(a["dimensionId"].toString());
    });
    add("sketch.annotationSelect","Select dimension annotation","Select an existing dimension by stable identity without changing geometry.",{{"dimensionId",dimensionId}},[this](const auto& a){
        if(session_.value("command")!="sketch.edit")return failure("no_active_sketch","Enter a sketch first");
        bool found=false;for(const auto& entry:session_.value("draftSketch").toObject()["parameters"].toObject()["dimensions"].toArray())found|=entry.toObject()["id"]==a["dimensionId"];
        if(!found)return failure("stale_entity","Dimension does not exist");
        auto selection=session_.value("selection").toObject();selection["dimensionId"]=a["dimensionId"];session_["selection"]=selection;return selection;
    });
    auto annotationCoordinate=parameter("number","Canonical sketch-plane placement in millimeters.");annotationCoordinate["minimum"]=-100000;annotationCoordinate["maximum"]=100000;
    add("sketch.annotationMove","Move dimension annotation","Change canonical placement through the shared sketch draft, preserving geometry and dimension value.",
        {{"dimensionId",dimensionId},{"uMm",annotationCoordinate},{"vMm",annotationCoordinate},{"session",optionalSession}},[this](const auto& a){
            if(session_.value("command")!="sketch.edit")return failure("no_active_sketch","Enter a sketch first");
            if(a.contains("session") && a["session"]!=session_.value("id"))return failure("invalid_session","Session is no longer active");
            return publishSketchSolve(SketchGeometry::updateAnnotation(session_.value("draftSketch").toObject(),
                {{"dimensionId",a["dimensionId"]},{"annotation",QJsonObject{{"uMm",a["uMm"]},{"vMm",a["vMm"]}}}}));
        });
    add("sketch.movePoint","Move sketch point","Solve a seeded point movement without deforming the usable draft on failure.",
        {{"pointId",parameter("string","Stable authored point UUID.")},{"x",coordinate},{"y",coordinate},{"units",optional(units)},{"session",optionalSession}},[this](const auto& a){
            if(session_.value("command").toString()!="sketch.edit")return failure("no_active_sketch","Enter a sketch first");
            if(a.contains("session") && a["session"]!=session_.value("id"))return failure("invalid_session","Session is no longer active");
            const auto units=a.value("units").toString(session_.value("inputs").toObject()["units"].toString());
            return publishSketchSolve(SketchGeometry::movePoint(session_.value("draftSketch").toObject(),{{"pointId",a["pointId"]},{"uMm",CadDocument::millimeters(a["x"].toDouble(),units)},{"vMm",CadDocument::millimeters(a["y"].toDouble(),units)}}));
        });
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
            if(feature["type"].toString()=="sketch")return beginSketch(feature["parameters"].toObject()["plane"].toString(),feature["name"].toString(),feature["id"].toString());
            const auto p=feature["parameters"].toObject();
            if(feature["type"]=="extrude") {
                const auto unit=document_.authored()["displayUnits"].toString();const double scale=unit=="in" ? 25.4 : 1;
                return beginSession("solid.extrude",{{"featureId",feature["id"]},{"sketchId",p["sketchId"]},{"profileId",p["profileId"]},
                    {"distance",p["distanceMm"].toDouble()/scale},{"units",unit},{"name",feature["name"]}});
            }
            const auto unit=document_.authored()["displayUnits"].toString();const double scale=unit=="in" ? 25.4 : 1;
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
        selectedBody_=id;selectedSketch_.clear();selectedProfile_.clear();view_->selectProfile({},{});view_->selectBody(id);return QJsonObject{{"bodyId",id},{"kind","body"}};
    });
    add("selection.profileSet","Select profile","Select a retained accepted region through the same path as canvas picking.",
        {{"sketchId",sketchIdentity},{"profileId",profileIdentity}},[this](const auto& a){return selectFinishedProfile(a["sketchId"].toString(),a["profileId"].toString());});
    add("selection.sketchSet","Select sketch","Select a finished sketch through the same path as the Browser.",
        {{"sketchId",sketchIdentity}},[this](const auto& a){
            const auto id=a["sketchId"].toString();for(const auto& sketch:document_.sketches())if(sketch.featureId==id) {
                selectedBody_.clear();selectedSketch_=id;selectedProfile_.clear();view_->selectBody({});view_->selectProfile(id,{});
                if(session_.value("command")=="solid.extrude" && sketch.profiles.size()==1)return selectFinishedProfile(id,sketch.profiles.front().profileId);
                return QJsonObject{{"kind","sketch"},{"sketchId",id}};
            }
            return failure("stale_entity","Sketch does not exist","sketchId");
        });
    auto point=parameter("number","Viewport-local logical pixel coordinate.");point["minimum"]=0;point["maximum"]=10000;
    add("selection.pick","Pick body","Pick actual OCCT geometry at viewport-local coordinates.",{{"x",point},{"y",point}},[this](const auto& a){
        const bool selectingProfile=session_.value("command")=="solid.extrude";
        const auto profile=view_->pickFinishedProfile(a["x"].toDouble(),a["y"].toDouble());
        selectedBody_=selectingProfile ? QString{} : view_->pickBody(a["x"].toDouble(),a["y"].toDouble());
        if(!profile.isEmpty() && (selectingProfile || selectedBody_.isEmpty()))return selectFinishedProfile(profile["sketchId"].toString(),profile["profileId"].toString());
        selectedSketch_.clear();selectedProfile_.clear();view_->selectProfile({},{});view_->selectBody(selectedBody_);
        return QJsonObject{{"bodyId",selectedBody_},{"kind","body"}};
    });
    add("view.projectBody","Project body center","Return actual OCCT projection for a body's center, in logical pixels.",{{"bodyId",bodyId}},[this](const auto& a){
        for(const auto& body:document_.bodies()) if(body.bodyId==a["bodyId"].toString())return view_->projectBody(body);
        return failure("stale_entity","Body does not exist");
    });
    auto sessionTarget = parameter("string", "Implemented session target; modeling targets follow alongside their features.");
    sessionTarget["enum"] = QJsonArray{"document.units", "commands.search","solid.box","solid.cylinder","solid.extrude","sketch.create"};
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
        } else if(target=="solid.extrude") {
            if(updates.contains("featureId") && updates.value("featureId")!=session_.value("inputs").toObject().value("featureId"))return failure("invalid_arguments","Extrusion edit identity cannot change","featureId");
            const auto error=validateExtrudeInputs(inputs);if(!error.isEmpty())return failure("invalid_arguments",error);
        } else if(target.startsWith("sketch.")) {
            for(auto it=updates.begin();it!=updates.end();++it)
                if(it.key()!="name" && it.key()!="units")return failure("invalid_arguments","Only sketch name and display units can change in this draft",it.key());
            if(!inputs["name"].isString() || inputs["name"].toString().trimmed().isEmpty())return failure("invalid_arguments","Sketch name must be nonempty","name");
            if(inputs["units"]!="mm" && inputs["units"]!="in")return failure("invalid_arguments","Unsupported display units","units");
            if(target=="sketch.edit"){auto draft=session_.value("draftSketch").toObject();draft["name"]=inputs["name"];session_["draftSketch"]=draft;}
        } else {
            QString field;
            const auto error = validateArguments(*findCommand(target), inputs, &field);
            if (!error.isEmpty()) return failure("invalid_arguments", error, field);
        }
        session_["inputs"] = inputs;if(target=="solid.extrude")session_["source"]=QJsonObject{{"sketchId",inputs.value("sketchId")},{"profileId",inputs.value("profileId")}};
        session_["previewed"] = false;view_->clearPreview();return session_;
    });
    add("command.preview", "Inspect command preview", "Inspect disposable authored draft; no evaluated geometry is implied.", sessionId, [this](const auto& a) {
        if (session_.isEmpty() || session_.value("id") != a["session"]) return failure("invalid_session", "Session is no longer active");
        if(session_.value("command")=="solid.extrude")return previewExtrude();
        session_["previewed"]=false;view_->clearPreview();
        if (session_.value("command").toString() != "commands.search"
            && session_.value("baseDocumentRevision").toInteger() != static_cast<qint64>(document_.revision()))
            return failure("stale_session", "The document changed; cancel and restart this edit");
        auto result = session_;
        if (session_.value("command").toString() == "document.units") {
            auto draft = document_.authored(); draft["displayUnits"] = session_.value("inputs").toObject()["units"];
            result["authoredDraft"] = draft;
        }
        if(session_.value("command").toString()=="sketch.edit") {
            const auto draft=session_.value("draftSketch").toObject();
            const auto evaluated=SketchGeometry::evaluateFeature(draft);view_->showSketchDraft(evaluated);
            result["authoredDraft"]=draft;
            result["documentDraft"]=SketchGeometry::replaceFeature(document_.authored(),draft);
            result["sketchGeometry"]=evaluated.summary;result["geometryEvaluated"]=true;
        } else if(session_.value("command").toString().startsWith("solid.")) {
            const auto candidate=primitiveCandidate(session_.value("command").toString(),session_.value("inputs").toObject());
            const auto features=candidate["features"].toArray();
            const auto id=session_.value("inputs").toObject()["featureId"].toString();
            for(const auto& entry:features) {const auto feature=entry.toObject();if(!id.isEmpty() && feature["id"]!=id)continue;
                if(id.isEmpty() && feature!=features.last().toObject())continue;
                const auto shape=GeometryKernel::primitive(feature["type"].toString(),feature["parameters"].toObject());
                view_->showPreview(shape,id.isEmpty() ? QString{} : feature["bodyId"].toString());
                result["geometry"]=GeometryKernel::summarize(shape);break;
            }
            result["geometryEvaluated"]=true;
        } else result["geometryEvaluated"] = false;
        session_["previewed"]=true;result["previewed"]=true;return result;
    });
    add("command.commit", "Commit command session", "Commit the validated draft through the same authoritative command.", sessionId,
        [this](const auto& a) { return finishSession(a["session"].toString(), true); });
    add("command.cancel", "Cancel command session", "Discard the real dialog draft without an undo entry.", sessionId,
        [this](const auto& a) { return finishSession(a["session"].toString(), false); });
    add("command.panelDock","Dock command panel","Dock or float the active modeling panel through Qt.",
        {{"docked",parameter("boolean","True docks the panel on the right; false floats it.")}},[this](const auto& a){
            const auto target=session_.value("command").toString();
            auto* dock=target=="solid.extrude" ? extrudeDock_ : target.startsWith("sketch.") ? sketchDock_ : target.startsWith("solid.") ? primitiveDock_ : nullptr;
            if(!dock)
                return failure("no_active_dialog","No modeling panel is active");
            dock->setFloating(!a["docked"].toBool());
            if(a["docked"].toBool())canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,dock);
            return QJsonObject{{"docked",!dock->isFloating()}};
        });
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
    add("view.home", "Home view", "Fit visible bodies and sketches in the isometric view.", {}, [this](const auto&) {
        view_->orientation="isometric";
        if(session_.value("command")=="solid.extrude" && session_.value("previewed").toBool()) {
            const auto candidate=CadDocument::evaluateCandidate(session_.value("documentDraft").toObject());
            if(candidate.ok){view_->fit(candidate.evaluation->bodies,candidate.evaluation->sketches);return QJsonObject{};}
        }
        view_->fit(document_.bodies(),document_.sketches());return QJsonObject{};
    });
    add("capture.screenshot", "Capture screenshot", "Save a PNG of the actual main window, excluding OS decorations and separate dialogs.", {{"path", parameter("string", "Absolute destination PNG path; parent directory must exist.")}}, [this](const auto& a) {
        const QString path = a.value("path").toString();
        if (!QDir::isAbsolutePath(path) || !path.endsWith(".png", Qt::CaseInsensitive)) return failure("invalid_arguments", "Screenshot requires an absolute .png path");
        auto pixels = grab();
        const auto framebuffer = view_->framebuffer();
        if (framebuffer.isNull()) return failure("capture_failed", "OCCT framebuffer capture failed");
        { QPainter composition(&pixels); composition.drawImage(view_->mapTo(this, QPoint(0, 0)), framebuffer);
            if(dimensionEditor_ && dimensionEditor_->isVisible())composition.drawPixmap(dimensionEditor_->mapTo(this,QPoint()),dimensionEditor_->grab()); }
        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly) || !pixels.save(&file, "PNG") || !file.commit()) return failure("capture_failed", "Could not write screenshot");
        return QJsonObject{{"path", path}, {"pixelWidth", pixels.width()}, {"pixelHeight", pixels.height()}, {"devicePixelRatio", pixels.devicePixelRatio()}, {"source", "QWidget chrome + OCCT OpenGL framebuffer"}, {"viewportPixelWidth", framebuffer.width()}, {"viewportPixelHeight", framebuffer.height()}, {"viewportSha256", QString::fromLatin1(QCryptographicHash::hash(QByteArrayView(reinterpret_cast<const char*>(framebuffer.constBits()), framebuffer.sizeInBytes()), QCryptographicHash::Sha256).toHex())}};
    });
    add("capture.dialog", "Capture active dialog", "Capture the real search/settings QWidget; no geometry or OS decoration claim.", {{"path", parameter("string", "Absolute destination PNG path.")}}, [this](const auto& a) {
        const auto path = a["path"].toString();
        if (!QDir::isAbsolutePath(path) || !path.endsWith(".png", Qt::CaseInsensitive)) return failure("invalid_arguments", "Requires an absolute PNG path", "path");
        QWidget* dialog = session_.value("command").toString() == "commands.search" ? search_
            : session_.value("command").toString() == "solid.extrude" ? extrudePanel_
            : session_.value("command").toString().startsWith("solid.") ? primitivePanel_
            : session_.value("command").toString().startsWith("sketch.") ? sketchPanel_ : unitsDialog_;
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
    for (const auto& pair : QList<QPair<QString, QString>>{{"solid.revolve", "Revolve"}, {"solid.fillet", "Fillet"}, {"solid.shell", "Shell"}, {"solid.move", "Move"}, {"construction.plane", "Plane"}, {"inspect.measure", "Measure"}}) {
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
        if (value.isUndefined() || (type == "boolean" && !value.isBool()) || (type == "string" && !value.isString()) || (type == "number" && !value.isDouble()) || (type == "object" && !value.isObject()) || (type == "array" && !value.isArray())) return "Argument " + it.key() + " must be " + type;
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
        : target=="solid.extrude" ? bool(extrudePanel_ && extrudePanel_->isVisible())
        : target.startsWith("solid.") ? bool(primitivePanel_ && primitivePanel_->isVisible())
        : target.startsWith("sketch.") ? bool(sketchPanel_ && sketchPanel_->isVisible()) : bool(unitsDialog_ && unitsDialog_->isVisible());
    const auto dialogInputs=session_.isEmpty() ? QJsonObject{} : target=="solid.extrude" ? session_.value("inputs").toObject() : target.startsWith("solid.") ? primitiveWidgetInputs()
        : target.startsWith("sketch.") ? session_.value("inputs").toObject()
        : target=="commands.search" ? QJsonObject{{"query",searchInput_ ? searchInput_->text() : QString{}}}
        : QJsonObject{{"units",unitsInput_ ? unitsInput_->currentText() : QString{}}};
    const auto* activeDock=target=="solid.extrude" ? extrudeDock_ : target.startsWith("sketch.") ? sketchDock_ : primitiveDock_;
    const auto panelRect=activeDock ? QRect(activeDock->mapToGlobal(QPoint())-mapToGlobal(QPoint()),activeDock->size()) : QRect{};
    const auto viewPosition=view_->mapTo(this,QPoint());
    return {{"application", "Alloy"}, {"version", "0.1.0"}, {"stage", "sketch-authoring"},
        {"runtime", QJsonObject{{"qtVersion", qVersion()}, {"occtBuildVersion", ALLOY_OCCT_VERSION}}},
        {"revision", static_cast<qint64>(revision_)},
        {"identity", identity()}, {"activeSession", session_},
        {"inlineEditor",inlineEditorState()},
        {"document", document_.state()},
        {"selection",target=="sketch.edit" ? QJsonObject{{"kind","sketch"},{"sketchId",session_.value("draftSketch").toObject()["id"]},
            {"pointIds",session_.value("selection").toObject()["pointIds"]},{"entityIds",session_.value("selection").toObject()["entityIds"]},
            {"profileIds",session_.value("selection").toObject().value("profileIds").toArray()},{"dimensionId",session_.value("selection").toObject().value("dimensionId")}}
            : !selectedProfile_.isEmpty() ? QJsonObject{{"kind","profile"},{"sketchId",selectedSketch_},{"profileIds",QJsonArray{selectedProfile_}},{"profileId",selectedProfile_}}
            : !selectedSketch_.isEmpty() ? QJsonObject{{"kind","sketch"},{"sketchId",selectedSketch_}}
            : QJsonObject{{"kind","body"},{"bodyId",selectedBody_}}},
        {"kernel", QJsonObject{{"candidate", "Open CASCADE"}, {"discoveredVersion", ALLOY_OCCT_VERSION}, {"integrated", true}, {"visualizationIntegrated", true}, {"policy",GeometryKernel::policy()}}},
        {"ui", QJsonObject{{"workspace", "design"}, {"tab",target=="sketch.edit" ? "sketch" : "solid"}, {"browserVisible", browserVisible_}, {"timelineVisible", timelineVisible_}, {"navigationVisible", navigationVisible_}, {"commandSearchVisible", search_ && search_->isVisible()}, {"windowWidth", width()}, {"windowHeight", height()}, {"devicePixelRatio", devicePixelRatioF()},
            {"activeDialog", QJsonObject{{"command", session_.value("command")},
                {"visible",dialogVisible},{"inputs",dialogInputs},
                {"presentation",target.startsWith("solid.") || target.startsWith("sketch.") ? (activeDock && activeDock->isFloating() ? "floating-panel" : "docked-panel") : "dialog"},
                {"bounds",QJsonObject{{"x",panelRect.x()},{"y",panelRect.y()},{"width",panelRect.width()},{"height",panelRect.height()}}}}}}},
        {"viewport", QJsonObject{{"renderer", "OCCT OpenGL"}, {"gpu", view_->diagnostics()}, {"editDisplay",view_->diagnostics().value("editDisplay")}, {"sketchAnnotations",view_->annotations()}, {"orientation", view_->orientation}, {"gridVisible", view_->grid}, {"originVisible", view_->origin}, {"zoom", view_->zoom}, {"pan", QJsonObject{{"x", view_->pan.x()}, {"y", view_->pan.y()}}}, {"x",viewPosition.x()},{"y",viewPosition.y()},{"width", view_->width()}, {"height", view_->height()}}}};
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
        view_->clearPreview();view_->clearSketchDraft();view_->setEditDisplay({},{});displayedSketchDraft_={};session_["previewed"]=false;
    }
    if(displayedDocumentRevision_!=document_.revision()) {
        const QSignalBlocker block(tree_);
        qDeleteAll(bodiesNode_->takeChildren());
        qDeleteAll(sketchesNode_->takeChildren());
        while(auto* item=timelineLayout_->takeAt(0)){delete item->widget();delete item;}
        timelineLayout_->addWidget(new QLabel("TIMELINE"));
        bool selectedExists=false;
        for(const auto& body:document_.bodies()) {
            auto* item=new QTreeWidgetItem(bodiesNode_,{body.name});item->setData(0,Qt::UserRole+1,body.bodyId);item->setData(0,Qt::UserRole+2,body.featureId);item->setFlags(item->flags()|Qt::ItemIsUserCheckable);item->setCheckState(0,body.visible ? Qt::Checked : Qt::Unchecked);
            selectedExists=selectedExists || (body.bodyId==selectedBody_ && body.visible);
        }
        if(!selectedExists)selectedBody_.clear();
        for(const auto& sketch:document_.sketches()) {
            auto* item=new QTreeWidgetItem(sketchesNode_,{sketch.name});item->setData(0,Qt::UserRole+2,sketch.featureId);item->setFlags(item->flags()|Qt::ItemIsUserCheckable);item->setCheckState(0,sketch.visible ? Qt::Checked : Qt::Unchecked);
        }
        // Timeline order follows retained authored feature order, independent
        // of whether evaluation produced a solid body or a sketch wire.
        for(const auto& entry:document_.authored()["features"].toArray()) {
            const auto feature=entry.toObject();const auto id=feature["id"].toString(),name=feature["name"].toString();
            auto* button=new QToolButton;button->setText((feature["type"]=="sketch" ? "▧ " : "")+name);button->setToolTip("Edit "+name);button->setProperty("featureId",id);
            connect(button,&QToolButton::clicked,this,[this,id]{execute("feature.editDialog",{{"featureId",id}});});
            auto* menu=new QMenu(button);menu->addAction("Delete",this,[this,id]{execute("feature.delete",{{"featureId",id}});});
            button->setMenu(menu);button->setPopupMode(QToolButton::MenuButtonPopup);timelineLayout_->addWidget(button);
        }
        timelineLayout_->addStretch();bodiesNode_->setExpanded(true);
        view_->setBodies(document_.bodies(),document_.revision());
        view_->setSketches(document_.sketches(),document_.revision());
        bool sketchExists=false,profileExists=false;
        for(const auto& sketch:document_.sketches())if(sketch.featureId==selectedSketch_ && sketch.visible){sketchExists=true;for(const auto& profile:sketch.profiles)profileExists=profileExists || profile.profileId==selectedProfile_;}
        if(!sketchExists)selectedSketch_.clear();if(!profileExists)selectedProfile_.clear();view_->selectProfile(selectedSketch_,selectedProfile_);
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
    if(session_.value("command").toString()=="solid.extrude")refreshExtrudeControls();
    else if(session_.value("command").toString().startsWith("solid.") && primitivePanel_) {
        const auto inputs=session_.value("inputs").toObject();
        for(auto it=primitiveInputs_.begin();it!=primitiveInputs_.end();++it){const QSignalBlocker block(it.value());it.value()->setValue(inputs[it.key()].toDouble());it.value()->setSuffix(" "+inputs["units"].toString());}
        const QSignalBlocker block(primitiveName_);primitiveName_->setText(inputs["name"].toString());
    }
    const bool editingSketch=session_.value("command").toString()=="sketch.edit";
    sketchToolbar_->setVisible(editingSketch);
    sketchEscape_->setEnabled(editingSketch);
    if(extrudeEscape_)extrudeEscape_->setEnabled(session_.value("command")=="solid.extrude");
    if(editingSketch) {
        const auto inputs=session_.value("inputs").toObject(),draft=session_.value("draftSketch").toObject();
        if(sketchName_){const QSignalBlocker block(sketchName_);sketchName_->setText(inputs["name"].toString());}
        for(auto* input:sketchInputs_)input->setSuffix(" "+inputs["units"].toString());
        const bool current=session_.value("baseDocumentRevision").toInteger()==static_cast<qint64>(document_.revision());
        if(current && draft!=displayedSketchDraft_){view_->showSketchDraft(SketchGeometry::evaluateFeature(draft));displayedSketchDraft_=draft;}
        if(current)refreshSketchControls();
        const auto diagnostics=session_.value("solverDiagnostics").toObject();
        if(sketchInfo_)sketchInfo_->setText(QString("%1 plane · %2 edges\n%3\n%4")
            .arg(inputs["plane"].toString()).arg(draft["parameters"].toObject()["entities"].toArray().size())
            .arg(session_.value("tool").toString()=="select" || session_.value("tool").toString()=="dimension" ? "Click to select · Shift-click to add" : session_.contains("firstPoint") ? "Click the next point · Escape stops drawing" : "Click the first point")
            .arg(diagnostics["dof"].toInt(-1)==0 ? "Fully constrained" : QString("%1 degrees of freedom").arg(diagnostics["dof"].toInt())));
        if(!current && sketchError_)sketchError_->setText("The document changed. Cancel and restart this sketch.");
    }
    view_->sync();
    positionDimensionEditor();
    const auto gpu = view_->diagnostics();
    status_->setText(QString("  %3 · %4 bodies · %1 · Zoom %2%    |    "+QString(editingSketch ? "L: line · R: rectangle · D: dimension · Shift-click: add selection" : "Click: select")+" · Alt/Option + drag: pan · Wheel: zoom")
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
        {"capabilities", QJsonArray{"shell", "authored-document", "command-sessions","sketch-authoring"}}};
}

QString AlloyWindow::validateExtrudeInputs(const QJsonObject& inputs) const {
    auto schema=findCommand("solid.extrude")->parameters;
    schema["sketchId"]=QJsonObject{{"type","string"},{"required",false}};
    schema["profileId"]=QJsonObject{{"type","string"},{"required",false}};
    Command command=*findCommand("solid.extrude");command.parameters=schema;
    const auto error=validateArguments(command,inputs);if(!error.isEmpty())return error;
    if(inputs.value("name").toString().trimmed().isEmpty())return "Extrusion name must be nonempty";
    return {};
}
QJsonObject AlloyWindow::extrusionCandidate(const QJsonObject& inputs,bool hideSource) const {
    QJsonObject arguments{{"sketchId",inputs.value("sketchId")},{"profileId",inputs.value("profileId")},
        {"distanceMm",CadDocument::millimeters(inputs.value("distance").toDouble(),inputs.value("units").toString(document_.authored()["displayUnits"].toString()))}};
    if(inputs.contains("name"))arguments["name"]=inputs.value("name");
    if(!inputs.value("featureId").toString().isEmpty())arguments["featureId"]=inputs.value("featureId");
    else if(session_.value("command")=="solid.extrude" && session_.value("inputs").toObject().value("sketchId")==inputs.value("sketchId")
        && session_.value("inputs").toObject().value("profileId")==inputs.value("profileId")) {
        arguments["newFeatureId"]=session_.value("newFeatureId");arguments["newBodyId"]=session_.value("newBodyId");
    }
    auto candidate=CadDocument::extrusionCandidate(document_.authored(),arguments);
    // Consume only a newly created feature. Re-editing an extrusion preserves
    // a sketch the user explicitly showed after its original consumption.
    if(hideSource && inputs.value("featureId").toString().isEmpty()) {
        auto features=candidate["features"].toArray();for(int i=0;i<features.size();++i){auto feature=features[i].toObject();
            if(feature["id"]==inputs.value("sketchId")){feature["visible"]=false;features[i]=feature;break;}}
        candidate["features"]=features;
    }
    return candidate;
}
QJsonObject AlloyWindow::beginExtrude(const QJsonObject& overrides) {
    if(!session_.isEmpty())return failure("session_busy","Complete or cancel the active command first");
    const auto units=document_.authored()["displayUnits"].toString();
    QJsonObject inputs{{"distance",0},{"units",units},{"name","Extrude"}};
    for(auto i=overrides.begin();i!=overrides.end();++i)inputs[i.key()]=i.value();
    if(inputs.value("sketchId").toString().isEmpty() && !selectedSketch_.isEmpty())inputs["sketchId"]=selectedSketch_;
    if(inputs.value("profileId").toString().isEmpty() && !selectedProfile_.isEmpty() && inputs.value("sketchId")==selectedSketch_)inputs["profileId"]=selectedProfile_;
    if(inputs.value("profileId").toString().isEmpty()) {
        std::vector<EvaluatedProfile> candidates;
        for(const auto& sketch:document_.sketches())if(inputs.value("sketchId").toString().isEmpty() ? sketch.visible : sketch.featureId==inputs["sketchId"].toString())
            candidates.insert(candidates.end(),sketch.profiles.begin(),sketch.profiles.end());
        if(candidates.size()==1){inputs["sketchId"]=candidates.front().sketchId;inputs["profileId"]=candidates.front().profileId;}
    }
    const auto error=validateExtrudeInputs(inputs);if(!error.isEmpty())return failure("invalid_arguments",error);
    const auto id=inputs.value("featureId").toString();
    if(!id.isEmpty()) {
        bool found=false;for(const auto& entry:document_.authored()["features"].toArray()){const auto feature=entry.toObject();found=found || (feature["id"]==id && feature["type"]=="extrude");}
        if(!found)return failure("stale_entity","Extrusion feature does not exist","featureId");
    }
    session_={{"id",CadDocument::newId()},{"command","solid.extrude"},{"status","active"},{"inputs",inputs},
        {"source",QJsonObject{{"sketchId",inputs.value("sketchId")},{"profileId",inputs.value("profileId")}}},
        {"previewed",false},{"baseDocumentRevision",static_cast<qint64>(document_.revision())},
        {"inputSchema",findCommand("solid.extrude")->parameters}};
    if(id.isEmpty()){session_["newFeatureId"]=CadDocument::newId();session_["newBodyId"]=CadDocument::newId();}
    selectedBody_.clear();selectedSketch_=inputs.value("sketchId").toString();selectedProfile_=inputs.value("profileId").toString();
    view_->selectBody({});view_->selectProfile(selectedSketch_,selectedProfile_);showExtrude();
    const auto preview=previewExtrude();if(preview.value("ok").isBool() && !preview.value("ok").toBool())extrudeError_->setText(preview["error"].toObject()["message"].toString());
    refreshExtrudeControls();return session_;
}
QJsonObject AlloyWindow::previewExtrude() {
    session_["previewed"]=false;session_["geometryEvaluated"]=false;session_.remove("geometry");session_.remove("documentDraft");view_->clearPreview();
    if(session_.value("baseDocumentRevision").toInteger()!=static_cast<qint64>(document_.revision())) {
        view_->setEditDisplay({},{});return failure("stale_session","The document changed; cancel and restart this extrusion");
    }
    auto reject=[this](QJsonObject error){view_->setEditDisplay({},{});return error;};
    const auto inputs=session_.value("inputs").toObject();
    QJsonArray hidden;
    if(!inputs.value("featureId").toString().isEmpty())for(const auto& body:document_.bodies())if(body.featureId==inputs["featureId"].toString())hidden.append(body.bodyId);
    view_->setEditDisplay(inputs.value("sketchId").toString(),hidden);
    if(inputs.value("sketchId").toString().isEmpty() || inputs.value("profileId").toString().isEmpty())return reject(failure("profile_required","Select a closed sketch region to extrude","profileId"));
    QJsonObject candidate;try {candidate=extrusionCandidate(inputs,false);}catch(const std::exception& error){return reject(failure("evaluation_failed",QString::fromUtf8(error.what())));}
    const auto evaluated=CadDocument::evaluateCandidate(candidate);
    if(!evaluated.ok){auto result=failure("evaluation_failed",evaluated.error);auto error=result["error"].toObject();error["diagnostics"]=evaluated.diagnostics;result["error"]=error;return reject(result);}
    const auto featureId=inputs.value("featureId").toString(session_.value("newFeatureId").toString());
    const EvaluatedBody* body=nullptr;const EvaluatedProfile* profile=nullptr;
    for(const auto& item:evaluated.evaluation->bodies)if(item.featureId==featureId)body=&item;
    for(const auto& sketch:evaluated.evaluation->sketches)if(sketch.featureId==inputs["sketchId"].toString())
        for(const auto& item:sketch.profiles)if(item.profileId==inputs["profileId"].toString())profile=&item;
    if(!body || !profile)return reject(failure("evaluation_failed","The evaluated source or extrusion result is unavailable"));
    const auto distance=CadDocument::millimeters(inputs["distance"].toDouble(),inputs["units"].toString());
    view_->showExtrudePreview(*body,*profile,distance);view_->selectProfile(profile->sketchId,profile->profileId);
    session_["previewed"]=true;session_["geometryEvaluated"]=true;session_["geometry"]=body->summary;
    session_["featureId"]=body->featureId;session_["bodyId"]=body->bodyId;session_["documentDraft"]=evaluated.authored;
    return session_;
}
QJsonObject AlloyWindow::selectFinishedProfile(const QString& sketchId,const QString& profileId) {
    bool found=false;for(const auto& sketch:document_.sketches())if(sketch.featureId==sketchId)
        for(const auto& profile:sketch.profiles)found=found || profile.profileId==profileId;
    if(!found)return failure("stale_entity","Closed profile does not exist in that sketch","profileId");
    selectedBody_.clear();selectedSketch_=sketchId;selectedProfile_=profileId;view_->selectBody({});view_->selectProfile(sketchId,profileId);
    if(session_.value("command")=="solid.extrude")updateExtrude({{"sketchId",sketchId},{"profileId",profileId}});
    return {{"kind","profile"},{"sketchId",sketchId},{"profileId",profileId},{"profileIds",QJsonArray{profileId}}};
}
QJsonArray AlloyWindow::dependentBodies(const QString& sketchId) const {
    QJsonArray result;
    // The supported graph currently contains direct sketch -> extrusion edges.
    for(const auto& entry:document_.authored()["features"].toArray()) {
        const auto feature=entry.toObject();if(feature["type"]=="extrude" && feature["parameters"].toObject()["sketchId"]==sketchId)result.append(feature["bodyId"]);
    }
    return result;
}
void AlloyWindow::showExtrude() {
    if(extrudeEscape_){delete extrudeEscape_;extrudeEscape_=nullptr;}
    if(extrudeDock_)delete extrudeDock_;
    auto* dock=new CommandDock("Extrude",canvasWindow_);extrudeDock_=dock;dock->setObjectName("extrudeDock");
    dock->setAllowedAreas(Qt::LeftDockWidgetArea|Qt::RightDockWidgetArea);
    extrudePanel_=new QWidget;extrudePanel_->setObjectName("extrudePanel");extrudePanel_->setFixedWidth(286);
    extrudePanel_->setStyleSheet("#extrudePanel { background:#f7f8f9; color:#323c44; border-left:1px solid #cdd3d8; }");dock->setWidget(extrudePanel_);
    dock->onClose=[this]{if(session_.value("command")=="solid.extrude")execute("command.cancel",{{"session",session_.value("id")}});};
    auto* layout=new QVBoxLayout(extrudePanel_);layout->setContentsMargins(14,14,14,12);layout->setSpacing(12);
    auto* form=new QFormLayout;form->setVerticalSpacing(12);
    extrudeProfile_=new QComboBox;extrudeProfile_->setObjectName("extrude.profile");form->addRow("Profiles",extrudeProfile_);
    form->addRow("Start",new QLabel("Profile Plane"));form->addRow("Direction",new QLabel("One Side"));form->addRow("Extent Type",new QLabel("Distance"));
    extrudeDistance_=new QDoubleSpinBox;extrudeDistance_->setObjectName("extrude.distance");extrudeDistance_->setRange(-100000,100000);
    extrudeDistance_->setDecimals(6);extrudeDistance_->setKeyboardTracking(false);extrudeDistance_->setValue(session_.value("inputs").toObject()["distance"].toDouble());
    form->addRow("Distance",extrudeDistance_);form->addRow("Operation",new QLabel("New Body"));layout->addLayout(form);
    extrudeError_=new QLabel;extrudeError_->setWordWrap(true);extrudeError_->setStyleSheet("color:#a63832;");layout->addWidget(extrudeError_);
    auto* hint=new QLabel("Select a closed sketch region.\nA negative distance reverses the profile normal.");hint->setWordWrap(true);hint->setStyleSheet("color:#687780;");layout->addWidget(hint);
    layout->addStretch();auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);extrudeOk_=buttons->button(QDialogButtonBox::Ok);layout->addWidget(buttons);
    connect(extrudeDistance_->findChild<QLineEdit*>(),&QLineEdit::returnPressed,this,[this]{
        extrudeDistance_->interpretText();updateExtrude({{"distance",extrudeDistance_->value()}});
        if(session_.value("previewed").toBool())execute("command.commit",{{"session",session_.value("id")}});
    });
    connect(extrudeDistance_,qOverload<double>(&QDoubleSpinBox::valueChanged),this,[this](double distance){updateExtrude({{"distance",distance}});});
    connect(extrudeProfile_,qOverload<int>(&QComboBox::currentIndexChanged),this,[this](int index){
        const auto selected=extrudeProfile_->itemData(index).toJsonObject();if(!selected.isEmpty())execute("selection.profileSet",selected);
    });
    connect(buttons,&QDialogButtonBox::accepted,this,[this]{const auto response=execute("command.commit",{{"session",session_.value("id")}});if(!response["ok"].toBool())extrudeError_->setText(response["error"].toObject()["message"].toString());});
    connect(buttons,&QDialogButtonBox::rejected,this,[this]{execute("command.cancel",{{"session",session_.value("id")}});});
    auto* escape=new QShortcut(QKeySequence(Qt::Key_Escape),extrudePanel_);escape->setContext(Qt::WidgetWithChildrenShortcut);
    connect(escape,&QShortcut::activated,this,[this]{execute("command.cancel",{{"session",session_.value("id")}});});
    extrudeEscape_=new QShortcut(QKeySequence(Qt::Key_Escape),view_);extrudeEscape_->setContext(Qt::WidgetShortcut);
    connect(extrudeEscape_,&QShortcut::activated,this,[this]{if(session_.value("command")=="solid.extrude")execute("command.cancel",{{"session",session_.value("id")}});});
    canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,dock);dock->show();refreshExtrudeControls();extrudeDistance_->setFocus();extrudeDistance_->selectAll();
}
void AlloyWindow::refreshExtrudeControls() {
    if(!extrudePanel_ || session_.value("command")!="solid.extrude")return;
    const auto inputs=session_.value("inputs").toObject();
    selectedSketch_=inputs.value("sketchId").toString();selectedProfile_=inputs.value("profileId").toString();view_->selectProfile(selectedSketch_,selectedProfile_);
    {const QSignalBlocker block(extrudeProfile_);extrudeProfile_->clear();extrudeProfile_->addItem("Select profile…");
        int selected=0;for(const auto& sketch:document_.sketches())if(sketch.visible || sketch.featureId==inputs.value("sketchId").toString())for(const auto& profile:sketch.profiles) {
            extrudeProfile_->addItem(sketch.name+" · "+QString::number(profile.summary.value("areaMm2").toDouble(),'g',6)+" mm²",QVariant::fromValue(QJsonObject{{"sketchId",sketch.featureId},{"profileId",profile.profileId}}));
            if(profile.profileId==inputs.value("profileId").toString() && sketch.featureId==inputs.value("sketchId").toString())selected=extrudeProfile_->count()-1;
        }extrudeProfile_->setCurrentIndex(selected);}
    {const QSignalBlocker block(extrudeDistance_);extrudeDistance_->setValue(inputs["distance"].toDouble());extrudeDistance_->setSuffix(" "+inputs["units"].toString());}
    extrudeOk_->setEnabled(session_.value("previewed").toBool() && session_.value("baseDocumentRevision").toInteger()==static_cast<qint64>(document_.revision()));
}
void AlloyWindow::updateExtrude(const QJsonObject& inputs) {
    auto response=execute("command.update",{{"session",session_.value("id")},{"inputs",inputs}});
    if(response["ok"].toBool())response=execute("command.preview",{{"session",session_.value("id")}});
    if(extrudeError_)extrudeError_->setText(response["ok"].toBool() ? "" : response["error"].toObject()["message"].toString());
    refreshExtrudeControls();
}

QJsonObject AlloyWindow::beginSession(const QString& command, const QJsonObject& inputs) {
    if(command=="solid.extrude")return beginExtrude(inputs);
    if(command=="sketch.create") {
        const auto error=validateArguments(*findCommand("sketch.begin"),inputs);
        if(!error.isEmpty())return failure("invalid_arguments",error);
        auto result=beginSketch(inputs["plane"].toString(),inputs.value("name").toString("Sketch"));
        if(!result["ok"].isBool() && inputs.contains("units")){auto values=session_.value("inputs").toObject();values["units"]=inputs["units"];session_["inputs"]=values;result=session_;}
        return result;
    }
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
    if(commit && target=="sketch.plane")return failure("plane_required","Choose an origin plane before finishing a sketch");
    if(commit && target=="sketch.edit") {
        if(session_.value("baseDocumentRevision").toInteger()!=static_cast<qint64>(document_.revision()))return failure("stale_session","The document changed; cancel and restart this sketch");
        const auto candidate=SketchGeometry::replaceFeature(document_.authored(),session_.value("draftSketch").toObject());
        const auto error=document_.commit(candidate,session_.value("inputs").toObject().contains("sketchId") ? "Edit sketch" : "Create sketch");
        if(!error.isEmpty())return failure("evaluation_failed",error);
        result["document"]=document_.state();
    } else if (commit && target != "commands.search") {
        if (session_.value("baseDocumentRevision").toInteger() != static_cast<qint64>(document_.revision()))
            return failure("stale_session", "The document changed; cancel and restart this edit");
        auto response = execute(target, session_.value("inputs").toObject());
        if (!response["ok"].toBool()) return response;
        result["commandResult"] = response;
    }
    session_ = {};
    if(dimensionEditor_)dimensionEditor_->hide();
    if (target == "commands.search" && search_) search_->hide();
    if (target == "document.units" && unitsDialog_) unitsDialog_->hide();
    if(target=="solid.extrude"){if(extrudeEscape_)extrudeEscape_->setEnabled(false);if(extrudeDock_)extrudeDock_->hide();view_->clearPreview();view_->setEditDisplay({},{});}
    else if(target.startsWith("solid.")) {if(primitiveDock_)primitiveDock_->hide();view_->clearPreview();}
    if(target.startsWith("sketch.")) {
        if(sketchDock_)sketchDock_->hide();view_->clearSketchDraft();displayedSketchDraft_={};view_->setSketchPlane({});view_->setEditDisplay({},{});
    }
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
    if(primitiveDock_)delete primitiveDock_;
    primitiveInputs_.clear();primitivePanel_=new QWidget;
    primitivePanel_->setObjectName("primitivePanel");primitivePanel_->setFixedWidth(312);
    primitivePanel_->setStyleSheet("#primitivePanel { background: #f7f8f9; border-left: 1px solid #cdd3d8; }");
    const auto command=session_.value("command").toString();const auto inputs=session_.value("inputs").toObject();
    auto* dock=new CommandDock(command=="solid.box" ? "Box" : "Cylinder",canvasWindow_);
    primitiveDock_=dock;dock->setObjectName("primitiveDock");
    dock->setAllowedAreas(Qt::LeftDockWidgetArea|Qt::RightDockWidgetArea);dock->setWidget(primitivePanel_);
    dock->onClose=[this]{if(session_.value("command").toString().startsWith("solid."))execute("command.cancel",{{"session",session_.value("id")}});};
    primitivePanel_->setWindowTitle(command=="solid.box" ? "Box" : "Cylinder");
    auto* layout=new QVBoxLayout(primitivePanel_);auto* form=new QFormLayout;
    auto* plane=new QLabel(inputs.value("featureId").toString().isEmpty() ? "XY plane · New Body" : "XY plane · Edit Body");layout->addWidget(plane);
    primitiveName_=new QLineEdit(inputs["name"].toString());primitiveName_->setObjectName("primitive.name");form->addRow("Name",primitiveName_);
    const QStringList fields=command=="solid.box" ? QStringList{"width","depth","height","x","y","z"}
        : QStringList{"radius","height","x","y","z"};
    for(const auto& field:fields) {
        auto* input=new QDoubleSpinBox;input->setKeyboardTracking(false);input->setDecimals(6);input->setRange(field.size()==1 ? -100000 : 0.000001,100000);
        input->setSuffix(" "+inputs["units"].toString());input->setValue(inputs[field].toDouble());input->setObjectName("primitive."+field);
        primitiveInputs_[field]=input;form->addRow(field.first(1).toUpper()+field.sliced(1),input);
        connect(input,qOverload<double>(&QDoubleSpinBox::valueChanged),this,[this,field](double value){
            updatePrimitive({{field,value}});
        });
    }
    connect(primitiveName_,&QLineEdit::editingFinished,this,[this]{
        if(session_.value("command").toString().startsWith("solid."))updatePrimitive({{"name",primitiveName_->text()}});
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
    auto* escape=new QShortcut(QKeySequence(Qt::Key_Escape),primitivePanel_);
    connect(escape,&QShortcut::activated,this,[this]{execute("command.cancel",{{"session",session_.value("id")}});});
    layout->addWidget(buttons);layout->addStretch();canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,dock);
    dock->show();
}
void AlloyWindow::updatePrimitive(const QJsonObject& inputs) {
    auto response=execute("command.update",{{"session",session_.value("id")},{"inputs",inputs}});
    if(response["ok"].toBool())response=execute("command.preview",{{"session",session_.value("id")}});
    primitiveError_->setText(response["ok"].toBool() ? "" : response["error"].toObject()["message"].toString());
}

QJsonObject AlloyWindow::beginSketch(const QString& plane,const QString& name,const QString& sketchId) {
    if(!session_.isEmpty() && session_.value("command").toString()!="sketch.plane")return failure("session_busy","Complete or cancel the active command first");
    const auto seed=sketchId.isEmpty() ? SketchGeometry::newFeature(document_.authored()["documentId"].toString(),plane,name)
        : SketchGeometry::findFeature(document_.authored(),sketchId);
    const auto solved=SketchGeometry::solveDraft(seed);
    if(!solved.ok){auto response=failure("sketch_solve_failed",solved.error);auto error=response["error"].toObject();error["diagnostics"]=solved.diagnostics;response["error"]=error;return response;}
    const auto draft=solved.feature;
    auto inputs=QJsonObject{{"plane",plane},{"name",draft["name"]},{"units",session_.value("inputs").toObject().value("units").toString(document_.authored()["displayUnits"].toString())}};
    if(!sketchId.isEmpty())inputs["sketchId"]=sketchId;
    const auto sessionId=session_.value("id").toString(CadDocument::newId());
    session_={{"id",sessionId},{"command","sketch.edit"},{"status","active"},{"inputs",inputs},{"draftSketch",draft},
        {"tool","select"},{"selection",QJsonObject{{"pointIds",QJsonArray{}},{"entityIds",QJsonArray{}}}},{"solverDiagnostics",solved.diagnostics},
        {"previewed",false},{"baseDocumentRevision",static_cast<qint64>(document_.revision())}};
    selectedBody_.clear();view_->clearPreview();view_->clearSketchDraft();displayedSketchDraft_={};
    selectedSketch_.clear();selectedProfile_.clear();view_->selectProfile({},{});view_->setEditDisplay(draft["id"].toString(),dependentBodies(draft["id"].toString()));
    view_->lookAtSketch(plane);showSketch();return session_;
}

QJsonObject AlloyWindow::appendSketchEntity(const QString& tool,const QJsonObject& inputs) {
    if(session_.value("command").toString()!="sketch.edit")return failure("no_active_sketch","Enter a sketch first");
    if(inputs.contains("session") && inputs["session"]!=session_.value("id"))return failure("invalid_session","Session is no longer active");
    if(session_.value("baseDocumentRevision").toInteger()!=static_cast<qint64>(document_.revision()))return failure("stale_session","The document changed; cancel and restart this sketch");
    const auto units=inputs.value("units").toString(session_.value("inputs").toObject()["units"].toString());
    QJsonObject canonical{{"u1Mm",CadDocument::millimeters(inputs["x1"].toDouble(),units)},{"v1Mm",CadDocument::millimeters(inputs["y1"].toDouble(),units)},
        {"u2Mm",CadDocument::millimeters(inputs["x2"].toDouble(),units)},{"v2Mm",CadDocument::millimeters(inputs["y2"].toDouble(),units)}};
    const auto draft=session_.value("draftSketch").toObject();
    const auto updated=tool=="line" ? SketchGeometry::addLine(draft,canonical) : SketchGeometry::addRectangle(draft,canonical);
    return publishSketchSolve(SketchGeometry::solveDraft(updated));
}

QJsonObject AlloyWindow::publishSketchSolve(const SketchGeometry::SolveResult& solved) {
    if(session_.value("command").toString()!="sketch.edit")return failure("no_active_sketch","Enter a sketch first");
    if(session_.value("baseDocumentRevision").toInteger()!=static_cast<qint64>(document_.revision()))return failure("stale_session","The document changed; cancel and restart this sketch");
    if(!solved.ok) {
        session_["lastSolveFailure"]=solved.diagnostics;
        auto response=failure("sketch_solve_failed",solved.error);auto error=response["error"].toObject();error["diagnostics"]=solved.diagnostics;response["error"]=error;return response;
    }
    const auto candidate=SketchGeometry::replaceFeature(document_.authored(),solved.feature);const auto error=CadDocument::validate(candidate);
    if(!error.isEmpty())return failure("invalid_sketch",error);
    const auto evaluated=SketchGeometry::evaluateFeature(solved.feature);
    session_["draftSketch"]=solved.feature;session_["solverDiagnostics"]=solved.diagnostics;session_["previewed"]=false;
    session_.remove("lastSolveFailure");session_.remove("firstPoint");session_.remove("pointerPoint");
    view_->clearSketchCursor();view_->showSketchDraft(evaluated);displayedSketchDraft_=solved.feature;
    return {{"sessionId",session_.value("id")},{"draftSketch",solved.feature},{"sketchGeometry",evaluated.summary},{"diagnostics",solved.diagnostics},{"authoredCommitted",false}};
}

QJsonObject AlloyWindow::setSketchSelection(const QJsonObject& selection) {
    if(session_.value("command").toString()!="sketch.edit")return failure("no_active_sketch","Enter a sketch first");
    if(session_.value("baseDocumentRevision").toInteger()!=static_cast<qint64>(document_.revision()))return failure("stale_session","The document changed; cancel and restart this sketch");
    const auto parameters=session_.value("draftSketch").toObject()["parameters"].toObject();
    QJsonObject normalized;
    for(const auto& field:QStringList{"pointIds","entityIds","profileIds"}) {
        QSet<QString> available;
        if(field=="profileIds")for(const auto& profile:SketchGeometry::evaluateFeature(session_.value("draftSketch").toObject()).profiles)available.insert(profile.profileId);
        else for(const auto& entry:parameters[field=="pointIds" ? "points" : "entities"].toArray())available.insert(entry.toObject()["id"].toString());
        QJsonArray ids;for(const auto& value:selection[field].toArray()) {
            if(!value.isString() || !available.contains(value.toString()))return failure("stale_entity","Selected sketch geometry does not exist",field);
            if(!ids.contains(value))ids.append(value);
        }
        normalized[field]=ids;
    }
    session_["selection"]=normalized;
    cancelDimension();
    if(session_.value("tool")=="dimension")refreshSketchControls();
    auto result=normalized;result["selection"]=normalized;return result;
}

QJsonObject AlloyWindow::sketchDimensionTargets(const QJsonObject& args) const {
    const auto selection=session_.value("selection").toObject();
    auto points=args.contains("pointIds") ? args["pointIds"].toArray() : selection["pointIds"].toArray();
    const auto entities=selection["entityIds"].toArray();
    const auto entity=args.value("entityId").toString(points.isEmpty() && entities.size()==1 ? entities[0].toString() : QString{});
    if(args.contains("entityId") && entity.isEmpty())throw std::invalid_argument("The requested sketch line does not exist");
    if(!entity.isEmpty()) {
        if(args.contains("pointIds"))throw std::invalid_argument("Choose two points or one line for a dimension");
        bool found=false;points={};
        for(const auto& entry:session_.value("draftSketch").toObject()["parameters"].toObject()["entities"].toArray()) {
            const auto line=entry.toObject();if(line["id"]==entity){points=QJsonArray{line["startPointId"],line["endPointId"]};found=true;break;}
        }
        if(!found)throw std::invalid_argument("The requested sketch line does not exist");
    }
    if(points.size()!=2 || points[0]==points[1])throw std::invalid_argument("Select one line or two distinct points for a dimension");
    return {{"pointIds",points}};
}

void AlloyWindow::sketchResponse(const QJsonObject& response) {
    if(sketchError_)sketchError_->setText(response["ok"].toBool() ? "" : response["error"].toObject()["message"].toString());
    if(dimensionEditor_ && dimensionEditor_->isVisible() && !response.value("ok").toBool())dimensionEditorError_->setText(response.value("error").toObject().value("message").toString());
}

void AlloyWindow::cancelDimension() {
    session_.remove("pendingDimension");
    if(dimensionEditor_)dimensionEditor_->hide();
}
QJsonObject AlloyWindow::previewDimension(const QJsonObject& args) {
    if(session_.value("command")!="sketch.edit")return failure("no_active_sketch","Enter a sketch first");
    if(args.contains("session") && args["session"]!=session_.value("id"))return failure("invalid_session","Session is no longer active");
    if(session_.value("baseDocumentRevision").toInteger()!=static_cast<qint64>(document_.revision()))return failure("stale_session","The document changed; cancel and restart this sketch");
    if(session_.value("pendingDimension").toObject().value("phase")=="editing")return failure("dimension_input_active","Accept or cancel the current value first");
    auto pending=session_.value("pendingDimension").toObject();const auto targets=sketchDimensionTargets({});
    const auto draft=session_.value("draftSketch").toObject();const auto plane=session_.value("inputs").toObject().value("plane").toString();
    const auto placement=view_->sketchPoint(args.value("x").toDouble(),args.value("y").toDouble(),plane);
    QMap<QString,QPointF> points;
    for(const auto& entry:draft.value("parameters").toObject().value("points").toArray()){const auto p=entry.toObject();points[p.value("id").toString()]={p.value("uMm").toDouble(),p.value("vMm").toDouble()};}
    const auto ids=targets.value("pointIds").toArray();
    if(!points.contains(ids[0].toString()) || !points.contains(ids[1].toString()))return failure("stale_entity","Dimension targets do not exist");
    const auto a=points.value(ids[0].toString()),b=points.value(ids[1].toString());const double du=std::abs(b.x()-a.x()),dv=std::abs(b.y()-a.y());
    const auto orientation=args.value("kind").toString(pending.value("orientation").toString("auto"));QString kind=orientation;
    if(kind=="auto") {
        const double u=placement.value("uMm").toDouble(),v=placement.value("vMm").toDouble();
        if(dv<1e-7)kind="horizontal";else if(du<1e-7)kind="vertical";
        else if(v<std::min(a.y(),b.y()) || v>std::max(a.y(),b.y()))kind="horizontal";
        else if(u<std::min(a.x(),b.x()) || u>std::max(a.x(),b.x()))kind="vertical";else kind="linear";
    }
    const double value=kind=="horizontal" ? du : kind=="vertical" ? dv : std::hypot(du,dv);
    if(value<1e-6)return failure("invalid_dimension","This orientation has no measurable span");
    pending={{"phase","placing"},{"pointIds",ids},{"kind",kind},{"orientation",orientation},
        {"driving",args.value("driving").toBool(pending.value("driving").toBool(sketchDimensionDriving_ ? sketchDimensionDriving_->isChecked() : true))},
        {"annotation",QJsonObject{{"uMm",placement.value("uMm")},{"vMm",placement.value("vMm")}}},{"measuredValueMm",value}};
    session_["tool"]="dimension";session_["pendingDimension"]=pending;return pending;
}
QJsonObject AlloyWindow::acceptDimension(const QJsonObject& args) {
    if(session_.value("command")!="sketch.edit")return failure("no_active_sketch","Enter a sketch first");
    if(args.contains("session") && args["session"]!=session_.value("id"))return failure("invalid_session","Session is no longer active");
    const auto pending=session_.value("pendingDimension").toObject();if(pending.value("phase")!="editing")return failure("no_pending_dimension","Place or edit a dimension first");
    if(args.contains("text") && args.contains("value"))return failure("invalid_arguments","Choose text or a numeric value");
    const bool driving=pending.value("driving").toBool();double valueMm=pending.value("measuredValueMm").toDouble();
    if(driving) {
        QString units=args.value("units").toString(session_.value("inputs").toObject().value("units").toString());
        double value;
        if(args.contains("value"))value=args.value("value").toDouble();
        else {
            const QString text=args.value("text").toString(dimensionEditorValue_ ? dimensionEditorValue_->text() : QString{});
            const QRegularExpression expression(QStringLiteral(R"(^\s*([+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?)\s*(mm|in)?\s*$)"),QRegularExpression::CaseInsensitiveOption);
            const auto match=expression.match(text);if(!match.hasMatch())return failure("invalid_dimension_value","Enter a positive number with optional mm or in; expressions are not supported");
            bool ok=false;value=match.captured(1).toDouble(&ok);if(!ok)return failure("invalid_dimension_value","Dimension is outside the supported numeric range");
            if(!match.captured(2).isEmpty())units=match.captured(2).toLower();
        }
        valueMm=CadDocument::millimeters(value,units);
        if(!std::isfinite(valueMm) || valueMm<0.000001 || valueMm>100000)return failure("invalid_dimension_value","Dimension must be 0.000001..100000 mm");
    }
    const auto draft=session_.value("draftSketch").toObject();QJsonObject result;
    if(pending.contains("dimensionId")) {
        if(!driving){cancelDimension();view_->setFocus();return {{"authoredChange",false}};}
        result=publishSketchSolve(SketchGeometry::updateDimension(draft,{{"dimensionId",pending.value("dimensionId")},{"valueMm",valueMm}}));
    } else {
        QJsonObject mutation{{"pointIds",pending.value("pointIds")},{"kind",pending.value("kind")},{"driving",driving},{"annotation",pending.value("annotation")}};
        if(driving)mutation["valueMm"]=valueMm;
        result=publishSketchSolve(SketchGeometry::addDimension(draft,mutation));
    }
    if(result.value("ok").isBool() && !result.value("ok").toBool())return result;
    cancelDimension();session_["selection"]=QJsonObject{{"pointIds",QJsonArray{}},{"entityIds",QJsonArray{}}};view_->setFocus();return result;
}
QJsonObject AlloyWindow::editDimension(const QString& id) {
    if(session_.value("command")!="sketch.edit")return failure("no_active_sketch","Enter a sketch first");
    if(session_.value("baseDocumentRevision").toInteger()!=static_cast<qint64>(document_.revision()))return failure("stale_session","The document changed; cancel and restart this sketch");
    QJsonObject dimension;for(const auto& entry:session_.value("draftSketch").toObject()["parameters"].toObject()["dimensions"].toArray())if(entry.toObject()["id"]==id){dimension=entry.toObject();break;}
    if(dimension.isEmpty())return failure("stale_entity","Dimension does not exist");
    double value=dimension.value("valueMm").toDouble();for(const auto& entry:session_.value("solverDiagnostics").toObject().value("dimensionValues").toArray())if(entry.toObject().value("id")==id)value=entry.toObject().value("valueMm").toDouble();
    const auto layout=view_->dimensionLayout(session_.value("draftSketch").toObject(),dimension,value,session_.value("inputs").toObject().value("units").toString());
    if(layout.isEmpty())return failure("annotation_unavailable","Look At the sketch plane before editing this label");
    cancelDimension();dimension["dimensionId"]=id;dimension["phase"]="editing";dimension["annotation"]=layout.value("annotation");dimension["measuredValueMm"]=value;
    session_["pendingDimension"]=dimension;auto selection=session_.value("selection").toObject();selection["dimensionId"]=id;session_["selection"]=selection;showDimensionEditor();return dimension;
}
void AlloyWindow::showDimensionEditor() {
    if(!dimensionEditor_) {
        dimensionEditor_=new QFrame(view_);dimensionEditor_->setObjectName("sketchInlineEditor");dimensionEditor_->setAttribute(Qt::WA_NativeWindow);dimensionEditor_->setAutoFillBackground(true);
        dimensionEditor_->setFixedWidth(245);dimensionEditor_->setStyleSheet("#sketchInlineEditor { background: #f7f8f9; border: 1px solid #9aabb8; } QLineEdit { background: white; color: #20252a; padding: 5px; }");
        auto* layout=new QVBoxLayout(dimensionEditor_);layout->setContentsMargins(7,7,7,7);layout->setSpacing(4);
        dimensionEditorValue_=new QLineEdit;dimensionEditorValue_->setObjectName("sketch.inlineValue");layout->addWidget(dimensionEditorValue_);
        dimensionEditorError_=new QLabel;dimensionEditorError_->setWordWrap(true);dimensionEditorError_->setStyleSheet("color: #53616b; font-size: 11px; background: #f7f8f9;");layout->addWidget(dimensionEditorError_);
        connect(dimensionEditorValue_,&QLineEdit::returnPressed,this,[this]{sketchResponse(execute("sketch.dimensionAccept",{{"text",dimensionEditorValue_->text()}}));});
        auto* escape=new QShortcut(QKeySequence(Qt::Key_Escape),dimensionEditor_);escape->setContext(Qt::WidgetWithChildrenShortcut);
        connect(escape,&QShortcut::activated,this,[this]{execute("sketch.dimensionCancel");});
    }
    const auto pending=session_.value("pendingDimension").toObject();const auto units=session_.value("inputs").toObject().value("units").toString();const double scale=units=="in" ? 25.4 : 1;
    const bool driving=pending.value("driving").toBool();dimensionEditorValue_->setReadOnly(!driving);
    dimensionEditorValue_->setText(QString::number(pending.value("measuredValueMm").toDouble()/scale,'g',9)+" "+units);
    dimensionEditorError_->setText(driving ? "Enter accepts · Esc cancels" : "Driven measurement · Enter / Esc closes");
    dimensionEditor_->adjustSize();positionDimensionEditor();dimensionEditor_->show();dimensionEditor_->raise();dimensionEditorValue_->setFocus();dimensionEditorValue_->selectAll();
}
void AlloyWindow::positionDimensionEditor() {
    if(!dimensionEditor_ || session_.value("pendingDimension").toObject().value("phase")!="editing")return;
    const auto point=session_.value("pendingDimension").toObject().value("annotation").toObject();const auto plane=session_.value("inputs").toObject().value("plane").toString();
    const auto p=view_->projectSketchPoint(point.value("uMm").toDouble(),point.value("vMm").toDouble(),plane);
    dimensionEditor_->move(std::clamp(qRound(p.value("x").toDouble()+12),0,std::max(0,view_->width()-dimensionEditor_->width())),
        std::clamp(qRound(p.value("y").toDouble()+12),0,std::max(0,view_->height()-dimensionEditor_->height())));
}
QJsonObject AlloyWindow::inlineEditorState() const {
    if(!dimensionEditor_)return {{"visible",false}};
    return {{"visible",dimensionEditor_->isVisible()},{"text",dimensionEditorValue_->text()},{"readOnly",dimensionEditorValue_->isReadOnly()},
        {"bounds",QJsonObject{{"x",dimensionEditor_->x()},{"y",dimensionEditor_->y()},{"width",dimensionEditor_->width()},{"height",dimensionEditor_->height()}}}};
}

void AlloyWindow::dimensionSelectionChanged() {
    if(!sketchDimensionList_ || !sketchDimensionEdit_)return;
    const auto id=sketchDimensionList_->currentData().toString();const auto units=session_.value("inputs").toObject()["units"].toString();
    const double scale=units=="in" ? 25.4 : 1;
    const QSignalBlocker block(sketchDimensionEdit_);sketchDimensionEdit_->setSuffix(" "+units);
    for(const auto& entry:session_.value("solverDiagnostics").toObject()["dimensionValues"].toArray()) {
        const auto value=entry.toObject();if(value["id"]!=id)continue;
        sketchDimensionEdit_->setValue(value["valueMm"].toDouble()/scale);sketchDimensionEdit_->setEnabled(value["driving"].toBool());
        sketchDimensionMeasure_->setText(QString("%1: %2 %3").arg(value["driving"].toBool() ? "Driving" : "Driven measurement")
            .arg(QString::number(value["valueMm"].toDouble()/scale,'g',8)).arg(units));return;
    }
    sketchDimensionEdit_->setEnabled(false);sketchDimensionMeasure_->setText("Select a dimension to inspect or edit it");
}

void AlloyWindow::refreshSketchControls() {
    if(!sketchSelectionTree_ || session_.value("command")!="sketch.edit")return;
    const auto draft=session_.value("draftSketch").toObject(),selection=session_.value("selection").toObject();
    const auto units=session_.value("inputs").toObject()["units"].toString();
    auto presentationSelection=selection;presentationSelection["tool"]=session_.value("tool");
    view_->decorateSketch(draft,presentationSelection,session_.value("solverDiagnostics").toObject(),units,session_.value("pendingDimension").toObject());
    const bool changed=draft!=sketchWidgetDraft_,selected=selection!=sketchWidgetSelection_,unitChanged=units!=sketchWidgetUnits_;
    if(!changed && !selected && !unitChanged)return;
    const auto parameters=draft["parameters"].toObject();const double scale=units=="in" ? 25.4 : 1;
    QMap<QString,QJsonObject> points;QMap<QString,int> pointNumbers;
    QMap<QString,QStringList> relations;
    for(const auto& entry:parameters["constraints"].toArray()) {
        const auto constraint=entry.toObject();const auto type=constraint["type"].toString();
        for(const auto& id:constraint["entityIds"].toArray())relations[id.toString()].append(type.first(1).toUpper()+type.sliced(1));
        for(const auto& id:constraint["pointIds"].toArray())relations[id.toString()].append(type.first(1).toUpper()+type.sliced(1));
    }
    {
        const QSignalBlocker block(sketchSelectionTree_);sketchSelectionTree_->clear();
        auto* pointGroup=new QTreeWidgetItem(sketchSelectionTree_,{"Points"});pointGroup->setFlags(pointGroup->flags() & ~Qt::ItemIsSelectable);
        auto* lineGroup=new QTreeWidgetItem(sketchSelectionTree_,{"Lines"});lineGroup->setFlags(lineGroup->flags() & ~Qt::ItemIsSelectable);
        int index=0;
        for(const auto& entry:parameters["points"].toArray()) {
            const auto point=entry.toObject();const auto id=point["id"].toString();points[id]=point;pointNumbers[id]=++index;
            auto* item=new QTreeWidgetItem(pointGroup,{QString("P%1  (%2, %3) %4").arg(index).arg(QString::number(point["uMm"].toDouble()/scale,'g',5)).arg(QString::number(point["vMm"].toDouble()/scale,'g',5)).arg(units)});
            if(relations.contains(id))item->setText(0,item->text(0)+" · "+relations[id].join(", "));
            item->setData(0,Qt::UserRole,"point");item->setData(0,Qt::UserRole+1,id);item->setSelected(selection["pointIds"].toArray().contains(id));
        }
        index=0;
        for(const auto& entry:parameters["entities"].toArray()) {
            const auto entity=entry.toObject();const auto id=entity["id"].toString();
            auto* item=new QTreeWidgetItem(lineGroup,{QString("Line %1  ·  P%2 → P%3").arg(++index).arg(pointNumbers.value(entity["startPointId"].toString())).arg(pointNumbers.value(entity["endPointId"].toString()))});
            if(relations.contains(id))item->setText(0,item->text(0)+" · "+relations[id].join(", "));
            item->setData(0,Qt::UserRole,"entity");item->setData(0,Qt::UserRole+1,id);item->setSelected(selection["entityIds"].toArray().contains(id));
        }
        pointGroup->setExpanded(true);lineGroup->setExpanded(true);
    }
    QMap<QString,QJsonObject> values;const auto diagnostics=session_.value("solverDiagnostics").toObject();
    for(const auto& entry:diagnostics["dimensionValues"].toArray()){const auto value=entry.toObject();values[value["id"].toString()]=value;}
    if(changed || unitChanged) {
        const auto previous=sketchDimensionList_->currentData();const QSignalBlocker block(sketchDimensionList_);sketchDimensionList_->clear();
        for(const auto& entry:parameters["dimensions"].toArray()) {
            const auto dimension=entry.toObject();const auto measured=values.value(dimension["id"].toString());const auto kind=dimension["kind"].toString();
            sketchDimensionList_->addItem(QString("%1 %2 %3 · %4").arg(kind.first(1).toUpper()+kind.sliced(1))
                .arg(QString::number(measured["valueMm"].toDouble()/scale,'g',6)).arg(units).arg(dimension["driving"].toBool() ? "Driving" : "Driven"),dimension["id"].toString());
        }
        if(sketchDimensionList_->count()==0)sketchDimensionList_->addItem("No dimensions");
        const int previousIndex=sketchDimensionList_->findData(previous);if(previousIndex>=0)sketchDimensionList_->setCurrentIndex(previousIndex);
        dimensionSelectionChanged();
    }
    const QSignalBlocker block(sketchDimensionValue_);sketchDimensionValue_->setSuffix(" "+units);
    try {
        const auto target=sketchDimensionTargets({})["pointIds"].toArray();const auto a=points.value(target[0].toString()),b=points.value(target[1].toString());
        const double u=std::abs(a["uMm"].toDouble()-b["uMm"].toDouble()),v=std::abs(a["vMm"].toDouble()-b["vMm"].toDouble());
        const auto kind=sketchDimensionKind_->currentData().toString();sketchDimensionValue_->setValue((kind=="horizontal" ? u : kind=="vertical" ? v : std::hypot(u,v))/scale);
    }catch(const std::invalid_argument&){}
    sketchWidgetDraft_=draft;sketchWidgetSelection_=selection;sketchWidgetUnits_=units;
}

void AlloyWindow::showSketch() {
    if(sketchDock_)delete sketchDock_;
    sketchInputs_.clear();sketchInfo_=nullptr;sketchName_=nullptr;
    sketchSelectionTree_=nullptr;sketchDimensionKind_=nullptr;sketchDimensionValue_=nullptr;sketchDimensionDriving_=nullptr;
    sketchDimensionList_=nullptr;sketchDimensionEdit_=nullptr;sketchDimensionMeasure_=nullptr;sketchWidgetDraft_={};sketchWidgetSelection_={};sketchWidgetUnits_.clear();
    sketchPanel_=new QWidget;sketchPanel_->setObjectName("sketchPanel");sketchPanel_->setFixedWidth(312);
    sketchPanel_->setStyleSheet("#sketchPanel { background: #f7f8f9; border-left: 1px solid #cdd3d8; }");
    const bool choosing=session_.value("command").toString()=="sketch.plane";
    auto* dock=new CommandDock(choosing ? "Create Sketch" : "Sketch Palette",canvasWindow_);sketchDock_=dock;
    dock->setObjectName("sketchDock");dock->setAllowedAreas(Qt::LeftDockWidgetArea|Qt::RightDockWidgetArea);dock->setWidget(sketchPanel_);
    dock->onClose=[this]{if(session_.value("command").toString().startsWith("sketch."))execute("sketch.cancel");};
    auto* panelLayout=new QVBoxLayout(sketchPanel_);panelLayout->setContentsMargins(5,5,5,5);
    auto* content=new QWidget;content->setObjectName("sketchScrollContent");
    content->setStyleSheet("#sketchScrollContent { background: #f7f8f9; color: #323c44; }");
    auto* layout=new QVBoxLayout(content);layout->setContentsMargins(6,6,6,6);
    auto* scroll=new QScrollArea;sketchScroll_=scroll;scroll->setWidgetResizable(true);scroll->setFrameShape(QFrame::NoFrame);
    scroll->viewport()->setObjectName("sketchScrollViewport");
    scroll->viewport()->setStyleSheet("#sketchScrollViewport { background: #f7f8f9; color: #323c44; }");
    scroll->setWidget(content);panelLayout->addWidget(scroll,1);
    if(choosing) {
        auto* help=new QLabel("Choose an origin plane to begin a sketch.");help->setWordWrap(true);layout->addWidget(help);
        for(const auto& plane:QStringList{"XY","XZ","YZ"}) {
            auto* button=new QPushButton(plane+" Plane");button->setObjectName("sketch.plane."+plane);layout->addWidget(button);
            // Defer replacement of the panel until this clicked signal returns.
            connect(button,&QPushButton::clicked,this,[this,plane]{QTimer::singleShot(0,this,[this,plane]{sketchResponse(execute("sketch.begin",{{"plane",plane}}));});});
        }
    } else {
        const auto inputs=session_.value("inputs").toObject();
        auto* form=new QFormLayout;
        sketchName_=new QLineEdit(inputs["name"].toString());sketchName_->setObjectName("sketch.name");form->addRow("Name",sketchName_);
        connect(sketchName_,&QLineEdit::editingFinished,this,[this]{
            if(session_.value("command").toString()=="sketch.edit")sketchResponse(execute("command.update",{{"session",session_.value("id")},{"inputs",QJsonObject{{"name",sketchName_->text()}}}}));
        });
        layout->addLayout(form);sketchInfo_=new QLabel;sketchInfo_->setObjectName("sketch.info");sketchInfo_->setWordWrap(true);layout->addWidget(sketchInfo_);
        auto* guidance=new QLabel("Select geometry in the canvas or below.\nShift-click adds to the selection.");guidance->setWordWrap(true);layout->addWidget(guidance);
        sketchSelectionTree_=new QTreeWidget;sketchSelectionTree_->setObjectName("sketch.selection");sketchSelectionTree_->setHeaderHidden(true);
        sketchSelectionTree_->setSelectionMode(QAbstractItemView::ExtendedSelection);sketchSelectionTree_->setMaximumHeight(145);layout->addWidget(sketchSelectionTree_);
        connect(sketchSelectionTree_,&QTreeWidget::itemSelectionChanged,this,[this]{
            QJsonArray points,entities;for(auto* item:sketchSelectionTree_->selectedItems()) {
                const auto kind=item->data(0,Qt::UserRole).toString(),id=item->data(0,Qt::UserRole+1).toString();
                if(kind=="point")points.append(id);else if(kind=="entity")entities.append(id);
            }
            sketchResponse(execute("sketch.selectionSet",{{"pointIds",points},{"entityIds",entities}}));
        });
        layout->addWidget(new QLabel("CONSTRAINTS"));auto* constraints=new QGridLayout;
        const QStringList types{"coincident","horizontal","vertical","parallel","perpendicular","fixed"};
        for(int i=0;i<types.size();++i) {
            const auto type=types[i];auto* button=new QPushButton(type.first(1).toUpper()+type.sliced(1));button->setObjectName("sketch.constraint."+type);
            button->setToolTip(type=="fixed" ? "Select one point; retain its position" : type=="coincident" ? "Select two points"
                : type=="horizontal" || type=="vertical" ? "Select one line" : "Select two lines");
            constraints->addWidget(button,i/2,i%2);connect(button,&QPushButton::clicked,this,[this,type]{sketchResponse(execute("sketch.constraint",{{"type",type}}));});
        }
        layout->addLayout(constraints);layout->addWidget(new QLabel("DIMENSION  D"));auto* dimensions=new QFormLayout;
        sketchDimensionKind_=new QComboBox;sketchDimensionKind_->setObjectName("sketch.dimension.kind");
        for(const auto& kind:QStringList{"linear","horizontal","vertical"})sketchDimensionKind_->addItem(kind.first(1).toUpper()+kind.sliced(1),kind);
        dimensions->addRow("Measure",sketchDimensionKind_);
        sketchDimensionDriving_=new QCheckBox("Driving dimension");sketchDimensionDriving_->setChecked(true);sketchDimensionDriving_->setObjectName("sketch.dimension.driving");dimensions->addRow(sketchDimensionDriving_);
        sketchDimensionValue_=new QDoubleSpinBox;sketchDimensionValue_->setDecimals(6);sketchDimensionValue_->setRange(0.000001,100000);sketchDimensionValue_->setKeyboardTracking(false);
        sketchDimensionValue_->setObjectName("sketch.dimension.value");dimensions->addRow("Value",sketchDimensionValue_);layout->addLayout(dimensions);
        connect(sketchDimensionKind_,qOverload<int>(&QComboBox::currentIndexChanged),this,[this]{sketchWidgetSelection_={};refreshSketchControls();});
        connect(sketchDimensionDriving_,&QCheckBox::toggled,sketchDimensionValue_,&QWidget::setEnabled);
        auto* addDimension=new QPushButton("Add Dimension");addDimension->setObjectName("sketch.dimension.add");layout->addWidget(addDimension);
        connect(addDimension,&QPushButton::clicked,this,[this]{
            QJsonObject args{{"kind",sketchDimensionKind_->currentData().toString()},{"driving",sketchDimensionDriving_->isChecked()}};
            if(sketchDimensionDriving_->isChecked())args["value"]=sketchDimensionValue_->value();sketchResponse(execute("sketch.dimension",args));
        });
        sketchDimensionList_=new QComboBox;sketchDimensionList_->setObjectName("sketch.dimension.list");layout->addWidget(sketchDimensionList_);
        sketchDimensionMeasure_=new QLabel;sketchDimensionMeasure_->setObjectName("sketch.dimension.measure");layout->addWidget(sketchDimensionMeasure_);
        sketchDimensionEdit_=new QDoubleSpinBox;sketchDimensionEdit_->setDecimals(6);sketchDimensionEdit_->setRange(0.000001,100000);sketchDimensionEdit_->setKeyboardTracking(false);
        sketchDimensionEdit_->setObjectName("sketch.dimension.edit");layout->addWidget(sketchDimensionEdit_);
        connect(sketchDimensionList_,qOverload<int>(&QComboBox::currentIndexChanged),this,[this]{dimensionSelectionChanged();});
        auto* editDimension=new QPushButton("Apply Dimension Value");editDimension->setObjectName("sketch.dimension.apply");layout->addWidget(editDimension);
        connect(editDimension,&QPushButton::clicked,this,[this]{
            const auto id=sketchDimensionList_->currentData().toString();if(id.isEmpty())return;
            sketchResponse(execute("sketch.dimensionUpdate",{{"dimensionId",id},{"value",sketchDimensionEdit_->value()}}));
        });
        auto* manualTitle=new QPushButton("Exact-coordinate creation ▾");manualTitle->setCheckable(true);layout->addWidget(manualTitle);
        auto* manual=new QWidget;auto* manualLayout=new QVBoxLayout(manual);manualLayout->setContentsMargins(0,0,0,0);manual->hide();layout->addWidget(manual);
        connect(manualTitle,&QPushButton::toggled,manual,&QWidget::setVisible);
        auto* coordinates=new QFormLayout;
        for(const auto& field:QStringList{"x1","y1","x2","y2"}) {
            auto* input=new QDoubleSpinBox;input->setKeyboardTracking(false);input->setDecimals(6);input->setRange(-100000,100000);
            const double scale=inputs["units"]=="in" ? 25.4 : 1;
            input->setValue((field=="x2" ? 40 : field=="y2" ? 25 : 0)/scale);input->setSuffix(" "+inputs["units"].toString());input->setObjectName("sketch."+field);
            sketchInputs_[field]=input;coordinates->addRow(field=="x1" ? "Start X" : field=="y1" ? "Start Y" : field=="x2" ? "End X" : "End Y",input);
        }
        manualLayout->addLayout(coordinates);
        auto* addRow=new QHBoxLayout;
        for(const auto& tool:QStringList{"line","rectangle"}) {
            auto* button=new QPushButton(tool=="line" ? "Add Line" : "Add Rectangle");button->setObjectName("sketch.add."+tool);addRow->addWidget(button);
            connect(button,&QPushButton::clicked,this,[this,tool]{
                QJsonObject args;for(auto it=sketchInputs_.begin();it!=sketchInputs_.end();++it)args[it.key()]=it.value()->value();
                sketchResponse(execute("sketch."+tool,args));
            });
        }
        manualLayout->addLayout(addRow);
        auto* grid=new QPushButton("Show / Hide Sketch Grid");layout->addWidget(grid);
        connect(grid,&QPushButton::clicked,this,[this]{execute("view.grid",{{"visible",!view_->grid}});});
    }
    layout->addStretch();sketchError_=new QLabel;sketchError_->setObjectName("sketch.error");sketchError_->setWordWrap(true);sketchError_->setStyleSheet("color: #a63832;");panelLayout->addWidget(sketchError_);
    auto* actions=new QHBoxLayout;
    if(!choosing) {
        auto* finish=new QPushButton("Finish Sketch");finish->setObjectName("sketch.palette.finish");actions->addWidget(finish);
        connect(finish,&QPushButton::clicked,this,[this]{sketchResponse(execute("sketch.finish"));});
    }
    auto* cancel=new QPushButton("Cancel");cancel->setObjectName("sketch.cancel");actions->addWidget(cancel);
    connect(cancel,&QPushButton::clicked,this,[this]{execute("sketch.cancel");});panelLayout->addLayout(actions);
    canvasWindow_->addDockWidget(Qt::RightDockWidgetArea,dock);dock->show();
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
