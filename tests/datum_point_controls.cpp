// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QJsonDocument>
#include <QPushButton>
#include <QTest>
class DatumPointControlsTest final:public QObject {
 Q_OBJECT
 QJsonObject call(AlloyWindow& w,const QString& c,const QJsonObject& a={}){const auto r=w.execute(c,a);if(!r.value("ok").toBool())qFatal("%s: %s",qPrintable(c),QJsonDocument(r).toJson().constData());return r.value("result").toObject();}
 void choose(QComboBox* combo,int steps){QVERIFY(combo);QVERIFY(QApplication::widgetAt(combo->mapToGlobal(combo->rect().center()))==combo);QTest::mouseClick(combo,Qt::LeftButton);QTRY_VERIFY(QApplication::activePopupWidget());auto* recipient=QApplication::focusWidget();QVERIFY(recipient);QTest::keyClick(recipient,Qt::Key_Home);for(int i=0;i<steps;++i)QTest::keyClick(recipient,Qt::Key_Down);QTest::keyClick(recipient,Qt::Key_Return);QTRY_VERIFY(QApplication::activePopupWidget()==nullptr);}
 void click(QPushButton* button){QVERIFY(QApplication::widgetAt(button->mapToGlobal(button->rect().center()))==button);QTest::mouseClick(button,Qt::LeftButton);}
private slots:
 void constructorsPreviewCancelEdit(){
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1440,960);w.show();QVERIFY(QTest::qWaitForWindowExposed(&w));QApplication::setActiveWindow(&w);w.activateWindow();
  call(w,"solid.box",{{"width",10},{"depth",20},{"height",30},{"units","mm"}});
  call(w,"sketch.begin",{{"plane","XY"},{"name","Point path"}});call(w,"sketch.line",{{"x1",-5},{"y1",-10},{"x2",15},{"y2",-10},{"units","mm"}});call(w,"sketch.finish");call(w,"view.home");
  const auto initial=call(w,"document.inspect").value("authored").toObject();
  for(bool commit:{false,true}){
   call(w,"construction.point");auto* dock=w.findChild<QDockWidget*>("pointDock");QVERIFY(dock && dock->isVisible());QVERIFY(w.state().value("ui").toObject().value("activeDialog").toObject().value("visible").toBool());
   auto* buttons=dock->findChild<QDialogButtonBox*>("point.buttons");QVERIFY(!buttons->button(QDialogButtonBox::Ok)->isEnabled());choose(dock->findChild<QComboBox*>("point.vertex"),1);QTRY_VERIFY(buttons->button(QDialogButtonBox::Ok)->isEnabled());QVERIFY(w.state().value("activeSession").toObject().value("previewPoint").toObject().value("positionMm").isArray());QCOMPARE(call(w,"document.inspect").value("authored").toObject(),initial);
   const auto output=qEnvironmentVariable("ALLOY_DATUM_POINT_EVIDENCE");if(!output.isEmpty())call(w,"capture.screenshot",{{"path",output+(commit?"/vertex-preview-commit.png":"/vertex-preview-cancel.png")}});
   click(buttons->button(commit?QDialogButtonBox::Ok:QDialogButtonBox::Cancel));QTRY_VERIFY(w.state().value("activeSession").toObject().isEmpty());if(!commit)QCOMPARE(call(w,"document.inspect").value("authored").toObject(),initial);
  }
  auto rows=call(w,"document.inspect").value("authored").toObject().value("features").toArray();QCOMPARE(rows.size(),3);call(w,"feature.editDialog",{{"featureId",rows[2].toObject().value("id")}});auto* dock=w.findChild<QDockWidget*>("pointDock");QCOMPARE(dock->findChild<QComboBox*>("point.vertex")->currentIndex(),1);call(w,"command.cancel",{{"session",w.state().value("activeSession").toObject().value("id")}});
  call(w,"construction.point");dock=w.findChild<QDockWidget*>("pointDock");choose(dock->findChild<QComboBox*>("point.method"),1);choose(dock->findChild<QComboBox*>("point.line"),1);choose(dock->findChild<QComboBox*>("point.mode"),1);
  auto* field=dock->findChild<QDoubleSpinBox*>("point.value");QVERIFY(QApplication::widgetAt(field->mapToGlobal(field->rect().center()))==field || field->isAncestorOf(QApplication::widgetAt(field->mapToGlobal(field->rect().center()))));auto* hit=QApplication::widgetAt(field->mapToGlobal(field->rect().center()));QTest::mouseClick(hit,Qt::LeftButton);auto* recipient=QApplication::focusWidget();QVERIFY(recipient);QTest::keyClick(recipient,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(recipient,"5");QTest::keyClick(recipient,Qt::Key_Return);
  auto* buttons=dock->findChild<QDialogButtonBox*>("point.buttons");QTRY_VERIFY(buttons->button(QDialogButtonBox::Ok)->isEnabled());QCOMPARE(w.state().value("activeSession").toObject().value("inputs").toObject().value("location").toObject().value("distanceMm").toDouble(),5.);
  choose(dock->findChild<QComboBox*>("point.mode"),0);QCOMPARE(dock->findChild<QDoubleSpinBox*>("point.value")->value(),.25);choose(dock->findChild<QComboBox*>("point.mode"),1);QCOMPARE(dock->findChild<QDoubleSpinBox*>("point.value")->value(),5.);
  const auto output=qEnvironmentVariable("ALLOY_DATUM_POINT_EVIDENCE");if(!output.isEmpty())call(w,"capture.screenshot",{{"path",output+"/line-physical-preview.png"}});click(buttons->button(QDialogButtonBox::Ok));QTRY_VERIFY(w.state().value("activeSession").toObject().isEmpty());rows=call(w,"document.inspect").value("authored").toObject().value("features").toArray();QCOMPARE(rows.size(),4);
  call(w,"feature.editDialog",{{"featureId",rows[3].toObject().value("id")}});dock=w.findChild<QDockWidget*>("pointDock");QCOMPARE(dock->findChild<QComboBox*>("point.method")->currentIndex(),1);QCOMPARE(dock->findChild<QComboBox*>("point.mode")->currentIndex(),1);QCOMPARE(dock->findChild<QDoubleSpinBox*>("point.value")->value(),5.);auto* escapeRecipient=QApplication::focusWidget();QVERIFY(escapeRecipient);QTest::keyClick(escapeRecipient,Qt::Key_Escape);QTRY_VERIFY(w.state().value("activeSession").toObject().isEmpty());
 }
};
QTEST_MAIN(DatumPointControlsTest)
#include "datum_point_controls.moc"
