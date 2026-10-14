// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include <QApplication>
#include <QDialogButtonBox>
#include <QDir>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QJsonDocument>
#include <QLineEdit>
#include <QPushButton>
#include <QTest>

class ExtrudeDistanceExpressionControlsTest final:public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& args={}) {
        const auto response=window.executeApi(command,args);
        if(!response.value("ok").toBool())qFatal("%s: %s",qPrintable(command),QJsonDocument(response).toJson().constData());
        return response.value("result").toObject();
    }
private slots:
    void typedDistanceFormula() {
        AlloyTheme::apply(*qApp);
        AlloyWindow window;window.resize(1300,850);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));QApplication::setActiveWindow(&window);
        QTRY_VERIFY(QApplication::activeWindow()!=nullptr);
        call(window,"document.parameterAdd",{{"name","Width"},{"value",40},{"units","mm"}});
        call(window,"sketch.begin",{{"plane","XY"},{"units","mm"}});
        call(window,"sketch.rectangle",{{"x1",0},{"y1",0},{"x2",40},{"y2",30},{"units","mm"}});
        call(window,"sketch.finish");
        call(window,"view.orientation",{{"name","isometric"}});
        call(window,"view.zoom",{{"factor",0.5}});
        const auto inspection=call(window,"document.inspect");
        const auto sketch=inspection.value("evaluation").toObject().value("sketches").toArray().first().toObject();
        const auto profile=sketch.value("profiles").toArray().first().toObject().value("profileId").toString();
        call(window,"solid.extrudeDialog",{{"sketchId",sketch.value("sketchId")},{"profileId",profile},{"distance",10},{"units","mm"}});
        auto* dock=window.findChild<QDockWidget*>("extrudeDock");QVERIFY(dock);
        auto* distance=dock->findChild<QDoubleSpinBox*>("extrude.distance");QVERIFY(distance);
        auto* editor=distance->findChild<QLineEdit*>();QVERIFY(editor);
        QTest::mouseClick(editor,Qt::LeftButton);
        auto* focused=QApplication::focusWidget();QVERIFY(focused==distance || focused==editor);
        QTest::keyClick(focused,Qt::Key_A,Qt::ControlModifier);
        QTest::keyClicks(focused,"Width*2");
        QCOMPARE(editor->text(),QString("Width*2"));
        QTest::keyClick(focused,Qt::Key_Tab);
        auto session=window.state().value("activeSession").toObject();
        QCOMPARE(session.value("inputs").toObject().value("distanceExpression").toString(),QString("Width*2"));
        QVERIFY(session.value("previewed").toBool());
        QCOMPARE(session.value("resolvedDistanceMm").toDouble(),80.0);
        const auto evidence=qEnvironmentVariable("ALLOY_EXTRUDE_FORMULA_EVIDENCE");
        if(!evidence.isEmpty())QVERIFY(QDir().mkpath(evidence));
        if(!evidence.isEmpty())call(window,"capture.screenshot",{{"path",evidence+"/formula-preview.png"}});
        auto* buttons=dock->findChild<QDialogButtonBox*>();QVERIFY(buttons);
        auto* ok=buttons->button(QDialogButtonBox::Ok);QVERIFY(ok && ok->isEnabled());
        QTest::mouseClick(ok,Qt::LeftButton);
        const auto committed=call(window,"document.inspect");
        const auto feature=committed.value("authored").toObject().value("features").toArray().last().toObject();
        QCOMPARE(feature.value("parameters").toObject().value("distanceExpression").toString(),QString("Width*2"));
        QCOMPARE(committed.value("evaluation").toObject().value("bodies").toArray().first().toObject().value("volumeMm3").toDouble(),96000.0);
        call(window,"feature.editDialog",{{"featureId",feature.value("id")}});
        dock=window.findChild<QDockWidget*>("extrudeDock");QVERIFY(dock);
        distance=dock->findChild<QDoubleSpinBox*>("extrude.distance");QVERIFY(distance);
        QCOMPARE(distance->findChild<QLineEdit*>()->text(),QString("Width*2"));
        if(!evidence.isEmpty())call(window,"capture.screenshot",{{"path",evidence+"/formula-edit.png"}});
        editor=distance->findChild<QLineEdit*>();QTest::mouseClick(editor,Qt::LeftButton);
        focused=QApplication::focusWidget();QVERIFY(focused==distance || focused==editor);
        QTest::keyClick(focused,Qt::Key_A,Qt::ControlModifier);
        QTest::keyClicks(focused,"Width+5 mm");
        QCOMPARE(editor->text(),QString("Width+5 mm"));
        QTest::keyClick(focused,Qt::Key_Tab);
        session=window.state().value("activeSession").toObject();
        QCOMPARE(session.value("inputs").toObject().value("distanceExpression").toString(),QString("Width+5 mm"));
        QVERIFY(session.value("previewed").toBool());
        QCOMPARE(session.value("resolvedDistanceMm").toDouble(),45.0);
        buttons=dock->findChild<QDialogButtonBox*>();QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel),Qt::LeftButton);
        QCOMPARE(call(window,"document.inspect").value("authored"),committed.value("authored"));
        call(window,"document.new",{{"discard",true}});window.close();
    }
};
QTEST_MAIN(ExtrudeDistanceExpressionControlsTest)
#include "extrude_distance_expression_controls.moc"
