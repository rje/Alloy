#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QAction>
#include <QApplication>
#include <QDir>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QTest>
#include <cmath>

class CenterPointArcSlotControlsTest final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& name,const QJsonObject& arguments={}){
        const auto response=window.execute(name,arguments);
        if(!response["ok"].toBool())qFatal("%s",QJsonDocument(response).toJson().constData());
        return response["result"].toObject();
    }
    QJsonObject active(AlloyWindow& window){return window.state().value("activeSession").toObject();}
    QJsonObject draft(AlloyWindow& window){return active(window).value("draftSketch").toObject();}
private slots:
    void nativeArcSlotGesture(){
        const auto output=qEnvironmentVariable("ALLOY_ARC_SLOT_EVIDENCE");QVERIFY(!output.isEmpty());QVERIFY(QDir().mkpath(output));
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1500,950);window.show();window.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&window));QApplication::setActiveWindow(&window);
        ReferenceView* view=nullptr;for(auto* child:window.findChildren<QWidget*>())
            if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;
        QVERIFY(view);view->setFocus();
        auto* arc=window.findChild<QAction*>("sketch.tool.centerPointArcSlot");QVERIFY(arc);
        auto pixel=[&](double u,double v){const auto projected=call(window,"view.projectSketchPoint",
            {{"sketchId",draft(window)["id"]},{"uMm",u},{"vMm",v}});
            return QPoint(qRound(projected["x"].toDouble()),qRound(projected["y"].toDouble()));};
        auto motion=[&](const QPoint& p){QMouseEvent move(QEvent::MouseMove,QPointF(p),QPointF(view->mapToGlobal(p)),
            Qt::NoButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(view,&move);};

        // Native fields: path radius + Tab direction, signed sweep, full width.
        call(window,"sketch.begin",{{"plane","XY"}});call(window,"view.zoom",{{"factor",0.2}});arc->trigger();
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(0,0));
        QTest::keyClick(view,Qt::Key_1);
        auto* radius=window.findChild<QLineEdit*>("sketch.slot.halfLength");QVERIFY(radius && radius->isVisible());
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(radius));
        QTest::keyClicks(radius,"48");QTest::keyClick(radius,Qt::Key_Tab);
        auto* angle=window.findChild<QLineEdit*>("sketch.slot.angle");QVERIFY(angle && angle->isVisible());
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(angle));
        angle->selectAll();QTest::keyClicks(angle,"135");QTest::keyClick(angle,Qt::Key_Return);
        QTRY_VERIFY(active(window).contains("secondPoint"));
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(angle));
        angle->selectAll();QTest::keyClicks(angle,"-90");QTest::keyClick(angle,Qt::Key_Return);
        QTRY_VERIFY(active(window).contains("thirdPoint"));
        auto* width=window.findChild<QLineEdit*>("sketch.slot.width");QVERIFY(width && width->isVisible());
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(width));
        QTest::keyClicks(width,"30");QTest::keyClick(width,Qt::Key_Return);
        const auto typedDraft=draft(window);
        QCOMPARE(typedDraft.value("parameters").toObject().value("arcSlotGroups").toArray().size(),1);
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
        QApplication::processEvents();QTest::qWait(80);call(window,"capture.screenshot",{{"path",output+"/typed-arc-slot.png"}});
        call(window,"sketch.cancel");

        // Source-like pointer picks on a sketch already carrying the straight slot.
        call(window,"sketch.begin",{{"plane","XY"}});call(window,"view.zoom",{{"factor",0.2}});
        call(window,"sketch.centerPointSlot",{{"centerX",0},{"centerY",0},{"endX",68},{"endY",13},
            {"width",44},{"units","mm"},{"centerSnapTarget",QJsonObject{{"kind","origin"}}}});
        arc->trigger();QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(0,0));
        motion(pixel(-100,110));QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(-100,110));
        motion(pixel(105,98));QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(105,98));
        motion(pixel(0,164));
        QApplication::processEvents();QTest::qWait(70);call(window,"capture.screenshot",{{"path",output+"/arc-slot-preview.png"}});
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(0,164));
        QCOMPARE(draft(window).value("parameters").toObject().value("arcSlotGroups").toArray().size(),1);
        QTest::keyClick(view,Qt::Key_Escape);
        QCOMPARE(active(window)["tool"].toString(),QString("select"));
        QTRY_COMPARE(active(window)["profileSummaries"].toArray().size(),2);
        QApplication::processEvents();QTest::qWait(80);call(window,"capture.screenshot",{{"path",output+"/rough-arc-slot.png"}});
        call(window,"sketch.cancel");window.close();
    }
};
QTEST_MAIN(CenterPointArcSlotControlsTest)
#include "center_point_arc_slot_controls.moc"
