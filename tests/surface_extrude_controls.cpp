// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QTest>
#include <QToolButton>
#include <QAction>
#include <QComboBox>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QDockWidget>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QJsonDocument>
#include <QDir>
#include <QTemporaryDir>
#include <QElapsedTimer>
#include <QStatusBar>

class SurfaceExtrudeControlsTest final:public QObject {
 Q_OBJECT
 QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& arguments={}){
  auto result=window.execute(command,arguments);
  if(result["completion"]=="pending"){
   const auto initial=result["result"].toObject();QElapsedTimer deadline;deadline.start();
   do{QTest::qWait(25);result=window.execute("command.status",{{"session",initial["id"]}});
    if(deadline.elapsed()>120000)qFatal("Surface Extrude evaluation timed out");
   }while(result["completion"]=="pending");
  }
  if(!result["ok"].toBool())qFatal("%s",QJsonDocument(result).toJson().constData());
  return result["result"].toObject();
 }
 QJsonObject session(AlloyWindow& window){return window.state()["activeSession"].toObject();}
private slots:
 void asynchronousSessionContracts(){
  QTemporaryDir temp;QVERIFY(temp.isValid());CadDocument fixture;
  auto sketch=SketchGeometry::newFeature(fixture.authored()["documentId"].toString(),"XZ","Source");
  sketch=SketchGeometry::addLine(sketch,{{"u1Mm",0},{"v1Mm",0},{"u2Mm",20},{"v2Mm",5}});
  auto authored=fixture.authored();authored["features"]=QJsonArray{sketch};QVERIFY(fixture.commit(authored,"Sketch").isEmpty());
  const auto path=temp.filePath("source.alloy");QVERIFY(fixture.save(path).isEmpty());
  AlloyWindow w;call(w,"document.open",{{"path",path},{"discard",true}});const auto before=call(w,"document.inspect");
  const QJsonValue entity=sketch["parameters"].toObject()["entities"].toArray()[0].toObject()["id"];
  const QJsonObject args{{"sketchId",sketch["id"]},{"entityIds",QJsonArray{entity}},{"sideOneMm",10},{"sideTwoMm",12}};
  auto raw=w.execute("surface.extrudeDialog",args);QVERIFY(raw["ok"].toBool());QCOMPARE(raw["completion"].toString(),QString("pending"));
  const QJsonValue id=raw["result"].toObject().value("id"),generation=raw["result"].toObject().value("previewGeneration");
  raw=w.execute("command.preview",{{"session",id}});QCOMPARE(raw["completion"].toString(),QString("pending"));QCOMPARE(raw["result"].toObject()["previewGeneration"],generation);
  raw=w.execute("command.commit",{{"session",id}});QVERIFY(!raw["ok"].toBool());QCOMPARE(raw["error"].toObject()["code"].toString(),QString("preview_required"));
  raw=w.execute("command.update",{{"session",id},{"inputs",QJsonObject{{"entityIds",QJsonArray{}}}}});QVERIFY(!raw["ok"].toBool());
  QCOMPARE(session(w)["evaluationStatus"].toString(),QString("failed"));
  raw=w.execute("command.update",{{"session",id},{"inputs",QJsonObject{{"entityIds",QJsonArray{entity}}}}});
  QVERIFY(raw["ok"].toBool());QCOMPARE(raw["completion"].toString(),QString("pending"));
  QVERIFY(raw["result"].toObject()["previewGeneration"].toInteger()>generation.toInteger());
  QCOMPARE(w.execute("command.status",{{"session",id},{"generation",generation}})["error"].toObject()["code"].toString(),QString("stale_preview"));
  QTRY_COMPARE_WITH_TIMEOUT(session(w)["evaluationStatus"].toString(),QString("completed"),10000);
  QVERIFY(session(w)["previewed"].toBool());
  raw=w.execute("command.update",{{"session",id},{"inputs",QJsonObject{{"sideOneMm",25}}}});QVERIFY(raw["ok"].toBool());QVERIFY(raw["result"].toObject()["previewGeneration"].toInteger()>generation.toInteger());
  raw=w.execute("command.cancel",{{"session",id}});QVERIFY(raw["ok"].toBool());QVERIFY(raw["result"].toObject()["resultDiscarded"].toBool());
  QVERIFY(w.statusBar()->currentMessage().contains("Finishing background geometry"));
  QCOMPARE(w.execute("command.status",{{"session",id}})["error"].toObject()["code"].toString(),QString("invalid_session"));
  QTRY_VERIFY_WITH_TIMEOUT(!w.state()["geometryWork"].toObject()["active"].toBool(),10000);
  QVERIFY(session(w).isEmpty());QVERIFY(!w.statusBar()->currentMessage().contains("Finishing background geometry"));QCOMPARE(call(w,"document.inspect")["authored"],before["authored"]);
  const auto ready=call(w,"surface.extrudeDialog",args);QVERIFY(ready["previewed"].toBool());QCOMPARE(ready["evaluationStatus"].toString(),QString("completed"));
  call(w,"command.commit",{{"session",ready["id"]}});const auto after=call(w,"document.inspect");QCOMPARE(after["features"].toInt(),2);
  const auto featureId=after["authored"].toObject()["features"].toArray().last().toObject().value("id");
  const auto unchanged=call(w,"feature.editDialog",{{"featureId",featureId}});call(w,"command.commit",{{"session",unchanged["id"]}});
  QCOMPARE(call(w,"document.inspect")["authored"],after["authored"]);QCOMPARE(call(w,"document.inspect")["undo"],after["undo"]);
  call(w,"history.undo");QCOMPARE(call(w,"document.inspect")["authored"],before["authored"]);
  call(w,"history.redo");QCOMPARE(call(w,"document.inspect")["authored"],after["authored"]);
  const auto saved=temp.filePath("committed.alloy");call(w,"document.save",{{"path",saved}});call(w,"document.open",{{"path",saved},{"discard",true}});QCOMPARE(call(w,"document.inspect")["authored"],after["authored"]);
  // Prepared adoption must reject even an otherwise valid stale revision.
  auto candidate=CadDocument::surfaceExtrudeCandidate(fixture.authored(),args);const auto evaluated=CadDocument::evaluateCandidate(candidate);
  QVERIFY(evaluated.ok);QVERIFY(!fixture.commitSurfaceExtrudeEvaluation(evaluated,candidate,fixture.authored(),fixture.revision()+1,"Stale").isEmpty());
  QCOMPARE(fixture.authored(),before["authored"].toObject());
 }
 void historicalSuffixNormalization(){
  QTemporaryDir temp;QVERIFY(temp.isValid());CadDocument d;const auto owner=d.authored()["documentId"].toString();
  auto source=SketchGeometry::newFeature(owner,"XZ","Source");source=SketchGeometry::addLine(source,{{"u1Mm",0},{"v1Mm",0},{"u2Mm",20},{"v2Mm",0}});
  auto authored=d.authored();authored["features"]=QJsonArray{source};QVERIFY(d.commit(authored,"Source").isEmpty());
  const QJsonValue entity=source["parameters"].toObject()["entities"].toArray()[0].toObject()["id"];
  QVERIFY(d.commit(CadDocument::surfaceExtrudeCandidate(d.authored(),{{"sketchId",source["id"]},{"entityIds",QJsonArray{entity}},{"sideOneMm",10},{"sideTwoMm",12}}),"Sheet").isEmpty());
  const auto feature=d.authored()["features"].toArray().last().toObject();const auto face=d.bodies().back().supportFaces[0];
  auto attached=SketchGeometry::newSupportedFeature(owner,{{"producerFeatureId",face.producerFeatureId},{"bodyId",face.bodyId},{"faceId",face.faceId}},"Dependent offset",GeometryKernel::supportEdgeDescriptors(face));
  const auto context=GeometryKernel::projectionContext(d.evaluation(),attached);QJsonArray projections;
  for(const auto& row:attached["parameters"].toObject()["projections"].toArray())projections.append(row.toObject()["id"]);
  const auto offset=SketchGeometry::addClosedRectOffset(attached,projections,1,context);QVERIFY2(offset.ok,qPrintable(offset.error));attached=offset.feature;
  auto candidate=d.authored();auto features=candidate["features"].toArray();features.append(attached);candidate["features"]=features;QVERIFY(d.commit(candidate,"Dependent sketch").isEmpty());
  QVERIFY(d.commit(CadDocument::removeBodyCandidate(d.authored(),{{"bodyId",face.bodyId},{"ownerId",owner}}),"Consume sheet").isEmpty());QVERIFY(d.bodies().empty());
  const auto before=d.authored();const auto path=temp.filePath("historical.alloy");QVERIFY(d.save(path).isEmpty());
  AlloyWindow w;call(w,"document.open",{{"path",path},{"discard",true}});
  const auto edit=call(w,"feature.editDialog",{{"featureId",feature["id"]}});QVERIFY(edit["previewed"].toBool());QCOMPARE(edit["resultBodyIds"].toArray().size(),1);
  const auto changed=call(w,"command.update",{{"session",edit["id"]},{"inputs",QJsonObject{{"sideOneMm",25}}}});QVERIFY(changed["previewed"].toBool());
  QVERIFY(changed["documentDraft"].toObject()["features"].toArray()[2]!=before["features"].toArray()[2]);
  call(w,"command.commit",{{"session",edit["id"]}});const auto committed=call(w,"document.inspect");QCOMPARE(committed["bodies"].toInt(),0);
  call(w,"history.undo");QCOMPARE(call(w,"document.inspect")["authored"],before);
  call(w,"history.redo");QCOMPARE(call(w,"document.inspect")["authored"],committed["authored"]);
 }
 void surfaceLoftRoute(){
  AlloyWindow window;
  call(window,"ui.designTab",{{"tab","surface"}});
  call(window,"surface.loftDialog");
  QCOMPARE(session(window)["inputs"].toObject()["operation"].toString(),QString("newComponent"));
  QVERIFY(session(window)["surfaceOnly"].toBool());
  auto* operation=window.findChild<QComboBox*>("loftOperation");QVERIFY(operation && !operation->isEnabled());
  QVERIFY(!window.execute("surface.loftDialog")["ok"].toBool());
  QVERIFY(!window.execute("command.update",{{"session",session(window)["id"]},{"inputs",QJsonObject{{"operation","newBody"}}}})["ok"].toBool());
  call(window,"command.cancel",{{"session",session(window)["id"]}});
 }
 void nativeOpenCurveTwoSidesCancel(){
  const auto fixture=qEnvironmentVariable("ALLOY_SURFACE_EXTRUDE_SOURCE");QVERIFY(!fixture.isEmpty());
  const auto output=qEnvironmentVariable("ALLOY_SURFACE_EXTRUDE_EVIDENCE");QVERIFY(!output.isEmpty());QDir().mkpath(output);
  AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1440,900);window.show();window.activateWindow();
  QVERIFY(QTest::qWaitForWindowExposed(&window));
  ReferenceView* view=nullptr;for(auto* child:window.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;
  QVERIFY(view);
  call(window,"document.open",{{"path",fixture},{"discard",true}});call(window,"view.home");
  QApplication::setActiveWindow(&window);window.setFocus();
  const auto before=call(window,"document.inspect");
  const auto source=before["authored"].toObject()["features"].toArray().last().toObject();
  const auto sketchId=source["id"].toString();QVERIFY(source["type"]=="sketch");
  auto* tab=window.findChild<QToolButton*>("design.tab.surface");QVERIFY(tab);
  QTest::mouseClick(tab,Qt::LeftButton);
  QCOMPARE(window.state()["ui"].toObject()["tab"].toString(),QString("surface"));
  QVERIFY(!window.findChild<QAction*>("solid.action.surface.extrudeDialog"));
  auto* start=window.findChild<QToolButton*>("surface.command.surface.extrudeDialog");QVERIFY(start && start->isVisible());
  QTest::mouseClick(start,Qt::LeftButton);
  QTRY_COMPARE(session(window)["command"].toString(),QString("surface.extrude"));
  auto* dock=window.findChild<QDockWidget*>("surfaceExtrudeDock");QVERIFY(dock && dock->isVisible());
  auto* chaining=dock->findChild<QCheckBox*>("surfaceExtrude.chaining");QVERIFY(chaining && chaining->isChecked());QTest::mouseClick(chaining,Qt::LeftButton);
  const auto mapped=call(window,"view.projectSketchPoint",{{"sketchId",sketchId},{"uMm",25},{"vMm",7}});
  QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,QPoint(qRound(mapped["x"].toDouble()),qRound(mapped["y"].toDouble())));
  QTRY_COMPARE(session(window)["inputs"].toObject()["profiles"].toArray().size(),1);
  auto* side1=dock->findChild<QDoubleSpinBox*>("sideOneMm");auto* side2=dock->findChild<QDoubleSpinBox*>("sideTwoMm");
  QVERIFY(side1 && side2);
  QTest::mouseClick(side1,Qt::LeftButton);QTest::keyClick(side1,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(side1,"25");QTest::keyClick(side1,Qt::Key_Tab);
  QTest::mouseClick(side2,Qt::LeftButton);QTest::keyClick(side2,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(side2,"30");QTest::keyClick(side2,Qt::Key_Tab);
  QTRY_VERIFY_WITH_TIMEOUT(session(window)["previewed"].toBool(),120000);
  const auto inputs=session(window)["inputs"].toObject();
  QCOMPARE(inputs["sideOneMm"].toDouble(),25.0);QCOMPARE(inputs["sideTwoMm"].toDouble(),30.0);
  QCOMPARE(session(window)["resultBodies"].toArray().first().toObject()["geometry"].toObject()["topology"].toObject()["faces"].toInt(),1);
  auto* buttons=dock->findChild<QDialogButtonBox*>();QVERIFY(buttons && buttons->button(QDialogButtonBox::Ok)->isEnabled());
  const auto preview=session(window);
  auto* solidTab=window.findChild<QToolButton*>("design.tab.solid");QVERIFY(solidTab);
  QTest::mouseClick(solidTab,Qt::LeftButton);QCOMPARE(session(window),preview);
  QTest::mouseClick(tab,Qt::LeftButton);QCOMPARE(session(window),preview);
  call(window,"capture.screenshot",{{"path",output+"/native-two-sides-preview.png"}});
  QVERIFY(QApplication::focusWidget());
  QTest::keyClick(QApplication::focusWidget(),Qt::Key_Escape);
  QTRY_VERIFY(session(window).isEmpty());
  QCOMPARE(call(window,"document.inspect")["authored"],before["authored"]);
  QVERIFY(view->diagnostics()["commandSketches"].toArray().isEmpty());
 }
};
QTEST_MAIN(SurfaceExtrudeControlsTest)
#include "surface_extrude_controls.moc"
