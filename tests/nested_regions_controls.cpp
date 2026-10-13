// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QTest>
#include <QToolButton>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QTreeWidget>
#include <QLabel>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <numbers>
#include <cmath>
class NestedRegionsControlsTest final:public QObject{
 Q_OBJECT
 QJsonObject call(AlloyWindow& w,const QString& c,const QJsonObject& a={}){auto r=w.execute(c,a);if(!r["ok"].toBool())qFatal("%s",QJsonDocument(r).toJson().constData());return r["result"].toObject();}
 QJsonObject session(AlloyWindow& w){return w.state()["activeSession"].toObject();}
 void capture(AlloyWindow& w,const QString& out,const QString& name){QApplication::processEvents();QTest::qWait(40);call(w,"capture.screenshot",{{"path",out+"/"+name+".png"}});QFile file(out+"/"+name+".json");if(!file.open(QIODevice::WriteOnly))qFatal("evidence");file.write(QJsonDocument(w.state()).toJson(QJsonDocument::Indented));}
private slots:
 void selectAnnulusIslandAndDiagnostics(){
  QTemporaryDir tmp;auto out=qEnvironmentVariable("ALLOY_NESTED_REGIONS_EVIDENCE");if(out.isEmpty())out=tmp.path();QDir().mkpath(out);AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1400,900);w.show();w.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&w));ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(child))view=v;QVERIFY(view);
  call(w,"sketch.create",{{"plane","XY"},{"name","Nested fixture"}});call(w,"sketch.rectangle",{{"x1",0},{"y1",0},{"x2",40},{"y2",25},{"units","mm"}});call(w,"sketch.addCircle",{{"centerX",20},{"centerY",12.5},{"diameter",10},{"units","mm"}});call(w,"sketch.addCircle",{{"centerX",20},{"centerY",12.5},{"diameter",4},{"units","mm"}});const auto source=session(w)["draftSketch"].toObject();const auto preview=call(w,"command.preview",{{"session",session(w)["id"]}});const auto regions=preview["sketchGeometry"].toObject()["profiles"].toArray();QCOMPARE(regions.size(),3);QString outer,island;for(const auto& q:regions){const auto p=q.toObject();if(std::abs(p["areaMm2"].toDouble()-(1000-25*std::numbers::pi))<1e-6)outer=p["profileId"].toString();if(std::abs(p["areaMm2"].toDouble()-4*std::numbers::pi)<1e-6)island=p["profileId"].toString();}QVERIFY(!outer.isEmpty() && !island.isEmpty());auto* status=w.findChild<QLabel*>("sketch.profileStatus");QVERIFY(status && status->isVisible() && status->text().contains("3 selectable"));capture(w,out,"nested-sketch");call(w,"sketch.finish");call(w,"view.orientation",{{"name","top"}});call(w,"view.home");const auto initial=call(w,"document.inspect");
  auto begin=[&](double u,double v,const QString& expected){QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,QPoint(view->width()-18,view->height()-18));auto* button=w.findChild<QToolButton*>("command.solid.extrudeDialog");if(!button)qFatal("Extrude toolbar");QTest::mouseClick(button,Qt::LeftButton);const auto target=call(w,"view.projectSketchPoint",{{"sketchId",source["id"]},{"uMm",u},{"vMm",v}});QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,QPoint(qRound(target["x"].toDouble()),qRound(target["y"].toDouble())));const auto inputs=session(w)["inputs"].toObject();const auto picks=inputs["profileIds"].toArray();const auto actual=inputs.contains("profileId") ? inputs["profileId"].toString():(picks.isEmpty()?QString{}:picks.first().toString());if(actual.isEmpty())qFatal("No profile at %s; session=%s",QJsonDocument(target).toJson().constData(),QJsonDocument(session(w)).toJson().constData());if(actual!=expected)qFatal("Wrong actual region");auto* distance=w.findChild<QDoubleSpinBox*>("extrude.distance");if(!distance)qFatal("Distance control");distance->setFocus();for(int i=0;i<6 && distance->value()<5;++i)QTest::keyClick(distance,Qt::Key_Up);};
  begin(5,12.5,outer);QTRY_VERIFY(session(w)["previewed"].toBool());QVERIFY(std::abs(session(w)["geometry"].toObject()["volumeMm3"].toDouble()-(5000-125*std::numbers::pi))<1e-5);capture(w,out,"annulus-preview");auto* box=w.findChild<QDockWidget*>("extrudeDock")->findChild<QDialogButtonBox*>();QTest::mouseClick(box->button(QDialogButtonBox::Cancel),Qt::LeftButton);QTRY_VERIFY(session(w).isEmpty());QCOMPARE(call(w,"document.inspect")["authored"],initial["authored"]);QCOMPARE(call(w,"document.inspect")["undo"],initial["undo"]);
  begin(5,12.5,outer);QTRY_VERIFY(session(w)["previewed"].toBool());box=w.findChild<QDockWidget*>("extrudeDock")->findChild<QDialogButtonBox*>();QTest::mouseClick(box->button(QDialogButtonBox::Ok),Qt::LeftButton);QTRY_VERIFY(session(w).isEmpty());QCOMPARE(call(w,"document.inspect")["evaluation"].toObject()["bodies"].toArray().size(),1);
  // Ordinary Browser row/Space reveals the retained source, as in accepted input.
  QTreeWidgetItem* item=nullptr;QTreeWidget* tree=nullptr;for(auto* candidate:w.findChildren<QTreeWidget*>()){QTreeWidgetItemIterator it(candidate);while(*it){if((*it)->data(0,Qt::UserRole+2)==source["id"].toString()){item=*it;tree=candidate;break;}++it;}if(item)break;}QVERIFY(item && tree);tree->scrollToItem(item);QTest::mouseClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,tree->visualItemRect(item).center());QTest::keyClick(tree,Qt::Key_Space);QTRY_COMPARE(item->checkState(0),Qt::Checked);QTRY_VERIFY(call(w,"document.inspect")["evaluation"].toObject()["sketches"].toArray().first().toObject()["visible"].toBool());capture(w,out,"retained-sketch-visible");const auto beforeIsland=call(w,"document.inspect");begin(20,12.5,island);QTRY_VERIFY(session(w)["previewed"].toBool());QVERIFY(std::abs(session(w)["geometry"].toObject()["volumeMm3"].toDouble()-20*std::numbers::pi)<1e-5);capture(w,out,"island-preview");box=w.findChild<QDockWidget*>("extrudeDock")->findChild<QDialogButtonBox*>();QTest::mouseClick(box->button(QDialogButtonBox::Ok),Qt::LeftButton);QTRY_VERIFY(session(w).isEmpty());const auto accepted=call(w,"document.inspect");QCOMPARE(accepted["evaluation"].toObject()["bodies"].toArray().size(),2);call(w,"view.home");capture(w,out,"nested-solids");call(w,"history.undo");QCOMPARE(call(w,"document.inspect")["authored"],beforeIsland["authored"]);call(w,"history.redo");QCOMPARE(call(w,"document.inspect")["authored"],accepted["authored"]);const auto path=out+"/native-nested-regions.alloy";call(w,"document.save",{{"path",path}});call(w,"document.open",{{"path",path}});QCOMPARE(call(w,"document.inspect")["authored"],accepted["authored"]);
  call(w,"sketch.edit",{{"sketchId",source["id"]}});call(w,"sketch.line",{{"x1",60},{"y1",0},{"x2",65},{"y2",0},{"units","mm"}});call(w,"sketch.line",{{"x1",40},{"y1",0},{"x2",0},{"y2",0},{"units","mm"}});QTRY_VERIFY(w.findChild<QLabel*>("sketch.profileStatus")->text().contains("coincident"));QVERIFY(w.findChild<QLabel*>("sketch.profileStatus")->text().contains("excluded"));call(w,"sketch.lookAt");capture(w,out,"visible-open-duplicate-status");call(w,"command.cancel",{{"session",session(w)["id"]}});QCOMPARE(call(w,"document.inspect")["authored"],accepted["authored"]);
 }
};
QTEST_MAIN(NestedRegionsControlsTest)
#include "nested_regions_controls.moc"
