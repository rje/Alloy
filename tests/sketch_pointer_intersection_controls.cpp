// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QDir>
#include <QJsonDocument>
#include <QMouseEvent>
#include <QProcess>
#include <QTest>
#include <QWindow>
class SketchIntersectionControls final:public QObject {
 Q_OBJECT
 QJsonObject call(AlloyWindow& w,const QString& name,const QJsonObject& args={}){const auto r=w.executeApi(name,args);if(!r.value("ok").toBool())qFatal("%s",QJsonDocument(r).toJson(QJsonDocument::Compact).constData());return r.value("result").toObject();}
 QJsonObject session(AlloyWindow& w){return w.state().value("activeSession").toObject();}
 QJsonObject parameters(AlloyWindow& w){return session(w).value("draftSketch").toObject().value("parameters").toObject();}
private slots:
 void pointerPair(){
  const auto out=qEnvironmentVariable("ALLOY_INTERSECTION_EVIDENCE");QVERIFY(!out.isEmpty());QVERIFY(QDir().mkpath(out));
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1500,950);w.show();QVERIFY(QTest::qWaitForWindowExposed(&w));
  ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(child))view=v;QVERIFY(view);
  call(w,"sketch.begin",{{"plane","XY"},{"name","Retained line intersection"},{"units","mm"}});
  QJsonArray sources;for(const auto& args:QList<QJsonObject>{{{"x1",-30},{"y1",10},{"x2",40},{"y2",10}},{{"x1",10},{"y1",-20},{"x2",10},{"y2",40}}}){
   call(w,"sketch.line",args);const auto e=parameters(w).value("entities").toArray().last().toObject();sources.append(e.value("id"));call(w,"sketch.construction",{{"entityIds",QJsonArray{e.value("id")}},{"construction",true}});call(w,"sketch.constraint",{{"type","fixed"},{"entityIds",QJsonArray{e.value("id")}}});}
  call(w,"sketch.lookAt");call(w,"view.zoom",{{"factor",0.75}});
#ifdef Q_OS_MACOS
  QProcess activate;activate.start("osascript",{"-e",QString("tell application \"System Events\" to set frontmost of first application process whose unix id is %1 to true").arg(QCoreApplication::applicationPid())});QVERIFY(activate.waitForFinished(5000));QCOMPARE(activate.exitCode(),0);
#endif
  w.raise();w.activateWindow();w.windowHandle()->requestActivate();QApplication::setActiveWindow(&w);QTRY_VERIFY(w.isActiveWindow());
  call(w,"sketch.tool",{{"tool","line"}});
  auto pixel=[&](double u,double v){const auto p=call(w,"view.projectSketchPoint",{{"plane","XY"},{"uMm",u},{"vMm",v}});return QPoint(qRound(p.value("x").toDouble()),qRound(p.value("y").toDouble()));};
  auto move=[&](QPoint point){QMouseEvent event(QEvent::MouseMove,QPointF(point),QPointF(view->mapToGlobal(point)),Qt::NoButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(view,&event);QTest::qWait(80);};
  const auto before=session(w).value("draftSketch");const auto target=pixel(10,10);move(target);
  QCOMPARE(session(w).value("snap").toObject().value("target").toObject().value("kind").toString(),QString("intersection"));
  QCOMPARE(session(w).value("draftSketch"),before);QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QTest::keyClick(view,Qt::Key_Escape);QCOMPARE(session(w).value("draftSketch"),before);
  call(w,"sketch.tool",{{"tool","line"}});QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(-10,-10));QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));move(target);
  QCOMPARE(session(w).value("snap").toObject().value("target").toObject().value("entityIds").toArray(),sources);
  call(w,"capture.screenshot",{{"path",out+"/native-intersection-candidate.png"}});QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,target);
  auto p=parameters(w);const auto relation=p.value("constraints").toArray().last().toObject();QCOMPARE(relation.value("type").toString(),QString("intersection"));QCOMPARE(relation.value("entityIds").toArray(),sources);
  QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QTest::keyClick(view,Qt::Key_Escape);call(w,"sketch.tool",{{"tool","select"}});
  auto gpu=call(w,"app.state").value("viewport").toObject().value("gpu").toObject();QJsonObject glyph;for(const auto& row:gpu.value("sketchConstraintGlyphs").toArray())if(row.toObject().value("constraintId")==relation.value("id"))glyph=row.toObject();QVERIFY(!glyph.isEmpty());
  call(w,"capture.screenshot",{{"path",out+"/native-retained-intersection.png"}});
  const auto bounds=glyph.value("bounds").toObject();const QPoint badge(qRound(bounds.value("x").toDouble()+bounds.value("width").toDouble()/2),qRound(bounds.value("y").toDouble()+bounds.value("height").toDouble()/2));
  QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,badge);QCOMPARE(session(w).value("selection").toObject().value("constraintIds").toArray(),QJsonArray{relation.value("id")});QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QTest::keyClick(view,Qt::Key_Delete);
  QVERIFY(!parameters(w).value("constraints").toArray().contains(relation));call(w,"history.undo");QCOMPARE(parameters(w),p);
  call(w,"sketch.finish");const auto authored=call(w,"document.inspect").value("authored");call(w,"document.save",{{"path",out+"/native-intersection.alloy"}});QCOMPARE(call(w,"document.inspect").value("authored"),authored);w.close();
 }
};
QTEST_MAIN(SketchIntersectionControls)
#include "sketch_pointer_intersection_controls.moc"
