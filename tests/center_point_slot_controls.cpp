#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QAction>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QTest>
#include <cmath>
#include <numbers>

class CenterPointSlotControlsTest final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& w,const QString& command,const QJsonObject& args={}) {
        const auto response=w.execute(command,args);
        if(!response["ok"].toBool())qFatal("%s",QJsonDocument(response).toJson().constData());
        return response["result"].toObject();
    }
    QJsonObject active(AlloyWindow& w){return w.state()["activeSession"].toObject();}
    QJsonObject draft(AlloyWindow& w){return active(w)["draftSketch"].toObject();}
private slots:
    void nativeSlotSourceChain() {
        const auto output=qEnvironmentVariable("ALLOY_SLOT_EVIDENCE");QVERIFY(!output.isEmpty());QVERIFY(QDir().mkpath(output));
        AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1350,900);w.show();w.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&w));QApplication::setActiveWindow(&w);
        ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())
            if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;
        QVERIFY(view);view->setFocus();
        auto* slot=w.findChild<QAction*>("sketch.tool.centerPointSlot");QVERIFY(slot);
        auto pixel=[&](double u,double v){const auto p=call(w,"view.projectSketchPoint",{{"sketchId",draft(w)["id"]},{"uMm",u},{"vMm",v}});
            return QPoint(qRound(p["x"].toDouble()),qRound(p["y"].toDouble()));};
        auto motion=[&](const QPoint& p){QMouseEvent event(QEvent::MouseMove,QPointF(p),QPointF(view->mapToGlobal(p)),
            Qt::NoButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(view,&event);};

        // A discarded typed gesture proves the native distance -> Tab angle ->
        // width fields without altering the source sketch below.
        call(w,"sketch.begin",{{"plane","XY"}});call(w,"view.zoom",{{"factor",0.65}});slot->trigger();
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(0,0));
        QTRY_VERIFY(active(w).contains("firstPoint"));
        QTest::keyClick(view,Qt::Key_6);
        auto* distance=w.findChild<QLineEdit*>("sketch.slot.halfLength");QVERIFY(distance && distance->isVisible());
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(distance));
        QTest::keyClicks(distance,"5");QTest::keyClick(distance,Qt::Key_Tab);
        auto* angle=w.findChild<QLineEdit*>("sketch.slot.angle");QVERIFY(angle && angle->isVisible());
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(angle));
        QTest::keyClick(angle,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(angle,"20");
        QCOMPARE(active(w)["pendingSlotTyped"].toObject()["valid"].toBool(),true);
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(70,25));
        QTRY_VERIFY(active(w).contains("secondPoint"));
        const auto typedEnd=active(w)["secondPoint"].toObject();
        QVERIFY(std::abs(typedEnd["uMm"].toDouble()-65*std::cos(20*std::numbers::pi/180))<1e-6);
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
        QTest::keyClick(view,Qt::Key_5);
        auto* width=w.findChild<QLineEdit*>("sketch.slot.width");QVERIFY(width && width->isVisible());
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(width));
        QTest::keyClicks(width,"0");QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(0,25));
        QCOMPARE(draft(w)["parameters"].toObject()["slotGroups"].toArray().size(),1);
        QCOMPARE(active(w)["solverDiagnostics"].toObject()["dof"].toInt(-1),3);
        QApplication::processEvents();QTest::qWait(80);
        auto* sketchInfo=w.findChild<QLabel*>("sketch.info");
        auto* profileInfo=w.findChild<QLabel*>("sketch.profileStatus");
        QVERIFY(sketchInfo && profileInfo);
        QTRY_VERIFY(sketchInfo->mapToGlobal(sketchInfo->rect().bottomLeft()).y()
            < profileInfo->mapToGlobal(profileInfo->rect().topLeft()).y());
        call(w,"capture.screenshot",{{"path",output+"/typed-slot.png"}});
        call(w,"sketch.cancel");

        // Real source order: center, rough angled end center, side edge,
        // Horizontal/Vertical on the retained guide, D150, R25.
        call(w,"sketch.begin",{{"plane","XY"}});call(w,"view.zoom",{{"factor",0.65}});slot->trigger();
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(0,0));
        motion(pixel(67,13));QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(67,13));
        motion(pixel(0,21));QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(0,21));
        QCOMPARE(active(w)["solverDiagnostics"].toObject()["dof"].toInt(-1),3);
        auto p=draft(w)["parameters"].toObject();const auto group=p["slotGroups"].toArray().first().toObject();
        QCOMPARE(p["dimensions"].toArray().size(),0);QCOMPARE(p["entities"].toArray().size(),5);
        QApplication::processEvents();QTest::qWait(50);call(w,"capture.screenshot",{{"path",output+"/rough-slot.png"}});
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QTest::keyClick(view,Qt::Key_Escape);
        QCOMPARE(active(w)["tool"].toString(),QString("select"));
        const auto guideEntity=[&]() -> QJsonObject {
            for(const auto& value:draft(w)["parameters"].toObject()["entities"].toArray())
                if(value.toObject()["id"]==group["centerlineId"])return value.toObject();
            return {};
        }();QVERIFY(!guideEntity.isEmpty());
        const auto originalEnd=guideEntity.value("endPointId");
        const auto from=pixel(67,13),to=pixel(60,25);
        QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,from);
        QMouseEvent drag(QEvent::MouseMove,QPointF(to),QPointF(view->mapToGlobal(to)),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);
        QApplication::sendEvent(view,&drag);QTest::mouseRelease(view,Qt::LeftButton,Qt::NoModifier,to);
        QCOMPARE(active(w)["solverDiagnostics"].toObject()["dof"].toInt(-1),3);
        QJsonObject draggedEnd;for(const auto& value:draft(w)["parameters"].toObject()["points"].toArray())
            if(value.toObject()["id"]==originalEnd){draggedEnd=value.toObject();break;}
        QVERIFY(!draggedEnd.isEmpty() && std::abs(draggedEnd["vMm"].toDouble()-13)>0.5);
        QApplication::processEvents();QTest::qWait(40);call(w,"capture.screenshot",{{"path",output+"/dragged-slot.png"}});
        auto* horizontal=w.findChild<QAction*>("sketch.action.constraint.horizontalVertical");QVERIFY(horizontal);horizontal->trigger();
        QCOMPARE(active(w)["tool"].toString(),QString("constraint"));
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(draggedEnd["uMm"].toDouble()/2,draggedEnd["vMm"].toDouble()/2));
        QTRY_COMPARE(active(w)["solverDiagnostics"].toObject()["dof"].toInt(-1),2);
        QTest::keyClick(view,Qt::Key_Escape);QCOMPARE(active(w)["tool"].toString(),QString("select"));
        for(int index=0;index<2;++index){
            QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QTest::keyClick(view,Qt::Key_D);
            QCOMPARE(active(w)["tool"].toString(),QString("dimension"));
            const auto target=index==0 ? pixel(35,0) : pixel(96,0);
            QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,target);
            QTRY_COMPARE(active(w)["selection"].toObject()["entityIds"].toArray().size(),1);
            const auto label=index==0 ? pixel(20,35) : pixel(85,30);
            motion(label);QApplication::processEvents();
            QTRY_COMPARE(active(w)["pendingDimension"].toObject()["phase"].toString(),QString("placing"));
            QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,label);
            auto* editor=w.findChild<QLineEdit*>("sketch.inlineValue");QVERIFY(editor && editor->isVisible());
            QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(editor));
            editor->selectAll();QTest::keyClicks(editor,index==0 ? "150" : "25");QTest::keyClick(editor,Qt::Key_Return);
            QTRY_VERIFY(!active(w).contains("pendingDimension"));
            QCOMPARE(active(w)["solverDiagnostics"].toObject()["dof"].toInt(-1),1-index);
            QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QTest::keyClick(view,Qt::Key_Escape);
        }
        p=draft(w)["parameters"].toObject();QCOMPARE(p["dimensions"].toArray().size(),2);
        QCOMPARE(p["slotGroups"].toArray().first().toObject(),group);
        QApplication::processEvents();QTest::qWait(70);call(w,"capture.screenshot",{{"path",output+"/defined-slot.png"}});
        call(w,"sketch.finish");const QJsonValue authored=call(w,"document.inspect").value("authored");
        const auto path=output+"/center-point-slot.alloy";call(w,"document.save",{{"path",path}});
        call(w,"document.open",{{"path",path}});QCOMPARE(QJsonValue(call(w,"document.inspect").value("authored")),authored);
        QFile evidence(output+"/native-state.json");QVERIFY(evidence.open(QIODevice::WriteOnly));evidence.write(QJsonDocument(w.state()).toJson(QJsonDocument::Indented));
        w.close();
    }
    void escapeCancelsPendingSlotDrag() {
        AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1350,900);w.show();w.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&w));QApplication::setActiveWindow(&w);
        ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())
            if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;
        QVERIFY(view);view->setFocus();
        call(w,"sketch.begin",{{"plane","XY"}});call(w,"view.zoom",{{"factor",0.65}});
        call(w,"sketch.centerPointSlot",{{"centerX",0},{"centerY",0},{"endX",65},{"endY",12},
            {"width",40},{"units","mm"},{"centerSnapTarget",QJsonObject{{"kind","origin"}}}});
        const QJsonObject before=draft(w);
        const auto group=before["parameters"].toObject()["slotGroups"].toArray().first().toObject();
        QJsonObject guide;for(const auto& value:before["parameters"].toObject()["entities"].toArray())
            if(value.toObject()["id"]==group["centerlineId"]){guide=value.toObject();break;}
        QVERIFY(!guide.isEmpty());
        const auto projected=call(w,"view.projectSketchPoint",{{"sketchId",before["id"]},{"uMm",65},{"vMm",12}});
        const QPoint from(qRound(projected["x"].toDouble()),qRound(projected["y"].toDouble()));
        const QPoint to=from+QPoint(-35,-45);
        QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,from);
        QMouseEvent drag(QEvent::MouseMove,QPointF(to),QPointF(view->mapToGlobal(to)),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);
        QApplication::sendEvent(view,&drag);
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
        QTest::keyClick(view,Qt::Key_Escape);
        QCOMPARE(active(w)["tool"].toString(),QString("select"));
        QTest::mouseRelease(view,Qt::LeftButton,Qt::NoModifier,to);
        QCOMPARE(draft(w),before);
        call(w,"sketch.cancel");
        w.close();
    }
};
QTEST_MAIN(CenterPointSlotControlsTest)
#include "center_point_slot_controls.moc"
