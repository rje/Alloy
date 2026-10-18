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
#include <QTimer>
#include <QJsonDocument>

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
