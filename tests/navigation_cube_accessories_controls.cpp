// SPDX-License-Identifier: MIT
#include "AlloyTheme.h"
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QLabel>
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
        auto* toggle=window.findChild<QToolButton*>("cube.accessories.toggle");
        QVERIFY(home && roll && adjacent && toggle);
        QVERIFY(!home->isVisible());QTest::mouseClick(toggle,Qt::LeftButton);
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
    void accessoryDisclosureSmallSketch() {
        const auto output=qEnvironmentVariable("ALLOY_CUBE_ACCESSORY_DISCLOSURE_EVIDENCE");
        QVERIFY(!output.isEmpty());QVERIFY(QDir().mkpath(output));
        const auto source=QFileInfo(QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("../examples/dimensioned-rectangle.alloy")).canonicalFilePath();
        QVERIFY(!source.isEmpty());AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1360,850);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));window.activateWindow();QVERIFY(QTest::qWaitForWindowActive(&window));
        call(window,"document.open",{{"path",source},{"discard",true}});
        const auto before=call(window,"document.inspect");
        const auto features=before.value("authored").toObject().value("features").toArray();QCOMPARE(features.size(),1);
        const auto sketchId=features.first().toObject().value("id").toString();QVERIFY(!sketchId.isEmpty());
        call(window,"sketch.edit",{{"sketchId",sketchId}});call(window,"sketch.lookAt");
        const auto activeSketch=window.state().value("activeSession").toObject();
        QCOMPARE(activeSketch.value("command"),QJsonValue("sketch.edit"));
        ReferenceView* view=nullptr;for(auto* child:window.findChildren<QWidget*>())
            if(auto* found=dynamic_cast<ReferenceView*>(child))view=found;
        QVERIFY(view);
        auto* toggle=window.findChild<QToolButton*>("cube.accessories.toggle");
        auto* accessories=window.findChild<QWidget*>("cube.accessories");
        auto* home=window.findChild<QToolButton*>("cube.home");
        auto* up=window.findChild<QLabel*>("cube.upAxis");
        QVERIFY(toggle && accessories && home && up);QVERIFY(toggle->isVisible());
        auto capture=[&](const QString& name){return call(window,"capture.screenshot",{{"path",output+"/"+name+".png"}});};
        auto movePointer=[&](const QPoint& local)->QWidget* {
            const QPoint global=view->mapToGlobal(local);auto* receiver=QApplication::widgetAt(global);
            if(!receiver)return nullptr;
            QTest::mouseMove(receiver,receiver->mapFromGlobal(global));QApplication::processEvents();return receiver;
        };
        QTRY_VERIFY(!accessories->isVisible());QVERIFY(!up->isVisible());capture("collapsed-1360x850");
        window.resize(1040,640);QTRY_COMPARE(window.size(),QSize(1040,640));
        QTRY_VERIFY(!accessories->isVisible());QVERIFY(!up->isVisible());capture("collapsed-1040x640");
        const QPoint formerGrid(view->width()-142,80);
        QCOMPARE(movePointer(formerGrid),static_cast<QWidget*>(view));QTRY_VERIFY(!accessories->isVisible());
        QCOMPARE(movePointer(QPoint(view->width()-40,40)),static_cast<QWidget*>(view));QTRY_VERIFY(accessories->isVisible());
        QCOMPARE(movePointer(QPoint(view->width()-92,60)),static_cast<QWidget*>(view));QTRY_VERIFY(accessories->isVisible());
        const QPoint homePoint=home->mapTo(view,home->rect().center());
        QCOMPARE(movePointer(homePoint),static_cast<QWidget*>(home));QTRY_VERIFY(accessories->isVisible());
        QCOMPARE(movePointer(QPoint(30,view->height()-30)),static_cast<QWidget*>(view));QTRY_VERIFY(!accessories->isVisible());
        QCOMPARE(movePointer(formerGrid),static_cast<QWidget*>(view));QTRY_VERIFY(!accessories->isVisible());
        const auto toggleCenter=toggle->mapToGlobal(toggle->rect().center());
        QCOMPARE(QApplication::widgetAt(toggleCenter),static_cast<QWidget*>(toggle));
        QTest::mouseClick(toggle,Qt::LeftButton);QTRY_VERIFY(accessories->isVisible());
        const auto homeCenter=home->mapToGlobal(home->rect().center());
        QCOMPARE(QApplication::widgetAt(homeCenter),static_cast<QWidget*>(home));
        const auto cameraBefore=camera(window);QTest::mouseClick(home,Qt::LeftButton);
        QVERIFY(camera(window).value("orientation")!=cameraBefore.value("orientation"));
        auto* focused=QApplication::focusWidget();QCOMPARE(focused,static_cast<QWidget*>(home));
        QTest::keyClick(focused,Qt::Key_Tab,Qt::ShiftModifier);focused=QApplication::focusWidget();
        QCOMPARE(focused,static_cast<QWidget*>(toggle));QTest::keyClick(toggle,Qt::Key_Space);
        QTRY_VERIFY(accessories->isVisible());
        focused=QApplication::focusWidget();QVERIFY(accessories->isAncestorOf(focused));
        QTest::keyClick(focused,Qt::Key_Escape);QTRY_VERIFY(!accessories->isVisible());
        QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(toggle));
        const auto after=call(window,"document.inspect");
        QCOMPARE(after.value("authored"),before.value("authored"));
        QCOMPARE(after.value("undo"),before.value("undo"));
        QCOMPARE(window.state().value("activeSession").toObject().value("id"),activeSketch.value("id"));
        QCOMPARE(window.state().value("activeSession").toObject().value("command"),QJsonValue("sketch.edit"));
        call(window,"command.cancel",{{"session",window.state().value("activeSession").toObject().value("id")}});
        const auto afterCancel=call(window,"document.inspect");
        QCOMPARE(afterCancel.value("authored"),before.value("authored"));
        QCOMPARE(afterCancel.value("undo"),before.value("undo"));window.close();
    }
};
QTEST_MAIN(NavigationCubeAccessoriesControls)
#include "navigation_cube_accessories_controls.moc"
