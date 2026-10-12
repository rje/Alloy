#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QCryptographicHash>
#include <QDialog>
#include <QDialogButtonBox>
#include <QComboBox>
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
    auto* unavailable = fileMenu->addAction("New / Open / Save — document layer pending"); unavailable->setEnabled(false);
    fileMenu->addAction("Document Settings…", this, [this] { execute("document.settings"); });
    fileMenu->addSeparator();
    fileMenu->addAction("Quit", this, [this] { execute("app.quit"); });
    file->setMenu(fileMenu); top->addWidget(file);
    top->addWidget(commandButton("document.save", "Save"));
    top->addWidget(commandButton("history.undo", "↶"));
    top->addWidget(commandButton("history.redo", "↷"));
    top->addStretch();
    auto* title = new QLabel("◇   Untitled"); title->setMinimumWidth(280); title->setAlignment(Qt::AlignCenter);
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
    group("CREATE", {{"sketch.create", "▧"}, {"solid.extrude", "▣"}, {"solid.revolve", "◒"}});
    group("MODIFY", {{"solid.fillet", "◩"}, {"solid.shell", "▱"}, {"solid.move", "✥"}});
    group("CONSTRUCT", {{"construction.plane", "▰"}});
    group("INSPECT", {{"inspect.measure", "↔"}});
    groups->addStretch();
    auto* stage = new QLabel("FOUNDATION\nModeling tools pending");
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
    document->setExpanded(true); views->setExpanded(true);
    connect(tree_, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem* item, int) {
        const auto name = item->data(0, Qt::UserRole).toString();
        if (!name.isEmpty()) execute("view.orientation", {{"name", name}});
    });
    browserLayout->addWidget(tree_, 1);
    auto* browserNote = new QLabel("Empty authored document\nModeling features will appear here\nwhen implemented.");
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
    auto* timelineLayout = new QHBoxLayout(timeline_); timelineLayout->setContentsMargins(14, 6, 14, 6);
    auto* historyControls = new QLabel("|◀    ◀    ▶    ▶|"); historyControls->setStyleSheet("color: #a0a8ad; border: 0;");
    timelineLayout->addWidget(historyControls); timelineLayout->addSpacing(24);
    auto* historyNote = new QLabel("TIMELINE    No modeling features"); historyNote->setStyleSheet("font-size: 11px; color: #8a959d; border: 0;");
    timelineLayout->addWidget(historyNote); timelineLayout->addStretch(); layout->addWidget(timeline_);
    status_ = new QLabel; statusBar()->addWidget(status_, 1);
    auto* shell = new QLabel("Alloy 0.1 · UI shell"); statusBar()->addPermanentWidget(shell);

    auto* viewMenu = menuBar()->addMenu("View");
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
    auto sessionTarget = parameter("string", "Implemented session target; modeling targets follow alongside their features.");
    sessionTarget["enum"] = QJsonArray{"document.units", "commands.search"};
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
        session_["inputs"] = inputs; session_["previewed"] = false; return session_;
    });
    add("command.preview", "Inspect command preview", "Inspect disposable authored draft; no evaluated geometry is implied.", sessionId, [this](const auto& a) {
        if (session_.isEmpty() || session_.value("id") != a["session"]) return failure("invalid_session", "Session is no longer active");
        if (session_.value("command").toString() != "commands.search"
            && session_.value("baseDocumentRevision").toInteger() != static_cast<qint64>(document_.revision()))
            return failure("stale_session", "The document changed; cancel and restart this edit");
        session_["previewed"] = true;
        auto result = session_;
        if (session_.value("command").toString() == "document.units") {
            auto draft = document_.authored(); draft["displayUnits"] = session_.value("inputs").toObject()["units"];
            result["authoredDraft"] = draft;
        }
        result["geometryEvaluated"] = false; return result;
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
    add("view.home", "Home view", "Restore isometric orientation, zoom 1, and zero pan.", {}, [this](const auto&) { view_->orientation = "isometric"; view_->zoom = 1; view_->pan = {}; return QJsonObject{}; });
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
        auto* dialog = session_.value("command").toString() == "commands.search" ? search_ : unitsDialog_;
        if (session_.isEmpty() || !dialog || !dialog->isVisible()) return failure("no_active_dialog", "No active dialog to capture");
        const auto pixels = dialog->grab(); QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly) || !pixels.save(&file, "PNG") || !file.commit()) return failure("capture_failed", "Could not save active dialog");
        return QJsonObject{{"path", path}, {"source", "active QWidget dialog"}, {"sessionId", session_.value("id")},
            {"pixelWidth", pixels.width()}, {"pixelHeight", pixels.height()}};
    });
    add("app.quit", "Quit Alloy", "Complete the request, then exit on the next event-loop turn.", {}, [](const auto&) { QTimer::singleShot(0, qApp, &QApplication::quit); return QJsonObject{{"quitting", true}}; });
    for (const auto& pair : QList<QPair<QString, QString>>{{"sketch.create", "Create Sketch"}, {"solid.extrude", "Extrude"}, {"solid.revolve", "Revolve"}, {"solid.fillet", "Fillet"}, {"solid.shell", "Shell"}, {"solid.move", "Move"}, {"construction.plane", "Plane"}, {"inspect.measure", "Measure"}, {"document.save", "Save"}, {"history.undo", "Undo"}, {"history.redo", "Redo"}}) {
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
    auto result = command->run(args);
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
        if (value.isUndefined() || (type == "boolean" && !value.isBool()) || (type == "string" && !value.isString()) || (type == "number" && !value.isDouble()) || (type == "object" && !value.isObject())) return "Argument " + it.key() + " must be " + type;
        if (spec.contains("enum") && !spec.value("enum").toArray().contains(value)) return "Unsupported value for " + it.key();
        if (type == "number" && (!std::isfinite(value.toDouble()) || value.toDouble() < spec.value("minimum").toDouble() || value.toDouble() > spec.value("maximum").toDouble())) return "Argument out of range: " + it.key();
    }
    return {};
}

QJsonObject AlloyWindow::state() const {
    // Report the current native drawable, not a previous offscreen capture FBO.
    view_->sync();
    return {{"application", "Alloy"}, {"version", "0.1.0"}, {"stage", "document-foundation"},
        {"runtime", QJsonObject{{"qtVersion", qVersion()}, {"occtBuildVersion", ALLOY_OCCT_VERSION}}},
        {"revision", static_cast<qint64>(revision_)},
        {"identity", identity()}, {"activeSession", session_},
        {"document", document_.state()},
        {"kernel", QJsonObject{{"candidate", "Open CASCADE"}, {"discoveredVersion", ALLOY_OCCT_VERSION}, {"integrated", false}, {"visualizationIntegrated", true}}},
        {"ui", QJsonObject{{"workspace", "design"}, {"tab", "solid"}, {"browserVisible", browserVisible_}, {"timelineVisible", timelineVisible_}, {"navigationVisible", navigationVisible_}, {"commandSearchVisible", search_ && search_->isVisible()}, {"windowWidth", width()}, {"windowHeight", height()}, {"devicePixelRatio", devicePixelRatioF()},
            {"activeDialog", QJsonObject{{"command", session_.value("command")},
                {"visible", session_.value("command").toString() == "commands.search" ? bool(search_ && search_->isVisible()) : bool(unitsDialog_ && unitsDialog_->isVisible())},
                {"inputs", session_.isEmpty() ? QJsonObject{} : (session_.value("command").toString() == "commands.search"
                    ? QJsonObject{{"query", searchInput_ ? searchInput_->text() : QString{}}}
                    : QJsonObject{{"units", unitsInput_ ? unitsInput_->currentText() : QString{}}})}}}}},
        {"viewport", QJsonObject{{"renderer", "OCCT OpenGL"}, {"gpu", view_->diagnostics()}, {"orientation", view_->orientation}, {"gridVisible", view_->grid}, {"originVisible", view_->origin}, {"zoom", view_->zoom}, {"pan", QJsonObject{{"x", view_->pan.x()}, {"y", view_->pan.y()}}}, {"width", view_->width()}, {"height", view_->height()}}}};
}

void AlloyWindow::refresh() {
    if (!browser_) return;
    browser_->setVisible(browserVisible_); timeline_->setVisible(timelineVisible_); navigation_->setVisible(navigationVisible_);
    tree_->topLevelItem(0)->child(0)->child(0)->setText(0, "Units: " + document_.authored()["displayUnits"].toString());
    setWindowTitle(QString("Untitled%1 — Alloy").arg(document_.history().isClean() ? "" : " *"));
    if (session_.value("command").toString() == "document.units" && unitsInput_) {
        const QSignalBlocker block(unitsInput_);
        unitsInput_->setCurrentText(session_.value("inputs").toObject()["units"].toString());
    }
    if (session_.value("command").toString() == "commands.search" && searchInput_) {
        searchInput_->setText(session_.value("inputs").toObject()["query"].toString());
    }
    view_->sync();
    const auto gpu = view_->diagnostics();
    status_->setText(QString("  %3 · No model geometry · %1 · Zoom %2%    |    Middle mouse: pan · Wheel: zoom · S: commands")
        .arg(view_->orientation).arg(qRound(view_->zoom * 100)).arg(gpu.value("hardwareAccelerated").toBool() ? "OCCT OpenGL" : "GPU UNQUALIFIED: " + gpu.value("qualification").toString()));
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
    if (command == "commands.search") showSearch(); else showUnits();
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
