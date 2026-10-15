// SPDX-License-Identifier: MIT
#include "AlloyTheme.h"
#include "AlloyWindow.h"
#include <QApplication>
#include <QContextMenuEvent>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QLineEdit>
#include <QMenu>
#include <QTest>
#include <QTimer>
#include <QTreeWidget>
#include <cmath>

class NavigationNamedViewsControls final : public QObject {
    Q_OBJECT
    static QJsonObject call(AlloyWindow& window,const QString& name,const QJsonObject& args={}) {
        const auto response=window.executeApi(name,args);
        if(!response.value("ok").toBool())qFatal("%s",QJsonDocument(response).toJson(QJsonDocument::Compact).constData());
        return response.value("result").toObject();
    }
    static QJsonObject camera(AlloyWindow& window) {
        return window.state().value("viewport").toObject().value("navigation").toObject();
    }
private slots:
    void persistedCustomView() {
        const auto output=qEnvironmentVariable("ALLOY_NAV_NAMED_EVIDENCE");
        QVERIFY(!output.isEmpty());QVERIFY(QDir().mkpath(output));
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1360,850);window.show();window.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto* tree=window.findChild<QTreeWidget*>("browser.tree");QVERIFY(tree);
        call(window,"solid.box",{{"width",36},{"depth",22},{"height",13},{"units","mm"}});
        call(window,"view.home");call(window,"view.projection",{{"mode","perspective"}});
        call(window,"view.orbit",{{"deltaX",47},{"deltaY",22}});
        call(window,"view.pan",{{"x",31},{"y",-17}});call(window,"view.zoom",{{"factor",1.35}});
        const auto savedCamera=camera(window),savedGpu=window.state().value("viewport").toObject().value("gpu").toObject().value("camera").toObject();
        const auto beforeCreate=call(window,"document.inspect");
        const auto tabId=call(window,"document.tabs").value("activeTabId");
        auto* folder=tree->topLevelItem(0)->child(1);QVERIFY(folder);
        QCOMPARE(folder->text(0),QString("Named Views"));tree->scrollToItem(folder);
        const auto pixel=tree->visualItemRect(folder).center();QVERIFY(tree->viewport()->rect().contains(pixel));
        bool menuOwned=false;
        QTimer::singleShot(50,&window,[&]{
            auto* popup=qobject_cast<QMenu*>(QApplication::activePopupWidget());if(!popup)return;
            QAction* create=nullptr;for(auto* action:popup->actions())if(action->objectName()=="context.namedViewCreate")create=action;
            if(!create)return;const auto row=popup->actionGeometry(create).center();
            menuOwned=QApplication::widgetAt(popup->mapToGlobal(row))==popup;
            if(menuOwned)QTest::mouseClick(popup,Qt::LeftButton,Qt::NoModifier,row);
        });
        QTimer::singleShot(2000,&window,[]{if(auto* popup=QApplication::activePopupWidget())popup->close();});
        QContextMenuEvent context(QContextMenuEvent::Mouse,pixel,tree->viewport()->mapToGlobal(pixel));
        QApplication::sendEvent(tree->viewport(),&context);
        QVERIFY(menuOwned);
        const auto afterCreate=call(window,"document.inspect");
        const auto views=afterCreate.value("namedViews").toArray();QCOMPARE(views.size(),1);
        const auto row=views.first().toObject();
        const auto viewId=row.value("id").toString();
        QVERIFY(!viewId.isEmpty());QCOMPARE(row.value("name"),QJsonValue("View 1"));
        QCOMPARE(row.value("camera").toObject().value("projectionPolicy"),QJsonValue("perspective"));
        QCOMPARE(row.value("camera").toObject().value("customDirection"),savedGpu.value("direction"));
        QCOMPARE(row.value("camera").toObject().value("customUp"),savedGpu.value("up"));
        QCOMPARE(afterCreate.value("evaluation").toObject().value("bodies"),beforeCreate.value("evaluation").toObject().value("bodies"));
        QVERIFY(afterCreate.value("undo").toObject().value("count").toInt()>beforeCreate.value("undo").toObject().value("count").toInt());
        QTreeWidgetItem* custom=nullptr;
        for(int index=0;index<folder->childCount();++index)if(folder->child(index)->data(0,Qt::UserRole+9)==viewId)custom=folder->child(index);
        QVERIFY(custom && !custom->isHidden());tree->scrollToItem(custom);
        const auto itemPixel=tree->visualItemRect(custom).center();
        QTest::mouseClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,itemPixel);
        QTest::mouseDClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,itemPixel);
        QCoreApplication::processEvents();
        auto* editor=qobject_cast<QLineEdit*>(QApplication::focusWidget());QVERIFY(editor);
        editor->selectAll();QTest::keyClicks(editor,"Assembly Angle");QTest::keyClick(editor,Qt::Key_Return);
        QTRY_COMPARE_WITH_TIMEOUT(call(window,"document.inspect").value("namedViews").toArray().first().toObject().value("name"),QJsonValue("Assembly Angle"),1000);
        QCOMPARE(call(window,"document.inspect").value("namedViews").toArray().first().toObject().value("id"),QJsonValue(viewId));
        tree->scrollToItem(custom);const auto renamePixel=tree->visualItemRect(custom).center();
        bool renameMenuOwned=false;
        QTimer::singleShot(50,&window,[&]{
            auto* popup=qobject_cast<QMenu*>(QApplication::activePopupWidget());if(!popup)return;
            QAction* rename=nullptr;for(auto* action:popup->actions())if(action->objectName()=="context.namedViewRename")rename=action;
            if(!rename)return;const auto point=popup->actionGeometry(rename).center();
            renameMenuOwned=QApplication::widgetAt(popup->mapToGlobal(point))==popup;
            if(renameMenuOwned)QTest::mouseClick(popup,Qt::LeftButton,Qt::NoModifier,point);
        });
        QTimer::singleShot(2000,&window,[]{if(auto* popup=QApplication::activePopupWidget())popup->close();});
        QContextMenuEvent renameContext(QContextMenuEvent::Mouse,renamePixel,tree->viewport()->mapToGlobal(renamePixel));
        QApplication::sendEvent(tree->viewport(),&renameContext);QVERIFY(renameMenuOwned);
        QCoreApplication::processEvents();
        auto* renameEditor=qobject_cast<QLineEdit*>(QApplication::focusWidget());QVERIFY(renameEditor);
        renameEditor->selectAll();QTest::keyClicks(renameEditor,"Assembly View");QTest::keyClick(renameEditor,Qt::Key_Return);
        QTRY_COMPARE_WITH_TIMEOUT(call(window,"document.inspect").value("namedViews").toArray().first().toObject().value("name"),QJsonValue("Assembly View"),1000);
        QCoreApplication::processEvents();
        QVERIFY(QApplication::activePopupWidget()==nullptr);
        for(auto* widget:QApplication::allWidgets())if(auto* menu=qobject_cast<QMenu*>(widget))QVERIFY(!menu->isVisible());
        call(window,"capture.screenshot",{{"path",output+"/named-view-row.png"}});

        call(window,"view.home");call(window,"view.projection",{{"mode","orthographic"}});
        const auto beforeRestore=call(window,"document.inspect");
        tree->scrollToItem(custom);QTest::mouseClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,tree->visualItemRect(custom).center());
        const auto restored=camera(window),restoredGpu=window.state().value("viewport").toObject().value("gpu").toObject().value("camera").toObject();
        QCOMPARE(restored.value("projectionPolicy"),QJsonValue("perspective"));
        QCOMPARE(restored.value("resolvedProjection"),QJsonValue("perspective"));
        QCOMPARE(restored.value("viewCenter"),savedCamera.value("viewCenter"));
        for(const auto* key:{"centerX","centerY","centerZ","scale","zoom","panX","panY"})QCOMPARE(restored.value(key),savedCamera.value(key));
        for(const auto* key:{"direction","up"}) {
            const auto actual=restoredGpu.value(key).toArray(),expected=savedGpu.value(key).toArray();
            QCOMPARE(actual.size(),3);QCOMPARE(expected.size(),3);
            for(int index=0;index<3;++index)QVERIFY(std::abs(actual[index].toDouble()-expected[index].toDouble())<1e-7);
        }
        QCOMPARE(call(window,"document.inspect").value("authored"),beforeRestore.value("authored"));
        QCOMPARE(call(window,"document.inspect").value("undo"),beforeRestore.value("undo"));
        call(window,"capture.screenshot",{{"path",output+"/restored-perspective.png"}});
        const auto file=output+"/named-view.alloy";call(window,"document.save",{{"path",file}});
        call(window,"document.tabNew");call(window,"document.tabActivate",{{"tabId",tabId}});
        QCOMPARE(camera(window).value("projectionPolicy"),QJsonValue("perspective"));
        call(window,"view.namedDelete",{{"viewId",viewId}});
        QCOMPARE(call(window,"document.inspect").value("namedViews").toArray().size(),0);
        call(window,"history.undo");QCOMPARE(call(window,"document.inspect").value("namedViews").toArray().size(),1);
        call(window,"history.redo");QCOMPARE(call(window,"document.inspect").value("namedViews").toArray().size(),0);
        QFile source(file);QVERIFY(source.open(QIODevice::ReadOnly));
        auto malformed=QJsonDocument::fromJson(source.readAll()).object();source.close();
        auto records=malformed.value("namedViews").toArray();auto bad=records.first().toObject();auto pose=bad.value("camera").toObject();
        pose["scale"]=0;bad["camera"]=pose;records[0]=bad;malformed["namedViews"]=records;
        const auto badFile=output+"/invalid-named-view.alloy";QFile destination(badFile);QVERIFY(destination.open(QIODevice::WriteOnly));
        destination.write(QJsonDocument(malformed).toJson());destination.close();
        const auto beforeBad=call(window,"document.inspect").value("authored");
        const auto refusal=window.executeApi("document.open",{{"path",badFile},{"discard",true}});
        QVERIFY(!refusal.value("ok").toBool());QCOMPARE(call(window,"document.inspect").value("authored"),beforeBad);
        call(window,"document.open",{{"path",file},{"discard",true}});
        QCOMPARE(call(window,"document.inspect").value("namedViews").toArray().first().toObject().value("id"),QJsonValue(viewId));
        call(window,"view.namedRestore",{{"viewId",viewId}});
        QCOMPARE(camera(window).value("projectionPolicy"),QJsonValue("perspective"));
        window.close();
    }
};
QTEST_MAIN(NavigationNamedViewsControls)
#include "navigation_named_views_controls.moc"
