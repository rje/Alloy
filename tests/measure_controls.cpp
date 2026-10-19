// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QAbstractItemView>
#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QLabel>
#include <QMenu>
#include <QPointer>
#include <QProcess>
#include <QPushButton>
#include <QTest>
#include <QTimer>
#include <QToolButton>
#include <QWindow>

class MeasureControls final : public QObject {
 Q_OBJECT
 QJsonArray transcript_;
 QJsonObject call(AlloyWindow& w,const QString& id,const QJsonObject& args={}) {
  const auto r=w.executeApi(id,args);transcript_.append(QJsonObject{{"command",id},{"arguments",args},{"response",r}});
  if(!r.value("ok").toBool())qFatal("%s",QJsonDocument(r).toJson().constData());
  return r.value("result").toObject();
 }
 void click(QWidget* widget,QPoint point={-1,-1}) {
  QVERIFY(widget);if(point.x()<0)point=widget->rect().center();
  QCOMPARE(QApplication::widgetAt(widget->mapToGlobal(point)),widget);
  QTest::mouseClick(widget,Qt::LeftButton,Qt::NoModifier,point);
 }
private slots:
 void nativeSequence() {
  const auto output=qEnvironmentVariable("ALLOY_MEASURE_EVIDENCE");QVERIFY(!output.isEmpty());QVERIFY(QDir().mkpath(output));
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1400,900);w.show();QVERIFY(QTest::qWaitForWindowExposed(&w));
  // API fixture setup precedes initial harness activation and all demonstrated input.
  call(w,"solid.box",{{"width",20},{"depth",10},{"height",5},{"x",60},{"units","mm"}});
  call(w,"solid.cylinder",{{"radius",5},{"height",12},{"x",90},{"units","mm"}});
  call(w,"sketch.begin",{{"plane","XY"}});
  call(w,"sketch.line",{{"x1",0},{"y1",0},{"x2",3},{"y2",4},{"units","mm"}});
  call(w,"sketch.addPoint",{{"x",-3.3},{"y",5.6},{"units","mm"}});
  call(w,"sketch.line",{{"x1",40},{"y1",0},{"x2",45},{"y2",0},{"units","mm"}});
  call(w,"sketch.line",{{"x1",45},{"y1",0},{"x2",45},{"y2",5},{"units","mm"}});
  const auto sketchId=call(w,"app.state").value("activeSession").toObject().value("draftSketch").toObject().value("id");
  call(w,"sketch.finish");call(w,"view.orientation",{{"name","top"}});call(w,"view.fit");
  ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;QVERIFY(view);
#ifdef Q_OS_MACOS
  QProcess activation;activation.start("osascript",{"-e",QString("tell application \"System Events\" to set frontmost of first application process whose unix id is %1 to true").arg(QCoreApplication::applicationPid())});
  QVERIFY(activation.waitForFinished(5000));QCOMPARE(activation.exitCode(),0);
#endif
  w.raise();w.activateWindow();w.windowHandle()->requestActivate();QApplication::setActiveWindow(&w);QTRY_VERIFY(w.isActiveWindow());
  call(w,"document.save",{{"path",output+"/before.alloy"}});
  const auto savedBefore=call(w,"document.inspect");
  auto pixel=[&](double u,double v){const auto p=call(w,"view.projectSketchPoint",{{"sketchId",sketchId},{"uMm",u},{"vMm",v}});return QPoint(qRound(p.value("x").toDouble()),qRound(p.value("y").toDouble()));};
  // Press opens the native popup. Do not release onto the toolbar button after
  // the action may have rebuilt it; the menu receives the actual click/release.
  QPointer<QToolButton> inspect=w.findChild<QToolButton*>("solid.menu.inspect");QVERIFY(inspect);
  QPointer<QMenu> menu=inspect->menu();QVERIFY(menu);auto* action=w.findChild<QAction*>("solid.action.inspect.measure");QVERIFY(action);
  bool ready=false;QTimer::singleShot(150,&w,[&]{ready=menu && menu->isVisible() && menu->isActiveWindow() && QApplication::activePopupWidget()==menu;
   if(ready)click(menu,menu->actionGeometry(action).center());else if(menu)menu->close();});
  QCOMPARE(QApplication::widgetAt(inspect->mapToGlobal(inspect->rect().center())),static_cast<QWidget*>(inspect));
  QTest::mousePress(inspect,Qt::LeftButton);QVERIFY(ready);QTRY_VERIFY(!menu || !menu->isVisible());
  QPointer<QDialog> dialog=w.findChild<QDialog*>("measure.dialog");QVERIFY(dialog);QTRY_VERIFY(dialog->isVisible());
  auto results=[&]{return dialog->findChild<QLabel*>("measure.Results");};
  auto clear=[&]{click(dialog->findChild<QPushButton*>("measure.clear"));QTRY_VERIFY(results()->text().contains("Select"));};
  auto filter=[&](const QString& text){auto* combo=dialog->findChild<QComboBox*>("measure.filter");click(combo);
   QTRY_VERIFY(combo->view()->isVisible());QTRY_VERIFY(combo->view()->window()->isActiveWindow());
   auto* receiver=QApplication::focusWidget();QVERIFY(receiver);
   QVERIFY(receiver==combo->view() || combo->view()->isAncestorOf(receiver));
   QTest::keyClick(receiver,Qt::Key_Home);
   for(int i=0;i<combo->findText(text);++i)QTest::keyClick(receiver,Qt::Key_Down);
   QTRY_COMPARE(combo->view()->currentIndex().row(),combo->findText(text));
   QTest::keyClick(receiver,Qt::Key_Return);
   QTRY_VERIFY(!combo->view()->isVisible());QCOMPARE(combo->currentText(),text);};
  click(view,pixel(1.5,2));QTRY_VERIFY(results()->text().contains("5.0000 mm"));
  QCOMPARE(call(w,"inspect.measureResult").value("sources").toArray().first().toObject().value("kind").toString(),QString("sketchEdge"));
  clear();filter("Point");click(view,pixel(-3.3,5.6));
  QCOMPARE(dialog->findChild<QLabel*>("measure.Selection 1")->text().startsWith("Sketch Point"),true);
  filter("Edge");click(view,pixel(1.5,2));QTRY_VERIFY(results()->text().contains("6.0000 mm"));
  const auto pair=call(w,"inspect.measureResult");QCOMPARE(pair.value("sources").toArray().size(),2);
  QApplication::clipboard()->setText("copy sentinel");
  // A real hit on the rendered rich-text anchor, not linkActivated emission.
  const auto* label=results();
  click(results(),QPoint(label->fontMetrics().horizontalAdvance("Distance: ")+25,label->height()/2));
  QTRY_COMPARE(QApplication::clipboard()->text(),QString("6.0000 mm"));
  call(w,"capture.screenshot",{{"path",output+"/point-line.png"}});
  QCOMPARE(call(w,"document.inspect"),savedBefore);
  clear();click(view,pixel(42.5,0));click(view,pixel(45,2.5));QTRY_VERIFY(results()->text().contains("90.0000 deg"));
  QVERIFY(results()->height()>=results()->fontMetrics().lineSpacing()*2+4);
  call(w,"capture.screenshot",{{"path",output+"/right-angle.png"}});
  clear();filter("Face");const auto fp=call(w,"view.projectSketchPoint",{{"plane","XY"},{"uMm",70},{"vMm",5}});
  click(view,QPoint(qRound(fp.value("x").toDouble()),qRound(fp.value("y").toDouble())));QTRY_VERIFY(results()->text().contains("200.0000 mm"));
  call(w,"capture.screenshot",{{"path",output+"/planar.png"}});
  clear();filter("Edge");
  const auto box=savedBefore.value("evaluation").toObject().value("bodies").toArray().first().toObject();QJsonObject edge;
  for(const auto& value:box.value("solidEdges").toArray()) {const auto candidate=value.toObject();const auto provenance=candidate.value("provenance").toObject();
   const auto a=provenance.value("startMm").toArray(),b=provenance.value("endMm").toArray();
   if(a.size()==3 && b.size()==3 && a[2].toDouble()==5 && b[2].toDouble()==5 && a[1].toDouble()==0 && b[1].toDouble()==0){edge=candidate;break;}}
  QVERIFY(!edge.isEmpty());const auto ep=call(w,"view.projectEdge",{{"bodyId",box.value("bodyId")},{"edgeId",edge.value("edgeId")}});
  click(view,QPoint(qRound(ep.value("x").toDouble()),qRound(ep.value("y").toDouble())));QTRY_VERIFY(results()->text().contains("20.0000 mm"));
  QCOMPARE(call(w,"inspect.measureResult").value("sources").toArray().first().toObject().value("kind").toString(),QString("edge"));
  call(w,"capture.screenshot",{{"path",output+"/body-edge.png"}});
  clear();filter("Face");
  // API camera setup only: place the curved body's side away from the panel.
  call(w,"view.orientation",{{"name","front"}});call(w,"view.fit");
  const auto cylinder=savedBefore.value("evaluation").toObject().value("bodies").toArray().last().toObject();
  auto cp=call(w,"view.projectBody",{{"bodyId",cylinder.value("bodyId")}});
  call(w,"view.pan",{{"x",view->width()*0.35-cp.value("x").toDouble()},{"y",view->height()*0.6-cp.value("y").toDouble()}});
  cp=call(w,"view.projectBody",{{"bodyId",cylinder.value("bodyId")}});
  click(view,QPoint(qRound(cp.value("x").toDouble()),qRound(cp.value("y").toDouble())));QTRY_VERIFY(results()->text().contains("376.9911 mm"));
  QCOMPARE(call(w,"inspect.measureResult").value("sources").toArray().first().toObject().value("bodyId"),cylinder.value("bodyId"));
  call(w,"capture.screenshot",{{"path",output+"/cylindrical.png"}});
  // Deliver Escape to the keyboard receiver actually produced by native steps.
  QVERIFY(QApplication::focusWidget());QTest::keyClick(QApplication::focusWidget(),Qt::Key_Escape);
  QTRY_VERIFY(!dialog);
  QTRY_VERIFY(w.isActiveWindow());QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
  QTest::keyClick(QApplication::focusWidget(),Qt::Key_I);
  dialog=w.findChild<QDialog*>("measure.dialog");QVERIFY(dialog);QTRY_VERIFY(dialog->isVisible());
  auto* close=dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Close);click(close);QTRY_VERIFY(!dialog);
  QTRY_VERIFY(w.isActiveWindow());QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
  click(view,QPoint(qRound(cp.value("x").toDouble()),qRound(cp.value("y").toDouble())));QVERIFY(!w.findChild<QDialog*>("measure.dialog"));
  QCOMPARE(w.state().value("selection").toObject().value("kind").toString(),QString("face"));
  QCOMPARE(w.state().value("selection").toObject().value("support").toObject().value("bodyId"),cylinder.value("bodyId"));
  QCOMPARE(call(w,"document.inspect"),savedBefore);
  call(w,"document.save",{{"path",output+"/after.alloy"}});
  QFile a(output+"/before.alloy"),b(output+"/after.alloy");QVERIFY(a.open(QIODevice::ReadOnly));QVERIFY(b.open(QIODevice::ReadOnly));QCOMPARE(a.readAll(),b.readAll());
  QFile receipt(output+"/receipt.json");QVERIFY(receipt.open(QIODevice::WriteOnly));receipt.write(QJsonDocument(QJsonObject{{"gpu",view->diagnostics()},{"state",w.state()},{"transcript",transcript_},{"result","passed bounded native workflow"}}).toJson());
  w.close();
 }
};
QTEST_MAIN(MeasureControls)
#include "measure_controls.moc"
