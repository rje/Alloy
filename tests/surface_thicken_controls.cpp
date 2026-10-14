// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QTest>
#include <QToolButton>
#include <QPushButton>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QDialogButtonBox>
#include <QJsonDocument>
#include <QDir>

class SurfaceThickenControlsTest final:public QObject {
 Q_OBJECT
 QJsonObject call(AlloyWindow& w,const QString& name,const QJsonObject& a={}){
  const auto r=w.execute(name,a);if(!r["ok"].toBool())qFatal("%s",QJsonDocument(r).toJson().constData());return r["result"].toObject();
 }
 QJsonObject session(AlloyWindow& w){return w.state()["activeSession"].toObject();}
private slots:
 void clickSurfaceAndFlip(){
  const auto fixture=qEnvironmentVariable("ALLOY_SURFACE_THICKEN_SOURCE"),out=qEnvironmentVariable("ALLOY_SURFACE_THICKEN_EVIDENCE");
  QVERIFY(!fixture.isEmpty() && !out.isEmpty());QDir().mkpath(out);
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1440,900);w.show();w.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&w));
  ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(child))view=v;QVERIFY(view);
  call(w,"document.open",{{"path",fixture},{"discard",true}});call(w,"view.home");
  const auto initial=call(w,"document.inspect");const auto body=initial["evaluation"].toObject()["bodies"].toArray()[0].toObject();
  QCOMPARE(body["type"].toString(),QString("surface"));
  auto* start=w.findChild<QToolButton*>("command.solid.thickenDialog");QVERIFY(start);QTest::mouseClick(start,Qt::LeftButton);
  QTRY_COMPARE(session(w)["command"].toString(),QString("solid.thicken"));
  auto* dock=w.findChild<QDockWidget*>("thickenDock");QVERIFY(dock && dock->isVisible());
  auto* face=dock->findChild<QPushButton*>("thickenFace");QVERIFY(face);QTest::mouseClick(face,Qt::LeftButton);
  // Captured interior of this source-sized sheet in the fixed home view.
  QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,QPoint(520,230));
  call(w,"capture.screenshot",{{"path",out+"/native-after-click.png"}});
  QTRY_VERIFY(session(w)["previewed"].toBool());QCOMPARE(session(w)["geometry"].toObject()["type"].toString(),QString("solid"));
  auto* amount=dock->findChild<QDoubleSpinBox*>("thickenThickness");QVERIFY(amount);
  auto* flip=dock->findChild<QPushButton*>("thickenFlip");QVERIFY(flip);QTest::mouseClick(flip,Qt::LeftButton);
  QTRY_COMPARE(session(w)["inputs"].toObject()["signedThicknessMm"].toDouble(),-3.0);
  QTRY_VERIFY(session(w)["previewed"].toBool());
  call(w,"capture.screenshot",{{"path",out+"/native-thicken-flipped.png"}});
  auto* buttons=dock->findChild<QDialogButtonBox*>();QVERIFY(buttons);
  QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel),Qt::LeftButton);
  QTRY_VERIFY(session(w).isEmpty());QCOMPARE(call(w,"document.inspect")["authored"],initial["authored"]);
 }
};
QTEST_MAIN(SurfaceThickenControlsTest)
#include "surface_thicken_controls.moc"
