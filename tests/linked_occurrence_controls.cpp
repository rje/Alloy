// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QContextMenuEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QJsonDocument>
#include <QMenu>
#include <QPointer>
#include <QPushButton>
#include <QTest>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>

class LinkedOccurrenceControlsTest final:public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& args={}){
        const auto response=window.execute(command,args);
        if(!response["ok"].toBool())qFatal("%s: %s",qPrintable(command),QJsonDocument(response).toJson().constData());
        return response["result"].toObject();
    }
    QTreeWidgetItem* row(QTreeWidget* tree,const QString& id,int role){
        for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,role).toString()==id && !(*it)->isHidden())return *it;
        return nullptr;
    }
    void copyAction(AlloyWindow& window,QTreeWidget* tree,QTreeWidgetItem* item){
        QVERIFY(item);tree->scrollToItem(item);QApplication::processEvents();
        const auto point=tree->visualItemRect(item).center();QVERIFY(tree->viewport()->rect().contains(point));
        QTimer::singleShot(40,&window,[]{auto* menu=qobject_cast<QMenu*>(QApplication::activePopupWidget());
            if(!menu)qFatal("Component context menu unavailable");
            for(auto* action:menu->actions())if(action->objectName()=="context.copyComponent"){
                QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,menu->actionGeometry(action).center());return;
            }qFatal("Copy component action unavailable");});
        QContextMenuEvent event(QContextMenuEvent::Mouse,point,tree->viewport()->mapToGlobal(point));
        QApplication::sendEvent(tree->viewport(),&event);
    }
private slots:
    void copyAndPickBothInstances(){
        const auto source=qEnvironmentVariable("ALLOY_LINKED_SOURCE");QVERIFY(!source.isEmpty());
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1510,920);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));window.activateWindow();
        call(window,"document.open",{{"path",source},{"discard",true}});
        const auto before=call(window,"document.inspect");const auto occurrences=before["authored"].toObject()["occurrences"].toArray();
        QString parentId,childId,childDefinition;
        for(const auto& value:occurrences){const auto r=value.toObject();
            if(r["definitionId"]=="740a1389-b1da-4c11-b8fd-856cb7745fe4")parentId=r["id"].toString();
            if(r["definitionId"]=="ef562c10-ef8a-4148-93d0-5241769b0d84"){
                childId=r["id"].toString();childDefinition=r["definitionId"].toString();}}
        QVERIFY(!parentId.isEmpty() && !childId.isEmpty());
        auto* tree=window.findChild<QTreeWidget*>("browser.tree");QVERIFY(tree);tree->expandAll();
        copyAction(window,tree,row(tree,parentId,Qt::UserRole+8));
        QPointer<QDialog> dialog=window.findChild<QDialog*>("component.copy.dialog");QVERIFY(dialog && dialog->isVisible());
        QCOMPARE(dialog->findChild<QDoubleSpinBox*>("component.copy.x")->value(),0.0);
        const auto authored=before["authored"];
        QTest::keyClick(dialog,Qt::Key_Escape);QTRY_VERIFY(!dialog || !dialog->isVisible());
        QCOMPARE(call(window,"document.inspect")["authored"],authored);
        copyAction(window,tree,row(tree,parentId,Qt::UserRole+8));
        dialog=window.findChild<QDialog*>("component.copy.dialog");QVERIFY(dialog && dialog->isVisible());
        auto* x=dialog->findChild<QDoubleSpinBox*>("component.copy.x");QVERIFY(x);
        QTest::mouseClick(x,Qt::LeftButton);QTest::keyClick(x,Qt::Key_A,Qt::ControlModifier);
        QTest::keyClicks(x,"-180");QTest::keyClick(x,Qt::Key_Tab);QCOMPARE(x->value(),-180.0);
        const auto evidence=qEnvironmentVariable("ALLOY_LINKED_EVIDENCE");
        if(!evidence.isEmpty()){QVERIFY(QDir().mkpath(evidence));QVERIFY(dialog->grab().save(evidence+"/copy-placement.png"));}
        QTest::mouseClick(dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok),Qt::LeftButton);
        QTRY_VERIFY(!dialog || !dialog->isVisible());
        const auto after=call(window,"component.inspect")["localInstances"].toArray();
        QString copiedParent,copiedChild,bodyId;
        for(const auto& value:after){const auto r=value.toObject();if(r["definitionId"]=="740a1389-b1da-4c11-b8fd-856cb7745fe4" && r["occurrenceId"]!=parentId)copiedParent=r["occurrenceId"].toString();
            if(r["definitionId"]==childDefinition && r["occurrenceId"]!=childId){copiedChild=r["occurrenceId"].toString();bodyId=r["bodies"].toArray()[0].toObject()["bodyId"].toString();}}
        QVERIFY(!copiedParent.isEmpty() && !copiedChild.isEmpty() && !bodyId.isEmpty());
        auto* copiedParentRow=row(tree,copiedParent,Qt::UserRole+8);auto* copiedChildRow=row(tree,copiedChild,Qt::UserRole+8);
        QVERIFY(copiedParentRow && copiedChildRow && copiedChildRow->parent()==copiedParentRow);
        QVERIFY(copiedChildRow->text(0).contains("Outer piston"));
        int sourceRows=0,copyRows=0;
        for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole+1).toString()==bodyId && !(*it)->isHidden()){
            sourceRows+=(*it)->data(0,Qt::UserRole+8).toString()==childId;
            copyRows+=(*it)->data(0,Qt::UserRole+8).toString()==copiedChild;
            if((*it)->data(0,Qt::UserRole+8).toString()==copiedChild)QVERIFY(!((*it)->flags()&Qt::ItemIsUserCheckable));
        }
        QCOMPARE(sourceRows,1);QCOMPARE(copyRows,1);
        call(window,"view.home");call(window,"view.orientation",{{"name","top"}});
        call(window,"selection.priority",{{"mode","body"}});
        ReferenceView* viewport=nullptr;for(auto* child:window.findChildren<QWidget*>())
            if(auto* found=dynamic_cast<ReferenceView*>(child)){viewport=found;break;}QVERIFY(viewport);
        for(const auto& instance:{childId,copiedChild}){
            const auto point=call(window,"view.projectBody",{{"bodyId",bodyId},{"occurrenceId",instance}});
            const auto pixel=QPoint(qRound(point["x"].toDouble()),qRound(point["y"].toDouble()));
            QVERIFY(viewport->rect().contains(pixel));QTest::mouseClick(viewport,Qt::LeftButton,Qt::NoModifier,pixel);
            const auto selected=call(window,"app.state")["selection"].toObject();
            QCOMPARE(selected["bodyId"].toString(),bodyId);QCOMPARE(selected["occurrenceId"].toString(),instance);
        }
        const auto guarded=window.execute("solid.moveDialog",{});QVERIFY(!guarded["ok"].toBool());
        QCOMPARE(guarded["error"].toObject()["code"].toString(),"evaluation_failed");
        const auto current=call(window,"document.inspect");QJsonObject face;
        for(const auto& value:current["evaluation"].toObject()["bodies"].toArray())if(value.toObject()["bodyId"]==bodyId){
            for(const auto& candidate:value.toObject()["supportFaces"].toArray())if(candidate.toObject()["planar"].toBool()){
                face=candidate.toObject();break;}
            break;
        }
        QVERIFY(!face.isEmpty());QJsonObject faceRef{{"bodyId",bodyId},{"faceId",face["faceId"]},
            {"producerFeatureId",face["producerFeatureId"]},{"occurrenceId",copiedChild}};
        const auto picked=call(window,"selection.faceSet",{{"support",faceRef}});QVERIFY(picked["readOnly"].toBool());
        const auto sketch=window.execute("sketch.create",{});QVERIFY(!sketch["ok"].toBool());
        QCOMPARE(sketch["error"].toObject()["code"].toString(),"unsupported_instance_edit");
        QCOMPARE(call(window,"document.inspect")["authored"],current["authored"]);
        call(window,"selection.set",{{"bodyId",bodyId},{"occurrenceId",copiedChild}});
        tree->expandAll();tree->scrollToItem(copiedParentRow,QAbstractItemView::PositionAtCenter);
        call(window,"view.orientation",{{"name","isometric"}});QApplication::processEvents();
        if(!evidence.isEmpty())call(window,"capture.screenshot",{{"path",evidence+"/linked-browser-and-viewport.png"}});
        call(window,"document.new",{{"discard",true}});window.close();
    }
};
QTEST_MAIN(LinkedOccurrenceControlsTest)
#include "linked_occurrence_controls.moc"
