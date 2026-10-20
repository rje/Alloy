// SPDX-License-Identifier: MIT
#include "AlloyTheme.h"
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QContextMenuEvent>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QMenu>
#include <QMouseEvent>
#include <QProcess>
#include <QTest>
#include <QWindow>
#include <cmath>
#include <stdexcept>
#ifdef Q_OS_MACOS
#include <dlfcn.h>
#endif

// A1 is an independently authored setup checkpoint. Every A2 pick uses the
// actual native widget hit. API opens A1 and frames its camera, then only
// inspects/captures A2 and checks edits, history and persistence.
class BracketMidpointControls final : public QObject {
  Q_OBJECT
  AlloyWindow *w = nullptr;
  ReferenceView *view = nullptr;
  QString output;
  QJsonArray evidence;
  QJsonObject native() {
    QJsonObject r;
#ifdef Q_OS_MACOS
    auto selector = reinterpret_cast<void *(*)(const char *)>(
        dlsym(RTLD_DEFAULT, "sel_registerName"));
    auto send = reinterpret_cast<void *(*)(void *, void *)>(
        dlsym(RTLD_DEFAULT, "objc_msgSend"));
    auto boolean = reinterpret_cast<bool (*)(void *, void *)>(
        dlsym(RTLD_DEFAULT, "objc_msgSend"));
    auto cls = reinterpret_cast<void *(*)(const char *)>(
        dlsym(RTLD_DEFAULT, "objc_getClass"));
    auto app = send(cls("NSApplication"), selector("sharedApplication"));
    auto window =
        send(reinterpret_cast<void *>(w->winId()), selector("window"));
    r.insert("applicationActive", boolean(app, selector("isActive")));
    r.insert("keyWindowEditor", send(app, selector("keyWindow")) == window);
#endif
    return r;
  }
  void ready() {
    check(QApplication::focusWidget() == view,
          "natural viewport focus before user input");
    const auto n = native();
    check(n.value("applicationActive").toBool(true) &&
              n.value("keyWindowEditor").toBool(true),
          "native editor activation before user input");
  }
  void check(bool ok, const QString &why) {
    if (!ok)
      throw std::runtime_error(why.toStdString());
  }
  QJsonObject call(const QString &c, const QJsonObject &a = {}) {
    auto r = w->executeApi(c, a);
    check(r.value("ok").toBool(), c + ": " + QJsonDocument(r).toJson());
    if (r.value("completion") == "pending") {
      auto initial = r.value("result").toObject();
      check(QTest::qWaitFor(
                [&] {
                  r = w->executeApi(
                      "command.status",
                      {{"session", initial.value("id")},
                       {"generation", initial.value("previewGeneration")}});
                  return r.value("completion") != "pending";
                },
                30000),
            "asynchronous completion");
      check(r.value("ok").toBool(), c + ": " + QJsonDocument(r).toJson());
    }
    return r.value("result").toObject();
  }
  QJsonObject session() { return w->state().value("activeSession").toObject(); }
  QJsonObject draft() { return session().value("draftSketch").toObject(); }
  QJsonObject params() { return draft().value("parameters").toObject(); }
  QJsonObject gpu() {
    return w->state().value("viewport").toObject().value("gpu").toObject();
  }
  QJsonObject entity(int i) {
    return params().value("entities").toArray().at(i).toObject();
  }
  QPointF point(const QJsonValue &id) {
    for (auto v : params().value("points").toArray()) {
      auto p = v.toObject();
      if (p.value("id") == id)
        return {p.value("uMm").toDouble(), p.value("vMm").toDouble()};
    }
    throw std::runtime_error("point absent");
  }
  QPair<QPointF, QPointF> line(int i) {
    auto e = entity(i);
    return {point(e.value("startPointId")), point(e.value("endPointId"))};
  }
  QPointF center(int i) {
    auto l = line(i);
    return (l.first + l.second) / 2;
  }
  QPoint pixel(QPointF p) {
    auto r = call(
        "view.projectSketchPoint",
        {{"sketchId", draft().value("id")}, {"uMm", p.x()}, {"vMm", p.y()}});
    return {qRound(r.value("x").toDouble()), qRound(r.value("y").toDouble())};
  }
  void hit(QPoint p, Qt::MouseButton b = Qt::LeftButton,
           Qt::KeyboardModifiers m = Qt::NoModifier) {
    ready();
    auto global = view->mapToGlobal(p);
    check(QApplication::widgetAt(global) == view,
          "viewport receives actual hit");
    QTest::mouseClick(view, b, m, p);
    QTest::qWait(80);
  }
  void move(QPoint p) {
    ready();
    check(QApplication::widgetAt(view->mapToGlobal(p)) == view,
          "viewport receives hover");
    QMouseEvent e(QEvent::MouseMove, QPointF(p), QPointF(view->mapToGlobal(p)),
                  Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(view, &e);
    QTest::qWait(80);
  }
  void key(Qt::Key k) {
    ready();
    auto *f = QApplication::focusWidget();
    check(f == view, "viewport owns natural keyboard focus");
    QTest::keyClick(f, k);
    QTest::qWait(80);
  }
  void capture(QString name) {
    call("capture.screenshot", {{"path", output + "/" + name + ".png"}});
    evidence.append(QJsonObject{{"stage", name},
                                {"state", w->state()},
                                {"draft", draft()},
                                {"native", native()}});
  }
  void near(double a, double b) {
    check(std::abs(a - b) < 1e-5, QString("geometry mismatch %1 != %2")
                                      .arg(a, 0, 'g', 16)
                                      .arg(b, 0, 'g', 16));
  }
  void relations(double radius) {
    auto p = params();
    check(p.value("entities").toArray().size() == 8,
          "six source edges and two circles");
    for (int j = 0; j < 2; ++j) {
      int end = j ? 2 : 5, tangent = j ? 1 : 0;
      auto c = entity(6 + j);
      auto actual = point(c.value("centerPointId"));
      auto expected = center(end);
      near(actual.x(), expected.x());
      near(actual.y(), expected.y());
      near(c.value("radiusMm").toDouble(), radius);
      bool mid = false, tan = false;
      for (auto v : p.value("constraints").toArray()) {
        auto r = v.toObject();
        auto ids = r.value("entityIds").toArray();
        if (r.value("type") == "midpoint" &&
            ids.contains(entity(end).value("id")) &&
            r.value("pointIds").toArray().contains(c.value("centerPointId")))
          mid = true;
        if (r.value("type") == "tangent" && ids.contains(c.value("id")) &&
            ids.contains(entity(tangent).value("id")))
          tan = true;
      }
      check(mid && tan, "retained midpoint and tangent use actual stable IDs");
      auto l = line(tangent);
      auto d = l.second - l.first;
      near(std::abs(d.x() * (actual.y() - l.first.y()) -
                    d.y() * (actual.x() - l.first.x())) /
               std::hypot(d.x(), d.y()),
           radius);
    }
  }
private slots:
  void activity2() {
    output = qEnvironmentVariable("ALLOY_BRACKET_MIDPOINT_EVIDENCE");
    if (output.isEmpty())
      output = QDir::tempPath() + "/alloy-bracket-midpoint";
    QDir().mkpath(output);
    AlloyTheme::apply(*qApp);
    AlloyWindow window;
    w = &window;
    window.resize(1500, 950);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    for (auto *child : window.findChildren<QWidget *>())
      if (auto *v = dynamic_cast<ReferenceView *>(child))
        view = v;
    QVERIFY(view);
#ifdef Q_OS_MACOS
    QProcess activate;
    activate.start(
        "osascript",
        {"-e", QString("tell application \"System Events\" to set frontmost of "
                       "first application process whose unix id is %1 to true")
                   .arg(QCoreApplication::applicationPid())});
    QVERIFY(activate.waitForFinished(5000));
    QCOMPARE(activate.exitCode(), 0);
#endif
    window.raise();
    window.activateWindow();
    window.windowHandle()->requestActivate();
    const bool strictNative =
        qEnvironmentVariableIntValue("ALLOY_BRACKET_MIDPOINT_STRICT_NATIVE");
    // The default compiled Qt gate permits one initial bookkeeping activation.
    // Strict native mode omits it; neither mode repairs focus during user
    // steps.
    if (!strictNative)
      QApplication::setActiveWindow(&window);
    try {
      call("document.open",
           {{"path", QString(ALLOY_TEST_SOURCE_DIR) +
                         "/tests/workflows/fixtures/bracket-activity1.alloy"},
            {"discard", true}});
      QTest::qWait(200);
      const auto sketch = call("document.inspect")
                              .value("authored")
                              .toObject()
                              .value("features")
                              .toArray()
                              .first()
                              .toObject()
                              .value("id")
                              .toString();
      call("feature.editDialog", {{"featureId", sketch}});
      QTest::qWait(200); // A1 setup; native contract starts in the open sketch.
      check(session().value("command") == "sketch.edit",
            "A1 setup opens sketch");
      call("sketch.lookAt");
      call("view.fit");
      call("view.zoom", {{"factor", 0.75}});
      check(gpu().value("hardwareAccelerated").toBool(), "hardware GPU");
      const auto initialDraft = draft();
      auto upper = entity(2).value("id"), lower = entity(5).value("id");
      // Click away from midpoint constraint badges; preserve the exact
      // documented order.
      auto pick = [&](int i) {
        auto l = line(i);
        return pixel(l.first * 0.35 + l.second * 0.65);
      };
      hit(pick(2));
      check(session()
                    .value("selection")
                    .toObject()
                    .value("entityIds")
                    .toArray() == QJsonArray{upper},
            "upper edge selected");
      hit(pick(5), Qt::LeftButton, Qt::ShiftModifier);
      auto selected =
          session().value("selection").toObject().value("entityIds").toArray();
      check(selected.size() == 2 && selected.contains(upper) &&
                selected.contains(lower),
            "Shift adds lower edge");
      auto p = pick(5);
      hit(p, Qt::RightButton);
      QContextMenuEvent context(QContextMenuEvent::Mouse, p,
                                view->mapToGlobal(p));
      QApplication::sendEvent(view, &context);
      QTest::qWait(100);
      auto *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
      check(menu, "actual sketch popup open");
      QAction *action = nullptr;
      for (auto *a : menu->actions())
        if (a->objectName() == "sketch.action.constructionToggle")
          action = a;
      check(action, "Normal/Construction action present");
      auto actions =
          gpu().value("lastContextMenu").toObject().value("actions").toArray();
      bool diagnosed = false;
      for (auto row : actions)
        diagnosed |= row.toObject().value("id") == action->objectName();
      check(diagnosed, "API diagnostics expose construction action");
      capture("01-construction-menu");
      auto local = menu->actionGeometry(action).center();
      check(QApplication::widgetAt(menu->mapToGlobal(local)) == menu,
            "menu receives actual action hit");
      QTest::mouseClick(menu, Qt::LeftButton, Qt::NoModifier, local);
      check(QTest::qWaitFor(
                [] { return QApplication::activePopupWidget() == nullptr; },
                2000),
            "menu closes");
      QTest::qWait(500);
      check(entity(2).value("id") == upper && entity(5).value("id") == lower,
            "conversion stable IDs");
      check(entity(2).value("construction").toBool() &&
                entity(5).value("construction").toBool(),
            "both ends construction");
      auto preview =
          call("command.preview", {{"session", session().value("id")}});
      check(preview.value("sketchGeometry")
                .toObject()
                .value("profiles")
                .toArray()
                .isEmpty(),
            "construction caps excluded from profiles");
      capture("02-construction");
      key(Qt::Key_C);
      check(session().value("tool") == "circle",
            "C activates Center Diameter Circle");
      for (int j = 0; j < 2; ++j) {
        int end = j ? 2 : 5, tangent = j ? 1 : 0;
        auto c = center(end);
        move(pixel(c));
        auto cue = gpu().value("sketchSnapCue").toObject();
        check(cue.value("marker") == "midpoint-triangle" &&
                  cue.value("target").toObject().value("entityId") ==
                      entity(end).value("id"),
              "actual midpoint triangle");
        capture(QString("0%1-midpoint").arg(3 + j * 2));
        hit(pixel(c));
        auto l = line(tangent);
        auto d = l.second - l.first;
        double t =
            QPointF::dotProduct(c - l.first, d) / QPointF::dotProduct(d, d);
        auto contact = l.first + t * d;
        move(pixel(contact));
        cue = gpu().value("sketchSnapCue").toObject();
        check(cue.value("marker") == "tangent-square" &&
                  cue.value("target").toObject().value("entityId") ==
                      entity(tangent).value("id"),
              "actual tangent square");
        capture(QString("0%1-tangent").arg(4 + j * 2));
        hit(pixel(contact));
      }
      key(Qt::Key_Escape);
      check(session().value("tool") == "select", "Escape exits Circle");
      relations(50);
      capture("07-two-retained-circles");
      const auto completed = draft();
      call("history.undo");
      check(params().value("entities").toArray().size() == 7,
            "Undo removes only upper circle");
      call("history.redo");
      check(draft() == completed, "Redo exact circle relations");
      call("history.undo");
      call("history.undo");
      call("history.undo");
      check(draft() == initialDraft,
            "Undo both circles and conversion exactly");
      call("history.redo");
      check(entity(2).value("construction").toBool() &&
                entity(5).value("construction").toBool(),
            "Redo restores conversion");
      call("history.redo");
      call("history.redo");
      check(draft() == completed, "Redo restores both circles exactly");
      const auto cap =
          params().value("dimensions").toArray().last().toObject().value("id");
      call("sketch.dimensionEdit", {{"dimensionId", cap}});
      call("sketch.dimensionAccept", {{"value", 120}});
      relations(60);
      const auto edited = draft();
      capture("08-cap120");
      call("history.undo");
      check(draft() == completed, "Undo source edit exact");
      call("history.redo");
      check(draft() == edited, "Redo source edit exact");
      call("sketch.finish");
      const auto authored = call("document.inspect").value("authored");
      call("document.save", {{"path", output + "/bracket-midpoint.alloy"}});
      call("document.new", {{"discard", false}});
      call("document.open", {{"path", output + "/bracket-midpoint.alloy"}});
      check(call("document.inspect").value("authored") == authored,
            "save reopen exact authored relations");
      QFile receipt(output + "/receipt.json");
      check(receipt.open(QIODevice::WriteOnly), "receipt opens");
      receipt.write(
          QJsonDocument(
              QJsonObject{{"status", "passed"},
                          {"route",
                           "compiled real Alloy Qt actual-hit pointer/key; "
                           "initial Qt harness activation; no interstep focus "
                           "or selection repair"},
                          {"physicalOSInput", false},
                          {"strictNative", strictNative},
                          {"checkpoints", evidence},
                          {"reopened", call("document.inspect")}})
              .toJson());
      call("document.new", {{"discard", true}});
      window.close();
    } catch (const std::exception &e) {
      QFile receipt(output + "/failure.json");
      if (receipt.open(QIODevice::WriteOnly))
        receipt.write(QJsonDocument(QJsonObject{{"error", e.what()},
                                                {"native", native()},
                                                {"state", w->state()},
                                                {"checkpoints", evidence}})
                          .toJson());
      w->executeApi("capture.screenshot", {{"path", output + "/failure.png"}});
      w->executeApi("document.new", {{"discard", true}});
      window.close();
      QFAIL(e.what());
    }
  }
};
QTEST_MAIN(BracketMidpointControls)
#include "bracket_midpoint_controls.moc"
