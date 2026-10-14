// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QTest>
#include <QComboBox>
#include <QPushButton>
#include <QDockWidget>
#include <QDialogButtonBox>
#include <QJsonDocument>
#include <QDir>

class SurfaceRailLoftControlsTest final:public QObject {
 Q_OBJECT
 QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& args={}){
  const auto response=window.execute(command,args);
  if(!response["ok"].toBool())qFatal("%s",QJsonDocument(response).toJson().constData());
  return response["result"].toObject();
 }
 QJsonObject session(AlloyWindow& window){return window.state()["activeSession"].toObject();}
private slots:
 void selectRailAndCancel(){
  const auto source=qEnvironmentVariable("ALLOY_RAIL_LOFT_SOURCE"),out=qEnvironmentVariable("ALLOY_RAIL_LOFT_EVIDENCE");
  QVERIFY(!source.isEmpty() && !out.isEmpty());QDir().mkpath(out);
  AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1440,900);window.show();window.activateWindow();
  QVERIFY(QTest::qWaitForWindowExposed(&window));
  ReferenceView* view=nullptr;for(auto* child:window.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;
  QVERIFY(view);
  call(window,"document.open",{{"path",source},{"discard",true}});call(window,"view.home");
  const auto before=call(window,"document.inspect");
  const auto features=before["authored"].toObject()["features"].toArray();
  QString back,lower,rail;
  for(const auto& value:features){const auto feature=value.toObject();
   if(feature["name"]=="Back open arc")back=feature["id"].toString();
   if(feature["name"]=="Mirrored lower chain")lower=feature["id"].toString();
   if(feature["name"]=="Exterior guide rail")rail=feature["id"].toString();}
  QVERIFY(!back.isEmpty() && !lower.isEmpty() && !rail.isEmpty());
  auto pixel=[&](const QString& sketch,double u,double v){const auto point=call(window,"view.projectSketchPoint",{{"sketchId",sketch},{"uMm",u},{"vMm",v}});
   return QPoint(qRound(point["x"].toDouble()),qRound(point["y"].toDouble()));};
  call(window,"surface.loftDialog",{{"componentName","Iron upper"}});
  auto* dock=window.findChild<QDockWidget*>("loftDock");QVERIFY(dock && dock->isVisible());
  auto* guide=dock->findChild<QComboBox*>("loftGuideType");QVERIFY(guide);
  QCOMPARE(guide->currentData().toString(),QString("centerline"));
  QTest::mouseClick(guide,Qt::LeftButton);QTest::keyClick(guide,Qt::Key_Down);QTest::keyClick(guide,Qt::Key_Return);
  QTRY_COMPARE(session(window)["inputs"].toObject()["guideType"].toString(),QString("rail"));
  QTRY_VERIFY(window.findChild<QPushButton*>("loft.rail")!=nullptr);
  dock=window.findChild<QDockWidget*>("loftDock");auto* first=dock->findChild<QPushButton*>("loft.section1");QVERIFY(first);
  QTest::mouseClick(first,Qt::LeftButton);
  QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(back,0,10));
  QTRY_COMPARE(session(window)["inputs"].toObject()["sections"].toArray()[0].toObject()["entityIds"].toArray().size(),1);
  QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(lower,-12.5,-2.5));
  QTRY_COMPARE(session(window)["inputs"].toObject()["sections"].toArray()[1].toObject()["entityIds"].toArray().size(),1);
  QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(lower,12.5,-2.5));
  QTRY_COMPARE(session(window)["inputs"].toObject()["sections"].toArray()[1].toObject()["entityIds"].toArray().size(),2);
  QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(rail,-31,40));
  QTRY_COMPARE(session(window)["inputs"].toObject()["rail"].toObject()["sketchId"].toString(),rail);
  QTRY_VERIFY(session(window)["previewed"].toBool());
  QCOMPARE(session(window)["geometry"].toObject()["topology"].toObject()["faces"].toInt(),2);
  dock=window.findChild<QDockWidget*>("loftDock");auto* buttons=dock->findChild<QDialogButtonBox*>();QVERIFY(buttons);
  QVERIFY(buttons->button(QDialogButtonBox::Ok)->isEnabled());
  call(window,"capture.screenshot",{{"path",out+"/native-rail-preview.png"}});
  QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel),Qt::LeftButton);
  QTRY_VERIFY(session(window).isEmpty());
  QCOMPARE(call(window,"document.inspect")["authored"],before["authored"]);
 }
};
QTEST_MAIN(SurfaceRailLoftControlsTest)
#include "surface_rail_loft_controls.moc"
