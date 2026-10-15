// SPDX-License-Identifier: MIT
#include "AlloyTheme.h"
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QDir>
#include <QJsonDocument>
#include <QMouseEvent>
#include <QPushButton>
#include <QTest>
#include <cmath>

class NavigationPanZoomWindowControls final : public QObject {
    Q_OBJECT
    static QJsonObject call(AlloyWindow& window,const QString& name,const QJsonObject& args={}) {
        const auto response=window.executeApi(name,args);
        if(!response.value("ok").toBool())qFatal("%s",QJsonDocument(response).toJson(QJsonDocument::Compact).constData());
        return response.value("result").toObject();
    }
    static QJsonObject camera(AlloyWindow& window) {
        return window.state().value("viewport").toObject().value("navigation").toObject();
    }
    static void move(ReferenceView* view,const QPoint& point) {
        QMouseEvent event(QEvent::MouseMove,QPointF(point),QPointF(view->mapToGlobal(point)),
            Qt::NoButton,Qt::LeftButton,Qt::NoModifier);
        QApplication::sendEvent(view,&event);
    }
    static void drag(ReferenceView* view,const QPoint& from,const QPoint& to) {
        QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,from);
        QVERIFY(view->hasFocus());
        move(view,to);QTest::mouseRelease(view,Qt::LeftButton,Qt::NoModifier,to);
    }
private slots:
    void pointerToolsPreserveDesign() {
        const auto out=qEnvironmentVariable("ALLOY_NAV_PAN_ZOOM_EVIDENCE");
        QVERIFY(!out.isEmpty());QVERIFY(QDir().mkpath(out));
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1360,850);window.show();window.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QApplication::setActiveWindow(&window);QTRY_VERIFY(window.isActiveWindow());
        ReferenceView* view=nullptr;for(auto* child:window.findChildren<QWidget*>())
            if(auto* found=dynamic_cast<ReferenceView*>(child))view=found;
        QVERIFY(view);
        auto* pan=window.findChild<QPushButton*>("view.navigationTool.pan");
        auto* zoom=window.findChild<QPushButton*>("view.navigationTool.zoom");
        auto* rect=window.findChild<QPushButton*>("view.navigationTool.zoomWindow");
        auto* orbit=window.findChild<QPushButton*>("view.orbitTool");
        QVERIFY(pan && zoom && rect && orbit);
        call(window,"solid.box",{{"width",46},{"depth",27},{"height",13},{"units","mm"}});
        const auto body=call(window,"document.inspect").value("evaluation").toObject().value("bodies").toArray().first().toObject();
        const auto bodyId=body.value("bodyId").toString();QVERIFY(!bodyId.isEmpty());
        call(window,"view.home");const auto original=call(window,"document.inspect");
        const QPoint center(view->width()/2,view->height()/2);
        QTest::mouseClick(pan,Qt::LeftButton);QVERIFY(pan->isChecked());
        QCOMPARE(camera(window).value("navigationTool"),QJsonValue("pan"));
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,center);
        QCOMPARE(camera(window).value("panX"),QJsonValue(0));
        drag(view,center,center+QPoint(55,-25));
        QVERIFY(std::abs(camera(window).value("panX").toDouble()-55)<1e-6);
        QTest::mouseClick(zoom,Qt::LeftButton);QVERIFY(zoom->isChecked() && !pan->isChecked());
        drag(view,center,center+QPoint(0,-25));
        QVERIFY(camera(window).value("zoom").toDouble()>1.2);
        QTest::mouseClick(rect,Qt::LeftButton);QVERIFY(rect->isChecked() && !zoom->isChecked());
        const auto before=camera(window);
        const QPoint a(qRound(view->width()*0.08),qRound(view->height()*0.10));
        const QPoint b(qRound(view->width()*0.90),qRound(view->height()*0.90));
        QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,a);QVERIFY(view->hasFocus());
        move(view,b);
        auto* band=view->findChild<QWidget*>("view.zoomWindowBand");QVERIFY(band && band->isVisible());
        call(window,"capture.screenshot",{{"path",out+"/zoom-window-drag.png"}});
        QTest::mouseRelease(view,Qt::LeftButton,Qt::NoModifier,b);
        QVERIFY(!band->isVisible());
        QVERIFY(camera(window).value("zoom").toDouble()>before.value("zoom").toDouble());
        call(window,"capture.screenshot",{{"path",out+"/zoom-window-result.png"}});
        QCOMPARE(call(window,"document.inspect").value("authored"),original.value("authored"));
        QCOMPARE(call(window,"document.inspect").value("undo"),original.value("undo"));
        QVERIFY(view->hasFocus());QTest::keyClick(view,Qt::Key_Escape);
        QCOMPARE(camera(window).value("navigationTool"),QJsonValue("select"));QVERIFY(!rect->isChecked());
        const auto projected=call(window,"view.projectBody",{{"bodyId",bodyId}});
        const QPoint pixel(qRound(projected.value("x").toDouble()),qRound(projected.value("y").toDouble()));
        QCOMPARE(view->pickBody(pixel.x(),pixel.y()),bodyId);
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel);
        const auto selected=window.state().value("selection").toObject();
        const auto selectedBody=selected.value("kind")=="face" ? selected.value("support").toObject().value("bodyId") : selected.value("bodyId");
        QCOMPARE(selectedBody,QJsonValue(bodyId));
        call(window,"document.save",{{"path",out+"/navigation-tools.alloy"}});window.close();
    }
};
QTEST_MAIN(NavigationPanZoomWindowControls)
#include "navigation_pan_zoom_window_controls.moc"
