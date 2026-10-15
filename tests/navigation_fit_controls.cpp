// SPDX-License-Identifier: MIT
#include "AlloyTheme.h"
#include "AlloyWindow.h"
#include <QApplication>
#include <QDir>
#include <QJsonDocument>
#include <QShortcut>
#include <QTest>
#include <QToolButton>

class NavigationFitControls final : public QObject {
    Q_OBJECT
    static QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& args={}) {
        const auto response=window.executeApi(command,args);
        if(!response.value("ok").toBool())qFatal("%s",QJsonDocument(response).toJson(QJsonDocument::Compact).constData());
        return response.value("result").toObject();
    }
    static QJsonObject navigation(AlloyWindow& window) {
        return window.state().value("viewport").toObject().value("navigation").toObject();
    }
private slots:
    void fitKeepsPoseAndFitsPreview() {
        const auto output=qEnvironmentVariable("ALLOY_NAV_FIT_EVIDENCE");
        QVERIFY(!output.isEmpty());QVERIFY(QDir().mkpath(output));
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1360,850);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto* fit=window.findChild<QToolButton*>("command.view.fit");QVERIFY(fit && fit->isVisible());
        QVERIFY(fit->toolTip().contains("F6"));
        auto* shortcut=window.findChild<QShortcut*>("view.fit.shortcut");QVERIFY(shortcut);
        QCOMPARE(shortcut->key(),QKeySequence(Qt::Key_F6));
        QCOMPARE(shortcut->context(),Qt::WindowShortcut);
        call(window,"solid.box",{{"width",46},{"depth",27},{"height",13},{"units","mm"}});
        call(window,"view.home");call(window,"view.orbit",{{"deltaX",67},{"deltaY",31}});
        const auto pose=navigation(window);QCOMPARE(pose.value("orientation"),QJsonValue("custom"));
        call(window,"view.pan",{{"x",70},{"y",-28}});call(window,"view.zoom",{{"factor",1.35}});
        const auto before=call(window,"document.inspect");
        QTest::mouseClick(fit,Qt::LeftButton);
        const auto fitted=navigation(window);
        QCOMPARE(fitted.value("orientation"),QJsonValue("custom"));
        QCOMPARE(fitted.value("customDirection"),pose.value("customDirection"));
        QCOMPARE(fitted.value("customUp"),pose.value("customUp"));
        QCOMPARE(fitted.value("orbitPivot"),pose.value("orbitPivot"));
        QCOMPARE(fitted.value("zoom"),QJsonValue(1));
        QCOMPARE(fitted.value("panX"),QJsonValue(0));QCOMPARE(fitted.value("panY"),QJsonValue(0));
        QCOMPARE(call(window,"document.inspect").value("authored"),before.value("authored"));
        QCOMPARE(call(window,"document.inspect").value("undo"),before.value("undo"));
        call(window,"capture.screenshot",{{"path",output+"/fit-custom-pose.png"}});
        auto* home=window.findChild<QToolButton*>("command.view.home");QVERIFY(home);
        QTest::mouseClick(home,Qt::LeftButton);
        QCOMPARE(navigation(window).value("orientation"),QJsonValue("isometric"));

        call(window,"sketch.begin",{{"plane","XY"},{"units","mm"}});
        call(window,"sketch.rectangle",{{"x1",160},{"y1",8},{"x2",180},{"y2",30},{"units","mm"}});
        call(window,"sketch.finish");
        const auto sketches=call(window,"document.inspect").value("evaluation").toObject().value("sketches").toArray();
        const auto sketch=sketches.last().toObject();
        const auto profile=sketch.value("profiles").toArray().first().toObject();
        call(window,"view.orbit",{{"deltaX",-32},{"deltaY",18}});
        const auto previewPose=navigation(window);
        const auto session=call(window,"solid.extrudeDialog",{{"sketchId",sketch.value("sketchId")},
            {"profileId",profile.value("profileId")},{"distance",80},{"units","mm"}});
        QVERIFY(window.state().value("activeSession").toObject().value("previewed").toBool());
        const auto previewBefore=call(window,"document.inspect");
        QTest::mouseClick(fit,Qt::LeftButton);
        const auto previewFit=navigation(window);
        QCOMPARE(previewFit.value("orientation"),QJsonValue("custom"));
        QCOMPARE(previewFit.value("customDirection"),previewPose.value("customDirection"));
        QVERIFY(previewFit.value("centerZ").toDouble()>25);
        QCOMPARE(window.state().value("activeSession").toObject().value("previewed"),QJsonValue(true));
        QCOMPARE(call(window,"document.inspect").value("authored"),previewBefore.value("authored"));
        call(window,"capture.screenshot",{{"path",output+"/fit-pending-preview.png"}});
        call(window,"command.cancel",{{"session",session.value("id")}});
        call(window,"document.save",{{"path",output+"/fit-nav.alloy"}});window.close();
    }
};
QTEST_MAIN(NavigationFitControls)
#include "navigation_fit_controls.moc"
