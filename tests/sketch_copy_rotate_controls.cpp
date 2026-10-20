// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include "SketchGeometry.h"
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QMenu>
#include <QPushButton>
#include <QTest>
#include <QTimer>
#include <QToolButton>
#include <cmath>
class SketchCopyRotateControls final:public QObject{
 Q_OBJECT
 QJsonObject call(AlloyWindow& w,const QString& id,const QJsonObject& a={}){auto r=w.execute(id,a);if(!r["ok"].toBool())qFatal("%s",QJsonDocument(r).toJson().constData());const auto first=r["result"].toObject();QElapsedTimer t;t.start();while(r["completion"]=="pending"){if(t.elapsed()>20000)qFatal("Command timeout: %s",qPrintable(id));QTest::qWait(10);r=w.execute("command.status",{{"session",first["id"]},{"generation",first["previewGeneration"]}});if(!r["ok"].toBool())qFatal("%s",QJsonDocument(r).toJson().constData());}return r["result"].toObject();}
 QJsonObject active(AlloyWindow& w){return w.state()["activeSession"].toObject();}QJsonObject draft(AlloyWindow& w){return active(w)["draftSketch"].toObject();}
 void click(QWidget* w,const QPoint& p){QVERIFY(w&&w->isVisible());const auto g=w->mapToGlobal(p);auto* hit=QApplication::widgetAt(g);QVERIFY(hit==w||w->isAncestorOf(hit));QTest::mouseClick(hit,Qt::LeftButton,Qt::NoModifier,hit->mapFromGlobal(g));}void click(QWidget* w){click(w,w->rect().center());}
private slots:
 void createCopyBesideRotate(){
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1500,950);w.show();w.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&w));QApplication::setActiveWindow(&w);
  const double xy[4][2]={{12,6},{32,10},{27,22},{9,18}};call(w,"sketch.begin",{{"plane","XY"}});for(int i=0;i<4;++i)call(w,"sketch.line",{{"x1",xy[i][0]},{"y1",xy[i][1]},{"x2",xy[(i+1)%4][0]},{"y2",xy[(i+1)%4][1]},{"units","mm"}});auto points=draft(w)["parameters"].toObject()["points"].toArray();for(int i=0;i<5;++i){const int a=i<4?i:0,b=i<4?(i+1)%4:2;call(w,"sketch.dimension",{{"kind","linear"},{"pointIds",QJsonArray{points[a].toObject()["id"],points[b].toObject()["id"]}},{"value",std::hypot(xy[a][0]-xy[b][0],xy[a][1]-xy[b][1])},{"units","mm"}});}QJsonArray rotateIds;for(const auto& e:draft(w)["parameters"].toObject()["entities"].toArray())rotateIds.append(e.toObject()["id"]);call(w,"sketch.3dMode",{{"enabled",true}});auto axis=call(w,"sketch.axis3d",{{"start",QJsonObject{{"u",4},{"v",3},{"w",0}}},{"end",QJsonObject{{"u",4},{"v",3},{"w",10}}},{"units","mm"}})["axis"].toObject();call(w,"sketch.rotatePreview",{{"entityIds",rotateIds},{"axisLineId",axis["id"]},{"angleDeg",30}});call(w,"sketch.rotateAccept");call(w,"sketch.rectangle",{{"x1",80},{"y1",8},{"x2",100},{"y2",20},{"units","mm"}});const auto source=draft(w);const auto owner=source["parameters"].toObject().value("rotateGroups");const auto axisData=source["parameters"].toObject().value("constructionLines3d");const auto history=active(w).value("draftHistory");
  ReferenceView* view=nullptr;for(auto* v:w.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(v))view=candidate;QVERIFY(view);const auto sid=source["id"].toString();auto project=[&](double u,double v){const auto q=call(w,"view.projectSketchPoint",{{"sketchId",sid},{"uMm",u},{"vMm",v}});return QPoint(qRound(q["x"].toDouble()),qRound(q["y"].toDouble()));};
  call(w,"sketch.tool",{{"tool","select"}});call(w,"sketch.lookAt");call(w,"view.zoom",{{"factor",.55}});int pickIndex=0;for(const auto& uv:QJsonArray{QJsonArray{90,8},QJsonArray{100,14},QJsonArray{90,20},QJsonArray{80,14}}){auto a=uv.toArray();const auto p=project(a[0].toDouble(),a[1].toDouble());QCOMPARE(QApplication::widgetAt(view->mapToGlobal(p)),static_cast<QWidget*>(view));const auto global=view->mapToGlobal(p);auto* hit=QApplication::widgetAt(global);QTest::mouseClick(hit,Qt::LeftButton,pickIndex++?Qt::ShiftModifier:Qt::NoModifier,hit->mapFromGlobal(global));}QCOMPARE(active(w)["selection"].toObject()["entityIds"].toArray().size(),4);QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
  auto* modify=w.findChild<QToolButton*>("solid.menu.modify");auto* menu=w.findChild<QMenu*>("solid.commands.modify");auto* action=w.findChild<QAction*>("solid.action.solid.moveDialog");QVERIFY(modify&&menu&&action);bool opened=false;QTimer::singleShot(0,[&]{if(menu->isVisible()){const auto p=menu->actionGeometry(action).center();auto* hit=QApplication::widgetAt(menu->mapToGlobal(p));if(hit==menu){opened=true;QTest::mouseClick(hit,Qt::LeftButton,Qt::NoModifier,p);return;}}menu->close();});click(modify);QVERIFY(opened);QTRY_VERIFY(QApplication::activePopupWidget()==nullptr);auto* panel=w.findChild<QDialog*>("sketch.move.dialog");QTRY_VERIFY(panel);QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));auto* copy=panel->findChild<QCheckBox*>("sketch.move.createCopy");QVERIFY(copy&&copy->isEnabled());click(copy,QPoint(10,copy->height()/2));QVERIFY(copy->isChecked());
  auto enter=[&](QDoubleSpinBox* box,const QString& value){QVERIFY(box);click(box);QVERIFY(box==QApplication::focusWidget()||box->isAncestorOf(QApplication::focusWidget()));auto* f=QApplication::focusWidget();QTest::keySequence(f,QKeySequence::SelectAll);QTest::keyClicks(f,value);QTest::keyClick(f,Qt::Key_Tab);};enter(panel->findChild<QDoubleSpinBox*>("sketch.move.x"),"65");enter(panel->findChild<QDoubleSpinBox*>("sketch.move.y"),"0");QTRY_VERIFY(active(w).contains("pendingSketchMove"));const auto preview=active(w)["pendingSketchMove"].toObject()["draft"].toObject();QCOMPARE(preview["parameters"].toObject()["rotateGroups"],owner);QCOMPARE(preview["parameters"].toObject()["constructionLines3d"],axisData);QCOMPARE(preview["parameters"].toObject()["entities"].toArray().size(),12);QCOMPARE(draft(w),source);QCOMPARE(active(w)["draftHistory"],history);
  click(panel->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok));QTRY_VERIFY(!panel->isVisible());const auto final=draft(w);QCOMPARE(final["parameters"].toObject()["entities"].toArray().size(),12);QCOMPARE(final["parameters"].toObject()["rotateGroups"],owner);QCOMPARE(final["parameters"].toObject()["constructionLines3d"],axisData);call(w,"history.undo");QCOMPARE(draft(w),source);call(w,"history.redo");QCOMPARE(draft(w),final);QCOMPARE(SketchGeometry::evaluateFeature(final).profiles.size(),3);const auto evidence=qEnvironmentVariable("ALLOY_COPY_ROTATE_EVIDENCE");if(!evidence.isEmpty()){QDir().mkpath(evidence);call(w,"capture.screenshot",{{"path",evidence+"/native-create-copy.png"}});QFile report(evidence+"/native-report.json");QVERIFY(report.open(QIODevice::WriteOnly));report.write(QJsonDocument(QJsonObject{{"passed",true},{"original",source},{"final",final},{"rotateOwner",owner},{"axis",axisData},{"state",w.state()}}).toJson());}call(w,"sketch.cancel");w.close();
 }
};
QTEST_MAIN(SketchCopyRotateControls)
#include "sketch_copy_rotate_controls.moc"
