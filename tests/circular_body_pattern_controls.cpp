// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QTest>
#include <QToolButton>
#include <QSpinBox>
#include <QComboBox>
#include <QAbstractItemView>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QDockWidget>
#include <QJsonDocument>
#include <QFile>
class CircularBodyPatternControlsTest final:public QObject {
 Q_OBJECT
 QJsonObject call(AlloyWindow& w,const QString& c,const QJsonObject& a={}){const auto r=w.execute(c,a);if(!r["ok"].toBool())qFatal("%s",QJsonDocument(r).toJson().constData());return r["result"].toObject();}
 QJsonObject session(AlloyWindow& w){return w.state().value("activeSession").toObject();}
private slots:
 void fullEightBodies(){
  const auto output=qEnvironmentVariable("ALLOY_CIRCULAR_EVIDENCE");QVERIFY(!output.isEmpty());AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1280,860);w.show();w.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&w));ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(child))view=v;QVERIFY(view);
  call(w,"document.open",{{"path",output+"/source-seed.alloy"}});const auto before=call(w,"document.inspect").value("authored").toObject();call(w,"view.home");call(w,"view.orientation",{{"name","top"}});QApplication::setActiveWindow(&w);view->setFocus();QTest::qWait(100);QVERIFY(view->hasFocus()); // Initial harness activation only.
  auto* launch=w.findChild<QToolButton*>("command.solid.circularPatternDialog");QVERIFY(launch);auto choose=[&]{QTest::mouseClick(launch,Qt::LeftButton);QCOMPARE(session(w).value("command").toString(),QString("solid.circularPattern"));QVERIFY(view->hasFocus());const auto xy=view->projectSketchPoint(10,0,"XY");QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,QPoint(qRound(xy["x"].toDouble()),qRound(xy["y"].toDouble())));QVERIFY(!session(w).value("inputs").toObject().value("sourceBodyId").toString().isEmpty());QVERIFY(view->hasFocus());auto* axis=w.findChild<QComboBox*>("circular.axis");QVERIFY(axis);QTest::mouseClick(axis,Qt::LeftButton);QVERIFY(axis->view()->isVisible());auto* popupFocus=QApplication::focusWidget();QVERIFY(popupFocus && (popupFocus==axis->view() || axis->view()->isAncestorOf(popupFocus)));QTest::keyClick(popupFocus,Qt::Key_End);QTest::keyClick(popupFocus,Qt::Key_Return);QTRY_VERIFY(session(w).value("previewed").toBool());QTRY_VERIFY(QApplication::activePopupWidget()==nullptr);QTRY_VERIFY(view->hasFocus());auto* count=w.findChild<QSpinBox*>("circular.quantity");QVERIFY(count);QTest::mouseClick(count,Qt::LeftButton);QTest::keyClick(count,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(count,"8");QTest::keyClick(count,Qt::Key_Return);QTRY_COMPARE(session(w).value("geometry").toObject().value("occurrences").toArray().size(),8);QCOMPARE(call(w,"document.inspect").value("authored").toObject(),before);};
  choose();call(w,"capture.screenshot",{{"path",output+"/native-eight-preview.png"}});QFile preview(output+"/native-preview-state.json");QVERIFY(preview.open(QIODevice::WriteOnly));preview.write(QJsonDocument(w.state()).toJson());preview.close();auto* dock=w.findChild<QDockWidget*>("circularPatternDock");QVERIFY(dock);QTest::mouseClick(dock->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Cancel),Qt::LeftButton);QVERIFY(session(w).isEmpty());QCOMPARE(call(w,"document.inspect").value("authored").toObject(),before);
  choose();dock=w.findChild<QDockWidget*>("circularPatternDock");auto* buttons=dock->findChild<QDialogButtonBox*>();QVERIFY(buttons->button(QDialogButtonBox::Ok)->isEnabled());QTest::mouseClick(buttons->button(QDialogButtonBox::Ok),Qt::LeftButton);QVERIFY(session(w).isEmpty());const auto after=call(w,"document.inspect");QCOMPARE(after.value("evaluation").toObject().value("bodies").toArray().size(),8);const auto authored=after.value("authored").toObject();const auto feature=authored.value("features").toArray().last().toObject();
  call(w,"feature.editDialog",{{"featureId",feature.value("id")}});QTRY_VERIFY(session(w).value("previewed").toBool());auto* count=w.findChild<QSpinBox*>("circular.quantity");QTest::mouseClick(count,Qt::LeftButton);QTest::keyClick(count,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(count,"10");QTest::keyClick(count,Qt::Key_Return);QTRY_COMPARE(session(w).value("geometry").toObject().value("occurrences").toArray().size(),10);QTest::keyClick(count,Qt::Key_Escape);QVERIFY(session(w).isEmpty());QCOMPARE(call(w,"document.inspect").value("authored").toObject(),authored);
  call(w,"capture.screenshot",{{"path",output+"/native-eight-accepted.png"}});call(w,"document.save",{{"path",output+"/native-eight.alloy"}});call(w,"history.undo");QCOMPARE(call(w,"document.inspect").value("authored").toObject(),before);call(w,"history.redo");QCOMPARE(call(w,"document.inspect").value("authored").toObject(),authored);call(w,"document.open",{{"path",output+"/native-eight.alloy"}});QCOMPARE(call(w,"document.inspect").value("authored").toObject(),authored);QFile final(output+"/native-final-state.json");QVERIFY(final.open(QIODevice::WriteOnly));final.write(QJsonDocument(w.state()).toJson());w.close();
 }
};
QTEST_MAIN(CircularBodyPatternControlsTest)
#include "circular_body_pattern_controls.moc"
