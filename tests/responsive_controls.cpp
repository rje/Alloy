// SPDX-License-Identifier: MIT
#include "AlloyTheme.h"
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QDir>
#include <QJsonDocument>
#include <QMenu>
#include <QTest>
#include <QTimer>
#include <QToolButton>

class ResponsiveControls final : public QObject {
    Q_OBJECT
    static QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& args={}) {
        const auto response=window.executeApi(command,args);
        if(!response.value("ok").toBool())qFatal("%s",QJsonDocument(response).toJson(QJsonDocument::Compact).constData());
        return response.value("result").toObject();
    }
    static void choose(QToolButton* opener,const QString& actionName) {
        QVERIFY(opener && opener->isVisible());
        QCOMPARE(QApplication::widgetAt(opener->mapToGlobal(opener->rect().center())),static_cast<QWidget*>(opener));
        auto* menu=opener->menu();QVERIFY(menu);
        bool active=false,hit=false,clicked=false,watchdogClosed=false;
        QTimer::singleShot(50,opener,[&]{
            active=QApplication::activePopupWidget()==menu;
            QAction* action=nullptr;
            for(auto* candidate:menu->actions())if(candidate->objectName()==actionName)action=candidate;
            if(!active || !action || !action->isEnabled())return;
            const auto pixel=menu->actionGeometry(action).center();
            hit=QApplication::widgetAt(menu->mapToGlobal(pixel))==menu;
            if(hit){QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,pixel);clicked=true;}
        });
        QTimer watchdog;watchdog.setSingleShot(true);
        QObject::connect(&watchdog,&QTimer::timeout,opener,[&]{if(menu->isVisible()){watchdogClosed=true;menu->close();}});
        watchdog.start(2000);
        QTest::mouseClick(opener,Qt::LeftButton);
        watchdog.stop();
        QVERIFY(active);QVERIFY(hit);QVERIFY(clicked);
        QVERIFY(!watchdogClosed);
        QTRY_VERIFY(QApplication::activePopupWidget()==nullptr);
    }
private slots:
    void compactMenusAndNavigation() {
        const auto out=qEnvironmentVariable("ALLOY_RESPONSIVE_EVIDENCE");QVERIFY(!out.isEmpty());QVERIFY(QDir().mkpath(out));
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1920,900);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto* ribbon=window.findChild<QWidget*>("modelingRibbon");QVERIFY(ribbon);
        auto* nav=window.findChild<QToolButton*>("view.compact.navigation");QVERIFY(nav);
        auto* viewMenu=window.findChild<QToolButton*>("view.compact.view");QVERIFY(viewMenu);
        QTRY_VERIFY(window.findChild<QWidget*>("solid.icons.create")->isVisible());
        call(window,"capture.screenshot",{{"path",out+"/wide-idle.png"}});
        window.resize(1360,850);
        call(window,"capture.screenshot",{{"path",out+"/normal-idle.png"}});
        window.resize(1040,640);
        QTRY_VERIFY(!window.findChild<QToolButton*>("command.primitive.box")->isVisible());
        QVERIFY(window.findChild<QToolButton*>("command.sketch.create")->isVisible());
        QTRY_VERIFY(nav->isVisible() && viewMenu->isVisible());
        QCOMPARE(ribbon->height(),94);
        call(window,"capture.screenshot",{{"path",out+"/minimum-idle.png"}});
        ReferenceView* canvas=nullptr;for(auto* child:window.findChildren<QWidget*>())
            if(auto* candidate=dynamic_cast<ReferenceView*>(child))canvas=candidate;
        QVERIFY(canvas);
        choose(nav,"view.compact.pan");
        QCOMPARE(window.state().value("viewport").toObject().value("navigation").toObject().value("navigationTool"),QJsonValue("pan"));
        call(window,"capture.screenshot",{{"path",out+"/minimum-pan-active.png"}});
        const auto before=window.state().value("viewport").toObject().value("navigation").toObject();
        const QPoint start(canvas->width()/2,canvas->height()/2);
        QCOMPARE(QApplication::widgetAt(canvas->mapToGlobal(start)),static_cast<QWidget*>(canvas));
        QTest::mousePress(canvas,Qt::LeftButton,Qt::NoModifier,start);
        QTest::mouseMove(canvas,start+QPoint(31,14));
        QTest::mouseRelease(canvas,Qt::LeftButton,Qt::NoModifier,start+QPoint(31,14));
        QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(canvas));
        const auto after=window.state().value("viewport").toObject().value("navigation").toObject();
        QVERIFY(after.value("panX")!=before.value("panX") || after.value("panY")!=before.value("panY"));
        QTest::keyClick(canvas,Qt::Key_Escape);
        QCOMPARE(window.state().value("viewport").toObject().value("navigation").toObject().value("navigationTool"),QJsonValue("select"));
        choose(window.findChild<QToolButton*>("solid.menu.create"),"solid.action.primitive.box");
        QCOMPARE(window.state().value("activeSession").toObject().value("command"),QJsonValue("solid.box"));
        call(window,"capture.screenshot",{{"path",out+"/minimum-box-dock.png"}});
        call(window,"command.cancel",{{"session",window.state().value("activeSession").toObject().value("id")}});
        window.close();
    }
};
QTEST_MAIN(ResponsiveControls)
#include "responsive_controls.moc"
