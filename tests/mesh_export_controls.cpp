// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include <QApplication>
#include <QTest>
#include <QTemporaryDir>
#include <QFile>
#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QCryptographicHash>
#include <QTimer>
#include <QJsonDocument>
#include <QContextMenuEvent>
#include <QMenu>
#include <QPointer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>

class MeshExportControlsTest final:public QObject {
 Q_OBJECT
 QJsonObject call(AlloyWindow& w,const QString& command,const QJsonObject& args={}){
  const auto result=w.execute(command,args);if(!result.value("ok").toBool())qFatal("%s",QJsonDocument(result).toJson().constData());return result.value("result").toObject();
 }
 QJsonObject start(AlloyWindow& w,const QString& path){
  const auto response=w.execute("document.exportStl",{{"path",path},{"allVisible",true},{"units","mm"},{"format","binary"},{"surfaceDeviationMm",.1},{"normalDeviationDeg",10}});
  if(response.value("completion")!="pending")qFatal("Expected pending: %s",QJsonDocument(response).toJson().constData());
  w.geometryEvaluationPoll_->stop();return response.value("result").toObject();
 }
 QJsonObject status(AlloyWindow& w,const QJsonObject& op){return w.execute("command.status",{{"session",op.value("id")},{"generation",op.value("previewGeneration")}});}
 QByteArray bytes(const QString& path){QFile f(path);if(!f.open(QIODevice::ReadOnly))return {};return f.readAll();}
 void sentinel(const QString& path){QFile f(path);QVERIFY(f.open(QIODevice::WriteOnly));QCOMPARE(f.write("preserve"),8);}
 void waitForApproval(const std::shared_ptr<AlloyWindow::GeometryEvaluation>& task){
  QTRY_VERIFY_WITH_TIMEOUT(task->exportPublication->state()==MeshExport::PublicationGate::State::AwaitingApproval,10000);
 }
 void drain(AlloyWindow& w){QTRY_VERIFY_WITH_TIMEOUT(w.geometryEvaluationRunning_->ready.load(std::memory_order_acquire),10000);w.pollGeometryEvaluation();QVERIFY(!w.geometryEvaluationRunning_);}
private slots:
 void preparedPreviewLifecycle(){
  QTemporaryDir dir;QVERIFY(dir.isValid());AlloyWindow w;
  call(w,"solid.box",{{"width",20},{"depth",10},{"height",15},{"units","mm"}});
  const auto before=call(w,"document.inspect");const auto id=before.value("evaluation").toObject().value("bodies").toArray().first().toObject().value("bodyId").toString();
  QJsonObject inputs{{"bodyId",id},{"format","binary"},{"units","mm"},{"preset","Medium"},{"surfaceDeviationMm",.1},{"normalDeviationDeg",10}};
  auto op=call(w,"document.previewMesh",inputs);QVERIFY(!op.contains("triangles"));
  QTRY_COMPARE_WITH_TIMEOUT(call(w,"document.meshPreview").value("evaluationStatus").toString(),QString("completed"),15000);
  const auto prepared=call(w,"document.meshPreview",{{"includeBytes",true}});const auto data=QByteArray::fromBase64(prepared.value("bytesBase64").toString().toLatin1());
  QCOMPARE(prepared.value("triangles").toInt(),12);QCOMPARE(QString::fromLatin1(QCryptographicHash::hash(data,QCryptographicHash::Sha256).toHex()),prepared.value("sha256").toString());
  QCOMPARE(call(w,"document.inspect"),before);
  QVERIFY(!w.execute("document.saveMeshPreview",{{"previewId",op.value("previewId")},{"path",dir.filePath("missing/no.stl")}}).value("ok").toBool());
  const auto path=dir.filePath("same.stl");sentinel(path);
  const auto published=call(w,"document.saveMeshPreview",{{"previewId",op.value("previewId")},{"path",path}});qInfo()<<"prepared publication bytes"<<data.size()<<"elapsedMs"<<published.value("publicationMs").toInteger();QCOMPARE(bytes(path),data);QCOMPARE(call(w,"document.inspect"),before);
  call(w,"document.clearMeshPreview",{{"previewId",op.value("previewId")}});QVERIFY(!call(w,"document.meshPreview").contains("triangles"));QCOMPARE(bytes(path),data);
  op=call(w,"document.previewMesh",inputs);const auto running=w.meshPreviewWork_;call(w,"document.clearMeshPreview",{{"previewId",op.value("previewId")}});
  QVERIFY(!call(w,"document.meshPreview").contains("triangles"));QTRY_VERIFY_WITH_TIMEOUT(running->ready.load(std::memory_order_acquire),15000);QVERIFY(!w.meshPreviewWork_);
  op=call(w,"document.previewMesh",inputs);QTRY_COMPARE_WITH_TIMEOUT(call(w,"document.meshPreview").value("evaluationStatus").toString(),QString("completed"),15000);
  call(w,"solid.box",{{"width",2},{"depth",2},{"height",2},{"units","mm"}});QVERIFY(!call(w,"document.meshPreview").contains("triangles"));
  QVERIFY(!w.execute("document.saveMeshPreview",{{"previewId",op.value("previewId")},{"path",dir.filePath("stale.stl")}}).value("ok").toBool());QVERIFY(!QFile::exists(dir.filePath("stale.stl")));
 }
 void refinementSheetPreview(){
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1400,900);w.show();QVERIFY(QTest::qWaitForWindowExposed(&w));
  call(w,"solid.box",{{"width",20},{"depth",10},{"height",15},{"units","mm"}});const auto before=call(w,"document.inspect");
  call(w,"document.exportMeshDialog");QPointer<QDialog> dialog=w.findChild<QDialog*>("mesh.export.dialog");QVERIFY(dialog);
  auto* refinement=dialog->findChild<QComboBox*>("mesh.export.refinement");auto* surface=dialog->findChild<QDoubleSpinBox*>("mesh.export.surfaceDeviation");auto* normal=dialog->findChild<QDoubleSpinBox*>("mesh.export.normalDeviation");
  QCOMPARE(refinement->currentData().toString(),QString("Medium"));refinement->setCurrentIndex(refinement->findData("High"));QCOMPARE(surface->value(),.02);QCOMPARE(normal->value(),5.);
  QTest::mouseClick(dialog->findChild<QPushButton*>("mesh.export.preview"),Qt::LeftButton);
  QTRY_COMPARE_WITH_TIMEOUT(dialog->findChild<QLabel*>("mesh.export.triangleCount")->text(),QString("Triangle Count: 12"),15000);
  QVERIFY(dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Save)->isEnabled());
  const auto evidence=qEnvironmentVariable("ALLOY_MESH_PREVIEW_CAPTURE");if(!evidence.isEmpty())QVERIFY(w.captureImage().save(evidence));
  surface->setValue(.03);QCOMPARE(refinement->currentData().toString(),QString("Custom"));QCOMPARE(dialog->findChild<QLabel*>("mesh.export.triangleCount")->text(),QString("Triangle Count: —"));QVERIFY(!dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Save)->isEnabled());
  QTest::mouseClick(dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Cancel),Qt::LeftButton);QTRY_VERIFY(!dialog || !dialog->isVisible());QCOMPARE(call(w,"document.inspect"),before);
 }
 void browserContextMeshCancel(){
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1400,900);w.show();QVERIFY(QTest::qWaitForWindowExposed(&w));
  call(w,"solid.box",{{"width",20},{"depth",10},{"height",15},{"units","mm"}});
  const auto before=call(w,"document.inspect");const auto bodyId=before.value("evaluation").toObject().value("bodies").toArray().first().toObject().value("bodyId").toString();
  auto* tree=w.findChild<QTreeWidget*>("browser.tree");QVERIFY(tree);tree->expandAll();
  QTreeWidgetItem* row=nullptr;for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole+1).toString()==bodyId){row=*it;break;}
  QVERIFY(row);tree->scrollToItem(row);QApplication::processEvents();
  const auto point=tree->visualItemRect(row).center();QVERIFY(tree->viewport()->rect().contains(point));
  QContextMenuEvent event(QContextMenuEvent::Mouse,point,tree->viewport()->mapToGlobal(point));
  QApplication::sendEvent(tree->viewport(),&event);
  auto* menu=qobject_cast<QMenu*>(QApplication::activePopupWidget());QVERIFY(menu);QCOMPARE(menu->objectName(),QString("context.browserBody"));
  QAction* mesh=nullptr;for(auto* action:menu->actions())if(action->objectName()=="context.exportMesh")mesh=action;
  QVERIFY(mesh);QVERIFY(mesh->isEnabled());QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,menu->actionGeometry(mesh).center());
  QPointer<QDialog> dialog;QTRY_VERIFY((dialog=w.findChild<QDialog*>("mesh.export.dialog"))&&dialog->isVisible());
  QTRY_VERIFY(QApplication::activePopupWidget()==nullptr);
  QCOMPARE(dialog->findChild<QComboBox*>("mesh.export.body")->currentData().toString(),bodyId);
  auto* format=dialog->findChild<QComboBox*>("mesh.export.format");QVERIFY(format);
  const int threeMf=format->findData("3mf");QVERIFY(threeMf>=0);
  format->setCurrentIndex(threeMf);
  QVERIFY(!dialog->findChild<QComboBox*>("mesh.export.units")->isVisible());
  QVERIFY(dialog->findChild<QLabel*>("mesh.export.note")->text().contains("millimeters"));
  QTest::mouseClick(dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Cancel),Qt::LeftButton);
  QTRY_VERIFY(!dialog||!dialog->isVisible());
  QCOMPARE(call(w,"document.inspect"),before);
 }
 void publicationAndNativeCompletion(){
  QTemporaryDir dir;QVERIFY(dir.isValid());AlloyWindow w;call(w,"solid.box",{{"width",20},{"depth",10},{"height",15},{"units","mm"}});
  const auto before=call(w,"document.inspect");const auto path=dir.filePath("solid.stl");sentinel(path);
  call(w,"document.exportMeshDialog");auto* dialog=w.findChild<QDialog*>("mesh.export.dialog");QVERIFY(dialog);
  auto op=start(w,path);dialog->setProperty("exportOperation",op.value("id").toString());w.refreshMeshExportDialog();
  QVERIFY(!dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Save)->isEnabled());
  QVERIFY(!dialog->findChild<QComboBox*>("mesh.export.format")->isEnabled());
  QVERIFY(!w.execute("history.undo").value("ok").toBool());
  QCOMPARE(w.execute("command.cancel",{{"session",op.value("id")},{"unexpected",true}}).value("error").toObject().value("code").toString(),QString("invalid_arguments"));
  const auto task=w.geometryEvaluationRunning_;waitForApproval(task);QCOMPARE(bytes(path),QByteArray("preserve"));
  QVERIFY(call(w,"command.cancel",{{"session",op.value("id")}}).value("resultDiscarded").toBool());
  drain(w);QCOMPARE(bytes(path),QByteArray("preserve"));QCOMPARE(call(w,"document.inspect"),before);
  QCOMPARE(status(w,op).value("error").toObject().value("code").toString(),QString("export_cancelled"));
  QVERIFY(dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Save)->isEnabled());
  QVERIFY(!dialog->findChild<QLabel*>("mesh.export.status")->text().isEmpty());QVERIFY(dialog->isVisible());
  op=start(w,path);dialog->setProperty("exportOperation",op.value("id").toString());w.refreshMeshExportDialog();
  const auto approved=w.geometryEvaluationRunning_;waitForApproval(approved);w.pollMeshExport();
  // Even if the writer has already committed, no GUI completion has occurred.
  // Cancel must neither claim success nor erase the pending result after approval.
  QCOMPARE(w.execute("command.cancel",{{"session",op.value("id")}}).value("error").toObject().value("code").toString(),QString("export_publication_started"));
  QCOMPARE(status(w,op).value("completion").toString(),QString("pending"));
  QVERIFY(!w.state().value("geometryWork").toObject().value("resultDiscardAvailable").toBool());
  drain(w);const auto result=status(w,op);QCOMPARE(result.value("completion").toString(),QString("completed"));
  const auto metadata=result.value("result").toObject();QCOMPARE(metadata.value("path").toString(),path);QCOMPARE(metadata.value("triangles").toInt(),12);QVERIFY(metadata.value("bytes").toInt()>84);
  QVERIFY(!dialog->isVisible());QCOMPARE(call(w,"document.inspect"),before);
  const auto exported=bytes(path);QVERIFY(exported!="preserve");
  const auto ordinary=MeshExport::writeStl(w.document_->bodies().front(),dir.filePath("ordinary.stl"),"mm",true,.1,10);
  QCOMPARE(bytes(ordinary.value("path").toString()),exported);
  op=start(w,dir.filePath("missing/failed.stl"));drain(w);QCOMPARE(status(w,op).value("error").toObject().value("code").toString(),QString("invalid_path"));QCOMPARE(call(w,"document.inspect"),before);
  // A new session invalidates the previous terminal, without losing an active caller.
  call(w,"command.begin",{{"command","solid.box"},{"inputs",QJsonObject{}}});QCOMPARE(status(w,op).value("error").toObject().value("code").toString(),QString("invalid_session"));
  call(w,"command.cancel",{{"session",w.session_.value("id")}});
 }
 void threeMfCommandCancellation(){
  QTemporaryDir dir;QVERIFY(dir.isValid());AlloyWindow w;
  call(w,"solid.box",{{"width",20},{"depth",10},{"height",5},{"units","mm"}});
  const auto before=call(w,"document.inspect");const auto path=dir.filePath("protected.3mf");sentinel(path);
  const auto response=w.execute("document.export3mf",{{"path",path},{"allVisible",true},
      {"surfaceDeviationMm",.1},{"normalDeviationDeg",10}});
  QCOMPARE(response.value("completion").toString(),QString("pending"));
  const auto op=response.value("result").toObject();w.geometryEvaluationPoll_->stop();
  const auto task=w.geometryEvaluationRunning_;waitForApproval(task);
  QCOMPARE(bytes(path),QByteArray("preserve"));
  QVERIFY(call(w,"command.cancel",{{"session",op.value("id")}}).value("resultDiscarded").toBool());
  drain(w);QCOMPARE(bytes(path),QByteArray("preserve"));QCOMPARE(call(w,"document.inspect"),before);
  QCOMPARE(status(w,op).value("error").toObject().value("code").toString(),QString("export_cancelled"));
 }
 void objDialogStateAndPublication(){
  QTemporaryDir dir;QVERIFY(dir.isValid());AlloyWindow w;
  call(w,"solid.box",{{"width",20},{"depth",10},{"height",5},{"units","mm"}});
  const auto before=call(w,"document.inspect");const auto path=dir.filePath("protected.obj");sentinel(path);
  call(w,"document.exportMeshDialog");QPointer<QDialog> dialog=w.findChild<QDialog*>("mesh.export.dialog");QVERIFY(dialog);
  auto* format=dialog->findChild<QComboBox*>("mesh.export.format");QVERIFY(format);
  // State coverage only: native chooser/input is verified separately.
  format->setCurrentIndex(format->findData("obj"));
  QCOMPARE(format->currentData().toString(),QString("obj"));
  QVERIFY(!dialog->findChild<QComboBox*>("mesh.export.units")->isHidden());
  QVERIFY(dialog->findChild<QLabel*>("mesh.export.note")->text().contains("Z-up"));
  dialog->reject();QApplication::processEvents();QCOMPARE(call(w,"document.inspect"),before);
  const auto response=w.execute("document.exportObj",{{"path",path},{"allVisible",true},{"units","in"},
      {"surfaceDeviationMm",.1},{"normalDeviationDeg",10}});
  QCOMPARE(response.value("completion").toString(),QString("pending"));
  const auto op=response.value("result").toObject();w.geometryEvaluationPoll_->stop();
  waitForApproval(w.geometryEvaluationRunning_);QCOMPARE(bytes(path),QByteArray("preserve"));
  QCOMPARE(w.execute("history.undo").value("error").toObject().value("code").toString(),QString("evaluation_busy"));
  QVERIFY(call(w,"command.cancel",{{"session",op.value("id")}}).value("resultDiscarded").toBool());
  drain(w);QCOMPARE(bytes(path),QByteArray("preserve"));QCOMPARE(call(w,"document.inspect"),before);
  QCOMPARE(status(w,op).value("error").toObject().value("code").toString(),QString("export_cancelled"));
  call(w,"document.new",{{"discard",true}});w.close();
 }
 void staleAndTeardownReleaseAuthorization(){
  QTemporaryDir dir;QVERIFY(dir.isValid());const auto path=dir.filePath("protected.stl");sentinel(path);
  auto w=std::make_unique<AlloyWindow>();call(*w,"solid.box",{{"width",20},{"depth",10},{"height",15},{"units","mm"}});const auto before=call(*w,"document.inspect");
  const auto op=start(*w,path);const auto task=w->geometryEvaluationRunning_;waitForApproval(task);
  // Simulate a direct native callback bypassing execute's mutation freeze.
  w->session_={{"id","changed"}};w->pollMeshExport();w->session_={};drain(*w);
  QCOMPARE(status(*w,op).value("error").toObject().value("code").toString(),QString("stale_export"));QCOMPARE(bytes(path),QByteArray("preserve"));QCOMPARE(call(*w,"document.inspect"),before);
  start(*w,path);const auto orphan=w->geometryEvaluationRunning_;waitForApproval(orphan);w.reset();
  QTRY_VERIFY_WITH_TIMEOUT(orphan->ready.load(std::memory_order_acquire),10000);QCOMPARE(bytes(path),QByteArray("preserve"));
  QCOMPARE(orphan->exportError.value("code").toString(),QString("export_cancelled"));
  QCOMPARE(QDir(dir.path()).entryList(QDir::Files|QDir::Hidden|QDir::NoDotAndDotDot),QStringList{"protected.stl"});
 }
};
QTEST_MAIN(MeshExportControlsTest)
#include "mesh_export_controls.moc"
