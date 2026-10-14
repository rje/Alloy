// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include <QApplication>
#include <QContextMenuEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QJsonDocument>
#include <QLineEdit>
#include <QMenu>
#include <QPointer>
#include <QProcess>
#include <QPushButton>
#include <QTest>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QWindow>

class BodyRenameControlsTest final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& args={}) {
        const auto response=window.executeApi(command,args);
        if(!response["ok"].toBool())qFatal("%s: %s",qPrintable(command),QJsonDocument(response).toJson().constData());
        return response["result"].toObject();
    }
    QTreeWidgetItem* row(QTreeWidget* tree,const QString& id) {
        for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole+1).toString()==id && !(*it)->isHidden())return *it;
        return nullptr;
    }
    void context(AlloyWindow& window,QTreeWidget* tree,QTreeWidgetItem* item) {
        QVERIFY(item);tree->scrollToItem(item);QTest::qWait(50);
        const auto point=tree->visualItemRect(item).center();QVERIFY(tree->viewport()->rect().contains(point));
        QTimer::singleShot(40,&window,[]{
            auto* menu=qobject_cast<QMenu*>(QApplication::activePopupWidget());if(!menu)qFatal("Body context menu unavailable");
            for(auto* action:menu->actions())if(action->objectName()=="context.renameBody"){
                QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,menu->actionGeometry(action).center());return;}
            qFatal("Rename action unavailable");
        });
        QContextMenuEvent event(QContextMenuEvent::Mouse,point,tree->viewport()->mapToGlobal(point));
        QApplication::sendEvent(tree->viewport(),&event);
    }
private slots:
    void browserRename(){
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1260,850);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));window.activateWindow();
        call(window,"solid.box",{{"width",20},{"depth",15},{"height",10},{"units","mm"}});
        const auto before=call(window,"document.inspect");const auto body=before["evaluation"].toObject()["bodies"].toArray()[0].toObject();
        const auto id=body["bodyId"].toString();auto* tree=window.findChild<QTreeWidget*>("browser.tree");QVERIFY(tree);
        tree->expandAll();auto* item=row(tree,id);QVERIFY(item);
#ifdef Q_OS_MACOS
        QProcess activation;activation.start("osascript",{"-e",QString("tell application \"System Events\" to set frontmost of first application process whose unix id is %1 to true").arg(QCoreApplication::applicationPid())});
        QVERIFY(activation.waitForFinished(5000));QCOMPARE(activation.exitCode(),0);
#endif
        window.raise();window.activateWindow();window.windowHandle()->requestActivate();QApplication::setActiveWindow(&window);
        QTRY_VERIFY(window.isActiveWindow());
        context(window,tree,item);QPointer<QDialog> dialog=window.findChild<QDialog*>("body.rename.dialog");QVERIFY(dialog && dialog->isVisible());
        QTRY_COMPARE(QApplication::activeModalWidget(),static_cast<QWidget*>(dialog));
        auto* name=dialog->findChild<QLineEdit*>("body.rename.name");QVERIFY(name);
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(name));
        QCOMPARE(name->text(),body["name"].toString());QCOMPARE(name->selectedText(),name->text());
        QTest::keyClick(name,Qt::Key_Escape);QTRY_VERIFY(!dialog || !dialog->isVisible());
        QCOMPARE(call(window,"document.inspect")["authored"],before["authored"]);
        context(window,tree,row(tree,id));dialog=window.findChild<QDialog*>("body.rename.dialog");QVERIFY(dialog && dialog->isVisible());
        QTRY_COMPARE(QApplication::activeModalWidget(),static_cast<QWidget*>(dialog));
        name=dialog->findChild<QLineEdit*>("body.rename.name");QVERIFY(name);
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(name));
        QCOMPARE(name->selectedText(),name->text());QTest::keyClicks(name,"Bottom");
        QCOMPARE(name->text(),QString("Bottom"));
        const auto evidence=qEnvironmentVariable("ALLOY_BODY_RENAME_EVIDENCE");
        if(!evidence.isEmpty())call(window,"capture.dialog",{{"path",evidence+"/rename-dialog.png"}});
        QTest::keyClick(name,Qt::Key_Return);
        QTRY_VERIFY(!dialog || !dialog->isVisible());
        const auto after=call(window,"document.inspect");
        QCOMPARE(after["evaluation"].toObject()["bodies"].toArray()[0].toObject()["name"].toString(),QString("Bottom"));
        QCOMPARE(after["authored"].toObject()["features"],before["authored"].toObject()["features"]);
        auto oldBody=before["evaluation"].toObject()["bodies"].toArray()[0].toObject();oldBody.remove("name");
        auto newBody=after["evaluation"].toObject()["bodies"].toArray()[0].toObject();newBody.remove("name");QCOMPARE(newBody,oldBody);
        QCOMPARE(row(tree,id)->text(0),QString("Bottom"));
        if(!evidence.isEmpty())call(window,"capture.screenshot",{{"path",evidence+"/renamed-browser.png"}});
        call(window,"document.new",{{"discard",true}});window.close();
    }
};
QTEST_MAIN(BodyRenameControlsTest)
#include "body_rename_controls.moc"
