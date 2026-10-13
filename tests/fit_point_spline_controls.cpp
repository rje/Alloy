// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include "SketchSpline.h"
#include <QApplication>
#include <QTest>
#include <QAction>
#include <QMenu>
#include <QTimer>
#include <QPushButton>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
class FitSplineControlsTest final:public QObject {
 Q_OBJECT
 QJsonObject call(AlloyWindow& w,const QString& command,const QJsonObject& a={}){const auto r=w.execute(command,a);if(!r["ok"].toBool())qFatal("%s",QJsonDocument(r).toJson().constData());return r["result"].toObject();}
 QJsonObject session(AlloyWindow& w){return w.state()["activeSession"].toObject();}
 QJsonObject draft(AlloyWindow& w){return session(w)["draftSketch"].toObject();}
 QJsonObject curve(AlloyWindow& w){return draft(w)["parameters"].toObject()["entities"].toArray().first().toObject();}
private slots:
 void sourceFitPicksDeleteInsertAndHandleWithoutFocusRepair(){
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1280,860);w.show();w.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&w));
  ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(child))view=v;QVERIFY(view);
  call(w,"sketch.create",{{"plane","XY"}});QApplication::setActiveWindow(&w);view->setFocus();QTest::qWait(150);QVERIFY(view->hasFocus()); // Initial harness activation only.
  auto pixel=[&](double u,double v){const auto p=view->projectSketchPoint(u,v,"XY");return QPoint(qRound(p["x"].toDouble()),qRound(p["y"].toDouble()));};
  const auto originalDispatch=view->dispatch;view->dispatch=[&](const QString& command,const QJsonObject& args){qInfo()<<command<<QJsonDocument(args).toJson(QJsonDocument::Compact);originalDispatch(command,args);};
  auto* action=w.findChild<QAction*>("sketch.tool.fitSpline");QVERIFY(action);action->trigger();QCOMPARE(session(w)["tool"].toString(),QString("fitSpline"));QVERIFY(view->hasFocus());
  for(const auto p:{QPointF(-40,0),QPointF(-20,25),QPointF(40,0)})QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(p.x(),p.y()));
  if(session(w)["pendingSplinePoints"].toArray().size()!=3)qWarning()<<QJsonDocument(w.state()).toJson(QJsonDocument::Compact);
  QCOMPARE(session(w)["pendingSplinePoints"].toArray().size(),3);QCOMPARE(draft(w)["parameters"].toObject()["entities"].toArray().size(),0);QVERIFY(view->hasFocus());
  auto* complete=w.findChild<QPushButton*>("sketch.spline.complete");QVERIFY(complete&&complete->isVisible()&&complete->isEnabled());QTest::mouseClick(complete,Qt::LeftButton);QCOMPARE(curve(w)["pointIds"].toArray().size(),3);QVERIFY(view->hasFocus());
  QTest::keyClick(view,Qt::Key_Escape);QCOMPARE(session(w)["tool"].toString(),QString("select"));QVERIFY(view->hasFocus());
  auto points=draft(w)["parameters"].toObject()["points"].toArray();const auto ids=curve(w)["pointIds"].toArray();const auto first=ids[0],last=ids[2];QJsonObject middle;for(const auto& p:points)if(p.toObject()["id"]==ids[1])middle=p.toObject();
  const auto beforePointPick=draft(w);const auto beforePickHistory=session(w)["draftHistory"].toObject();
  QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(middle["uMm"].toDouble(),middle["vMm"].toDouble()));QCOMPARE(session(w)["selection"].toObject()["pointIds"].toArray(),QJsonArray{ids[1]});QVERIFY(view->hasFocus());QCOMPARE(draft(w),beforePointPick);QCOMPARE(session(w)["draftHistory"].toObject(),beforePickHistory);
  QTest::keyClick(view,Qt::Key_Backspace);QCOMPARE(curve(w)["pointIds"].toArray(),(QJsonArray{first,last}));QVERIFY(view->hasFocus());
  auto eval=SketchSpline::evaluate(draft(w)["parameters"].toObject(),curve(w));auto on=eval.curve->Value(.5);const auto onPixel=pixel(on.X(),on.Y());QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,onPixel);QCOMPARE(session(w)["selection"].toObject()["entityIds"].toArray(),QJsonArray{curve(w)["id"]});QVERIFY(view->hasFocus());
  bool menuVisible=false;QTimer::singleShot(100,&w,[&]{auto* menu=qobject_cast<QMenu*>(QApplication::activePopupWidget());if(!menu)return;menuVisible=true;for(auto* option:menu->actions())if(option->objectName()=="sketch.spline.insertFitPoint")QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,menu->actionGeometry(option).center());});
  QContextMenuEvent context(QContextMenuEvent::Mouse,onPixel,view->mapToGlobal(onPixel));QApplication::sendEvent(view,&context);QVERIFY(menuVisible);QCOMPARE(session(w)["tool"].toString(),QString("insertFitPoint"));QVERIFY(view->hasFocus());
  QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,onPixel);QCOMPARE(curve(w)["pointIds"].toArray().size(),3);QCOMPARE(curve(w)["pointIds"].toArray().first(),first);QCOMPARE(curve(w)["pointIds"].toArray().last(),last);QCOMPARE(session(w)["tool"].toString(),QString("select"));QVERIFY(view->hasFocus());
  const auto prior=draft(w);const auto history=session(w)["draftHistory"].toObject()["count"].toInt();const auto grips=view->splineHandles()["grips"].toArray();QVERIFY(grips.size()==6);auto grip=grips[3].toObject();const QPoint from(qRound(grip["x"].toDouble()),qRound(grip["y"].toDouble())),to=from+QPoint(28,-24);
  QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,from);QVERIFY(session(w).contains("pendingSplineDrag"));QMouseEvent move(QEvent::MouseMove,QPointF(to),QPointF(view->mapToGlobal(to)),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(view,&move);QCOMPARE(draft(w),prior);QVERIFY(session(w)["pendingSplineDrag"].toObject().contains("previewDraft"));QVERIFY(view->hasFocus());
  QTest::mouseRelease(view,Qt::LeftButton,Qt::NoModifier,to);QVERIFY(!session(w).contains("pendingSplineDrag"));QVERIFY(draft(w)!=prior);QCOMPARE(session(w)["draftHistory"].toObject()["count"].toInt(),history+1);QVERIFY(view->hasFocus());
  const auto edited=draft(w);const auto editedGrips=view->splineHandles()["grips"].toArray();QVERIFY(editedGrips.size()==6);grip=editedGrips[3].toObject();const QPoint cancelFrom(qRound(grip["x"].toDouble()),qRound(grip["y"].toDouble()));QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,cancelFrom);const auto cancelTo=cancelFrom+QPoint(-15,12);QMouseEvent cancelMove(QEvent::MouseMove,QPointF(cancelTo),QPointF(view->mapToGlobal(cancelTo)),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(view,&cancelMove);QTest::keyClick(view,Qt::Key_Escape);QCOMPARE(draft(w),edited);QVERIFY(!session(w).contains("pendingSplineDrag"));QTest::mouseRelease(view,Qt::LeftButton,Qt::NoModifier,cancelTo);QCOMPARE(draft(w),edited);
  const auto evidence=qEnvironmentVariable("ALLOY_FIT_SPLINE_EVIDENCE");if(!evidence.isEmpty()){QDir().mkpath(evidence);call(w,"capture.screenshot",{{"path",evidence+"/native-fit-handles.png"}});QFile state(evidence+"/native-fit-state.json");QVERIFY(state.open(QIODevice::WriteOnly));state.write(QJsonDocument(w.state()).toJson(QJsonDocument::Indented));}
  grip=view->splineHandles()["grips"].toArray()[3].toObject();const QPoint undoFrom(qRound(grip["x"].toDouble()),qRound(grip["y"].toDouble()));QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,undoFrom);const auto undoTo=undoFrom+QPoint(20,16);QMouseEvent undoMove(QEvent::MouseMove,QPointF(undoTo),QPointF(view->mapToGlobal(undoTo)),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(view,&undoMove);call(w,"history.undo");QVERIFY(!session(w).contains("pendingSplineDrag"));QCOMPARE(draft(w),prior);QTest::mouseRelease(view,Qt::LeftButton,Qt::NoModifier,undoTo);QCOMPARE(draft(w),prior);call(w,"history.redo");QCOMPARE(draft(w),edited);
  action->trigger();QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(-30,-30));QTest::keyClick(view,Qt::Key_Escape);QCOMPARE(draft(w),edited);QVERIFY(!session(w).contains("pendingSplinePoints"));call(w,"sketch.cancel");w.hide();
 }
};
QTEST_MAIN(FitSplineControlsTest)
#include "fit_point_spline_controls.moc"
