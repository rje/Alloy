// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QTest>
#include <QAction>
#include <QLineEdit>
#include <QPushButton>
#include <QMenu>
#include <QTimer>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QJsonDocument>
#include <QFile>
#include <QDir>

class ConicControlsTest final:public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& args={}) {
        const auto response=window.execute(command,args);
        if(!response["ok"].toBool())qFatal("%s",QJsonDocument(response).toJson().constData());
        return response["result"].toObject();
    }
    QJsonObject session(AlloyWindow& window) { return window.state()["activeSession"].toObject(); }
    QJsonObject draft(AlloyWindow& window) { return session(window)["draftSketch"].toObject(); }
private slots:
    void createDragContextAcceptAndCancel() {
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1280,860);window.show();window.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        ReferenceView* view=nullptr;
        for(auto* child:window.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;
        QVERIFY(view);call(window,"sketch.create",{{"plane","XY"}});
        QApplication::setActiveWindow(&window);view->setFocus();QTest::qWait(150); // Initial harness activation only.
        QVERIFY(view->hasFocus());
        auto pixel=[&](double u,double v) { const auto p=view->projectSketchPoint(u,v,"XY");return QPoint(qRound(p["x"].toDouble()),qRound(p["y"].toDouble())); };
        auto* action=window.findChild<QAction*>("sketch.tool.conic");QVERIFY(action);action->trigger();
        const auto original=draft(window);
        for(const auto p:{QPointF(-30,-10),QPointF(30,-10),QPointF(0,35)}) {
            const auto at=pixel(p.x(),p.y());QVERIFY(view->rect().contains(at));
            QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,at);QVERIFY(view->hasFocus());
        }
        QCOMPARE(session(window)["pendingConic"].toObject()["points"].toArray().size(),3);
        QCOMPARE(draft(window),original);QVERIFY(session(window)["pendingConic"].toObject()["valid"].toBool());
        const auto handle=view->conicHandle();QVERIFY(!handle.isEmpty());
        const QPoint from(qRound(handle["x"].toDouble()),qRound(handle["y"].toDouble()));
        const auto to=from+QPoint(0,-24);
        QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,from);
        QMouseEvent move(QEvent::MouseMove,QPointF(to),QPointF(view->mapToGlobal(to)),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);
        QApplication::sendEvent(view,&move);
        const double firstRho=session(window)["pendingConic"].toObject()["rho"].toDouble();
        const auto finalTo=to+QPoint(0,-12);
        QMouseEvent secondMove(QEvent::MouseMove,QPointF(finalTo),QPointF(view->mapToGlobal(finalTo)),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);
        QApplication::sendEvent(view,&secondMove);
        QVERIFY(session(window)["pendingConic"].toObject()["rho"].toDouble()>firstRho);
        QTest::mouseRelease(view,Qt::LeftButton,Qt::NoModifier,finalTo);
        QVERIFY(view->hasFocus());QCOMPARE(draft(window),original);
        const double rho=session(window)["pendingConic"].toObject()["rho"].toDouble();QVERIFY(rho>.5 && rho<1);
        const auto evidence=qEnvironmentVariable("ALLOY_CONIC_EVIDENCE");
        if(!evidence.isEmpty()) {
            QDir().mkpath(evidence);call(window,"capture.screenshot",{{"path",evidence+"/native-rho-handle.png"}});
            QFile state(evidence+"/native-preview.json");QVERIFY(state.open(QIODevice::WriteOnly));state.write(QJsonDocument(window.state()).toJson());
        }
        bool menuVisible=false;
        QTimer::singleShot(100,&window,[&] {
            auto* menu=qobject_cast<QMenu*>(QApplication::activePopupWidget());if(!menu)return;
            menuVisible=true;for(auto* option:menu->actions())if(option->objectName()=="sketch.conic.ok")
                QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,menu->actionGeometry(option).center());
        });
        QContextMenuEvent context(QContextMenuEvent::Mouse,to,view->mapToGlobal(to));QApplication::sendEvent(view,&context);
        QVERIFY(menuVisible);QVERIFY(view->hasFocus());QVERIFY(!session(window).contains("pendingConic"));
        const auto created=draft(window);const auto entity=created["parameters"].toObject()["entities"].toArray()[0].toObject();
        QCOMPARE(entity["rho"].toDouble(),rho);
        // Existing-curve editing uses the shared editor; typed invalid input must block OK.
        call(window,"sketch.conicEdit",{{"entityId",entity["id"]}});
        auto* input=window.findChild<QLineEdit*>("sketch.conic.rho");auto* ok=window.findChild<QPushButton*>("sketch.conic.accept");
        QVERIFY(input&&input->isVisible()&&ok);QTest::mouseClick(input,Qt::LeftButton);QVERIFY(input->hasFocus());
        QTest::keySequence(input,QKeySequence::SelectAll);QTest::keyClicks(input,"1.2");
        QVERIFY(!ok->isEnabled());QTest::keyClick(input,Qt::Key_Return);QCOMPARE(draft(window),created);
        QTest::keyClick(input,Qt::Key_Escape);QVERIFY(!session(window).contains("pendingConic"));QCOMPARE(draft(window),created);
        // Cancel while a handle is held; a later release must not revive the preview.
        call(window,"sketch.conicEdit",{{"entityId",entity["id"]}});
        const auto cancelHandle=view->conicHandle();const QPoint cancelFrom(qRound(cancelHandle["x"].toDouble()),qRound(cancelHandle["y"].toDouble()));
        QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,cancelFrom);QVERIFY(view->hasFocus());
        QTest::keyClick(view,Qt::Key_Escape);QTest::mouseRelease(view,Qt::LeftButton,Qt::NoModifier,cancelFrom+QPoint(0,15));
        QVERIFY(!session(window).contains("pendingConic"));QCOMPARE(draft(window),created);
        call(window,"sketch.cancel");window.hide();
    }
};
QTEST_MAIN(ConicControlsTest)
#include "conic_controls.moc"
