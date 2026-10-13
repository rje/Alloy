// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include "SketchSpline.h"
#include <QApplication>
#include <QTest>
#include <QAction>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QMouseEvent>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
class ControlSplineControlsTest final:public QObject {
 Q_OBJECT
 QJsonObject call(AlloyWindow& w,const QString& c,const QJsonObject& a={}){const auto r=w.execute(c,a);if(!r["ok"].toBool())qFatal("%s",QJsonDocument(r).toJson().constData());return r["result"].toObject();}
 QJsonObject session(AlloyWindow& w){return w.state()["activeSession"].toObject();}
 QJsonObject draft(AlloyWindow& w){return session(w)["draftSketch"].toObject();}
 QJsonObject control(AlloyWindow& w){for(const auto& v:draft(w)["parameters"].toObject()["entities"].toArray())if(v.toObject()["type"]=="controlSpline")return v.toObject();return {};}
private slots:
 void nativeCageAndComb(){
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1280,860);w.show();w.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&w));ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(child))view=v;QVERIFY(view);
  call(w,"sketch.create",{{"plane","XY"}});call(w,"sketch.splineAdd",{{"points",QJsonArray{QJsonObject{{"uMm",-35},{"vMm",-25}},QJsonObject{{"uMm",0},{"vMm",-15}},QJsonObject{{"uMm",35},{"vMm",-25}}}}});
  QApplication::setActiveWindow(&w);view->setFocus();QTest::qWait(150);QVERIFY(view->hasFocus()); // Initial harness activation, before demonstrated steps.
  auto pixel=[&](double u,double v){const auto p=view->projectSketchPoint(u,v,"XY");return QPoint(qRound(p["x"].toDouble()),qRound(p["y"].toDouble()));};
  auto* action=w.findChild<QAction*>("sketch.tool.controlSpline");QVERIFY(action);action->trigger();QVERIFY(view->hasFocus());auto* degree=w.findChild<QComboBox*>("sketch.spline.maximumDegree");QVERIFY(degree&&degree->isVisible());QCOMPARE(degree->currentData().toInt(),5);
  const auto before=draft(w);for(const auto p:{QPointF(-35,0),QPointF(0,20),QPointF(35,0)})QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(p.x(),p.y()));QCOMPARE(session(w)["pendingSplinePoints"].toArray().size(),3);QCOMPARE(draft(w),before);QVERIFY(view->diagnostics()["splineCagePreviewVisible"].toBool());
  auto* complete=w.findChild<QPushButton*>("sketch.spline.complete");QVERIFY(complete&&complete->isVisible());QTest::mouseClick(complete,Qt::LeftButton);QVERIFY(view->hasFocus());QCOMPARE(control(w)["pointIds"].toArray().size(),3);QTest::keyClick(view,Qt::Key_Escape);QCOMPARE(session(w)["tool"].toString(),QString("select"));QVERIFY(view->hasFocus());
  auto cages=view->splineHandles()["cages"].toArray();QCOMPARE(cages.size(),1);QCOMPARE(cages[0].toObject()["poles"].toArray().size(),3);QCOMPARE(cages[0].toObject()["legs"].toArray().size(),2);const auto ids=control(w)["pointIds"].toArray();auto pole=cages[0].toObject()["poles"].toArray()[1].toObject();QPoint from(qRound(pole["x"].toDouble()),qRound(pole["y"].toDouble()));const auto original=draft(w),history=session(w)["draftHistory"].toObject();QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,from);QCOMPARE(draft(w),original);QCOMPARE(session(w)["draftHistory"].toObject(),history);QVERIFY(view->hasFocus());
  auto move=[&](const QPoint& to){QMouseEvent e(QEvent::MouseMove,QPointF(to),QPointF(view->mapToGlobal(to)),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(view,&e);};const auto to=from+QPoint(12,-18);QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,from);move(to);QCOMPARE(draft(w),original);QVERIFY(session(w)["pendingSplineDrag"].toObject().contains("previewDraft"));QTest::keyClick(view,Qt::Key_Escape);QTest::mouseRelease(view,Qt::LeftButton,Qt::NoModifier,to);QCOMPARE(draft(w),original);QVERIFY(view->hasFocus());
  QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,from);move(to);QTest::mouseRelease(view,Qt::LeftButton,Qt::NoModifier,to);QVERIFY(draft(w)!=original);QCOMPARE(control(w)["pointIds"].toArray(),ids);QCOMPARE(session(w)["draftHistory"].toObject()["count"].toInt(),history["count"].toInt()+1);QVERIFY(view->hasFocus());
  const auto evaluated=SketchSpline::evaluate(draft(w)["parameters"].toObject(),control(w));const auto on=evaluated.curve->Value(.35);QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(on.X(),on.Y()));QCOMPARE(session(w)["selection"].toObject()["entityIds"].toArray(),QJsonArray{control(w)["id"]});auto* comb=w.findChild<QCheckBox*>("sketch.spline.curvatureComb");QVERIFY(comb&&comb->isVisible()&&!comb->isChecked());const auto points=draft(w)["parameters"].toObject()["points"].toArray();QTest::mouseClick(comb,Qt::LeftButton,Qt::NoModifier,QPoint(10,comb->height()/2));QVERIFY(comb->isChecked()&&view->hasFocus());QCOMPARE(draft(w)["parameters"].toObject()["points"].toArray(),points);QCOMPARE(view->splineHandles()["curvatureCombs"].toArray().size(),1);
  auto* scale=w.findChild<QDoubleSpinBox*>("sketch.spline.combScale");auto* density=w.findChild<QSpinBox*>("sketch.spline.combDensity");QVERIFY(scale&&density&&scale->isEnabled());QTest::mouseClick(scale,Qt::LeftButton);QTest::keyClick(scale,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(scale,"0.10");QTest::keyClick(scale,Qt::Key_Return);QTest::mouseClick(density,Qt::LeftButton);QTest::keyClick(density,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(density,"16");QTest::keyClick(density,Qt::Key_Return);QCOMPARE(control(w)["curvatureComb"].toObject()["density"].toInt(),16);QCOMPARE(control(w)["curvatureComb"].toObject()["scale"].toDouble(),.1);
  QJsonObject fit;for(const auto& v:draft(w)["parameters"].toObject()["entities"].toArray())if(v.toObject()["type"]=="fitSpline")fit=v.toObject();const auto fitEval=SketchSpline::evaluate(draft(w)["parameters"].toObject(),fit);const auto fitOn=fitEval.curve->Value(.3);QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(fitOn.X(),fitOn.Y()));QCOMPARE(session(w)["selection"].toObject()["entityIds"].toArray(),QJsonArray{fit["id"]});QVERIFY(comb->isVisible()&&!comb->isChecked());QTest::mouseClick(comb,Qt::LeftButton,Qt::NoModifier,QPoint(10,comb->height()/2));QVERIFY(view->hasFocus());QCOMPARE(view->splineHandles()["curvatureCombs"].toArray().size(),2);
  const auto evidence=qEnvironmentVariable("ALLOY_CONTROL_SPLINE_EVIDENCE");if(!evidence.isEmpty()){QDir().mkpath(evidence);call(w,"capture.screenshot",{{"path",evidence+"/native-control-comb.png"}});QFile state(evidence+"/native-control-state.json");QVERIFY(state.open(QIODevice::WriteOnly));state.write(QJsonDocument(w.state()).toJson(QJsonDocument::Indented));}
  const auto edited=draft(w);action->trigger();QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(-30,30));QTest::keyClick(view,Qt::Key_Escape);QCOMPARE(draft(w),edited);QVERIFY(!session(w).contains("pendingSplinePoints"));call(w,"sketch.cancel");w.hide();
 }
};
QTEST_MAIN(ControlSplineControlsTest)
#include "control_point_spline_controls.moc"
