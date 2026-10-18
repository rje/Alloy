#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QAction>
#include <QApplication>
#include <QJsonDocument>
#include <QMouseEvent>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTest>
#include <cmath>
class PolygonDriverControlsTest final:public QObject {
 Q_OBJECT
 QJsonObject call(AlloyWindow& w,const QString& name,const QJsonObject& args={}){const auto r=w.execute(name,args);if(!r["ok"].toBool())qFatal("%s",QJsonDocument(r).toJson().constData());return r["result"].toObject();}
 QJsonObject active(AlloyWindow& w){return w.state()["activeSession"].toObject();}
 QJsonObject parameters(AlloyWindow& w){return active(w)["draftSketch"].toObject()["parameters"].toObject();}
private slots:
 void nativeSecondDrivers(){
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1500,950);w.show();w.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&w));QApplication::setActiveWindow(&w);
  ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(child))view=v;QVERIFY(view);view->setFocus();
  call(w,"sketch.begin",{{"plane","XY"}});call(w,"view.zoom",{{"factor",0.45}});
  call(w,"sketch.addPoint",{{"x",-25},{"y",30}});const auto radialPoint=parameters(w)["entities"].toArray().last().toObject().value("pointId");
  call(w,"sketch.addPoint",{{"x",-25},{"y",-30}});const auto basePoint=parameters(w)["entities"].toArray().last().toObject().value("pointId");
  call(w,"sketch.line",{{"x1",15},{"y1",15},{"x2",35},{"y2",15}});const auto sourceLine=parameters(w)["entities"].toArray().last().toObject();call(w,"sketch.construction",{{"entityIds",QJsonArray{sourceLine["id"]}},{"construction",true}});
  auto pixel=[&](double u,double v){const auto p=call(w,"view.projectSketchPoint",{{"sketchId",active(w)["draftSketch"].toObject()["id"]},{"uMm",u},{"vMm",v}});return QPoint(qRound(p["x"].toDouble()),qRound(p["y"].toDouble()));};
  auto move=[&](double u,double v){const auto p=pixel(u,v);QMouseEvent e(QEvent::MouseMove,QPointF(p),QPointF(view->mapToGlobal(p)),Qt::NoButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(view,&e);};
  auto click=[&](double u,double v){QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(u,v));};
  auto tool=[&](const char* mode){auto* action=w.findChild<QAction*>(QString("sketch.tool.")+mode);if(!action)qFatal("missing polygon action");action->trigger();};
  tool("inscribedPolygon");QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));click(-40,30);move(-25,30);
  QCOMPARE(active(w)["snap"].toObject()["target"].toObject()["pointId"],radialPoint);QVERIFY(view->diagnostics()["sketchSnapCue"].toObject()["visible"].toBool());click(-25,30);QTest::keyClick(view,Qt::Key_Return);
  auto groups=parameters(w)["polygonGroups"].toArray();QCOMPARE(groups.size(),1);QCOMPARE(groups[0].toObject()["driverSource"],QJsonObject({{"kind","point"},{"pointId",radialPoint}}));
  tool("circumscribedPolygon");click(25,0);move(25,15);QCOMPARE(active(w)["snap"].toObject()["target"].toObject()["kind"].toString(),QString("midpoint"));click(25,15);QTest::keyClick(view,Qt::Key_Return);
  groups=parameters(w)["polygonGroups"].toArray();QCOMPARE(groups.size(),2);QCOMPARE(groups[1].toObject()["driverSource"],QJsonObject({{"kind","midpoint"},{"entityId",sourceLine["id"]}}));
  tool("edgePolygon");click(-40,-30);move(-25,-30);QCOMPARE(active(w)["snap"].toObject()["target"].toObject()["pointId"],basePoint);click(-25,-30);QVERIFY(active(w)["secondPoint"].toObject().contains("target"));click(-33,-20);QTest::keyClick(view,Qt::Key_Return);
  groups=parameters(w)["polygonGroups"].toArray();QCOMPARE(groups.size(),3);QCOMPARE(groups[2].toObject()["driverSource"],QJsonObject({{"kind","point"},{"pointId",basePoint}}));QCOMPARE(active(w)["profileSummaries"].toArray().size(),3);
  const auto annotations=view->diagnostics()["sketchAnnotations"].toArray();QCOMPARE(annotations.size(),3);for(const auto& row:annotations){const auto a=row.toObject();QVERIFY(!a["driving"].toBool());QCOMPARE(a["sizeOwner"].toString(),QString("secondDriver"));QVERIFY(a["label"].toObject()["text"].toString().startsWith("("));}
  call(w,"sketch.movePoint",{{"pointId",radialPoint},{"x",-40},{"y",50}});groups=parameters(w)["polygonGroups"].toArray();const auto moved=groups[0].toObject();const double du=-40-moved["uMm"].toDouble(),dv=50-moved["vMm"].toDouble();QVERIFY(std::abs(moved["sizeMm"].toDouble()-std::hypot(du,dv))<1e-5);QVERIFY(std::abs(moved["angleDeg"].toDouble()-std::atan2(dv,du)*180/std::acos(-1))<1e-5);
  call(w,"sketch.addPoint",{{"x",45},{"y",-45}});
  const auto before=active(w)["draftSketch"].toObject();call(w,"sketch.polygonEditSelect",{{"polygonId",groups[0].toObject()["id"]}});
  auto* size=w.findChild<QLineEdit*>("sketch.polygon.size");auto* apply=w.findChild<QPushButton*>("sketch.polygon.apply");QVERIFY(size && apply);QVERIFY(size->toolTip().contains("retained second point"));
  QTest::mouseClick(size,Qt::LeftButton);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(size));QTest::keyClick(size,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(size,"30");QTest::mouseClick(apply,Qt::LeftButton);
  QCOMPARE(active(w)["draftSketch"].toObject(),before);QVERIFY(w.findChild<QLabel*>("sketch.error")->text().contains("edit that source instead"));
  // A numeric lock that disagrees with a target must not show a retained cue.
  tool("inscribedPolygon");call(w,"sketch.polygonInput",{{"count",6},{"size",10}});click(30,-45);move(45,-45);QCOMPARE(active(w)["snap"].toObject()["status"].toString(),QString("none"));QVERIFY(!view->diagnostics()["sketchSnapCue"].toObject()["visible"].toBool());QTest::keyClick(view,Qt::Key_Escape);QCOMPARE(active(w)["draftSketch"].toObject(),before);
  call(w,"sketch.cancel");w.close();
 }
};
QTEST_MAIN(PolygonDriverControlsTest)
#include "polygon_driver_controls.moc"
