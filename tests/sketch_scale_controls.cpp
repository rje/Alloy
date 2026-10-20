// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDebug>
#include <QDoubleSpinBox>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonDocument>
#include <QMenu>
#include <QPushButton>
#include <QStatusBar>
#include <QTest>
#include <QTimer>
#include <QToolButton>
#include <cmath>
class SketchScaleControls final:public QObject{
 Q_OBJECT
 QJsonArray transcript_;
 QJsonObject call(AlloyWindow& w,const QString& id,const QJsonObject& args={}){auto r=w.execute(id,args);transcript_.append(QJsonObject{{"command",id},{"args",args},{"response",r}});if(!r["ok"].toBool())qFatal("%s",QJsonDocument(r).toJson().constData());const auto initial=r["result"].toObject();QElapsedTimer timer;timer.start();while(r["completion"]=="pending"){if(timer.elapsed()>20000)qFatal("Pending timeout");QTest::qWait(10);r=w.execute("command.status",{{"session",initial["id"]},{"generation",initial["previewGeneration"]}});if(!r["ok"].toBool())qFatal("%s",QJsonDocument(r).toJson().constData());}return r["result"].toObject();}
 QJsonObject active(AlloyWindow& w){return w.state()["activeSession"].toObject();}
 QJsonObject draft(AlloyWindow& w){return active(w)["draftSketch"].toObject();}
 void click(QWidget* widget,const QPoint& point){QVERIFY(widget && widget->isVisible());const auto global=widget->mapToGlobal(point);auto* hit=QApplication::widgetAt(global);QVERIFY(hit==widget || widget->isAncestorOf(hit));QTest::mouseClick(hit,Qt::LeftButton,Qt::NoModifier,hit->mapFromGlobal(global));}
 void click(QWidget* widget){click(widget,widget->rect().center());}
private slots:
 void nativeScale(){
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1500,950);w.show();w.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&w));QApplication::setActiveWindow(&w);
  const auto output=QDir(qEnvironmentVariable("ALLOY_SCALE_EVIDENCE")).absolutePath();QVERIFY(!qEnvironmentVariable("ALLOY_SCALE_EVIDENCE").isEmpty());QDir().mkpath(output);
  call(w,"sketch.begin",{{"plane","XY"},{"name","Asymmetric retained Scale"}});const double xy[4][2]={{10,10},{30,10},{27,20},{12,24}};
  for(int n=0;n<4;++n)call(w,"sketch.line",{{"x1",xy[n][0]},{"y1",xy[n][1]},{"x2",xy[(n+1)%4][0]},{"y2",xy[(n+1)%4][1]},{"units","mm"}});
  QJsonArray ids;for(const auto& e:draft(w)["parameters"].toObject()["entities"].toArray())ids.append(e.toObject()["id"]);
  call(w,"sketch.constraint",{{"type","horizontal"},{"entityIds",QJsonArray{ids[0]}}});call(w,"sketch.addPoint",{{"x",4},{"y",6},{"units","mm"}});
  const auto sid=draft(w)["id"].toString();const auto pivot=draft(w)["parameters"].toObject()["entities"].toArray().last().toObject()["pointId"].toString();
  call(w,"sketch.finish");auto base=call(w,"document.inspect");const auto profile=base["evaluation"].toObject()["sketches"].toArray().first().toObject()["profiles"].toArray().first().toObject()["profileId"].toString();
  call(w,"solid.extrude",{{"sketchId",sid},{"profileId",profile},{"distance",8},{"units","mm"}});base=call(w,"document.inspect");const auto body=base["evaluation"].toObject()["bodies"].toArray().first().toObject();QVERIFY(std::abs(body["volumeMm3"].toDouble()-1672)<1e-6);call(w,"document.save",{{"path",output+"/source.alloy"}});
  call(w,"sketch.edit",{{"sketchId",sid}});call(w,"sketch.lookAt");call(w,"view.zoom",{{"factor",.6}});const auto source=draft(w);const auto history=active(w).value("draftHistory");
  ReferenceView* view=nullptr;for(auto* v:w.findChildren<QWidget*>())if(auto* r=dynamic_cast<ReferenceView*>(v))view=r;QVERIFY(view);
  auto panel=[&]{return w.findChild<QDialog*>("sketch.scale.dialog");};
  auto pixel=[&](double u,double v){const auto p=call(w,"view.projectSketchPoint",{{"sketchId",sid},{"uMm",u},{"vMm",v}});return QPoint(qRound(p["x"].toDouble()),qRound(p["y"].toDouble()));};
  auto menu=[&](const QString& action){auto* b=w.findChild<QToolButton*>("sketch.menu.modify");auto* m=w.findChild<QMenu*>("sketch.commands.modify");auto* a=w.findChild<QAction*>(action);QVERIFY(b&&m&&a&&a->isEnabled());bool hit=false;QTimer::singleShot(0,[&]{if(m->isVisible() && QApplication::activePopupWidget()==m){const auto p=m->actionGeometry(a).center();if(QApplication::widgetAt(m->mapToGlobal(p))==m){hit=true;QTest::mouseClick(m,Qt::LeftButton,Qt::NoModifier,p);return;}}m->close();});click(b);QVERIFY(hit);QTRY_VERIFY(!m->isVisible());QElapsedTimer wait;wait.start();while(!panel() && wait.elapsed()<5000)QTest::qWait(10);if(!panel())qFatal("Scale did not open: %s",QJsonDocument(active(w)).toJson().constData());QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));};
  auto picks=[&]{for(int n=0;n<4;++n){const auto at=pixel((xy[n][0]+xy[(n+1)%4][0])/2,(xy[n][1]+xy[(n+1)%4][1])/2);const auto hit=view->pickSketch(draft(w),at.x(),at.y(),"entity",active(w)["resolvedFrame"].toObject());click(view,at);if(active(w)["scaleInteraction"].toObject()["entityIds"].toArray().size()!=n+1){call(w,"capture.screenshot",{{"path",output+"/pick-failure.png"}});QFile f(output+"/pick-failure.json");if(f.open(QIODevice::WriteOnly))f.write(QJsonDocument(QJsonObject{{"state",w.state()},{"pickHit",hit},{"x",at.x()},{"y",at.y()},{"status",w.statusBar()->currentMessage()}}).toJson());qFatal("Scale actual pick failed: %s",QJsonDocument(QJsonObject{{"hit",hit},{"status",w.statusBar()->currentMessage()},{"interaction",active(w)["scaleInteraction"]}}).toJson().constData());}}QCOMPARE(active(w)["scaleInteraction"].toObject()["entityIds"].toArray().size(),4);click(panel()->findChild<QPushButton*>("sketch.scale.pivot"));QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));click(view,pixel(4,6));QCOMPARE(active(w)["scaleInteraction"].toObject()["pivotPointId"],pivot);};
  auto factor=[&](const QString& value){auto* spin=panel()->findChild<QDoubleSpinBox*>("sketch.scale.factor");click(spin);QVERIFY(spin==QApplication::focusWidget() || spin->isAncestorOf(QApplication::focusWidget()));QTest::keySequence(QApplication::focusWidget(),QKeySequence::SelectAll);QTest::keyClicks(QApplication::focusWidget(),value);QTest::keyClick(QApplication::focusWidget(),Qt::Key_Tab);QVERIFY(active(w).contains("pendingSketchScale"));};
  auto capture=[&](const QString& name){QTest::qWait(30);call(w,"capture.screenshot",{{"path",output+"/"+name+".png"}});};
  menu("sketch.action.scale");picks();factor("1.5");const auto candidate=active(w)["pendingSketchScale"].toObject()["draft"].toObject();const auto gid=candidate["parameters"].toObject()["scaleGroups"].toArray().first().toObject()["id"].toString();QCOMPARE(draft(w),source);QCOMPARE(active(w)["draftHistory"],history);factor("2");QCOMPARE(active(w)["pendingSketchScale"].toObject()["groupId"],gid);factor("1.5");capture("factor15-preview");click(panel()->findChild<QPushButton*>("sketch.scale.cancel"));QTRY_VERIFY(!panel());QCOMPARE(draft(w),source);QCOMPARE(active(w)["draftHistory"],history);
  menu("sketch.action.scale");picks();factor("1.5");QTest::keyClick(QApplication::focusWidget(),Qt::Key_Escape);QTRY_VERIFY(!panel());QCOMPARE(draft(w),source);QCOMPARE(active(w)["draftHistory"],history);
  menu("sketch.action.scale");picks();factor("1.5");click(panel()->findChild<QPushButton*>("sketch.scale.ok"));QTRY_VERIFY(!panel());const auto first=draft(w);call(w,"history.undo");QCOMPARE(draft(w),source);call(w,"history.redo");QCOMPARE(draft(w),first);
  menu("sketch.action.editScale");factor("2");capture("edit-factor2-preview");auto* spin=panel()->findChild<QDoubleSpinBox*>("sketch.scale.factor");click(spin);QTest::keyClick(QApplication::focusWidget(),Qt::Key_Return);QTRY_VERIFY(!panel());const auto second=draft(w);QCOMPARE(second["parameters"].toObject()["scaleGroups"].toArray().first().toObject()["id"],first["parameters"].toObject()["scaleGroups"].toArray().first().toObject()["id"]);call(w,"history.undo");QCOMPARE(draft(w),first);call(w,"history.redo");QCOMPARE(draft(w),second);
  menu("sketch.action.editScale");call(w,"sketch.scaleCancel");QTRY_VERIFY(!panel());menu("sketch.action.editScale");factor("1.5");call(w,"sketch.scaleAccept");QTRY_VERIFY(!panel());QCOMPARE(draft(w),first);call(w,"sketch.finish");const auto scaled=call(w,"document.inspect");QVERIFY(std::abs(scaled["evaluation"].toObject()["bodies"].toArray().first().toObject()["volumeMm3"].toDouble()-3762)<1e-6);call(w,"document.save",{{"path",output+"/scaled.alloy"}});call(w,"document.exportStl",{{"path",output+"/scaled.stl"},{"bodyId",body["bodyId"]},{"units","mm"},{"format","binary"},{"surfaceDeviationMm",.01},{"normalDeviationDeg",5}});
  call(w,"sketch.edit",{{"sketchId",sid}});const auto c=first["parameters"].toObject()["points"].toArray()[2].toObject()["id"].toString();call(w,"sketch.movePoint",{{"pointId",c},{"x",35.5},{"y",27},{"units","mm"}});const auto moved=draft(w);capture("source-c-edit");call(w,"history.undo");QCOMPARE(draft(w),first);call(w,"history.redo");QCOMPARE(draft(w),moved);call(w,"sketch.finish");const auto final=call(w,"document.inspect");const auto finalBody=final["evaluation"].toObject()["bodies"].toArray().first().toObject();QCOMPARE(finalBody["bodyId"],body["bodyId"]);QVERIFY(std::abs(finalBody["volumeMm3"].toDouble()-3510)<1e-6);call(w,"history.undo");QCOMPARE(call(w,"document.inspect")["authored"],scaled["authored"]);call(w,"history.redo");QCOMPARE(call(w,"document.inspect")["authored"],final["authored"]);call(w,"view.home");capture("source-edited-solid");
  call(w,"document.save",{{"path",output+"/source-edited-scale.alloy"}});call(w,"document.exportStl",{{"path",output+"/source-edited-scale.stl"},{"bodyId",body["bodyId"]},{"units","mm"},{"format","binary"},{"surfaceDeviationMm",.01},{"normalDeviationDeg",5}});call(w,"document.open",{{"path",output+"/source-edited-scale.alloy"}});QCOMPARE(call(w,"document.inspect")["authored"],final["authored"]);
  call(w,"sketch.edit",{{"sketchId",sid}});menu("sketch.action.editScale");call(w,"sketch.cancel");QTRY_VERIFY(!panel());QCOMPARE(call(w,"document.inspect")["authored"],final["authored"]);
  QFile report(output+"/native-report.json");QVERIFY(report.open(QIODevice::WriteOnly));report.write(QJsonDocument(QJsonObject{{"passed",true},{"sketchId",sid},{"pivotPointId",pivot},{"entityIds",ids},{"source",source},{"factor15",first},{"factor2",second},{"sourceEdited",moved},{"baselineDocument",base},{"scaledDocument",scaled},{"authored",final["authored"]},{"state",w.state()},{"transcript",transcript_}}).toJson());call(w,"document.new",{{"discard",true}});w.close();
 }
};
QTEST_MAIN(SketchScaleControls)
#include "sketch_scale_controls.moc"
