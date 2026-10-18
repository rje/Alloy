#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QAction>
#include <QApplication>
#include <QJsonDocument>
#include <QMouseEvent>
#include <QTest>
#include <cmath>
class PolygonAnchorControlsTest final:public QObject {
 Q_OBJECT
 QJsonObject call(AlloyWindow& w,const QString& name,const QJsonObject& args={}){const auto r=w.execute(name,args);if(!r["ok"].toBool())qFatal("%s",QJsonDocument(r).toJson().constData());return r["result"].toObject();}
 QJsonObject active(AlloyWindow& w){return w.state()["activeSession"].toObject();}
 QJsonObject parameters(AlloyWindow& w){return active(w)["draftSketch"].toObject()["parameters"].toObject();}
private slots:
 void nativeFirstAnchors(){
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1500,950);w.show();w.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&w));QApplication::setActiveWindow(&w);
  ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(child))view=v;QVERIFY(view);view->setFocus();
  call(w,"sketch.begin",{{"plane","XY"}});call(w,"view.zoom",{{"factor",0.45}});call(w,"sketch.addPoint",{{"x",-45},{"y",20}});const auto sourcePoint=parameters(w)["entities"].toArray().last().toObject().value("pointId");
  call(w,"sketch.line",{{"x1",-10},{"y1",-30},{"x2",10},{"y2",-30}});const auto sourceLine=parameters(w)["entities"].toArray().last().toObject();call(w,"sketch.construction",{{"entityIds",QJsonArray{sourceLine["id"]}},{"construction",true}});
  auto pixel=[&](double u,double v){const auto p=call(w,"view.projectSketchPoint",{{"sketchId",active(w)["draftSketch"].toObject()["id"]},{"uMm",u},{"vMm",v}});return QPoint(qRound(p["x"].toDouble()),qRound(p["y"].toDouble()));};
  auto move=[&](double u,double v){const auto p=pixel(u,v);QMouseEvent e(QEvent::MouseMove,QPointF(p),QPointF(view->mapToGlobal(p)),Qt::NoButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(view,&e);};
  auto click=[&](double u,double v){QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(u,v));};
  auto tool=[&](const char* mode){auto* action=w.findChild<QAction*>(QString("sketch.tool.")+mode);if(!action)qFatal("missing polygon action");action->trigger();};
  tool("inscribedPolygon");QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));move(-45,20);QCOMPARE(active(w)["snap"].toObject()["status"].toString(),QString("acquired"));QCOMPARE(active(w)["snap"].toObject()["target"].toObject()["pointId"],sourcePoint);QVERIFY(view->diagnostics()["sketchSnapCue"].toObject()["visible"].toBool());click(-45,20);move(-35,20);click(-35,20);
  auto groups=parameters(w)["polygonGroups"].toArray();QCOMPARE(groups.size(),1);auto g=groups[0].toObject();QCOMPARE(g["anchorSource"],QJsonObject({{"kind","point"},{"pointId",sourcePoint}}));QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QTest::keyClick(view,Qt::Key_Return);
  call(w,"sketch.movePoint",{{"pointId",sourcePoint},{"x",-30},{"y",20}});groups=parameters(w)["polygonGroups"].toArray();QVERIFY(std::abs(groups[0].toObject()["uMm"].toDouble()+30)<1e-5);QCOMPARE(groups[0].toObject()["entityIds"],g["entityIds"]);
  tool("circumscribedPolygon");move(0,-30);QCOMPARE(active(w)["snap"].toObject()["target"].toObject()["kind"].toString(),QString("midpoint"));QVERIFY(view->diagnostics()["sketchSnapCue"].toObject()["visible"].toBool());click(0,-30);click(0,-20);QTest::keyClick(view,Qt::Key_Return);groups=parameters(w)["polygonGroups"].toArray();QCOMPARE(groups.size(),2);QCOMPARE(groups[1].toObject()["anchorSource"],QJsonObject({{"kind","midpoint"},{"entityId",sourceLine["id"]}}));
  call(w,"sketch.movePoint",{{"pointId",sourceLine["endPointId"]},{"x",20},{"y",-30}});QVERIFY(std::abs(parameters(w)["polygonGroups"].toArray()[1].toObject()["uMm"].toDouble()-5)<1e-5);
  tool("edgePolygon");move(0,0);QCOMPARE(active(w)["snap"].toObject()["target"].toObject()["kind"].toString(),QString("origin"));QVERIFY(view->diagnostics()["sketchSnapCue"].toObject()["visible"].toBool());click(0,0);click(10,0);click(5,8);QTest::keyClick(view,Qt::Key_Return);groups=parameters(w)["polygonGroups"].toArray();QCOMPARE(groups.size(),3);QCOMPARE(groups[2].toObject()["anchorSource"].toObject()["kind"].toString(),QString("origin"));QCOMPARE(active(w)["profileSummaries"].toArray().size(),3);
  call(w,"sketch.addPoint",{{"x",45},{"y",-30}});call(w,"sketch.addPoint",{{"x",45},{"y",-30}});const auto before=active(w)["draftSketch"].toObject();tool("inscribedPolygon");move(45,-30);QCOMPARE(active(w)["snap"].toObject()["status"].toString(),QString("ambiguous"));click(45,-30);QVERIFY(!active(w).contains("firstPoint"));QCOMPARE(active(w)["draftSketch"].toObject(),before);QTest::keyClick(view,Qt::Key_Escape);
  call(w,"sketch.cancel");w.close();
 }
};
QTEST_MAIN(PolygonAnchorControlsTest)
#include "polygon_anchor_controls.moc"
