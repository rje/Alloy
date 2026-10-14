// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include <QApplication>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QJsonDocument>
#include <QMenu>
#include <QPointer>
#include <QPushButton>
#include <QTest>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
class PistonComponentsControlsTest final:public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& args={}){
        const auto result=window.execute(command,args);if(!result["ok"].toBool())qFatal("%s",QJsonDocument(result).toJson().constData());
        return result["result"].toObject();
    }
    QTreeWidgetItem* row(QTreeWidget* tree,const QString& id,int role){
        for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,role).toString()==id && !(*it)->isHidden())return *it;
        return nullptr;
    }
    void action(AlloyWindow& window,QTreeWidget* tree,QTreeWidgetItem* item,const QString& name){
        QVERIFY(item);tree->scrollToItem(item);QApplication::processEvents();
        const auto point=tree->visualItemRect(item).center();QVERIFY(tree->viewport()->rect().contains(point));
        QTimer::singleShot(40,&window,[name]{auto* menu=qobject_cast<QMenu*>(QApplication::activePopupWidget());
            if(!menu)qFatal("Browser menu unavailable");for(auto* entry:menu->actions())if(entry->objectName()==name){
                QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,menu->actionGeometry(entry).center());return;}
            qFatal("Browser action unavailable");});
        QContextMenuEvent event(QContextMenuEvent::Mouse,point,tree->viewport()->mapToGlobal(point));
        QApplication::sendEvent(tree->viewport(),&event);
    }
private slots:
    void nativeConversionAndHierarchy(){
        const auto source=qEnvironmentVariable("ALLOY_PISTON_SOURCE");QVERIFY(!source.isEmpty());
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1500,900);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));window.activateWindow();
        call(window,"document.open",{{"path",source},{"discard",true}});
        const auto initial=call(window,"document.inspect");const auto bodies=initial["evaluation"].toObject()["bodies"].toArray();QCOMPARE(bodies.size(),4);
        auto* tree=window.findChild<QTreeWidget*>("browser.tree");QVERIFY(tree);tree->expandAll();
        auto selectFour=[&]{for(int i:{0,3}){auto* item=row(tree,bodies[i].toObject()["bodyId"].toString(),Qt::UserRole+1);QVERIFY(item);
            tree->scrollToItem(item);QApplication::processEvents();const auto point=tree->visualItemRect(item).center();
            QVERIFY(tree->viewport()->rect().contains(point));QTest::mouseClick(tree->viewport(),Qt::LeftButton,i ? Qt::ShiftModifier : Qt::NoModifier,point);
        }};selectFour();
        QCOMPARE(tree->selectedItems().size(),4);
        action(window,tree,row(tree,bodies[3].toObject()["bodyId"].toString(),Qt::UserRole+1),"context.createComponentsFromBodies");
        QPointer<QDialog> dialog=window.findChild<QDialog*>("component.convert.dialog");QVERIFY(dialog && dialog->isVisible());
        const auto baseline=initial["authored"];
        QTest::keyClick(dialog,Qt::Key_Escape);QTRY_VERIFY(!dialog || !dialog->isVisible());
        QCOMPARE(call(window,"document.inspect")["authored"],baseline);
        selectFour();QCOMPARE(tree->selectedItems().size(),4);
        action(window,tree,row(tree,bodies[3].toObject()["bodyId"].toString(),Qt::UserRole+1),"context.createComponentsFromBodies");
        dialog=window.findChild<QDialog*>("component.convert.dialog");QVERIFY(dialog && dialog->isVisible());
        const auto evidence=qEnvironmentVariable("ALLOY_PISTON_EVIDENCE");
        if(!evidence.isEmpty()){QVERIFY(QDir().mkpath(evidence));QVERIFY(dialog->grab().save(evidence+"/convert-four-dialog.png"));}
        QTest::mouseClick(dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok),Qt::LeftButton);
        QTRY_VERIFY(!dialog || !dialog->isVisible());
        const auto converted=call(window,"document.inspect");const auto components=converted["authored"].toObject()["components"].toArray();
        QCOMPARE(components.size(),4);QCOMPARE(converted["evaluation"].toObject()["bodies"].toArray().size(),4);
        const auto parent=call(window,"component.create",{{"name","Piston subassembly"},{"activate",false}});
        const auto parentId=parent["definitionId"].toString();
        for(int i=2;i<4;++i){auto* item=row(tree,components[i].toObject()["id"].toString(),Qt::UserRole+3);QVERIFY(item);
            action(window,tree,item,"context.reparentComponent");
            dialog=window.findChild<QDialog*>("component.move.dialog");QVERIFY(dialog && dialog->isVisible());
            auto* chooser=dialog->findChild<QComboBox*>("component.move.parent");QVERIFY(chooser);
            QTest::mouseClick(chooser,Qt::LeftButton);QTest::keyClick(chooser,Qt::Key_End);QTest::keyClick(chooser,Qt::Key_Return);
            QCOMPARE(chooser->currentData().toString(),parentId);
            QTest::mouseClick(dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok),Qt::LeftButton);
            QTRY_VERIFY(!dialog || !dialog->isVisible());
        }
        const auto grouped=call(window,"document.inspect");const auto occurrences=grouped["authored"].toObject()["occurrences"].toArray();
        QCOMPARE(occurrences[2].toObject()["parentId"].toString(),parentId);
        QCOMPARE(occurrences[3].toObject()["parentId"].toString(),parentId);
        auto* parentRow=row(tree,parentId,Qt::UserRole+3);QVERIFY(parentRow);tree->expandAll();tree->scrollToItem(parentRow,QAbstractItemView::PositionAtTop);
        QVERIFY(row(tree,components[2].toObject()["id"].toString(),Qt::UserRole+3)->parent()==parentRow);
        QVERIFY(row(tree,components[3].toObject()["id"].toString(),Qt::UserRole+3)->parent()==parentRow);
        if(!evidence.isEmpty())call(window,"capture.screenshot",{{"path",evidence+"/nested-component-browser.png"}});
        call(window,"document.new",{{"discard",true}});window.close();
    }
};
QTEST_MAIN(PistonComponentsControlsTest)
#include "piston_components_controls.moc"
