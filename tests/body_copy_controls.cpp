// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include <QApplication>
#include <QContextMenuEvent>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QTest>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QJsonDocument>
#include <QDir>
#include <QProcess>
#include <QWindow>
class BodyCopyControlsTest final:public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& w,const QString& c,const QJsonObject& a={}){auto r=w.execute(c,a);if(!r["ok"].toBool())qFatal("%s",QJsonDocument(r).toJson().constData());return r["result"].toObject();}
private slots:
    void browserCopyPastePlacement(){
        AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1400,900);w.show();QVERIFY(QTest::qWaitForWindowExposed(&w));
        // Initial harness activation only; subsequent focus comes from real hits.
#ifdef Q_OS_MACOS
        QProcess activation;activation.start("osascript",{"-e",QString("tell application \"System Events\" to set frontmost of first application process whose unix id is %1 to true").arg(QCoreApplication::applicationPid())});
        QVERIFY(activation.waitForFinished(5000));QCOMPARE(activation.exitCode(),0);
#endif
        w.raise();w.activateWindow();w.windowHandle()->requestActivate();QApplication::setActiveWindow(&w);QVERIFY(QTest::qWaitForWindowActive(&w));
        call(w,"solid.box",{{"width",20},{"depth",20},{"height",10},{"x",-10},{"y",-10},{"z",0},{"units","mm"},{"name","Source"}});call(w,"view.home");
        auto before=call(w,"document.inspect");auto authored=before["authored"].toObject();const auto source=authored["features"].toArray().first().toObject()["bodyId"].toString();
        auto* tree=w.findChild<QTreeWidget*>("browser.tree");QVERIFY(tree);tree->expandAll();
        auto open=[&](QTreeWidgetItem* item){
            tree->scrollToItem(item);QApplication::processEvents();const auto point=tree->visualItemRect(item).center();
            if(QApplication::widgetAt(tree->viewport()->mapToGlobal(point))!=tree->viewport())return static_cast<QMenu*>(nullptr);
            QContextMenuEvent event(QContextMenuEvent::Mouse,point,tree->viewport()->mapToGlobal(point));QApplication::sendEvent(tree->viewport(),&event);
            return qobject_cast<QMenu*>(QApplication::activePopupWidget());
        };
        QTreeWidgetItem* row=nullptr;for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole+1)==source){row=*it;break;}QVERIFY(row);
        auto* folder=row->parent();auto* menu=open(row);QVERIFY(menu);auto* copy=menu->findChild<QAction*>("context.copyBody");QVERIFY(copy && copy->isEnabled());
        QCOMPARE(QApplication::widgetAt(menu->mapToGlobal(menu->actionGeometry(copy).center())),static_cast<QWidget*>(menu));QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,menu->actionGeometry(copy).center());
        QTRY_VERIFY(!QApplication::activePopupWidget());QCOMPARE(w.state()["selection"].toObject()["bodyId"],QJsonValue(source));
        auto paste=[&](){auto* popup=open(folder);if(!popup)return false;auto* action=popup->findChild<QAction*>("context.pasteBody");if(!action || !action->isEnabled())return false;QTest::mouseClick(popup,Qt::LeftButton,Qt::NoModifier,popup->actionGeometry(action).center());return true;};
        QVERIFY(paste());QDockWidget* dock=nullptr;QTRY_VERIFY((dock=w.findChild<QDockWidget*>("body.copy.dialog"))&&dock->isVisible());QTRY_VERIFY(!QApplication::activePopupWidget());
        const auto evidence=qEnvironmentVariable("ALLOY_BODY_COPY_EVIDENCE");if(!evidence.isEmpty())QVERIFY(QDir().mkpath(evidence));
        auto typeX=[&](){auto* field=dock->findChild<QDoubleSpinBox*>("body.copy.xMm");if(!field)return false;auto* editor=field->findChild<QLineEdit*>();if(!editor)return false;const auto p=editor->rect().center();if(QApplication::widgetAt(editor->mapToGlobal(p))!=editor)return false;QTest::mouseClick(editor,Qt::LeftButton,Qt::NoModifier,p);if(!editor->hasFocus())return false;QTest::keyClick(editor,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(editor,"30");QTest::keyClick(editor,Qt::Key_Tab);return true;};
        QVERIFY(typeX());QCOMPARE(w.state()["activeSession"].toObject()["inputs"].toObject()["xMm"].toDouble(),30.0);QVERIFY(w.state()["activeSession"].toObject()["previewed"].toBool());QCOMPARE(call(w,"document.inspect")["authored"].toObject(),authored);
        if(!evidence.isEmpty()){call(w,"capture.screenshot",{{"path",evidence+"/native-preview.png"}});call(w,"capture.dialog",{{"path",evidence+"/native-panel.png"}});}
        auto click=[&](const char* name){auto* button=dock->findChild<QPushButton*>(name);if(!button || QApplication::widgetAt(button->mapToGlobal(button->rect().center()))!=button)return false;QTest::mouseClick(button,Qt::LeftButton);return true;};
        QVERIFY(click("body.copy.cancel"));QTRY_VERIFY(w.state()["activeSession"].toObject().isEmpty());QCOMPARE(call(w,"document.inspect")["authored"].toObject(),authored);
        QVERIFY(paste());QTRY_VERIFY((dock=w.findChild<QDockWidget*>("body.copy.dialog"))&&dock->isVisible());QTRY_VERIFY(!QApplication::activePopupWidget());QVERIFY(typeX());QVERIFY(click("body.copy.ok"));QTRY_VERIFY(w.state()["activeSession"].toObject().isEmpty());
        const auto result=call(w,"document.inspect");QCOMPARE(result["bodies"].toInt(),2);const auto f=result["authored"].toObject()["features"].toArray().last().toObject();QCOMPARE(f["type"].toString(),QString("bodyCopy"));QVERIFY(f["bodyId"]!=source);QCOMPARE(f["parameters"].toObject()["xMm"].toDouble(),30.0);
        if(!evidence.isEmpty()){call(w,"view.home");call(w,"capture.screenshot",{{"path",evidence+"/native-committed.png"}});}
        w.close();
    }
};
QTEST_MAIN(BodyCopyControlsTest)
#include "body_copy_controls.moc"
