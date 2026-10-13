// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QTest>
#include <QApplication>
#include <QToolButton>
#include <QPushButton>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QMenu>
#include <QTimer>
#include <QFile>
#include <QDir>
#include <QTemporaryDir>
#include <QJsonDocument>
#include <numbers>
#include <cmath>
class SweepControlsTest final:public QObject {
 Q_OBJECT
 QJsonObject call(AlloyWindow& w,const QString& command,const QJsonObject& args={}){const auto r=w.execute(command,args);if(!r["ok"].toBool())qFatal("%s",QJsonDocument(r).toJson().constData());return r["result"].toObject();}
 QJsonObject session(AlloyWindow& w){return w.state()["activeSession"].toObject();}
 void capture(AlloyWindow& w,const QString& out,const QString& name){QApplication::processEvents();QTest::qWait(30);call(w,"capture.screenshot",{{"path",out+"/"+name+".png"}});QFile file(out+"/"+name+".json");if(!file.open(QIODevice::WriteOnly))qFatal("evidence");file.write(QJsonDocument(w.state()).toJson(QJsonDocument::Indented));}
 QPair<QJsonObject,QJsonObject> seed(AlloyWindow& w,bool clip=true){
  call(w,"sketch.create",{{"plane","XZ"},{"name","Circular section"}});call(w,"sketch.addCircle",{{"centerX",0},{"centerY",0},{"diameter",2},{"units","mm"}});const auto profile=session(w)["draftSketch"].toObject();call(w,"sketch.finish");call(w,"sketch.create",{{"plane","XY"},{"name","Analytic path controls fixture"}});
  auto line=[&](double x1,double y1,double x2,double y2){call(w,"sketch.line",{{"x1",x1},{"y1",y1},{"x2",x2},{"y2",y2},{"units","mm"}});};auto arc=[&](double x,double y,double r,double a){call(w,"sketch.addArc",{{"mode","center"},{"centerX",x},{"centerY",y},{"radius",r},{"startAngleDeg",a},{"sweepAngleDeg",-90},{"units","mm"}});};line(0,0,0,clip?33:-10);if(clip){arc(7,33,7,180);arc(7,33,7,90);line(14,33,14,-14.5);arc(8.5,-14.5,5.5,0);arc(8.5,-14.5,5.5,-90);line(3,-14.5,3,20);arc(7,20,4,180);arc(7,20,4,90);line(11,20,11,0);}const auto path=session(w)["draftSketch"].toObject();call(w,"sketch.finish");call(w,"view.home");return {profile,path};
 }
private slots:
 void nativeRolesChainCancelCommitEdit(){
  QTemporaryDir tmp;auto out=qEnvironmentVariable("ALLOY_SWEEP_CONTROLS_EVIDENCE");if(out.isEmpty())out=tmp.path();QDir().mkpath(out);AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1400,900);w.show();w.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&w));ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(child))view=v;QVERIFY(view);const auto [profile,path]=seed(w);const auto initial=call(w,"document.inspect");
  auto pixel=[&](const QJsonObject& sketch,double u,double v){QApplication::processEvents();const auto q=call(w,"view.projectSketchPoint",{{"sketchId",sketch["id"]},{"uMm",u},{"vMm",v}});return QPoint(qRound(q["x"].toDouble()),qRound(q["y"].toDouble()));};auto pick=[&](const QJsonObject& sketch,double u,double v){QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(sketch,u,v));};
  // Actual Create popup and its native QAction, no synthetic substitute menu.
  auto* create=w.findChild<QToolButton*>("solid.menu.create");QVERIFY(create && create->menu());auto* menu=create->menu();QAction* sweep=nullptr;for(auto* a:menu->actions())if(a->objectName()=="solid.action.solid.sweepDialog")sweep=a;QVERIFY(sweep);QTimer::singleShot(30,&w,[&]{if(!menu->isVisible())qFatal("Create menu not visible");QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,menu->actionGeometry(sweep).center());});QTest::mouseClick(create,Qt::LeftButton);QTRY_COMPARE(session(w)["command"].toString(),QString("solid.sweep"));QCOMPARE(session(w)["selectionRole"].toString(),QString("profile"));QVERIFY(session(w)["chainSelection"].toBool());QApplication::processEvents();
  auto* dock=w.findChild<QDockWidget*>("sweepDock");QVERIFY(dock && dock->isVisible());auto buttons=[&]{return dock->findChild<QDialogButtonBox*>();};QVERIFY(!buttons()->button(QDialogButtonBox::Ok)->isEnabled());QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,QPoint(view->width()-15,view->height()-15));QCOMPARE(session(w)["selectionError"].toObject()["code"].toString(),QString("profile_required"));
  pick(profile,0,0);QTRY_COMPARE(session(w)["selectionRole"].toString(),QString("path"));QCOMPARE(session(w)["inputs"].toObject()["profileSketchId"],profile["id"]);pick(profile,1,0);QCOMPARE(session(w)["selectionError"].toObject()["code"].toString(),QString("unsupported_path_entity"));capture(w,out,"invalid-path-selection");
  pick(path,0,16);QTRY_VERIFY(session(w)["previewed"].toBool());QCOMPARE(session(w)["inputs"].toObject()["pathEntityIds"].toArray().size(),10);const auto volume=(135+16.5*std::numbers::pi)*std::numbers::pi;QVERIFY(std::abs(session(w)["geometry"].toObject()["volumeMm3"].toDouble()-volume)<1e-5);QCOMPARE(view->diagnostics()["sweepPath"].toObject()["edgeCount"].toInt(),10);capture(w,out,"full-chain-preview");
  // Manual sections use the same picker; checkbox toggling preserves selection.
  auto* chain=dock->findChild<QCheckBox*>("sweep.chainSelection");QVERIFY(chain);chain->setFocus();QTest::keyClick(chain,Qt::Key_Space);QVERIFY(!session(w)["chainSelection"].toBool());QTest::mouseClick(dock->findChild<QPushButton*>("sweep.path.clear"),Qt::LeftButton);pick(path,0,16);QTRY_VERIFY(session(w)["previewed"].toBool());QCOMPARE(session(w)["inputs"].toObject()["pathEntityIds"].toArray().size(),1);QVERIFY(std::abs(session(w)["geometry"].toObject()["volumeMm3"].toDouble()-33*std::numbers::pi)<1e-5);
  const double a=135*std::numbers::pi/180,b=45*std::numbers::pi/180;pick(path,7+7*std::cos(a),33+7*std::sin(a));pick(path,7+7*std::cos(b),33+7*std::sin(b));QTRY_VERIFY(session(w)["previewed"].toBool());QCOMPARE(session(w)["inputs"].toObject()["pathEntityIds"].toArray().size(),3);capture(w,out,"manual-three-sections");chain->setFocus();QTest::keyClick(chain,Qt::Key_Space);QTest::mouseClick(dock->findChild<QPushButton*>("sweep.path.clear"),Qt::LeftButton);pick(path,0,16);QTRY_VERIFY(session(w)["previewed"].toBool());
  QTest::mouseClick(buttons()->button(QDialogButtonBox::Cancel),Qt::LeftButton);QTRY_VERIFY(session(w).isEmpty());QCOMPARE(call(w,"document.inspect")["authored"],initial["authored"]);QCOMPARE(call(w,"document.inspect")["undo"],initial["undo"]);QVERIFY(view->diagnostics()["sweepPath"].toObject().isEmpty());QVERIFY(view->diagnostics()["commandSketches"].toArray().isEmpty());
  auto* button=w.findChild<QToolButton*>("command.solid.sweepDialog");QVERIFY(button);QTest::mouseClick(button,Qt::LeftButton);dock=w.findChild<QDockWidget*>("sweepDock");pick(profile,0,0);pick(path,0,16);QTRY_VERIFY(session(w)["previewed"].toBool());const auto reserved=session(w)["bodyId"].toString();QTest::mouseClick(buttons()->button(QDialogButtonBox::Ok),Qt::LeftButton);QTRY_VERIFY(session(w).isEmpty());const auto accepted=call(w,"document.inspect");const auto body=accepted["evaluation"].toObject()["bodies"].toArray().first().toObject();QCOMPARE(body["bodyId"].toString(),reserved);QVERIFY(std::abs(body["volumeMm3"].toDouble()-volume)<1e-5);call(w,"view.home");capture(w,out,"committed-sweep");
  // Real Timeline edit re-exposes both auto-hidden source sketches.
  QToolButton* tile=nullptr;for(auto* b:w.findChildren<QToolButton*>())if(b->property("featureId").toString()==body["featureId"].toString())tile=b;QVERIFY(tile);QTest::mouseClick(tile,Qt::LeftButton);QTRY_COMPARE(session(w)["command"].toString(),QString("solid.sweep"));dock=w.findChild<QDockWidget*>("sweepDock");QTRY_VERIFY(session(w)["previewed"].toBool());QVERIFY(view->diagnostics()["commandSketches"].toArray().contains(profile["id"]));QVERIFY(view->diagnostics()["commandSketches"].toArray().contains(path["id"]));QTest::mouseClick(dock->findChild<QPushButton*>("sweep.profile.clear"),Qt::LeftButton);pick(profile,0,0);pick(path,0,16);QTRY_VERIFY(session(w)["previewed"].toBool());capture(w,out,"edit-retained-sources");QTest::mouseClick(buttons()->button(QDialogButtonBox::Cancel),Qt::LeftButton);QCOMPARE(call(w,"document.inspect")["authored"],accepted["authored"]);call(w,"history.undo");QCOMPARE(call(w,"document.inspect")["authored"],initial["authored"]);call(w,"history.redo");QCOMPARE(call(w,"document.inspect")["authored"],accepted["authored"]);const auto file=out+"/native-sweep.alloy";call(w,"document.save",{{"path",file}});call(w,"document.open",{{"path",file}});QCOMPARE(call(w,"document.inspect")["authored"],accepted["authored"]);
  // A short negative-Y path proves the parallel normal sign through native picks.
  call(w,"document.new",{{"discard",true}});const auto pair=seed(w,false);QTest::mouseClick(button,Qt::LeftButton);dock=w.findChild<QDockWidget*>("sweepDock");pick(pair.first,0,0);pick(pair.second,0,-5);QTRY_VERIFY(session(w)["previewed"].toBool());QVERIFY(std::abs(session(w)["geometry"].toObject()["volumeMm3"].toDouble()-10*std::numbers::pi)<1e-5);capture(w,out,"parallel-normal-preview");QTest::mouseClick(buttons()->button(QDialogButtonBox::Cancel),Qt::LeftButton);QVERIFY(session(w).isEmpty());
 }
};
QTEST_MAIN(SweepControlsTest)
#include "sweep_controls.moc"
