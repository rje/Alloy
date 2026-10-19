// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QAbstractItemView>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QJsonDocument>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QTest>
#include <cmath>
class ExtrudeThinControlsTest final:public QObject {
 Q_OBJECT
 QJsonObject call(AlloyWindow& w,const QString& c,const QJsonObject& a={}){const auto r=w.execute(c,a);if(!r["ok"].toBool())qFatal("%s %s",qPrintable(c),QJsonDocument(r).toJson().constData());return r["result"].toObject();}
 void hit(QWidget* widget){QTRY_VERIFY(([&]{const auto at=widget->mapToGlobal(widget->rect().center());auto* target=QApplication::widgetAt(at);return target && (target==widget || widget->isAncestorOf(target));})());const auto global=widget->mapToGlobal(widget->rect().center());auto* actual=QApplication::widgetAt(global);QVERIFY(actual);QVERIFY(actual==widget || widget->isAncestorOf(actual));QTest::mouseClick(actual,Qt::LeftButton,Qt::NoModifier,actual->mapFromGlobal(global));}
 void choose(QComboBox* combo,const QString& data){hit(combo);auto* view=combo->view();QTRY_VERIFY(view->isVisible());QTRY_VERIFY(QApplication::activePopupWidget()==view->window());auto* focus=QApplication::focusWidget();QVERIFY(focus && (focus==view || view->isAncestorOf(focus)));QTest::keyClick(focus,Qt::Key_Home);for(int i=0;i<combo->findData(data);++i)QTest::keyClick(focus,Qt::Key_Down);QTest::keyClick(focus,Qt::Key_Return);QTRY_VERIFY(!QApplication::activePopupWidget());QCOMPARE(combo->currentData().toString(),data);}

 void type(QDoubleSpinBox* field,const char* text){auto* line=field->findChild<QLineEdit*>();hit(line);QVERIFY(line->hasFocus());QTest::keyClick(line,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(line,text);QTest::keyClick(line,Qt::Key_Tab);}
private slots:
 void thinControls(){AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1300,900);w.show();QVERIFY(QTest::qWaitForWindowExposed(&w));QApplication::setActiveWindow(&w);
 call(w,"sketch.begin",{{"plane","XY"}});call(w,"sketch.line",{{"x1",0},{"y1",0},{"x2",40},{"y2",0},{"units","mm"}});call(w,"sketch.finish");const auto before=call(w,"document.inspect");
 auto session=call(w,"solid.extrudeDialog",{{"distance",20},{"direction","symmetric"},{"measurement","wholeLength"}});auto* dock=w.findChild<QDockWidget*>("extrudeDock");choose(dock->findChild<QComboBox*>("extrude.type"),"thin");
 const auto normalized=w.state()["activeSession"].toObject()["inputs"].toObject();QCOMPARE(normalized["direction"].toString(),"oneSide");QVERIFY(!normalized.contains("measurement"));QCOMPARE(normalized["taperAngleDeg"].toDouble(),0.);QVERIFY(dock->findChild<QLabel*>("extrude.error")->text().contains("open straight Line"));
 ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;QVERIFY(view);const auto sketchId=before["evaluation"].toObject()["sketches"].toArray().first().toObject()["featureId"];const auto point=call(w,"view.projectSketchPoint",{{"sketchId",sketchId},{"uMm",20},{"vMm",0}});const QPoint local(qRound(point["x"].toDouble()),qRound(point["y"].toDouble()));const auto global=view->mapToGlobal(local);auto* actual=QApplication::widgetAt(global);QVERIFY(actual && (actual==view || view->isAncestorOf(actual)));QTest::mouseClick(actual,Qt::LeftButton,Qt::NoModifier,actual->mapFromGlobal(global));QVERIFY(w.state()["activeSession"].toObject()["inputs"].toObject().contains("sourceEntityId"));
 auto* profiles=dock->findChild<QComboBox*>("extrude.profile");QVERIFY(profiles->currentData().toJsonObject().contains("sourceEntityId"));
 auto* wall=dock->findChild<QDoubleSpinBox*>("extrude.wallThickness");QVERIFY(wall->isVisible());type(wall,"4");choose(dock->findChild<QComboBox*>("extrude.wallLocation"),"center");QVERIFY(w.state()["activeSession"].toObject()["previewed"].toBool());
 const auto evidence=qEnvironmentVariable("ALLOY_EXTRUDE_THIN_EVIDENCE");if(!evidence.isEmpty()){call(w,"view.home");call(w,"capture.screenshot",{{"path",evidence+"/native-center-preview.png"}});call(w,"capture.dialog",{{"path",evidence+"/native-center-dialog.png"}});}
 auto* buttons=dock->findChild<QDialogButtonBox*>("extrude.buttons");hit(buttons->button(QDialogButtonBox::Cancel));QVERIFY(w.state()["activeSession"].toObject().isEmpty());QCOMPARE(call(w,"document.inspect"),before);
 const auto sketch=before["evaluation"].toObject()["sketches"].toArray().first().toObject();const auto entity=sketch["entities"].toArray().first().toObject();
 session=call(w,"solid.extrudeDialog",{{"sketchId",sketch["featureId"]},{"sourceEntityId",entity["id"]},{"wallThickness",4},{"wallLocation","center"},{"distance",20}});dock=w.findChild<QDockWidget*>("extrudeDock");QCOMPARE(dock->findChild<QComboBox*>("extrude.type")->currentData().toString(),"thin");buttons=dock->findChild<QDialogButtonBox*>("extrude.buttons");QVERIFY(buttons->button(QDialogButtonBox::Ok)->isEnabled());hit(buttons->button(QDialogButtonBox::Ok));
 const auto created=call(w,"document.inspect");const auto feature=created["authored"].toObject()["features"].toArray().last().toObject();QCOMPARE(feature["parameters"].toObject()["wallThicknessMm"].toDouble(),4.);
 session=call(w,"feature.editDialog",{{"featureId",feature["id"]}});dock=w.findChild<QDockWidget*>("extrudeDock");wall=dock->findChild<QDoubleSpinBox*>("extrude.wallThickness");QCOMPARE(wall->value(),4.);type(wall,"6");choose(dock->findChild<QComboBox*>("extrude.wallLocation"),"side1");buttons=dock->findChild<QDialogButtonBox*>("extrude.buttons");QVERIFY(buttons->button(QDialogButtonBox::Ok)->isEnabled());if(!evidence.isEmpty())call(w,"capture.screenshot",{{"path",evidence+"/native-side1-edit.png"}});hit(buttons->button(QDialogButtonBox::Ok));
 const auto edited=call(w,"document.inspect");QCOMPARE(edited["authored"].toObject()["features"].toArray().last().toObject()["id"],feature["id"]);QCOMPARE(edited["authored"].toObject()["features"].toArray().last().toObject()["parameters"].toObject()["sourceEntityId"],entity["id"]);QVERIFY(std::abs(edited["evaluation"].toObject()["bodies"].toArray().first().toObject()["volumeMm3"].toDouble()-4800)<1e-5);
 call(w,"document.new",{{"discard",true}});w.close();
 }
};
QTEST_MAIN(ExtrudeThinControlsTest)
#include "extrude_thin_controls.moc"
