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
#include <QLineEdit>
#include <QMouseEvent>
#include <QPushButton>
#include <QSpinBox>
#include <QTest>
#include <QTimer>
#include <QToolButton>
#include <cmath>
#include <numbers>
class SketchRotateSourceSizeControls final:public QObject{
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
 void nativeSourceSize(){
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1500,950);w.show();w.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&w));QApplication::setActiveWindow(&w);
  call(w,"sketch.begin",{{"plane","XY"},{"name","Rotate source size"}});const double xy[4][2]={{12,6},{32,10},{27,22},{9,18}};for(int n=0;n<4;++n)call(w,"sketch.line",{{"x1",xy[n][0]},{"y1",xy[n][1]},{"x2",xy[(n+1)%4][0]},{"y2",xy[(n+1)%4][1]},{"units","mm"}});auto pp=draft(w)["parameters"].toObject()["points"].toArray();for(int n=0;n<5;++n){const int a=n<4?n:0,b=n<4?(n+1)%4:2;const double length=std::hypot(xy[b][0]-xy[a][0],xy[b][1]-xy[a][1]),au=(xy[a][0]+xy[b][0])/2+(n<4?4*(xy[b][1]-xy[a][1])/length:0),av=(xy[a][1]+xy[b][1])/2-(n<4?4*(xy[b][0]-xy[a][0])/length:0);call(w,"sketch.dimension",{{"kind","linear"},{"pointIds",QJsonArray{pp[a].toObject()["id"],pp[b].toObject()["id"]}},{"value",length},{"units","mm"},{"annotation",QJsonObject{{"uMm",au},{"vMm",av}}}});}QJsonArray ids;for(const auto& v:draft(w)["parameters"].toObject()["entities"].toArray())ids.append(v.toObject()["id"]);call(w,"sketch.3dMode",{{"enabled",true}});const auto axis=call(w,"sketch.axis3d",{{"start",QJsonObject{{"u",4},{"v",3},{"w",0}}},{"end",QJsonObject{{"u",4},{"v",3},{"w",10}}},{"units","mm"}})["axis"].toObject();call(w,"sketch.rotatePreview",{{"entityIds",ids},{"axisLineId",axis["id"]},{"angleDeg",30}});call(w,"sketch.rotateAccept");const auto sid=draft(w)["id"].toString();call(w,"sketch.finish");auto before=call(w,"document.inspect");const auto profile=before["evaluation"].toObject()["sketches"].toArray().first().toObject()["profiles"].toArray().first().toObject().value("profileId");call(w,"solid.extrude",{{"sketchId",sid},{"profileId",profile},{"distance",8},{"units","mm"}});before=call(w,"document.inspect");const auto body=before["evaluation"].toObject()["bodies"].toArray().first().toObject();if(!qEnvironmentVariable("ALLOY_ROTATE_SOURCE_SIZE_EVIDENCE").isEmpty()){const auto dir=QDir(qEnvironmentVariable("ALLOY_ROTATE_SOURCE_SIZE_EVIDENCE")).absolutePath();QDir().mkpath(dir);call(w,"document.save",{{"path",dir+"/legacy-before-size.alloy"}});}call(w,"sketch.edit",{{"sketchId",sid}});call(w,"sketch.lookAt");call(w,"view.zoom",{{"factor",.6}});const auto legacy=draft(w),dim=legacy["parameters"].toObject()["dimensions"].toArray().first().toObject();const auto history=active(w).value("draftHistory");QVERIFY(!legacy["parameters"].toObject()["rotateGroups"].toArray().first().toObject().contains("sourceFrame"));
  ReferenceView* view=nullptr;for(auto* widget:w.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(widget))view=v;QVERIFY(view);auto editor=[&]{return w.findChild<QLineEdit*>("sketch.inlineValue");};auto open=[&]{const auto d=draft(w)["parameters"].toObject()["dimensions"].toArray().first().toObject();const auto label=view->dimensionLayout(draft(w),d,d["valueMm"].toDouble(),"mm")["label"].toObject();const QPoint at(qRound(label["x"].toDouble()),qRound(label["y"].toDouble()));QVERIFY(view->rect().contains(at));const auto global=view->mapToGlobal(at);auto* hit=QApplication::widgetAt(global);QVERIFY(hit==view || view->isAncestorOf(hit));QTest::mouseDClick(hit,Qt::LeftButton,Qt::NoModifier,hit->mapFromGlobal(global));QTRY_VERIFY(editor() && editor()->isVisible());QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(editor()));QCOMPARE(active(w)["pendingDimension"].toObject()["dimensionId"],dim["id"]);};auto type=[&](const QString& text){QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(editor()));QTest::keySequence(editor(),QKeySequence::SelectAll);QTest::keyClicks(editor(),text);};
  auto output=qEnvironmentVariable("ALLOY_ROTATE_SOURCE_SIZE_EVIDENCE");if(!output.isEmpty()){output=QDir(output).absolutePath();QDir().mkpath(output);}auto capture=[&](const QString& name){QTest::qWait(30);if(!output.isEmpty())call(w,"capture.screenshot",{{"path",output+"/"+name+".png"}});};
  open();type("22");QCOMPARE(draft(w),legacy);QCOMPARE(active(w).value("draftHistory"),history);capture("source-length22-input");QTest::keyClick(editor(),Qt::Key_Escape);QTRY_VERIFY(!active(w).contains("pendingDimension"));QCOMPARE(draft(w),legacy);QCOMPARE(active(w).value("draftHistory"),history);QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
  open();type("1");QTest::keyClick(editor(),Qt::Key_Return);QVERIFY(active(w).contains("pendingDimension"));QCOMPARE(draft(w),legacy);QCOMPARE(active(w).value("draftHistory"),history);QVERIFY(editor()->isVisible());QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(editor()));capture("impossible-size-refused");type("22");QTest::keyClick(editor(),Qt::Key_Return);QTRY_VERIFY(!active(w).contains("pendingDimension"));QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));const auto sized=draft(w),sg=sized["parameters"].toObject()["rotateGroups"].toArray().first().toObject();QVERIFY(sg.contains("sourceFrame"));QCOMPARE(sg["angleDeg"].toDouble(),30.);QCOMPARE(sg["id"],legacy["parameters"].toObject()["rotateGroups"].toArray().first().toObject()["id"]);QCOMPARE(sized["parameters"].toObject()["dimensions"].toArray().first().toObject()["valueMm"].toDouble(),22.);capture("source-length22-rotated");call(w,"history.undo");QCOMPARE(draft(w),legacy);call(w,"history.redo");QCOMPARE(draft(w),sized);
  auto* mode=w.findChild<QCheckBox*>("sketch.3dMode");QVERIFY(mode && !mode->isChecked());click(mode,QPoint(10,mode->height()/2));auto* create=w.findChild<QToolButton*>("sketch.menu.create");auto* menu=w.findChild<QMenu*>("sketch.commands.create");auto* action=w.findChild<QAction*>("sketch.action.axis3d");bool invoked=false;QTimer::singleShot(0,[&]{if(menu->isVisible() && QApplication::activePopupWidget()==menu){const auto at=menu->actionGeometry(action).center();if(QApplication::widgetAt(menu->mapToGlobal(at))==menu){invoked=true;QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,at);return;}}menu->close();});click(create);QVERIFY(invoked);auto* panel=w.findChild<QDialog*>("sketch.axis3d.dialog");QVERIFY(panel && panel->isVisible());QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));auto enter=[&](const QString& key,const QString& text){auto* spin=panel->findChild<QDoubleSpinBox*>("sketch.axis3d."+key);click(spin);QVERIFY(spin==QApplication::focusWidget() || spin->isAncestorOf(QApplication::focusWidget()));QTest::keySequence(QApplication::focusWidget(),QKeySequence::SelectAll);QTest::keyClicks(QApplication::focusWidget(),text);QTest::keyClick(QApplication::focusWidget(),Qt::Key_Tab);};enter("u","6");enter("v","5");click(panel->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok));QTRY_VERIFY(!w.findChild<QDialog*>("sketch.axis3d.dialog"));const auto pivot=draft(w);QCOMPARE(pivot["parameters"].toObject()["rotateGroups"],sized["parameters"].toObject()["rotateGroups"]);capture("resized-source-pivot6-5");call(w,"history.undo");QCOMPARE(draft(w),sized);call(w,"history.redo");QCOMPARE(draft(w),pivot);call(w,"sketch.finish");const auto inspection=call(w,"document.inspect");const auto finalBody=inspection["evaluation"].toObject()["bodies"].toArray().first().toObject();QCOMPARE(finalBody["bodyId"],body["bodyId"]);QVERIFY(std::abs(finalBody["volumeMm3"].toDouble()-2003.0545357589)<1e-5);QCOMPARE(inspection["evaluation"].toObject()["sketches"].toArray().first().toObject()["profiles"].toArray().first().toObject()["profileId"],profile);call(w,"history.undo");QCOMPARE(call(w,"document.inspect")["authored"],before["authored"]);call(w,"history.redo");QCOMPARE(call(w,"document.inspect")["authored"],inspection["authored"]);call(w,"view.home");capture("resized-rotated-solid");
  if(!output.isEmpty()){call(w,"document.save",{{"path",output+"/resized-rotated-profile.alloy"}});call(w,"document.exportStl",{{"path",output+"/resized-rotated-profile.stl"},{"bodyId",body["bodyId"]},{"units","mm"},{"format","binary"},{"surfaceDeviationMm",.01},{"normalDeviationDeg",5}});call(w,"document.open",{{"path",output+"/resized-rotated-profile.alloy"}});QCOMPARE(call(w,"document.inspect")["authored"],inspection["authored"]);QFile file(output+"/native-report.json");QVERIFY(file.open(QIODevice::WriteOnly));file.write(QJsonDocument(QJsonObject{{"passed",true},{"sketchId",sid},{"dimensionId",dim["id"]},{"legacy",legacy},{"sized",sized},{"pivot",pivot},{"baselineDocument",before},{"authored",inspection["authored"]},{"state",w.state()},{"transcript",transcript_}}).toJson());}call(w,"document.new",{{"discard",true}});w.close();
 }
};
QTEST_MAIN(SketchRotateSourceSizeControls)
#include "sketch_rotate_source_size_controls.moc"
