// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QTest>
#include <QToolButton>
#include <QComboBox>
#include <QAbstractItemView>
#include <QPushButton>
#include <QDockWidget>
#include <QDialogButtonBox>
#include <QJsonDocument>
#include <QFile>
#include <QDir>

class SurfaceLoftControlsTest final:public QObject {
 Q_OBJECT
 QJsonObject call(AlloyWindow& w,const QString& name,const QJsonObject& a={}){const auto r=w.execute(name,a);if(!r["ok"].toBool())qFatal("%s",QJsonDocument(r).toJson().constData());return r["result"].toObject();}
 QJsonObject session(AlloyWindow& w){return w.state()["activeSession"].toObject();}
private slots:
 void nativeOpenSectionsAndGuide(){
  const auto fixture=qEnvironmentVariable("ALLOY_SURFACE_LOFT_SOURCE");QVERIFY(!fixture.isEmpty());
  const auto out=qEnvironmentVariable("ALLOY_SURFACE_LOFT_EVIDENCE");QVERIFY(!out.isEmpty());QDir().mkpath(out);
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1440,900);w.show();w.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&w));
  ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(child))view=v;QVERIFY(view);
  call(w,"document.open",{{"path",fixture},{"discard",true}});call(w,"view.home");
  const auto initial=call(w,"document.inspect");const auto features=initial["authored"].toObject()["features"].toArray();
  QString arc,linen,path;for(const auto& row:features){const auto f=row.toObject();if(f["name"]=="Arc")arc=f["id"].toString();else if(f["name"]=="Line")linen=f["id"].toString();else if(f["name"]=="Path")path=f["id"].toString();}
  QVERIFY(!arc.isEmpty() && !linen.isEmpty() && !path.isEmpty());
  auto pixel=[&](const QString& sketch,double u,double v){const auto q=call(w,"view.projectSketchPoint",{{"sketchId",sketch},{"uMm",u},{"vMm",v}});return QPoint(qRound(q["x"].toDouble()),qRound(q["y"].toDouble()));};
  call(w,"solid.loftDialog",{{"operation","newComponent"},{"componentName","Shoehorn"}});
  QTRY_COMPARE(session(w)["command"].toString(),QString("solid.loft"));
  auto* dock=w.findChild<QDockWidget*>("loftDock");QVERIFY(dock && dock->isVisible());
  auto* operation=dock->findChild<QComboBox*>("loftOperation");QVERIFY(operation);
  QCOMPARE(operation->currentData().toString(),QString("newComponent"));
  auto* section1=dock->findChild<QPushButton*>("loft.section1");QVERIFY(section1);
  QTest::mouseClick(section1,Qt::LeftButton);QTRY_VERIFY(session(w)["selectionRole"]=="section1");
  QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(arc,15,3.244));
  QTRY_COMPARE(session(w)["inputs"].toObject()["sections"].toArray()[0].toObject()["sketchId"].toString(),arc);
  QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(linen,10,0));
  QTRY_COMPARE(session(w)["inputs"].toObject()["sections"].toArray()[1].toObject()["sketchId"].toString(),linen);
  QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(path,50,16));
  QTRY_COMPARE(session(w)["inputs"].toObject()["centerline"].toObject()["sketchId"].toString(),path);
  QTRY_VERIFY(session(w)["previewed"].toBool());QCOMPARE(session(w)["geometry"].toObject()["type"].toString(),QString("surface"));
  auto* buttons=dock->findChild<QDialogButtonBox*>();QVERIFY(buttons && buttons->button(QDialogButtonBox::Ok)->isEnabled());
  call(w,"capture.screenshot",{{"path",out+"/native-guided-preview.png"}});
  QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel),Qt::LeftButton);QTRY_VERIFY(session(w).isEmpty());
  QCOMPARE(call(w,"document.inspect")["authored"],initial["authored"]);
  QVERIFY(view->diagnostics()["commandSketches"].toArray().isEmpty());
  // Check the native control's change handler without repairing keyboard focus.
  auto* start=w.findChild<QToolButton*>("command.solid.loftDialog");QVERIFY(start);
  QTest::mouseClick(start,Qt::LeftButton);QTRY_COMPARE(session(w)["command"].toString(),QString("solid.loft"));
  dock=w.findChild<QDockWidget*>("loftDock");operation=dock->findChild<QComboBox*>("loftOperation");QVERIFY(operation);
  operation->setCurrentIndex(1);QTRY_COMPARE(session(w)["inputs"].toObject()["operation"].toString(),QString("newComponent"));
  QTRY_VERIFY(w.findChild<QPushButton*>("loft.centerline")!=nullptr);
  call(w,"command.cancel",{{"session",session(w)["id"]}});
  QCOMPARE(call(w,"document.inspect")["authored"],initial["authored"]);
 }
};
QTEST_MAIN(SurfaceLoftControlsTest)
#include "surface_loft_controls.moc"
