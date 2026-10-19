// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include <QAction>
#include <QApplication>
#include <QContextMenuEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QJsonDocument>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QTest>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>

class BodyPropertiesControlsTest final: public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& args={}) {
        const auto response=window.execute(command,args);
        if(!response.value("ok").toBool())qFatal("%s",QJsonDocument(response).toJson().constData());
        return response.value("result").toObject();
    }
private slots:
    void browserPropertiesFollowUnitsAndEdit() {
        AlloyTheme::apply(*qApp);
        AlloyWindow window;window.resize(1400,900);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        call(window,"solid.box",{{"width",25.4},{"depth",12.7},{"height",6.35},{"units","mm"},{"name","Asymmetric stock"}});
        const auto before=call(window,"document.inspect");
        const auto body=before.value("evaluation").toObject().value("bodies").toArray().first().toObject();
        const auto bodyId=body.value("bodyId").toString();
        const auto feature=before.value("authored").toObject().value("features").toArray().first().toObject();
        auto* tree=window.findChild<QTreeWidget*>("browser.tree");QVERIFY(tree);tree->expandAll();
        QTreeWidgetItem* row=nullptr;
        for(QTreeWidgetItemIterator it(tree);*it;++it)
            if((*it)->data(0,Qt::UserRole+1).toString()==bodyId){row=*it;break;}
        QVERIFY(row);tree->scrollToItem(row);QApplication::processEvents();
        const auto point=tree->visualItemRect(row).center();
        QVERIFY(tree->viewport()->rect().contains(point));
        QCOMPARE(QApplication::widgetAt(tree->viewport()->mapToGlobal(point)),tree->viewport());
        QContextMenuEvent event(QContextMenuEvent::Mouse,point,tree->viewport()->mapToGlobal(point));
        QApplication::sendEvent(tree->viewport(),&event);
        auto* menu=qobject_cast<QMenu*>(QApplication::activePopupWidget());QVERIFY(menu);
        QCOMPARE(menu->objectName(),QString("context.browserBody"));
        auto* properties=menu->findChild<QAction*>("context.bodyProperties");QVERIFY(properties);
        QVERIFY(properties->isEnabled());
        QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,menu->actionGeometry(properties).center());
        QPointer<QDialog> dialog;
        QTRY_VERIFY((dialog=window.findChild<QDialog*>("body.properties.dialog"))&&dialog->isVisible());
        QTRY_VERIFY(!QApplication::activePopupWidget());
        QCOMPARE(window.state().value("selection").toObject().value("bodyId"),QJsonValue(bodyId));
        const auto label=[&](const char* name){auto* found=dialog->findChild<QLabel*>(name);return found?found->text():QString{};};
        QCOMPARE(label("body.properties.body"),QString("Asymmetric stock"));
        QCOMPARE(label("body.properties.length"),QString("25.4 mm"));
        QCOMPARE(label("body.properties.width"),QString("12.7 mm"));
        QCOMPARE(label("body.properties.height"),QString("6.35 mm"));
        QVERIFY(label("body.properties.area").endsWith(" mm²"));
        QVERIFY(label("body.properties.volume").endsWith(" mm³"));
        const auto evidence=qEnvironmentVariable("ALLOY_BODY_PROPERTIES_EVIDENCE");
        if(!evidence.isEmpty()){QVERIFY(QDir().mkpath(evidence));QVERIFY(dialog->grab().save(evidence+"/properties-mm.png"));}
        call(window,"document.units",{{"units","in"}});
        QCOMPARE(label("body.properties.length"),QString("1 in"));
        QCOMPARE(label("body.properties.width"),QString("0.5 in"));
        QCOMPARE(label("body.properties.height"),QString("0.25 in"));
        QVERIFY(label("body.properties.area").endsWith(" in²"));
        QVERIFY(label("body.properties.volume").endsWith(" in³"));
        if(!evidence.isEmpty())QVERIFY(dialog->grab().save(evidence+"/properties-in.png"));
        call(window,"solid.box",{{"featureId",feature.value("id")},{"width",50.8},{"depth",12.7},
            {"height",6.35},{"units","mm"}});
        QCOMPARE(label("body.properties.length"),QString("2 in"));
        auto* close=dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Close);QVERIFY(close);
        QCOMPARE(QApplication::widgetAt(close->mapToGlobal(close->rect().center())),static_cast<QWidget*>(close));
        QTest::mouseClick(close,Qt::LeftButton);
        QTRY_VERIFY(!dialog || !dialog->isVisible());
        call(window,"component.createFromBodies",{{"bodyIds",QJsonArray{bodyId}}});
        const auto component=call(window,"document.inspect").value("authored").toObject();
        const auto originalOccurrence=component.value("occurrences").toArray().first().toObject().value("id").toString();
        const auto copy=call(window,"component.copyOccurrence",{{"occurrenceId",originalOccurrence},
            {"placement",QJsonObject{{"axis","Z"},{"angleDeg",0},{"translationMm",QJsonArray{100,0,0}}}}});
        const auto copiedOccurrence=copy.value("rootOccurrenceId").toString();
        tree->expandAll();QTreeWidgetItem* linked=nullptr;
        for(QTreeWidgetItemIterator it(tree);*it;++it)
            if((*it)->data(0,Qt::UserRole+1).toString()==bodyId
                && (*it)->data(0,Qt::UserRole+8).toString()==copiedOccurrence
                && (*it)->parent() && (*it)->parent()->data(0,Qt::UserRole+4)=="instanceBodies"){
                linked=*it;break;
            }
        QVERIFY(linked);tree->scrollToItem(linked);QApplication::processEvents();
        const auto linkedPoint=tree->visualItemRect(linked).center();
        QVERIFY(tree->viewport()->rect().contains(linkedPoint));
        QContextMenuEvent linkedEvent(QContextMenuEvent::Mouse,linkedPoint,tree->viewport()->mapToGlobal(linkedPoint));
        QApplication::sendEvent(tree->viewport(),&linkedEvent);
        auto* instanceMenu=qobject_cast<QMenu*>(QApplication::activePopupWidget());QVERIFY(instanceMenu);
        QCOMPARE(instanceMenu->objectName(),QString("context.browserInstanceBody"));
        auto* instanceProperties=instanceMenu->findChild<QAction*>("context.bodyProperties");QVERIFY(instanceProperties);
        QTest::mouseClick(instanceMenu,Qt::LeftButton,Qt::NoModifier,
            instanceMenu->actionGeometry(instanceProperties).center());
        QTRY_VERIFY((dialog=window.findChild<QDialog*>("body.properties.dialog"))&&dialog->isVisible());
        QCOMPARE(window.state().value("selection").toObject().value("occurrenceId"),QJsonValue(copiedOccurrence));
        QCOMPARE(label("body.properties.occurrence"),copiedOccurrence);
        call(window,"history.undo");
        QCOMPARE(label("body.properties.length"),QString("—"));
        QVERIFY(label("body.properties.status").contains("no longer available"));
        close=dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Close);
        QTest::mouseClick(close,Qt::LeftButton);
        QTRY_VERIFY(!dialog || !dialog->isVisible());
    }
};
QTEST_MAIN(BodyPropertiesControlsTest)
#include "body_properties_controls.moc"
