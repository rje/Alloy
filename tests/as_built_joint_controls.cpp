// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include <QAbstractItemView>
#include <QAbstractButton>
#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QJsonDocument>
#include <QPushButton>
#include <QTest>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>

class AsBuiltJointControlsTest final : public QObject {
    Q_OBJECT
    static QJsonObject call(AlloyWindow& window,const QString& name,const QJsonObject& args={}){
        const auto response=window.execute(name,args);
        if(!response["ok"].toBool())qFatal("%s: %s",qPrintable(name),QJsonDocument(response).toJson().constData());
        return response["result"].toObject();
    }
    static QString occurrence(const QJsonObject& authored,const QString& name){
        QString definition;
        for(const auto& value:authored["components"].toArray())if(value.toObject()["name"]==name)
            definition=value.toObject()["id"].toString();
        for(const auto& value:authored["occurrences"].toArray())if(value.toObject()["definitionId"]==definition)
            return value.toObject()["id"].toString();
        return {};
    }
    static void pickBrowser(QTreeWidget* tree,const QString& id){
        tree->expandAll();QApplication::processEvents();
        for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole+8).toString()==id){
            tree->scrollToItem(*it);QApplication::processEvents();
            auto* control=qobject_cast<QAbstractButton*>(tree->itemWidget(*it,0));
            if(!control || !control->isVisible())qFatal("Original component activation control is unavailable");
            QTest::mouseClick(control,Qt::LeftButton);return;
        }
        qFatal("Component occurrence is missing from Browser: %s",qPrintable(id));
    }
    static void choose(QComboBox* combo,int row){
        if(!combo || row<0)qFatal("Native combo choice unavailable");
        QTest::mouseClick(combo,Qt::LeftButton);
        QTRY_VERIFY(combo->view()->isVisible());
        QTest::keyClick(combo,Qt::Key_Home);
        for(int i=0;i<row;++i)QTest::keyClick(combo,Qt::Key_Down);
        QTest::keyClick(combo,Qt::Key_Return);
        QApplication::processEvents();
    }
    static QDockWidget* dock(AlloyWindow& window,const QString& name){
        for(auto* item:window.findChildren<QDockWidget*>(name))if(item->isVisible())return item;
        return nullptr;
    }
    static void chooseOrigin(AlloyWindow& window,QDockWidget* panel,const QString& comboName,const QString& prefix){
        auto* combo=panel->findChild<QComboBox*>(comboName);
        if(!combo)qFatal("Position combo unavailable");
        int choice=-1;
        for(int i=1;i<combo->count();++i)if(combo->itemText(i).startsWith(prefix)){
            choice=i;break;
        }
        if(choice<1)qFatal("Retained Position unavailable in native selector");
        const auto position=combo->itemData(choice).toJsonValue().toObject();
        if(position.isEmpty())qFatal("Retained Position selector data is empty");
        call(window,"command.update",{{"session",window.state()["activeSession"].toObject()["id"]},
            {"inputs",QJsonObject{{"position",position}}}});
    }
private slots:
    void nativeTypeComponentsAndOrigin(){
        const auto source=qEnvironmentVariable("ALLOY_AS_BUILT_SOURCE");QVERIFY(!source.isEmpty());
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1500,940);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));QApplication::setActiveWindow(&window);
        auto* tree=window.findChild<QTreeWidget*>("browser.tree");QVERIFY(tree);
        const auto evidence=qEnvironmentVariable("ALLOY_AS_BUILT_EVIDENCE");
        for(const auto& kind:{QString("revolute"),QString("cylindrical")}){
            call(window,"document.open",{{"path",source},{"discard",true}});
            const auto before=call(window,"document.inspect")["authored"].toObject();
            const auto activeOwner=call(window,"component.inspect").value("activeOwnerId").toString();
            const auto moving=occurrence(before,kind=="revolute" ? "Upper arm" : "Inner piston");
            const auto fixed=occurrence(before,kind=="revolute" ? "Lower arm" : "Outer piston");
            QVERIFY(!moving.isEmpty() && !fixed.isEmpty());
            call(window,"joint.asBuiltDialog");
            auto* panel=dock(window,"revoluteDock");QVERIFY(panel);
            auto* first=panel->findChild<QPushButton*>("asBuilt.pickComponent1");
            auto* second=panel->findChild<QPushButton*>("asBuilt.pickComponent2");
            QVERIFY(first && second);
            QTest::mouseClick(first,Qt::LeftButton);pickBrowser(tree,moving);
            QTRY_COMPARE(window.state()["activeSession"].toObject()["inputs"].toObject()["component1OccurrenceId"].toString(),moving);
            QTest::mouseClick(second,Qt::LeftButton);pickBrowser(tree,fixed);
            QTRY_COMPARE(window.state()["activeSession"].toObject()["inputs"].toObject()["component2OccurrenceId"].toString(),fixed);
            QCOMPARE(call(window,"component.inspect").value("activeOwnerId").toString(),activeOwner);
            auto session=window.state()["activeSession"].toObject();
            QCOMPARE(session["inputs"].toObject()["component1OccurrenceId"].toString(),moving);
            QCOMPARE(session["inputs"].toObject()["component2OccurrenceId"].toString(),fixed);
            auto* type=panel->findChild<QComboBox*>("asBuilt.type");QVERIFY(type);
            if(kind=="revolute"){
                choose(type,type->findData("cylindrical"));
                QTRY_VERIFY(dock(window,"cylindricalDock"));
                panel=dock(window,"cylindricalDock");type=panel->findChild<QComboBox*>("asBuilt.type");
                choose(type,type->findData("revolute"));
                QTRY_VERIFY(dock(window,"revoluteDock"));panel=dock(window,"revoluteDock");
            }else{
                choose(type,type->findData("cylindrical"));
                QTRY_VERIFY(dock(window,"cylindricalDock"));panel=dock(window,"cylindricalDock");
            }
            session=window.state()["activeSession"].toObject();
            QCOMPARE(session["inputs"].toObject()["component1OccurrenceId"].toString(),moving);
            QCOMPARE(session["inputs"].toObject()["component2OccurrenceId"].toString(),fixed);
            QVERIFY(session["inputs"].toObject()["position"].toObject().isEmpty());
            chooseOrigin(window,panel,kind=="revolute" ? "revolute.position" : "cylindrical.position",
                kind=="revolute" ? "Bore X0.0 Z80.0" : "Circle");
            session=window.state()["activeSession"].toObject();
            QVERIFY2(session["previewed"].toBool(),qPrintable(QJsonDocument(session).toJson()));
            if(!evidence.isEmpty())call(window,"capture.screenshot",{{"path",evidence+"/native-"+kind+"-preview.png"}});
            auto* buttons=panel->findChild<QDialogButtonBox*>(kind=="revolute" ? "revolute.buttons" : "cylindrical.buttons");
            QVERIFY(buttons && buttons->button(QDialogButtonBox::Ok)->isEnabled());
            QTest::mouseClick(buttons->button(QDialogButtonBox::Ok),Qt::LeftButton);
            const auto after=call(window,"document.inspect")["authored"].toObject();
            QCOMPARE(after["joints"].toArray().size(),before["joints"].toArray().size()+1);
            QCOMPARE(after["joints"].toArray().last().toObject()["type"].toString(),
                kind=="revolute" ? QString("asBuiltRevolute") : QString("asBuiltCylindrical"));
            call(window,"history.undo");QCOMPARE(call(window,"document.inspect")["authored"].toObject(),before);
        }
        window.close();
    }
};
QTEST_MAIN(AsBuiltJointControlsTest)
#include "as_built_joint_controls.moc"
