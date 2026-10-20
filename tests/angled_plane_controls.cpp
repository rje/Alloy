// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QJsonDocument>
#include <QLineEdit>
#include <QPushButton>
#include <QTest>
#include <cmath>
class AngledPlaneControlsTest final:public QObject {
 Q_OBJECT
 QJsonObject call(AlloyWindow& w,const QString& c,const QJsonObject& a={}){const auto r=w.execute(c,a);if(!r.value("ok").toBool())qFatal("%s: %s",qPrintable(c),QJsonDocument(r).toJson().constData());return r.value("result").toObject();}
private slots:
 void typedAngleFlipPreviewAndCancel(){
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1440,960);w.show();QVERIFY(QTest::qWaitForWindowExposed(&w));
  QApplication::setActiveWindow(&w);w.activateWindow();
  call(w,"sketch.begin",{{"plane","XY"},{"name","Y pivot"}});call(w,"sketch.line",{{"x1",0},{"y1",0},{"x2",0},{"y2",20},{"units","mm"}});call(w,"sketch.finish");
  const auto initial=call(w,"document.inspect").value("authored").toObject();
  for(bool commit:{false,true}){
   call(w,"construction.angledPlane");QVERIFY(w.state().value("ui").toObject().value("activeDialog").toObject().value("visible").toBool());auto* dock=w.findChild<QDockWidget*>("angledPlaneDock");QVERIFY(dock && dock->isVisible());
   auto* line=dock->findChild<QComboBox*>("angledPlane.line");auto* plane=dock->findChild<QComboBox*>("angledPlane.plane");auto* angle=dock->findChild<QDoubleSpinBox*>("angledPlane.angleDeg");auto* flip=dock->findChild<QCheckBox*>("angledPlane.flipSide");auto* buttons=dock->findChild<QDialogButtonBox*>("angledPlane.buttons");QVERIFY(line && plane && angle && flip && buttons);
   QVERIFY(!buttons->button(QDialogButtonBox::Ok)->isEnabled());
   // Real popup recipients receive the actual keys; no focus repair between steps.
   for(auto* combo:{line,plane}){QTest::mouseClick(combo,Qt::LeftButton);QTRY_VERIFY(QApplication::activePopupWidget());auto* recipient=QApplication::focusWidget();QVERIFY(recipient);QTest::keyClick(recipient,Qt::Key_Home);QTest::keyClick(recipient,Qt::Key_Down);QTest::keyClick(recipient,Qt::Key_Return);QTRY_VERIFY(QApplication::activePopupWidget()==nullptr);}
   auto* editor=angle->findChild<QLineEdit*>();QVERIFY(editor);QVERIFY(QApplication::widgetAt(editor->mapToGlobal(editor->rect().center()))==editor);QTest::mouseClick(editor,Qt::LeftButton);QVERIFY(editor->hasFocus());QTest::keyClick(editor,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(editor,"30");QVERIFY(editor->text().startsWith("30"));QTest::keyClick(editor,Qt::Key_Tab);
   QTRY_COMPARE(w.state().value("activeSession").toObject().value("inputs").toObject().value("angleDeg").toDouble(),30.);
   const QPoint flipHit(10,flip->height()/2);QVERIFY(QApplication::widgetAt(flip->mapToGlobal(flipHit))==flip);QTest::mouseClick(flip,Qt::LeftButton,Qt::NoModifier,flipHit);QTRY_VERIFY(w.state().value("activeSession").toObject().value("inputs").toObject().value("flipSide").toBool());
   QTRY_VERIFY(buttons->button(QDialogButtonBox::Ok)->isEnabled());const auto preview=w.state().value("activeSession").toObject().value("previewFrame").toObject();QVERIFY(std::abs(preview.value("normal").toArray()[0].toDouble()+.5)<1e-8);
   const auto output=qEnvironmentVariable("ALLOY_ANGLED_PLANE_EVIDENCE");if(!output.isEmpty())call(w,"capture.screenshot",{{"path",output+(commit?"/numeric-preview-commit.png":"/numeric-preview-cancel.png")}});
   QTest::mouseClick(buttons->button(commit?QDialogButtonBox::Ok:QDialogButtonBox::Cancel),Qt::LeftButton);QTRY_VERIFY(w.state().value("activeSession").toObject().isEmpty());
   if(!commit)QCOMPARE(call(w,"document.inspect").value("authored").toObject(),initial);
  }
  const auto inspect=call(w,"document.inspect");const auto rows=inspect.value("authored").toObject().value("features").toArray();QCOMPARE(rows.size(),2);
  call(w,"feature.editDialog",{{"featureId",rows[1].toObject().value("id")}});auto* dock=w.findChild<QDockWidget*>("angledPlaneDock");QVERIFY(dock && dock->isVisible());QCOMPARE(dock->findChild<QDoubleSpinBox*>("angledPlane.angleDeg")->value(),30.);QVERIFY(dock->findChild<QCheckBox*>("angledPlane.flipSide")->isChecked());call(w,"command.cancel",{{"session",w.state().value("activeSession").toObject().value("id")}});
 }
};
QTEST_MAIN(AngledPlaneControlsTest)
#include "angled_plane_controls.moc"
