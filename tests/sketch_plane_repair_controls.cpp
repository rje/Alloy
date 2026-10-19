// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "sketch_plane_repair_fixtures.h"
#include <QApplication>
#include <QAction>
#include <QMenu>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTest>
class SketchPlaneRepairControlsTest final : public QObject {
 Q_OBJECT
 QJsonObject call(AlloyWindow& w,const QString& command,const QJsonObject& args={}) {auto r=w.execute(command,args);const auto initial=r.value("result").toObject();for(int i=0;r.value("completion")=="pending" && i<500;++i){QTest::qWait(10);r=w.execute("command.status",{{"session",initial.value("id")}});}if(!r.value("ok").toBool())qFatal("%s",QJsonDocument(r).toJson().constData());return r.value("result").toObject();}
private slots:
 void explicitPreviewCancelAndCommit() {
  QTemporaryDir scratch;const auto folder=qEnvironmentVariable("ALLOY_PLANE_REPAIR_EVIDENCE",scratch.path());QDir().mkpath(folder);const auto ids=SketchPlaneRepairFixtures::create(folder+"/fixtures");
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1360,860);w.show();QVERIFY(QTest::qWaitForWindowExposed(&w));call(w,"document.open",{{"path",folder+"/fixtures/ordinary-plane-repair.alloy"}});const auto base=call(w,"document.inspect");
  const auto failed=w.execute("solid.combine",{{"featureId",ids.value("combineFeatureId")},{"targetBodyId",ids.value("targetBodyId")},{"toolBodyIds",QJsonArray{ids.value("deleteBodyId")}},{"operation","cut"},{"keepTools",true}});QVERIFY(!failed.value("ok").toBool());const auto cached=w.state().value("activeSession");
  auto* timeline=w.findChild<QAction*>("timeline.redefineSketchPlane."+ids.value("sketchId").toString());QVERIFY(timeline);QMenu* timelineMenu=nullptr;for(auto* menu:w.findChildren<QMenu*>())if(menu->actions().contains(timeline))timelineMenu=menu;QVERIFY(timelineMenu);QVERIFY(QMetaObject::invokeMethod(timelineMenu,"aboutToShow",Qt::DirectConnection));QVERIFY(timeline->isEnabled());
  auto* begin=w.findChild<QPushButton*>("failedEditRepair.redefinePlane");QVERIFY(begin && begin->isVisible());QTest::mouseClick(begin,Qt::LeftButton);
  auto* list=w.findChild<QListWidget*>("sketch.redefinePlane.choices");QVERIFY(list);QListWidgetItem* target=nullptr;for(int i=0;i<list->count();++i)if(list->item(i)->data(Qt::UserRole).toJsonObject().value("support")==ids.value("replacementSupport"))target=list->item(i);QVERIFY(target);list->scrollToItem(target);QTest::qWait(10);const auto hit=list->visualItemRect(target).center();QCOMPARE(QApplication::widgetAt(list->viewport()->mapToGlobal(hit)),list->viewport());QTest::mouseClick(list->viewport(),Qt::LeftButton,Qt::NoModifier,hit);
  QVERIFY(w.state().value("activeSession").toObject().value("planeRedefinition").toObject().value("previewed").toBool());QVERIFY(w.findChild<QLabel*>("sketch.redefinePlane.frame")->text().contains("Normal [0, 0, 1]"));QCOMPARE(call(w,"document.inspect"),base);
  auto* cancel=w.findChild<QPushButton*>("sketch.redefinePlane.cancel");QVERIFY(cancel && cancel->isVisible());QTest::mouseClick(cancel,Qt::LeftButton);QCOMPARE(w.state().value("activeSession"),cached);QCOMPARE(call(w,"document.inspect"),base);const auto restored=w.state().value("viewport").toObject().value("gpu").toObject();QVERIFY(restored.value("draftSketchVisible").toBool());QCOMPARE(restored.value("draftSketchId"),ids.value("sketchId"));QVERIFY(restored.value("bodyStageKey").toString().isEmpty());
  QTest::mouseClick(begin,Qt::LeftButton);list=w.findChild<QListWidget*>("sketch.redefinePlane.choices");QVERIFY(list);target=nullptr;for(int i=0;i<list->count();++i)if(list->item(i)->data(Qt::UserRole).toJsonObject().value("support")==ids.value("replacementSupport"))target=list->item(i);QVERIFY(target);list->scrollToItem(target);QTest::qWait(10);QTest::mouseClick(list->viewport(),Qt::LeftButton,Qt::NoModifier,list->visualItemRect(target).center());
  call(w,"view.orientation",{{"name","isometric"}});call(w,"view.fit");QTest::qWait(20);call(w,"capture.screenshot",{{"path",folder+"/preview.png"}});
  auto* ok=w.findChild<QPushButton*>("sketch.redefinePlane.ok");QVERIFY(ok && ok->isVisible() && ok->isEnabled());QTest::mouseClick(ok,Qt::LeftButton);QVERIFY(w.state().value("activeSession").toObject().isEmpty());const auto after=call(w,"document.inspect");QVERIFY(!w.findChild<QWidget*>("sketch.redefinePlane.dock")->isVisible());QVERIFY(call(w,"history.failedEditInspect").isEmpty());const auto stale=w.execute("sketch.redefinePlaneCommit");QVERIFY(!stale.value("ok").toBool());QCOMPARE(stale.value("error").toObject().value("code").toString(),QString("replacement_required"));QCOMPARE(call(w,"document.inspect"),after);QCOMPARE(after.value("undo").toObject().value("count").toInt(),base.value("undo").toObject().value("count").toInt()+1);
  const auto repaired=SketchGeometry::findFeature(after.value("authored").toObject(),ids.value("sketchId").toString());QCOMPARE(repaired.value("parameters").toObject().value("support"),ids.value("replacementSupport"));
  call(w,"history.undo");QCOMPARE(call(w,"document.inspect").value("authored"),base.value("authored"));call(w,"history.redo");QCOMPARE(call(w,"document.inspect").value("authored"),after.value("authored"));
  const auto gpu=w.state().value("viewport").toObject().value("gpu").toObject();QVERIFY(gpu.value("hardwareAccelerated").toBool());QFile receipt(folder+"/qt-receipt.json");QVERIFY(receipt.open(QIODevice::WriteOnly));receipt.write(QJsonDocument(QJsonObject{{"status","pass"},{"input","Qt-delivered only; physical input unqualified"},{"gpu",gpu},{"after",after}}).toJson(QJsonDocument::Indented));
 }
};
QTEST_MAIN(SketchPlaneRepairControlsTest)
#include "sketch_plane_repair_controls.moc"
