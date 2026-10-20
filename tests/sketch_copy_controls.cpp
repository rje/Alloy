// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QAction>
#include <QStatusBar>
#include <QLabel>
#include <QContextMenuEvent>
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
class SketchCopyControls final:public QObject{
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
 void nativeCopyPaste(){
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1500,950);w.show();w.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&w));QApplication::setActiveWindow(&w);
  call(w,"sketch.begin",{{"plane","XY"},{"name","Independent copied profiles"}});call(w,"sketch.rectangle",{{"x1",10},{"y1",8},{"x2",30},{"y2",20},{"units","mm"}});const auto sketchId=draft(w)["id"].toString();QJsonArray ids;for(const auto& row:draft(w)["parameters"].toObject()["entities"].toArray())ids.append(row.toObject()["id"]);
  call(w,"sketch.dimension",{{"kind","horizontal"},{"entityId",ids[0]},{"value",20},{"units","mm"},{"annotation",QJsonObject{{"uMm",20},{"vMm",4}}}});call(w,"sketch.dimension",{{"kind","vertical"},{"entityId",ids[1]},{"value",12},{"units","mm"},{"annotation",QJsonObject{{"uMm",34},{"vMm",14}}}});call(w,"sketch.tool",{{"tool","select"}});call(w,"sketch.lookAt");call(w,"view.zoom",{{"factor",.45}});const auto source=draft(w);
  ReferenceView* view=nullptr;for(auto* widget:w.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(widget))view=v;QVERIFY(view);
  const auto dispatch=view->dispatch;view->dispatch=[&](const QString& command,const QJsonObject& args){dispatch(command,args);if(command=="sketch.copy")qInfo().noquote()<<"Copy dispatch"<<QJsonDocument(args).toJson(QJsonDocument::Compact)<<w.statusBar()->currentMessage()<<w.findChild<QLabel*>("sketch.error")->text();};
  auto pixel=[&](double u,double v){const auto p=call(w,"view.projectSketchPoint",{{"sketchId",sketchId},{"uMm",u},{"vMm",v}});return QPoint(qRound(p["x"].toDouble()),qRound(p["y"].toDouble()));};
  auto select=[&]{int n=0;for(const auto& xy:QJsonArray{QJsonArray{20,8},QJsonArray{30,14},QJsonArray{20,20},QJsonArray{10,14}}){const auto at=xy.toArray();const auto point=pixel(at[0].toDouble(),at[1].toDouble());auto* hit=QApplication::widgetAt(view->mapToGlobal(point));QVERIFY(hit==view);QTest::mouseClick(hit,Qt::LeftButton,n++ ? Qt::ShiftModifier : Qt::NoModifier,point);}QCOMPARE(active(w)["selection"].toObject()["entityIds"].toArray().size(),4);QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));};
  auto context=[&](const QPoint& at,const QString& actionName){QCOMPARE(QApplication::widgetAt(view->mapToGlobal(at)),static_cast<QWidget*>(view));QContextMenuEvent event(QContextMenuEvent::Mouse,at,view->mapToGlobal(at));QApplication::sendEvent(view,&event);auto* menu=qobject_cast<QMenu*>(QApplication::activePopupWidget());QVERIFY(menu && menu->objectName()=="context.sketchGeometry");auto* action=menu->findChild<QAction*>(actionName);QVERIFY(action && action->isEnabled());const auto point=menu->actionGeometry(action).center();auto* hit=QApplication::widgetAt(menu->mapToGlobal(point));QVERIFY(hit==menu);QTest::mouseClick(hit,Qt::LeftButton,Qt::NoModifier,point);QTRY_VERIFY(QApplication::activePopupWidget()==nullptr);};
  auto panel=[&](){return w.findChild<QDialog*>("sketch.paste.dialog");};auto enter=[&](QWidget* widget,const QString& value){click(widget);QVERIFY(widget==QApplication::focusWidget() || widget->isAncestorOf(QApplication::focusWidget()));QTest::keySequence(QApplication::focusWidget(),QKeySequence::SelectAll);QTest::keyClicks(QApplication::focusWidget(),value);QTest::keyClick(QApplication::focusWidget(),Qt::Key_Tab);};auto values=[&]{auto* p=panel();QVERIFY(p);enter(p->findChild<QDoubleSpinBox*>("sketch.paste.x"),"35");enter(p->findChild<QDoubleSpinBox*>("sketch.paste.y"),"0");QVERIFY(active(w).contains("pendingSketchPaste"));};
  auto output=qEnvironmentVariable("ALLOY_SKETCH_COPY_EVIDENCE");if(!output.isEmpty()){output=QDir(output).absolutePath();QDir().mkpath(output);}auto capture=[&](const QString& name){if(!output.isEmpty())call(w,"capture.screenshot",{{"path",output+"/"+name+".png"}});};
  select();const auto before=active(w);context(pixel(20,8),"sketch.action.copy");QTRY_VERIFY_WITH_TIMEOUT(call(w,"sketch.clipboardInspect")["available"].toBool(),2000);QCOMPARE(draft(w),source);context(pixel(45,8),"sketch.action.paste");QTRY_VERIFY(panel());values();QCOMPARE(draft(w),source);QCOMPARE(active(w).value("draftHistory"),before.value("draftHistory"));capture("context-paste-preview");click(panel()->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Cancel));QTRY_VERIFY(!panel());QCOMPARE(draft(w),source);QCOMPARE(active(w).value("selection"),before.value("selection"));QCOMPARE(active(w).value("draftHistory"),before.value("draftHistory"));QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
  call(w,"sketch.paste");QVERIFY(panel());call(w,"sketch.pasteCancel");QTRY_VERIFY(!panel());QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
  QTest::keySequence(view,QKeySequence::Copy);QTest::keySequence(view,QKeySequence::Paste);QTRY_VERIFY(panel());values();QTest::keyClick(QApplication::focusWidget(),Qt::Key_Escape);QTRY_VERIFY(!panel());QCOMPARE(draft(w),source);QCOMPARE(active(w).value("draftHistory"),before.value("draftHistory"));QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
  QTest::keySequence(view,QKeySequence::Paste);QTRY_VERIFY(panel());values();QTest::keyClick(QApplication::focusWidget(),Qt::Key_Return);QTRY_VERIFY(!panel());const auto entered=draft(w);QCOMPARE(entered["parameters"].toObject()["entities"].toArray().size(),8);QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));call(w,"history.undo");QCOMPARE(draft(w),source);call(w,"history.redo");QCOMPARE(draft(w),entered);call(w,"history.undo");QCOMPARE(draft(w),source);
  // A separate explicit user selection starts the click-confirm trial.
  select();QTest::keySequence(view,QKeySequence::Copy);QTest::keySequence(view,QKeySequence::Paste);QTRY_VERIFY(panel());const auto at=pixel(45,8);QTest::mouseMove(view,at);QTRY_VERIFY(active(w).contains("pendingSketchPaste"));const auto pointer=active(w)["pasteInteraction"].toObject();QVERIFY(std::abs(pointer["xMm"].toDouble()-35)<.2);QVERIFY(std::abs(pointer["yMm"].toDouble())<.2);values();capture("keyboard-paste-preview");click(view,at);QTRY_VERIFY(!panel());const auto final=draft(w);QCOMPARE(final["parameters"].toObject()["entities"].toArray().size(),8);QCOMPARE(active(w)["selection"].toObject()["entityIds"].toArray().size(),4);call(w,"sketch.finish");auto inspection=call(w,"document.inspect");const auto profiles=inspection["evaluation"].toObject()["sketches"].toArray().first().toObject()["profiles"].toArray();QCOMPARE(profiles.size(),2);for(const auto& row:profiles)call(w,"solid.extrude",{{"sketchId",sketchId},{"profileId",row.toObject()["profileId"]},{"distance",8},{"units","mm"}});inspection=call(w,"document.inspect");const auto bodies=inspection["evaluation"].toObject()["bodies"].toArray();QCOMPARE(bodies.size(),2);QJsonArray bodyIds;double volume=0;for(const auto& row:bodies){bodyIds.append(row.toObject()["bodyId"]);volume+=row.toObject()["volumeMm3"].toDouble();}QVERIFY(std::abs(volume-3840)<1e-6);call(w,"view.home");capture("two-independent-solids");
  if(!output.isEmpty()){call(w,"document.save",{{"path",output+"/copied-profiles.alloy"}});call(w,"document.exportStl",{{"path",output+"/copied-profiles.stl"},{"bodyIds",bodyIds},{"units","mm"},{"format","binary"},{"surfaceDeviationMm",.01},{"normalDeviationDeg",5}});call(w,"document.open",{{"path",output+"/copied-profiles.alloy"}});QCOMPARE(call(w,"document.inspect")["authored"],inspection["authored"]);QFile file(output+"/native-report.json");QVERIFY(file.open(QIODevice::WriteOnly));file.write(QJsonDocument(QJsonObject{{"passed",true},{"sketchId",sketchId},{"source",source},{"final",final},{"sourceEntityIds",ids},{"authored",inspection["authored"]},{"inspection",inspection},{"state",w.state()},{"transcript",transcript_}}).toJson());}call(w,"sketch.edit",{{"sketchId",sketchId}});call(w,"sketch.paste");QVERIFY(panel());call(w,"command.cancel",{{"session",active(w)["id"]}});QTRY_VERIFY(!panel());QVERIFY(active(w).isEmpty());QCOMPARE(call(w,"document.inspect")["authored"],inspection["authored"]);call(w,"document.new",{{"discard",true}});w.close();
 }
};
QTEST_MAIN(SketchCopyControls)
#include "sketch_copy_controls.moc"
