// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QTest>
#include <QComboBox>
#include <QPushButton>
#include <QToolButton>
#include <QDockWidget>
#include <QDialogButtonBox>
#include <QJsonDocument>
#include <QDir>
#include <QMap>

class SurfaceHandleBridgeControlsTest final:public QObject {
 Q_OBJECT
 QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& args={}){
  const auto response=window.execute(command,args);if(!response["ok"].toBool())qFatal("%s",QJsonDocument(response).toJson().constData());return response["result"].toObject();
 }
 QJsonObject session(AlloyWindow& window){return window.state()["activeSession"].toObject();}
private slots:
 void actualBoundaryChainsEllipseAndNewBody(){
  const auto source=qEnvironmentVariable("ALLOY_HANDLE_BRIDGE_SOURCE"),out=qEnvironmentVariable("ALLOY_HANDLE_BRIDGE_EVIDENCE");
  QVERIFY(!source.isEmpty() && !out.isEmpty());QDir().mkpath(out);
  AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1440,900);window.show();window.activateWindow();
  QVERIFY(QTest::qWaitForWindowExposed(&window));ReferenceView* view=nullptr;
  for(auto* child:window.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;QVERIFY(view);
  call(window,"document.open",{{"path",source},{"discard",true}});call(window,"view.home");
  const auto before=call(window,"document.inspect");const auto features=before["authored"].toObject()["features"].toArray();
  QString middle;for(const auto& value:features)if(value.toObject()["name"]=="Handle middle ellipse")middle=value.toObject()["id"].toString();QVERIFY(!middle.isEmpty());
  const auto target=before["evaluation"].toObject()["bodies"].toArray().first().toObject();const auto bodyId=target["bodyId"].toString();
  QStringList selected;for(const auto& faceValue:target["supportFaces"].toArray()){
   const auto face=faceValue.toObject();QMap<QString,QString> edges;
   for(const auto& value:target["solidEdges"].toArray()){const auto edge=value.toObject(),p=edge["provenance"].toObject();
    if(p["role"]=="trimBoundary" && p["sourceFaceId"]==face["provenance"].toObject()["sourceFaceId"])edges[p["sourceEntityId"].toString()]=edge["edgeId"].toString();}
   QCOMPARE(edges.size(),2);selected.append(edges.first());
  }QCOMPARE(selected.size(),2);
  auto edgePixel=[&](int side){const auto p=call(window,"view.projectEdge",{{"bodyId",bodyId},{"edgeId",selected[side]}});return QPoint(qRound(p["x"].toDouble()),qRound(p["y"].toDouble()));};
  auto ellipsePixel=[&]{const auto p=call(window,"view.projectSketchPoint",{{"sketchId",middle},{"uMm",60},{"vMm",27}});return QPoint(qRound(p["x"].toDouble()),qRound(p["y"].toDouble()));};
  auto start=[&]{
   auto* tab=window.findChild<QToolButton*>("design.tab.surface");QVERIFY(tab);QTest::mouseClick(tab,Qt::LeftButton);
   auto* button=window.findChild<QToolButton*>("surface.command.surface.loftDialog");QVERIFY(button && button->isVisible());QTest::mouseClick(button,Qt::LeftButton);
   auto* dock=window.findChild<QDockWidget*>("loftDock");QVERIFY(dock && dock->isVisible());auto* guide=dock->findChild<QComboBox*>("loftGuideType");QVERIFY(guide);
   QTest::mouseClick(guide,Qt::LeftButton);QTest::keyClick(guide,Qt::Key_End);QTest::keyClick(guide,Qt::Key_Return);
   QTRY_COMPARE(session(window)["inputs"].toObject()["guideType"].toString(),QString("none"));
   QTRY_VERIFY(QApplication::activePopupWidget()==nullptr);QTest::qWait(50);
   dock=window.findChild<QDockWidget*>("loftDock");auto* operation=dock->findChild<QComboBox*>("loftOperation");QVERIFY(operation && operation->isEnabled());
   QTest::mouseClick(operation,Qt::LeftButton);QTest::keyClick(operation,Qt::Key_Home);QTest::keyClick(operation,Qt::Key_Return);
   QTRY_COMPARE(session(window)["inputs"].toObject()["operation"].toString(),QString("newBody"));
   QTRY_VERIFY(QApplication::activePopupWidget()==nullptr);QTest::qWait(50);
   call(window,"view.orientation",{{"name","isometric"}});call(window,"view.fit");
   QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,edgePixel(0));
   QTRY_COMPARE(session(window)["inputs"].toObject()["sections"].toArray()[0].toObject()["edgeIds"].toArray().size(),2);
   QCOMPARE(session(window)["selectionRole"].toString(),QString("section2"));
   call(window,"view.orientation",{{"name","front"}});
   QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,ellipsePixel());
   QTRY_COMPARE(session(window)["inputs"].toObject()["sections"].toArray()[1].toObject()["entityIds"].toArray().size(),1);
   QCOMPARE(session(window)["selectionRole"].toString(),QString("section3"));
   call(window,"view.orientation",{{"name","rearIsometric"}});call(window,"view.fit");
   QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,edgePixel(1));
   QTRY_COMPARE(session(window)["inputs"].toObject()["sections"].toArray()[2].toObject()["edgeIds"].toArray().size(),2);
   QTRY_VERIFY(session(window)["previewed"].toBool());
  };
  start();auto* dock=window.findChild<QDockWidget*>("loftDock");QVERIFY(dock);auto* buttons=dock->findChild<QDialogButtonBox*>();QVERIFY(buttons && buttons->button(QDialogButtonBox::Ok)->isEnabled());
  call(window,"capture.screenshot",{{"path",out+"/native-bridge-preview.png"}});
  QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel),Qt::LeftButton);QTRY_VERIFY(session(window).isEmpty());QCOMPARE(call(window,"document.inspect")["authored"],before["authored"]);
  start();dock=window.findChild<QDockWidget*>("loftDock");buttons=dock->findChild<QDialogButtonBox*>();QVERIFY(buttons && buttons->button(QDialogButtonBox::Ok)->isEnabled());
  QTest::mouseClick(buttons->button(QDialogButtonBox::Ok),Qt::LeftButton);QTRY_VERIFY(session(window).isEmpty());QTest::qWait(100);
  const auto after=call(window,"document.inspect");QCOMPARE(after["evaluation"].toObject()["bodies"].toArray().size(),2);
  const auto bridge=after["evaluation"].toObject()["bodies"].toArray().last().toObject();QCOMPARE(bridge["type"].toString(),QString("surface"));QCOMPARE(bridge["topology"].toObject()["faces"].toInt(),2);
  call(window,"view.home");call(window,"capture.screenshot",{{"path",out+"/native-bridge-committed.png"}});call(window,"document.save",{{"path",out+"/native-handle-bridge.alloy"}});
 }
};
QTEST_MAIN(SurfaceHandleBridgeControlsTest)
#include "surface_handle_bridge_controls.moc"
