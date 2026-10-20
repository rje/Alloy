// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QDir>
#include <QJsonDocument>
#include <QMouseEvent>
#include <QTest>
#include <QWindow>
#include <QToolButton>
#include <QMenu>
#include <QTimer>
class SketchBreakControls final:public QObject {
 Q_OBJECT
 QJsonObject call(AlloyWindow& w,const QString& c,const QJsonObject& a={}){auto r=w.executeApi(c,a);if(!r.value("ok").toBool())qFatal("%s",QJsonDocument(r).toJson(QJsonDocument::Compact).constData());return r.value("result").toObject();}
 QJsonObject session(AlloyWindow& w){return w.state().value("activeSession").toObject();}
private slots:
 void nativeCircle(){
  const auto out=qEnvironmentVariable("ALLOY_SKETCH_BREAK_EVIDENCE");QVERIFY(!out.isEmpty());QVERIFY(QDir().mkpath(out));
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1500,950);w.show();QVERIFY(QTest::qWaitForWindowExposed(&w));ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* found=dynamic_cast<ReferenceView*>(child))view=found;QVERIFY(view);
  w.raise();w.activateWindow();w.windowHandle()->requestActivate();QApplication::setActiveWindow(&w);QTRY_VERIFY(w.isActiveWindow());
  call(w,"sketch.begin",{{"plane","XY"},{"name","Analytic Break"},{"units","mm"}});
  call(w,"sketch.addCircle",{{"centerX",0},{"centerY",0},{"diameter",20},{"units","mm"}});
  call(w,"sketch.line",{{"x1",-15},{"y1",0},{"x2",15},{"y2",0},{"units","mm"}});call(w,"sketch.line",{{"x1",30},{"y1",0},{"x2",50},{"y2",0},{"units","mm"}});
  call(w,"sketch.line",{{"x1",40},{"y1",-15},{"x2",40},{"y2",15},{"units","mm"}});
  call(w,"sketch.addArc",{{"mode","center"},{"centerX",70},{"centerY",0},{"radius",10},{"startAngleDeg",0},{"sweepAngleDeg",180},{"units","mm"},{"session",session(w).value("id")}});
  call(w,"sketch.line",{{"x1",70},{"y1",-15},{"x2",70},{"y2",15},{"units","mm"}});call(w,"sketch.lookAt");
  const auto before=session(w).value("draftSketch").toObject();const auto id=before.value("id");
  auto pixel=[&](double u,double v){auto p=call(w,"view.projectSketchPoint",{{"sketchId",id},{"uMm",u},{"vMm",v}});return QPoint(qRound(p.value("x").toDouble()),qRound(p.value("y").toDouble()));};
  auto* modify=w.findChild<QToolButton*>("sketch.menu.modify");QVERIFY(modify);auto* action=w.findChild<QAction*>("sketch.tool.break");QVERIFY(action);bool active=false;
  QTimer::singleShot(100,&w,[&]{auto* menu=modify->menu();active=QApplication::activePopupWidget()==menu&&menu->isActiveWindow();if(active)QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,menu->actionGeometry(action).center());else menu->close();});
  QTest::mouseClick(modify,Qt::LeftButton);QVERIFY(active);QTRY_VERIFY(!modify->menu()->isVisible());QCOMPARE(session(w).value("tool"),QJsonValue("break"));
  const auto at=pixel(0,10);QCOMPARE(QApplication::widgetAt(view->mapToGlobal(at)),static_cast<QWidget*>(view));
  QMouseEvent hover(QEvent::MouseMove,QPointF(at),QPointF(view->mapToGlobal(at)),Qt::NoButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(view,&hover);QTest::qWait(100);
  const auto pending=session(w).value("pendingBreak").toObject();QCOMPARE(pending.value("diagnostics").toObject().value("breakPoints").toArray().size(),2);QCOMPARE(session(w).value("draftSketch"),QJsonValue(before));
  call(w,"capture.screenshot",{{"path",out+"/circle-break-preview.png"}});
  QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QTest::keyClick(view,Qt::Key_Escape);QCOMPARE(session(w).value("tool"),QJsonValue("select"));QVERIFY(!session(w).contains("pendingBreak"));QCOMPARE(session(w).value("draftSketch"),QJsonValue(before));
  call(w,"sketch.tool",{{"tool","break"}});QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QApplication::sendEvent(view,&hover);QVERIFY(session(w).contains("pendingBreak"));
  QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,at);auto after=session(w).value("draftSketch").toObject();QCOMPARE(after.value("parameters").toObject().value("entities").toArray().size(),7);QCOMPARE(session(w).value("tool"),QJsonValue("break"));QVERIFY(!session(w).contains("pendingBreak"));
  for(const auto& coords:QVector<QPointF>{{45,0},{77,7}}){const auto p=pixel(coords.x(),coords.y());QCOMPARE(QApplication::widgetAt(view->mapToGlobal(p)),static_cast<QWidget*>(view));
    QMouseEvent move(QEvent::MouseMove,QPointF(p),QPointF(view->mapToGlobal(p)),Qt::NoButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(view,&move);QTest::qWait(50);
    QCOMPARE(session(w).value("pendingBreak").toObject().value("diagnostics").toObject().value("breakPoints").toArray().size(),1);
    call(w,"capture.screenshot",{{"path",out+QString("/break-preview-%1.png").arg(coords.x())}});
    QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,p);QCOMPARE(session(w).value("tool"),QJsonValue("break"));}
  after=session(w).value("draftSketch").toObject();QCOMPARE(after.value("parameters").toObject().value("entities").toArray().size(),9);
  QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QTest::keyClick(view,Qt::Key_Return);QCOMPARE(session(w).value("tool"),QJsonValue("select"));
  call(w,"capture.screenshot",{{"path",out+"/circle-break-result.png"}});
  call(w,"history.undo");call(w,"history.undo");call(w,"history.undo");QCOMPARE(session(w).value("draftSketch"),QJsonValue(before));call(w,"history.redo");call(w,"history.redo");call(w,"history.redo");QCOMPARE(session(w).value("draftSketch"),QJsonValue(after));
  call(w,"sketch.tool",{{"tool","break"}});QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QTest::keyClick(view,Qt::Key_Escape);QCOMPARE(session(w).value("tool"),QJsonValue("select"));QCOMPARE(session(w).value("draftSketch"),QJsonValue(after));
  call(w,"sketch.finish");auto authored=call(w,"document.inspect").value("authored");call(w,"document.save",{{"path",out+"/circle-break.alloy"}});call(w,"document.open",{{"path",out+"/circle-break.alloy"}});QCOMPARE(call(w,"document.inspect").value("authored"),authored);w.close();
 }
};
QTEST_MAIN(SketchBreakControls)
#include "sketch_break_controls.moc"
