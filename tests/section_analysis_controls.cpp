#include "AlloyWindow.h"
#include "ReferenceView.h"
#include "AlloyTheme.h"
#include <QTest>
#include <QContextMenuEvent>
#include <QApplication>
#include <QToolButton>
#include <QMenu>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QAbstractItemView>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QPushButton>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QTimer>
#include <QWindow>
#include <QProcess>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
class SectionControlsTest final:public QObject {
 Q_OBJECT
 QJsonObject call(AlloyWindow& w,const QString& command,const QJsonObject& args={}){const auto r=w.execute(command,args);if(!r["ok"].toBool())qFatal("%s",QJsonDocument(r).toJson().constData());return r["result"].toObject();}
 QJsonObject session(AlloyWindow& w){return w.state()["activeSession"].toObject();}
 void capture(AlloyWindow& w,const QString& name){const auto out=qEnvironmentVariable("ALLOY_SECTION_EVIDENCE");if(out.isEmpty())return;QDir().mkpath(out);QTest::qWait(100);call(w,"capture.screenshot",{{"path",out+"/"+name+".png"}});QFile f(out+"/"+name+".json");if(!f.open(QIODevice::WriteOnly))qFatal("evidence");f.write(QJsonDocument(w.state()).toJson());}
private slots:
 void selectedFaceNative(){
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1400,900);w.show();w.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&w));
  ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;QVERIFY(view);
  call(w,"solid.box",{{"width",40},{"depth",30},{"height",20}});call(w,"view.home");call(w,"view.orientation",{{"name","top"}});
  const auto before=call(w,"document.inspect"),body=before["evaluation"].toObject()["bodies"].toArray()[0].toObject();QJsonObject face;
  for(const auto& item:body["supportFaces"].toArray())if(item.toObject()["role"]=="box.z1")face=item.toObject();QVERIFY(!face.isEmpty());
  QJsonObject support;for(const auto& key:{"producerFeatureId","bodyId","faceId"})support[key]=face[key];
  const auto projected=call(w,"view.projectFacePoint",{{"support",support},{"uMm",20},{"vMm",15}});
  const QPoint point(qRound(projected["x"].toDouble()),qRound(projected["y"].toDouble()));
  // One initial harness activation after API setup, before the native sequence.
  #ifdef Q_OS_MACOS
  QProcess activation;activation.start("osascript",{"-e",QString("tell application \"System Events\" to set frontmost of first application process whose unix id is %1 to true").arg(QCoreApplication::applicationPid())});QVERIFY(activation.waitForFinished(5000));QCOMPARE(activation.exitCode(),0);
#endif
  w.raise();w.activateWindow();w.windowHandle()->requestActivate();QApplication::setActiveWindow(&w);QTest::qWait(100);
  call(w,"inspect.sectionDialog");auto* dock=w.findChild<QDockWidget*>("sectionDock");QVERIFY(dock && dock->isVisible());
  auto* plane=dock->findChild<QComboBox*>("sectionPlane");QVERIFY(plane);
  auto choosePlane=[&](int row){QTRY_VERIFY(plane->hasFocus());QTest::keyClick(plane,Qt::Key_Home);for(int i=0;i<row;++i)QTest::keyClick(plane,Qt::Key_Down);};
  choosePlane(1);QTRY_COMPARE(session(w)["inputs"].toObject()["plane"].toString(),QString("XY"));QVERIFY(session(w)["previewed"].toBool());
  choosePlane(4);QTRY_COMPARE(session(w)["inputs"].toObject()["plane"].toString(),QString("face"));QVERIFY(!session(w)["previewed"].toBool());QVERIFY(!dock->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->isEnabled());
  QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,point);QTRY_COMPARE(session(w)["inputs"].toObject()["support"].toObject(),support);
  auto* offset=dock->findChild<QDoubleSpinBox*>("sectionOffset");QVERIFY(offset && offset->isEnabled());QTest::mouseClick(offset,Qt::LeftButton);QTRY_VERIFY(offset->hasFocus());QTest::keyClick(offset,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(offset,"-4");QTest::keyClick(offset,Qt::Key_Return);QTRY_COMPARE(session(w)["inputs"].toObject()["offsetMm"].toDouble(),-4.0);
  auto* name=dock->findChild<QLineEdit*>("sectionName");QVERIFY(name);QTest::mouseClick(name,Qt::LeftButton);QTRY_VERIFY(name->hasFocus());QTest::keyClick(name,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(name,"Top section");QTest::keyClick(name,Qt::Key_Tab);QTRY_COMPARE(session(w)["inputs"].toObject()["name"].toString(),QString("Top section"));
  capture(w,"native-face-preview");QTest::mouseClick(dock->findChild<QPushButton*>("sectionClearFace"),Qt::LeftButton);QVERIFY(!session(w)["inputs"].toObject().contains("plane"));QVERIFY(!dock->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->isEnabled());
  QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,point);QTRY_COMPARE(session(w)["inputs"].toObject()["support"].toObject(),support);
  QTest::mouseClick(dock->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Cancel),Qt::LeftButton);QCOMPARE(call(w,"document.inspect"),before);
  call(w,"inspect.sectionDialog");dock=w.findChild<QDockWidget*>("sectionDock");QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,point);QTRY_COMPARE(session(w)["inputs"].toObject()["support"].toObject(),support);
  offset=dock->findChild<QDoubleSpinBox*>("sectionOffset");QTest::mouseClick(offset,Qt::LeftButton);QTRY_VERIFY(offset->hasFocus());QTest::keyClick(offset,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(offset,"-4");QTest::keyClick(offset,Qt::Key_Return);
  QTest::mouseClick(dock->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok),Qt::LeftButton);QTRY_VERIFY(session(w).isEmpty());
  QCOMPARE(call(w,"analysis.inspect")["analyses"].toArray()[0].toObject()["resolvedPlane"].toObject()["origin"].toArray(),QJsonArray({0,0,16}));capture(w,"native-face-committed");
 }
 void nativeLifecycle(){
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1400,900);w.show();w.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&w));
  call(w,"sketch.begin",{{"plane","XY"}});call(w,"sketch.addCircle",{{"centerX",0},{"centerY",0},{"diameter",50},{"units","mm"}});const auto sketch=session(w)["draftSketch"].toObject();call(w,"sketch.finish");const auto profile=call(w,"document.inspect")["evaluation"].toObject()["sketches"].toArray()[0].toObject()["profiles"].toArray()[0].toObject()["profileId"];
  call(w,"solid.extrude",{{"sketchId",sketch["id"]},{"profileId",profile},{"distance",60},{"units","mm"}});const auto body=call(w,"document.inspect")["evaluation"].toObject()["bodies"].toArray()[0].toObject();QJsonObject support;for(const auto& f:body["supportFaces"].toArray())if(f.toObject()["role"]=="extrude.endCap")for(const auto& key:{"producerFeatureId","bodyId","faceId"})support[key]=f.toObject()[key];
  call(w,"solid.shell",{{"targetBodyId",body["bodyId"]},{"removedFace",support},{"thicknessMm",5}});call(w,"view.home");const auto before=call(w,"document.inspect");
  // One initial harness activation after API setup, before the native sequence.
  #ifdef Q_OS_MACOS
  QProcess activation;activation.start("osascript",{"-e",QString("tell application \"System Events\" to set frontmost of first application process whose unix id is %1 to true").arg(QCoreApplication::applicationPid())});QVERIFY(activation.waitForFinished(5000));QCOMPARE(activation.exitCode(),0);
#endif
  w.raise();w.activateWindow();w.windowHandle()->requestActivate();QApplication::setActiveWindow(&w);QTest::qWait(100);
  auto* inspect=w.findChild<QToolButton*>("solid.menu.inspect");QVERIFY(inspect && inspect->menu());auto* menu=inspect->menu();QAction* action=nullptr;for(auto* a:menu->actions())if(a->objectName()=="solid.action.inspect.sectionDialog")action=a;QVERIFY(action);
  auto open=[&]{QTimer::singleShot(30,&w,[&]{QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,menu->actionGeometry(action).center());});QTest::mouseClick(inspect,Qt::LeftButton);};
  open();QCOMPARE(session(w)["command"].toString(),QString("inspect.section"));auto* dock=w.findChild<QDockWidget*>("sectionDock");QVERIFY(dock && dock->isVisible());auto* plane=dock->findChild<QComboBox*>("sectionPlane");QVERIFY(session(w)["status"]=="active");QVERIFY(!dock->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->isEnabled());
  QTest::mouseClick(plane,Qt::LeftButton);QTRY_VERIFY(plane->view()->isVisible());QTRY_VERIFY(plane->hasFocus());QTest::keyClick(plane,Qt::Key_Down);QTest::keyClick(plane,Qt::Key_Down);QTest::keyClick(plane,Qt::Key_Return);QTRY_COMPARE(session(w)["inputs"].toObject()["plane"].toString(),QString("XZ"));QVERIFY(session(w)["previewed"].toBool());capture(w,"native-preview");QTest::mouseClick(dock->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Cancel),Qt::LeftButton);QVERIFY(session(w).isEmpty());QCOMPARE(call(w,"document.inspect"),before);
  open();dock=w.findChild<QDockWidget*>("sectionDock");plane=dock->findChild<QComboBox*>("sectionPlane");QVERIFY(session(w)["status"]=="active");QTest::mouseClick(plane,Qt::LeftButton);QTRY_VERIFY(plane->view()->isVisible());QTRY_VERIFY(plane->hasFocus());QTest::keyClick(plane,Qt::Key_Down);QTest::keyClick(plane,Qt::Key_Down);QTest::keyClick(plane,Qt::Key_Return);QTRY_VERIFY(session(w)["previewed"].toBool());QTest::mouseClick(dock->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok),Qt::LeftButton);QVERIFY(session(w).isEmpty());const auto accepted=call(w,"document.inspect");QCOMPARE(accepted["authored"].toObject()["features"],before["authored"].toObject()["features"]);auto ae=accepted["evaluation"].toObject(),be=before["evaluation"].toObject();ae.remove("documentRevision");be.remove("documentRevision");QCOMPARE(ae,be);capture(w,"native-committed");
  auto* tree=w.findChild<QTreeWidget*>();QVERIFY(tree);QTreeWidgetItem* row=nullptr;for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->parent() && (*it)->parent()->text(0)=="Analysis" && !(*it)->isHidden())row=*it;QVERIFY(row);
  const auto point=tree->visualItemRect(row).center();QTimer::singleShot(40,&w,[&]{auto* popup=qobject_cast<QMenu*>(QApplication::activePopupWidget());if(!popup)qFatal("analysis context menu missing");QTest::mouseClick(popup,Qt::LeftButton,Qt::NoModifier,popup->actionGeometry(popup->actions()[0]).center());});QContextMenuEvent context(QContextMenuEvent::Mouse,point,tree->viewport()->mapToGlobal(point));QApplication::sendEvent(tree->viewport(),&context);QTRY_COMPARE(session(w)["command"].toString(),QString("inspect.section"));dock=w.findChild<QDockWidget*>("sectionDock");auto* hatch=dock->findChild<QCheckBox*>("sectionHatch");QVERIFY(hatch->isChecked());QTest::mouseClick(hatch,Qt::LeftButton,Qt::NoModifier,QPoint(8,hatch->height()/2));QVERIFY(!session(w)["inputs"].toObject()["hatch"].toBool());capture(w,"native-edit-plain");QTest::mouseClick(dock->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok),Qt::LeftButton);QVERIFY(session(w).isEmpty());const auto edited=call(w,"document.inspect");call(w,"history.undo");QCOMPARE(call(w,"document.inspect")["authored"],accepted["authored"]);call(w,"history.redo");QCOMPARE(call(w,"document.inspect")["authored"],edited["authored"]);
  // Actual checkbox hit in the Browser; no activation/focus repair after setup.
  const auto rect=tree->visualItemRect(row);QTest::mouseClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,QPoint(rect.left()+8,rect.center().y()));QTRY_VERIFY(!call(w,"analysis.inspect")["analyses"].toArray()[0].toObject()["visible"].toBool());capture(w,"native-hidden");QTest::mouseClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,QPoint(rect.left()+8,rect.center().y()));QTRY_VERIFY(call(w,"analysis.inspect")["analyses"].toArray()[0].toObject()["visible"].toBool());capture(w,"native-shown");
 }
};
QTEST_MAIN(SectionControlsTest)
#include "section_analysis_controls.moc"
