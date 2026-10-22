// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QComboBox>
#include <QCryptographicHash>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QLabel>
#include <QPushButton>
#include <QProcess>
#include <QTest>
#include <QTimer>
#include <QWindow>
class ResponsiveStlImportControls final:public QObject {
 Q_OBJECT
 QJsonArray transcript_;
 QJsonObject call(AlloyWindow& w,const QString& id,const QJsonObject& args={}) {
  auto r=w.execute(id,args);transcript_.append(QJsonObject{{"command",id},{"response",r}});
  const auto initial=r.value("result").toObject();QElapsedTimer pending;pending.start();
  while(r.value("completion")=="pending") {if(pending.elapsed()>30000)qFatal("pending command timeout");QTest::qWait(25);r=w.execute("command.status",{{"session",initial.value("id")},{"generation",initial.value("previewGeneration")}});}
  if(!r.value("ok").toBool())qFatal("%s",QJsonDocument(r).toJson().constData());return r.value("result").toObject();
 }
 void click(AlloyWindow& w,const QString& name) {
  auto* target=w.findChild<QPushButton*>(name);QVERIFY(target);QVERIFY(target->isEnabled());
  const auto global=target->mapToGlobal(target->rect().center());auto* hit=QApplication::widgetAt(global);QVERIFY(hit);QVERIFY(hit==target || target->isAncestorOf(hit));
  QTest::mouseClick(hit,Qt::LeftButton,Qt::NoModifier,hit->mapFromGlobal(global));
 }
private slots:
 void responsiveLoad() {
  const auto output=qEnvironmentVariable("ALLOY_MESH_EVIDENCE"),source=qEnvironmentVariable("ALLOY_MESH_CAPACITY"),fixtures=qEnvironmentVariable("ALLOY_MESH_FIXTURES");
  QVERIFY(!output.isEmpty());QVERIFY(QFile::exists(source));QVERIFY(!fixtures.isEmpty());QDir().mkpath(output);
  const auto ownedSource=output+"/capacity-200000.stl";QFile::remove(ownedSource);QVERIFY(QFile::copy(source,ownedSource));
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1280,900);w.show();w.raise();w.activateWindow();w.windowHandle()->requestActivate();QVERIFY(QTest::qWaitForWindowExposed(&w));
#ifdef Q_OS_MACOS
  QProcess activation;activation.start("osascript",{"-e",QString("tell application \"System Events\" to set frontmost of first application process whose unix id is %1 to true").arg(QCoreApplication::applicationPid()),"-e","tell application \"System Events\" to get unix id of first application process whose frontmost is true"});
  QVERIFY(activation.waitForFinished(5000));QCOMPARE(activation.exitCode(),0);QCOMPARE(activation.readAllStandardOutput().trimmed().toLongLong(),QCoreApplication::applicationPid());
#endif
  QTRY_VERIFY(w.isActiveWindow());
  ReferenceView* v=nullptr;for(auto* widget:w.findChildren<QWidget*>())if(auto* view=dynamic_cast<ReferenceView*>(widget))v=view;QVERIFY(v);
  auto state=[&]{return w.state().value("meshImport").toObject();};
  auto waitReady=[&]{QTRY_COMPARE_WITH_TIMEOUT(state().value("status").toString(),QString("ready"),30000);};
  QJsonObject report;QElapsedTimer clock;clock.start();qint64 last=clock.elapsed(),maxGap=0;int ticks=0;
  QTimer timer;timer.setInterval(5);connect(&timer,&QTimer::timeout,&w,[&]{const auto now=clock.elapsed();maxGap=std::max(maxGap,now-last);last=now;++ticks;});timer.start();
  auto reset=[&]{last=clock.elapsed();maxGap=0;ticks=0;};
  const auto before=call(w,"document.inspect");const auto selection=w.state().value("selection");
  call(w,"document.save",{{"path",output+"/before.alloy"}});QFile beforeFile(output+"/before.alloy");QVERIFY(beforeFile.open(QIODevice::ReadOnly));const auto saved=beforeFile.readAll();beforeFile.close();
  reset();const auto submitted=call(w,"mesh.importPreview",{{"path",ownedSource}});QCOMPARE(submitted.value("status").toString(),QString("pending"));QCOMPARE(state().value("status").toString(),QString("pending"));
  QTRY_VERIFY(w.findChild<QPushButton*>("mesh.import.cancel")->isVisible());QTest::qWait(30);QCOMPARE(state().value("status").toString(),QString("pending"));
  auto* loadingLabel=w.findChild<QLabel*>("mesh.import.summary");QVERIFY(loadingLabel);QVERIFY(loadingLabel->height()>=loadingLabel->heightForWidth(loadingLabel->width()));
  call(w,"capture.screenshot",{{"path",output+"/loading.png"}});QVERIFY(v->framebuffer().save(output+"/loading-gpu.png"));
  click(w,"mesh.import.cancel");QVERIFY(state().isEmpty());QCOMPARE(w.state().value("meshImportOperation").toObject().value("status").toString(),QString("cancelled"));QCOMPARE(call(w,"document.inspect").value("authored"),before.value("authored"));QCOMPARE(w.state().value("selection"),selection);
  report["pendingCancelGapMs"]=double(maxGap);QCOMPARE(call(w,"document.inspect").value("meshes").toArray().size(),0);
  // The draining job holds the only worker lease. Poll through its explicit
  // refusal until it exits; no second preparation may overlap it.
  QJsonObject start;QElapsedTimer drain;drain.start();do {start=w.execute("mesh.importPreview",{{"path",ownedSource}});if(!start.value("ok").toBool()){QCOMPARE(start.value("error").toObject().value("code").toString(),QString("import_busy"));QTest::qWait(20);}}while(!start.value("ok").toBool() && drain.elapsed()<10000);QVERIFY(start.value("ok").toBool());
  call(w,"document.tabNew");QVERIFY(state().isEmpty());const auto newDoc=call(w,"document.inspect");QTest::qWait(350);QVERIFY(state().isEmpty());QCOMPARE(call(w,"document.inspect"),newDoc);
  // Return to original design after the stale result has safely drained.
  const auto tabs=call(w,"document.tabs").value("tabs").toArray();QCOMPARE(tabs.size(),2);call(w,"document.tabActivate",{{"tabId",tabs.first().toObject().value("tabId")}});
  do {start=w.execute("mesh.importPreview",{{"path",ownedSource}});if(!start.value("ok").toBool())QTest::qWait(20);}while(!start.value("ok").toBool() && drain.elapsed()<10000);QVERIFY(start.value("ok").toBool());
  reset();waitReady();if(QTest::currentTestFailed())return;QTest::qWait(20);report["loadHeartbeatMaxGapMs"]=double(maxGap);report["loadHeartbeatTicks"]=ticks;report["load"]=state();QCOMPARE(state().value("phase").toString(),QString("prepared"));QVERIFY(maxGap<2000);QVERIFY(ticks>5);
  QVERIFY(QFile::remove(ownedSource)); // Later units and OK must use owned preparation.
  call(w,"capture.screenshot",{{"path",output+"/ready.png"}});QVERIFY(v->framebuffer().save(output+"/ready-gpu.png"));
  // Deliver the next key to the focus produced by publication, without repair.
  auto* units=w.findChild<QComboBox*>("mesh.import.units");QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(units));
  reset();QTest::keyClick(units,Qt::Key_Down);QCOMPARE(state().value("status").toString(),QString("pending"));waitReady();if(QTest::currentTestFailed())return;QTest::qWait(20);report["inch"]=state();report["inchHeartbeatMaxGapMs"]=double(maxGap);QVERIFY(maxGap<2000);
  QCOMPARE(state().value("preview").toObject().value("boundsMm").toObject().value("max").toArray(),(QJsonArray{499.*25.4,1599.*25.4,0}));
  call(w,"window.resize",{{"width",1360},{"height",840}});QTest::qWait(30);call(w,"capture.screenshot",{{"path",output+"/inch-resized.png"}});QVERIFY(v->framebuffer().save(output+"/inch-resized-gpu.png"));
  reset();QElapsedTimer commit;commit.start();click(w,"mesh.import.ok");QTRY_VERIFY(state().isEmpty());report["actualHitCommitMs"]=double(commit.elapsed());QTest::qWait(25);report["commitHeartbeatMaxGapMs"]=double(maxGap);report["commit"]=w.state().value("meshImportOperation");QVERIFY(maxGap<2000);
  const auto imported=call(w,"document.inspect");QCOMPARE(imported.value("meshes").toArray().size(),1);const auto id=imported.value("meshes").toArray().first().toObject().value("id").toString();
  call(w,"document.save",{{"path",output+"/imported-inch.alloy"}});call(w,"history.undo");QCOMPARE(call(w,"document.inspect").value("authored"),before.value("authored"));call(w,"history.redo");QCOMPARE(call(w,"document.inspect").value("meshes"),imported.value("meshes"));
  call(w,"document.new",{{"discard",true}});call(w,"document.open",{{"path",output+"/imported-inch.alloy"},{"discard",true}});QCOMPARE(call(w,"document.inspect").value("meshes"),imported.value("meshes"));
  call(w,"document.save",{{"path",output+"/reopened-inch.alloy"}});
  auto hashFile=[](const QString& path){QFile f(path);if(!f.open(QIODevice::ReadOnly))return QByteArray{};QCryptographicHash hash(QCryptographicHash::Sha256);if(!hash.addData(&f))return QByteArray{};return hash.result().toHex();};
  const auto originalHash=hashFile(output+"/imported-inch.alloy");QVERIFY(!originalHash.isEmpty());QCOMPARE(hashFile(output+"/reopened-inch.alloy"),originalHash);report["exactReopenSha256"]=QString::fromLatin1(originalHash);QVERIFY(QFile::remove(output+"/reopened-inch.alloy"));
  call(w,"document.new",{{"discard",true}});
  for(const auto& name:{QString("truncated_binary.stl"),QString("capacity-200001.stl")}) {
   const auto path=name.startsWith("capacity") ? QFileInfo(source).dir().filePath(name) : fixtures+"/"+name;
   call(w,"mesh.importPreview",{{"path",path}});QTRY_COMPARE_WITH_TIMEOUT(state().value("status").toString(),QString("failed"),10000);QVERIFY(!state().value("error").toObject().value("remedy").toString().isEmpty());QCOMPARE(call(w,"document.inspect").value("meshes").toArray().size(),0);click(w,"mesh.import.cancel");QCOMPARE(w.state().value("meshImportOperation").toObject().value("status").toString(),QString("cancelled"));
  }
  QVERIFY(beforeFile.open(QIODevice::ReadOnly));QCOMPARE(beforeFile.readAll(),saved);
  report["gpu"]=w.state().value("viewport").toObject().value("gpu");report["transcript"]=transcript_;report["passed"]=true;
  QFile file(output+"/report.json");QVERIFY(file.open(QIODevice::WriteOnly));file.write(QJsonDocument(report).toJson());w.hide();
 }
};
QTEST_MAIN(ResponsiveStlImportControls)
#include "responsive_stl_import_controls.moc"
