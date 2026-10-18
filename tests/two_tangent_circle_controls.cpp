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
class TwoTangentCircleControlsTest final:public QObject {
 Q_OBJECT
 QJsonObject call(AlloyWindow& w,const QString& c,const QJsonObject& a={}){const auto r=w.execute(c,a);if(!r["ok"].toBool())qFatal("%s",QJsonDocument(r).toJson().constData());return r["result"].toObject();}
 QJsonObject session(AlloyWindow& w){return w.state()["activeSession"].toObject();}
 QJsonObject draft(AlloyWindow& w){return session(w)["draftSketch"].toObject();}
 int circles(AlloyWindow& w){int count=0;for(const auto& e:draft(w)["parameters"].toObject()["entities"].toArray())count+=e.toObject()["type"]=="circle";return count;}
private slots:
 void nativeSnapsPreviewRepeatRefusalAndExit(){
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1500,950);w.show();w.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&w));QApplication::setActiveWindow(&w);
  ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;QVERIFY(view);view->setFocus();
  call(w,"sketch.begin",{{"plane","XY"}});call(w,"view.zoom",{{"factor",0.45}});
  call(w,"sketch.line",{{"x1",-40},{"y1",0},{"x2",40},{"y2",0},{"units","mm"}});
  call(w,"sketch.line",{{"x1",40},{"y1",0},{"x2",40},{"y2",60},{"units","mm"}});
  auto pixel=[&](double u,double v){const auto p=call(w,"view.projectSketchPoint",{{"sketchId",draft(w)["id"]},{"uMm",u},{"vMm",v}});return QPoint(qRound(p["x"].toDouble()),qRound(p["y"].toDouble()));};
  auto click=[&](double u,double v){QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(u,v));};
  auto move=[&](double u,double v){const auto p=pixel(u,v);QMouseEvent e(QEvent::MouseMove,QPointF(p),QPointF(view->mapToGlobal(p)),Qt::NoButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(view,&e);};
  const auto output=qEnvironmentVariable("ALLOY_TWO_TANGENT_CIRCLE_EVIDENCE");if(!output.isEmpty())QDir().mkpath(output);auto capture=[&](const QString& name){if(!output.isEmpty()){QTest::qWait(100);call(w,"capture.screenshot",{{"path",output+"/"+name+".png"}});}};
  auto* action=w.findChild<QAction*>("sketch.tool.twoTangentCircle");QVERIFY(action);
  auto* create=w.findChild<QToolButton*>("sketch.menu.create");auto* circleMenu=qobject_cast<QMenu*>(action->parent());QVERIFY(create && circleMenu);bool menuClicked=false;
  QTimer watchdog;watchdog.setSingleShot(true);connect(&watchdog,&QTimer::timeout,&w,[&]{circleMenu->close();create->menu()->close();});watchdog.start(3000);
  QTimer::singleShot(80,&w,[&]{auto* menu=create->menu();QTimer::singleShot(100,&w,[&]{if(circleMenu->isVisible()){menuClicked=true;QTest::mouseClick(circleMenu,Qt::LeftButton,Qt::NoModifier,circleMenu->actionGeometry(action).center());}});QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,menu->actionGeometry(circleMenu->menuAction()).center());});
  QTest::mouseClick(create,Qt::LeftButton);watchdog.stop();QVERIFY(menuClicked);QTRY_VERIFY(QApplication::activePopupWidget()==nullptr);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
  click(-20,0);QCOMPARE(session(w)["tangentCircleLineIds"].toArray().size(),1);click(40,40);QCOMPARE(session(w)["tangentCircleLineIds"].toArray().size(),2);
  const auto before=draft(w);move(30,10);QVERIFY(session(w)["tangentCirclePreview"].toBool());QCOMPARE(draft(w),before);capture("01-pointer-preview");
  QTest::keyClicks(view,"8");auto* editor=w.findChild<QLineEdit*>("sketch.twoTangentCircle.radius");QVERIFY(editor);QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(editor));QCOMPARE(editor->text(),QString("8"));QCOMPARE(draft(w),before);QTest::keyClick(editor,Qt::Key_Return);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QCOMPARE(circles(w),0);move(30,10);capture("02-typed-radius-preview");click(30,10);QCOMPARE(circles(w),1);QCOMPARE(session(w)["tool"].toString(),QString("twoTangentCircle"));QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
  int relations=0;for(const auto& row:draft(w)["parameters"].toObject()["constraints"].toArray())relations+=row.toObject()["type"]=="tangent";QCOMPARE(relations,2);capture("03-retained-circle");
  click(-20,0);click(40,40);const auto beforeInvalid=draft(w);click(-80,80);QCOMPARE(draft(w),beforeInvalid);move(20,20);QVERIFY(session(w)["tangentCirclePreview"].toBool());QTest::keyClick(view,Qt::Key_Escape);QCOMPARE(session(w)["tool"].toString(),QString("select"));QCOMPARE(draft(w),beforeInvalid);QVERIFY(!session(w).contains("tangentCircleLineIds"));
  action->trigger();QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));click(-20,0);click(40,40);move(20,20);click(20,20);QCOMPARE(circles(w),2);QTest::keyClick(view,Qt::Key_Return);QCOMPARE(session(w)["tool"].toString(),QString("select"));capture("04-repeat-enter");
  call(w,"sketch.cancel");w.close();
 }
};
QTEST_MAIN(TwoTangentCircleControlsTest)
#include "two_tangent_circle_controls.moc"
