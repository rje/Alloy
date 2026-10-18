// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QAction>
#include <QApplication>
#include <QDir>
#include <QJsonDocument>
#include <QLineEdit>
#include <QMouseEvent>
#include <QTest>
#include <cmath>
class ThreePointRectangleControlsTest final:public QObject {
 Q_OBJECT
 QJsonObject call(AlloyWindow& w,const QString& c,const QJsonObject& a={}){const auto r=w.execute(c,a);if(!r["ok"].toBool())qFatal("%s",QJsonDocument(r).toJson().constData());return r["result"].toObject();}
 QJsonObject session(AlloyWindow& w){return w.state()["activeSession"].toObject();}
 QJsonObject draft(AlloyWindow& w){return session(w)["draftSketch"].toObject();}
private slots:
 void nativeOrderDistancesRepeatCancel(){
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1500,950);w.show();w.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&w));QApplication::setActiveWindow(&w);
  ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;QVERIFY(view);view->setFocus();
  call(w,"sketch.begin",{{"plane","XY"}});call(w,"view.zoom",{{"factor",0.65}});
  auto pixel=[&](double u,double v){const auto p=call(w,"view.projectSketchPoint",{{"sketchId",draft(w)["id"]},{"uMm",u},{"vMm",v}});return QPoint(qRound(p["x"].toDouble()),qRound(p["y"].toDouble()));};
  auto click=[&](double u,double v){QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(u,v));};
  auto move=[&](double u,double v){const auto p=pixel(u,v);QMouseEvent e(QEvent::MouseMove,QPointF(p),QPointF(view->mapToGlobal(p)),Qt::NoButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(view,&e);};
  const auto output=qEnvironmentVariable("ALLOY_THREE_POINT_RECTANGLE_EVIDENCE");if(!output.isEmpty())QDir().mkpath(output);
  auto capture=[&](const QString& name){if(!output.isEmpty()){QTest::qWait(100);call(w,"capture.screenshot",{{"path",output+"/"+name+".png"}});}};
  auto* action=w.findChild<QAction*>("sketch.tool.threePointRectangle");QVERIFY(action);action->trigger();QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
  click(0,0);QVERIFY(session(w).contains("firstPoint"));move(24,18);QCOMPARE(draft(w)["parameters"].toObject()["entities"].toArray().size(),0);
  QTest::keyClicks(view,"3");auto* input=w.findChild<QLineEdit*>("sketch.threePointRectangle.distance");QVERIFY(input && input->isVisible());QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(input));QTest::keyClicks(input,"0");QTest::keyClick(input,Qt::Key_Tab);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QVERIFY(input->isReadOnly());
  click(24,18);QVERIFY(session(w).contains("secondPoint"));QVERIFY(!input->isVisible());move(15,30);capture("01-rotated-preview");QCOMPARE(draft(w)["parameters"].toObject()["entities"].toArray().size(),0);
  QTest::keyClicks(view,"1");QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(input));QTest::keyClicks(input,"5");QTest::keyClick(input,Qt::Key_Tab);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));capture("02-second-distance-locked");click(15,30);
  const auto created=draft(w);QCOMPARE(created["parameters"].toObject()["entities"].toArray().size(),4);const auto dims=created["parameters"].toObject()["dimensions"].toArray();QCOMPARE(dims.size(),2);QCOMPARE(dims[0].toObject()["valueMm"].toDouble(),30.0);QCOMPARE(dims[1].toObject()["valueMm"].toDouble(),15.0);QCOMPARE(session(w)["tool"].toString(),QString("threePointRectangle"));QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));capture("03-completed");
  click(50,0);move(60,10);QTest::keyClicks(view,"0");QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(input));QTest::keyClick(input,Qt::Key_Tab);QVERIFY(!input->isReadOnly());click(60,10);QCOMPARE(draft(w),created);QVERIFY(!session(w).contains("secondPoint"));QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QTest::keyClick(view,Qt::Key_Escape);QCOMPARE(session(w)["tool"].toString(),QString("select"));QVERIFY(!input->isVisible());QCOMPARE(draft(w),created);
  action->trigger();QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));click(50,0);click(60,10);move(70,0);click(70,0);QCOMPARE(draft(w)["parameters"].toObject()["entities"].toArray().size(),8);QTest::keyClick(view,Qt::Key_Escape);QCOMPARE(session(w)["tool"].toString(),QString("select"));
  call(w,"sketch.dimensionEdit",{{"dimensionId",dims[0].toObject()["id"]}});auto* editor=w.findChild<QLineEdit*>("sketch.inlineValue");QVERIFY(editor && editor->isVisible());QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(editor));QTest::keyClick(editor,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(editor,"36");QTest::keyClick(editor,Qt::Key_Return);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QCOMPARE(draft(w)["parameters"].toObject()["dimensions"].toArray()[0].toObject()["valueMm"].toDouble(),36.0);capture("04-native-dimension-edit");
  call(w,"sketch.cancel");w.close();
 }
};
QTEST_MAIN(ThreePointRectangleControlsTest)
#include "three_point_rectangle_controls.moc"
