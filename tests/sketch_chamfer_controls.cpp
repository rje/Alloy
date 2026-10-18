// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QAction>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QLineEdit>
#include <QMenu>
#include <QProcess>
#include <QPushButton>
#include <QTest>
#include <QTimer>
#include <QToolButton>
#include <QWindow>
class SketchChamferControlsTest final:public QObject{
 Q_OBJECT
 QJsonObject call(AlloyWindow& w,const QString& cmd,const QJsonObject& a={}){const auto r=w.execute(cmd,a);if(!r.value("ok").toBool())qFatal("%s",QJsonDocument(r).toJson().constData());return r.value("result").toObject();}
 QJsonObject session(AlloyWindow& w){return w.state().value("activeSession").toObject();}
 QJsonObject draft(AlloyWindow& w){return session(w).value("draftSketch").toObject();}
private slots:
 void nativeCornerPreviewCancelAndRetainedEdit(){
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1500,950);w.show();QVERIFY(QTest::qWaitForWindowExposed(&w));
#ifdef Q_OS_MACOS
  QProcess activate;activate.start("osascript",{"-e",QString("tell application \"System Events\" to set frontmost of first application process whose unix id is %1 to true").arg(QCoreApplication::applicationPid())});QVERIFY(activate.waitForFinished(5000));QCOMPARE(activate.exitCode(),0);
#endif
  w.raise();w.activateWindow();w.windowHandle()->requestActivate();QApplication::setActiveWindow(&w);QTRY_VERIFY(w.isActiveWindow());
  ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(child))view=v;QVERIFY(view);
  const auto output=qEnvironmentVariable("ALLOY_CHAMFER_EVIDENCE");if(!output.isEmpty())QDir().mkpath(output);
  auto capture=[&](const QString& name){if(!output.isEmpty()){QTest::qWait(60);call(w,"capture.screenshot",{{"path",output+"/"+name+".png"}});}};
  call(w,"sketch.begin",{{"plane","XZ"},{"units","mm"}});call(w,"sketch.centerRectangle",{{"centerX",0},{"centerY",0},{"width",150},{"height",75},{"units","mm"}});call(w,"sketch.lookAt");call(w,"view.fit");call(w,"view.zoom",{{"factor",1.7}});
  const auto base=draft(w);const auto sourceDimensions=base.value("parameters").toObject().value("dimensions");
  auto pixel=[&](double u,double v){const auto p=call(w,"view.projectSketchPoint",{{"sketchId",draft(w).value("id")},{"uMm",u},{"vMm",v}});return QPoint(qRound(p.value("x").toDouble()),qRound(p.value("y").toDouble()));};
  auto* modify=w.findChild<QToolButton*>("sketch.menu.modify");auto* submenu=w.findChild<QMenu*>("sketch.menu.chamfer");auto* action=w.findChild<QAction*>("sketch.tool.chamfer");QVERIFY(modify && submenu && action);QCOMPARE(action->text(),QString("Equal Distance Chamfer"));
  // Queued events operate each visible popup, then observe closure and focus.
  bool sawModify=false,sawChamfer=false;
  QTimer::singleShot(80,&w,[&]{sawModify=modify->menu()->isVisible();QTest::mouseClick(modify->menu(),Qt::LeftButton,Qt::NoModifier,modify->menu()->actionGeometry(submenu->menuAction()).center());QTimer::singleShot(200,&w,[&]{sawChamfer=submenu->isVisible();if(sawChamfer){capture("01-equal-distance-menu");QTest::mouseClick(submenu,Qt::LeftButton,Qt::NoModifier,submenu->actionGeometry(action).center());}else modify->menu()->close();});});
  QTest::mouseClick(modify,Qt::LeftButton);QVERIFY(sawModify && sawChamfer);QTRY_COMPARE(session(w).value("tool").toString(),QString("chamfer"));QTRY_VERIFY(!modify->menu()->isVisible() && !submenu->isVisible());QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
  const auto corner=pixel(75,37.5);QVERIFY(view->rect().contains(corner));QTest::mouseMove(view,corner);capture("02-corner-hover");QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,corner);
  auto* distance=w.findChild<QLineEdit*>("sketch.chamfer.distance");QVERIFY(distance && distance->isVisible());QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(distance));QVERIFY(session(w).value("pendingChamfer").toObject().contains("previewDraft"));QCOMPARE(draft(w),base);capture("03-distance-preview");
  QTest::keyClick(distance,Qt::Key_Escape);QTRY_VERIFY(!session(w).contains("pendingChamfer"));QCOMPARE(draft(w),base);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
  action->trigger();QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
  QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(75,0));QVERIFY(session(w).value("pendingChamfer").toObject().contains("firstSideId"));QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
  QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(0,37.5));QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(distance));QTest::keyClicks(distance,"10");QCOMPARE(draft(w),base);QTest::keyClick(distance,Qt::Key_Return);QTRY_VERIFY(!session(w).contains("pendingChamfer"));QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
  auto group=draft(w).value("parameters").toObject().value("chamferGroups").toArray().first().toObject();QCOMPARE(group.value("distanceMm").toDouble(),10.0);QCOMPARE(draft(w).value("parameters").toObject().value("dimensions"),sourceDimensions);capture("04-committed-setbacks");
  auto* edit=w.findChild<QPushButton*>("sketch.chamfer.edit");QVERIFY(edit && edit->isVisible());QTest::mouseClick(edit,Qt::LeftButton);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(distance));QTest::keyClicks(distance,"12");QTest::keyClick(distance,Qt::Key_Return);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QCOMPARE(draft(w).value("parameters").toObject().value("chamferGroups").toArray().first().toObject().value("lineId"),group.value("lineId"));
  // Existing native dimension editor owns the original width/height intent.
  const auto dimensions=sourceDimensions.toArray();for(int i=0;i<2;++i){call(w,"sketch.dimensionEdit",{{"dimensionId",dimensions[i].toObject().value("id")}});auto* input=w.findChild<QLineEdit*>("sketch.inlineValue");QVERIFY(input && input->isVisible());QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(input));QTest::keyClick(input,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(input,i==0?"160":"80");QTest::keyClick(input,Qt::Key_Return);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));}capture("05-source-and-distance-edit");
  const auto before=draft(w);QTest::mouseClick(edit,Qt::LeftButton);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(distance));QTest::keyClicks(distance,"90");QVERIFY(!session(w).value("pendingChamfer").toObject().contains("previewDraft"));QTest::keyClick(distance,Qt::Key_Return);QCOMPARE(draft(w),before);capture("06-transactional-refusal");QTest::keyClick(distance,Qt::Key_Escape);QCOMPARE(draft(w),before);
  const auto gpu=w.state().value("viewport").toObject().value("gpu").toObject();QVERIFY(gpu.value("hardwareAccelerated").toBool());QVERIFY(!gpu.value("vendor").toString().isEmpty());QVERIFY(!gpu.value("version").toString().isEmpty());if(!output.isEmpty()){QFile f(output+"/native-state.json");QVERIFY(f.open(QIODevice::WriteOnly));f.write(QJsonDocument(w.state()).toJson());const auto captureInfo=call(w,"capture.screenshot",{{"path",output+"/native-framebuffer-composite.png"}});QFile captureFile(output+"/capture-metadata.json");QVERIFY(captureFile.open(QIODevice::WriteOnly));captureFile.write(QJsonDocument(captureInfo).toJson());QVERIFY(view->framebuffer().save(output+"/native-framebuffer.png"));}
  w.resize(1300,850);QTest::qWait(80);QVERIFY(w.state().value("viewport").toObject().value("gpu").toObject().value("hardwareAccelerated").toBool());capture("07-resized");call(w,"sketch.cancel");w.close();
 }
};
QTEST_MAIN(SketchChamferControlsTest)
#include "sketch_chamfer_controls.moc"
