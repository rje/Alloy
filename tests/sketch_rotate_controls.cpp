// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QAction>
#include <QCheckBox>
#include <QComboBox>
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
class SketchRotateControls final:public QObject{
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
 void nativeRotate(){
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1500,950);w.show();w.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&w));QApplication::setActiveWindow(&w);
  call(w,"sketch.begin",{{"plane","XY"},{"name","Asymmetric retained Rotate"}});const double xy[4][2]={{12,6},{32,10},{27,22},{9,18}};for(int n=0;n<4;++n)call(w,"sketch.line",{{"x1",xy[n][0]},{"y1",xy[n][1]},{"x2",xy[(n+1)%4][0]},{"y2",xy[(n+1)%4][1]},{"units","mm"}});auto pp=draft(w)["parameters"].toObject()["points"].toArray();for(int n=0;n<5;++n){const int a=n<4?n:0,b=n<4?(n+1)%4:2;const double length=std::hypot(xy[b][0]-xy[a][0],xy[b][1]-xy[a][1]),au=(xy[a][0]+xy[b][0])/2+(n<4?4*(xy[b][1]-xy[a][1])/length:0),av=(xy[a][1]+xy[b][1])/2-(n<4?4*(xy[b][0]-xy[a][0])/length:0);call(w,"sketch.dimension",{{"kind","linear"},{"pointIds",QJsonArray{pp[a].toObject()["id"],pp[b].toObject()["id"]}},{"value",std::hypot(xy[a][0]-xy[b][0],xy[a][1]-xy[b][1])},{"units","mm"},{"annotation",QJsonObject{{"uMm",au},{"vMm",av}}}});}QCOMPARE(active(w)["solverDiagnostics"].toObject()["dof"].toInt(),3);call(w,"sketch.3dMode",{{"enabled",true}});const auto axis=call(w,"sketch.axis3d",{{"start",QJsonObject{{"u",4},{"v",3},{"w",0}}},{"end",QJsonObject{{"u",4},{"v",3},{"w",10}}},{"units","mm"}})["axis"].toObject();const auto sid=draft(w)["id"].toString();call(w,"sketch.finish");auto base=call(w,"document.inspect");const auto profile=base["evaluation"].toObject()["sketches"].toArray().first().toObject()["profiles"].toArray().first().toObject().value("profileId");call(w,"solid.extrude",{{"sketchId",sid},{"profileId",profile},{"distance",8},{"units","mm"}});base=call(w,"document.inspect");const auto body=base["evaluation"].toObject()["bodies"].toArray().first().toObject();call(w,"sketch.edit",{{"sketchId",sid}});call(w,"sketch.lookAt");call(w,"view.zoom",{{"factor",.6}});const auto source=draft(w);const auto h=active(w).value("draftHistory");
  ReferenceView* view=nullptr;for(auto* widget:w.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(widget))view=v;QVERIFY(view);auto panel=[&]{return w.findChild<QDialog*>("sketch.rotate.dialog");};auto pixel=[&](double u,double v){const auto p=call(w,"view.projectSketchPoint",{{"sketchId",sid},{"uMm",u},{"vMm",v}});return QPoint(qRound(p["x"].toDouble()),qRound(p["y"].toDouble()));};
  auto menuAction=[&](const QString& button,const QString& name,const QString& action){auto* b=w.findChild<QToolButton*>(button);auto* m=w.findChild<QMenu*>(name);auto* a=w.findChild<QAction*>(action);QVERIFY(b&&m&&a&&a->isEnabled());bool hit=false;QTimer::singleShot(0,[&]{if(m->isVisible() && QApplication::activePopupWidget()==m){const auto p=m->actionGeometry(a).center();if(QApplication::widgetAt(m->mapToGlobal(p))==m){hit=true;QTest::mouseClick(m,Qt::LeftButton,Qt::NoModifier,p);return;}}m->close();});click(b);QVERIFY(hit);QTRY_VERIFY(!m->isVisible());};
  auto rotateMode=[&]{auto* d=w.findChild<QDialog*>("sketch.move.dialog");QVERIFY(d);auto* combo=d->findChild<QComboBox*>("sketch.move.type");QVERIFY(combo);click(combo);QTest::keyClick(QApplication::focusWidget(),Qt::Key_End);QTest::keyClick(QApplication::focusWidget(),Qt::Key_Return);QTRY_VERIFY(panel());QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));};
  auto picks=[&]{for(int n=0;n<4;++n)click(view,pixel((xy[n][0]+xy[(n+1)%4][0])/2,(xy[n][1]+xy[(n+1)%4][1])/2));QCOMPARE(active(w)["moveInteraction"].toObject()["entityIds"].toArray().size(),4);click(panel()->findChild<QPushButton*>("sketch.rotate.axis"));QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));click(view,pixel(4,3));QCOMPARE(active(w)["moveInteraction"].toObject()["axisLineId"],axis["id"]);};
  auto angle=[&](const QString& value){auto* spin=panel()->findChild<QDoubleSpinBox*>("sketch.rotate.angle");click(spin);QVERIFY(spin==QApplication::focusWidget() || spin->isAncestorOf(QApplication::focusWidget()));QTest::keySequence(QApplication::focusWidget(),QKeySequence::SelectAll);QTest::keyClicks(QApplication::focusWidget(),value);QTest::keyClick(QApplication::focusWidget(),Qt::Key_Tab);QVERIFY(active(w).contains("pendingSketchMove"));};
  auto output=qEnvironmentVariable("ALLOY_ROTATE_EVIDENCE");if(!output.isEmpty()){output=QDir(output).absolutePath();QDir().mkpath(output);}auto capture=[&](const QString& name){if(!output.isEmpty())call(w,"capture.screenshot",{{"path",output+"/"+name+".png"}});};
  menuAction("solid.menu.modify","solid.commands.modify","solid.action.solid.moveDialog");rotateMode();picks();angle("30");const auto candidate=active(w)["pendingSketchMove"].toObject()["draft"].toObject();QCOMPARE(draft(w),source);QCOMPARE(active(w).value("draftHistory"),h);angle("35");QCOMPARE(active(w)["pendingSketchMove"].toObject()["draft"].toObject()["parameters"].toObject()["rotateGroups"].toArray().first().toObject()["id"],candidate["parameters"].toObject()["rotateGroups"].toArray().first().toObject()["id"]);angle("30");capture("angle30-preview");click(panel()->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Cancel));QTRY_VERIFY(!panel());QCOMPARE(draft(w),source);QCOMPARE(active(w).value("draftHistory"),h);
  QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QTest::keyClick(view,Qt::Key_M);rotateMode();picks();angle("30");QTest::keyClick(QApplication::focusWidget(),Qt::Key_Escape);QTRY_VERIFY(!panel());QCOMPARE(draft(w),source);
  QTest::keyClick(QApplication::focusWidget(),Qt::Key_M);rotateMode();picks();angle("30");click(panel()->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok));QTRY_VERIFY(!panel());const auto first=draft(w);const auto group=first["parameters"].toObject()["rotateGroups"].toArray().first().toObject();call(w,"history.undo");QCOMPARE(draft(w),source);call(w,"history.redo");QCOMPARE(draft(w),first);
  menuAction("sketch.menu.modify","sketch.commands.modify","sketch.action.editRotate");QTRY_VERIFY(panel());angle("-15");capture("edit-minus15-preview");click(panel()->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok));QTRY_VERIFY(!panel());const auto final=draft(w);QCOMPARE(final["parameters"].toObject()["rotateGroups"].toArray().first().toObject()["id"],group["id"]);call(w,"history.undo");QCOMPARE(draft(w),first);call(w,"history.redo");QCOMPARE(draft(w),final);call(w,"sketch.finish");const auto inspection=call(w,"document.inspect");const auto finalBody=inspection["evaluation"].toObject()["bodies"].toArray().first().toObject();QCOMPARE(finalBody["bodyId"],body["bodyId"]);QVERIFY(std::abs(finalBody["volumeMm3"].toDouble()-1952)<1e-6);call(w,"history.undo");QCOMPARE(call(w,"document.inspect")["authored"],base["authored"]);call(w,"history.redo");QCOMPARE(call(w,"document.inspect")["authored"],inspection["authored"]);call(w,"view.home");capture("edited-rotated-solid");
  if(!output.isEmpty()){call(w,"document.save",{{"path",output+"/retained-rotate.alloy"}});call(w,"document.exportStl",{{"path",output+"/retained-rotate.stl"},{"bodyId",body["bodyId"]},{"units","mm"},{"format","binary"},{"surfaceDeviationMm",.01},{"normalDeviationDeg",5}});call(w,"document.open",{{"path",output+"/retained-rotate.alloy"}});QCOMPARE(call(w,"document.inspect")["authored"],inspection["authored"]);QFile file(output+"/native-report.json");QVERIFY(file.open(QIODevice::WriteOnly));file.write(QJsonDocument(QJsonObject{{"passed",true},{"sketchId",sid},{"axis",axis},{"source",source},{"angle30",first},{"final",final},{"baselineDocument",base},{"authored",inspection["authored"]},{"state",w.state()},{"transcript",transcript_}}).toJson());}call(w,"document.new",{{"discard",true}});w.close();
 }
};
QTEST_MAIN(SketchRotateControls)
#include "sketch_rotate_controls.moc"
