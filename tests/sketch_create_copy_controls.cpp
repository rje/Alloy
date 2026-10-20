// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QAction>
#include <QCheckBox>
#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonDocument>
#include <QMenu>
#include <QMouseEvent>
#include <QPushButton>
#include <QSpinBox>
#include <QTest>
#include <QTimer>
#include <QToolButton>
#include <cmath>
#include <numbers>
class SketchCreateCopyControls final:public QObject{
 Q_OBJECT
 QJsonArray transcript_;
 QJsonObject call(AlloyWindow& w,const QString& id,const QJsonObject& a={}){
  auto response=w.execute(id,a);transcript_.append(QJsonObject{{"command",id},{"args",a},{"response",response}});
  if(!response["ok"].toBool())qFatal("%s",QJsonDocument(response).toJson().constData());
  const auto initial=response["result"].toObject();QElapsedTimer timer;timer.start();
  while(response["completion"]=="pending"){if(timer.elapsed()>20000)qFatal("Pending command timeout");QTest::qWait(10);response=w.execute("command.status",{{"session",initial["id"]},{"generation",initial["previewGeneration"]}});if(!response["ok"].toBool())qFatal("%s",QJsonDocument(response).toJson().constData());}
  return response["result"].toObject();
 }
 QJsonObject active(AlloyWindow& w){return w.state()["activeSession"].toObject();}
 QJsonObject draft(AlloyWindow& w){return active(w)["draftSketch"].toObject();}
 QJsonObject interaction(AlloyWindow& w){return active(w)["patternInteraction"].toObject();}
 QJsonObject gpu(AlloyWindow& w){return w.state()["viewport"].toObject()["gpu"].toObject();}
 void click(QWidget* widget,const QPoint& at){QVERIFY(widget && widget->isVisible());const auto global=widget->mapToGlobal(at);auto* hit=QApplication::widgetAt(global);QVERIFY(hit==widget || widget->isAncestorOf(hit));QTest::mouseClick(hit,Qt::LeftButton,Qt::NoModifier,hit->mapFromGlobal(global));}
 void click(QWidget* widget){click(widget,widget->rect().center());}
private slots:
 void nativeCreateCopy(){
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1500,950);w.show();w.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&w));QApplication::setActiveWindow(&w);
  // Seed a different user clipboard; Create Copy must never replace it.
  call(w,"sketch.begin",{{"plane","XY"}});call(w,"sketch.rectangle",{{"x1",-20},{"y1",0},{"x2",-10},{"y2",5},{"units","mm"}});QJsonArray seedIds;for(const auto& row:draft(w)["parameters"].toObject()["entities"].toArray())seedIds.append(row.toObject()["id"]);call(w,"sketch.copy",{{"entityIds",seedIds}});call(w,"sketch.cancel");const auto clipboard=call(w,"sketch.clipboardInspect").value("clipboard");
  call(w,"sketch.begin",{{"plane","XY"},{"name","Move Copy independent profiles"}});call(w,"sketch.rectangle",{{"x1",10},{"y1",8},{"x2",30},{"y2",20},{"units","mm"}});QJsonArray ids;for(const auto& row:draft(w)["parameters"].toObject()["entities"].toArray())ids.append(row.toObject()["id"]);const auto sketchId=draft(w)["id"].toString();
  call(w,"sketch.dimension",{{"kind","horizontal"},{"entityId",ids[0]},{"value",20},{"units","mm"},{"annotation",QJsonObject{{"uMm",20},{"vMm",4}}}});call(w,"sketch.dimension",{{"kind","vertical"},{"entityId",ids[1]},{"value",12},{"units","mm"},{"annotation",QJsonObject{{"uMm",34},{"vMm",14}}}});call(w,"sketch.lookAt");call(w,"view.zoom",{{"factor",.45}});const auto source=draft(w);const auto history=active(w).value("draftHistory");
  ReferenceView* view=nullptr;for(auto* widget:w.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(widget))view=v;QVERIFY(view);auto panel=[&]{return w.findChild<QDialog*>("sketch.move.dialog");};
  auto pixel=[&](double u,double v){const auto p=call(w,"view.projectSketchPoint",{{"sketchId",sketchId},{"uMm",u},{"vMm",v}});return QPoint(qRound(p["x"].toDouble()),qRound(p["y"].toDouble()));};
  auto* modify=w.findChild<QToolButton*>("solid.menu.modify");auto* menu=w.findChild<QMenu*>("solid.commands.modify");auto* action=w.findChild<QAction*>("solid.action.solid.moveDialog");QVERIFY(modify && menu && action);bool invoked=false;QTimer::singleShot(0,[&]{if(menu->isVisible() && QApplication::activePopupWidget()==menu){const auto at=menu->actionGeometry(action).center();auto* hit=QApplication::widgetAt(menu->mapToGlobal(at));if(hit==menu){invoked=true;QTest::mouseClick(hit,Qt::LeftButton,Qt::NoModifier,at);return;}}menu->close();});click(modify);QVERIFY(invoked);QTRY_VERIFY(panel());QTRY_VERIFY(!menu->isVisible());QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
  auto pick=[&]{for(const auto& xy:QJsonArray{QJsonArray{20,8},QJsonArray{30,14},QJsonArray{20,20},QJsonArray{10,14}}){const auto a=xy.toArray();click(view,pixel(a[0].toDouble(),a[1].toDouble()));}QCOMPARE(active(w)["moveInteraction"].toObject()["entityIds"].toArray().size(),4);};
  auto toggle=[&](const QString& name){auto* box=panel()->findChild<QCheckBox*>("sketch.move."+name);QVERIFY(box && box->isEnabled());click(box,QPoint(10,box->height()/2));};auto copied=[&]{return active(w)["pendingSketchMove"].toObject()["diagnostics"].toObject().value("copiedEntityIds");};
  auto enter=[&](QWidget* widget,const QString& value){click(widget);QVERIFY(widget==QApplication::focusWidget() || widget->isAncestorOf(QApplication::focusWidget()));QTest::keySequence(QApplication::focusWidget(),QKeySequence::SelectAll);QTest::keyClicks(QApplication::focusWidget(),value);QTest::keyClick(QApplication::focusWidget(),Qt::Key_Tab);};auto values=[&](const QString& x,const QString& y){enter(panel()->findChild<QDoubleSpinBox*>("sketch.move.x"),x);enter(panel()->findChild<QDoubleSpinBox*>("sketch.move.y"),y);QVERIFY(active(w).contains("pendingSketchMove"));};
  auto output=qEnvironmentVariable("ALLOY_CREATE_COPY_EVIDENCE");if(!output.isEmpty()){output=QDir(output).absolutePath();QDir().mkpath(output);}auto capture=[&](const QString& name){if(!output.isEmpty())call(w,"capture.screenshot",{{"path",output+"/"+name+".png"}});};
  pick();QVERIFY(!panel()->findChild<QCheckBox*>("sketch.move.createCopy")->isChecked());toggle("createCopy");QVERIFY(panel()->findChild<QCheckBox*>("sketch.move.createCopy")->isChecked());QVERIFY(!panel()->findChild<QCheckBox*>("sketch.move.retain")->isEnabled());values("35","0");const auto firstIds=copied();values("40","1");QCOMPARE(copied(),firstIds);values("35","0");QCOMPARE(copied(),firstIds);QCOMPARE(draft(w),source);QCOMPARE(active(w).value("draftHistory"),history);QCOMPARE(call(w,"sketch.clipboardInspect").value("clipboard"),clipboard);capture("create-copy-preview");
  toggle("createCopy");QCOMPARE(active(w)["pendingSketchMove"].toObject()["draft"].toObject()["parameters"].toObject()["entities"].toArray().size(),4);toggle("retain");QVERIFY(!panel()->findChild<QCheckBox*>("sketch.move.createCopy")->isEnabled());QCOMPARE(active(w)["pendingSketchMove"].toObject()["draft"].toObject()["parameters"].toObject()["moveGroups"].toArray().size(),1);toggle("retain");toggle("createCopy");QVERIFY(copied()!=firstIds);QCOMPARE(draft(w),source);click(panel()->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Cancel));QTRY_VERIFY(!panel());QCOMPARE(draft(w),source);QCOMPARE(active(w).value("draftHistory"),history);QCOMPARE(call(w,"sketch.clipboardInspect").value("clipboard"),clipboard);
  QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QTest::keyClick(view,Qt::Key_M);QTRY_VERIFY(panel());pick();toggle("createCopy");values("35","0");QTest::keyClick(QApplication::focusWidget(),Qt::Key_Escape);QTRY_VERIFY(!panel());QCOMPARE(draft(w),source);QCOMPARE(active(w).value("draftHistory"),history);
  QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QTest::keyClick(view,Qt::Key_M);QTRY_VERIFY(panel());pick();toggle("createCopy");values("35","0");capture("final-create-copy-preview");click(panel()->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok));QTRY_VERIFY(!panel());const auto final=draft(w);QCOMPARE(final["parameters"].toObject()["entities"].toArray().size(),8);QVERIFY(final["parameters"].toObject()["moveGroups"].toArray().isEmpty());QCOMPARE(call(w,"sketch.clipboardInspect").value("clipboard"),clipboard);call(w,"history.undo");QCOMPARE(draft(w),source);call(w,"history.redo");QCOMPARE(draft(w),final);
  call(w,"sketch.finish");auto inspection=call(w,"document.inspect");const auto sketchDocument=inspection.value("authored");call(w,"history.undo");call(w,"history.redo");QCOMPARE(call(w,"document.inspect").value("authored"),sketchDocument);const auto profiles=inspection["evaluation"].toObject()["sketches"].toArray().first().toObject()["profiles"].toArray();QCOMPARE(profiles.size(),2);for(const auto& row:profiles)call(w,"solid.extrude",{{"sketchId",sketchId},{"profileId",row.toObject()["profileId"]},{"distance",8},{"units","mm"}});inspection=call(w,"document.inspect");const auto bodies=inspection["evaluation"].toObject()["bodies"].toArray();QCOMPARE(bodies.size(),2);QJsonArray bodyIds;double volume=0;for(const auto& row:bodies){bodyIds.append(row.toObject()["bodyId"]);volume+=row.toObject()["volumeMm3"].toDouble();}QVERIFY(std::abs(volume-3840)<1e-6);call(w,"view.home");QTest::qWait(100);capture("two-create-copy-solids");
  if(!output.isEmpty()){call(w,"document.save",{{"path",output+"/copied-profiles.alloy"}});call(w,"document.exportStl",{{"path",output+"/copied-profiles.stl"},{"bodyIds",bodyIds},{"units","mm"},{"format","binary"},{"surfaceDeviationMm",.01},{"normalDeviationDeg",5}});call(w,"document.open",{{"path",output+"/copied-profiles.alloy"}});QCOMPARE(call(w,"document.inspect")["authored"],inspection["authored"]);QFile file(output+"/native-report.json");QVERIFY(file.open(QIODevice::WriteOnly));file.write(QJsonDocument(QJsonObject{{"passed",true},{"sketchId",sketchId},{"source",source},{"final",final},{"sourceEntityIds",ids},{"clipboard",clipboard},{"authored",inspection["authored"]},{"inspection",inspection},{"state",w.state()},{"transcript",transcript_}}).toJson());}call(w,"document.new",{{"discard",true}});w.close();
 }
};
QTEST_MAIN(SketchCreateCopyControls)
#include "sketch_create_copy_controls.moc"
