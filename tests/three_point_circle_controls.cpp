// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QAction>
#include <QApplication>
#include <QDir>
#include <QJsonDocument>
#include <QMouseEvent>
#include <QTest>
class ThreePointCircleControlsTest final:public QObject {
 Q_OBJECT
 QJsonObject call(AlloyWindow& w,const QString& c,const QJsonObject& a={}){const auto r=w.execute(c,a);if(!r["ok"].toBool())qFatal("%s",QJsonDocument(r).toJson().constData());return r["result"].toObject();}
 QJsonObject session(AlloyWindow& w){return w.state()["activeSession"].toObject();}
 QJsonObject draft(AlloyWindow& w){return session(w)["draftSketch"].toObject();}
 int circles(AlloyWindow& w){int count=0;for(const auto& e:draft(w)["parameters"].toObject()["entities"].toArray())count+=e.toObject()["type"]=="circle";return count;}
private slots:
 void nativeSnapsPreviewRepeatRefusalAndExit(){
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1500,950);w.show();w.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&w));QApplication::setActiveWindow(&w);
  ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;QVERIFY(view);view->setFocus();
  call(w,"sketch.begin",{{"plane","XY"}});call(w,"view.zoom",{{"factor",0.45}});
  QJsonArray sourceIds;for(const auto& p:{QPointF(-20,0),QPointF(0,20),QPointF(20,0)}){call(w,"sketch.addPoint",{{"x",p.x()},{"y",p.y()},{"units","mm"}});sourceIds.append(draft(w)["parameters"].toObject()["entities"].toArray().last().toObject()["pointId"]);}
  auto pixel=[&](double u,double v){const auto p=call(w,"view.projectSketchPoint",{{"sketchId",draft(w)["id"]},{"uMm",u},{"vMm",v}});return QPoint(qRound(p["x"].toDouble()),qRound(p["y"].toDouble()));};
  auto click=[&](double u,double v){QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(u,v));};
  auto move=[&](double u,double v){const auto p=pixel(u,v);QMouseEvent e(QEvent::MouseMove,QPointF(p),QPointF(view->mapToGlobal(p)),Qt::NoButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(view,&e);};
  const auto output=qEnvironmentVariable("ALLOY_THREE_POINT_CIRCLE_EVIDENCE");if(!output.isEmpty())QDir().mkpath(output);auto capture=[&](const QString& name){if(!output.isEmpty()){QTest::qWait(100);call(w,"capture.screenshot",{{"path",output+"/"+name+".png"}});}};
  auto* action=w.findChild<QAction*>("sketch.tool.threePointCircle");QVERIFY(action);action->trigger();QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
  move(-20,0);QCOMPARE(session(w)["snap"].toObject()["status"].toString(),QString("acquired"));click(-20,0);QVERIFY(session(w).contains("firstPoint"));click(0,20);QVERIFY(session(w).contains("secondPoint"));
  const auto before=draft(w);move(20,0);QCOMPARE(session(w)["snap"].toObject()["target"].toObject()["pointId"],sourceIds[2]);QCOMPARE(draft(w),before);capture("01-snapped-circle-preview");click(20,0);QCOMPARE(circles(w),1);QCOMPARE(session(w)["tool"].toString(),QString("threePointCircle"));QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
  const auto circle=draft(w)["parameters"].toObject()["entities"].toArray().last().toObject();int retained=0;for(const auto& row:draft(w)["parameters"].toObject()["constraints"].toArray()){const auto c=row.toObject();if(c["type"]=="pointOnCurve"){QVERIFY(sourceIds.contains(c["pointIds"].toArray().first()));QCOMPARE(c["entityIds"].toArray(),QJsonArray{circle["id"]});++retained;}}QCOMPARE(retained,3);capture("02-retained-circle");
  click(50,0);click(70,0);const auto beforeInvalid=draft(w);click(90,0);QCOMPARE(draft(w),beforeInvalid);QVERIFY(session(w).contains("secondPoint"));move(70,20);capture("03-free-circle-preview");click(70,20);QCOMPARE(circles(w),2);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
  // Repeated creation uses the current tool; Enter ends it after preserving circles.
  QTest::keyClick(view,Qt::Key_Return);QCOMPARE(session(w)["tool"].toString(),QString("select"));QCOMPARE(circles(w),2);
  action->trigger();QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));click(-60,0);click(-40,0);move(-40,20);const auto beforeCancel=draft(w);QTest::keyClick(view,Qt::Key_Escape);QCOMPARE(session(w)["tool"].toString(),QString("select"));QCOMPARE(draft(w),beforeCancel);QVERIFY(!session(w).contains("firstPoint") && !session(w).contains("secondPoint"));capture("04-cancel-preserves-circles");
  call(w,"sketch.cancel");w.close();
 }
};
QTEST_MAIN(ThreePointCircleControlsTest)
#include "three_point_circle_controls.moc"
