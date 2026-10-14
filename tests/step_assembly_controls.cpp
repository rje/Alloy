// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include <QApplication>
#include <QDir>
#include <QJsonDocument>
#include <QTest>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>

class StepAssemblyControlsTest final:public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& input={}) {
        const auto response=window.execute(command,input);
        if(!response["ok"].toBool())qFatal("%s: %s",qPrintable(command),QJsonDocument(response).toJson().constData());
        return response["result"].toObject();
    }
private slots:
    void importedBrowserAndSelection() {
        const auto fixture=qEnvironmentVariable("ALLOY_ASSEMBLY_FIXTURE");
        if(fixture.isEmpty())QSKIP("External qualified assembly fixture unavailable");
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1500,950);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));window.activateWindow();
        const auto owner=call(window,"component.create",{{"name","Imported board"}})["definitionId"].toString();
        const auto state=call(window,"document.importStep",{{"path",fixture}});
        QCOMPARE(state["bodies"].toInt(),12);
        auto* tree=window.findChild<QTreeWidget*>("browser.tree");QVERIFY(tree);
        QTreeWidgetItem* importRoot=nullptr;int leaves=0;
        for(QTreeWidgetItemIterator it(tree);*it;++it) {
            auto* row=*it;
            if(!row->data(0,Qt::UserRole+6).toString().isEmpty())importRoot=row;
            if(!row->data(0,Qt::UserRole+1).toString().isEmpty() && !row->isHidden())++leaves;
        }
        QVERIFY(importRoot);QCOMPARE(leaves,12);
        QVERIFY(importRoot->parent() && importRoot->parent()->parent());
        QCOMPARE(importRoot->parent()->parent()->data(0,Qt::UserRole+3).toString(),owner);
        const auto bodies=state["evaluation"].toObject()["bodies"].toArray();
        const auto target=bodies[0].toObject();QTreeWidgetItem* leaf=nullptr;
        for(QTreeWidgetItemIterator it(importRoot);*it;++it)if((*it)->data(0,Qt::UserRole+1).toString()==target["bodyId"].toString()) {leaf=*it;break;}
        QVERIFY(leaf);
        for(auto* parent=leaf->parent();parent;parent=parent->parent())parent->setExpanded(true);
        tree->scrollToItem(leaf);QApplication::processEvents();
        const auto rect=tree->visualItemRect(leaf);
        QVERIFY2(rect.isValid() && tree->itemAt(rect.center())==leaf,"Imported Browser leaf must be visible at its click point");
        QTest::mouseClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,rect.center());
        QCOMPARE(window.state()["selection"].toObject()["bodyId"],target["bodyId"]);
        QCOMPARE(call(window,"selection.set",{{"bodyId",target["bodyId"]}})["occurrenceId"],target["occurrenceId"]);
        const auto output=qEnvironmentVariable("ALLOY_ASSEMBLY_EVIDENCE");
        if(!output.isEmpty()) {QDir().mkpath(output);call(window,"view.home");
            call(window,"capture.screenshot",{{"path",output+"/imported-browser.png"}});}
        call(window,"history.undo");
        QVERIFY(importRoot->isHidden());
        call(window,"history.redo");
        QVERIFY(!importRoot->isHidden());
        call(window,"document.new",{{"discard",true}});
        window.close();
    }
};
QTEST_MAIN(StepAssemblyControlsTest)
#include "step_assembly_controls.moc"
