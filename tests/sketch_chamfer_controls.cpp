// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QAction>
#include <QApplication>
#include <QDir>
#include <QComboBox>
#include <QFile>
#include <QJsonDocument>
#include <QLineEdit>
#include <QMenu>
#include <QMouseEvent>
#include <QProcess>
#include <QPushButton>
#include <QTest>
#include <QTimer>
#include <QTemporaryDir>
#include <QToolButton>
#include <QWindow>
class SketchChamferControlsTest final:public QObject{
 Q_OBJECT
 QJsonObject call(AlloyWindow& w,const QString& cmd,const QJsonObject& a={}){const auto r=w.execute(cmd,a);if(!r.value("ok").toBool())qFatal("%s",QJsonDocument(r).toJson().constData());return r.value("result").toObject();}
 QJsonObject session(AlloyWindow& w){return w.state().value("activeSession").toObject();}
 QJsonObject draft(AlloyWindow& w){return session(w).value("draftSketch").toObject();}
private slots:
 void nativeDistanceAndAngle(){
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1500,950);w.show();QVERIFY(QTest::qWaitForWindowExposed(&w));
#ifdef Q_OS_MACOS
  QProcess activate;activate.start("osascript",{"-e",QString("tell application \"System Events\" to set frontmost of first application process whose unix id is %1 to true").arg(QCoreApplication::applicationPid())});QVERIFY(activate.waitForFinished(5000));QCOMPARE(activate.exitCode(),0);
#endif
  w.raise();w.activateWindow();w.windowHandle()->requestActivate();QApplication::setActiveWindow(&w);QTRY_VERIFY(w.isActiveWindow());
  ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(child))view=v;QVERIFY(view);
  const auto output=qEnvironmentVariable("ALLOY_ANGLE_CHAMFER_EVIDENCE");if(!output.isEmpty())QDir().mkpath(output);
  auto capture=[&](const QString& name){if(!output.isEmpty())call(w,"capture.screenshot",{{"path",output+"/"+name+".png"}});};
  auto coords=[](const QJsonObject& p){return QPoint(qRound(p.value("x").toDouble()),qRound(p.value("y").toDouble()));};
  auto pixel=[&](double u,double v){return coords(call(w,"view.projectSketchPoint",{{"sketchId",draft(w).value("id")},{"uMm",u},{"vMm",v}}));};
  auto group=[&]{return draft(w).value("parameters").toObject().value("chamferGroups").toArray().first().toObject();};
  auto label=[&](bool angle){for(const auto& row:view->annotations()){const auto a=row.toObject();if(a.value("kind")== (angle?"chamferAngle":"chamferDistance"))return a;}return QJsonObject{};};
  auto motion=[&](const QPoint& to){QMouseEvent event(QEvent::MouseMove,QPointF(to),QPointF(view->mapToGlobal(to)),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(view,&event);};
  auto targetAngle=[&](const QJsonObject& h,double degrees){const auto radians=h.value("turn").toDouble()*degrees*std::acos(-1.0)/180;const double u=h.value("axisU").toDouble(),v=h.value("axisV").toDouble();return pixel(h.value("originUMm").toDouble()+20*(u*std::cos(radians)-v*std::sin(radians)),h.value("originVMm").toDouble()+20*(u*std::sin(radians)+v*std::cos(radians)));};
  call(w,"sketch.begin",{{"plane","XZ"},{"units","mm"}});call(w,"sketch.centerRectangle",{{"centerX",0},{"centerY",0},{"width",150},{"height",75},{"units","mm"}});call(w,"sketch.lookAt");call(w,"view.fit");call(w,"view.zoom",{{"factor",1.7}});const auto base=draft(w);
  auto* modify=w.findChild<QToolButton*>("sketch.menu.modify");auto* submenu=w.findChild<QMenu*>("sketch.menu.chamfer");auto* action=w.findChild<QAction*>("sketch.tool.distanceAngleChamfer");QVERIFY(modify && submenu && action);QCOMPARE(action->text(),QString("Distance And Angle Chamfer"));bool sawModify=false,sawChamfer=false;
  QTimer::singleShot(80,&w,[&]{sawModify=modify->menu()->isVisible();QTest::mouseClick(modify->menu(),Qt::LeftButton,Qt::NoModifier,modify->menu()->actionGeometry(submenu->menuAction()).center());QTimer::singleShot(200,&w,[&]{sawChamfer=submenu->isVisible();if(sawChamfer){capture("01-distance-angle-menu");QTest::mouseClick(submenu,Qt::LeftButton,Qt::NoModifier,submenu->actionGeometry(action).center());}else modify->menu()->close();});});
  QTest::mouseClick(modify,Qt::LeftButton);QVERIFY(sawModify && sawChamfer);QTRY_VERIFY(!modify->menu()->isVisible() && !submenu->isVisible());QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
  QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(75,37.5));auto* distance=w.findChild<QLineEdit*>("sketch.chamfer.distance");auto* angle=w.findChild<QLineEdit*>("sketch.chamfer.angle");auto* mode=w.findChild<QComboBox*>("sketch.chamfer.mode");QVERIFY(distance && angle && mode && angle->isVisible());QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(distance));
  QTest::keyClicks(distance,"10");QTest::keyClick(distance,Qt::Key_Tab);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(angle));QTest::keyClicks(angle,"60");QCOMPARE(draft(w),base);QVERIFY(session(w).value("pendingChamfer").toObject().contains("previewDraft"));capture("02-distance-angle-preview");
  // All three implemented modes share Feature Options and keep the pending corner.
  QTest::mouseClick(mode,Qt::LeftButton);QTest::keyClick(mode,Qt::Key_Home);QTest::keyClick(mode,Qt::Key_Return);QVERIFY(!angle->isVisible());QCOMPARE(draft(w),base);
  QTest::mouseClick(mode,Qt::LeftButton);QTest::keyClick(mode,Qt::Key_Down);QTest::keyClick(mode,Qt::Key_Return);QCOMPARE(mode->currentData().toString(),QString("twoDistance"));QCOMPARE(draft(w),base);
  QTest::mouseClick(mode,Qt::LeftButton);QTest::keyClick(mode,Qt::Key_End);QTest::keyClick(mode,Qt::Key_Return);QVERIFY(angle->isVisible());QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(distance));QTest::keyClick(distance,Qt::Key_Tab);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(angle));QTest::keyClick(angle,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(angle,"60");QTest::keyClick(angle,Qt::Key_Return);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QCOMPARE(group().value("type").toString(),QString("distanceAngle"));QCOMPARE(group().value("angleDeg").toDouble(),60.0);const auto stable=group();
  // Both driving annotations open the existing inline quantity editor.
  for(int side=0;side<2;++side){const auto annotation=label(side==1);QVERIFY(annotation.value("driving").toBool());const auto at=coords(annotation.value("label").toObject());QCOMPARE(QApplication::widgetAt(view->mapToGlobal(at)),static_cast<QWidget*>(view));QTest::mouseDClick(view,Qt::LeftButton,Qt::NoModifier,at);auto* input=w.findChild<QLineEdit*>("sketch.inlineValue");QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(input));QVERIFY(!input->isReadOnly());QTest::keyClicks(input,side?"30 deg":"12 mm");QTest::keyClick(input,Qt::Key_Return);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QCOMPARE(group().value(side?"distanceMm":"angleDeg").toDouble(),side?12.0:60.0);}capture("03-driving-distance-angle");
  const auto beforeHandles=draft(w);auto handles=view->diagnostics().value("chamferHandles").toArray();QCOMPARE(handles.size(),2);auto from=coords(handles[0].toObject()),to=pixel(75,23.5);QCOMPARE(QApplication::widgetAt(view->mapToGlobal(from)),static_cast<QWidget*>(view));QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,from);motion(to);QCOMPARE(draft(w),beforeHandles);QTest::mouseRelease(view,Qt::LeftButton,Qt::NoModifier,to);QVERIFY(std::abs(group().value("distanceMm").toDouble()-14)<0.5);QCOMPARE(group().value("angleDeg").toDouble(),30.0);call(w,"history.undo");QCOMPARE(draft(w),beforeHandles);call(w,"history.redo");call(w,"history.undo");
  handles=view->diagnostics().value("chamferHandles").toArray();QCOMPARE(handles.size(),2);auto angleHandle=handles[1].toObject();QCOMPARE(angleHandle.value("kind").toString(),QString("angle"));from=coords(angleHandle);to=targetAngle(angleHandle,45);QCOMPARE(QApplication::widgetAt(view->mapToGlobal(from)),static_cast<QWidget*>(view));QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,from);motion(to);QCOMPARE(draft(w),beforeHandles);QVERIFY(std::abs(angle->text().toDouble()-45)<0.5);capture("04-angle-handle-preview");QTest::mouseRelease(view,Qt::LeftButton,Qt::NoModifier,to);QVERIFY(std::abs(group().value("angleDeg").toDouble()-45)<0.5);QCOMPARE(group().value("distanceMm").toDouble(),12.0);call(w,"history.undo");QCOMPARE(draft(w),beforeHandles);call(w,"history.redo");call(w,"history.undo");
  handles=view->diagnostics().value("chamferHandles").toArray();QCOMPARE(handles.size(),2);angleHandle=handles[1].toObject();from=coords(angleHandle);to=targetAngle(angleHandle,100);QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,from);motion(to);QVERIFY(!session(w).value("pendingChamfer").toObject().contains("previewDraft"));QCOMPARE(draft(w),beforeHandles);capture("05-out-of-wedge-refusal");QTest::mouseRelease(view,Qt::LeftButton,Qt::NoModifier,to);QCOMPARE(draft(w),beforeHandles);QVERIFY(!session(w).contains("pendingChamfer"));
  handles=view->diagnostics().value("chamferHandles").toArray();QCOMPARE(handles.size(),2);angleHandle=handles[1].toObject();from=coords(angleHandle);to=targetAngle(angleHandle,50);QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,from);motion(to);QTest::keyClick(view,Qt::Key_Escape);QTest::mouseRelease(view,Qt::LeftButton,Qt::NoModifier,to);QCOMPARE(draft(w),beforeHandles);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
  auto* edit=w.findChild<QPushButton*>("sketch.chamfer.edit");auto* cancel=w.findChild<QPushButton*>("sketch.chamfer.cancel");QTest::mouseClick(edit,Qt::LeftButton);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(distance));QTest::keyClicks(distance,"16");QCOMPARE(QApplication::widgetAt(cancel->mapToGlobal(cancel->rect().center())),static_cast<QWidget*>(cancel));QTest::mouseClick(cancel,Qt::LeftButton);QCOMPARE(draft(w),beforeHandles);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
  const auto dimensions=base.value("parameters").toObject().value("dimensions").toArray();for(int i=0;i<2;++i){call(w,"sketch.dimensionEdit",{{"dimensionId",dimensions[i].toObject().value("id")}});auto* input=w.findChild<QLineEdit*>("sketch.inlineValue");QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(input));QTest::keyClicks(input,i?"80":"160");QTest::keyClick(input,Qt::Key_Return);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));}QCOMPARE(group().value("id"),stable.value("id"));QCOMPARE(group().value("distanceMm").toDouble(),12.0);QCOMPARE(group().value("angleDeg").toDouble(),30.0);capture("06-source-edit-angle");
  if(!output.isEmpty()){QFile f(output+"/native-final-state.json");QVERIFY(f.open(QIODevice::WriteOnly));f.write(QJsonDocument(w.state()).toJson());QVERIFY(view->framebuffer().save(output+"/native-framebuffer.png"));}QVERIFY(view->diagnostics().value("hardwareAccelerated").toBool());
  call(w,"sketch.finish");auto inspection=call(w,"document.inspect");const auto profile=inspection.value("evaluation").toObject().value("sketches").toArray().first().toObject().value("profiles").toArray().first().toObject();const double area=12800-72/std::sqrt(3.0);QVERIFY(std::abs(profile.value("areaMm2").toDouble()-area)<1e-6);call(w,"solid.extrude",{{"sketchId",base.value("id")},{"profileId",profile.value("profileId")},{"distance",5},{"units","mm"}});inspection=call(w,"document.inspect");QVERIFY(std::abs(inspection.value("evaluation").toObject().value("bodies").toArray().first().toObject().value("volumeMm3").toDouble()-5*area)<1e-5);call(w,"history.undo");call(w,"history.redo");QCOMPARE(call(w,"document.inspect").value("authored"),inspection.value("authored"));
  QTemporaryDir storage;QVERIFY(storage.isValid());const auto model=output.isEmpty()?storage.filePath("distance-angle.alloy"):output+"/distance-angle.alloy";call(w,"document.save",{{"path",model}});call(w,"document.open",{{"path",model}});QCOMPARE(call(w,"document.inspect").value("authored"),inspection.value("authored"));call(w,"view.home");capture("07-reopened-solid");if(!output.isEmpty()){QFile f(output+"/capture-metadata.json");QVERIFY(f.open(QIODevice::WriteOnly));f.write(QJsonDocument(call(w,"capture.screenshot",{{"path",output+"/native-composite.png"}})).toJson());}w.close();
 }
 void nativeTwoDistanceAndFeatureOptions(){
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1500,950);w.show();QVERIFY(QTest::qWaitForWindowExposed(&w));
#ifdef Q_OS_MACOS
  QProcess activate;activate.start("osascript",{"-e",QString("tell application \"System Events\" to set frontmost of first application process whose unix id is %1 to true").arg(QCoreApplication::applicationPid())});QVERIFY(activate.waitForFinished(5000));QCOMPARE(activate.exitCode(),0);
#endif
  w.raise();w.activateWindow();w.windowHandle()->requestActivate();QApplication::setActiveWindow(&w);QTRY_VERIFY(w.isActiveWindow());
  ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(child))view=v;QVERIFY(view);
  const auto output=qEnvironmentVariable("ALLOY_TWO_CHAMFER_EVIDENCE");if(!output.isEmpty())QDir().mkpath(output);
  auto capture=[&](const QString& name){if(!output.isEmpty())call(w,"capture.screenshot",{{"path",output+"/"+name+".png"}});};
  auto coords=[](const QJsonObject& p){return QPoint(qRound(p.value("x").toDouble()),qRound(p.value("y").toDouble()));};
  auto pixel=[&](double u,double v){return coords(call(w,"view.projectSketchPoint",{{"sketchId",draft(w).value("id")},{"uMm",u},{"vMm",v}}));};
  auto group=[&]{return draft(w).value("parameters").toObject().value("chamferGroups").toArray().first().toObject();};
  auto label=[&](int side){for(const auto& row:view->annotations()){const auto a=row.toObject();if(a.value("kind")=="chamferDistance" && a.value("side").toInt()==side)return a;}return QJsonObject{};};
  auto motion=[&](const QPoint& to){QMouseEvent event(QEvent::MouseMove,QPointF(to),QPointF(view->mapToGlobal(to)),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(view,&event);};
  call(w,"sketch.begin",{{"plane","XZ"},{"units","mm"}});call(w,"sketch.centerRectangle",{{"centerX",0},{"centerY",0},{"width",150},{"height",75},{"units","mm"}});call(w,"sketch.lookAt");call(w,"view.fit");call(w,"view.zoom",{{"factor",1.7}});const auto base=draft(w);
  auto* modify=w.findChild<QToolButton*>("sketch.menu.modify");auto* submenu=w.findChild<QMenu*>("sketch.menu.chamfer");auto* action=w.findChild<QAction*>("sketch.tool.twoDistanceChamfer");QVERIFY(modify && submenu && action);QCOMPARE(action->text(),QString("Two Distance Chamfer"));
  bool sawModify=false,sawChamfer=false;
  QTimer::singleShot(80,&w,[&]{sawModify=modify->menu()->isVisible();QTest::mouseClick(modify->menu(),Qt::LeftButton,Qt::NoModifier,modify->menu()->actionGeometry(submenu->menuAction()).center());QTimer::singleShot(200,&w,[&]{sawChamfer=submenu->isVisible();if(sawChamfer){capture("01-two-distance-menu");QTest::mouseClick(submenu,Qt::LeftButton,Qt::NoModifier,submenu->actionGeometry(action).center());}else modify->menu()->close();});});
  QTest::mouseClick(modify,Qt::LeftButton);QVERIFY(sawModify && sawChamfer);QTRY_VERIFY(!modify->menu()->isVisible() && !submenu->isVisible());QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
  QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(75,37.5));
  auto* first=w.findChild<QLineEdit*>("sketch.chamfer.distance");auto* second=w.findChild<QLineEdit*>("sketch.chamfer.distance2");auto* mode=w.findChild<QComboBox*>("sketch.chamfer.mode");QVERIFY(first && second && mode && second->isVisible());QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(first));
  // The documented initial first-handle adjustment moves both setbacks.
  auto initialHandles=view->diagnostics().value("chamferHandles").toArray();QCOMPARE(initialHandles.size(),2);
  const auto firstFrom=coords(initialHandles.first().toObject()),firstTo=pixel(75,22.5);
  QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,firstFrom);motion(firstTo);
  const auto initialDrag=session(w).value("pendingChamfer").toObject();QVERIFY(std::abs(initialDrag.value("distanceMm").toDouble()-15)<0.5);QCOMPARE(initialDrag.value("distanceMm"),initialDrag.value("distance2Mm"));
  QTest::keyClick(view,Qt::Key_Escape);QTest::mouseRelease(view,Qt::LeftButton,Qt::NoModifier,firstTo);QCOMPARE(draft(w),base);
  call(w,"sketch.chamferMode",{{"type","twoDistance"}});QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(75,37.5));QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(first));
  QTest::keyClicks(first,"10");QTest::keyClick(first,Qt::Key_Tab);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(second));QTest::keyClicks(second,"20");QCOMPARE(draft(w),base);capture("02-independent-preview");
  // Feature Options switches the pending corner without authoring geometry.
  QTest::mouseClick(mode,Qt::LeftButton);QTest::keyClick(mode,Qt::Key_Home);QTest::keyClick(mode,Qt::Key_Return);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(first));QVERIFY(!second->isVisible());QCOMPARE(draft(w),base);
  QTest::mouseClick(mode,Qt::LeftButton);QTest::keyClick(mode,Qt::Key_Home);QTest::keyClick(mode,Qt::Key_Down);QTest::keyClick(mode,Qt::Key_Return);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(first));QVERIFY(second->isVisible());QCOMPARE(draft(w),base);
  QTest::keyClick(first,Qt::Key_Tab);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(second));QTest::keyClick(second,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(second,"20");QTest::keyClick(second,Qt::Key_Return);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QCOMPARE(group().value("type").toString(),QString("twoDistance"));QCOMPARE(group().value("distanceMm").toDouble(),10.0);QCOMPARE(group().value("distance2Mm").toDouble(),20.0);
  const auto stable=group();const auto entities=base.value("parameters").toObject().value("entities").toArray();QCOMPARE(group().value("sourceEntityIds"),QJsonValue(QJsonArray{entities[1].toObject().value("id"),entities[2].toObject().value("id")}));
  for(int side=0;side<2;++side){const auto annotation=label(side);QVERIFY(annotation.value("driving").toBool());QCOMPARE(QApplication::widgetAt(view->mapToGlobal(coords(annotation.value("label").toObject()))),static_cast<QWidget*>(view));QTest::mouseDClick(view,Qt::LeftButton,Qt::NoModifier,coords(annotation.value("label").toObject()));auto* input=w.findChild<QLineEdit*>("sketch.inlineValue");QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(input));QVERIFY(!input->isReadOnly());QTest::keyClicks(input,side?"18":"12");QTest::keyClick(input,Qt::Key_Return);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QCOMPARE(group().value(side?"distanceMm":"distance2Mm").toDouble(),side?12.0:20.0);}capture("03-both-driving-labels");
  const auto beforeDrag=draft(w);auto handles=view->diagnostics().value("chamferHandles").toArray();QCOMPARE(handles.size(),2);const auto from=coords(handles[0].toObject());const auto to=pixel(75,23.5);QCOMPARE(QApplication::widgetAt(view->mapToGlobal(from)),static_cast<QWidget*>(view));QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,from);motion(to);QCOMPARE(draft(w),beforeDrag);QVERIFY(session(w).value("pendingChamfer").toObject().contains("previewDraft"));capture("04-first-side-handle");QTest::mouseRelease(view,Qt::LeftButton,Qt::NoModifier,to);QVERIFY(std::abs(group().value("distanceMm").toDouble()-14)<0.5);QCOMPARE(group().value("distance2Mm").toDouble(),18.0);call(w,"history.undo");QCOMPARE(draft(w),beforeDrag);call(w,"history.redo");
  const auto beforeSecondDrag=draft(w);handles=view->diagnostics().value("chamferHandles").toArray();const auto secondFrom=coords(handles[1].toObject()),secondTo=pixel(53,37.5);
  const auto firstUnchanged=group().value("distanceMm");QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,secondFrom);motion(secondTo);QTest::mouseRelease(view,Qt::LeftButton,Qt::NoModifier,secondTo);QCOMPARE(group().value("distanceMm"),firstUnchanged);QVERIFY(std::abs(group().value("distance2Mm").toDouble()-22)<0.5);call(w,"history.undo");QCOMPARE(draft(w),beforeSecondDrag);call(w,"history.redo");call(w,"history.undo");QCOMPARE(draft(w),beforeSecondDrag);
  const auto beforeBad=draft(w);auto* edit=w.findChild<QPushButton*>("sketch.chamfer.edit");QTest::mouseClick(edit,Qt::LeftButton);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(first));QTest::keyClick(first,Qt::Key_Tab);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(second));QTest::keyClicks(second,"200");QVERIFY(!session(w).value("pendingChamfer").toObject().contains("previewDraft"));QTest::keyClick(second,Qt::Key_Return);QCOMPARE(draft(w),beforeBad);capture("05-invalid-second-distance");QTest::keyClick(second,Qt::Key_Escape);QCOMPARE(draft(w),beforeBad);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
  const auto dimensions=base.value("parameters").toObject().value("dimensions").toArray();for(int i=0;i<2;++i){call(w,"sketch.dimensionEdit",{{"dimensionId",dimensions[i].toObject().value("id")}});auto* input=w.findChild<QLineEdit*>("sketch.inlineValue");QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(input));QTest::keyClicks(input,i?"80":"160");QTest::keyClick(input,Qt::Key_Return);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));}QCOMPARE(group().value("id"),stable.value("id"));capture("06-source-edit");
  if(!output.isEmpty()){QFile f(output+"/native-final-state.json");QVERIFY(f.open(QIODevice::WriteOnly));f.write(QJsonDocument(w.state()).toJson());QVERIFY(view->framebuffer().save(output+"/native-framebuffer.png"));}QVERIFY(view->diagnostics().value("hardwareAccelerated").toBool());
  const auto distance1=group().value("distanceMm").toDouble();call(w,"sketch.finish");const auto inspect=call(w,"document.inspect");const auto profile=inspect.value("evaluation").toObject().value("sketches").toArray().first().toObject().value("profiles").toArray().first().toObject();QVERIFY(std::abs(profile.value("areaMm2").toDouble()-(12800-distance1*18/2))<1e-6);
  call(w,"solid.extrude",{{"sketchId",base.value("id")},{"profileId",profile.value("profileId")},{"distance",5},{"units","mm"}});const auto final=call(w,"document.inspect");QVERIFY(std::abs(final.value("evaluation").toObject().value("bodies").toArray().first().toObject().value("volumeMm3").toDouble()-5*(12800-distance1*18/2))<1e-5);call(w,"history.undo");call(w,"history.redo");QCOMPARE(call(w,"document.inspect").value("authored"),final.value("authored"));
  QTemporaryDir storage;QVERIFY(storage.isValid());const auto model=output.isEmpty()?storage.filePath("two-distance.alloy"):output+"/two-distance.alloy";
  call(w,"document.save",{{"path",model}});call(w,"document.open",{{"path",model}});QCOMPARE(call(w,"document.inspect").value("authored"),final.value("authored"));call(w,"view.home");capture("07-reopened-solid");
  if(!output.isEmpty()){QFile f(output+"/capture-metadata.json");QVERIFY(f.open(QIODevice::WriteOnly));f.write(QJsonDocument(call(w,"capture.screenshot",{{"path",output+"/native-composite.png"}})).toJson());}
  w.close();
 }
 void nativeHandleAndDirectAnnotation(){
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1500,950);w.show();QVERIFY(QTest::qWaitForWindowExposed(&w));
#ifdef Q_OS_MACOS
  QProcess activate;activate.start("osascript",{"-e",QString("tell application \"System Events\" to set frontmost of first application process whose unix id is %1 to true").arg(QCoreApplication::applicationPid())});QVERIFY(activate.waitForFinished(5000));QCOMPARE(activate.exitCode(),0);
#endif
  w.raise();w.activateWindow();w.windowHandle()->requestActivate();QApplication::setActiveWindow(&w);QTRY_VERIFY(w.isActiveWindow());
  ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(child))view=v;QVERIFY(view);
  const auto output=qEnvironmentVariable("ALLOY_CHAMFER_DRAG_EVIDENCE");if(!output.isEmpty())QDir().mkpath(output);
  auto capture=[&](const QString& name){if(!output.isEmpty())call(w,"capture.screenshot",{{"path",output+"/"+name+".png"}});};
  auto saveJson=[&](const QString& name,const QJsonObject& value){if(!output.isEmpty()){QFile f(output+"/"+name+".json");QVERIFY(f.open(QIODevice::WriteOnly));f.write(QJsonDocument(value).toJson());}};
  auto coords=[](const QJsonObject& p){return QPoint(qRound(p.value("x").toDouble()),qRound(p.value("y").toDouble()));};
  auto pixel=[&](double u,double v){return coords(call(w,"view.projectSketchPoint",{{"sketchId",draft(w).value("id")},{"uMm",u},{"vMm",v}}));};
  auto handles=[&]{return view->diagnostics().value("chamferHandles").toArray();};
  auto motion=[&](const QPoint& to){QMouseEvent event(QEvent::MouseMove,QPointF(to),QPointF(view->mapToGlobal(to)),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(view,&event);};
  auto group=[&]{return draft(w).value("parameters").toObject().value("chamferGroups").toArray().first().toObject();};
  auto label=[&](bool driving){for(const auto& row:view->annotations()){const auto a=row.toObject();if(a.value("kind")=="chamferDistance" && a.value("driving").toBool()==driving)return a;}return QJsonObject{};};
  call(w,"sketch.begin",{{"plane","XZ"},{"units","mm"}});call(w,"sketch.centerRectangle",{{"centerX",0},{"centerY",0},{"width",150},{"height",75},{"units","mm"}});call(w,"sketch.lookAt");call(w,"view.fit");call(w,"view.zoom",{{"factor",1.7}});
  const auto base=draft(w);const auto history=session(w).value("draftHistory").toObject();
  call(w,"sketch.tool",{{"tool","chamfer"}});QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
  QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(75,37.5));QCOMPARE(handles().size(),1);
  auto* distance=w.findChild<QLineEdit*>("sketch.chamfer.distance");QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(distance));capture("01-selected-corner-handle");
  auto from=coords(handles().first().toObject());auto to=pixel(65,27.5); // midpoint for a 20 mm setback
  QCOMPARE(QApplication::widgetAt(view->mapToGlobal(from)),static_cast<QWidget*>(view));
  QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,from);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QVERIFY(session(w).value("pendingChamfer").toObject().value("dragActive").toBool());
  motion(pixel(67.5,30));motion(to);QCOMPARE(draft(w),base);QVERIFY(session(w).value("pendingChamfer").toObject().contains("previewDraft"));QVERIFY(std::abs(distance->text().toDouble()-20)<0.5);capture("02-native-drag-preview");saveJson("drag-preview-state",w.state());
  QTest::mouseRelease(view,Qt::LeftButton,Qt::NoModifier,to);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QVERIFY(!session(w).contains("pendingChamfer"));QVERIFY(std::abs(group().value("distanceMm").toDouble()-20)<0.5);
  QCOMPARE(session(w).value("draftHistory").toObject().value("count").toInt(),history.value("count").toInt()+1);
  const auto created=draft(w);const auto stableGroup=group();call(w,"history.undo");QCOMPARE(draft(w),base);call(w,"history.redo");QCOMPARE(draft(w),created);
  // Existing-corner drag is disposable until release, and Escape rolls it back.
  from=coords(handles().first().toObject());to=pixel(60,22.5);QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,from);motion(to);QCOMPARE(draft(w),created);QTest::keyClick(view,Qt::Key_Escape);QVERIFY(!session(w).contains("pendingChamfer"));QTest::mouseRelease(view,Qt::LeftButton,Qt::NoModifier,to);QCOMPARE(draft(w),created);
  from=coords(handles().first().toObject());to=pixel(62.5,25);QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,from);motion(to);QTest::mouseRelease(view,Qt::LeftButton,Qt::NoModifier,to);QVERIFY(std::abs(group().value("distanceMm").toDouble()-25)<0.5);QCOMPARE(group().value("id"),stableGroup.value("id"));
  const auto beforeInvalid=draft(w);call(w,"history.undo");QCOMPARE(draft(w),created);call(w,"history.redo");QCOMPARE(draft(w),beforeInvalid);from=coords(handles().first().toObject());to=pixel(25,-12.5);QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,from);motion(to);QVERIFY(!session(w).value("pendingChamfer").toObject().contains("previewDraft"));QCOMPARE(draft(w),beforeInvalid);capture("03-invalid-drag");QTest::mouseRelease(view,Qt::LeftButton,Qt::NoModifier,to);QCOMPARE(draft(w),beforeInvalid);QVERIFY(!session(w).contains("pendingChamfer"));
  // First displayed dimension owns the edit; the other is visibly driven.
  auto first=label(true);QVERIFY(!first.isEmpty());QTest::mouseDClick(view,Qt::LeftButton,Qt::NoModifier,coords(first.value("label").toObject()));
  auto* input=w.findChild<QLineEdit*>("sketch.inlineValue");QVERIFY(input && input->isVisible());QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(input));QVERIFY(!input->isReadOnly());capture("04-inline-first-distance");
  QTest::keyClicks(input,"12 mm");QTest::keyClick(input,Qt::Key_Return);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QCOMPARE(group().value("distanceMm").toDouble(),12.0);
  const auto afterAnnotation=draft(w);call(w,"history.undo");QCOMPARE(draft(w),beforeInvalid);call(w,"history.redo");QCOMPARE(draft(w),afterAnnotation);QTest::mouseDClick(view,Qt::LeftButton,Qt::NoModifier,coords(label(true).value("label").toObject()));QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(input));QTest::keyClicks(input,"15");QTest::keyClick(input,Qt::Key_Escape);QCOMPARE(draft(w),afterAnnotation);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
  auto second=label(false);QVERIFY(second.value("label").toObject().value("text").toString().startsWith("("));QTest::mouseDClick(view,Qt::LeftButton,Qt::NoModifier,coords(second.value("label").toObject()));QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(input));QVERIFY(input->isReadOnly());capture("05-driven-second-distance");QTest::keyClick(input,Qt::Key_Return);QCOMPARE(draft(w),afterAnnotation);
  const auto dimensions=base.value("parameters").toObject().value("dimensions").toArray();call(w,"sketch.dimensionUpdate",{{"dimensionId",dimensions[0].toObject().value("id")},{"value",160},{"units","mm"}});call(w,"sketch.dimensionUpdate",{{"dimensionId",dimensions[1].toObject().value("id")},{"value",80},{"units","mm"}});QCOMPARE(group().value("id"),stableGroup.value("id"));QCOMPARE(group().value("distanceMm").toDouble(),12.0);capture("06-source-edited-handle");
  const auto finalDraft=draft(w);call(w,"history.undo");call(w,"history.redo");QCOMPARE(draft(w),finalDraft);saveJson("native-final-state",w.state());QVERIFY(view->diagnostics().value("hardwareAccelerated").toBool());
  call(w,"sketch.finish");const auto inspect=call(w,"document.inspect");const auto sketches=inspect.value("evaluation").toObject().value("sketches").toArray();QCOMPARE(sketches.first().toObject().value("profiles").toArray().size(),1);QVERIFY(std::abs(sketches.first().toObject().value("profiles").toArray().first().toObject().value("areaMm2").toDouble()-12728)<1e-6);
  QTemporaryDir storage;QVERIFY(storage.isValid());const auto model=output.isEmpty()?storage.filePath("chamfer-handle.alloy"):output+"/chamfer-handle.alloy";
  call(w,"document.save",{{"path",model}});call(w,"document.open",{{"path",model}});QCOMPARE(call(w,"document.inspect").value("authored"),inspect.value("authored"));
  if(!output.isEmpty()){saveJson("capture-metadata",call(w,"capture.screenshot",{{"path",output+"/07-reopened.png"}}));QVERIFY(view->framebuffer().save(output+"/native-framebuffer.png"));}
  w.close();
 }
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
  action->trigger();QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
  QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(-75,37.5));
  QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(distance));QVERIFY(session(w).value("pendingChamfer").toObject().contains("previewDraft"));
  QTest::keyClicks(distance,"8");capture("04b-adjacent-corner-preview");QTest::keyClick(distance,Qt::Key_Return);
  QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
  auto* retained=w.findChild<QComboBox*>("sketch.chamfer.retained");QVERIFY(retained && retained->isVisible());QCOMPARE(retained->count(),2);
  const auto second=draft(w).value("parameters").toObject().value("chamferGroups").toArray()[1].toObject();
  // A real key on the chooser selects the second retained corner for editing.
  QTest::mouseClick(retained,Qt::LeftButton);QTest::keyClick(retained,Qt::Key_End);QTest::keyClick(retained,Qt::Key_Return);
  QTRY_COMPARE(retained->currentIndex(),1);
  auto* edit=w.findChild<QPushButton*>("sketch.chamfer.edit");QVERIFY(edit && edit->isVisible());QTest::mouseClick(edit,Qt::LeftButton);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(distance));QTest::keyClicks(distance,"9");QTest::keyClick(distance,Qt::Key_Return);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
  auto secondEdited=draft(w).value("parameters").toObject().value("chamferGroups").toArray()[1].toObject();QCOMPARE(secondEdited.value("id"),second.value("id"));QCOMPARE(secondEdited.value("distanceMm").toDouble(),9.0);
  QTest::mouseClick(retained,Qt::LeftButton);QTest::keyClick(retained,Qt::Key_Home);QTest::keyClick(retained,Qt::Key_Return);QTRY_COMPARE(retained->currentIndex(),0);
  QTest::mouseClick(edit,Qt::LeftButton);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(distance));QTest::keyClicks(distance,"12");QTest::keyClick(distance,Qt::Key_Return);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QCOMPARE(draft(w).value("parameters").toObject().value("chamferGroups").toArray().first().toObject().value("lineId"),group.value("lineId"));
  // Existing native dimension editor owns the original width/height intent.
  const auto dimensions=sourceDimensions.toArray();for(int i=0;i<2;++i){call(w,"sketch.dimensionEdit",{{"dimensionId",dimensions[i].toObject().value("id")}});auto* input=w.findChild<QLineEdit*>("sketch.inlineValue");QVERIFY(input && input->isVisible());QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(input));QTest::keyClick(input,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(input,i==0?"160":"80");QTest::keyClick(input,Qt::Key_Return);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));}capture("05-source-and-distance-edit");
  const auto before=draft(w);QTest::mouseClick(edit,Qt::LeftButton);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(distance));QTest::keyClicks(distance,"155");QVERIFY(!session(w).value("pendingChamfer").toObject().contains("previewDraft"));QTest::keyClick(distance,Qt::Key_Return);QCOMPARE(draft(w),before);capture("06-transactional-refusal");QTest::keyClick(distance,Qt::Key_Escape);QCOMPARE(draft(w),before);
  const auto gpu=w.state().value("viewport").toObject().value("gpu").toObject();QVERIFY(gpu.value("hardwareAccelerated").toBool());QVERIFY(!gpu.value("vendor").toString().isEmpty());QVERIFY(!gpu.value("version").toString().isEmpty());if(!output.isEmpty()){QFile f(output+"/native-state.json");QVERIFY(f.open(QIODevice::WriteOnly));f.write(QJsonDocument(w.state()).toJson());const auto captureInfo=call(w,"capture.screenshot",{{"path",output+"/native-framebuffer-composite.png"}});QFile captureFile(output+"/capture-metadata.json");QVERIFY(captureFile.open(QIODevice::WriteOnly));captureFile.write(QJsonDocument(captureInfo).toJson());QVERIFY(view->framebuffer().save(output+"/native-framebuffer.png"));}
  w.resize(1300,850);QTest::qWait(80);QVERIFY(w.state().value("viewport").toObject().value("gpu").toObject().value("hardwareAccelerated").toBool());capture("07-resized");call(w,"sketch.cancel");w.close();
 }
};
QTEST_MAIN(SketchChamferControlsTest)
#include "sketch_chamfer_controls.moc"
