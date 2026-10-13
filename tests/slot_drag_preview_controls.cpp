// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QTest>
#include <QMouseEvent>
#include <QJsonDocument>
#include <QDir>
#include <QFile>
#include <cmath>
#include <numbers>
class SlotDragPreviewTest final:public QObject {
 Q_OBJECT
 QJsonObject call(AlloyWindow& w,const QString& c,const QJsonObject& a={}){const auto r=w.execute(c,a);if(!r["ok"].toBool())qFatal("%s",QJsonDocument(r).toJson().constData());return r["result"].toObject();}
 QJsonObject state(AlloyWindow& w){return w.state()["activeSession"].toObject();}
 QJsonObject draft(AlloyWindow& w){return state(w)["draftSketch"].toObject();}
private slots:
 void solvedTemporaryCapsule(){
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1280,860);w.show();w.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&w));ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(child))view=v;QVERIFY(view);
  call(w,"sketch.begin",{{"plane","XY"}});call(w,"sketch.centerPointSlot",{{"centerX",0},{"centerY",0},{"endX",35},{"endY",5},{"width",18},{"units","mm"},{"centerSnapTarget",QJsonObject{{"kind","origin"}}}});
  QApplication::setActiveWindow(&w);view->setFocus();QTest::qWait(100);QVERIFY(view->hasFocus()); // Initial activation only.
  const auto before=draft(w),history=state(w)["draftHistory"].toObject();const auto group=before["parameters"].toObject()["slotGroups"].toArray().first().toObject();QJsonObject guide;for(const auto& v:before["parameters"].toObject()["entities"].toArray())if(v.toObject()["id"]==group["centerlineId"])guide=v.toObject();const auto endpoint=guide["endPointId"].toString();
  auto pixel=[&](double u,double v){const auto p=view->projectSketchPoint(u,v,"XY");return QPoint(qRound(p["x"].toDouble()),qRound(p["y"].toDouble()));};
  auto pointPixel=[&]{for(const auto& v:draft(w)["parameters"].toObject()["points"].toArray())if(v.toObject()["id"]==endpoint)return pixel(v.toObject()["uMm"].toDouble(),v.toObject()["vMm"].toDouble());return QPoint();};
  auto motion=[&](QPoint to){QMouseEvent e(QEvent::MouseMove,QPointF(to),QPointF(view->mapToGlobal(to)),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(view,&e);};
  const auto from=pointPixel();QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,from);QCOMPARE(draft(w),before);QCOMPARE(state(w)["draftHistory"].toObject(),history);QVERIFY(view->hasFocus());
  QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,from);QVERIFY(state(w).contains("pendingSlotDrag"));const auto first=from+QPoint(16,-14),second=from+QPoint(30,-28);motion(first);auto preview1=state(w)["pendingSlotDrag"].toObject();QVERIFY(preview1.contains("previewDraft"));QCOMPARE(draft(w),before);QCOMPARE(state(w)["draftHistory"].toObject(),history);QVERIFY(view->hasFocus());motion(second);auto preview=state(w)["pendingSlotDrag"].toObject();QVERIFY(preview["previewDraft"].toObject()!=preview1["previewDraft"].toObject());QCOMPARE(draft(w),before);QCOMPARE(state(w)["draftHistory"].toObject(),history);QVERIFY(view->diagnostics()["slotDragPreviewVisible"].toBool());QVERIFY(!view->diagnostics()["cursorPreviewVisible"].toBool());
  const auto geometry=preview["previewGeometry"].toObject();QCOMPARE(geometry["profiles"].toArray().size(),1);QJsonObject previewGuide;for(const auto& e:geometry["entities"].toArray())if(e.toObject()["id"]==group["centerlineId"])previewGuide=e.toObject();double radius=0;for(const auto& e:geometry["entities"].toArray())if(e.toObject()["id"]==group["arcIds"].toArray().first()){radius=e.toObject()["radiusMm"].toDouble();QVERIFY(std::abs(std::abs(e.toObject()["sweepAngleDeg"].toDouble())-180)<1e-6);}QVERIFY(radius>0);const auto expectedArea=2*radius*previewGuide["lengthMm"].toDouble()+radius*radius*std::numbers::pi;qInfo()<<"Solved radius, guide length, actual area"<<radius<<previewGuide["lengthMm"].toDouble()<<geometry["profiles"].toArray()[0].toObject()["areaMm2"].toDouble();QVERIFY(std::abs(geometry["profiles"].toArray()[0].toObject()["areaMm2"].toDouble()-expectedArea)<1e-4);QVERIFY(preview["solveElapsedMs"].toDouble()<500);qInfo()<<"Two preview solve times ms"<<preview1["solveElapsedMs"].toDouble()<<preview["solveElapsedMs"].toDouble();
  const auto output=qEnvironmentVariable("ALLOY_SLOT_DRAG_EVIDENCE");if(!output.isEmpty()){QDir().mkpath(output);call(w,"capture.screenshot",{{"path",output+"/solved-intermediate-slot.png"}});QFile f(output+"/intermediate-state.json");QVERIFY(f.open(QIODevice::WriteOnly));f.write(QJsonDocument(w.state()).toJson(QJsonDocument::Indented));}
  QTest::mouseRelease(view,Qt::LeftButton,Qt::NoModifier,second);const auto committed=draft(w);QVERIFY(committed!=before);QVERIFY(!state(w).contains("pendingSlotDrag"));QCOMPARE(state(w)["draftHistory"].toObject()["count"].toInt(),history["count"].toInt()+1);QCOMPARE(committed["parameters"].toObject()["slotGroups"].toArray().first().toObject(),group);QVERIFY(view->hasFocus());
  auto cancelGesture=[&](const QString& command,const QJsonObject& args={}){const auto base=draft(w),priorHistory=state(w)["draftHistory"].toObject();const auto press=pointPixel(),to=press+QPoint(-18,-15);QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,press);motion(to);QVERIFY(state(w)["pendingSlotDrag"].toObject().contains("previewDraft"));if(command.isEmpty())QTest::keyClick(view,Qt::Key_Escape);else call(w,command,args);QVERIFY(!state(w).contains("pendingSlotDrag"));QTest::mouseRelease(view,Qt::LeftButton,Qt::NoModifier,to);QCOMPARE(draft(w),base);QCOMPARE(state(w)["draftHistory"].toObject(),priorHistory);QVERIFY(!view->diagnostics()["slotDragPreviewVisible"].toBool());};
  cancelGesture({});cancelGesture("sketch.tool",{{"tool","line"}});call(w,"sketch.tool",{{"tool","select"}});QVERIFY(view->hasFocus());
  auto press=pointPixel();QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,press);motion(press+QPoint(15,-10));call(w,"history.undo");QCOMPARE(draft(w),before);QVERIFY(!state(w).contains("pendingSlotDrag"));QTest::mouseRelease(view,Qt::LeftButton,Qt::NoModifier,press+QPoint(15,-10));QCOMPARE(draft(w),before);call(w,"history.redo");QCOMPARE(draft(w),committed);
  press=pointPixel();QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,press);motion(pixel(100100,0));QVERIFY(!state(w).contains("pendingSlotDrag"));QTest::mouseRelease(view,Qt::LeftButton,Qt::NoModifier,pixel(100100,0));QCOMPARE(draft(w),committed);QVERIFY(!view->diagnostics()["slotDragPreviewVisible"].toBool());
  call(w,"sketch.slotDrag",{{"phase","begin"},{"pointId",endpoint}});const auto refusal=w.execute("sketch.slotDrag",{{"phase","move"},{"uMm",100001},{"vMm",0}});QVERIFY(!refusal["ok"].toBool());QVERIFY(!state(w).contains("pendingSlotDrag"));QVERIFY(!w.execute("sketch.slotDrag",{{"phase","end"},{"uMm",35},{"vMm",5}})["ok"].toBool());QCOMPARE(draft(w),committed);
  press=pointPixel();QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,press);motion(press+QPoint(14,-10));call(w,"sketch.cancel");QVERIFY(w.state()["activeSession"].toObject().isEmpty());QTest::mouseRelease(view,Qt::LeftButton,Qt::NoModifier,press+QPoint(14,-10));QVERIFY(call(w,"document.inspect")["authored"].toObject()["features"].toArray().isEmpty());w.close();
 }
};
QTEST_MAIN(SlotDragPreviewTest)
#include "slot_drag_preview_controls.moc"
