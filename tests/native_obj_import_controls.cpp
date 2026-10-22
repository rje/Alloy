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
class NativeObjImportControls final:public QObject {
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
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1280,900);w.show();w.raise();w.activateWindow();w.windowHandle()->requestActivate();QVERIFY(QTest::qWaitForWindowExposed(&w));
  QProcess activation;activation.start("osascript",{"-e",QString("tell application \"System Events\" to set frontmost of first application process whose unix id is %1 to true").arg(QCoreApplication::applicationPid())});QVERIFY(activation.waitForFinished(5000));QCOMPARE(activation.exitCode(),0);
  QTRY_VERIFY(native(w).value("applicationActive").toBool(true));QTRY_VERIFY(w.isActiveWindow());QTRY_VERIFY(native(w).value("keyWindowEditor").toBool(true));
  ReferenceView* v=nullptr;for(auto* widget:w.findChildren<QWidget*>())if(auto* view=dynamic_cast<ReferenceView*>(widget))v=view;QVERIFY(v);
  auto state=[&]{return w.state().value("meshImport").toObject();};
  auto press=[&](const QString& id){auto* button=w.findChild<QPushButton*>(id);QVERIFY(button);click(button,button->rect().center());};
  auto choose=[&](const QString& name){call(w,"mesh.importPreview",{{"path",fixtures+"/"+name}});QTRY_COMPARE_WITH_TIMEOUT(state().value("status").toString(),QString("ready"),30000);QTRY_VERIFY(native(w).value("keyWindowEditor").toBool(true));QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(w.findChild<QComboBox*>("mesh.import.units")));};
  call(w,"solid.box",{{"width",8},{"depth",9},{"height",10},{"x",30},{"units","mm"}});call(w,"document.save",{{"path",output+"/before.alloy"}});
  const auto before=call(w,"document.inspect");const auto selection=w.state().value("selection");QFile beforeFile(output+"/before.alloy");QVERIFY(beforeFile.open(QIODevice::ReadOnly));const auto bytes=beforeFile.readAll();beforeFile.close();
  choose("groups.obj");if(QTest::currentTestFailed())return;auto* units=w.findChild<QComboBox*>("mesh.import.units");QTest::keyClick(units,Qt::Key_Down);QTRY_COMPARE_WITH_TIMEOUT(state().value("status").toString(),QString("ready"),30000);QCOMPARE(units->currentData().toString(),QString("in"));QCOMPARE(state().value("preview").toObject().value("boundsMm").toObject().value("max").toArray(),(QJsonArray{381,127,127}));
  auto* summary=w.findChild<QLabel*>("mesh.import.summary");QVERIFY(summary->text().contains("Ignored records:"));QVERIFY(summary->text().contains("Source labels: 4"));QVERIFY(summary->text().contains("1 more retained"));QTRY_VERIFY(summary->height()>=summary->heightForWidth(summary->width()));QCOMPARE(summary->textFormat(),Qt::PlainText);
  call(w,"capture.screenshot",{{"path",output+"/preview-inch.png"}});QVERIFY(v->framebuffer().save(output+"/preview-inch-gpu.png"));press("mesh.import.cancel");QTRY_VERIFY(state().isEmpty());QCOMPARE(call(w,"document.inspect"),before);QCOMPARE(w.state().value("selection"),selection);QVERIFY(beforeFile.open(QIODevice::ReadOnly));QCOMPARE(beforeFile.readAll(),bytes);beforeFile.close();
  call(w,"document.save",{{"path",output+"/cancelled.alloy"}});const auto gpu=w.state().value("viewport").toObject().value("gpu").toObject();QVERIFY(gpu.value("hardwareAccelerated").toBool());
  QFile report(output+"/report.json");QVERIFY(report.open(QIODevice::WriteOnly));report.write(QJsonDocument(QJsonObject{{"passed",true},{"gpu",gpu},{"native",native(w)},{"transcript",transcript_}}).toJson());w.hide();
 }
 void commitPanel(){
  const auto output=qEnvironmentVariable("ALLOY_MESH_EVIDENCE")+"/commit",fixtures=qEnvironmentVariable("ALLOY_MESH_FIXTURES");QDir().mkpath(output);
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1280,900);w.show();w.raise();w.activateWindow();w.windowHandle()->requestActivate();QVERIFY(QTest::qWaitForWindowExposed(&w));
  QProcess activation;activation.start("osascript",{"-e",QString("tell application \"System Events\" to set frontmost of first application process whose unix id is %1 to true").arg(QCoreApplication::applicationPid())});QVERIFY(activation.waitForFinished(5000));QCOMPARE(activation.exitCode(),0);
  QTRY_VERIFY(native(w).value("applicationActive").toBool(true));QTRY_VERIFY(w.isActiveWindow());QTRY_VERIFY(native(w).value("keyWindowEditor").toBool(true));
  ReferenceView* v=nullptr;for(auto* widget:w.findChildren<QWidget*>())if(auto* view=dynamic_cast<ReferenceView*>(widget))v=view;QVERIFY(v);
  auto* tree=w.findChild<QTreeWidget*>();QVERIFY(tree);
  for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->text(0)=="Named Views" && (*it)->isExpanded()) {const auto r=tree->visualItemRect(*it);click(tree->viewport(),{r.left()-10,r.center().y()});QTRY_VERIFY(!(*it)->isExpanded());}
  QVERIFY(native(w).value("keyWindowEditor").toBool(true));
  call(w,"mesh.importPreview",{{"path",fixtures+"/groups.obj"}});QTRY_COMPARE_WITH_TIMEOUT(w.state().value("meshImport").toObject().value("status").toString(),QString("ready"),30000);QTRY_VERIFY(native(w).value("keyWindowEditor").toBool(true));
  auto* units=w.findChild<QComboBox*>("mesh.import.units");QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(units));
  const auto preview=w.state().value("meshImport").toObject().value("preview").toObject();auto* ok=w.findChild<QPushButton*>("mesh.import.ok");QVERIFY(ok);click(ok,ok->rect().center());QTRY_VERIFY(w.state().value("meshImport").toObject().isEmpty());
  const auto id=preview.value("id");QCOMPARE(call(w,"mesh.inspect",{{"meshId",id}}).value("mesh"),preview);QCOMPARE(w.state().value("selection").toObject().value("meshId"),id);
  call(w,"history.undo");QVERIFY(call(w,"document.inspect").value("meshes").toArray().isEmpty());call(w,"history.redo");QCOMPARE(call(w,"mesh.inspect",{{"meshId",id}}).value("mesh"),preview);
  call(w,"view.fit");call(w,"window.resize",{{"width",1360},{"height",840}});QTest::qWait(60);call(w,"capture.screenshot",{{"path",output+"/committed-resized.png"}});QVERIFY(v->framebuffer().save(output+"/committed-resized-gpu.png"));
  call(w,"document.save",{{"path",output+"/native-imported.alloy"}});const auto gpu=w.state().value("viewport").toObject().value("gpu").toObject();QVERIFY(gpu.value("hardwareAccelerated").toBool());
  QFile report(output+"/report.json");QVERIFY(report.open(QIODevice::WriteOnly));report.write(QJsonDocument(QJsonObject{{"passed",true},{"gpu",gpu},{"native",native(w)},{"transcript",transcript_}}).toJson());w.hide();
 }

};
QTEST_MAIN(NativeObjImportControls)
#include "native_obj_import_controls.moc"
