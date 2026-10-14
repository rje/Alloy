// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QDir>
#include <QJsonDocument>
#include <QMouseEvent>
#include <QPushButton>
#include <QTest>
#include <QWindow>
#include <cmath>

class NavigationFreeOrbitControls final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& args={}) {
        const auto response=window.executeApi(command,args);
        if(!response.value("ok").toBool())qFatal("%s",QJsonDocument(response).toJson(QJsonDocument::Compact).constData());
        return response.value("result").toObject();
    }
    static QJsonObject camera(AlloyWindow& window) {
        return window.state().value("viewport").toObject().value("gpu").toObject().value("camera").toObject();
    }
    static double distance(const QJsonArray& a,const QJsonArray& b) {
        double sum=0;for(int i=0;i<3;++i)sum+=std::pow(a[i].toDouble()-b[i].toDouble(),2);return std::sqrt(sum);
    }
    static void pointer(QWidget* view,QEvent::Type type,const QPoint& pos,Qt::MouseButton button,
        Qt::MouseButtons buttons,Qt::KeyboardModifiers modifiers) {
        QMouseEvent event(type,QPointF(pos),QPointF(view->mapToGlobal(pos)),button,buttons,modifiers);
        QApplication::sendEvent(view,&event);
    }
private slots:
    void actualFreeOrbitAndVisibleFallback() {
        const auto out=qEnvironmentVariable("ALLOY_NAV_ORBIT_EVIDENCE");QVERIFY(!out.isEmpty());QVERIFY(QDir().mkpath(out));
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1360,850);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        ReferenceView* view=nullptr;for(auto* child:window.findChildren<QWidget*>())if(auto* found=dynamic_cast<ReferenceView*>(child))view=found;
        QVERIFY(view);window.raise();window.activateWindow();window.windowHandle()->requestActivate();
        QTRY_VERIFY(window.isActiveWindow());
        call(window,"sketch.begin",{{"plane","XY"},{"units","mm"}});
        call(window,"sketch.rectangle",{{"x1",0},{"y1",0},{"x2",40},{"y2",20},{"units","mm"}});
        call(window,"sketch.finish");
        const auto sketch=call(window,"document.inspect").value("evaluation").toObject().value("sketches").toArray().first().toObject();
        call(window,"solid.extrude",{{"sketchId",sketch.value("sketchId")},{"profileId",sketch.value("profiles").toArray().first().toObject().value("profileId")},{"distance",12},{"units","mm"}});
        const auto body=call(window,"document.inspect").value("evaluation").toObject().value("bodies").toArray().first().toObject();
        call(window,"selection.set",{{"bodyId",body.value("bodyId")}});
        call(window,"view.home");
        const auto document=call(window,"document.inspect");const auto original=document.value("authored");
        const auto history=document.value("undo");const auto revision=document.value("revision");
        const auto selection=window.state().value("selection");
        const auto before=camera(window);const auto center=before.value("center").toArray();
        const QPoint start(view->width()/2,view->height()/2),end(start+QPoint(110,65));
        QCOMPARE(QApplication::widgetAt(view->mapToGlobal(start)),static_cast<QWidget*>(view));
        pointer(view,QEvent::MouseButtonPress,start,Qt::MiddleButton,Qt::MiddleButton,Qt::NoModifier);
        pointer(view,QEvent::MouseMove,start+QPoint(18,12),Qt::NoButton,Qt::MiddleButton,Qt::NoModifier);
        pointer(view,QEvent::MouseButtonRelease,start+QPoint(18,12),Qt::MiddleButton,Qt::NoButton,Qt::NoModifier);
        QVERIFY(distance(before.value("direction").toArray(),camera(window).value("direction").toArray())<1e-8);
        call(window,"view.home");
        pointer(view,QEvent::MouseButtonPress,start,Qt::MiddleButton,Qt::MiddleButton,Qt::ShiftModifier);
        QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
        pointer(view,QEvent::MouseMove,end,Qt::NoButton,Qt::MiddleButton,Qt::ShiftModifier);
        pointer(view,QEvent::MouseButtonRelease,end,Qt::MiddleButton,Qt::NoButton,Qt::ShiftModifier);
        const auto first=camera(window);
        QVERIFY(distance(before.value("direction").toArray(),first.value("direction").toArray())>0.1);
        QVERIFY(distance(center,first.value("center").toArray())<1e-6);
        QCOMPARE(window.state().value("viewport").toObject().value("orientation"),QJsonValue("custom"));
        call(window,"capture.screenshot",{{"path",out+"/shift-middle-free-orbit.png"}});

        auto* tool=window.findChild<QPushButton*>("view.orbitTool");QVERIFY(tool);QTest::mouseClick(tool,Qt::LeftButton);
        QVERIFY(tool->isChecked());const QPoint second(start+QPoint(-80,-35));
        QCOMPARE(QApplication::widgetAt(view->mapToGlobal(start)),static_cast<QWidget*>(view));
        pointer(view,QEvent::MouseButtonPress,start,Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);
        QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
        pointer(view,QEvent::MouseMove,second,Qt::NoButton,Qt::LeftButton,Qt::NoModifier);
        pointer(view,QEvent::MouseButtonRelease,second,Qt::LeftButton,Qt::NoButton,Qt::NoModifier);
        const auto secondCamera=camera(window);
        QVERIFY(distance(first.value("direction").toArray(),secondCamera.value("direction").toArray())>0.1);
        QCOMPARE(call(window,"document.inspect").value("authored"),original);
        QCOMPARE(window.state().value("selection"),selection);
        call(window,"capture.screenshot",{{"path",out+"/visible-free-orbit.png"}});
        QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QTest::keyClick(view,Qt::Key_Escape);
        QVERIFY(!tool->isChecked());

        qInfo("orbit gestures and Escape passed");
        const auto tab=call(window,"document.tabs").value("activeTabId").toString();QVERIFY(!tab.isEmpty());
        call(window,"document.tabNew");call(window,"document.tabActivate",{{"tabId",tab}});
        qInfo("custom camera tab restoration passed");
        QCOMPARE(window.state().value("viewport").toObject().value("orientation"),QJsonValue("custom"));
        QVERIFY(distance(secondCamera.value("direction").toArray(),camera(window).value("direction").toArray())<1e-8);
        call(window,"view.home");QCOMPARE(window.state().value("viewport").toObject().value("orientation"),QJsonValue("isometric"));
        QVERIFY(distance(before.value("direction").toArray(),camera(window).value("direction").toArray())<1e-8);
        QCOMPARE(call(window,"document.inspect").value("authored"),original);
        QCOMPARE(call(window,"document.inspect").value("undo"),history);
        QCOMPARE(call(window,"document.inspect").value("revision"),revision);
        QCOMPARE(window.state().value("selection"),selection);
        call(window,"document.save",{{"path",out+"/asymmetric-orbit.alloy"}});
        window.close();
    }
};
QTEST_MAIN(NavigationFreeOrbitControls)
#include "navigation_free_orbit_controls.moc"
