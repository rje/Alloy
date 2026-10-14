// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFile>
#include <QJsonDocument>
#include <QLineEdit>
#include <QMouseEvent>
#include <QProcess>
#include <QPushButton>
#include <QTest>
#include <QWindow>
#include <cmath>

class SketchDimensionExpressionControls final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& w,const QString& command,const QJsonObject& args={}){
        const auto r=w.executeApi(command,args);if(!r.value("ok").toBool())qFatal("%s",QJsonDocument(r).toJson().constData());return r.value("result").toObject();
    }
    QJsonObject session(AlloyWindow& w){return w.state().value("activeSession").toObject();}
    QJsonObject draft(AlloyWindow& w){return session(w).value("draftSketch").toObject();}
    QJsonObject params(AlloyWindow& w){return draft(w).value("parameters").toObject();}
    QJsonObject byId(const QJsonArray& values,const QJsonValue& id){for(const auto& v:values)if(v.toObject().value("id")==id)return v.toObject();return {};}
private slots:
    void inlineFormulaAndDriverEdit(){
        const auto out=qEnvironmentVariable("ALLOY_SKETCH_EXPRESSION_EVIDENCE");QVERIFY(!out.isEmpty());QVERIFY(QDir().mkpath(out));
        AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1500,950);w.show();QVERIFY(QTest::qWaitForWindowExposed(&w));
        ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;QVERIFY(view);
#ifdef Q_OS_MACOS
        QProcess activate;activate.start("osascript",{"-e",QString("tell application \"System Events\" to set frontmost of first application process whose unix id is %1 to true").arg(QCoreApplication::applicationPid())});
        QVERIFY(activate.waitForFinished(5000));QCOMPARE(activate.exitCode(),0);
#endif
        w.raise();w.activateWindow();w.windowHandle()->requestActivate();QApplication::setActiveWindow(&w);QTRY_VERIFY(w.isActiveWindow());
        // Fixture setup uses shared commands, not claimed as native tutorial creation.
        call(w,"sketch.begin",{{"plane","XY"},{"units","mm"}});
        call(w,"sketch.centerToCenterSlot",{{"firstX",-90},{"firstY",-30},{"secondX",-45},{"secondY",-30},{"width",10},{"units","mm"}});
        const auto slot=params(w).value("slotGroups").toArray().first().toObject();
        call(w,"sketch.constraint",{{"type","horizontal"},{"entityIds",QJsonArray{slot.value("centerlineId")}}});
        call(w,"sketch.dimension",{{"kind","linear"},{"entityId",slot.value("centerlineId")},{"value",45},{"units","mm"}});
        call(w,"sketch.dimension",{{"kind","radius"},{"entityId",slot.value("arcIds").toArray().first()},{"value",5},{"units","mm"}});
        auto line=[&](double x1,double y1,double x2,double y2){call(w,"sketch.line",{{"x1",x1},{"y1",y1},{"x2",x2},{"y2",y2},{"units","mm"}});return params(w).value("entities").toArray().last().toObject();};
        const auto bottom=line(0,0,100,0),right=line(100,0,100,100),top=line(100,100,-50,110);
        line(-50,110,0,0);
        call(w,"sketch.constraint",{{"type","fixed"},{"pointIds",QJsonArray{bottom.value("startPointId")}}});
        call(w,"sketch.constraint",{{"type","horizontal"},{"entityIds",QJsonArray{bottom.value("id")}}});
        call(w,"sketch.constraint",{{"type","vertical"},{"entityIds",QJsonArray{right.value("id")}}});
        call(w,"sketch.constraint",{{"type","perpendicular"},{"entityIds",QJsonArray{top.value("id"),right.value("id")}}});
        call(w,"sketch.constraint",{{"type","equal"},{"entityIds",QJsonArray{bottom.value("id"),right.value("id")}}});
        call(w,"sketch.dimension",{{"kind","vertical"},{"entityId",right.value("id")},{"value",100},{"units","mm"},{"annotation",QJsonObject{{"uMm",120},{"vMm",50}}}});
        const auto driver=params(w).value("dimensions").toArray().last().toObject();QCOMPARE(driver.value("name").toString(),QString("d3"));
        call(w,"sketch.lookAt");call(w,"view.zoom",{{"factor",1.0}});QTest::qWait(100);
        auto pixel=[&](double u,double v){const auto p=call(w,"view.projectSketchPoint",{{"sketchId",draft(w).value("id")},{"uMm",u},{"vMm",v}});return QPoint(qRound(p.value("x").toDouble()),qRound(p.value("y").toDouble()));};
        const auto pp=params(w);const auto a=byId(pp.value("points").toArray(),top.value("startPointId")),b=byId(pp.value("points").toArray(),top.value("endPointId"));QVERIFY(!a.isEmpty() && !b.isEmpty());
        const auto edge=pixel((a.value("uMm").toDouble()+b.value("uMm").toDouble())/2,(a.value("vMm").toDouble()+b.value("vMm").toDouble())/2);
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QTest::keyClick(view,Qt::Key_D);
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,edge);
        QCOMPARE(session(w).value("selection").toObject().value("entityIds").toArray(),QJsonArray{top.value("id")});
        const auto label=pixel(25,135);QVERIFY(view->rect().contains(label));
        QMouseEvent move(QEvent::MouseMove,QPointF(label),QPointF(view->mapToGlobal(label)),Qt::NoButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(view,&move);
        QTRY_COMPARE(session(w).value("pendingDimension").toObject().value("phase").toString(),QString("placing"));
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,label);
        auto* editor=w.findChild<QLineEdit*>("sketch.inlineValue");QVERIFY(editor);
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(editor));QTest::keyClick(editor,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(editor,"d3 + 50");QTest::keyClick(editor,Qt::Key_Return);
        QTRY_VERIFY(!session(w).contains("pendingDimension"));
        const auto dependent=params(w).value("dimensions").toArray().last().toObject();
        QCOMPARE(dependent.value("expression").toString(),QString("d3 + 50"));QCOMPARE(dependent.value("referenceIds").toArray(),QJsonArray{driver.value("id")});
        QVERIFY(std::abs(dependent.value("valueMm").toDouble()-150)<1e-6);
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QTest::keyClick(view,Qt::Key_Escape);
        call(w,"capture.screenshot",{{"path",out+"/formula-100-150.png"}});
        const auto driverLayout=view->dimensionLayout(draft(w),driver,100,"mm").value("label").toObject();QVERIFY(!driverLayout.isEmpty());
        const QPoint driverLabel(qRound(driverLayout.value("x").toDouble()),qRound(driverLayout.value("y").toDouble()));QVERIFY(view->rect().contains(driverLabel));
        QTest::mouseDClick(view,Qt::LeftButton,Qt::NoModifier,driverLabel);
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(editor));
        QCOMPARE(session(w).value("pendingDimension").toObject().value("dimensionId"),driver.value("id"));
        QTest::keyClick(editor,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(editor,"120");QTest::keyClick(editor,Qt::Key_Return);
        QTRY_VERIFY(!session(w).contains("pendingDimension"));
        const auto changed=byId(params(w).value("dimensions").toArray(),dependent.value("id"));
        QVERIFY(std::abs(changed.value("valueMm").toDouble()-170)<1e-6);QCOMPARE(changed.value("referenceIds"),dependent.value("referenceIds"));
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
        call(w,"capture.screenshot",{{"path",out+"/formula-120-170.png"}});
        // Double-click the dependent label: the editor must retain authored text.
        const auto layout=view->dimensionLayout(draft(w),changed,170,"mm").value("label").toObject();QVERIFY(!layout.isEmpty());
        QTest::mouseDClick(view,Qt::LeftButton,Qt::NoModifier,QPoint(qRound(layout.value("x").toDouble()),qRound(layout.value("y").toDouble())));
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(editor));QCOMPARE(editor->text(),QString("d3 + 50"));
        QTest::keyClick(editor,Qt::Key_Escape);QTRY_VERIFY(!session(w).contains("pendingDimension"));
        auto* spin=w.findChild<QDoubleSpinBox*>("sketch.dimension.edit");auto* apply=w.findChild<QPushButton*>("sketch.dimension.apply");QVERIFY(spin && apply);
        QVERIFY(!spin->isEnabled());QCOMPARE(apply->text(),QString("Edit Expression"));
        QFile state(out+"/native-state.json");QVERIFY(state.open(QIODevice::WriteOnly));state.write(QJsonDocument(w.state()).toJson());
        call(w,"sketch.cancel");w.close();
    }
};
QTEST_MAIN(SketchDimensionExpressionControls)
#include "sketch_dimension_expression_controls.moc"
