// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QAbstractItemView>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFile>
#include <QJsonDocument>
#include <QPushButton>
#include <QSpinBox>
#include <QTest>
#include <cmath>
class CircularPatternControls final:public QObject{
 Q_OBJECT
 QJsonObject call(AlloyWindow& w,const QString& id,const QJsonObject& a={}){const auto r=w.execute(id,a);if(!r["ok"].toBool())qFatal("%s",QJsonDocument(r).toJson().constData());return r["result"].toObject();}
 QJsonObject active(AlloyWindow& w){return w.state()["activeSession"].toObject();}
 QJsonObject draft(AlloyWindow& w){return active(w)["draftSketch"].toObject();}
 void click(QWidget* widget){QVERIFY(widget && widget->isVisible());const auto global=widget->mapToGlobal(widget->rect().center());auto* hit=QApplication::widgetAt(global);QVERIFY(hit==widget || widget->isAncestorOf(hit));QTest::mouseClick(hit,Qt::LeftButton,Qt::NoModifier,hit->mapFromGlobal(global));}
 void enter(QWidget* widget,const char* value){click(widget);QTRY_VERIFY(widget==QApplication::focusWidget() || widget->isAncestorOf(QApplication::focusWidget()));QTest::keySequence(QApplication::focusWidget(),QKeySequence::SelectAll);QTest::keyClicks(QApplication::focusWidget(),value);QTest::keyClick(QApplication::focusWidget(),Qt::Key_Tab);}
private slots:
 void fullAngleGlyphHistory(){
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1500,950);w.show();w.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&w));QApplication::setActiveWindow(&w);
  call(w,"sketch.begin",{{"plane","XY"},{"name","Circular hole pattern"}});call(w,"sketch.addPoint",{{"x",25},{"y",25},{"units","mm"}});call(w,"sketch.addCircle",{{"centerX",37},{"centerY",25},{"diameter",3},{"units","mm"}});call(w,"sketch.lookAt");
  ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(child))view=v;QVERIFY(view);const auto original=draft(w);const auto history=active(w).value("draftHistory");auto* action=w.findChild<QAction*>("sketch.action.circularPattern");QVERIFY(action);
  auto open=[&]()->QDialog*{action->trigger();return w.findChild<QDialog*>("sketch.pattern.dialog");};auto* dialog=open();QVERIFY(dialog);QVERIFY(!dialog->isWindow());QTRY_VERIFY(dialog->isVisible());QTRY_COMPARE(QApplication::activeWindow(),static_cast<QWidget*>(&w));QVERIFY(!dialog->findChild<QDoubleSpinBox*>("sketch.pattern.angle")->isEnabled());QCOMPARE(draft(w),original);QCOMPARE(active(w).value("draftHistory"),history);call(w,"sketch.lookAt");QCOMPARE(w.state()["viewport"].toObject()["gpu"].toObject()["draftNormalEdgeCount"].toInt(),6);
  const auto glyphBounds=w.state()["viewport"].toObject()["gpu"].toObject()["sketchPatternGlyphs"].toArray().first().toObject()["bounds"].toObject();const auto centerPixel=call(w,"view.projectSketchPoint",{{"sketchId",original["id"]},{"uMm",25},{"vMm",25}});QVERIFY(std::abs(glyphBounds["x"].toDouble()+glyphBounds["width"].toDouble()/2-centerPixel["x"].toDouble())<1);QVERIFY(std::abs(glyphBounds["y"].toDouble()+glyphBounds["height"].toDouble()/2-centerPixel["y"].toDouble())<1);
  auto output=qEnvironmentVariable("ALLOY_CIRCULAR_PATTERN_EVIDENCE");if(!output.isEmpty()){output=QDir(output).absolutePath();QDir().mkpath(output);call(w,"capture.screenshot",{{"path",output+"/native-full-preview.png"}});dialog->grab().save(output+"/native-full-controls.png");}
  click(dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Cancel));QTRY_VERIFY(!w.findChild<QDialog*>("sketch.pattern.dialog"));QCOMPARE(draft(w),original);
  dialog=open();QVERIFY(dialog);click(dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok));QTRY_VERIFY(!w.findChild<QDialog*>("sketch.pattern.dialog"));const auto full=draft(w);
  auto glyph=[&]()->QDialog*{const auto glyphs=w.state()["viewport"].toObject()["gpu"].toObject()["sketchPatternGlyphs"].toArray();if(glyphs.size()!=1)return nullptr;const auto b=glyphs.first().toObject()["bounds"].toObject();const QPoint at(qRound(b["x"].toDouble()+b["width"].toDouble()/2),qRound(b["y"].toDouble()+b["height"].toDouble()/2));if(!view->rect().contains(at) || QApplication::widgetAt(view->mapToGlobal(at))!=view)return nullptr;QTest::mouseDClick(view,Qt::LeftButton,Qt::NoModifier,at);return w.findChild<QDialog*>("sketch.pattern.dialog");};
  dialog=glyph();QVERIFY(dialog);QTRY_VERIFY(dialog->isVisible());auto* quantity=dialog->findChild<QSpinBox*>("sketch.pattern.quantity");QVERIFY(quantity);enter(quantity,"8");QCOMPARE(quantity->value(),8);QCOMPARE(draft(w),full);click(dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok));QTRY_VERIFY(!w.findChild<QDialog*>("sketch.pattern.dialog"));const auto eight=draft(w);call(w,"history.undo");QCOMPARE(draft(w),full);call(w,"history.redo");QCOMPARE(draft(w),eight);call(w,"history.undo");
  dialog=glyph();QVERIFY(dialog);QTRY_VERIFY(dialog->isVisible());auto* mode=dialog->findChild<QComboBox*>("sketch.pattern.mode");QVERIFY(mode);click(mode);QTRY_VERIFY(mode->view()->isVisible());QVERIFY(QApplication::activePopupWidget());QVERIFY(QApplication::focusWidget());QTest::keyClick(QApplication::focusWidget(),Qt::Key_End);QTest::keyClick(QApplication::focusWidget(),Qt::Key_Return);QTRY_VERIFY(!mode->view()->isVisible());QCOMPARE(mode->currentData().toString(),QString("angle"));quantity=dialog->findChild<QSpinBox*>("sketch.pattern.quantity");enter(quantity,"4");auto* angle=dialog->findChild<QDoubleSpinBox*>("sketch.pattern.angle");QVERIFY(angle && angle->isEnabled());QCOMPARE(angle->value(),120.);QCOMPARE(draft(w),full);if(!output.isEmpty())call(w,"capture.screenshot",{{"path",output+"/native-angle120-preview.png"}});click(dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok));QTRY_VERIFY(!w.findChild<QDialog*>("sketch.pattern.dialog"));const auto angle120=draft(w);
  dialog=glyph();QVERIFY(dialog);QTRY_VERIFY(dialog->isVisible());angle=dialog->findChild<QDoubleSpinBox*>("sketch.pattern.angle");enter(angle,"150");QCOMPARE(angle->value(),150.);QCOMPARE(draft(w),angle120);if(!output.isEmpty()){dialog->grab().save(output+"/native-angle150-controls.png");call(w,"capture.screenshot",{{"path",output+"/native-angle150-preview.png"}});QFile file(output+"/native-state.json");QVERIFY(file.open(QIODevice::WriteOnly));file.write(QJsonDocument(w.state()).toJson());}click(dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Cancel));QTRY_VERIFY(!w.findChild<QDialog*>("sketch.pattern.dialog"));QCOMPARE(draft(w),angle120);
  dialog=glyph();QVERIFY(dialog);angle=dialog->findChild<QDoubleSpinBox*>("sketch.pattern.angle");enter(angle,"150");click(dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok));QTRY_VERIFY(!w.findChild<QDialog*>("sketch.pattern.dialog"));const auto angle150=draft(w);call(w,"history.undo");QCOMPARE(draft(w),angle120);call(w,"history.redo");QCOMPARE(draft(w),angle150);call(w,"sketch.cancel");w.close();
 }
};
QTEST_MAIN(CircularPatternControls)
#include "sketch_circular_pattern_controls.moc"
