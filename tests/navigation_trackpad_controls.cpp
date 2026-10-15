// SPDX-License-Identifier: MIT
#include "AlloyTheme.h"
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QDir>
#include <QJsonDocument>
#include <QNativeGestureEvent>
#include <QPointingDevice>
#include <QTest>
#include <cmath>

class NavigationTrackpadControls final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& name,const QJsonObject& args={}) {
        const auto response=window.executeApi(name,args);
        if(!response.value("ok").toBool())qFatal("%s",QJsonDocument(response).toJson(QJsonDocument::Compact).constData());
        return response.value("result").toObject();
    }
    static QJsonObject navigation(AlloyWindow& window) {
        return window.state().value("viewport").toObject().value("navigation").toObject();
    }
    static double separation(const QJsonArray& a,const QJsonArray& b) {
        double sum=0;for(int i=0;i<3;++i)sum+=std::pow(a[i].toDouble()-b[i].toDouble(),2);return std::sqrt(sum);
    }
    static double distance(const QJsonObject& a,const QJsonObject& b) {
        return std::hypot(a.value("x").toDouble()-b.value("x").toDouble(),
            a.value("y").toDouble()-b.value("y").toDouble());
    }
    static void wheel(ReferenceView* view,const QPointF& at,const QPoint pixel,const QPoint angle,
        Qt::KeyboardModifiers modifiers,const QPointingDevice* device,Qt::ScrollPhase phase=Qt::ScrollUpdate) {
        QWheelEvent event(at,view->mapToGlobal(at.toPoint()),pixel,angle,Qt::NoButton,modifiers,phase,false,
            Qt::MouseEventNotSynthesized,device);
        QApplication::sendEvent(view,&event);
    }
    static void gesture(ReferenceView* view,const QPointF& at,const QPointingDevice* device,
        Qt::NativeGestureType type,double value=0,const QPointF& delta={},Qt::KeyboardModifiers modifiers=Qt::NoModifier) {
        QNativeGestureEvent event(type,device,2,at,view->mapToGlobal(at.toPoint()),
            view->mapToGlobal(at.toPoint()),value,delta,1);
        event.setModifiers(modifiers);
        QApplication::sendEvent(view,&event);
    }
private slots:
    void distinctRoutesAndCursorAnchor() {
        const auto out=qEnvironmentVariable("ALLOY_NAV_TRACKPAD_EVIDENCE");
        QVERIFY(!out.isEmpty());QVERIFY(QDir().mkpath(out));
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1360,850);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        ReferenceView* view=nullptr;for(auto* child:window.findChildren<QWidget*>())
            if(auto* found=dynamic_cast<ReferenceView*>(child))view=found;
        QVERIFY(view);
        call(window,"sketch.begin",{{"plane","XY"},{"units","mm"}});
        call(window,"sketch.rectangle",{{"x1",20},{"y1",10},{"x2",60},{"y2",30},{"units","mm"}});
        call(window,"sketch.finish");
        const auto sketch=call(window,"document.inspect").value("evaluation").toObject().value("sketches").toArray().first().toObject();
        call(window,"solid.extrude",{{"sketchId",sketch.value("sketchId")},
            {"profileId",sketch.value("profiles").toArray().first().toObject().value("profileId")},
            {"distance",12},{"units","mm"}});
        const auto body=call(window,"document.inspect").value("evaluation").toObject().value("bodies").toArray().first().toObject();
        const auto bodyId=body.value("bodyId").toString();QVERIFY(!bodyId.isEmpty());
        call(window,"selection.set",{{"bodyId",bodyId}});
        call(window,"view.home");call(window,"view.pan",{{"x",73},{"y",-27}});
        const auto document=call(window,"document.inspect");
        auto projected=[&]{return call(window,"view.projectBody",{{"bodyId",bodyId}});};
        const auto anchored=projected();
        call(window,"view.zoom",{{"factor",1.2},{"x",anchored.value("x")},{"y",anchored.value("y")}});
        QVERIFY(distance(anchored,projected())<1.5);
        const auto oldPan=navigation(window).value("panX").toDouble();
        call(window,"view.zoom",{{"factor",1.05}});
        QCOMPARE(navigation(window).value("panX").toDouble(),oldPan);
        call(window,"view.zoom",{{"factor",1.05},{"x",view->width()/2.0},{"y",view->height()/2.0}});
        QVERIFY(std::abs(navigation(window).value("panX").toDouble()-oldPan*1.05)<1e-7);
        const auto beforeWheel=projected();
        const QPointF at(beforeWheel.value("x").toDouble(),beforeWheel.value("y").toDouble());
        const auto zoomBeforeWheel=navigation(window).value("zoom").toDouble();
        wheel(view,at,{},QPoint(0,60),Qt::NoModifier,QPointingDevice::primaryPointingDevice());
        QCOMPARE(navigation(window).value("lastInput").toObject().value("route"),QJsonValue("wheelZoom"));
        QVERIFY(navigation(window).value("zoom").toDouble()>zoomBeforeWheel);
        QVERIFY(distance(beforeWheel,projected())<1.5);

        QPointingDevice touchpad("NAV003 synthetic touchpad",57303,QInputDevice::DeviceType::TouchPad,
            QPointingDevice::PointerType::Finger,QInputDevice::Capability::Position,2,0);
        const auto beforePan=navigation(window);const auto zoomBeforePan=beforePan.value("zoom");
        wheel(view,at,QPoint(11,-8),QPoint(0,120),Qt::NoModifier,&touchpad);
        QCOMPARE(navigation(window).value("lastInput").toObject().value("route"),QJsonValue("touchpadPan"));
        QCOMPARE(navigation(window).value("zoom"),zoomBeforePan);
        QVERIFY(navigation(window).value("panX").toDouble()!=beforePan.value("panX").toDouble());
        const auto beforeOrbit=window.state().value("viewport").toObject().value("gpu").toObject().value("camera").toObject().value("direction").toArray();
        wheel(view,at,QPoint(26,17),QPoint(0,120),Qt::ShiftModifier,&touchpad);
        QCOMPARE(navigation(window).value("lastInput").toObject().value("route"),QJsonValue("touchpadOrbit"));
        QCOMPARE(navigation(window).value("zoom"),zoomBeforePan);
        const auto afterOrbit=window.state().value("viewport").toObject().value("gpu").toObject().value("camera").toObject().value("direction").toArray();
        QVERIFY(separation(beforeOrbit,afterOrbit)>0.02);

        const auto pinchAnchor=projected();const QPointF pinchAt(pinchAnchor.value("x").toDouble(),pinchAnchor.value("y").toDouble());
        gesture(view,pinchAt,&touchpad,Qt::BeginNativeGesture);
        const auto beforeSuppressed=navigation(window);
        wheel(view,pinchAt,QPoint(20,14),QPoint(0,120),Qt::NoModifier,&touchpad);
        QCOMPARE(navigation(window).value("lastInput").toObject().value("route"),QJsonValue("nativeGestureOwnsStream"));
        QCOMPARE(navigation(window).value("zoom"),beforeSuppressed.value("zoom"));
        QCOMPARE(navigation(window).value("panX"),beforeSuppressed.value("panX"));
        gesture(view,pinchAt,&touchpad,Qt::ZoomNativeGesture,0.1);
        QCOMPARE(navigation(window).value("lastInput").toObject().value("route"),QJsonValue("pinchZoom"));
        QVERIFY(distance(pinchAnchor,projected())<1.5);
        gesture(view,pinchAt,&touchpad,Qt::EndNativeGesture);
        gesture(view,pinchAt,&touchpad,Qt::BeginNativeGesture);
        QEvent interrupted(QEvent::WindowDeactivate);QApplication::sendEvent(view,&interrupted);
        wheel(view,pinchAt,QPoint(4,2),{},Qt::NoModifier,&touchpad);
        QCOMPARE(navigation(window).value("lastInput").toObject().value("route"),QJsonValue("touchpadPan"));
        const auto panBeforeNative=navigation(window).value("panX").toDouble();
        gesture(view,pinchAt,&touchpad,Qt::PanNativeGesture,0,QPointF(8,-5));
        QCOMPARE(navigation(window).value("lastInput").toObject().value("route"),QJsonValue("nativePan"));
        QVERIFY(navigation(window).value("panX").toDouble()!=panBeforeNative);
        const auto beforeNativeOrbit=window.state().value("viewport").toObject().value("gpu").toObject().value("camera").toObject().value("direction").toArray();
        gesture(view,pinchAt,&touchpad,Qt::PanNativeGesture,0,QPointF(7,4),Qt::ShiftModifier);
        QCOMPARE(navigation(window).value("lastInput").toObject().value("route"),QJsonValue("nativeOrbit"));
        const auto afterNativeOrbit=window.state().value("viewport").toObject().value("gpu").toObject().value("camera").toObject().value("direction").toArray();
        QVERIFY(separation(beforeNativeOrbit,afterNativeOrbit)>0.001);
        call(window,"capture.screenshot",{{"path",out+"/navigation-after-gestures.png"}});
        QCOMPARE(call(window,"document.inspect").value("authored"),document.value("authored"));
        QCOMPARE(call(window,"document.inspect").value("undo"),document.value("undo"));
        QCOMPARE(call(window,"document.inspect").value("revision"),document.value("revision"));
        QCOMPARE(window.state().value("selection").toObject().value("bodyId"),QJsonValue(bodyId));
        call(window,"selection.set",{{"bodyId",""}});
        call(window,"selection.priority",{{"mode","body"}});
        QPoint visibleHit;bool foundHit=false;
        const auto projectedBody=projected();
        const QPoint approximate(qRound(projectedBody.value("x").toDouble()),qRound(projectedBody.value("y").toDouble()));
        for(int dy=-90;dy<=90&&!foundHit;dy+=10)for(int dx=-90;dx<=90&&!foundHit;dx+=10) {
            const QPoint candidate=approximate+QPoint(dx,dy);
            if(view->rect().contains(candidate)&&view->pickBody(candidate.x(),candidate.y())==bodyId) {
                visibleHit=candidate;foundHit=true;
            }
        }
        QVERIFY(foundHit);
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,visibleHit);
        QCOMPARE(window.state().value("selection").toObject().value("bodyId"),QJsonValue(bodyId));
        call(window,"document.save",{{"path",out+"/navigation-gestures.alloy"}});window.close();
    }
};
QTEST_MAIN(NavigationTrackpadControls)
#include "navigation_trackpad_controls.moc"
