// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QAction>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QLineEdit>
#include <QMouseEvent>
#include <QProcess>
#include <QTest>
#include <QWindow>
#include <cmath>

class CenterToCenterSlotControls final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& w,const QString& command,const QJsonObject& args={}) {
        const auto response=w.executeApi(command,args);
        if(!response.value("ok").toBool())qFatal("%s",QJsonDocument(response).toJson().constData());
        return response.value("result").toObject();
    }
    QJsonObject active(AlloyWindow& w){return w.state().value("activeSession").toObject();}
    QJsonObject parameters(AlloyWindow& w){return active(w).value("draftSketch").toObject().value("parameters").toObject();}
    QJsonObject byId(const QJsonArray& values,const QJsonValue& id){
        for(const auto& v:values)if(v.toObject().value("id")==id)return v.toObject();
        return {};
    }
private slots:
    void sourceSlotDimensionSequence() {
        const auto output=qEnvironmentVariable("ALLOY_CENTER_TO_CENTER_SLOT_EVIDENCE");
        QVERIFY(!output.isEmpty());QVERIFY(QDir().mkpath(output));
        AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1350,900);w.show();
        QVERIFY(QTest::qWaitForWindowExposed(&w));
        ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())
            if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;
        QVERIFY(view);
#ifdef Q_OS_MACOS
        QProcess activation;activation.start("osascript",{"-e",QString("tell application \"System Events\" to set frontmost of first application process whose unix id is %1 to true").arg(QCoreApplication::applicationPid())});
        QVERIFY(activation.waitForFinished(5000));QCOMPARE(activation.exitCode(),0);
#endif
        w.raise();w.activateWindow();w.windowHandle()->requestActivate();QApplication::setActiveWindow(&w);
        QTRY_VERIFY(w.isActiveWindow());
        // API setup only; the new slot and dimensions use Qt pointer/key input.
        call(w,"sketch.begin",{{"plane","XY"},{"units","mm"}});
        call(w,"sketch.lookAt");QTest::qWait(100);
        auto pixel=[&](double u,double v){const auto p=call(w,"view.projectSketchPoint",{
            {"sketchId",active(w).value("draftSketch").toObject().value("id")},{"uMm",u},{"vMm",v}});
            return QPoint(qRound(p.value("x").toDouble()),qRound(p.value("y").toDouble()));};
        auto motion=[&](QPoint p){QMouseEvent event(QEvent::MouseMove,QPointF(p),QPointF(view->mapToGlobal(p)),Qt::NoButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(view,&event);};
        auto* slot=w.findChild<QAction*>("sketch.tool.centerToCenterSlot");QVERIFY(slot);slot->trigger();
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
        // A rough angle avoids automatic horizontal inference. First cap is at origin.
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(0,0));
        motion(pixel(42,8));QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(42,8));
        motion(pixel(20,11));QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(20,11));
        auto p=parameters(w);QCOMPARE(p.value("slotGroups").toArray().size(),1);
        QCOMPARE(p.value("dimensions").toArray().size(),0);
        const auto group=p.value("slotGroups").toArray().first().toObject();
        const auto guide=byId(p.value("entities").toArray(),group.value("centerlineId"));QVERIFY(!guide.isEmpty());
        const auto first=byId(p.value("points").toArray(),guide.value("startPointId"));
        const auto second=byId(p.value("points").toArray(),guide.value("endPointId"));
        QVERIFY(!first.isEmpty());QVERIFY(!second.isEmpty());
        QVERIFY(std::hypot(first.value("uMm").toDouble(),first.value("vMm").toDouble())<1e-5);
        QVERIFY(std::hypot(second.value("uMm").toDouble()-42,second.value("vMm").toDouble()-8)<1);
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QTest::keyClick(view,Qt::Key_Escape);
        auto sideMid=[&](){const auto current=parameters(w);const auto side=byId(current.value("entities").toArray(),group.value("sideIds").toArray().first());
            const auto a=byId(current.value("points").toArray(),side.value("startPointId"));
            const auto b=byId(current.value("points").toArray(),side.value("endPointId"));
            return pixel((a.value("uMm").toDouble()+b.value("uMm").toDouble())/2,(a.value("vMm").toDouble()+b.value("vMm").toDouble())/2);};
        auto* horizontal=w.findChild<QAction*>("sketch.action.constraint.horizontalVertical");QVERIFY(horizontal);horizontal->trigger();
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,sideMid());
        bool hasHorizontal=false;for(const auto& value:parameters(w).value("constraints").toArray())
            if(value.toObject().value("type")=="horizontal")hasHorizontal=true;
        QVERIFY(hasHorizontal);
        const auto horizontalFirst=byId(parameters(w).value("points").toArray(),guide.value("startPointId"));
        QVERIFY(!horizontalFirst.isEmpty());
        QVERIFY(std::hypot(horizontalFirst.value("uMm").toDouble(),horizontalFirst.value("vMm").toDouble())<1e-5);
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QTest::keyClick(view,Qt::Key_Escape);
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QTest::keyClick(view,Qt::Key_D);
        for(int index=0;index<2;++index){
            QCOMPARE(active(w).value("tool").toString(),QString("dimension"));
            QPoint target=sideMid();
            if(index==1){const auto current=parameters(w);const auto arc=byId(current.value("entities").toArray(),group.value("arcIds").toArray().first());
                const auto center=byId(current.value("points").toArray(),arc.value("centerPointId"));
                target=pixel(center.value("uMm").toDouble()+arc.value("radiusMm").toDouble(),center.value("vMm").toDouble());}
            QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,target);
            QTRY_COMPARE(active(w).value("selection").toObject().value("entityIds").toArray().size(),1);
            const auto label=index==0 ? pixel(20,22) : pixel(57,18);motion(label);
            QTRY_COMPARE(active(w).value("pendingDimension").toObject().value("phase").toString(),QString("placing"));
            QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,label);
            auto* editor=w.findChild<QLineEdit*>("sketch.inlineValue");QVERIFY(editor && editor->isVisible());
            QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(editor));
            QTest::keyClick(editor,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(editor,index==0 ? "45" : "5");QTest::keyClick(editor,Qt::Key_Return);
            QTRY_VERIFY(!active(w).contains("pendingDimension"));
            QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
            // Deliberately do not press D again: the source keeps the command active.
        }
        QTest::keyClick(view,Qt::Key_Escape);p=parameters(w);
        const auto dims=p.value("dimensions").toArray();QCOMPARE(dims.size(),2);
        QCOMPARE(dims[0].toObject().value("valueMm").toDouble(),45.0);
        QCOMPARE(dims[1].toObject().value("kind").toString(),QString("radius"));
        QCOMPARE(dims[1].toObject().value("valueMm").toDouble(),5.0);
        QCOMPARE(p.value("slotGroups").toArray().first().toObject(),group);
        const auto finalFirst=byId(p.value("points").toArray(),guide.value("startPointId"));
        QVERIFY(!finalFirst.isEmpty());
        QVERIFY(std::hypot(finalFirst.value("uMm").toDouble(),finalFirst.value("vMm").toDouble())<1e-5);
        bool anchored=false;for(const auto& value:p.value("constraints").toArray()){
            const auto relation=value.toObject();const auto ids=relation.value("pointIds").toArray();
            if(relation.value("type")=="coincident" && ids.contains(guide.value("startPointId"))
                && ids.contains(p.value("originPointId")))anchored=true;
        }
        QVERIFY(anchored);
        QTest::qWait(70);call(w,"capture.screenshot",{{"path",output+"/slot-dimensions.png"}});
        QFile state(output+"/native-state.json");QVERIFY(state.open(QIODevice::WriteOnly));state.write(QJsonDocument(w.state()).toJson());
        call(w,"sketch.cancel");w.close();
    }
};
QTEST_MAIN(CenterToCenterSlotControls)
#include "center_to_center_slot_controls.moc"
