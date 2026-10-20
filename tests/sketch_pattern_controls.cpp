// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QJsonDocument>
#include <QFile>
#include <QPushButton>
#include <QSpinBox>
#include <QTest>
class SketchPatternControls final:public QObject{
 Q_OBJECT
 QJsonObject call(AlloyWindow& w,const QString& id,const QJsonObject& a={}){const auto r=w.execute(id,a);if(!r["ok"].toBool())qFatal("%s",QJsonDocument(r).toJson().constData());return r["result"].toObject();}
 QJsonObject active(AlloyWindow& w){return w.state()["activeSession"].toObject();}
 QJsonObject draft(AlloyWindow& w){return active(w)["draftSketch"].toObject();}
 void click(QWidget* widget){const auto center=widget->rect().center();QVERIFY(widget->isVisible());auto* hit=QApplication::widgetAt(widget->mapToGlobal(center));QVERIFY(hit==widget || widget->isAncestorOf(hit));QTest::mouseClick(hit,Qt::LeftButton,Qt::NoModifier,hit->mapFromGlobal(widget->mapToGlobal(center)));}
private slots:
 void nativePreviewCancelCommitEdit(){
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1500,950);w.show();w.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&w));QApplication::setActiveWindow(&w);
  call(w,"sketch.begin",{{"plane","XY"},{"name","Rectangular holes"}});
  for(const auto& xy:QJsonArray{QJsonArray{10,0},QJsonArray{0,10}}){const auto end=xy.toArray();call(w,"sketch.line",{{"x1",0},{"y1",0},{"x2",end[0]},{"y2",end[1]},{"units","mm"}});const auto e=draft(w)["parameters"].toObject()["entities"].toArray().last().toObject();call(w,"sketch.construction",{{"entityIds",QJsonArray{e["id"]}},{"construction",true}});}
  call(w,"sketch.addCircle",{{"centerX",8},{"centerY",7},{"diameter",3},{"units","mm"}});call(w,"sketch.lookAt");const auto baseline=draft(w);const auto history=active(w).value("draftHistory");auto* action=w.findChild<QAction*>("sketch.action.rectangularPattern");QVERIFY(action);
  auto open=[&]()->QDialog*{action->trigger();auto* d=w.findChild<QDialog*>("sketch.pattern.dialog");return d;};
  auto* dialog=open();QVERIFY(dialog);QTRY_VERIFY(dialog->isVisible());QVERIFY(!dialog->isWindow());QTRY_COMPARE(QApplication::activeWindow(),static_cast<QWidget*>(&w));QVERIFY(active(w).contains("pendingSketchPattern"));QCOMPARE(draft(w),baseline);QCOMPARE(active(w).value("draftHistory"),history);
  call(w,"sketch.lookAt");QCOMPARE(w.state()["viewport"].toObject()["gpu"].toObject()["draftNormalEdgeCount"].toInt(),6);
  auto output=qEnvironmentVariable("ALLOY_SKETCH_PATTERN_EVIDENCE");if(!output.isEmpty())output=QDir(output).absolutePath();if(!output.isEmpty()){QDir().mkpath(output);dialog->grab().save(output+"/native-pattern-panel.png");call(w,"capture.screenshot",{{"path",output+"/native-pattern-preview.png"}});}
  auto* buttons=dialog->findChild<QDialogButtonBox*>();QVERIFY(buttons);click(buttons->button(QDialogButtonBox::Cancel));QTRY_VERIFY(!w.findChild<QDialog*>("sketch.pattern.dialog"));QVERIFY(!active(w).contains("pendingSketchPattern"));QCOMPARE(draft(w),baseline);
  dialog=open();QVERIFY(dialog);QTRY_COMPARE(QApplication::activeWindow(),static_cast<QWidget*>(&w));buttons=dialog->findChild<QDialogButtonBox*>();click(buttons->button(QDialogButtonBox::Ok));QTRY_VERIFY(!w.findChild<QDialog*>("sketch.pattern.dialog"));const auto accepted=draft(w);QCOMPARE(accepted["parameters"].toObject()["patternGroups"].toArray().size(),1);
  ReferenceView* view=nullptr;for(auto* widget:w.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(widget))view=candidate;QVERIFY(view);
  const auto glyphs=w.state()["viewport"].toObject()["gpu"].toObject()["sketchPatternGlyphs"].toArray();QCOMPARE(glyphs.size(),1);const auto bounds=glyphs.first().toObject()["bounds"].toObject();const QPoint point(qRound(bounds["x"].toDouble()+bounds["width"].toDouble()/2),qRound(bounds["y"].toDouble()+bounds["height"].toDouble()/2));QVERIFY(view->rect().contains(point));QCOMPARE(QApplication::widgetAt(view->mapToGlobal(point)),static_cast<QWidget*>(view));QTest::mouseDClick(view,Qt::LeftButton,Qt::NoModifier,point);
  dialog=w.findChild<QDialog*>("sketch.pattern.dialog");QVERIFY(dialog);QTRY_COMPARE(QApplication::activeWindow(),static_cast<QWidget*>(&w));QVERIFY(QApplication::focusWidget());
  if(!output.isEmpty()){dialog->grab().save(output+"/native-glyph-edit-panel.png");QFile file(output+"/activation.json");QVERIFY(file.open(QIODevice::WriteOnly));file.write(QJsonDocument(QJsonObject{{"activeWindow",QApplication::activeWindow()->objectName()},{"focusWidget",QApplication::focusWidget()->objectName()},{"glyphBounds",bounds},{"state",w.state()}}).toJson());}
  click(dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Cancel));QTRY_VERIFY(!w.findChild<QDialog*>("sketch.pattern.dialog"));QCOMPARE(draft(w),accepted);QVERIFY(!active(w).contains("pendingSketchPattern"));QTRY_COMPARE(QApplication::activeWindow(),static_cast<QWidget*>(&w));QTest::mouseDClick(view,Qt::LeftButton,Qt::NoModifier,point);
  dialog=w.findChild<QDialog*>("sketch.pattern.dialog");QVERIFY(dialog);QTRY_COMPARE(QApplication::activeWindow(),static_cast<QWidget*>(&w));auto* spacing=dialog->findChild<QDoubleSpinBox*>("sketch.pattern.spacing1");QVERIFY(spacing);click(spacing);QTRY_VERIFY(spacing==QApplication::focusWidget() || spacing->isAncestorOf(QApplication::focusWidget()));QTest::keySequence(QApplication::focusWidget(),QKeySequence::SelectAll);QTest::keyClicks(QApplication::focusWidget(),"16");QTest::keyClick(QApplication::focusWidget(),Qt::Key_Tab);QCOMPARE(spacing->value(),16.);QCOMPARE(draft(w),accepted);buttons=dialog->findChild<QDialogButtonBox*>();QVERIFY(buttons->button(QDialogButtonBox::Ok)->isEnabled());click(buttons->button(QDialogButtonBox::Ok));QTRY_VERIFY(!w.findChild<QDialog*>("sketch.pattern.dialog"));const auto changed=draft(w);QCOMPARE(changed["parameters"].toObject()["patternGroups"].toArray().first().toObject()["spacing1Mm"].toDouble(),16.);
  w.resize(1320,880);QTRY_COMPARE(w.size(),QSize(1320,880));QTest::qWait(60);const auto gpu=w.state()["viewport"].toObject()["gpu"].toObject();QVERIFY(gpu["hardwareAccelerated"].toBool());QCOMPARE(gpu["framebufferWidth"].toInt(),qRound(view->width()*view->devicePixelRatioF()));QCOMPARE(gpu["framebufferHeight"].toInt(),qRound(view->height()*view->devicePixelRatioF()));if(!output.isEmpty())call(w,"capture.screenshot",{{"path",output+"/native-resized-grid.png"}});
  call(w,"history.undo");QCOMPARE(draft(w),accepted);call(w,"history.redo");QCOMPARE(draft(w),changed);call(w,"sketch.cancel");w.close();
 }
};
QTEST_MAIN(SketchPatternControls)
#include "sketch_pattern_controls.moc"
