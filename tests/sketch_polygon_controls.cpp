#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QJsonDocument>
#include <QLineEdit>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QTest>
#include <cmath>
class PolygonControlsTest final:public QObject {
 Q_OBJECT
 QJsonObject call(AlloyWindow& w,const QString& cmd,const QJsonObject& a={}){auto r=w.execute(cmd,a);if(!r["ok"].toBool())qFatal("%s",QJsonDocument(r).toJson().constData());return r["result"].toObject();}
 QJsonObject session(AlloyWindow& w){return w.state()["activeSession"].toObject();}
 QJsonArray groups(AlloyWindow& w){return session(w)["draftSketch"].toObject()["parameters"].toObject()["polygonGroups"].toArray();}
private slots:
 void nativeCreationAndEdit(){
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1500,950);w.show();w.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&w));QApplication::setActiveWindow(&w);
  ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;QVERIFY(view);view->setFocus();
  call(w,"sketch.begin",{{"plane","XY"}});call(w,"view.zoom",{{"factor",0.45}});
  auto pixel=[&](double u,double v){auto p=call(w,"view.projectSketchPoint",{{"sketchId",session(w)["draftSketch"].toObject()["id"]},{"uMm",u},{"vMm",v}});return QPoint(qRound(p["x"].toDouble()),qRound(p["y"].toDouble()));};
  auto click=[&](double u,double v){QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(u,v));};
  auto move=[&](double u,double v){auto p=pixel(u,v);QMouseEvent event(QEvent::MouseMove,QPointF(p),QPointF(view->mapToGlobal(p)),Qt::NoButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(view,&event);};
  auto* action=w.findChild<QAction*>("sketch.tool.inscribedPolygon");QVERIFY(action);action->trigger();QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
  click(0,0);QVERIFY(session(w).contains("firstPoint"));move(20,0);QVERIFY(groups(w).isEmpty());
  auto* count=w.findChild<QLineEdit*>("sketch.polygon.count");auto* size=w.findChild<QLineEdit*>("sketch.polygon.size");auto* angle=w.findChild<QLineEdit*>("sketch.polygon.angle");QVERIFY(count && size && angle && count->isVisible());
  const auto beforeInvalid=session(w)["draftSketch"].toObject();const auto acceptedInput=session(w).value("polygonInput");
  QTest::mouseClick(count,Qt::LeftButton);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(count));QTest::keyClick(count,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(count,"2");click(30,10);
  QCOMPARE(session(w)["draftSketch"].toObject(),beforeInvalid);QCOMPARE(session(w).value("polygonInput"),acceptedInput);QVERIFY(session(w).contains("polygonInputError"));QCOMPARE(count->text(),QString("2"));QVERIFY(!w.findChild<QLabel*>("sketch.error")->text().isEmpty());
  QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QTest::keyClick(view,Qt::Key_Return);QCOMPARE(session(w)["tool"].toString(),QString("inscribedPolygon"));QVERIFY(session(w).contains("firstPoint"));
  QTest::mouseClick(count,Qt::LeftButton);QTest::keyClick(count,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(count,"8");QTest::keyClick(count,Qt::Key_Tab);
  QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(size));QTest::keyClicks(size,"0");click(30,10);QCOMPARE(session(w)["draftSketch"].toObject(),beforeInvalid);QVERIFY(session(w).contains("polygonInputError"));QCOMPARE(size->text(),QString("0"));QVERIFY(w.findChild<QLabel*>("sketch.error")->text().contains("greater than zero"));
  QTest::mouseClick(size,Qt::LeftButton);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(size));QTest::keyClick(size,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(size,"20");QVERIFY(!session(w).contains("polygonInputError"));QTest::keyClick(size,Qt::Key_Tab);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(angle));QTest::keyClicks(angle,"15");QCOMPARE(angle->text(),QString("15"));QCOMPARE(session(w)["polygonInput"].toObject()["angleDeg"].toDouble(),15.0);click(30,10);
  QCOMPARE(groups(w).size(),1);auto first=groups(w)[0].toObject();QCOMPARE(first["count"].toInt(),8);QCOMPARE(first["sizeMm"].toDouble(),20.0);QCOMPARE(first["angleDeg"].toDouble(),15.0);
  QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QTest::keyClick(view,Qt::Key_Return);QCOMPARE(session(w)["tool"].toString(),QString("select"));
  auto* mode=w.findChild<QComboBox*>("sketch.polygon.mode");QVERIFY(mode && mode->isVisible());QCOMPARE(mode->currentIndex(),-1);QCOMPARE(mode->placeholderText(),QString("Choose Polygon mode"));
  // Native retained edit fields use the same authoritative update command.
  call(w,"sketch.polygonEditSelect",{{"polygonId",first["id"]}});QCOMPARE(mode->currentText(),QString("Inscribed Polygon"));QTest::mouseClick(size,Qt::LeftButton);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(size));QTest::keyClick(size,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(size,"24");
  auto* apply=w.findChild<QPushButton*>("sketch.polygon.apply");QVERIFY(apply && apply->isVisible());QTest::mouseClick(apply,Qt::LeftButton);QCOMPARE(groups(w)[0].toObject()["sizeMm"].toDouble(),24.0);QCOMPARE(groups(w)[0].toObject()["pointIds"],first["pointIds"]);
  action=w.findChild<QAction*>("sketch.tool.edgePolygon");QVERIFY(action);action->trigger();QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));click(45,0);click(55,0);QVERIFY(session(w).contains("secondPoint"));QVERIFY(size->isReadOnly());const auto fixedEdge=session(w)["secondPoint"].toObject();
  QTest::mouseClick(count,Qt::LeftButton);QTest::keyClick(count,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(count,"7");QCOMPARE(session(w)["secondPoint"].toObject(),fixedEdge);
  move(50,-20);click(50,-20);QCOMPARE(groups(w).size(),2);QCOMPARE(groups(w)[1].toObject()["side"].toInt(),-1);QCOMPARE(groups(w)[1].toObject()["count"].toInt(),7);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QTest::keyClick(view,Qt::Key_Escape);QCOMPARE(session(w)["tool"].toString(),QString("select"));
  action=w.findChild<QAction*>("sketch.tool.circumscribedPolygon");QVERIFY(action);action->trigger();click(-50,0);move(-35,0);QTest::keyClick(view,Qt::Key_Escape);QCOMPARE(groups(w).size(),2);QVERIFY(!session(w).contains("firstPoint"));
  action->trigger();click(-50,0);click(-35,0);QCOMPARE(groups(w).size(),3);QCOMPARE(groups(w)[2].toObject()["mode"].toString(),QString("circumscribed"));QTest::keyClick(view,Qt::Key_Return);
  action=w.findChild<QAction*>("sketch.tool.edgePolygon");action->trigger();click(45,45);
  QTest::mouseClick(size,Qt::LeftButton);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(size));QTest::keyClicks(size,"12");QTest::keyClick(size,Qt::Key_Tab);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(angle));QTest::keyClicks(angle,"30");QTest::keyClick(angle,Qt::Key_Return);QVERIFY(session(w).contains("secondPoint"));click(40,60);QCOMPARE(groups(w).size(),4);QVERIFY(std::abs(groups(w)[3].toObject()["sizeMm"].toDouble()-12)<1e-9);
  call(w,"sketch.cancel");w.close();
 }
};
QTEST_MAIN(PolygonControlsTest)
#include "sketch_polygon_controls.moc"
