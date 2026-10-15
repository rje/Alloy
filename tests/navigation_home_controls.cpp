// SPDX-License-Identifier: MIT
#include "AlloyTheme.h"
#include "AlloyWindow.h"
#include <QApplication>
#include <QContextMenuEvent>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QMenu>
#include <QTest>
#include <QTimer>
#include <QToolButton>
#include <QTreeWidget>
#include <cmath>

class NavigationHomeControls final : public QObject {
    Q_OBJECT
    static QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& args={}) {
        const auto response=window.executeApi(command,args);
        if(!response.value("ok").toBool())qFatal("%s",QJsonDocument(response).toJson(QJsonDocument::Compact).constData());
        return response.value("result").toObject();
    }
    static QJsonObject camera(AlloyWindow& window) {
        return window.state().value("viewport").toObject().value("navigation").toObject();
    }
    static bool contextAction(AlloyWindow& window,QTreeWidget* tree,QTreeWidgetItem* item,const QString& name) {
        tree->scrollToItem(item);const auto pixel=tree->visualItemRect(item).center();
        if(!tree->viewport()->rect().contains(pixel))return false;
        bool owned=false;
        QTimer::singleShot(50,&window,[&]{
            auto* popup=qobject_cast<QMenu*>(QApplication::activePopupWidget());if(!popup)return;
            QAction* action=nullptr;for(auto* row:popup->actions())if(row->objectName()==name)action=row;
            if(!action || !action->isEnabled())return;
            const auto hit=popup->actionGeometry(action).center();
            owned=QApplication::widgetAt(popup->mapToGlobal(hit))==popup;
            if(owned)QTest::mouseClick(popup,Qt::LeftButton,Qt::NoModifier,hit);
        });
        QTimer::singleShot(2000,&window,[]{if(auto* popup=QApplication::activePopupWidget())popup->close();});
        QContextMenuEvent event(QContextMenuEvent::Mouse,pixel,tree->viewport()->mapToGlobal(pixel));
        QApplication::sendEvent(tree->viewport(),&event);
        QCoreApplication::processEvents();
        return owned && QApplication::activePopupWidget()==nullptr;
    }
private slots:
    void savedAndDefaultHome() {
        const auto output=qEnvironmentVariable("ALLOY_NAV_HOME_EVIDENCE");QVERIFY(!output.isEmpty());QVERIFY(QDir().mkpath(output));
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1360,850);window.show();window.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto* tree=window.findChild<QTreeWidget*>("browser.tree");QVERIFY(tree);
        auto* folder=tree->topLevelItem(0)->child(1);QVERIFY(folder);QCOMPARE(folder->text(0),QString("Named Views"));
        auto* home=folder->child(0);QVERIFY(home);QCOMPARE(home->text(0),QString("Home"));
        call(window,"solid.box",{{"width",41},{"depth",27},{"height",13},{"units","mm"}});
        call(window,"view.home");call(window,"view.projection",{{"mode","perspective"}});
        call(window,"view.orbit",{{"deltaX",43},{"deltaY",19}});call(window,"view.pan",{{"x",23},{"y",-11}});
        call(window,"view.zoom",{{"factor",1.3}});
        const auto saved=camera(window),gpu=window.state().value("viewport").toObject().value("gpu").toObject().value("camera").toObject();
        const auto before=call(window,"document.inspect");
        const auto tabId=call(window,"document.tabs").value("activeTabId");
        QVERIFY(contextAction(window,tree,home,"context.homeSet"));
        const auto stored=call(window,"document.inspect").value("homeCamera").toObject();QVERIFY(!stored.isEmpty());
        QCOMPARE(stored.value("projectionPolicy"),QJsonValue("perspective"));
        QCOMPARE(stored.value("customDirection"),gpu.value("direction"));
        QCOMPARE(call(window,"document.inspect").value("evaluation").toObject().value("bodies"),before.value("evaluation").toObject().value("bodies"));
        call(window,"view.orientation",{{"name","top"}});call(window,"view.projection",{{"mode","orthographic"}});
        const auto beforeRestore=call(window,"document.inspect");
        auto* homeButton=window.findChild<QToolButton*>("command.view.home");QVERIFY(homeButton);
        const auto buttonPixel=homeButton->rect().center();
        QCOMPARE(QApplication::widgetAt(homeButton->mapToGlobal(buttonPixel)),static_cast<QWidget*>(homeButton));
        QTest::mouseClick(homeButton,Qt::LeftButton,Qt::NoModifier,buttonPixel);
        const auto restored=camera(window),restoredGpu=window.state().value("viewport").toObject().value("gpu").toObject().value("camera").toObject();
        QCOMPARE(restored.value("projectionPolicy"),QJsonValue("perspective"));
        for(const auto* key:{"centerX","centerY","centerZ","viewCenter","scale","zoom","panX","panY"})
            QCOMPARE(restored.value(key),saved.value(key));
        for(const auto* key:{"direction","up"}){
            const auto actual=restoredGpu.value(key).toArray(),expected=gpu.value(key).toArray();
            QCOMPARE(actual.size(),3);QCOMPARE(expected.size(),3);
            for(int i=0;i<3;++i)QVERIFY(std::abs(actual[i].toDouble()-expected[i].toDouble())<1e-7);
        }
        QCOMPARE(call(window,"document.inspect").value("authored"),beforeRestore.value("authored"));
        QCOMPARE(call(window,"document.inspect").value("undo"),beforeRestore.value("undo"));
        call(window,"capture.screenshot",{{"path",output+"/saved-home.png"}});
        const auto file=output+"/saved-home.alloy";call(window,"document.save",{{"path",file}});
        call(window,"document.tabNew");call(window,"document.tabActivate",{{"tabId",tabId}});
        QCOMPARE(call(window,"document.inspect").value("homeCamera").toObject(),stored);
        QVERIFY(contextAction(window,tree,home,"context.homeReset"));
        QCOMPARE(call(window,"document.inspect").value("homeCamera").toObject(),QJsonObject{});
        call(window,"history.undo");QCOMPARE(call(window,"document.inspect").value("homeCamera").toObject(),stored);
        call(window,"history.redo");QCOMPARE(call(window,"document.inspect").value("homeCamera").toObject(),QJsonObject{});
        call(window,"view.orientation",{{"name","front"}});call(window,"view.projection",{{"mode","orthographic"}});
        QTest::mouseClick(homeButton,Qt::LeftButton,Qt::NoModifier,buttonPixel);
        QCOMPARE(camera(window).value("orientation"),QJsonValue("isometric"));
        QCOMPARE(camera(window).value("projectionPolicy"),QJsonValue("orthographic"));
        call(window,"capture.screenshot",{{"path",output+"/reset-home.png"}});
        QFile source(file);QVERIFY(source.open(QIODevice::ReadOnly));
        auto malformed=QJsonDocument::fromJson(source.readAll()).object();source.close();
        auto invalid=malformed.value("homeCamera").toObject();invalid["scale"]=0;malformed["homeCamera"]=invalid;
        const auto badFile=output+"/invalid-home.alloy";QFile bad(badFile);QVERIFY(bad.open(QIODevice::WriteOnly));
        bad.write(QJsonDocument(malformed).toJson());bad.close();
        const auto beforeBad=call(window,"document.inspect").value("authored");
        QVERIFY(!window.executeApi("document.open",{{"path",badFile},{"discard",true}}).value("ok").toBool());
        QCOMPARE(call(window,"document.inspect").value("authored"),beforeBad);
        call(window,"document.open",{{"path",file},{"discard",true}});
        QCOMPARE(call(window,"document.inspect").value("homeCamera").toObject(),stored);
        window.close();
    }
};
QTEST_MAIN(NavigationHomeControls)
#include "navigation_home_controls.moc"
