// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QTest>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QWindow>
#include <QToolButton>
#include <QMenu>
#include <QComboBox>
#include <QPushButton>
#include <QFileDialog>
#include <QProcess>
#include <QMessageBox>
#include <QTimer>
#include <QLabel>
#ifdef Q_OS_MACOS
#include <dlfcn.h>
#endif
class NativeStlImportControls final:public QObject {
 Q_OBJECT
 QJsonArray transcript_,steps_;
 QJsonObject native(AlloyWindow& w){
  QJsonObject result;
#ifdef Q_OS_MACOS
  const auto selector=reinterpret_cast<void*(*)(const char*)>(dlsym(RTLD_DEFAULT,"sel_registerName"));
  const auto send=reinterpret_cast<void*(*)(void*,void*)>(dlsym(RTLD_DEFAULT,"objc_msgSend"));
  const auto boolean=reinterpret_cast<bool(*)(void*,void*)>(dlsym(RTLD_DEFAULT,"objc_msgSend"));
  const auto cls=reinterpret_cast<void*(*)(const char*)>(dlsym(RTLD_DEFAULT,"objc_getClass"));
  const auto app=send(cls("NSApplication"),selector("sharedApplication"));
  const auto window=send(reinterpret_cast<void*>(w.winId()),selector("window"));
  const auto className=reinterpret_cast<const char*(*)(void*)>(dlsym(RTLD_DEFAULT,"object_getClassName"));
  result.insert("keyWindowClass",QString::fromUtf8(className(send(app,selector("keyWindow")))));
  result.insert("applicationActive",boolean(app,selector("isActive")));
  result.insert("keyWindowEditor",send(app,selector("keyWindow"))==window);
#endif
  return result;
 }
 QJsonObject call(AlloyWindow& w,const QString& id,const QJsonObject& args={}){
  const auto r=w.execute(id,args);transcript_.append(QJsonObject{{"command",id},{"args",args},{"response",r}});
  if(!r.value("ok").toBool())qFatal("%s",QJsonDocument(r).toJson().constData());return r.value("result").toObject();
 }
 void click(QWidget* surface,const QPoint& point){
  auto* hit=QApplication::widgetAt(surface->mapToGlobal(point));QVERIFY(hit);QVERIFY(hit==surface || surface->isAncestorOf(hit));
  QTest::mouseClick(hit,Qt::LeftButton,Qt::NoModifier,hit->mapFromGlobal(surface->mapToGlobal(point)));
 }
private slots:
 void readyPanel(){
  const auto output=qEnvironmentVariable("ALLOY_MESH_EVIDENCE"),fixtures=qEnvironmentVariable("ALLOY_MESH_FIXTURES");QVERIFY(!output.isEmpty());QVERIFY(!fixtures.isEmpty());QDir().mkpath(output);
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1280,900);w.show();w.raise();w.activateWindow();w.windowHandle()->requestActivate();
  QVERIFY(QTest::qWaitForWindowExposed(&w));
  QProcess activation;activation.start("osascript",{"-e",QString("tell application \"System Events\" to set frontmost of first application process whose unix id is %1 to true").arg(QCoreApplication::applicationPid())});QVERIFY(activation.waitForFinished(5000));QCOMPARE(activation.exitCode(),0);
  QTRY_VERIFY(native(w).value("applicationActive").toBool(true));QTRY_VERIFY(w.isActiveWindow());QTRY_VERIFY(native(w).value("keyWindowEditor").toBool(true));
  ReferenceView* v=nullptr;for(auto* widget:w.findChildren<QWidget*>())if(auto* view=dynamic_cast<ReferenceView*>(widget))v=view;QVERIFY(v);
  call(w,"solid.box",{{"width",8},{"depth",9},{"height",10},{"x",30},{"units","mm"}});
  const auto body=call(w,"document.inspect").value("evaluation").toObject().value("bodies").toArray().first().toObject().value("bodyId").toString();call(w,"selection.set",{{"bodyId",body}});
  call(w,"document.save",{{"path",output+"/before.alloy"}});const auto before=call(w,"document.inspect"),selection=w.state().value("selection").toObject();
  auto disk=[&]{QFile f(output+"/before.alloy");if(!f.open(QIODevice::ReadOnly))return QByteArray{};return f.readAll();};const auto savedBytes=disk();QVERIFY(!savedBytes.isEmpty());
  auto step=[&](const QString& name){steps_.append(QJsonObject{{"step",name},{"native",native(w)},{"state",w.state()}});QFile partial(output+"/steps.json");if(partial.open(QIODevice::WriteOnly))partial.write(QJsonDocument(steps_).toJson());};
  auto choose=[&](const QString& name){call(w,"mesh.importPreview",{{"path",fixtures+"/"+name}});QTRY_VERIFY(w.findChild<QComboBox*>("mesh.import.units"));QTRY_VERIFY(native(w).value("keyWindowEditor").toBool(true));QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(w.findChild<QComboBox*>("mesh.import.units")));step("API ready preview focused units");};
  choose("ascii_box_10mm.stl");if(QTest::currentTestFailed())return;QCOMPARE(call(w,"document.inspect"),before);QCOMPARE(w.state().value("selection").toObject(),selection);
  auto* units=w.findChild<QComboBox*>("mesh.import.units");
  call(w,"mesh.importUnits",{{"units","in"}});QCOMPARE(units->currentData().toString(),QString("in"));QVERIFY(w.findChild<QPushButton*>("mesh.import.ok")->isEnabled());
  const auto invalidUnits=w.execute("mesh.importUnits",{{"units","unsupported"}});QVERIFY(!invalidUnits.value("ok").toBool());QVERIFY(!w.findChild<QPushButton*>("mesh.import.ok")->isEnabled());
  call(w,"mesh.importUnits",{{"units","mm"}});QCOMPARE(units->currentData().toString(),QString("mm"));QVERIFY(w.findChild<QPushButton*>("mesh.import.ok")->isEnabled());
  QTest::keyClick(units,Qt::Key_Down);QCOMPARE(units->currentData().toString(),QString("in"));QCOMPARE(w.state().value("meshImport").toObject().value("preview").toObject().value("boundsMm").toObject().value("max").toArray(),(QJsonArray{254,254,254}));step("native inch preview");
  auto* summaryLabel=w.findChild<QLabel*>("mesh.import.summary");QVERIFY(summaryLabel);QTRY_VERIFY(summaryLabel->height()>=summaryLabel->heightForWidth(summaryLabel->width()));
  call(w,"capture.screenshot",{{"path",output+"/preview-inch.png"}});QVERIFY(v->framebuffer().save(output+"/preview-inch-gpu.png"));
  auto press=[&](const QString& id){auto* button=w.findChild<QPushButton*>(id);QVERIFY(button);click(button,button->rect().center());};
  press("mesh.import.cancel");QTRY_VERIFY(!w.findChild<QComboBox*>("mesh.import.units"));QCOMPARE(call(w,"document.inspect"),before);QCOMPARE(w.state().value("selection").toObject(),selection);QTRY_VERIFY(native(w).value("keyWindowEditor").toBool(true));QCOMPARE(disk(),savedBytes);step("preview Cancel");
  choose("ascii_box_10mm.stl");if(QTest::currentTestFailed())return;press("mesh.import.ok");QTRY_VERIFY(!w.findChild<QComboBox*>("mesh.import.units"));QCOMPARE(call(w,"document.inspect").value("meshes").toArray().size(),1);QCOMPARE(w.state().value("selection").toObject().value("kind").toString(),QString("mesh"));step("OK selects committed mesh");
  auto* tree=w.findChild<QTreeWidget*>();QVERIFY(tree);
  QTreeWidgetItem* named=nullptr;for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->text(0)=="Named Views")named=*it;QVERIFY(named);
  if(named->isExpanded()){const auto rect=tree->visualItemRect(named);click(tree->viewport(),{rect.left()-10,rect.center().y()});QTRY_VERIFY(!named->isExpanded());}
  call(w,"capture.screenshot",{{"path",output+"/committed.png"}});QVERIFY(v->framebuffer().save(output+"/committed-gpu.png"));
  const auto committedId=w.state().value("selection").toObject().value("meshId").toString();QTreeWidgetItem* meshRow=nullptr;for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole+14)==committedId)meshRow=*it;QVERIFY(meshRow);const auto meshRect=tree->visualItemRect(meshRow);QVERIFY(!meshRect.isEmpty());click(tree->viewport(),{meshRect.left()+8,meshRect.center().y()});QTRY_COMPARE(meshRow->checkState(0),Qt::Unchecked);QTRY_VERIFY(!w.state().value("viewport").toObject().value("gpu").toObject().value("meshes").toObject().value("bodies").toArray().first().toObject().value("displayed").toBool());step("Browser hide before open-sheet preview");
  choose("open_square.stl");if(QTest::currentTestFailed())return;call(w,"window.resize",{{"width",1360},{"height",840}});QTest::qWait(60);step("open preview resized");call(w,"capture.screenshot",{{"path",output+"/open-resized.png"}});QVERIFY(v->framebuffer().save(output+"/open-resized-gpu.png"));press("mesh.import.ok");QTRY_COMPARE(call(w,"document.inspect").value("meshes").toArray().size(),2);
  const auto beforeInvalid=call(w,"document.inspect"),selectedInvalid=w.state().value("selection").toObject();
  const auto refused=w.execute("mesh.importPreview",{{"path",fixtures+"/truncated_binary.stl"}});QVERIFY(!refused.value("ok").toBool());QCOMPARE(refused.value("error").toObject().value("code").toString(),QString("mesh_import_failed"));QCOMPARE(call(w,"document.inspect"),beforeInvalid);QCOMPARE(w.state().value("selection").toObject(),selectedInvalid);QCOMPARE(disk(),savedBytes);step("API invalid atomicity");
  for(const auto& value:call(w,"document.inspect").value("meshes").toArray())call(w,"mesh.inspect",{{"meshId",value.toObject().value("id")}});
  call(w,"document.save",{{"path",output+"/native-imported.alloy"}});
  const auto gpu=w.state().value("viewport").toObject().value("gpu").toObject();QVERIFY(gpu.value("hardwareAccelerated").toBool());
  QFile report(output+"/report.json");QVERIFY(report.open(QIODevice::WriteOnly));report.write(QJsonDocument(QJsonObject{{"passed",true},{"gpu",gpu},{"steps",steps_},{"transcript",transcript_}}).toJson());w.hide();
 }
 void chooserRouting(){
  const auto output=qEnvironmentVariable("ALLOY_MESH_EVIDENCE"),fixtures=qEnvironmentVariable("ALLOY_MESH_FIXTURES");QVERIFY(!output.isEmpty());QVERIFY(!fixtures.isEmpty());QDir().mkpath(output);
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1280,900);w.show();w.raise();w.activateWindow();w.windowHandle()->requestActivate();
  QVERIFY(QTest::qWaitForWindowExposed(&w));
  QProcess activation;activation.start("osascript",{"-e",QString("tell application \"System Events\" to set frontmost of first application process whose unix id is %1 to true").arg(QCoreApplication::applicationPid())});QVERIFY(activation.waitForFinished(5000));QCOMPARE(activation.exitCode(),0);
  QTRY_VERIFY(native(w).value("applicationActive").toBool(true));QTRY_VERIFY(w.isActiveWindow());QTRY_VERIFY(native(w).value("keyWindowEditor").toBool(true));
  ReferenceView* v=nullptr;for(auto* widget:w.findChildren<QWidget*>())if(auto* view=dynamic_cast<ReferenceView*>(widget))v=view;QVERIFY(v);
  call(w,"solid.box",{{"width",8},{"depth",9},{"height",10},{"x",30},{"units","mm"}});
  const auto body=call(w,"document.inspect").value("evaluation").toObject().value("bodies").toArray().first().toObject().value("bodyId").toString();call(w,"selection.set",{{"bodyId",body}});
  call(w,"document.save",{{"path",output+"/before.alloy"}});const auto before=call(w,"document.inspect"),selection=w.state().value("selection").toObject();
  auto disk=[&]{QFile f(output+"/before.alloy");if(!f.open(QIODevice::ReadOnly))return QByteArray{};return f.readAll();};const auto savedBytes=disk();QVERIFY(!savedBytes.isEmpty());
  auto step=[&](const QString& name){steps_.append(QJsonObject{{"step",name},{"native",native(w)},{"state",w.state()}});QFile partial(output+"/steps.json");if(partial.open(QIODevice::WriteOnly))partial.write(QJsonDocument(steps_).toJson());};
  auto entry=[&]{
   QPushButton* file=nullptr;for(auto* b:w.findChildren<QPushButton*>())if(b->text()=="File")file=b;QVERIFY(file);
   QTimer::singleShot(0,&w,[&]{QMenu* menu=nullptr;QTRY_VERIFY((menu=qobject_cast<QMenu*>(QApplication::activePopupWidget())));QAction* action=nullptr;for(auto* a:menu->actions())if(a->text()=="Import Mesh…")action=a;QVERIFY(action);const auto rect=menu->actionGeometry(action);QVERIFY(rect.isValid());click(menu,rect.center());});click(file,file->rect().center());
   QTRY_VERIFY(w.findChild<QFileDialog*>("mesh.import.chooser"));QVERIFY(!w.findChild<QFileDialog*>("mesh.import.chooser")->testOption(QFileDialog::DontUseNativeDialog));
   QTRY_VERIFY(!native(w).value("keyWindowEditor").toBool(true));step("native chooser active");
  };
  auto keys=[&](const QString& script){QProcess process;process.start("osascript",{"-e",QString("tell application \"System Events\"\nif unix id of first application process whose frontmost is true is not %1 then error \"Private fixture is not frontmost; input aborted\"\ntell first application process whose unix id is %1\n%2\nend tell\nend tell").arg(QCoreApplication::applicationPid()).arg(script)});QTRY_VERIFY_WITH_TIMEOUT(process.state()==QProcess::NotRunning,7000);if(process.exitCode()!=0){qWarning().noquote()<<process.readAllStandardError();QFAIL("Private fixture is not frontmost; OS input aborted");}};
  entry();if(QTest::currentTestFailed())return;keys("key code 53");if(QTest::currentTestFailed())return;QTRY_VERIFY(!w.findChild<QFileDialog*>("mesh.import.chooser"));QTRY_VERIFY(native(w).value("keyWindowEditor").toBool(true));QCOMPARE(call(w,"document.inspect"),before);QCOMPARE(w.state().value("selection").toObject(),selection);QCOMPARE(disk(),savedBytes);step("chooser Cancel returned naturally");
  QSKIP("XCH-022: PID-targeted native source selection and Go To readiness remain unqualified; no further OS key is sent.");
 }
};
QTEST_MAIN(NativeStlImportControls)
#include "native_stl_import_controls.moc"
