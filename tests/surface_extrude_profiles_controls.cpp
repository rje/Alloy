// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QTest>
#include <QToolButton>
#include <QPushButton>
#include <QDockWidget>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QJsonDocument>
#include <QDir>
#include <QMenu>
#include <QAction>
#include <QTimer>
class SurfaceExtrudeProfilesControlsTest final:public QObject {
 Q_OBJECT
 QJsonObject call(AlloyWindow& w,const QString& command,const QJsonObject& a={}){const auto r=w.execute(command,a);if(!r["ok"].toBool())qFatal("%s",QJsonDocument(r).toJson().constData());return r["result"].toObject();}
 QJsonObject session(AlloyWindow& w){return w.state()["activeSession"].toObject();}
 void choose(QToolButton* opener,const QString& actionName){
  QVERIFY(opener && opener->isVisible());auto* menu=opener->menu();QVERIFY(menu);bool active=false,hit=false,clicked=false,timeout=false;
  QTimer::singleShot(50,opener,[&]{active=QApplication::activePopupWidget()==menu;QAction* action=nullptr;for(auto* a:menu->actions())if(a->objectName()==actionName)action=a;if(!active || !action || !action->isEnabled())return;const auto p=menu->actionGeometry(action).center();hit=QApplication::widgetAt(menu->mapToGlobal(p))==menu;if(hit){QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,p);clicked=true;}});
  QTimer watchdog;watchdog.setSingleShot(true);connect(&watchdog,&QTimer::timeout,opener,[&]{if(menu->isVisible()){timeout=true;menu->close();}});watchdog.start(2000);QTest::mouseClick(opener,Qt::LeftButton);watchdog.stop();QVERIFY(active);QVERIFY(hit);QVERIFY(clicked);QVERIFY(!timeout);QTRY_VERIFY(QApplication::activePopupWidget()==nullptr);
 }
 void number(QDoubleSpinBox* field,const QString& text){QVERIFY(field && field->isVisible());QTest::mouseClick(field,Qt::LeftButton);QTest::keyClick(field,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(field,text);QTest::keyClick(field,Qt::Key_Tab);}
private slots:
 void actualGroupedProfilesPreviewCancelCommitEdit(){
  const auto source=qEnvironmentVariable("ALLOY_SURFACE_PROFILES_SOURCE"),out=qEnvironmentVariable("ALLOY_SURFACE_PROFILES_EVIDENCE");QVERIFY(!source.isEmpty() && !out.isEmpty());QDir().mkpath(out);
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1440,900);w.show();w.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&w));ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;QVERIFY(view);
  call(w,"document.open",{{"path",source},{"discard",true}});const auto before=call(w,"document.inspect");const auto sketch=before["authored"].toObject()["features"].toArray().last().toObject();QCOMPARE(sketch["name"].toString(),QString("Back and notch sheets"));const auto sketchId=sketch["id"].toString();call(w,"component.activate",{{"definitionId",sketch["ownerId"]}});
  auto pixel=[&](double u,double v){const auto p=call(w,"view.projectSketchPoint",{{"sketchId",sketchId},{"uMm",u},{"vMm",v}});return QPoint(qRound(p["x"].toDouble()),qRound(p["y"].toDouble()));};
  auto start=[&]{
   auto* tab=w.findChild<QToolButton*>("design.tab.surface");QVERIFY(tab);QTest::mouseClick(tab,Qt::LeftButton);choose(w.findChild<QToolButton*>("surface.menu.create"),"surface.action.surface.extrudeDialog");
   auto* dock=w.findChild<QDockWidget*>("surfaceExtrudeDock");QVERIFY(dock && dock->isVisible());auto* chain=dock->findChild<QCheckBox*>("surfaceExtrude.chaining");QVERIFY(chain && chain->isChecked());
   number(dock->findChild<QDoubleSpinBox*>("sideOneMm"),"60");number(dock->findChild<QDoubleSpinBox*>("sideTwoMm"),"65");
   call(w,"view.orientation",{{"name","front"}});call(w,"view.fit");QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(102,38));
   QTRY_COMPARE(session(w)["inputs"].toObject()["profiles"].toArray().size(),1);QCOMPARE(session(w)["inputs"].toObject()["profiles"].toArray()[0].toObject()["entityIds"].toArray().size(),2);
   QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(110,8));QTRY_COMPARE(session(w)["resultBodyIds"].toArray().size(),2);QVERIFY(session(w)["previewed"].toBool());
   QCOMPARE(session(w)["resultBodies"].toArray()[0].toObject()["geometry"].toObject()["topology"].toObject()["faces"].toInt(),2);QCOMPARE(session(w)["resultBodies"].toArray()[1].toObject()["geometry"].toObject()["topology"].toObject()["faces"].toInt(),1);
  };
  start();call(w,"view.home");call(w,"capture.screenshot",{{"path",out+"/native-two-profile-preview.png"}});auto* dock=w.findChild<QDockWidget*>("surfaceExtrudeDock");auto* buttons=dock->findChild<QDialogButtonBox*>();QVERIFY(buttons && buttons->button(QDialogButtonBox::Ok)->isEnabled());QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel),Qt::LeftButton);QTRY_VERIFY(session(w).isEmpty());QCOMPARE(call(w,"document.inspect")["authored"],before["authored"]);
  start();dock=w.findChild<QDockWidget*>("surfaceExtrudeDock");buttons=dock->findChild<QDialogButtonBox*>();QTest::mouseClick(buttons->button(QDialogButtonBox::Ok),Qt::LeftButton);QTRY_VERIFY(session(w).isEmpty());const auto after=call(w,"document.inspect");const auto f=after["authored"].toObject()["features"].toArray().last().toObject();QCOMPARE(after["evaluation"].toObject()["bodies"].toArray().size(),4);QVERIFY(!f.contains("bodyId"));
  auto edit=call(w,"feature.editDialog",{{"featureId",f["id"]}});QVERIFY(edit["previewed"].toBool());QCOMPARE(edit["resultBodyIds"].toArray().size(),2);call(w,"capture.dialog",{{"path",out+"/native-edit-two-profile-controls.png"}});dock=w.findChild<QDockWidget*>("surfaceExtrudeDock");buttons=dock->findChild<QDialogButtonBox*>();QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel),Qt::LeftButton);QTRY_VERIFY(session(w).isEmpty());QCOMPARE(call(w,"document.inspect")["authored"],after["authored"]);QVERIFY(view->diagnostics()["editDisplay"].toObject()["hiddenBodyIds"].toArray().isEmpty());
  call(w,"feature.visibility",{{"featureId",sketchId},{"visible",false}});call(w,"view.home");call(w,"capture.screenshot",{{"path",out+"/native-two-profile-committed.png"}});call(w,"document.save",{{"path",out+"/native-two-profile-sheets.alloy"}});
 }
};
QTEST_MAIN(SurfaceExtrudeProfilesControlsTest)
#include "surface_extrude_profiles_controls.moc"
