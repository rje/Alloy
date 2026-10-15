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

class SurfaceTrimControlsTest final:public QObject {
 Q_OBJECT
 QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& args={}){
  const auto response=window.execute(command,args);
  if(!response["ok"].toBool())qFatal("%s",QJsonDocument(response).toJson().constData());
  return response["result"].toObject();
 }
 QJsonObject session(AlloyWindow& window){return window.state()["activeSession"].toObject();}
private slots:
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
