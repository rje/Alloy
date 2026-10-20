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
class PatternCanvasControls final:public QObject{
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
 void picksHandlesAndPrintableCut(){
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1500,950);w.show();w.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&w));QApplication::setActiveWindow(&w);
  call(w,"sketch.begin",{{"plane","XY"},{"name","Canvas-picked hole grid"}});QStringList axes;
  for(const auto& xy:QJsonArray{QJsonArray{40,0},QJsonArray{0,40}}){const auto end=xy.toArray();call(w,"sketch.line",{{"x1",0},{"y1",0},{"x2",end[0]},{"y2",end[1]},{"units","mm"}});const auto id=draft(w)["parameters"].toObject()["entities"].toArray().last().toObject()["id"].toString();axes.append(id);call(w,"sketch.construction",{{"entityIds",QJsonArray{id}},{"construction",true}});}
  call(w,"sketch.addCircle",{{"centerX",8},{"centerY",7},{"diameter",3},{"units","mm"}});const auto source=draft(w)["parameters"].toObject()["entities"].toArray().last().toObject();call(w,"sketch.dimension",{{"kind","radius"},{"entityId",source["id"]},{"value",1.5},{"units","mm"}});const auto radius=draft(w)["parameters"].toObject()["dimensions"].toArray().last().toObject().value("id");call(w,"sketch.lookAt");
  ReferenceView* view=nullptr;for(auto* widget:w.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(widget))view=v;QVERIFY(view);
  const auto baseline=draft(w);const auto history=active(w).value("draftHistory");const auto sketchId=baseline["id"].toString();
  auto pixel=[&](double u,double v){const auto p=call(w,"view.projectSketchPoint",{{"sketchId",sketchId},{"uMm",u},{"vMm",v}});return QPoint(qRound(p["x"].toDouble()),qRound(p["y"].toDouble()));};
  auto* create=w.findChild<QToolButton*>("sketch.menu.create");auto* menu=w.findChild<QMenu*>("sketch.commands.create");auto* action=w.findChild<QAction*>("sketch.action.rectangularPattern");QVERIFY(create && menu && action);
  auto open=[&]()->QDialog*{bool ready=false;QTimer::singleShot(0,[&]{if(menu->isVisible() && QApplication::activePopupWidget()==menu){const auto at=menu->actionGeometry(action).center();auto* hit=QApplication::widgetAt(menu->mapToGlobal(at));if(hit==menu){ready=true;QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,at);return;}}menu->close();});click(create);if(!ready)return nullptr;return w.findChild<QDialog*>("sketch.pattern.dialog");};
  auto* dialog=open();QVERIFY(dialog);QTRY_VERIFY(dialog->isVisible());QTRY_VERIFY(!menu->isVisible());QCOMPARE(interaction(w)["role"].toString(),QString("objects"));
  auto output=qEnvironmentVariable("ALLOY_PATTERN_CANVAS_EVIDENCE");if(!output.isEmpty()){output=QDir(output).absolutePath();QDir().mkpath(output);}
  auto capture=[&](const QString& name){if(!output.isEmpty())call(w,"capture.screenshot",{{"path",output+"/"+name+".png"}});};
  auto hover=[&](const QPoint& at){QCOMPARE(QApplication::widgetAt(view->mapToGlobal(at)),static_cast<QWidget*>(view));QVERIFY(view->windowHandle());QTest::mouseMove(view->windowHandle(),at);};
  hover(pixel(9.5,7));QTRY_COMPARE(interaction(w)["hoveredEntityId"],source["id"]);capture("source-hover");click(view,pixel(9.5,7));QCOMPARE(interaction(w)["role"].toString(),QString("direction1"));QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
  hover(pixel(20,0));QTRY_COMPARE(interaction(w)["hoveredEntityId"].toString(),axes[0]);click(view,pixel(20,0));QCOMPARE(interaction(w)["role"].toString(),QString("direction2"));const auto prior=active(w).value("pendingSketchPattern");click(view,pixel(20,0));QCOMPARE(interaction(w)["role"].toString(),QString("direction2"));QCOMPARE(active(w).value("pendingSketchPattern"),prior);QCOMPARE(interaction(w)["pickedEntityIds"].toArray().size(),2);
  hover(pixel(0,20));QTRY_COMPARE(interaction(w)["hoveredEntityId"].toString(),axes[1]);click(view,pixel(0,20));QCOMPARE(interaction(w)["role"].toString(),QString("values"));QCOMPARE(interaction(w)["pickedEntityIds"].toArray().size(),3);QCOMPARE(draft(w),baseline);QCOMPARE(active(w).value("draftHistory"),history);QCOMPARE(gpu(w)["sketchPatternHandles"].toArray().size(),4);capture("picked-grid-handles");
  auto drag=[&](const QString& key,double wanted){QJsonObject handle;for(const auto& row:gpu(w)["sketchPatternHandles"].toArray())if(row.toObject()["key"]==key)handle=row.toObject();QVERIFY(!handle.isEmpty());const QPoint start(qRound(handle["x"].toDouble()),qRound(handle["y"].toDouble()));const double delta=(wanted-handle["value"].toDouble())*(handle["kind"]=="quantity" ? handle["spacingMm"].toDouble() : 1);const auto end=pixel(handle["uMm"].toDouble()+delta*handle["unitU"].toDouble(),handle["vMm"].toDouble()+delta*handle["unitV"].toDouble());QCOMPARE(QApplication::widgetAt(view->mapToGlobal(start)),static_cast<QWidget*>(view));QVERIFY(view->rect().contains(end));QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,start);QVERIFY(active(w).contains("patternHandleGesture"));QTest::mouseMove(view,end);if(key=="spacing1Mm" && wanted==2){QVERIFY(active(w)["patternHandleGesture"].toObject()["invalid"].toBool());capture("invalid-drag-refused");}QTest::mouseRelease(view,Qt::LeftButton,Qt::NoModifier,end);QVERIFY(!active(w).contains("patternHandleGesture"));};
  drag("spacing1Mm",16);if(QTest::currentTestFailed())return;QCOMPARE(dialog->findChild<QDoubleSpinBox*>("sketch.pattern.spacing1")->value(),16.);drag("spacing2Mm",12);if(QTest::currentTestFailed())return;QCOMPARE(dialog->findChild<QDoubleSpinBox*>("sketch.pattern.spacing2")->value(),12.);drag("quantity1",3);if(QTest::currentTestFailed())return;QCOMPARE(dialog->findChild<QSpinBox*>("sketch.pattern.quantity1")->value(),3);drag("quantity1",2);if(QTest::currentTestFailed())return;drag("quantity2",4);if(QTest::currentTestFailed())return;QCOMPARE(dialog->findChild<QSpinBox*>("sketch.pattern.quantity2")->value(),4);drag("quantity2",3);if(QTest::currentTestFailed())return;
  const auto valid=active(w).value("pendingSketchPattern");drag("spacing1Mm",2);if(QTest::currentTestFailed())return;QCOMPARE(active(w).value("pendingSketchPattern"),valid);QCOMPARE(dialog->findChild<QDoubleSpinBox*>("sketch.pattern.spacing1")->value(),16.);QCOMPARE(draft(w),baseline);capture("dragged-grid-preview");
  click(dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Cancel));QTRY_VERIFY(!w.findChild<QDialog*>("sketch.pattern.dialog"));QCOMPARE(draft(w),baseline);QCOMPARE(active(w).value("draftHistory"),history);
  dialog=open();QVERIFY(dialog);click(view,pixel(9.5,7));click(view,pixel(20,0));click(view,pixel(0,20));QCOMPARE(interaction(w)["role"].toString(),QString("values"));drag("spacing1Mm",16);drag("spacing2Mm",12);if(QTest::currentTestFailed())return;QTest::keyClick(view,Qt::Key_Escape);QTRY_VERIFY(!w.findChild<QDialog*>("sketch.pattern.dialog"));QCOMPARE(draft(w),baseline);
  dialog=open();QVERIFY(dialog);click(view,pixel(9.5,7));click(view,pixel(20,0));click(view,pixel(0,20));drag("spacing1Mm",16);drag("spacing2Mm",12);if(QTest::currentTestFailed())return;click(dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok));QTRY_VERIFY(!w.findChild<QDialog*>("sketch.pattern.dialog"));const auto accepted=draft(w);QCOMPARE(accepted["parameters"].toObject()["patternGroups"].toArray().first().toObject()["spacing1Mm"].toDouble(),16.);call(w,"history.undo");QCOMPARE(draft(w),baseline);call(w,"history.redo");QCOMPARE(draft(w),accepted);
  const auto glyph=gpu(w)["sketchPatternGlyphs"].toArray().first().toObject()["bounds"].toObject();const QPoint glyphAt(qRound(glyph["x"].toDouble()+glyph["width"].toDouble()/2),qRound(glyph["y"].toDouble()+glyph["height"].toDouble()/2));QCOMPARE(QApplication::widgetAt(view->mapToGlobal(glyphAt)),static_cast<QWidget*>(view));QTest::mouseDClick(view,Qt::LeftButton,Qt::NoModifier,glyphAt);dialog=w.findChild<QDialog*>("sketch.pattern.dialog");QVERIFY(dialog);QCOMPARE(interaction(w)["role"].toString(),QString("values"));drag("spacing1Mm",17);if(QTest::currentTestFailed())return;click(dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Cancel));QTRY_VERIFY(!w.findChild<QDialog*>("sketch.pattern.dialog"));QCOMPARE(draft(w),accepted);
  call(w,"sketch.finish");call(w,"solid.box",{{"width",42},{"depth",40},{"height",5},{"units","mm"},{"name","Canvas pattern plate"}});auto inspection=call(w,"document.inspect");const auto body=inspection["evaluation"].toObject()["bodies"].toArray().first().toObject();QJsonArray profiles;for(const auto& p:inspection["evaluation"].toObject()["sketches"].toArray().first().toObject()["profiles"].toArray())profiles.append(p.toObject()["profileId"]);QCOMPARE(profiles.size(),6);call(w,"solid.extrude",{{"sketchId",sketchId},{"profileIds",profiles},{"distance",5},{"units","mm"},{"operation","cut"},{"targetBodyIds",QJsonArray{body["bodyId"]}}});inspection=call(w,"document.inspect");QVERIFY(std::abs(inspection["evaluation"].toObject()["bodies"].toArray().first().toObject()["volumeMm3"].toDouble()-(8400-6*std::numbers::pi*1.5*1.5*5))<1e-5);call(w,"view.home");capture("native-picked-six-hole-plate");
  if(!output.isEmpty()){call(w,"document.save",{{"path",output+"/native-picked-grid.alloy"}});call(w,"document.exportStl",{{"path",output+"/native-picked-grid.stl"},{"bodyId",body["bodyId"]},{"units","mm"},{"format","binary"},{"surfaceDeviationMm",.01},{"normalDeviationDeg",5}});const auto authored=inspection["authored"];call(w,"document.open",{{"path",output+"/native-picked-grid.alloy"}});QCOMPARE(call(w,"document.inspect").value("authored"),authored);QFile report(output+"/native-report.json");QVERIFY(report.open(QIODevice::WriteOnly));report.write(QJsonDocument(QJsonObject{{"passed",true},{"authored",authored},{"sketchId",sketchId},{"sourceEntityId",source["id"]},{"sourceCenterPointId",source["centerPointId"]},{"radiusDimensionId",radius},{"state",w.state()},{"transcript",transcript_}}).toJson());}
  w.close();
 }
};
QTEST_MAIN(PatternCanvasControls)
#include "sketch_pattern_canvas_controls.moc"
