// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QJsonDocument>
#include <QLineF>
#include <QMenu>
#include <QMouseEvent>
#include <QPushButton>
#include <QTest>
#include <QTimer>
#include <QToolButton>

class FaceOffsetControlsTest final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window, const QString& command, const QJsonObject& args = {}) {
        const auto response = window.execute(command, args);
        if (!response.value("ok").toBool()) qFatal("%s", QJsonDocument(response).toJson().constData());
        return response.value("result").toObject();
    }
    QJsonObject inspect(AlloyWindow& window) { return call(window, "document.inspect"); }
    QJsonObject session(AlloyWindow& window) { return window.state().value("activeSession").toObject(); }
private slots:
    void curvedOffsetRimFeedsNativeFilletSelection() {
        AlloyTheme::apply(*qApp);
        AlloyWindow window;window.resize(1280,860);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        ReferenceView* view=nullptr;
        for(auto* widget:window.findChildren<QWidget*>())
            if(auto* candidate=dynamic_cast<ReferenceView*>(widget))view=candidate;
        QVERIFY(view);
        call(window,"sketch.begin",{{"plane","XY"}});
        call(window,"sketch.addCircle",{{"centerX",0},{"centerY",0},{"diameter",20},{"units","mm"}});
        call(window,"sketch.finish");
        const auto sketch=inspect(window).value("evaluation").toObject().value("sketches").toArray().first().toObject();
        call(window,"solid.extrude",{{"sketchId",sketch.value("sketchId")},
            {"profileId",sketch.value("profiles").toArray().first().toObject().value("profileId")},
            {"distance",10},{"units","mm"}});
        const auto body=inspect(window).value("evaluation").toObject().value("bodies").toArray().first().toObject();
        QJsonObject top;QString rim;
        for(const auto& item:body.value("supportFaces").toArray())if(item.toObject().value("role")=="extrude.endCap")top=item.toObject();
        for(const auto& item:body.value("solidEdges").toArray())if(item.toObject().value("provenance").toObject().value("role")=="endRim")rim=item.toObject().value("edgeId").toString();
        QVERIFY(!top.isEmpty()&&!rim.isEmpty());
        call(window,"solid.faceOffset",{{"targetBodyId",body.value("bodyId")},
            {"face",QJsonObject{{"bodyId",top.value("bodyId")},{"producerFeatureId",top.value("producerFeatureId")},{"faceId",top.value("faceId")}}},
            {"distanceMm",5}});
        const auto before=inspect(window).value("authored").toObject();
        call(window,"view.orientation",{{"name","isometric"}});call(window,"view.fit");
        QApplication::setActiveWindow(&window);view->setFocus();QTRY_VERIFY(view->hasFocus());
        call(window,"solid.filletDialog",{{"targetBodyId",body.value("bodyId")}});
        const auto pixel=call(window,"view.projectEdge",{{"bodyId",body.value("bodyId")},{"edgeId",rim}});
        const QPoint point(qRound(pixel.value("x").toDouble()),qRound(pixel.value("y").toDouble()));
        QCOMPARE(QApplication::widgetAt(view->mapToGlobal(point)),static_cast<QWidget*>(view));
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,point);
        const auto selected=session(window).value("inputs").toObject().value("edgeIds").toArray();
        QVERIFY(selected.contains(rim));
        auto* radius=window.findChild<QDoubleSpinBox*>("filletRadius");
        QVERIFY(radius&&radius->isVisible());
        QTest::mouseClick(radius,Qt::LeftButton);
        QTest::keyClick(radius,Qt::Key_A,Qt::ControlModifier);
        QTest::keyClicks(radius,"1");
        QTest::keyClick(radius,Qt::Key_Return);
        QTRY_VERIFY(session(window).value("previewed").toBool());
        QCOMPARE(inspect(window).value("authored").toObject(),before);
        const auto evidence=qEnvironmentVariable("ALLOY_FACE_OFFSET_EVIDENCE");
        if(!evidence.isEmpty())call(window,"capture.screenshot",{{"path",evidence+"/native-circular-rim-fillet.png"}});
        auto* dock=window.findChild<QDockWidget*>("filletDock");
        QVERIFY(dock);auto* buttons=dock->findChild<QDialogButtonBox*>();
        QVERIFY(buttons&&buttons->button(QDialogButtonBox::Cancel));
        QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel),Qt::LeftButton);
        QVERIFY(session(window).isEmpty());
        QCOMPARE(inspect(window).value("authored").toObject(),before);
        call(window,"document.new",{{"discard",true}});
        window.close();
    }

    void offsetEdgeFeedsNativeFilletSelection() {
        AlloyTheme::apply(*qApp);
        AlloyWindow window;
        window.resize(1280, 860);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        ReferenceView* view = nullptr;
        for (auto* widget : window.findChildren<QWidget*>())
            if (auto* candidate = dynamic_cast<ReferenceView*>(widget)) view = candidate;
        QVERIFY(view);
        call(window, "solid.box", {{"width", 40}, {"depth", 30}, {"height", 20}, {"units", "mm"}});
        const auto source = inspect(window).value("evaluation").toObject().value("bodies").toArray().first().toObject();
        QJsonObject top;
        for (const auto& item : source.value("supportFaces").toArray())
            if (item.toObject().value("role") == "box.z1") top = item.toObject();
        QVERIFY(!top.isEmpty());
        const QJsonObject support{{"producerFeatureId", top.value("producerFeatureId")},
            {"bodyId", top.value("bodyId")}, {"faceId", top.value("faceId")}};
        call(window, "solid.faceOffset", {{"targetBodyId", source.value("bodyId")},
            {"face", support}, {"distanceMm", 5}});
        const auto offset = inspect(window);
        const auto body = offset.value("evaluation").toObject().value("bodies").toArray().first().toObject();
        QString edgeId;
        for (const auto& item : body.value("solidEdges").toArray()) {
            const auto edge = item.toObject();
            if (edge.value("provenance").toObject().value("boxEdge") == "x*y0z1" && edge.value("available").toBool())
                edgeId = edge.value("edgeId").toString();
        }
        QVERIFY(!edgeId.isEmpty());
        call(window, "view.orientation", {{"name", "isometric"}});
        call(window, "view.fit");
        QApplication::setActiveWindow(&window);
        view->setFocus();
        QTRY_VERIFY(view->hasFocus());
        call(window, "solid.filletDialog", {{"targetBodyId", body.value("bodyId")}});
        QTRY_VERIFY(view->hasFocus());
        const auto pixel = call(window, "view.projectEdge", {{"bodyId", body.value("bodyId")}, {"edgeId", edgeId}});
        const QPoint hit(qRound(pixel.value("x").toDouble()), qRound(pixel.value("y").toDouble()));
        QCOMPARE(QApplication::widgetAt(view->mapToGlobal(hit)), static_cast<QWidget*>(view));
        QTest::mouseClick(view, Qt::LeftButton, Qt::NoModifier, hit);
        QTRY_VERIFY(session(window).value("inputs").toObject().value("edgeIds").toArray().contains(edgeId));
        auto* radius = window.findChild<QDoubleSpinBox*>("filletRadius");
        QVERIFY(radius && radius->isVisible());
        QTest::mouseClick(radius, Qt::LeftButton);
        QTest::keyClick(radius, Qt::Key_A, Qt::ControlModifier);
        QTest::keyClicks(radius, "1");
        QTest::keyClick(radius, Qt::Key_Return);
        QTRY_VERIFY(session(window).value("previewed").toBool());
        QCOMPARE(inspect(window).value("authored").toObject(), offset.value("authored").toObject());
        const auto evidence = qEnvironmentVariable("ALLOY_FACE_OFFSET_EVIDENCE");
        if (!evidence.isEmpty()) call(window, "capture.screenshot", {{"path", evidence + "/native-offset-edge-fillet.png"}});
        auto* dock = window.findChild<QDockWidget*>("filletDock");
        QVERIFY(dock);
        auto* buttons = dock->findChild<QDialogButtonBox*>();
        QVERIFY(buttons && buttons->button(QDialogButtonBox::Cancel));
        QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel), Qt::LeftButton);
        QVERIFY(session(window).isEmpty());
        QCOMPARE(inspect(window).value("authored").toObject(), offset.value("authored").toObject());
        call(window, "document.new", {{"discard", true}});
        window.close();
    }

    void canvasHandleDragKeepsSessionAndCancels() {
        AlloyTheme::apply(*qApp);
        AlloyWindow window;
        window.resize(1280, 860);
        window.show();
        window.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        ReferenceView* view = nullptr;
        for (auto* widget : window.findChildren<QWidget*>())
            if (auto* candidate = dynamic_cast<ReferenceView*>(widget)) view = candidate;
        QVERIFY(view);
        call(window, "solid.box", {{"width", 40}, {"depth", 30}, {"height", 20}, {"units", "mm"}});
        const auto before = inspect(window);
        const auto body = before.value("evaluation").toObject().value("bodies").toArray().first().toObject();
        QJsonObject top;
        for (const auto& entry : body.value("supportFaces").toArray())
            if (entry.toObject().value("role") == "box.z1") top = entry.toObject();
        QVERIFY(!top.isEmpty());
        const QJsonObject face{{"producerFeatureId", top.value("producerFeatureId")},
            {"bodyId", top.value("bodyId")}, {"faceId", top.value("faceId")}};
        call(window, "view.orientation", {{"name", "isometric"}});
        call(window, "view.fit");
        // API setup enters the same native command; all gesture input below is
        // delivered to the actual widget hit, with no intervening focus repair.
        QApplication::setActiveWindow(&window);
        view->setFocus();
        QTRY_VERIFY(view->hasFocus());
        call(window, "solid.faceOffsetDialog", {{"face", face}});
        QTRY_VERIFY(view->hasFocus());
        QTRY_VERIFY(view->faceOffsetCueState().value("dragAvailable").toBool());
        const auto initial = session(window);
        const auto cue = view->faceOffsetCueState();
        QCOMPARE(initial.value("inputs").toObject().value("distanceMm").toDouble(), 0.0);
        QVERIFY(cue.value("visible").toBool());
        QCOMPARE(cue.value("sourceFace").toObject(), face);
        auto* distance = window.findChild<QDoubleSpinBox*>("faceOffsetDistance");
        QVERIFY(distance && distance->isVisible());
        QCOMPARE(distance->value(), 0.0);
        auto point = [](const QJsonObject& value) { return QPointF(value.value("x").toDouble(), value.value("y").toDouble()); };
        const auto start = point(cue.value("startScreen").toObject());
        const auto end = point(cue.value("endScreen").toObject());
        QVERIFY(QLineF(start, end).length() >= 25.0);
        const auto axis = point(cue.value("axisPixelsPerMm").toObject());
        QVERIFY(QPointF::dotProduct(axis, axis) > 0.01);
        const QPoint press = (start + (end - start) * 0.7).toPoint();
        const QPoint positive = (QPointF(press) + axis * 5.0).toPoint();
        const QPoint negative = (QPointF(press) - axis * 3.0).toPoint();
        for (const auto& p : {press, positive, negative}) {
            QVERIFY(view->rect().contains(p));
            QCOMPARE(QApplication::widgetAt(view->mapToGlobal(p)), static_cast<QWidget*>(view));
        }
        const auto evidence = qEnvironmentVariable("ALLOY_FACE_OFFSET_EVIDENCE");
        if (!evidence.isEmpty()) call(window, "capture.screenshot", {{"path", evidence + "/native-handle-zero.png"}});
        QTest::mousePress(view, Qt::LeftButton, Qt::NoModifier, press);
        QVERIFY(view->hasFocus());
        QVERIFY(view->faceOffsetCueState().value("dragActive").toBool());
        auto move = [&](const QPoint& target) {
            QMouseEvent event(QEvent::MouseMove, QPointF(target), QPointF(view->mapToGlobal(target)),
                Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
            QApplication::sendEvent(view, &event);
        };
        move(positive);
        QTRY_VERIFY(session(window).value("previewed").toBool());
        const auto positiveSession = session(window);
        const auto positiveMm = positiveSession.value("inputs").toObject().value("distanceMm").toDouble();
        QVERIFY(positiveMm > 4.0 && positiveMm < 6.0);
        QCOMPARE(positiveSession.value("id"), initial.value("id"));
        QVERIFY(qAbs(distance->value() - positiveMm) < 0.0001);
        QVERIFY(view->faceOffsetCueState().value("dragActive").toBool());
        move(press);
        QTRY_VERIFY(!session(window).value("previewed").toBool());
        QCOMPARE(session(window).value("inputs").toObject().value("distanceMm").toDouble(), 0.0);
        QCOMPARE(view->faceOffsetCueState().value("sourceFace").toObject(), face);
        // Continue the same held gesture through zero, rather than starting a
        // second drag on a refreshed cue or updating Distance through the API.
        move(negative);
        QTRY_VERIFY(session(window).value("inputs").toObject().value("distanceMm").toDouble() < -2.0);
        QTest::mouseRelease(view, Qt::LeftButton, Qt::NoModifier, negative);
        const auto final = session(window);
        const auto negativeMm = final.value("inputs").toObject().value("distanceMm").toDouble();
        QVERIFY(negativeMm > -4.0 && negativeMm < -2.0);
        QCOMPARE(final.value("id"), initial.value("id"));
        QCOMPARE(final.value("newFeatureId"), initial.value("newFeatureId"));
        QCOMPARE(final.value("inputs").toObject().value("face").toObject(), face);
        QVERIFY(final.value("previewed").toBool());
        QVERIFY(final.value("geometryEvaluated").toBool());
        QCOMPARE(final.value("lastArrowDrag").toObject().value("source").toString(), QString("canvas"));
        QCOMPARE(final.value("lastArrowDrag").toObject().value("phase").toString(), QString("end"));
        QVERIFY(!view->faceOffsetCueState().value("dragActive").toBool());
        QVERIFY(qAbs(distance->value() - negativeMm) < 0.0001);
        QVERIFY(qAbs(final.value("geometry").toObject().value("boundsMm").toObject()
            .value("size").toArray()[2].toDouble() - (20.0 + negativeMm)) < 0.001);
        QCOMPARE(inspect(window).value("authored").toObject(), before.value("authored").toObject());
        if (!evidence.isEmpty()) call(window, "capture.screenshot", {{"path", evidence + "/native-handle-negative.png"}});
        const auto refused = window.execute("solid.faceOffsetDrag", {{"distanceMm", -25}});
        QVERIFY(!refused.value("ok").toBool());
        QVERIFY(!session(window).value("previewed").toBool());
        QCOMPARE(view->faceOffsetCueState().value("sourceFace").toObject(), face);
        QCOMPARE(inspect(window).value("authored").toObject(), before.value("authored").toObject());
        auto* buttons = window.findChild<QDialogButtonBox*>("faceOffsetButtons");
        QVERIFY(buttons && buttons->button(QDialogButtonBox::Cancel));
        auto* cancel = buttons->button(QDialogButtonBox::Cancel);
        QCOMPARE(QApplication::widgetAt(cancel->mapToGlobal(cancel->rect().center())), static_cast<QWidget*>(cancel));
        QTest::mouseClick(cancel, Qt::LeftButton);
        QVERIFY(session(window).isEmpty());
        QVERIFY(view->faceOffsetCueState().isEmpty());
        QCOMPARE(inspect(window).value("authored").toObject(), before.value("authored").toObject());
        call(window, "document.new", {{"discard", true}});
        window.close();
    }

    void commandFirstPickPreviewCancelAndCommit() {
        AlloyTheme::apply(*qApp);
        AlloyWindow window;
        window.resize(1280, 860);
        window.show();
        window.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        ReferenceView* view = nullptr;
        for (auto* widget : window.findChildren<QWidget*>())
            if (auto* candidate = dynamic_cast<ReferenceView*>(widget)) view = candidate;
        QVERIFY(view);
        call(window, "solid.box", {{"width", 40}, {"depth", 30}, {"height", 20}, {"units", "mm"}});
        const auto before = inspect(window);
        const auto body = before.value("evaluation").toObject().value("bodies").toArray().first().toObject();
        QJsonObject top;
        for (const auto& entry : body.value("supportFaces").toArray())
            if (entry.toObject().value("role") == "box.z1") top = entry.toObject();
        QVERIFY(!top.isEmpty());
        const QJsonObject face{{"producerFeatureId", top.value("producerFeatureId")}, {"bodyId", top.value("bodyId")}, {"faceId", top.value("faceId")}};
        call(window, "view.orientation", {{"name", "top"}});
        call(window, "view.fit");
        const auto projected = call(window, "view.projectFacePoint", {{"support", face}, {"uMm", 20}, {"vMm", 15}});
        const QPoint hit(qRound(projected.value("x").toDouble()), qRound(projected.value("y").toDouble()));
        // Harness activation precedes the demonstrated command-first workflow.
        window.activateWindow();
        QApplication::setActiveWindow(&window);
        view->setFocus();
        QTRY_VERIFY(view->hasFocus());
        auto* modify = window.findChild<QToolButton*>("solid.menu.modify");
        QVERIFY(modify && modify->isVisible());
        auto pickAndEnter = [&] {
            auto* menu = modify->menu();
            QVERIFY(menu);
            QAction* action = nullptr;
            for (auto* candidate : menu->actions())
                if (candidate->objectName() == "solid.action.solid.faceOffsetDialog") action = candidate;
            QVERIFY(action);
            QTimer::singleShot(0, menu, [menu, action] {
                if (!QTest::qWaitFor([menu] { return QApplication::activePopupWidget() == menu; }, 1000))
                    qFatal("Modify popup never became active");
                QTest::mouseClick(menu, Qt::LeftButton, Qt::NoModifier, menu->actionGeometry(action).center());
            });
            QTest::mouseClick(modify, Qt::LeftButton);
            QTRY_VERIFY(QApplication::activePopupWidget() == nullptr);
            QCOMPARE(session(window).value("command").toString(), QString("solid.faceOffset"));
            QVERIFY(view->hasFocus());
            QCOMPARE(QApplication::widgetAt(view->mapToGlobal(hit)), static_cast<QWidget*>(view));
            QTest::mouseClick(view, Qt::LeftButton, Qt::NoModifier, hit);
            QTRY_COMPARE(session(window).value("inputs").toObject().value("face").toObject(), face);
            QVERIFY(view->faceOffsetCueState().value("visible").toBool());
            QVERIFY(!view->faceOffsetCueState().value("dragAvailable").toBool());
            QCOMPARE(view->faceOffsetCueState().value("unavailableReason").toString(), QString("normal_aligned_with_view"));
            auto* distance = window.findChild<QDoubleSpinBox*>("faceOffsetDistance");
            QVERIFY(distance && distance->isVisible());
            QTest::mouseClick(distance, Qt::LeftButton);
            QTest::keyClick(distance, Qt::Key_A, Qt::ControlModifier);
            QTest::keyClicks(distance, "5");
            QTest::keyClick(distance, Qt::Key_Return);
            QTRY_VERIFY(session(window).value("previewed").toBool());
            QCOMPARE(session(window).value("inputs").toObject().value("distanceMm").toDouble(), 5.0);
            QCOMPARE(view->faceOffsetCueState().value("distanceMm").toDouble(), 5.0);
            QCOMPARE(inspect(window).value("authored").toObject(), before.value("authored").toObject());
        };
        pickAndEnter();
        const auto evidence = qEnvironmentVariable("ALLOY_FACE_OFFSET_EVIDENCE");
        if (!evidence.isEmpty()) call(window, "capture.screenshot", {{"path", evidence + "/native-preview.png"}});
        auto* buttons = window.findChild<QDialogButtonBox*>("faceOffsetButtons");
        QVERIFY(buttons && buttons->button(QDialogButtonBox::Cancel));
        QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel), Qt::LeftButton);
        QVERIFY(session(window).isEmpty());
        QCOMPARE(inspect(window).value("authored").toObject(), before.value("authored").toObject());
        pickAndEnter();
        buttons = window.findChild<QDialogButtonBox*>("faceOffsetButtons");
        QVERIFY(buttons && buttons->button(QDialogButtonBox::Ok)->isEnabled());
        if (!evidence.isEmpty()) {
            call(window, "view.orientation", {{"name", "isometric"}});
            call(window, "capture.screenshot", {{"path", evidence + "/native-oblique-preview.png"}});
        }
        QTest::mouseClick(buttons->button(QDialogButtonBox::Ok), Qt::LeftButton);
        QVERIFY(session(window).isEmpty());
        const auto after = inspect(window);
        const auto features = after.value("authored").toObject().value("features").toArray();
        QCOMPARE(features.last().toObject().value("type").toString(), QString("faceOffset"));
        const auto resultBody = after.value("evaluation").toObject().value("bodies").toArray().first().toObject();
        QCOMPARE(resultBody.value("boundsMm").toObject().value("size").toArray()[2].toDouble(), 25.0);
        if (!evidence.isEmpty()) {
            call(window, "view.fit");
            call(window, "capture.screenshot", {{"path", evidence + "/native-committed.png"}});
        }
        const auto featureId = features.last().toObject().value("id").toString();
        call(window, "feature.editDialog", {{"featureId", featureId}});
        QTRY_COMPARE(session(window).value("inputs").toObject().value("featureId").toString(), featureId);
        auto* distance = window.findChild<QDoubleSpinBox*>("faceOffsetDistance");
        QVERIFY(distance && distance->isVisible());
        QTest::mouseClick(distance, Qt::LeftButton);
        QTest::keyClick(distance, Qt::Key_A, Qt::ControlModifier);
        QTest::keyClicks(distance, "-2");
        QTest::keyClick(distance, Qt::Key_Return);
        QTRY_VERIFY(session(window).value("previewed").toBool());
        QCOMPARE(session(window).value("inputs").toObject().value("distanceMm").toDouble(), -2.0);
        QCOMPARE(view->faceOffsetCueState().value("distanceMm").toDouble(), -2.0);
        buttons = window.findChild<QDialogButtonBox*>("faceOffsetButtons");
        QTest::mouseClick(buttons->button(QDialogButtonBox::Ok), Qt::LeftButton);
        const auto edited = inspect(window);
        QCOMPARE(edited.value("evaluation").toObject().value("bodies").toArray().first().toObject()
            .value("boundsMm").toObject().value("size").toArray()[2].toDouble(), 18.0);
        call(window, "document.new", {{"discard", true}});
        window.close();
    }
};

QTEST_MAIN(FaceOffsetControlsTest)
#include "face_offset_controls.moc"
