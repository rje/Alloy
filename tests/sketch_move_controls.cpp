// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QAction>
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
class SketchMoveControls final:public QObject{
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
 void nativeAtomicMoves(){
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1500,950);w.show();w.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&w));QApplication::setActiveWindow(&w);
  call(w,"sketch.begin",{{"plane","XY"},{"name","Movable constrained rectangle"}});call(w,"sketch.rectangle",{{"x1",10},{"y1",8},{"x2",30},{"y2",20},{"units","mm"}});const auto original=draft(w);const auto sketchId=original["id"].toString();const auto edges=original["parameters"].toObject()["entities"].toArray();QJsonArray ids;for(const auto& e:edges)ids.append(e.toObject()["id"]);
  call(w,"sketch.dimension",{{"kind","horizontal"},{"entityId",ids[0]},{"value",20},{"units","mm"}});call(w,"sketch.dimension",{{"kind","vertical"},{"entityId",ids[1]},{"value",12},{"units","mm"}});QCOMPARE(active(w)["solverDiagnostics"].toObject()["dof"].toInt(),2);call(w,"sketch.finish");auto inspection=call(w,"document.inspect");const auto profile=inspection["evaluation"].toObject()["sketches"].toArray().first().toObject()["profiles"].toArray().first().toObject().value("profileId");call(w,"solid.extrude",{{"sketchId",sketchId},{"profileId",profile},{"distance",8},{"units","mm"}});const auto baselineDocument=call(w,"document.inspect");call(w,"sketch.edit",{{"sketchId",sketchId}});call(w,"sketch.lookAt");const auto baseline=draft(w);const auto history=active(w).value("draftHistory");
  ReferenceView* view=nullptr;for(auto* widget:w.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(widget))view=v;QVERIFY(view);
  auto pixel=[&](double u,double v){const auto p=call(w,"view.projectSketchPoint",{{"sketchId",sketchId},{"uMm",u},{"vMm",v}});return QPoint(qRound(p["x"].toDouble()),qRound(p["y"].toDouble()));};
  auto* modify=w.findChild<QToolButton*>("sketch.menu.modify");auto* menu=w.findChild<QMenu*>("sketch.commands.modify");auto* action=w.findChild<QAction*>("sketch.action.moveCopy");QVERIFY(modify && menu && action);
  auto open=[&]()->QDialog*{bool ready=false;QTimer::singleShot(0,[&]{if(menu->isVisible() && QApplication::activePopupWidget()==menu){const auto at=menu->actionGeometry(action).center();auto* hit=QApplication::widgetAt(menu->mapToGlobal(at));if(hit==menu){ready=true;QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,at);return;}}menu->close();});click(modify);if(!ready)return nullptr;return w.findChild<QDialog*>("sketch.move.dialog");};
  auto pick=[&]{for(const auto& xy:QJsonArray{QJsonArray{20,8},QJsonArray{30,14},QJsonArray{20,20},QJsonArray{10,14}}){const auto at=xy.toArray();click(view,pixel(at[0].toDouble(),at[1].toDouble()));}QCOMPARE(active(w)["moveInteraction"].toObject()["entityIds"].toArray().size(),4);};
  auto enter=[&](QWidget* widget,const QString& text){click(widget);QVERIFY(widget==QApplication::focusWidget() || widget->isAncestorOf(QApplication::focusWidget()));QTest::keySequence(QApplication::focusWidget(),QKeySequence::SelectAll);QTest::keyClicks(QApplication::focusWidget(),text);QTest::keyClick(QApplication::focusWidget(),Qt::Key_Tab);};
  auto values=[&](QDialog* dialog,const QString& x,const QString& y){enter(dialog->findChild<QDoubleSpinBox*>("sketch.move.x"),x);enter(dialog->findChild<QDoubleSpinBox*>("sketch.move.y"),y);QVERIFY(active(w).contains("pendingSketchMove"));};
  auto output=qEnvironmentVariable("ALLOY_SKETCH_MOVE_EVIDENCE");if(!output.isEmpty()){output=QDir(output).absolutePath();QDir().mkpath(output);}auto capture=[&](const QString& name){if(!output.isEmpty())call(w,"capture.screenshot",{{"path",output+"/"+name+".png"}});};
  auto* dialog=open();QVERIFY(dialog);QTRY_VERIFY(dialog->isVisible());QTRY_VERIFY(!menu->isVisible());QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));pick();values(dialog,"5","-3");QCOMPARE(draft(w),baseline);QCOMPARE(active(w).value("draftHistory"),history);QCOMPARE(call(w,"document.inspect")["authored"],baselineDocument["authored"]);capture("move5-minus3-preview");click(dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Cancel));QTRY_VERIFY(!w.findChild<QDialog*>("sketch.move.dialog"));QCOMPARE(draft(w),baseline);QCOMPARE(active(w).value("draftHistory"),history);
  QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QTest::keyClick(view,Qt::Key_M);dialog=w.findChild<QDialog*>("sketch.move.dialog");QVERIFY(dialog);pick();values(dialog,"5","-3");QTest::keyClick(QApplication::focusWidget(),Qt::Key_Escape);QTRY_VERIFY(!w.findChild<QDialog*>("sketch.move.dialog"));QCOMPARE(draft(w),baseline);QCOMPARE(active(w).value("draftHistory"),history);
  QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QTest::keyClick(view,Qt::Key_M);dialog=w.findChild<QDialog*>("sketch.move.dialog");QVERIFY(dialog);pick();values(dialog,"5","-3");click(dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok));QTRY_VERIFY(!w.findChild<QDialog*>("sketch.move.dialog"));const auto first=draft(w);QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QCOMPARE(active(w)["selection"].toObject()["entityIds"].toArray().size(),4);
  QTest::keyClick(view,Qt::Key_M);dialog=w.findChild<QDialog*>("sketch.move.dialog");QVERIFY(dialog);QCOMPARE(active(w)["moveInteraction"].toObject()["entityIds"].toArray().size(),4);values(dialog,"2","1");capture("second-relative-move-preview");click(dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok));QTRY_VERIFY(!w.findChild<QDialog*>("sketch.move.dialog"));const auto final=draft(w);
  const auto a=baseline["parameters"].toObject(),b=final["parameters"].toObject();for(const auto& key:{"entities","dimensions","constraints"})QCOMPARE(a[key],b[key]);const auto ap=a["points"].toArray(),bp=b["points"].toArray();for(int i=0;i<ap.size();++i){QVERIFY(std::abs(bp[i].toObject()["uMm"].toDouble()-ap[i].toObject()["uMm"].toDouble()-7)<1e-6);QVERIFY(std::abs(bp[i].toObject()["vMm"].toDouble()-ap[i].toObject()["vMm"].toDouble()+2)<1e-6);}call(w,"history.undo");QCOMPARE(draft(w),first);call(w,"history.redo");QCOMPARE(draft(w),final);call(w,"sketch.finish");inspection=call(w,"document.inspect");QCOMPARE(inspection["evaluation"].toObject()["sketches"].toArray().first().toObject()["profiles"].toArray().first().toObject()["profileId"],profile);const auto body=inspection["evaluation"].toObject()["bodies"].toArray().first().toObject();QVERIFY(std::abs(body["volumeMm3"].toDouble()-1920)<1e-6);QCOMPARE(body["bodyId"],baselineDocument["evaluation"].toObject()["bodies"].toArray().first().toObject()["bodyId"]);call(w,"history.undo");QCOMPARE(call(w,"document.inspect")["authored"],baselineDocument["authored"]);call(w,"history.redo");QCOMPARE(call(w,"document.inspect")["authored"],inspection["authored"]);call(w,"view.home");capture("moved-extruded-solid");
  if(!output.isEmpty()){call(w,"document.save",{{"path",output+"/moved-profile.alloy"}});call(w,"document.exportStl",{{"path",output+"/moved-profile.stl"},{"bodyId",body["bodyId"]},{"units","mm"},{"format","binary"},{"surfaceDeviationMm",.01},{"normalDeviationDeg",5}});call(w,"document.open",{{"path",output+"/moved-profile.alloy"}});QCOMPARE(call(w,"document.inspect")["authored"],inspection["authored"]);QFile report(output+"/native-report.json");QVERIFY(report.open(QIODevice::WriteOnly));report.write(QJsonDocument(QJsonObject{{"passed",true},{"sketchId",sketchId},{"entityIds",ids},{"profileId",profile},{"baseline",baseline},{"baselineDocument",baselineDocument},{"first",first},{"final",final},{"authored",inspection["authored"]},{"state",w.state()},{"transcript",transcript_}}).toJson());}
  call(w,"document.new",{{"discard",true}});w.close();
 }
 void sharedLifecycle(){
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1500,950);w.show();w.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&w));QApplication::setActiveWindow(&w);
  call(w,"sketch.begin",{{"plane","XY"}});call(w,"sketch.rectangle",{{"x1",10},{"y1",8},{"x2",30},{"y2",20},{"units","mm"}});call(w,"sketch.lookAt");QJsonArray ids;for(const auto& e:draft(w)["parameters"].toObject()["entities"].toArray())ids.append(e.toObject()["id"]);
  ReferenceView* view=nullptr;for(auto* widget:w.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(widget))view=v;QVERIFY(view);
  const auto before=call(w,"document.inspect");const auto original=draft(w);const auto history=active(w).value("draftHistory");const auto session=active(w).value("id");
  call(w,"sketch.moveStart");QVERIFY(w.findChild<QDialog*>("sketch.move.dialog"));call(w,"sketch.movePreview",{{"entityIds",ids},{"x",5},{"y",-3},{"units","mm"}});const auto candidate=active(w).value("pendingSketchMove");
  for(const auto& command:{QString("sketch.finish"),QString("command.commit")}){const auto response=w.execute(command,command=="command.commit" ? QJsonObject{{"session",session}} : QJsonObject{});QVERIFY(!response["ok"].toBool());QCOMPARE(response["error"].toObject()["code"].toString(),QString("pending_move"));QCOMPARE(active(w).value("pendingSketchMove"),candidate);QCOMPARE(draft(w),original);QCOMPARE(active(w).value("draftHistory"),history);QCOMPARE(call(w,"document.inspect")["authored"],before["authored"]);QVERIFY(w.findChild<QDialog*>("sketch.move.dialog")->isVisible());}
  call(w,"sketch.moveAccept");QTRY_VERIFY(!w.findChild<QDialog*>("sketch.move.dialog"));QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));const auto accepted=draft(w);QVERIFY(accepted!=original);
  QTest::keyClick(view,Qt::Key_M);auto* dialog=w.findChild<QDialog*>("sketch.move.dialog");QVERIFY(dialog && dialog->isVisible());QCOMPARE(dialog->findChild<QDoubleSpinBox*>("sketch.move.x")->value(),0.);QCOMPARE(active(w)["moveInteraction"].toObject()["entityIds"].toArray().size(),4);
  const auto output=qEnvironmentVariable("ALLOY_SKETCH_MOVE_EVIDENCE");if(!output.isEmpty())call(w,"capture.screenshot",{{"path",QDir(output).absoluteFilePath("api-accept-reopened-move.png")}});
  call(w,"sketch.movePreview",{{"entityIds",ids},{"x",2},{"y",1},{"units","mm"}});call(w,"sketch.moveCancel");QTRY_VERIFY(!w.findChild<QDialog*>("sketch.move.dialog"));QCOMPARE(draft(w),accepted);QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
  QTest::keyClick(view,Qt::Key_M);dialog=w.findChild<QDialog*>("sketch.move.dialog");QVERIFY(dialog && dialog->isVisible());QCOMPARE(dialog->findChild<QDoubleSpinBox*>("sketch.move.x")->value(),0.);call(w,"sketch.cancel");QTRY_VERIFY(!w.findChild<QDialog*>("sketch.move.dialog"));QVERIFY(active(w).isEmpty());QCOMPARE(call(w,"document.inspect")["authored"],before["authored"]);
  call(w,"sketch.begin",{{"plane","XY"}});call(w,"sketch.moveStart");QVERIFY(w.findChild<QDialog*>("sketch.move.dialog"));call(w,"command.cancel",{{"session",active(w).value("id")}});QTRY_VERIFY(!w.findChild<QDialog*>("sketch.move.dialog"));QVERIFY(active(w).isEmpty());w.close();
 }

};
QTEST_MAIN(SketchMoveControls)
#include "sketch_move_controls.moc"
