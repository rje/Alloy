// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QAction>
#include <QApplication>
#include <QDir>
#include <QJsonDocument>
#include <QLineEdit>
#include <QTest>
#include <cmath>
#include <numbers>

class OverallSlotControlsTest final:public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& w,const QString& command,const QJsonObject& args={}){
        const auto response=w.execute(command,args);if(!response["ok"].toBool())qFatal("%s",QJsonDocument(response).toJson().constData());
        return response["result"].toObject();
    }
private slots:
    void nativeTotalAngleWidthGesture(){
        AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1350,900);w.show();w.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&w));QApplication::setActiveWindow(&w);
        ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;
        QVERIFY(view);view->setFocus();call(w,"sketch.begin",{{"plane","XY"}});call(w,"view.zoom",{{"factor",0.65}});
        auto* action=w.findChild<QAction*>("sketch.tool.overallSlot");QVERIFY(action);action->trigger();
        const auto pixel=[&](double u,double v){const auto id=w.state()["activeSession"].toObject()["draftSketch"].toObject()["id"];
            const auto p=call(w,"view.projectSketchPoint",{{"sketchId",id},{"uMm",u},{"vMm",v}});
            return QPoint(qRound(p["x"].toDouble()),qRound(p["y"].toDouble()));};
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(0,0));
        QTest::keyClick(view,Qt::Key_3);
        auto* distance=w.findChild<QLineEdit*>("sketch.slot.overallLength");QVERIFY(distance && distance->isVisible());
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(distance));
        QTest::keyClicks(distance,"0");QTest::keyClick(distance,Qt::Key_Tab);
        auto* angle=w.findChild<QLineEdit*>("sketch.slot.angle");QVERIFY(angle && angle->isVisible());
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(angle));
        QTest::keyClick(angle,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(angle,"20");
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(25,12));
        const auto second=w.state()["activeSession"].toObject()["secondPoint"].toObject();
        QVERIFY(std::abs(second["uMm"].toDouble()-30*std::cos(20*std::numbers::pi/180))<1e-5);
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
        QTest::keyClick(view,Qt::Key_6);
        auto* width=w.findChild<QLineEdit*>("sketch.slot.width");QVERIFY(width && width->isVisible());
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(width));
        QTest::keyClicks(width,".5");QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(0,6));
        const auto session=w.state()["activeSession"].toObject(),p=session["draftSketch"].toObject()["parameters"].toObject();
        QCOMPARE(session["tool"].toString(),QString("overallSlot"));
        QCOMPARE(p["slotGroups"].toArray().size(),1);QCOMPARE(p["slotGroups"].toArray()[0].toObject()["mode"].toString(),QString("overall"));
        const auto dims=p["dimensions"].toArray();QCOMPARE(dims.size(),3);
        QCOMPARE(dims[0].toObject()["valueMm"].toDouble(),30.0);QCOMPARE(dims[1].toObject()["valueMm"].toDouble(),6.5);
        QCOMPARE(dims[2].toObject()["valueDeg"].toDouble(),20.0);
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QTest::keyClick(view,Qt::Key_Return);
        QCOMPARE(w.state()["activeSession"].toObject()["tool"].toString(),QString("select"));
        const auto output=qEnvironmentVariable("ALLOY_OVERALL_SLOT_EVIDENCE");if(!output.isEmpty()){QVERIFY(QDir().mkpath(output));
            QApplication::processEvents();QTest::qWait(70);
            call(w,"capture.screenshot",{{"path",output+"/overall-slot-native.png"}});}
        call(w,"sketch.finish");call(w,"document.new",{{"discard",true}});w.close();
    }
};
QTEST_MAIN(OverallSlotControlsTest)
#include "overall_slot_controls.moc"
