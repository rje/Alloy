// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QJsonDocument>
#include <QMenu>
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
        view->setFocus();
        QVERIFY(view->hasFocus());
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
            auto* distance = window.findChild<QDoubleSpinBox*>("faceOffsetDistance");
            QVERIFY(distance && distance->isVisible());
            QTest::mouseClick(distance, Qt::LeftButton);
            QTest::keyClick(distance, Qt::Key_A, Qt::ControlModifier);
            QTest::keyClicks(distance, "5");
            QTest::keyClick(distance, Qt::Key_Return);
            QTRY_VERIFY(session(window).value("previewed").toBool());
            QCOMPARE(session(window).value("inputs").toObject().value("distanceMm").toDouble(), 5.0);
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
