// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QLabel>
#include <QListWidget>
#include <QProcess>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTest>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QWindow>

class CombineNamingRecoveryTest final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& args={}) {
        auto response=window.executeApi(command,args);
        const auto initial=response["result"].toObject();QElapsedTimer timer;timer.start();
        while(response["completion"]=="pending" && timer.elapsed()<30000) {
            QTest::qWait(25);
            response=window.executeApi("command.status",{{"session",initial["id"]},{"generation",initial["previewGeneration"]}});
        }
        if(!response["ok"].toBool())qFatal("%s: %s",qPrintable(command),QJsonDocument(response).toJson().constData());
        return response["result"].toObject();
    }
    QTreeWidgetItem* row(QTreeWidget* tree,const QString& id) {
        for(QTreeWidgetItemIterator it(tree);*it;++it)
            if((*it)->data(0,Qt::UserRole+1).toString()==id && !(*it)->isHidden())return *it;
        return nullptr;
    }
    void click(QWidget* widget, QPoint point={}) {
        QVERIFY(widget && widget->isVisible() && widget->isEnabled());
        if(point.isNull())point=widget->rect().center();
        QCOMPARE(QApplication::widgetAt(widget->mapToGlobal(point)),widget);
        QTest::mouseClick(widget,Qt::LeftButton,Qt::NoModifier,point);
    }
private slots:
    void namedRecovery() {
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1440,1000);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
#ifdef Q_OS_MACOS
        QProcess activation;activation.start("osascript",{"-e",QString("tell application \"System Events\" to set frontmost of first application process whose unix id is %1 to true").arg(QCoreApplication::applicationPid())});
        QVERIFY(activation.waitForFinished(5000));QCOMPARE(activation.exitCode(),0);
#endif
        window.raise();window.activateWindow();window.windowHandle()->requestActivate();QApplication::setActiveWindow(&window);
        QTRY_VERIFY(window.isActiveWindow()); // Initial harness activation only.
        call(window,"solid.box",{{"width",20},{"depth",10},{"height",5},{"units","mm"}});
        call(window,"solid.box",{{"width",10},{"depth",10},{"height",5},{"x",15},{"units","mm"}});
        const auto features=call(window,"document.inspect")["authored"].toObject()["features"].toArray();
        const auto target=features[0].toObject()["bodyId"].toString(),tool=features[1].toObject()["bodyId"].toString();
        call(window,"body.rename",{{"bodyId",target},{"name","Plate"}});
        call(window,"body.rename",{{"bodyId",tool},{"name","Cutter"}});
        auto* tree=window.findChild<QTreeWidget*>("browser.tree");QVERIFY(tree);
        QVERIFY(row(tree,target));QCOMPARE(row(tree,target)->text(0),QString("Plate"));
        QVERIFY(row(tree,tool));QCOMPARE(row(tree,tool)->text(0),QString("Cutter"));
        const auto named=call(window,"document.inspect");
        call(window,"command.begin",{{"command","solid.combine"},{"inputs",QJsonObject{{"targetBodyId",target},{"toolBodyIds",QJsonArray{tool}},{"operation","join"},{"keepTools",false}}}});
        auto* targetBox=window.findChild<QComboBox*>("combine.targetBody");
        auto* tools=window.findChild<QListWidget*>("combine.toolBodies");
        QVERIFY(targetBox && tools);QVERIFY(targetBox->currentText().contains("Plate"));
        bool namedTool=false;
        for(int i=0;i<tools->count();++i)if(tools->item(i)->data(Qt::UserRole).toString()==tool)
            namedTool=tools->item(i)->text().contains("Cutter") && tools->item(i)->checkState()==Qt::Checked;
        QVERIFY(namedTool);
        auto* operation=window.findChild<QComboBox*>("combine.operation");
        auto* ok=window.findChild<QPushButton*>("combine.ok");
        // Native keyboard selection follows the real clicked combo's focus.
        click(operation);QTRY_VERIFY(QApplication::activePopupWidget());
        QTest::keyClick(QApplication::focusWidget(),Qt::Key_Down);
        QTest::keyClick(QApplication::focusWidget(),Qt::Key_Return);
        QTRY_VERIFY(!QApplication::activePopupWidget());QCOMPARE(operation->currentData().toString(),QString("cut"));
        QTRY_VERIFY(ok->isEnabled());click(ok);
        auto result=call(window,"document.inspect");QCOMPARE(result["bodies"].toInt(),1);
        QVERIFY(row(tree,target));QCOMPARE(row(tree,target)->text(0),QString("Plate"));QVERIFY(!row(tree,tool));
        call(window,"history.undo");QCOMPARE(call(window,"document.inspect")["authored"],named["authored"]);
        QVERIFY(row(tree,tool));QCOMPARE(row(tree,tool)->text(0),QString("Cutter"));
        call(window,"history.redo");
        const auto combine=result["authored"].toObject()["features"].toArray().last().toObject()["id"].toString();
        call(window,"feature.editDialog",{{"featureId",combine}});
        auto* keep=window.findChild<QCheckBox*>("combine.keepTools");QVERIFY(keep && !keep->isChecked());click(keep,QPoint(10,keep->height()/2));
        QTRY_VERIFY(keep->isChecked());ok=window.findChild<QPushButton*>("combine.ok");QTRY_VERIFY(ok->isEnabled());click(ok);
        QVERIFY(row(tree,tool));QCOMPARE(row(tree,tool)->text(0),QString("Cutter"));
        QTemporaryDir temp;QVERIFY(temp.isValid());
        call(window,"document.save",{{"path",temp.path()+"/named-cut.alloy"}});
        call(window,"document.new",{{"discard",true}});
        call(window,"document.open",{{"path",temp.path()+"/named-cut.alloy"}});
        QVERIFY(row(tree,target));QCOMPARE(row(tree,target)->text(0),QString("Plate"));
        QVERIFY(row(tree,tool));QCOMPARE(row(tree,tool)->text(0),QString("Cutter"));
        call(window,"solid.box",{{"width",5},{"depth",5},{"height",5},{"x",40},{"units","mm"}});
        const auto stable=call(window,"document.inspect");
        const auto outside=stable["authored"].toObject()["features"].toArray().last().toObject()["bodyId"].toString();
        call(window,"command.begin",{{"command","solid.combine"},{"inputs",QJsonObject{{"targetBodyId",target},{"toolBodyIds",QJsonArray{outside}},{"operation","intersect"},{"keepTools",false}}}});
        auto* error=window.findChild<QLabel*>("combine.error");ok=window.findChild<QPushButton*>("combine.ok");
        QVERIFY(error && error->isVisible());QVERIFY(error->text().contains("change Operation"));QVERIFY(!ok->isEnabled());
        QCOMPARE(call(window,"document.inspect"),stable);
        QTRY_VERIFY(error->height()>=error->heightForWidth(error->width()));
        const auto evidence=qEnvironmentVariable("ALLOY_COMBINE_RECOVERY_EVIDENCE");
        if(!evidence.isEmpty()){call(window,"view.home");call(window,"capture.screenshot",{{"path",evidence+"/native-refusal.png"}});call(window,"capture.dialog",{{"path",evidence+"/native-refusal-panel.png"}});}
        operation=window.findChild<QComboBox*>("combine.operation");click(operation);QTRY_VERIFY(QApplication::activePopupWidget());
        QTest::keyClick(QApplication::focusWidget(),Qt::Key_Up);QTest::keyClick(QApplication::focusWidget(),Qt::Key_Return);
        QTRY_VERIFY(!QApplication::activePopupWidget());QCOMPARE(operation->currentData().toString(),QString("cut"));
        QTRY_VERIFY(ok->isEnabled());QVERIFY(error->text().isEmpty());click(ok);
        QCOMPARE(call(window,"document.inspect")["bodies"].toInt(),2);
        QVERIFY(row(tree,target));QCOMPARE(row(tree,target)->text(0),QString("Plate"));
        QVERIFY(row(tree,tool));QCOMPARE(row(tree,tool)->text(0),QString("Cutter"));
        if(!evidence.isEmpty())call(window,"capture.screenshot",{{"path",evidence+"/native-recovered.png"}});
        call(window,"document.new",{{"discard",true}});window.close();
    }
};
QTEST_MAIN(CombineNamingRecoveryTest)
#include "combine_naming_recovery_controls.moc"
