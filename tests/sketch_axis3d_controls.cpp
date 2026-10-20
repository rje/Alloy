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
class SketchAxis3dControls final:public QObject{
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
 void nativeNormalLine(){
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1500,950);w.show();w.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&w));QApplication::setActiveWindow(&w);
  call(w,"sketch.begin",{{"plane","XY"},{"name","Asymmetric profile and normal reference"}});const double xy[4][2]={{12,6},{32,10},{27,22},{9,18}};for(int i=0;i<4;++i)call(w,"sketch.line",{{"x1",xy[i][0]},{"y1",xy[i][1]},{"x2",xy[(i+1)%4][0]},{"y2",xy[(i+1)%4][1]},{"units","mm"}});const auto sid=draft(w)["id"].toString();call(w,"sketch.finish");auto baselineDocument=call(w,"document.inspect");const auto profile=baselineDocument["evaluation"].toObject()["sketches"].toArray().first().toObject()["profiles"].toArray().first().toObject().value("profileId");call(w,"solid.extrude",{{"sketchId",sid},{"profileId",profile},{"distance",8},{"units","mm"}});baselineDocument=call(w,"document.inspect");const auto body=baselineDocument["evaluation"].toObject()["bodies"].toArray().first().toObject();QVERIFY(std::abs(body["volumeMm3"].toDouble()-1952)<1e-6);call(w,"sketch.edit",{{"sketchId",sid}});call(w,"sketch.lookAt");call(w,"view.zoom",{{"factor",.6}});const auto source=draft(w);const auto history=active(w).value("draftHistory");
  ReferenceView* view=nullptr;for(auto* widget:w.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(widget))view=v;QVERIFY(view);auto panel=[&]{return w.findChild<QDialog*>("sketch.axis3d.dialog");};auto pixel=[&](double u,double v){const auto p=call(w,"view.projectSketchPoint",{{"sketchId",sid},{"uMm",u},{"vMm",v}});return QPoint(qRound(p["x"].toDouble()),qRound(p["y"].toDouble()));};auto* mode=w.findChild<QCheckBox*>("sketch.3dMode");auto* create=w.findChild<QToolButton*>("sketch.menu.create");auto* menu=w.findChild<QMenu*>("sketch.commands.create");auto* action=w.findChild<QAction*>("sketch.action.axis3d");QVERIFY(mode && create && menu && action);QVERIFY(!mode->isChecked());QVERIFY(!action->isEnabled());click(mode,QPoint(10,mode->height()/2));QVERIFY(mode->isChecked());QVERIFY(action->isEnabled());
  auto open=[&](){bool ready=false;QTimer::singleShot(0,[&]{if(menu->isVisible() && QApplication::activePopupWidget()==menu){const auto at=menu->actionGeometry(action).center();auto* hit=QApplication::widgetAt(menu->mapToGlobal(at));if(hit==menu){ready=true;QTest::mouseClick(hit,Qt::LeftButton,Qt::NoModifier,at);return;}}menu->close();});click(create);QVERIFY(ready);QTRY_VERIFY(panel());QTRY_VERIFY(!menu->isVisible());QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));};
  auto enter=[&](QWidget* widget,const QString& text){click(widget);QVERIFY(widget==QApplication::focusWidget() || widget->isAncestorOf(QApplication::focusWidget()));QTest::keySequence(QApplication::focusWidget(),QKeySequence::SelectAll);QTest::keyClicks(QApplication::focusWidget(),text);QTest::keyClick(QApplication::focusWidget(),Qt::Key_Tab);};auto values=[&](const QString& h){enter(panel()->findChild<QDoubleSpinBox*>("sketch.axis3d.u"),"4");enter(panel()->findChild<QDoubleSpinBox*>("sketch.axis3d.v"),"3");enter(panel()->findChild<QDoubleSpinBox*>("sketch.axis3d.height"),h);};
  auto output=qEnvironmentVariable("ALLOY_AXIS3D_EVIDENCE");if(!output.isEmpty()){output=QDir(output).absolutePath();QDir().mkpath(output);}auto capture=[&](const QString& name){if(!output.isEmpty())call(w,"capture.screenshot",{{"path",output+"/"+name+".png"}});};
  open();click(view,pixel(4,3));QVERIFY(std::abs(panel()->findChild<QDoubleSpinBox*>("sketch.axis3d.u")->value()-4)<.2);QVERIFY(std::abs(panel()->findChild<QDoubleSpinBox*>("sketch.axis3d.v")->value()-3)<.2);values("10");QCOMPARE(draft(w),source);QCOMPARE(active(w).value("draftHistory"),history);click(panel()->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Cancel));QTRY_VERIFY(!panel());QCOMPARE(draft(w),source);QCOMPARE(active(w).value("draftHistory"),history);
  open();click(view,pixel(4,3));values("10");capture("normal-line-authoring");click(panel()->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok));QTRY_VERIFY(!panel());const auto authoredAxisDraft=draft(w);const auto axis=authoredAxisDraft["parameters"].toObject()["constructionLines3d"].toArray().first().toObject();const auto aid=axis["id"];QCOMPARE(axis["start"].toObject()["uMm"].toDouble(),4.);QCOMPARE(axis["start"].toObject()["vMm"].toDouble(),3.);QCOMPARE(axis["start"].toObject()["wMm"].toDouble(),0.);QCOMPARE(axis["end"].toObject()["wMm"].toDouble(),10.);for(const auto& key:{"points","entities","constraints","dimensions"})QCOMPARE(authoredAxisDraft["parameters"].toObject()[key],source["parameters"].toObject()[key]);
  // Real mode transitions clear the 3D selection; the next native marker hit must restore it.
  click(mode,QPoint(10,mode->height()/2));QVERIFY(!active(w)["selection"].toObject().contains("axis3dId"));click(view,pixel(4,3));QVERIFY(!active(w)["selection"].toObject().contains("axis3dId"));click(mode,QPoint(10,mode->height()/2));click(view,pixel(4,3));QCOMPARE(active(w)["selection"].toObject()["axis3dId"],aid);QCOMPARE(active(w)["selection"].toObject()["entityIds"].toArray().size(),0);auto markers=gpu(w)["sketchAxes3d"].toArray();QCOMPARE(markers.size(),1);QVERIFY(markers.first().toObject()["endOn"].toBool());QVERIFY(markers.first().toObject()["selected"].toBool());capture("end-on-axis-picked");
  call(w,"view.orientation",{{"name","isometric"}});markers=gpu(w)["sketchAxes3d"].toArray();QCOMPARE(markers.size(),1);QVERIFY(!markers.first().toObject()["endOn"].toBool());capture("oblique-real-axis-segment");call(w,"sketch.lookAt");call(w,"view.zoom",{{"factor",.6}});
  open();values("14");QTest::keyClick(QApplication::focusWidget(),Qt::Key_Escape);QTRY_VERIFY(!panel());QCOMPARE(draft(w),authoredAxisDraft);open();values("14");click(panel()->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok));QTRY_VERIFY(!panel());const auto changed=draft(w)["parameters"].toObject()["constructionLines3d"].toArray().first().toObject();QCOMPARE(changed["id"],aid);QCOMPARE(changed["start"].toObject()["id"],axis["start"].toObject()["id"]);QCOMPARE(changed["end"].toObject()["id"],axis["end"].toObject()["id"]);call(w,"history.undo");QCOMPARE(draft(w),authoredAxisDraft);call(w,"history.redo");QCOMPARE(draft(w)["parameters"].toObject()["constructionLines3d"].toArray().first().toObject(),changed);call(w,"history.undo");open();click(panel()->findChild<QPushButton*>("sketch.axis3d.delete"));QTRY_VERIFY(!panel());QVERIFY(call(w,"sketch.axis3dInspect")["axes"].toArray().isEmpty());call(w,"history.undo");QCOMPARE(draft(w),authoredAxisDraft);
  call(w,"sketch.finish");auto inspection=call(w,"document.inspect");QCOMPARE(inspection["evaluation"].toObject()["sketches"].toArray().first().toObject()["profiles"].toArray().size(),1);const auto finalBody=inspection["evaluation"].toObject()["bodies"].toArray().first().toObject();QCOMPARE(finalBody["bodyId"],body["bodyId"]);QVERIFY(std::abs(finalBody["volumeMm3"].toDouble()-1952)<1e-6);QCOMPARE(inspection["evaluation"].toObject()["sketches"].toArray().first().toObject()["profiles"].toArray().first().toObject()["profileId"],profile);call(w,"history.undo");QCOMPARE(call(w,"document.inspect")["authored"],baselineDocument["authored"]);call(w,"history.redo");QCOMPARE(call(w,"document.inspect")["authored"],inspection["authored"]);
  if(!output.isEmpty()){call(w,"document.save",{{"path",output+"/normal-axis-profile.alloy"}});call(w,"document.exportStl",{{"path",output+"/normal-axis-profile.stl"},{"bodyId",body["bodyId"]},{"units","mm"},{"format","binary"},{"surfaceDeviationMm",.01},{"normalDeviationDeg",5}});call(w,"document.open",{{"path",output+"/normal-axis-profile.alloy"}});QCOMPARE(call(w,"document.inspect")["authored"],inspection["authored"]);QFile file(output+"/native-report.json");QVERIFY(file.open(QIODevice::WriteOnly));file.write(QJsonDocument(QJsonObject{{"passed",true},{"sketchId",sid},{"axis",axis},{"source",source},{"withAxis",authoredAxisDraft},{"baselineDocument",baselineDocument},{"authored",inspection["authored"]},{"state",w.state()},{"transcript",transcript_}}).toJson());}call(w,"document.new",{{"discard",true}});w.close();
 }
};
QTEST_MAIN(SketchAxis3dControls)
#include "sketch_axis3d_controls.moc"
