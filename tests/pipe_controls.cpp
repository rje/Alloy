// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QTest>
#include <QApplication>
#include <QToolButton>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QLineEdit>
#include <QMenu>
#include <QTimer>
#include <QSignalSpy>
#include <QJsonDocument>
#include <QDir>
#include <QFile>
#include <cmath>
class PipeControlsTest final:public QObject {
 Q_OBJECT
 QJsonObject call(AlloyWindow& w,const QString& c,const QJsonObject& a={}){const auto r=w.execute(c,a);if(!r["ok"].toBool())qFatal("%s",QJsonDocument(r).toJson().constData());return r["result"].toObject();}
 QJsonObject session(AlloyWindow& w){return w.state()["activeSession"].toObject();}
private slots:
 void pathPreviewNumericCancel(){
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1400,900);w.show();w.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&w));
  ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;QVERIFY(view);
  call(w,"sketch.create",{{"plane","XY"},{"name","Pipe native path"}});call(w,"sketch.line",{{"x1",0},{"y1",0},{"x2",0},{"y2",30},{"units","mm"}});call(w,"sketch.addArc",{{"mode","center"},{"centerX",20},{"centerY",30},{"radius",20},{"startAngleDeg",180},{"sweepAngleDeg",-90},{"units","mm"}});const auto path=session(w)["draftSketch"].toObject();call(w,"sketch.finish");call(w,"view.home");const auto before=call(w,"document.inspect");
  auto* create=w.findChild<QToolButton*>("solid.menu.create");QVERIFY(create && create->menu());auto* menu=create->menu();QAction* pipe=nullptr;for(auto* a:menu->actions())if(a->objectName()=="solid.action.solid.pipeDialog")pipe=a;QVERIFY(pipe);
  QSignalSpy triggered(pipe,&QAction::triggered);
  QTimer::singleShot(50,&w,[&]{if(!menu->isVisible() || QApplication::activePopupWidget()!=menu)qFatal("Create menu not natively active");const auto hit=menu->actionGeometry(pipe).center();qInfo()<<"Pipe popup rect"<<menu->rect()<<"action"<<menu->actionGeometry(pipe)<<"hit"<<menu->actionAt(hit);QTest::mouseMove(menu,hit);if(menu->activeAction()!=pipe)qFatal("Pipe action is not the actual popup hit");QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,hit);});QTest::mouseClick(create,Qt::LeftButton);QCOMPARE(triggered.count(),1);QTRY_COMPARE(session(w)["command"].toString(),QString("solid.pipe"));QTRY_VERIFY(!menu->isVisible());QVERIFY(QApplication::activePopupWidget()==nullptr);QVERIFY(view->hasFocus());
  auto* dock=w.findChild<QDockWidget*>("pipeDock");QVERIFY(dock && dock->isVisible());auto* buttons=dock->findChild<QDialogButtonBox*>();QVERIFY(buttons);QVERIFY(!buttons->button(QDialogButtonBox::Ok)->isEnabled());
  const auto pixel=call(w,"view.projectSketchPoint",{{"sketchId",path["id"]},{"uMm",0},{"vMm",15}});QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,QPoint(qRound(pixel["x"].toDouble()),qRound(pixel["y"].toDouble())));QTRY_VERIFY(session(w)["previewed"].toBool());QCOMPARE(session(w)["inputs"].toObject()["pathEntityIds"].toArray().size(),2);QVERIFY(buttons->button(QDialogButtonBox::Ok)->isEnabled());const double volume=session(w)["geometry"].toObject()["volumeMm3"].toDouble();
  auto* diameter=dock->findChild<QLineEdit*>("pipe.diameterMm");QVERIFY(diameter);QTest::mouseClick(diameter,Qt::LeftButton);QVERIFY(diameter->hasFocus());QTest::keySequence(diameter,QKeySequence::SelectAll);QTest::keyClicks(diameter,"12");QTest::keyClick(diameter,Qt::Key_Return);QTRY_COMPARE(session(w)["inputs"].toObject()["diameterMm"].toDouble(),12.);QTRY_VERIFY(session(w)["previewed"].toBool());QVERIFY(session(w)["geometry"].toObject()["volumeMm3"].toDouble()>volume);
  const auto evidence=qEnvironmentVariable("ALLOY_PIPE_CONTROLS_EVIDENCE");if(!evidence.isEmpty()){QDir().mkpath(evidence);call(w,"capture.screenshot",{{"path",evidence+"/pipe-native-preview.png"}});QFile state(evidence+"/pipe-native-state.json");QVERIFY(state.open(QIODevice::WriteOnly));state.write(QJsonDocument(w.state()).toJson(QJsonDocument::Indented));}
  QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel),Qt::LeftButton);QTRY_VERIFY(session(w).isEmpty());const auto after=call(w,"document.inspect");QCOMPARE(after["authored"],before["authored"]);QCOMPARE(after["undo"],before["undo"]);QVERIFY(view->diagnostics()["sweepPath"].toObject().isEmpty());QVERIFY(!dock->isVisible());
 }
};
QTEST_MAIN(PipeControlsTest)
#include "pipe_controls.moc"
