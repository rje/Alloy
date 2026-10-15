// SPDX-License-Identifier: MIT
#include "AlloyTheme.h"
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QDir>
#include <QJsonDocument>
#include <QTest>
#include <QToolButton>

class NavigationCubeAccessoriesControls final : public QObject {
    Q_OBJECT
    static QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& args={}) {
        const auto response=window.executeApi(command,args);
        if(!response.value("ok").toBool())qFatal("%s",QJsonDocument(response).toJson(QJsonDocument::Compact).constData());
        return response.value("result").toObject();
    }
    static QJsonObject camera(AlloyWindow& window) {
        return window.state().value("viewport").toObject().value("navigation").toObject();
    }
private slots:
    void faceAdjacentRollHome() {
        const auto output=qEnvironmentVariable("ALLOY_CUBE_ACCESSORY_EVIDENCE");
        QVERIFY(!output.isEmpty());QVERIFY(QDir().mkpath(output));
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1360,850);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        ReferenceView* view=nullptr;for(auto* child:window.findChildren<QWidget*>())
            if(auto* found=dynamic_cast<ReferenceView*>(child))view=found;
        QVERIFY(view);
        auto* home=window.findChild<QToolButton*>("cube.home");
        auto* roll=window.findChild<QToolButton*>("cube.rollLeft");
        auto* adjacent=window.findChild<QToolButton*>("cube.adjacentRight");
        QVERIFY(home && roll && adjacent);
        QVERIFY(home->isVisible());
        QCOMPARE(QApplication::widgetAt(home->mapToGlobal(home->rect().center())),static_cast<QWidget*>(home));
        call(window,"solid.box",{{"width",46},{"depth",27},{"height",13},{"units","mm"}});
        const auto body=call(window,"document.inspect").value("evaluation").toObject().value("bodies").toArray().first().toObject();
        call(window,"view.home");call(window,"view.pan",{{"x",35},{"y",-16}});
        call(window,"view.zoom",{{"factor",1.2}});
        const auto before=call(window,"document.inspect");
        call(window,"view.cubeOrient",{{"region","top"}});
        QCOMPARE(call(window,"view.cubeFaceState").value("face"),QJsonValue("top"));
        QVERIFY(roll->isEnabled() && adjacent->isEnabled());
        QCOMPARE(home->parentWidget()->parentWidget(),static_cast<QWidget*>(view));
        const auto top=camera(window);
        call(window,"capture.screenshot",{{"path",output+"/cube-face-accessories.png"}});
        QTest::mouseClick(roll,Qt::LeftButton);
        const auto rolled=camera(window);
        QCOMPARE(rolled.value("customDirection"),top.value("customDirection"));
        QVERIFY(rolled.value("customUp")!=top.value("customUp"));
        QCOMPARE(rolled.value("orbitPivot"),top.value("orbitPivot"));
        QCOMPARE(rolled.value("panX"),top.value("panX"));QCOMPARE(rolled.value("zoom"),top.value("zoom"));
        QVERIFY(roll->isEnabled());
        QTest::mouseClick(adjacent,Qt::LeftButton);
        const auto turned=camera(window);
        QVERIFY(turned.value("customDirection")!=rolled.value("customDirection"));
        QCOMPARE(turned.value("orbitPivot"),top.value("orbitPivot"));
        QCOMPARE(turned.value("panX"),top.value("panX"));QCOMPARE(turned.value("zoom"),top.value("zoom"));
        QVERIFY(call(window,"view.cubeFaceState").value("enabled").toBool());
        call(window,"capture.screenshot",{{"path",output+"/cube-rolled-adjacent.png"}});
        view->sync();QCOMPARE(camera(window).value("customDirection"),turned.value("customDirection"));
        QTest::mouseClick(home,Qt::LeftButton);
        QCOMPARE(camera(window).value("orientation"),QJsonValue("isometric"));
        QVERIFY(!roll->isEnabled() && !adjacent->isEnabled());
        const auto homePose=camera(window);
        QTest::mouseClick(adjacent,Qt::LeftButton);
        QCOMPARE(camera(window).value("orientation"),homePose.value("orientation"));
        QCOMPARE(call(window,"document.inspect").value("authored"),before.value("authored"));
        QCOMPARE(call(window,"document.inspect").value("undo"),before.value("undo"));
        const auto projected=call(window,"view.projectBody",{{"bodyId",body.value("bodyId")}});
        const QPoint pixel(qRound(projected.value("x").toDouble()),qRound(projected.value("y").toDouble()));
        QCOMPARE(view->pickBody(pixel.x(),pixel.y()),body.value("bodyId").toString());
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel);
        const auto selected=window.state().value("selection").toObject();
        const auto selectedBody=selected.value("kind")=="face" ? selected.value("support").toObject().value("bodyId") : selected.value("bodyId");
        QCOMPARE(selectedBody,body.value("bodyId"));
        call(window,"capture.screenshot",{{"path",output+"/cube-home-selection.png"}});
        call(window,"document.save",{{"path",output+"/cube-accessories.alloy"}});window.close();
    }
};
QTEST_MAIN(NavigationCubeAccessoriesControls)
#include "navigation_cube_accessories_controls.moc"
