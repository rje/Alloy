// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include <QApplication>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QCursor>
#include <QDialog>
#include <QDialogButtonBox>
#include <QJsonDocument>
#include <QMenu>
#include <QPointer>
#include <QPushButton>
#include <QTest>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>

class ComponentReparentControlsTest final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& args={}) {
        const auto response=window.execute(command,args);
        if(!response["ok"].toBool())qFatal("%s: %s",qPrintable(command),QJsonDocument(response).toJson().constData());
        return response["result"].toObject();
    }
    QTreeWidgetItem* row(QTreeWidget* tree,const QString& id,int role) {
        for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,role).toString()==id && !(*it)->isHidden())return *it;
        return nullptr;
    }
    void openContext(AlloyWindow& window,QTreeWidget* tree,QTreeWidgetItem* item,const QString& actionId) {
        QVERIFY(item && !item->isHidden());
        const auto point=tree->visualItemRect(item).center();
        QTimer::singleShot(40,&window,[actionId]{
            auto* menu=qobject_cast<QMenu*>(QApplication::activePopupWidget());
            if(!menu)qFatal("Browser context menu unavailable");
            for(auto* action:menu->actions())if(action->objectName()==actionId){
                QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,menu->actionGeometry(action).center());return;
            }
            qFatal("Requested Browser move action unavailable");
        });
        QContextMenuEvent event(QContextMenuEvent::Mouse,point,tree->viewport()->mapToGlobal(point));
        QApplication::sendEvent(tree->viewport(),&event);
    }
private slots:
    void dragBodyThenSketches(){
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1320,850);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));window.activateWindow();
        call(window,"sketch.begin",{{"plane","XY"},{"name","Source sketch"}});
        call(window,"sketch.centerRectangle",{{"centerX",0},{"centerY",0},{"width",20},{"height",10},{"units","mm"}});
        call(window,"sketch.finish");
        const auto sketched=call(window,"document.inspect");
        const auto sketch=sketched["evaluation"].toObject()["sketches"].toArray()[0].toObject();
        const auto sketchId=sketch["sketchId"].toString();
        call(window,"solid.extrude",{{"sketchId",sketchId},{"profileId",sketch["profiles"].toArray()[0].toObject()["profileId"]},
            {"distance",5},{"units","mm"}});
        const auto owner=call(window,"component.create",{{"name","Finished part"},{"activate",false}})["definitionId"].toString();
        const auto before=call(window,"document.inspect");
        const auto bodyId=before["evaluation"].toObject()["bodies"].toArray()[0].toObject()["bodyId"].toString();
        auto* tree=window.findChild<QTreeWidget*>("browser.tree");QVERIFY(tree);tree->expandAll();
        auto* destination=row(tree,owner,Qt::UserRole+3);QVERIFY(destination);
        auto drag=[&](QTreeWidgetItem* source){
            QVERIFY(source);tree->scrollToItem(source);tree->scrollToItem(destination);
            const auto start=tree->visualItemRect(source).center(),end=tree->visualItemRect(destination).center();
            QVERIFY(tree->viewport()->rect().contains(start) && tree->viewport()->rect().contains(end));
            QTest::mousePress(tree->viewport(),Qt::LeftButton,Qt::NoModifier,start);
            QTimer::singleShot(100,&window,[=]{
                QCursor::setPos(tree->viewport()->mapToGlobal(end));
                QTest::mouseMove(tree->viewport(),end);
                QTest::mouseRelease(tree->viewport(),Qt::LeftButton,Qt::NoModifier,end);
            });
            QTest::mouseMove(tree->viewport(),start+QPoint(30,0));
            QTRY_VERIFY(window.findChild<QDialog*>("component.reparent.dialog"));
        };
        drag(row(tree,bodyId,Qt::UserRole+1));
        QPointer<QDialog> dialog=window.findChild<QDialog*>("component.reparent.dialog");
        QVERIFY(dialog && dialog->isVisible());
        QCOMPARE(dialog->findChild<QComboBox*>("component.reparent.destination")->currentData().toString(),owner);
        QTest::mouseClick(dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok),Qt::LeftButton);
        QTRY_VERIFY(!dialog || !dialog->isVisible());
        const auto bodyOnly=call(window,"document.inspect");
        QCOMPARE(bodyOnly["evaluation"].toObject()["bodies"].toArray()[0].toObject()["ownerId"].toString(),owner);
        QCOMPARE(bodyOnly["evaluation"].toObject()["sketches"].toArray()[0].toObject()["ownerId"].toString(),
            before["authored"].toObject()["documentId"].toString());
        drag(row(tree,"sketches",Qt::UserRole+4));
        dialog=window.findChild<QDialog*>("component.reparent.dialog");QVERIFY(dialog && dialog->isVisible());
        QTest::mouseClick(dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok),Qt::LeftButton);
        QTRY_VERIFY(!dialog || !dialog->isVisible());
        const auto both=call(window,"document.inspect");
        QCOMPARE(both["evaluation"].toObject()["sketches"].toArray()[0].toObject()["ownerId"].toString(),owner);
        const auto evidence=qEnvironmentVariable("ALLOY_REPARENT_EVIDENCE");
        if(!evidence.isEmpty())call(window,"capture.screenshot",{{"path",evidence+"/drag-body-then-sketches.png"}});
        call(window,"document.new",{{"discard",true}});window.close();
    }
    void bodyThenSketches(){
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1320,850);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));window.activateWindow();
        call(window,"sketch.begin",{{"plane","XY"},{"name","Source sketch"}});
        call(window,"sketch.centerRectangle",{{"centerX",0},{"centerY",0},{"width",20},{"height",10},{"units","mm"}});
        call(window,"sketch.finish");const auto sketched=call(window,"document.inspect");
        const auto sketch=sketched["evaluation"].toObject()["sketches"].toArray()[0].toObject();
        const auto sketchId=sketch["sketchId"].toString();
        call(window,"solid.extrude",{{"sketchId",sketchId},{"profileId",sketch["profiles"].toArray()[0].toObject()["profileId"]},
            {"distance",5},{"units","mm"}});
        const auto created=call(window,"component.create",{{"name","Finished part"},{"activate",false}});
        const auto owner=created["definitionId"].toString();const auto before=call(window,"document.inspect");
        const auto bodyId=before["evaluation"].toObject()["bodies"].toArray()[0].toObject()["bodyId"].toString();
        auto* tree=window.findChild<QTreeWidget*>("browser.tree");QVERIFY(tree);
        tree->expandAll();
        auto* body=row(tree,bodyId,Qt::UserRole+1);QVERIFY(body);
        openContext(window,tree,body,"context.reparentBody");
        QPointer<QDialog> dialog=window.findChild<QDialog*>("component.reparent.dialog");QVERIFY(dialog && dialog->isVisible());
        QCOMPARE(dialog->findChild<QComboBox*>("component.reparent.destination")->count(),2);
        const auto revision=before["revision"].toInteger();
        QTest::keyClick(dialog,Qt::Key_Escape);QTRY_VERIFY(!dialog || !dialog->isVisible());
        QCOMPARE(call(window,"document.inspect")["revision"].toInteger(),revision);
        body=row(tree,bodyId,Qt::UserRole+1);openContext(window,tree,body,"context.reparentBody");
        dialog=window.findChild<QDialog*>("component.reparent.dialog");QVERIFY(dialog && dialog->isVisible());
        auto* chooser=dialog->findChild<QComboBox*>("component.reparent.destination");QVERIFY(chooser);
        QCOMPARE(chooser->currentData().toString(),owner);
        const auto evidence=qEnvironmentVariable("ALLOY_REPARENT_EVIDENCE");
        if(!evidence.isEmpty())call(window,"capture.dialog",{{"path",evidence+"/body-move-dialog.png"}});
        QTest::mouseClick(dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok),Qt::LeftButton);
        QTRY_VERIFY(!dialog || !dialog->isVisible());
        const auto bodyOnly=call(window,"document.inspect");
        QCOMPARE(bodyOnly["evaluation"].toObject()["bodies"].toArray()[0].toObject()["ownerId"].toString(),owner);
        QCOMPARE(bodyOnly["evaluation"].toObject()["sketches"].toArray()[0].toObject()["ownerId"].toString(),
            before["authored"].toObject()["documentId"].toString());
        if(!evidence.isEmpty())call(window,"capture.screenshot",{{"path",evidence+"/body-only-browser.png"}});
        auto* folder=row(tree,"sketches",Qt::UserRole+4);QVERIFY(folder);
        openContext(window,tree,folder,"context.reparentSketches");
        dialog=window.findChild<QDialog*>("component.reparent.dialog");QVERIFY(dialog && dialog->isVisible());
        QCOMPARE(dialog->findChild<QComboBox*>("component.reparent.destination")->currentData().toString(),owner);
        if(!evidence.isEmpty())call(window,"capture.dialog",{{"path",evidence+"/sketches-move-dialog.png"}});
        QTest::mouseClick(dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok),Qt::LeftButton);
        QTRY_VERIFY(!dialog || !dialog->isVisible());
        const auto both=call(window,"document.inspect");
        QCOMPARE(both["evaluation"].toObject()["sketches"].toArray()[0].toObject()["ownerId"].toString(),owner);
        if(!evidence.isEmpty())call(window,"capture.screenshot",{{"path",evidence+"/body-and-sketches-browser.png"}});
        call(window,"document.new",{{"discard",true}});window.close();
    }
};
QTEST_MAIN(ComponentReparentControlsTest)
#include "component_reparent_controls.moc"
