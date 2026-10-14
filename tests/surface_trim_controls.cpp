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
 void toolFirstRegionPreviewCancel(){
  const auto source=qEnvironmentVariable("ALLOY_SURFACE_TRIM_SOURCE"),out=qEnvironmentVariable("ALLOY_SURFACE_TRIM_EVIDENCE");
  QVERIFY(!source.isEmpty() && !out.isEmpty());QDir().mkpath(out);
  AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1440,900);window.show();window.activateWindow();
  QVERIFY(QTest::qWaitForWindowExposed(&window));
  ReferenceView* view=nullptr;for(auto* child:window.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;
  QVERIFY(view);
  call(window,"document.open",{{"path",source},{"discard",true}});call(window,"view.home");
  QApplication::setActiveWindow(&window);window.setFocus();
  const auto before=call(window,"document.inspect");QString sketchId;
  for(const auto& value:before["authored"].toObject()["features"].toArray())if(value.toObject()["name"]=="Paired handle splines")sketchId=value.toObject()["id"].toString();
  QVERIFY(!sketchId.isEmpty());
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
  auto* buttons=dock->findChild<QDialogButtonBox*>();QVERIFY(buttons && buttons->button(QDialogButtonBox::Ok)->isEnabled());
  call(window,"view.orientation",{{"name","isometric"}});
  call(window,"capture.screenshot",{{"path",out+"/native-trim-red-preview.png"}});
  QVERIFY(view->hasFocus());
  QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel),Qt::LeftButton);
  QTRY_VERIFY(session(window).isEmpty());
  QCOMPARE(call(window,"document.inspect")["authored"],before["authored"]);
 }
};
QTEST_MAIN(SurfaceTrimControlsTest)
#include "surface_trim_controls.moc"
