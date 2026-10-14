// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QTest>
#include <QToolButton>
#include <QDoubleSpinBox>
#include <QDockWidget>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QJsonDocument>
#include <QDir>

class SurfaceExtrudeControlsTest final:public QObject {
 Q_OBJECT
 QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& arguments={}){
  const auto result=window.execute(command,arguments);
  if(!result["ok"].toBool())qFatal("%s",QJsonDocument(result).toJson().constData());
  return result["result"].toObject();
 }
 QJsonObject session(AlloyWindow& window){return window.state()["activeSession"].toObject();}
private slots:
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
  auto* start=window.findChild<QToolButton*>("command.surface.extrudeDialog");QVERIFY(start);
  QTest::mouseClick(start,Qt::LeftButton);
  QTRY_COMPARE(session(window)["command"].toString(),QString("surface.extrude"));
  auto* dock=window.findChild<QDockWidget*>("surfaceExtrudeDock");QVERIFY(dock && dock->isVisible());
  const auto mapped=call(window,"view.projectSketchPoint",{{"sketchId",sketchId},{"uMm",25},{"vMm",7}});
  QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,QPoint(qRound(mapped["x"].toDouble()),qRound(mapped["y"].toDouble())));
  QTRY_COMPARE(session(window)["inputs"].toObject()["entityIds"].toArray().size(),1);
  auto* side1=dock->findChild<QDoubleSpinBox*>("sideOneMm");auto* side2=dock->findChild<QDoubleSpinBox*>("sideTwoMm");
  QVERIFY(side1 && side2);
  QTest::mouseClick(side1,Qt::LeftButton);QTest::keyClick(side1,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(side1,"25");QTest::keyClick(side1,Qt::Key_Tab);
  QTest::mouseClick(side2,Qt::LeftButton);QTest::keyClick(side2,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(side2,"30");QTest::keyClick(side2,Qt::Key_Tab);
  QTRY_VERIFY(session(window)["previewed"].toBool());
  const auto inputs=session(window)["inputs"].toObject();
  QCOMPARE(inputs["sideOneMm"].toDouble(),25.0);QCOMPARE(inputs["sideTwoMm"].toDouble(),30.0);
  QCOMPARE(session(window)["geometry"].toObject()["topology"].toObject()["faces"].toInt(),1);
  auto* buttons=dock->findChild<QDialogButtonBox*>();QVERIFY(buttons && buttons->button(QDialogButtonBox::Ok)->isEnabled());
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
