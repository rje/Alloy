// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QJsonDocument>
#include <QLabel>
#include <QMenu>
#include <QProcess>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QToolButton>
#include <QWindow>
class InterferenceControls final:public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& id,const QJsonObject& args={}){const auto response=window.executeApi(id,args);if(!response.value("ok").toBool())qFatal("%s",QJsonDocument(response).toJson().constData());return response.value("result").toObject();}
    void click(QWidget* widget,const QPoint& point){QVERIFY(widget);QCOMPARE(QApplication::widgetAt(widget->mapToGlobal(point)),widget);QTest::mouseClick(widget,Qt::LeftButton,Qt::NoModifier,point);}
private slots:
    void nativePairComputeClearClose(){
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1400,900);window.show();QVERIFY(QTest::qWaitForWindowExposed(&window));
        call(window,"solid.box",{{"width",10},{"depth",10},{"height",10},{"name","Stock"},{"units","mm"}});
        call(window,"solid.box",{{"width",4},{"depth",4},{"height",4},{"x",8},{"y",2},{"z",3},{"name","Insert"},{"units","mm"}});
        call(window,"view.orientation",{{"name","top"}});call(window,"view.fit");QTemporaryDir saved;call(window,"document.save",{{"path",saved.path()+"/pair.alloy"}});
        const auto before=call(window,"document.inspect");const auto bodies=before.value("evaluation").toObject().value("bodies").toArray();QCOMPARE(bodies.size(),2);
        const auto first=bodies[0].toObject().value("bodyId"),second=bodies[1].toObject().value("bodyId");
        ReferenceView* view=nullptr;for(auto* child:window.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;QVERIFY(view);
#ifdef Q_OS_MACOS
        QProcess activation;activation.start("osascript",{"-e",QString("tell application \"System Events\" to set frontmost of first application process whose unix id is %1 to true").arg(QCoreApplication::applicationPid())});QVERIFY(activation.waitForFinished(5000));QCOMPARE(activation.exitCode(),0);
#endif
        // Initial harness activation only; all following focus belongs to app input.
        window.raise();window.activateWindow();window.windowHandle()->requestActivate();QApplication::setActiveWindow(&window);QTRY_VERIFY(window.isActiveWindow());
        QPointer<QToolButton> inspect=window.findChild<QToolButton*>("solid.menu.inspect");QVERIFY(inspect);QPointer<QMenu> menu=inspect->menu();QVERIFY(menu);
        auto* action=window.findChild<QAction*>("solid.action.inspect.interference");QVERIFY(action);bool ready=false;
        QTimer::singleShot(150,&window,[&]{ready=menu && menu->isVisible() && menu->isActiveWindow() && QApplication::activePopupWidget()==menu;if(ready)click(menu,menu->actionGeometry(action).center());else if(menu)menu->close();});
        QCOMPARE(QApplication::widgetAt(inspect->mapToGlobal(inspect->rect().center())),static_cast<QWidget*>(inspect));QTest::mousePress(inspect,Qt::LeftButton);QVERIFY(ready);QTRY_VERIFY(!menu || !menu->isVisible());
        QPointer<QDialog> dialog=window.findChild<QDialog*>("interference.dialog");QVERIFY(dialog);QTRY_VERIFY(dialog->isVisible());
        const auto p1=view->projectWorldPoint({2,5,10}).toPoint(),p2=view->projectWorldPoint({11,4,7}).toPoint();
        QCOMPARE(view->pickBodyReference(p1.x(),p1.y()).value("bodyId"),first);click(view,p1);
        auto state=call(window,"inspect.interferenceState");QCOMPARE(state.value("sources").toArray().size(),1);QCOMPARE(state.value("sources").toArray()[0].toObject().value("bodyId"),first);QVERIFY(QApplication::activeWindow()==dialog || QApplication::activeWindow()==&window);
        qInfo()<<"After canvas pick, active window / keyboard focus:"<<QApplication::activeWindow()->objectName()<<(QApplication::focusWidget()?QApplication::focusWidget()->objectName():QString("none"));
        QCOMPARE(view->pickBodyReference(p2.x(),p2.y()).value("bodyId"),second);click(view,p2);
        state=call(window,"inspect.interferenceState");QCOMPARE(state.value("sources").toArray().size(),2);QCOMPARE(state.value("sources").toArray()[1].toObject().value("bodyId"),second);QVERIFY(QApplication::activeWindow()==dialog || QApplication::activeWindow()==&window);
        qInfo()<<"After canvas pick, active window / keyboard focus:"<<QApplication::activeWindow()->objectName()<<(QApplication::focusWidget()?QApplication::focusWidget()->objectName():QString("none"));
        auto* compute=dialog->findChild<QPushButton*>("interference.compute");QVERIFY(compute);click(compute,compute->rect().center());
        auto* result=dialog->findChild<QLabel*>("interference.result");QVERIFY(result);QVERIFY(result->text().contains("32 mm³"));
        state=call(window,"inspect.interferenceState");QCOMPARE(state.value("result").toObject().value("kind"),QJsonValue("overlap"));
        QCOMPARE(call(window,"document.inspect").value("authored"),before.value("authored"));
        const auto evidence=qEnvironmentVariable("ALLOY_INTERFERENCE_EVIDENCE");if(!evidence.isEmpty()){QVERIFY(QDir().mkpath(evidence));QVERIFY(window.grab().save(evidence+"/native-overlap.png"));QVERIFY(dialog->grab().save(evidence+"/native-panel.png"));}
        auto* clear=dialog->findChild<QPushButton*>("interference.clear");click(clear,clear->rect().center());QCOMPARE(call(window,"inspect.interferenceState").value("sources").toArray().size(),0);QVERIFY(result->text().contains("Compute"));
        auto* close=dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Close);click(close,close->rect().center());QTRY_VERIFY(!dialog || !dialog->isVisible());QTRY_VERIFY(view->hasFocus());
        QCOMPARE(call(window,"document.inspect").value("authored"),before.value("authored"));QVERIFY(!call(window,"inspect.interferenceState").value("active").toBool());
    }
};
QTEST_MAIN(InterferenceControls)
#include "interference_controls.moc"
