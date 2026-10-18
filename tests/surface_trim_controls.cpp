// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QTest>
#include <QToolButton>
#include <QDockWidget>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QJsonDocument>
#include <QDir>
#include <QFile>
#include <QListWidget>
#include <QMenu>
#include <QTimer>
#include <QAction>
#include <QElapsedTimer>
#include <QTemporaryDir>

class SurfaceTrimControlsTest final:public QObject {
 Q_OBJECT
 QJsonObject response(AlloyWindow& window,const QString& command,const QJsonObject& args={}){
  auto result=window.execute(command,args);const auto initial=result.value("result").toObject();QElapsedTimer deadline;deadline.start();
  while(result.value("completion")=="pending"){
   QTest::qWait(25);result=window.execute("command.status",{{"session",initial.value("id")},{"generation",initial.value("previewGeneration")}});
   if(deadline.elapsed()>180000)qFatal("Trim evaluation timed out");
  }
  return result;
 }
 QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& args={}){
  const auto result=response(window,command,args);
  if(!result.value("ok").toBool())qFatal("%s",QJsonDocument(result).toJson().constData());return result.value("result").toObject();
 }
 QJsonObject session(AlloyWindow& window){return window.state()["activeSession"].toObject();}
private slots:
 void asynchronousPartitionCommitContracts(){
  QTemporaryDir temp;QVERIFY(temp.isValid());CadDocument fixture;
  auto sheet=[&](const QString& plane,double a,double b,double reach,double offset=0){
   auto sketch=SketchGeometry::newFeature(fixture.authored().value("documentId").toString(),plane,"Source");
   sketch=SketchGeometry::addLine(sketch,{{"u1Mm",a},{"v1Mm",offset},{"u2Mm",b},{"v2Mm",offset}});
   auto authored=fixture.authored();auto features=authored.value("features").toArray();features.append(sketch);authored["features"]=features;
   if(!fixture.commit(authored,"Sketch").isEmpty())qFatal("Sketch fixture failed");
   const auto entity=sketch.value("parameters").toObject().value("entities").toArray()[0].toObject().value("id");
   if(!fixture.commit(CadDocument::surfaceExtrudeCandidate(fixture.authored(),{{"sketchId",sketch.value("id")},{"entityIds",QJsonArray{entity}},{"sideOneMm",reach},{"sideTwoMm",reach}}),"Sheet").isEmpty())qFatal("Sheet fixture failed");
   return fixture.bodies().back().bodyId;
  };
  const auto target=sheet("XY",-20,20,10),tool=sheet("XZ",-30,30,20),badTool=sheet("XZ",-30,30,20,30);const auto path=temp.filePath("trim.alloy");QVERIFY(fixture.save(path).isEmpty());
  AlloyWindow w;call(w,"document.open",{{"path",path},{"discard",true}});const auto before=call(w,"document.inspect");
  const QJsonObject inputs{{"targetBodyId",target},{"toolBodyId",tool}};
  auto pending=w.execute("surface.trimDialog",inputs);QCOMPARE(pending.value("completion").toString(),QString("pending"));auto operation=pending.value("result").toObject();
  QVERIFY(w.state().value("geometryWork").toObject().value("resultDiscardAvailable").toBool());
  auto* dock=w.findChild<QDockWidget*>("surfaceTrimDock");QVERIFY(dock);auto* buttons=dock->findChild<QDialogButtonBox*>();QVERIFY(buttons);
  QVERIFY(!buttons->button(QDialogButtonBox::Ok)->isEnabled());QVERIFY(buttons->button(QDialogButtonBox::Cancel)->isEnabled());QVERIFY(!dock->findChild<QListWidget*>("surfaceTrim.regions")->isEnabled());
  QCOMPARE(w.execute("command.update",{{"session",operation.value("id")},{"inputs",QJsonObject{{"name","Changed"}}}}).value("error").toObject().value("code").toString(),QString("evaluation_busy"));
  QVERIFY(call(w,"command.cancel",{{"session",operation.value("id")}}).value("resultDiscarded").toBool());
  QTRY_VERIFY_WITH_TIMEOUT(!w.state().value("geometryWork").toObject().value("active").toBool(),10000);QCOMPARE(call(w,"document.inspect"),before);
  QCOMPARE(w.execute("command.status",{{"session",operation.value("id")}}).value("error").toObject().value("code").toString(),QString("command_cancelled"));
  pending=w.execute("surface.trimDialog",inputs);operation=pending.value("result").toObject();
  QPushButton* change=nullptr;for(auto* button:w.findChild<QDockWidget*>("surfaceTrimDock")->findChildren<QPushButton*>())if(button->text()=="Change Tools")change=button;
  QVERIFY(change && !change->isEnabled());QVERIFY(QMetaObject::invokeMethod(change,"clicked",Qt::DirectConnection,Q_ARG(bool,false)));
  QJsonObject stale;QTRY_VERIFY_WITH_TIMEOUT((stale=w.execute("command.status",{{"session",operation.value("id")}})).value("completion")!="pending",10000);
  QCOMPARE(stale.value("error").toObject().value("code").toString(),QString("stale_session"));QCOMPARE(call(w,"document.inspect"),before);call(w,"command.cancel",{{"session",operation.value("id")}});
  const auto failed=response(w,"surface.trimDialog",{{"targetBodyId",target},{"toolBodyId",badTool}});QVERIFY(!failed.value("ok").toBool());QCOMPARE(failed.value("completion").toString(),QString("failed"));
  QCOMPARE(call(w,"document.inspect"),before);call(w,"command.cancel",{{"session",failed.value("result").toObject().value("id")}});
  auto ready=call(w,"surface.trimDialog",inputs);QCOMPARE(ready.value("evaluationStatus").toString(),QString("completed"));QCOMPARE(ready.value("regions").toArray().size(),2);QVERIFY(!ready.value("previewed").toBool());
  const auto region=ready.value("regions").toArray()[0].toObject().value("regionId");call(w,"surface.trimRegion",{{"regionId",region}});
  const auto preview=session(w);QVERIFY(preview.value("previewed").toBool());const auto expectedInputs=preview.value("inputs").toObject();
  pending=w.execute("command.commit",{{"session",ready.value("id")}});QCOMPARE(pending.value("completion").toString(),QString("pending"));
  call(w,"command.cancel",{{"session",ready.value("id")}});QTRY_VERIFY_WITH_TIMEOUT(!w.state().value("geometryWork").toObject().value("active").toBool(),10000);QCOMPARE(call(w,"document.inspect"),before);
  auto reopened=call(w,"surface.trimDialog",inputs);call(w,"surface.trimRegion",{{"regionId",reopened.value("regions").toArray()[0].toObject().value("regionId")}});
  const auto accepted=call(w,"command.commit",{{"session",reopened.value("id")}});QCOMPARE(accepted.value("status").toString(),QString("committed"));QVERIFY(session(w).isEmpty());
  const auto after=call(w,"document.inspect");QCOMPARE(after.value("undo").toObject().value("count").toInt(),1);const auto feature=after.value("authored").toObject().value("features").toArray().last().toObject();
  auto args=expectedInputs;args["removedRegionIds"]=feature.value("parameters").toObject().value("removedRegionIds");args["newFeatureId"]=feature.value("id");
  QVERIFY(fixture.commit(CadDocument::surfaceTrimCandidate(fixture.authored(),args),"Baseline").isEmpty());QCOMPARE(after.value("evaluation").toObject().value("bodies"),fixture.state().value("evaluation").toObject().value("bodies"));
  call(w,"history.undo");QCOMPARE(call(w,"document.inspect").value("authored"),before.value("authored"));call(w,"history.redo");QCOMPARE(call(w,"document.inspect").value("authored"),after.value("authored"));
  const auto edit=call(w,"feature.editDialog",{{"featureId",feature.value("id")}});QVERIFY(edit.value("previewed").toBool());call(w,"command.cancel",{{"session",edit.value("id")}});
  const auto saved=temp.filePath("saved.alloy");call(w,"document.save",{{"path",saved}});call(w,"document.open",{{"path",saved},{"discard",true}});QCOMPARE(call(w,"document.inspect").value("evaluation").toObject().value("bodies"),after.value("evaluation").toObject().value("bodies"));
 }
 void actualCurvedToolFirstTargetPreviewCancel(){
  const auto source=qEnvironmentVariable("ALLOY_CURVED_TRIM_SOURCE"),out=qEnvironmentVariable("ALLOY_SURFACE_TRIM_EVIDENCE");
  const auto selection=qEnvironmentVariable("ALLOY_CURVED_TRIM_SELECTION");if(source.isEmpty())QSKIP("No actual four-body curved Trim source supplied");
  QFile receipt(selection);QVERIFY(receipt.open(QIODevice::ReadOnly));const auto metadata=QJsonDocument::fromJson(receipt.readAll()).object();
  QDir().mkpath(out);AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1500,950);window.show();window.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&window));
  ReferenceView* view=nullptr;for(auto* child:window.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;QVERIFY(view);
  call(window,"document.open",{{"path",source},{"discard",true}});auto inspected=call(window,"document.inspect");
  // Display setup only: expose the Inner bridge behind the exterior. Source
  // curves, four bodies and both finite sheet tools remain unchanged.
  const auto exterior=inspected["evaluation"].toObject()["bodies"].toArray()[0].toObject();
  call(window,"feature.visibility",{{"featureId",exterior["featureId"]},{"visible",false}});call(window,"view.home");
  const auto before=call(window,"document.inspect");QApplication::setActiveWindow(&window);window.setFocus(); // Initial harness activation only.
  auto* tab=window.findChild<QToolButton*>("design.tab.surface");QVERIFY(tab);QTest::mouseClick(tab,Qt::LeftButton);
  auto* opener=window.findChild<QToolButton*>("surface.menu.modify");QVERIFY(opener && opener->isVisible());auto* menu=opener->menu();QVERIFY(menu);
  bool active=false,clicked=false;QTimer::singleShot(50,opener,[&]{active=QApplication::activePopupWidget()==menu;
   for(auto* a:menu->actions())if(a->objectName()=="surface.action.surface.trimDialog" && active){const auto point=menu->actionGeometry(a).center();
    if(QApplication::widgetAt(menu->mapToGlobal(point))==menu){QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,point);clicked=true;}break;}});
  QTimer watchdog;watchdog.setSingleShot(true);connect(&watchdog,&QTimer::timeout,opener,[&]{menu->close();});watchdog.start(2000);QTest::mouseClick(opener,Qt::LeftButton);watchdog.stop();
  QVERIFY(active && clicked);QTRY_VERIFY(QApplication::activePopupWidget()==nullptr);
  QCOMPARE(session(window)["selectionRole"].toString(),QString("tool"));QVERIFY(session(window)["inputs"].toObject()["targetBodyId"].toString().isEmpty());QVERIFY(view->hasFocus());
  const auto toolPixel=view->projectWorldPoint({102,-55,38}).toPoint();QCOMPARE(QApplication::widgetAt(view->mapToGlobal(toolPixel)),static_cast<QWidget*>(view));
  QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,toolPixel);QTRY_COMPARE(session(window)["inputs"].toObject()["toolBodyId"],metadata["upperToolId"]);
  QCOMPARE(session(window)["selectionRole"].toString(),QString("target"));QVERIFY(view->hasFocus());
  const auto xyz=metadata["operations"].toArray()[0].toObject()["interiorPointsMm"].toArray()[0].toArray();QCOMPARE(xyz.size(),3);
  const auto regionPixel=view->projectWorldPoint({xyz[0].toDouble(),xyz[1].toDouble(),xyz[2].toDouble()}).toPoint();
  QCOMPARE(QApplication::widgetAt(view->mapToGlobal(regionPixel)),static_cast<QWidget*>(view));QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,regionPixel);
  QTRY_COMPARE(session(window)["inputs"].toObject()["targetBodyId"],metadata["targetBodyId"]);QTRY_VERIFY(session(window)["previewed"].toBool());
  QCOMPARE(session(window)["partitionBuildCount"].toInteger(),qint64(1));QVERIFY(session(window)["partitionReused"].toBool());QVERIFY(view->hasFocus());
  call(window,"capture.dialog",{{"path",out+"/native-curved-controls.png"}});call(window,"capture.screenshot",{{"path",out+"/native-curved-red-preview.png"}});
  auto* dock=window.findChild<QDockWidget*>("surfaceTrimDock");QVERIFY(dock);auto* buttons=dock->findChild<QDialogButtonBox*>();QVERIFY(buttons && buttons->button(QDialogButtonBox::Ok)->isEnabled());
  QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel),Qt::LeftButton);QTRY_VERIFY(session(window).isEmpty());QCOMPARE(call(window,"document.inspect")["authored"],before["authored"]);
 }
 void planarSheetToolRegionCancel(){
  const auto source=qEnvironmentVariable("ALLOY_SURFACE_SHEET_TRIM_SOURCE"),out=qEnvironmentVariable("ALLOY_SURFACE_TRIM_EVIDENCE");
  if(source.isEmpty())QSKIP("No planar crossing-sheet fixture supplied");
  QDir().mkpath(out);AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1440,900);window.show();window.activateWindow();
  QVERIFY(QTest::qWaitForWindowExposed(&window));
  ReferenceView* view=nullptr;for(auto* child:window.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;
  QVERIFY(view);call(window,"document.open",{{"path",source},{"discard",true}});call(window,"view.home");
  QApplication::setActiveWindow(&window);window.setFocus(); // Initial harness activation only.
  const auto before=call(window,"document.inspect");QString targetId,toolId;
  for(const auto& value:before["evaluation"].toObject()["bodies"].toArray()){
   const auto body=value.toObject();if(body["name"]=="Target sheet")targetId=body["bodyId"].toString();
   if(body["name"]=="Tool sheet")toolId=body["bodyId"].toString();
  }
  QVERIFY(!targetId.isEmpty() && !toolId.isEmpty());
  auto pixel=[&](double x,double y,double z){return view->projectWorldPoint({x,y,z}).toPoint();};
  QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(0,0,5));
  const auto selected=window.state()["selection"].toObject();
  QCOMPARE(selected["bodyId"].toString().isEmpty()?selected["support"].toObject()["bodyId"].toString():selected["bodyId"].toString(),targetId);
  auto* tab=window.findChild<QToolButton*>("design.tab.surface");QVERIFY(tab);QTest::mouseClick(tab,Qt::LeftButton);
  auto* start=window.findChild<QToolButton*>("surface.command.surface.trimDialog");QVERIFY(start);QTest::mouseClick(start,Qt::LeftButton);
  QTRY_COMPARE(session(window)["inputs"].toObject()["targetBodyId"].toString(),targetId);
  auto* dock=window.findChild<QDockWidget*>("surfaceTrimDock");QVERIFY(dock && dock->isVisible());
  QCOMPARE(session(window)["selectionRole"].toString(),QString("tool"));
  QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(0,10,0));
  QTRY_COMPARE(session(window)["inputs"].toObject()["toolBodyId"].toString(),toolId);
  QCOMPARE(session(window)["regions"].toArray().size(),2);QCOMPARE(session(window)["selectionRole"].toString(),QString("region"));
  QVERIFY(view->hasFocus());QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(0,0,5));
  QTRY_VERIFY(session(window)["previewed"].toBool());QCOMPARE(session(window)["inputs"].toObject()["removedRegionIds"].toArray().size(),1);
  QCOMPARE(session(window)["partitionBuildCount"].toInteger(),qint64(1));QVERIFY(session(window)["partitionReused"].toBool());
  const auto sessionId=session(window)["id"].toString();
  call(window,"command.preview",{{"session",sessionId}});
  call(window,"command.update",{{"session",sessionId},{"inputs",QJsonObject{{"name","Named Trim preview"}}}});
  QCOMPARE(session(window)["partitionBuildCount"].toInteger(),qint64(1));QVERIFY(session(window)["partitionReused"].toBool());
  const auto region=session(window)["inputs"].toObject()["removedRegionIds"].toArray()[0].toString();
  call(window,"surface.trimRegion",{{"regionId",region}});
  QVERIFY(!session(window)["previewed"].toBool());
  QCOMPARE(session(window)["previewError"].toObject()["code"].toString(),QString("trim_region_required"));
  QCOMPARE(session(window)["partitionBuildCount"].toInteger(),qint64(1));
  call(window,"surface.trimRegion",{{"regionId",region}});
  call(window,"command.update",{{"session",sessionId},{"inputs",QJsonObject{{"throughMarginMm",11.0}}}});
  QCOMPARE(session(window)["partitionBuildCount"].toInteger(),qint64(2));QVERIFY(!session(window)["partitionReused"].toBool());
  call(window,"capture.screenshot",{{"path",out+"/native-planar-sheet-red-preview.png"}});
  auto* buttons=dock->findChild<QDialogButtonBox*>();QVERIFY(buttons && buttons->button(QDialogButtonBox::Ok)->isEnabled());
  QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel),Qt::LeftButton);QTRY_VERIFY(session(window).isEmpty());
  QCOMPARE(call(window,"document.inspect")["authored"],before["authored"]);
  // A replacement session must build its own partition. A document revision
  // change invalidates it before another region can use its old identity.
  const auto replacement=call(window,"surface.trimDialog",{{"targetBodyId",targetId},{"toolBodyId",toolId}});
  QCOMPARE(replacement["partitionBuildCount"].toInteger(),qint64(1));QVERIFY(!replacement["partitionReused"].toBool());
  const auto targetChanged=window.execute("command.update",{{"session",replacement["id"]},
      {"inputs",QJsonObject{{"targetBodyId",toolId},{"toolBodyId",targetId}}}});
  QVERIFY(!targetChanged["ok"].toBool());
  QCOMPARE(targetChanged["error"].toObject()["code"].toString(),QString("trim_region_ambiguous"));
  QCOMPARE(session(window)["partitionBuildCount"].toInteger(),qint64(2));
  const auto restored=window.execute("command.update",{{"session",replacement["id"]},
      {"inputs",QJsonObject{{"targetBodyId",targetId},{"toolBodyId",toolId}}}});
  QVERIFY(!restored["ok"].toBool());
  QCOMPARE(restored["error"].toObject()["code"].toString(),QString("trim_region_required"));
  QCOMPARE(session(window)["partitionBuildCount"].toInteger(),qint64(3));
  const auto staleRegion=session(window)["regions"].toArray()[0].toObject()["regionId"].toString();
  const auto feature=before["authored"].toObject()["features"].toArray()[0].toObject();
  call(window,"feature.visibility",{{"featureId",feature["id"]},{"visible",!feature["visible"].toBool()}});
  const auto changed=call(window,"document.inspect").value("authored");
  const auto stale=window.execute("surface.trimRegion",{{"regionId",staleRegion}});
  QVERIFY(!stale["ok"].toBool());QCOMPARE(stale["error"].toObject()["code"].toString(),QString("stale_session"));
  QVERIFY(session(window)["regions"].toArray().isEmpty());
  const auto refused=window.execute("command.commit",{{"session",replacement["id"]}});
  QVERIFY(!refused["ok"].toBool());QCOMPARE(refused["error"].toObject()["code"].toString(),QString("stale_session"));
  call(window,"command.cancel",{{"session",replacement["id"]}});
  QCOMPARE(call(window,"document.inspect")["authored"],changed);
  // Leave the replacement command active when this native window is destroyed.
  // Its final focusChanged signal must not access destroyed session members.
  call(window,"surface.trimDialog",{{"targetBodyId",targetId},{"toolBodyId",toolId}});
  QVERIFY(!session(window).isEmpty());QVERIFY(view->hasFocus());
 }
 void toolFirstRegionPreviewCancel(){
  const auto source=qEnvironmentVariable("ALLOY_SURFACE_TRIM_SOURCE"),out=qEnvironmentVariable("ALLOY_SURFACE_TRIM_EVIDENCE");
  QVERIFY(!source.isEmpty() && !out.isEmpty());QDir().mkpath(out);
  AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1440,900);window.show();window.activateWindow();
  QVERIFY(QTest::qWaitForWindowExposed(&window));
  ReferenceView* view=nullptr;for(auto* child:window.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;
  QVERIFY(view);
  call(window,"document.open",{{"path",source},{"discard",true}});call(window,"view.home");
  QApplication::setActiveWindow(&window);window.setFocus();
  const auto before=call(window,"document.inspect");QString sketchId,otherSketchId,otherEntityId;
  for(const auto& value:before["authored"].toObject()["features"].toArray()){
   const auto feature=value.toObject();
   if(feature["name"]=="Paired handle splines")sketchId=feature["id"].toString();
   if(feature["name"]=="Exterior guide rail"){
    otherSketchId=feature["id"].toString();otherEntityId=feature["parameters"].toObject()["entities"].toArray()[0].toObject()["id"].toString();
   }
  }
  QVERIFY(!sketchId.isEmpty() && !otherSketchId.isEmpty() && !otherEntityId.isEmpty());
  auto* tab=window.findChild<QToolButton*>("design.tab.surface");QVERIFY(tab);QTest::mouseClick(tab,Qt::LeftButton);
  auto* start=window.findChild<QToolButton*>("surface.command.surface.trimDialog");QVERIFY(start && start->isVisible());
  QTest::mouseClick(start,Qt::LeftButton);QTRY_COMPARE(session(window)["command"].toString(),QString("surface.trim"));
  auto* dock=window.findChild<QDockWidget*>("surfaceTrimDock");QVERIFY(dock && dock->isVisible());
  auto pixel=[&](double u,double v){const auto point=call(window,"view.projectSketchPoint",{{"sketchId",sketchId},{"uMm",u},{"vMm",v}});
   return QPoint(qRound(point["x"].toDouble()),qRound(point["y"].toDouble()));};
  QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(0,10));
  QTRY_COMPARE(session(window)["inputs"].toObject()["toolEntityIds"].toArray().size(),1);
  QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(0,0));
  QTRY_COMPARE(session(window)["inputs"].toObject()["toolEntityIds"].toArray().size(),2);
  QCOMPARE(session(window)["selectionRole"].toString(),QString("region"));
  const auto regions=session(window)["regions"].toArray();QCOMPARE(regions.size(),4);
  call(window,"view.orientation",{{"name","front"}});
  QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(0,5));
  QTRY_COMPARE(session(window)["inputs"].toObject()["removedRegionIds"].toArray().size(),1);
  QString other;
  const auto chosen=session(window)["inputs"].toObject()["removedRegionIds"].toArray()[0].toString();
  for(const auto& value:regions)if(value.toObject()["inside"].toBool() && value.toObject()["regionId"]!=chosen)other=value.toObject()["regionId"].toString();
  QVERIFY(!other.isEmpty());call(window,"surface.trimRegion",{{"regionId",other}});
  QTRY_VERIFY(session(window)["previewed"].toBool());
  QCOMPARE(session(window)["partitionBuildCount"].toInteger(),qint64(1));QVERIFY(session(window)["partitionReused"].toBool());
  const auto reversed=session(window)["inputs"].toObject()["toolEntityIds"].toArray();
  call(window,"command.update",{{"session",session(window)["id"]},
      {"inputs",QJsonObject{{"toolEntityIds",QJsonArray{reversed[1],reversed[0]}}}}});
  QCOMPARE(session(window)["partitionBuildCount"].toInteger(),qint64(2));QVERIFY(!session(window)["partitionReused"].toBool());
  auto* buttons=dock->findChild<QDialogButtonBox*>();QVERIFY(buttons && buttons->button(QDialogButtonBox::Ok)->isEnabled());
  call(window,"view.orientation",{{"name","isometric"}});
  call(window,"capture.screenshot",{{"path",out+"/native-trim-red-preview.png"}});
  auto* change=dock->findChild<QPushButton*>();QVERIFY(change && change->text()=="Change Tools");
  QTest::mouseClick(change,Qt::LeftButton);
  QVERIFY(session(window)["inputs"].toObject()["toolSketchId"].toString().isEmpty());
  QCOMPARE(session(window)["selectionRole"].toString(),QString("tool"));
  call(window,"surface.trimEntity",{{"sketchId",otherSketchId},{"entityId",otherEntityId}});
  QCOMPARE(session(window)["inputs"].toObject()["toolSketchId"].toString(),otherSketchId);
  QVERIFY(view->hasFocus());
  QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel),Qt::LeftButton);
  QTRY_VERIFY(session(window).isEmpty());
  QCOMPARE(call(window,"document.inspect")["authored"],before["authored"]);
 }
};
QTEST_MAIN(SurfaceTrimControlsTest)
#include "surface_trim_controls.moc"
