// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QAction>
#include <QApplication>
#include <QDir>
#include <QJsonDocument>
#include <QMouseEvent>
#include <QLineEdit>
#include <QMenu>
#include <QToolButton>
#include <QTimer>
#include <QTest>
class ThreeTangentCircleControlsTest final:public QObject {
 Q_OBJECT
 QJsonObject call(AlloyWindow& w,const QString& c,const QJsonObject& a={}){const auto r=w.execute(c,a);if(!r["ok"].toBool())qFatal("%s",QJsonDocument(r).toJson().constData());return r["result"].toObject();}
 QJsonObject session(AlloyWindow& w){return w.state()["activeSession"].toObject();}
 QJsonObject draft(AlloyWindow& w){return session(w)["draftSketch"].toObject();}
 int circles(AlloyWindow& w){int count=0;for(const auto& e:draft(w)["parameters"].toObject()["entities"].toArray())count+=e.toObject()["type"]=="circle";return count;}
private slots:
 void nativeSnapsPreviewRepeatRefusalAndExit(){
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1500,950);w.show();w.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&w));QApplication::setActiveWindow(&w);
  ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;QVERIFY(view);view->setFocus();
  call(w,"sketch.begin",{{"plane","XY"}});
  call(w,"sketch.rectangle",{{"x1",0},{"y1",0},{"x2",30},{"y2",30},{"units","mm"}});
  call(w,"sketch.rectangle",{{"x1",-50},{"y1",0},{"x2",-20},{"y2",30},{"units","mm"}});
  QJsonArray lineIds;for(const auto& row:draft(w)["parameters"].toObject()["entities"].toArray())lineIds.append(row.toObject()["id"]);call(w,"sketch.construction",{{"entityIds",lineIds},{"construction",true}});
  auto pixel=[&](double u,double v){const auto p=call(w,"view.projectSketchPoint",{{"sketchId",draft(w)["id"]},{"uMm",u},{"vMm",v}});return QPoint(qRound(p["x"].toDouble()),qRound(p["y"].toDouble()));};
  auto click=[&](double u,double v){QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(u,v));};
  auto move=[&](double u,double v){const auto p=pixel(u,v);QMouseEvent e(QEvent::MouseMove,QPointF(p),QPointF(view->mapToGlobal(p)),Qt::NoButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(view,&e);};
  const auto output=qEnvironmentVariable("ALLOY_THREE_TANGENT_CIRCLE_EVIDENCE");if(!output.isEmpty())QDir().mkpath(output);auto capture=[&](const QString& name){if(!output.isEmpty()){QTest::qWait(100);call(w,"capture.screenshot",{{"path",output+"/"+name+".png"}});}};
  auto* action=w.findChild<QAction*>("sketch.tool.threeTangentCircle");QVERIFY(action);
  auto* create=w.findChild<QToolButton*>("sketch.menu.create");auto* circleMenu=qobject_cast<QMenu*>(action->parent());QVERIFY(create && circleMenu);bool menuClicked=false;
  QTimer watchdog;watchdog.setSingleShot(true);connect(&watchdog,&QTimer::timeout,&w,[&]{circleMenu->close();create->menu()->close();});watchdog.start(3000);
  QTimer::singleShot(80,&w,[&]{auto* menu=create->menu();QTimer::singleShot(100,&w,[&]{if(circleMenu->isVisible()){menuClicked=true;QTest::mouseClick(circleMenu,Qt::LeftButton,Qt::NoModifier,circleMenu->actionGeometry(action).center());}});QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,menu->actionGeometry(circleMenu->menuAction()).center());});
  QTest::mouseClick(create,Qt::LeftButton);watchdog.stop();QVERIFY(menuClicked);QTRY_VERIFY(QApplication::activePopupWidget()==nullptr);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
  click(10,0);QCOMPARE(session(w)["threeTangentCircleLineIds"].toArray().size(),1);click(30,20);QCOMPARE(session(w)["threeTangentCircleLineIds"].toArray().size(),2);
  const auto before=draft(w);move(10,30);QVERIFY(session(w)["threeTangentCirclePreview"].toBool());QCOMPARE(draft(w),before);QCOMPARE(circles(w),0);capture("01-third-line-preview");
  click(10,30);QCOMPARE(circles(w),1);QCOMPARE(session(w)["tool"].toString(),QString("threeTangentCircle"));QVERIFY(!session(w).contains("threeTangentCircleLineIds"));QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QVERIFY(draft(w)["parameters"].toObject()["dimensions"].toArray().isEmpty());
  int relations=0;for(const auto& row:draft(w)["parameters"].toObject()["constraints"].toArray())relations+=row.toObject()["type"]=="tangent";QCOMPARE(relations,3);capture("02-retained-three-tangent-circle");
  click(-40,0);click(-20,20);const auto beforeInvalid=draft(w);move(25.6,25.6);QVERIFY(!session(w)["threeTangentCirclePreview"].toBool());click(25.6,25.6);QCOMPARE(draft(w),beforeInvalid);QCOMPARE(session(w)["threeTangentCircleLineIds"].toArray().size(),2);move(-40,30);QVERIFY(session(w)["threeTangentCirclePreview"].toBool());QTest::keyClick(view,Qt::Key_Escape);QCOMPARE(session(w)["tool"].toString(),QString("select"));QCOMPARE(draft(w),beforeInvalid);QVERIFY(!session(w).contains("threeTangentCircleLineIds"));capture("03-escape-preserves-circle");
  action->trigger();QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));click(-40,0);click(-20,20);move(-40,30);click(-40,30);QCOMPARE(circles(w),2);QTest::keyClick(view,Qt::Key_Return);QCOMPARE(session(w)["tool"].toString(),QString("select"));capture("04-repeat-enter");
  call(w,"sketch.cancel");w.close();
 }
};
QTEST_MAIN(ThreeTangentCircleControlsTest)
#include "three_tangent_circle_controls.moc"
