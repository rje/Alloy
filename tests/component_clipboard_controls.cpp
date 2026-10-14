// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QJsonDocument>
#include <QMenu>
#include <QPointer>
#include <QTest>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>

class ComponentClipboardControlsTest final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& args={}) {
        const auto response=window.execute(command,args);
        if(!response["ok"].toBool())qFatal("%s: %s",qPrintable(command),QJsonDocument(response).toJson().constData());
        return response["result"].toObject();
    }
    static QTreeWidgetItem* row(QTreeWidget* tree,const QString& id) {
        for(QTreeWidgetItemIterator it(tree);*it;++it)
            if((*it)->data(0,Qt::UserRole+8).toString()==id
                && !(*it)->data(0,Qt::UserRole+3).toString().isEmpty() && !(*it)->isHidden())return *it;
        return nullptr;
    }
    static QDialog* visibleCopyDialog(AlloyWindow& window) {
        for(auto* dialog:window.findChildren<QDialog*>("component.copy.dialog"))if(dialog->isVisible())return dialog;
        return nullptr;
    }
    static void browserCopy(QTreeWidget* tree,QTreeWidgetItem* item) {
        tree->scrollToItem(item);QApplication::processEvents();
        const auto point=tree->visualItemRect(item).center();
        if(!tree->viewport()->rect().contains(point))qFatal("Browser Copy row offscreen");
        QTimer::singleShot(40,tree,[]{
            auto* menu=qobject_cast<QMenu*>(QApplication::activePopupWidget());
            if(!menu)qFatal("Browser Copy context menu missing");
            for(auto* action:menu->actions())if(action->objectName()=="context.copyOccurrence"){
                QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,menu->actionGeometry(action).center());return;
            }
            qFatal("Browser Copy action missing");
        });
        QContextMenuEvent event(QContextMenuEvent::Mouse,point,tree->viewport()->mapToGlobal(point));
        QApplication::sendEvent(tree->viewport(),&event);
    }
private slots:
    void copyPastePlacementAndDocumentBoundary() {
        const auto source=qEnvironmentVariable("ALLOY_ASM019_SOURCE");
        QVERIFY(!source.isEmpty());
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1510,920);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));window.activateWindow();
        call(window,"document.open",{{"path",source},{"discard",true}});
        const auto starting=call(window,"document.inspect"),authored=starting["authored"].toObject();
        QString parentDefinition,parentOccurrence;
        for(const auto& value:authored["components"].toArray())
            if(value.toObject()["name"]=="Piston subassembly")parentDefinition=value.toObject()["id"].toString();
        for(const auto& value:authored["occurrences"].toArray())
            if(value.toObject()["definitionId"]==parentDefinition)parentOccurrence=value.toObject()["id"].toString();
        QVERIFY(!parentOccurrence.isEmpty());
        const auto driver=authored["joints"].toArray()[0].toObject()["id"].toString();
        call(window,"joint.angleSet",{{"jointId",driver},{"angleDeg",0}});
        QApplication::setActiveWindow(&window);window.setFocus();
        auto* tree=window.findChild<QTreeWidget*>("browser.tree");QVERIFY(tree);tree->expandAll();
        auto* item=row(tree,parentOccurrence);QVERIFY(item);
        browserCopy(tree,item);
        QCOMPARE(call(window,"component.clipboardInspect")["occurrenceId"].toString(),parentOccurrence);
        QTRY_VERIFY2(tree->hasFocus(),"Browser Copy must leave Browser ready for Paste shortcut");
        QTest::keyClick(tree,Qt::Key_C,Qt::ControlModifier);
        QCOMPARE(call(window,"component.clipboardInspect")["occurrenceId"].toString(),parentOccurrence);
        const auto rest=call(window,"document.inspect")["authored"].toObject();

        QTest::keyClick(tree,Qt::Key_V,Qt::ControlModifier);
        QPointer<QDialog> dialog=visibleCopyDialog(window);
        QVERIFY(dialog && dialog->isVisible());
        QVERIFY(dialog->windowTitle().contains("Copy"));
        QTest::keyClick(dialog,Qt::Key_Escape);
        QTRY_VERIFY(!dialog || !dialog->isVisible());
        QCOMPARE(call(window,"document.inspect")["authored"].toObject(),rest);

        QTest::keyClick(tree,Qt::Key_V,Qt::ControlModifier);
        dialog=visibleCopyDialog(window);QVERIFY(dialog && dialog->isVisible());
        auto* x=dialog->findChild<QDoubleSpinBox*>("component.copy.x");QVERIFY(x);
        QCOMPARE(x->value(),0.0);
        const auto oldClipboard=QApplication::clipboard()->text();
        QApplication::clipboard()->setText("12");
        QTest::mouseClick(x,Qt::LeftButton);QTest::keyClick(x,Qt::Key_A,Qt::ControlModifier);
        QTest::keyClick(x,Qt::Key_V,Qt::ControlModifier);QTest::keyClick(x,Qt::Key_Tab);
        QCOMPARE(x->value(),12.0);
        QApplication::clipboard()->setText(oldClipboard);
        QTest::mouseClick(x,Qt::LeftButton);QTest::keyClick(x,Qt::Key_A,Qt::ControlModifier);
        QTest::keyClicks(x,"-180");QCOMPARE(x->value(),-180.0);
        const auto evidence=qEnvironmentVariable("ALLOY_ASM019_EVIDENCE");
        if(!evidence.isEmpty())call(window,"capture.screenshot",{{"path",evidence+"/native-paste-placement.png"}});
        QTest::keyClick(x,Qt::Key_Return);
        QTRY_VERIFY2(!dialog || !dialog->isVisible(),"Enter must commit the typed placement");
        const auto result=call(window,"document.inspect")["authored"].toObject();
        QCOMPARE(result["joints"].toArray().size(),rest["joints"].toArray().size()+1);
        for(int i=0;i<rest["joints"].toArray().size();++i)
            QCOMPARE(result["joints"].toArray()[i],rest["joints"].toArray()[i]);
        QCOMPARE(result["components"],rest["components"]);
        QCOMPARE(call(window,"component.clipboardInspect")["occurrenceId"].toString(),parentOccurrence);
        if(!evidence.isEmpty())call(window,"capture.screenshot",{{"path",evidence+"/native-pasted-piston.png"}});

        call(window,"document.new",{{"discard",true}});
        const auto stale=window.execute("component.pasteOccurrence",{{"placement",
            QJsonObject{{"axis","Z"},{"angleDeg",0},{"translationMm",QJsonArray{0,0,0}}}}});
        QVERIFY(!stale["ok"].toBool());
        QCOMPARE(stale["error"].toObject()["code"].toString(),"clipboard_document_mismatch");
        window.close();
    }
};
QTEST_MAIN(ComponentClipboardControlsTest)
#include "component_clipboard_controls.moc"
