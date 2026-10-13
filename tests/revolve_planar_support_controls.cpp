// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QTest>
#include <QToolButton>
#include <QDoubleSpinBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QDockWidget>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
class RevolvePlanarSupportControlsTest final:public QObject {
 Q_OBJECT
 QJsonObject call(AlloyWindow& w,const QString& c,const QJsonObject& a={}){const auto r=w.execute(c,a);if(!r["ok"].toBool())qFatal("%s",QJsonDocument(r).toJson().constData());return r["result"].toObject();}
 QJsonObject session(AlloyWindow& w){return w.state()["activeSession"].toObject();}
private slots:
 void bottomDiskToInsideShell(){
  const auto output=qEnvironmentVariable("ALLOY_REVOLVE_SUPPORT_EVIDENCE");QVERIFY(!output.isEmpty());AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1280,860);w.show();w.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&w));ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(child))view=v;QVERIFY(view);
  call(w,"document.open",{{"path",output+"/source-full-revolve.alloy"}});const auto before=call(w,"document.inspect");const auto body=before["evaluation"].toObject()["bodies"].toArray()[0].toObject();const auto faces=body["supportFaces"].toArray();QCOMPARE(faces.size(),1);const auto face=faces[0].toObject();QCOMPARE(face["role"].toString(),QString("revolve.radial"));QVERIFY(!face["provenance"].toObject()["sourceEntityId"].toString().isEmpty());
  call(w,"view.home");call(w,"view.orientation",{{"name","bottom"}});QApplication::setActiveWindow(&w);view->setFocus();QTest::qWait(100);QVERIFY(view->hasFocus()); // Initial activation only.
  auto* launch=w.findChild<QToolButton*>("command.solid.shellDialog");QVERIFY(launch);QTest::mouseClick(launch,Qt::LeftButton);QCOMPARE(session(w)["command"].toString(),QString("solid.shell"));QVERIFY(view->hasFocus());
  const auto projected=view->projectSketchPoint(0,0,"XY");const QPoint disk(qRound(projected["x"].toDouble()),qRound(projected["y"].toDouble()));QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,disk);auto selected=session(w)["inputs"].toObject()["removedFace"].toObject();QCOMPARE(selected["faceId"].toString(),face["faceId"].toString());QCOMPARE(selected["producerFeatureId"].toString(),face["producerFeatureId"].toString());QCOMPARE(selected["bodyId"].toString(),body["bodyId"].toString());QVERIFY(view->hasFocus());
  auto enterThickness=[&]{auto* thickness=w.findChild<QDoubleSpinBox*>("shellThickness");QVERIFY(thickness&&thickness->isVisible());QTest::mouseClick(thickness,Qt::LeftButton);QTest::keyClick(thickness,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(thickness,"5");QTest::keyClick(thickness,Qt::Key_Return);QTRY_VERIFY(session(w)["previewed"].toBool());QVERIFY(session(w)["geometryEvaluated"].toBool());QCOMPARE(session(w)["inputs"].toObject()["thicknessMm"].toDouble(),5.);QCOMPARE(call(w,"document.inspect")["authored"].toObject(),before["authored"].toObject());};enterThickness();
  call(w,"capture.screenshot",{{"path",output+"/native-bottom-shell-preview.png"}});QFile preview(output+"/native-preview-state.json");QVERIFY(preview.open(QIODevice::WriteOnly));preview.write(QJsonDocument(w.state()).toJson(QJsonDocument::Indented));preview.close();auto* dock=w.findChild<QDockWidget*>("shellDock");QVERIFY(dock);auto* buttons=dock->findChild<QDialogButtonBox*>();QVERIFY(buttons);QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel),Qt::LeftButton);QCOMPARE(call(w,"document.inspect")["authored"].toObject(),before["authored"].toObject());QVERIFY(session(w).isEmpty());
  QTest::mouseClick(launch,Qt::LeftButton);QVERIFY(view->hasFocus());QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,disk);QCOMPARE(session(w)["inputs"].toObject()["removedFace"].toObject()["faceId"].toString(),face["faceId"].toString());enterThickness();dock=w.findChild<QDockWidget*>("shellDock");buttons=dock->findChild<QDialogButtonBox*>();QVERIFY(buttons->button(QDialogButtonBox::Ok)->isEnabled());QTest::mouseClick(buttons->button(QDialogButtonBox::Ok),Qt::LeftButton);QVERIFY(session(w).isEmpty());const auto after=call(w,"document.inspect");const auto hollow=after["evaluation"].toObject()["bodies"].toArray()[0].toObject();QVERIFY(hollow["valid"].toBool()&&hollow["closed"].toBool());QCOMPARE(hollow["bodyId"],body["bodyId"]);QCOMPARE(hollow["supportFaces"].toArray()[0].toObject()["role"].toString(),QString("shell-closed-cap"));QCOMPARE(hollow["supportFaces"].toArray()[0].toObject()["faceId"].toString(),face["faceId"].toString());QVERIFY(hollow["volumeMm3"].toDouble()>0&&hollow["volumeMm3"].toDouble()<body["volumeMm3"].toDouble());
  call(w,"view.orientation",{{"name","isometric"}});call(w,"capture.screenshot",{{"path",output+"/native-revolved-shell.png"}});const auto saved=output+"/native-source-shell.alloy";call(w,"document.save",{{"path",saved}});call(w,"history.undo");QCOMPARE(call(w,"document.inspect")["authored"].toObject(),before["authored"].toObject());call(w,"history.redo");QCOMPARE(call(w,"document.inspect")["authored"].toObject(),after["authored"].toObject());call(w,"document.open",{{"path",saved}});QCOMPARE(call(w,"document.inspect")["authored"].toObject(),after["authored"].toObject());QFile final(output+"/native-final-state.json");QVERIFY(final.open(QIODevice::WriteOnly));final.write(QJsonDocument(w.state()).toJson(QJsonDocument::Indented));w.close();
 }
};
QTEST_MAIN(RevolvePlanarSupportControlsTest)
#include "revolve_planar_support_controls.moc"
