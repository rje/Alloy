// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include <QApplication>
#include <QAbstractItemView>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QJsonDocument>
#include <QLineEdit>
#include <QPushButton>
#include <QTest>
class ExtrudeDirectionsControlsTest final:public QObject {
 Q_OBJECT
 QJsonObject call(AlloyWindow& w,const QString& c,const QJsonObject& a={}){const auto r=w.execute(c,a);if(!r["ok"].toBool())qFatal("%s %s",qPrintable(c),QJsonDocument(r).toJson().constData());return r["result"].toObject();}
 void hit(QWidget* widget){QTRY_VERIFY(([&]{const auto at=widget->mapToGlobal(widget->rect().center());auto* target=QApplication::widgetAt(at);return target && (target==widget || widget->isAncestorOf(target));})());const auto global=widget->mapToGlobal(widget->rect().center());auto* actual=QApplication::widgetAt(global);QVERIFY(actual);QVERIFY(actual==widget || widget->isAncestorOf(actual));QTest::mouseClick(actual,Qt::LeftButton,Qt::NoModifier,actual->mapFromGlobal(global));}
 void choose(QComboBox* combo,const QString& data){hit(combo);auto* view=combo->view();QTRY_VERIFY(view->isVisible());QTRY_VERIFY(QApplication::activePopupWidget()==view->window());auto* focus=QApplication::focusWidget();QVERIFY(focus && (focus==view || view->isAncestorOf(focus)));QTest::keyClick(focus,Qt::Key_Home);for(int i=0;i<combo->findData(data);++i)QTest::keyClick(focus,Qt::Key_Down);QTest::keyClick(focus,Qt::Key_Return);QTRY_VERIFY(!QApplication::activePopupWidget());QCOMPARE(combo->currentData().toString(),data);}

 void type(QDoubleSpinBox* field,const char* text){auto* line=field->findChild<QLineEdit*>();hit(line);QVERIFY(line->hasFocus());QTest::keyClick(line,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(line,text);QTest::keyClick(line,Qt::Key_Tab);}
private slots:
 void directionControls(){AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1300,900);w.show();QVERIFY(QTest::qWaitForWindowExposed(&w));QApplication::setActiveWindow(&w);
 call(w,"sketch.begin",{{"plane","XY"}});call(w,"sketch.rectangle",{{"x1",-20},{"y1",-15},{"x2",20},{"y2",15},{"units","mm"}});call(w,"sketch.finish");
 const auto before=call(w,"document.inspect");const auto sketch=before["evaluation"].toObject()["sketches"].toArray().first().toObject();
 const QJsonObject inputs{{"sketchId",sketch["featureId"]},{"profileId",sketch["profiles"].toArray().first().toObject()["profileId"]},{"distance",10}};
 auto session=call(w,"solid.extrudeDialog",inputs);auto* dock=w.findChild<QDockWidget*>("extrudeDock");auto* direction=dock->findChild<QComboBox*>("extrude.direction");QVERIFY(direction);
 choose(direction,"symmetric");QCOMPARE(w.state()["activeSession"].toObject()["inputs"].toObject()["direction"].toString(),QString("symmetric"));
 auto* measurement=dock->findChild<QComboBox*>("extrude.measurement");QVERIFY(measurement->isVisible());choose(measurement,"wholeLength");
 type(dock->findChild<QDoubleSpinBox*>("extrude.distance"),"20");QVERIFY(w.state()["activeSession"].toObject()["previewed"].toBool());
 const auto evidence=qEnvironmentVariable("ALLOY_EXTRUDE_DIRECTIONS_EVIDENCE");if(!evidence.isEmpty())call(w,"capture.screenshot",{{"path",evidence+"/native-symmetric-whole.png"}});
 auto* buttons=dock->findChild<QDialogButtonBox*>("extrude.buttons");hit(buttons->button(QDialogButtonBox::Cancel));QVERIFY(w.state()["activeSession"].toObject().isEmpty());QCOMPARE(call(w,"document.inspect"),before);
 session=call(w,"solid.extrudeDialog",inputs);dock=w.findChild<QDockWidget*>("extrudeDock");direction=dock->findChild<QComboBox*>("extrude.direction");choose(direction,"twoSides");
 type(dock->findChild<QDoubleSpinBox*>("extrude.negativeDistance"),"0");QVERIFY(!dock->findChild<QDialogButtonBox*>("extrude.buttons")->button(QDialogButtonBox::Ok)->isEnabled());type(dock->findChild<QDoubleSpinBox*>("extrude.negativeDistance"),"8");type(dock->findChild<QDoubleSpinBox*>("extrude.distance"),"14");QVERIFY(w.state()["activeSession"].toObject()["previewed"].toBool());
 if(!evidence.isEmpty())call(w,"capture.screenshot",{{"path",evidence+"/native-two-sides.png"}});
 buttons=dock->findChild<QDialogButtonBox*>("extrude.buttons");QVERIFY(buttons->button(QDialogButtonBox::Ok)->isEnabled());hit(buttons->button(QDialogButtonBox::Ok));
 const auto created=call(w,"document.inspect");const auto feature=created["authored"].toObject()["features"].toArray().last().toObject();QCOMPARE(feature["parameters"].toObject()["negativeDistanceMm"].toDouble(),8.);QCOMPARE(feature["parameters"].toObject()["distanceMm"].toDouble(),14.);
 session=call(w,"feature.editDialog",{{"featureId",feature["id"]}});dock=w.findChild<QDockWidget*>("extrudeDock");QCOMPARE(dock->findChild<QComboBox*>("extrude.direction")->currentData().toString(),QString("twoSides"));type(dock->findChild<QDoubleSpinBox*>("extrude.negativeDistance"),"9");buttons=dock->findChild<QDialogButtonBox*>("extrude.buttons");hit(buttons->button(QDialogButtonBox::Ok));
 QCOMPARE(call(w,"document.inspect")["authored"].toObject()["features"].toArray().last().toObject()["id"],feature["id"]);
 call(w,"document.new",{{"discard",true}});w.close();
 }
};
QTEST_MAIN(ExtrudeDirectionsControlsTest)
#include "extrude_symmetric_two_sides_controls.moc"
