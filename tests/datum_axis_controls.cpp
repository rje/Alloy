// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QJsonDocument>
#include <QPushButton>
#include <QTest>
class DatumAxisControlsTest final:public QObject {
 Q_OBJECT
 QJsonObject call(AlloyWindow& w,const QString& c,const QJsonObject& a={}){const auto r=w.execute(c,a);if(!r.value("ok").toBool())qFatal("%s: %s",qPrintable(c),QJsonDocument(r).toJson().constData());return r.value("result").toObject();}
 void choose(QComboBox* combo,int steps){QVERIFY(QApplication::widgetAt(combo->mapToGlobal(combo->rect().center()))==combo);QTest::mouseClick(combo,Qt::LeftButton);QTRY_VERIFY(QApplication::activePopupWidget());auto* recipient=QApplication::focusWidget();QVERIFY(recipient);QTest::keyClick(recipient,Qt::Key_Home);for(int i=0;i<steps;++i)QTest::keyClick(recipient,Qt::Key_Down);QTest::keyClick(recipient,Qt::Key_Return);QTRY_VERIFY(QApplication::activePopupWidget()==nullptr);}
private slots:
 void sourceConstructorsPreviewCancelEdit(){
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1440,960);w.show();QVERIFY(QTest::qWaitForWindowExposed(&w));QApplication::setActiveWindow(&w);w.activateWindow();
  call(w,"sketch.begin",{{"plane","XY"},{"name","Axis sources"}});call(w,"sketch.addPoint",{{"x",2},{"y",3},{"units","mm"}});call(w,"sketch.addPoint",{{"x",8},{"y",11},{"units","mm"}});call(w,"sketch.line",{{"x1",-5},{"y1",0},{"x2",15},{"y2",0},{"units","mm"}});call(w,"sketch.finish");
  const auto initial=call(w,"document.inspect").value("authored").toObject();
  for(bool commit:{false,true}){
   call(w,"construction.axis");auto* dock=w.findChild<QDockWidget*>("axisDock");QVERIFY(dock && dock->isVisible());QVERIFY(w.state().value("ui").toObject().value("activeDialog").toObject().value("visible").toBool());
   auto* p1=dock->findChild<QComboBox*>("axis.point1");auto* p2=dock->findChild<QComboBox*>("axis.point2");auto* buttons=dock->findChild<QDialogButtonBox*>("axis.buttons");QVERIFY(p1 && p2 && buttons);QVERIFY(!buttons->button(QDialogButtonBox::Ok)->isEnabled());
   choose(p1,1);choose(p2,2);QTRY_VERIFY(buttons->button(QDialogButtonBox::Ok)->isEnabled());QVERIFY(w.state().value("activeSession").toObject().value("previewed").toBool());QCOMPARE(call(w,"document.inspect").value("authored").toObject(),initial);
   const auto output=qEnvironmentVariable("ALLOY_DATUM_AXIS_EVIDENCE");if(!output.isEmpty())call(w,"capture.screenshot",{{"path",output+(commit?"/points-preview-commit.png":"/points-preview-cancel.png")}});
   auto* button=buttons->button(commit?QDialogButtonBox::Ok:QDialogButtonBox::Cancel);QVERIFY(QApplication::widgetAt(button->mapToGlobal(button->rect().center()))==button);QTest::mouseClick(button,Qt::LeftButton);QTRY_VERIFY(w.state().value("activeSession").toObject().isEmpty());if(!commit)QCOMPARE(call(w,"document.inspect").value("authored").toObject(),initial);
  }
  const auto pointRows=call(w,"document.inspect").value("authored").toObject().value("features").toArray();QCOMPARE(pointRows.size(),2);
  call(w,"feature.editDialog",{{"featureId",pointRows[1].toObject().value("id")}});auto* dock=w.findChild<QDockWidget*>("axisDock");QVERIFY(dock);QCOMPARE(dock->findChild<QComboBox*>("axis.point1")->currentIndex(),1);QCOMPARE(dock->findChild<QComboBox*>("axis.point2")->currentIndex(),2);call(w,"command.cancel",{{"session",w.state().value("activeSession").toObject().value("id")}});
  call(w,"construction.axis");dock=w.findChild<QDockWidget*>("axisDock");choose(dock->findChild<QComboBox*>("axis.method"),1);choose(dock->findChild<QComboBox*>("axis.line"),1);auto* buttons=dock->findChild<QDialogButtonBox*>("axis.buttons");QTRY_VERIFY(buttons->button(QDialogButtonBox::Ok)->isEnabled());
  const auto output=qEnvironmentVariable("ALLOY_DATUM_AXIS_EVIDENCE");if(!output.isEmpty())call(w,"capture.screenshot",{{"path",output+"/line-preview.png"}});
  QTest::mouseClick(buttons->button(QDialogButtonBox::Ok),Qt::LeftButton);QTRY_VERIFY(w.state().value("activeSession").toObject().isEmpty());const auto rows=call(w,"document.inspect").value("authored").toObject().value("features").toArray();QCOMPARE(rows.size(),3);QCOMPARE(w.state().value("document").toObject().value("bodies").toInt(),0);
  call(w,"feature.editDialog",{{"featureId",rows[2].toObject().value("id")}});dock=w.findChild<QDockWidget*>("axisDock");QCOMPARE(dock->findChild<QComboBox*>("axis.method")->currentIndex(),1);QCOMPARE(dock->findChild<QComboBox*>("axis.line")->currentIndex(),1);call(w,"command.cancel",{{"session",w.state().value("activeSession").toObject().value("id")}});
 }
};
QTEST_MAIN(DatumAxisControlsTest)
#include "datum_axis_controls.moc"
