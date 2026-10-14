// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include <QApplication>
#include <QCheckBox>
#include <QContextMenuEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QJsonDocument>
#include <QLineEdit>
#include <QMenu>
#include <QPointer>
#include <QPushButton>
#include <QRadioButton>
#include <QTest>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>

class BrowserOrganizationControlsTest final:public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& w,const QString& id,const QJsonObject& args={}){
        const auto response=w.execute(id,args);
        if(!response["ok"].toBool())qFatal("%s: %s",qPrintable(id),QJsonDocument(response).toJson().constData());
        return response["result"].toObject();
    }
    static QTreeWidgetItem* row(QTreeWidget* tree,const QString& id,int role){
        for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,role).toString()==id && !(*it)->isHidden())return *it;
        return nullptr;
    }
    static void menuAction(QWidget* target,const QString& name){
        QTimer::singleShot(40,qApp,[name]{auto* menu=qobject_cast<QMenu*>(QApplication::activePopupWidget());
            if(!menu)qFatal("Browser context menu unavailable");
            for(auto* action:menu->actions())if(action->objectName()==name){
                QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,menu->actionGeometry(action).center());return;}
            qFatal("Browser context action unavailable: %s",qPrintable(name));});
        const auto point=target->rect().center();
        QContextMenuEvent event(QContextMenuEvent::Mouse,point,target->mapToGlobal(point));QApplication::sendEvent(target,&event);
    }
    static void menuAction(QTreeWidget* tree,QTreeWidgetItem* item,const QString& name){
        tree->scrollToItem(item);QApplication::processEvents();
        const auto point=tree->visualItemRect(item).center();
        if(!tree->viewport()->rect().contains(point))qFatal("Browser row is offscreen");
        QTimer::singleShot(40,tree,[name]{auto* menu=qobject_cast<QMenu*>(QApplication::activePopupWidget());
            if(!menu)qFatal("Browser context menu unavailable");
            for(auto* action:menu->actions())if(action->objectName()==name){
                QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,menu->actionGeometry(action).center());return;}
            qFatal("Browser context action unavailable: %s",qPrintable(name));});
        QContextMenuEvent event(QContextMenuEvent::Mouse,point,tree->viewport()->mapToGlobal(point));
        QApplication::sendEvent(tree->viewport(),&event);
    }
private slots:
    void sourceOrder(){
        const auto source=qEnvironmentVariable("ALLOY_ASM022_SOURCE");QVERIFY(!source.isEmpty());
        AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1510,920);w.show();QVERIFY(QTest::qWaitForWindowExposed(&w));w.activateWindow();
        call(w,"document.open",{{"path",source},{"discard",true}});
        const auto initial=call(w,"document.inspect"),baseline=initial["authored"].toObject();
        QCOMPARE(initial["evaluation"].toObject()["bodies"].toArray().size(),4);
        auto* tree=w.findChild<QTreeWidget*>("browser.tree");QVERIFY(tree);tree->expandAll();
        auto* folder=row(tree,"bodies",Qt::UserRole+4);QVERIFY(folder);
        menuAction(tree,folder,"context.createComponentsFromBodiesFolder");
        QPointer<QDialog> dialog=w.findChild<QDialog*>("component.convert.dialog");QVERIFY(dialog&&dialog->isVisible());
        QTest::keyClick(dialog,Qt::Key_Escape);QTRY_VERIFY(!dialog||!dialog->isVisible());
        QCOMPARE(call(w,"document.inspect")["authored"].toObject(),baseline);
        menuAction(tree,folder,"context.createComponentsFromBodiesFolder");
        dialog=w.findChild<QDialog*>("component.convert.dialog");QVERIFY(dialog&&dialog->isVisible());
        QTest::mouseClick(dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok),Qt::LeftButton);
        QTRY_VERIFY(!dialog||!dialog->isVisible());
        const auto converted=call(w,"document.inspect")["authored"].toObject();
        const auto components=converted["components"].toArray(),occurrences=converted["occurrences"].toArray();
        QCOMPARE(components.size(),4);QCOMPARE(occurrences.size(),4);
        auto* rootRadio=w.findChild<QRadioButton*>("component.activate.root");QVERIFY(rootRadio);
        menuAction(rootRadio,"context.newComponent");
        dialog=w.findChild<QDialog*>("component.new.dialog");QVERIFY(dialog&&dialog->isVisible());
        auto* newName=dialog->findChild<QLineEdit*>("component.new.name");QVERIFY(newName);
        QTest::keyClick(newName,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(newName,"Piston subassembly");
        QTest::mouseClick(dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok),Qt::LeftButton);
        QTRY_VERIFY(!dialog||!dialog->isVisible());
        const auto created=call(w,"document.inspect")["authored"].toObject();
        const auto parent=created["components"].toArray().last().toObject()["id"].toString();
        QVERIFY(!parent.isEmpty());
        tree->collapseAll();
        tree->topLevelItem(0)->setExpanded(true);
        QApplication::processEvents();
        {
            auto* sourceRow=row(tree,occurrences[2].toObject()["id"].toString(),Qt::UserRole+8);QVERIFY(sourceRow);
            auto* radio=qobject_cast<QRadioButton*>(tree->itemWidget(sourceRow,0));QVERIFY(radio);
            const auto outside=w.mapToGlobal(w.rect().bottomRight()+QPoint(40,40));
            QTest::mousePress(radio,Qt::LeftButton,Qt::NoModifier,radio->rect().center());
            QMouseEvent move(QEvent::MouseMove,radio->mapFromGlobal(outside),outside,Qt::NoButton,Qt::LeftButton,Qt::NoModifier);
            QApplication::sendEvent(radio,&move);
            QMouseEvent release(QEvent::MouseButtonRelease,radio->mapFromGlobal(outside),outside,Qt::LeftButton,Qt::NoButton,Qt::NoModifier);
            QApplication::sendEvent(radio,&release);
            QCOMPARE(call(w,"document.inspect")["authored"].toObject(),created);
            QVERIFY(!radio->isDown());
            auto* targetRow=row(tree,parent,Qt::UserRole+3);QVERIFY(targetRow);
            const auto targetPoint=tree->viewport()->mapToGlobal(tree->visualItemRect(targetRow).center());
            QTest::mousePress(radio,Qt::LeftButton,Qt::NoModifier,radio->rect().center());
            QMouseEvent escapeMove(QEvent::MouseMove,radio->mapFromGlobal(targetPoint),targetPoint,Qt::NoButton,Qt::LeftButton,Qt::NoModifier);
            QApplication::sendEvent(radio,&escapeMove);
            QTest::keyClick(radio,Qt::Key_Escape);
            QMouseEvent escapeRelease(QEvent::MouseButtonRelease,radio->mapFromGlobal(targetPoint),targetPoint,Qt::LeftButton,Qt::NoButton,Qt::NoModifier);
            QApplication::sendEvent(radio,&escapeRelease);
            QCOMPARE(call(w,"document.inspect")["authored"].toObject(),created);
            QVERIFY(!radio->isDown());
        }
        // Native drag starts on each ordinary component radio label and lands on the visible parent row.
        for(int i=2;i<4;++i){
            const auto sourceId=occurrences[i].toObject()["id"].toString();
            auto* sourceRow=row(tree,sourceId,Qt::UserRole+8);auto* targetRow=row(tree,parent,Qt::UserRole+3);
            QVERIFY(sourceRow&&targetRow);tree->scrollToItem(targetRow,QAbstractItemView::PositionAtCenter);QApplication::processEvents();
            auto* radio=qobject_cast<QRadioButton*>(tree->itemWidget(sourceRow,0));
            QVERIFY(radio);
            QVERIFY(tree->viewport()->rect().contains(tree->visualItemRect(sourceRow).center()));
            QVERIFY(tree->viewport()->rect().contains(tree->visualItemRect(targetRow).center()));
            const auto sourcePoint=radio->rect().center();const auto targetPoint=tree->viewport()->mapToGlobal(tree->visualItemRect(targetRow).center());
            QTest::mousePress(radio,Qt::LeftButton,Qt::NoModifier,sourcePoint);
            QMouseEvent move(QEvent::MouseMove,radio->mapFromGlobal(targetPoint),targetPoint,Qt::NoButton,Qt::LeftButton,Qt::NoModifier);
            QApplication::sendEvent(radio,&move);
            QMouseEvent release(QEvent::MouseButtonRelease,radio->mapFromGlobal(targetPoint),targetPoint,Qt::LeftButton,Qt::NoButton,Qt::NoModifier);
            QApplication::sendEvent(radio,&release);
            QTRY_COMPARE(call(w,"document.inspect")["authored"].toObject()["occurrences"].toArray()[i].toObject()["parentId"].toString(),parent);
        }
        auto* parentRow=row(tree,parent,Qt::UserRole+3);QVERIFY(parentRow);
        auto* parentRadio=qobject_cast<QRadioButton*>(tree->itemWidget(parentRow,0));QVERIFY(parentRadio);
        QCOMPARE(parentRow->text(0).startsWith("▣"),true);
        const auto grouped=call(w,"document.inspect")["authored"].toObject();
        QTest::mouseDClick(parentRadio,Qt::LeftButton,Qt::NoModifier,parentRadio->rect().center());
        QTRY_VERIFY(w.findChild<QDialog*>("component.rename.dialog")&&w.findChild<QDialog*>("component.rename.dialog")->isVisible());
        dialog=w.findChild<QDialog*>("component.rename.dialog");
        auto* rename=dialog->findChild<QLineEdit*>("component.rename.name");QVERIFY(rename);
        QTest::keyClick(rename,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(rename,"Piston pair");
        QTest::mouseClick(dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok),Qt::LeftButton);
        QTRY_VERIFY(!dialog||!dialog->isVisible());
        const auto renamed=call(w,"document.inspect")["authored"].toObject();
        QCOMPARE(renamed["components"].toArray().last().toObject()["name"].toString(),"Piston pair");
        QCOMPARE(renamed["occurrences"],grouped["occurrences"]);
        menuAction(rootRadio,"context.activateRoot");
        QCOMPARE(call(w,"document.inspect")["activeOwnerId"].toString(),baseline["documentId"].toString());
        const auto evidence=qEnvironmentVariable("ALLOY_ASM022_EVIDENCE");
        if(!evidence.isEmpty()){QVERIFY(QDir().mkpath(evidence));call(w,"capture.screenshot",{{"path",evidence+"/browser-organized.png"}});}
        call(w,"document.new",{{"discard",true}});w.close();
    }
};
QTEST_MAIN(BrowserOrganizationControlsTest)
#include "browser_organization_controls.moc"
