// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QTest>
#include <QComboBox>
#include <QAbstractItemView>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QJsonDocument>
#include <QDir>
#include <QTemporaryDir>
#include <cmath>
class IntersectControls final:public QObject {
 Q_OBJECT
 QJsonObject call(AlloyWindow& w,const QString& c,const QJsonObject& a={}){auto r=w.execute(c,a);if(!r["ok"].toBool())qFatal("%s",QJsonDocument(r).toJson().constData());return r["result"].toObject();}
private slots:
 void nativeOperationPreviewCancelCommit(){AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1400,900);w.show();w.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&w));
  auto rectangle=[&](double x1,double x2){call(w,"sketch.create",{{"plane","XY"}});call(w,"sketch.rectangle",{{"x1",x1},{"y1",0},{"x2",x2},{"y2",20},{"units","mm"}});call(w,"sketch.finish");return call(w,"document.inspect")["evaluation"].toObject()["sketches"].toArray().last().toObject();};
  const auto base=rectangle(0,20);call(w,"solid.extrude",{{"sketchId",base["sketchId"]},{"profileId",base["profiles"].toArray()[0].toObject()["profileId"]},{"distance",10},{"units","mm"}});const auto s=rectangle(5,25);call(w,"selection.profileSet",{{"sketchId",s["sketchId"]},{"profileId",s["profiles"].toArray()[0].toObject()["profileId"]}});const auto before=call(w,"document.inspect");
  ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* found=dynamic_cast<ReferenceView*>(child))view=found;QVERIFY(view);
  for(int trial=0;trial<2;++trial){call(w,"solid.extrudeDialog",{{"sketchId",s["sketchId"]},{"profileId",s["profiles"].toArray()[0].toObject()["profileId"]}});auto* distance=w.findChild<QDoubleSpinBox*>("extrude.distance");auto* operation=w.findChild<QComboBox*>("extrude.operation");QVERIFY(distance && operation);distance->setFocus();for(int i=0;i<6;++i)QTest::keyClick(distance,Qt::Key_Up);QTest::keyClick(distance,Qt::Key_Tab);
   operation->setFocus();QTest::keyClick(operation,Qt::Key_End);QTRY_COMPARE(w.state()["activeSession"].toObject()["inputs"].toObject()["operation"].toString(),QString("intersect"));QTRY_VERIFY(w.state()["activeSession"].toObject()["previewed"].toBool());QVERIFY(std::abs(w.state()["activeSession"].toObject()["geometry"].toObject()["volumeMm3"].toDouble()-1800)<1e-5);
   auto* panel=w.findChild<QWidget*>("extrudePanel");QVERIFY(panel);auto* buttons=panel->findChild<QDialogButtonBox*>();QVERIFY(buttons && buttons->button(QDialogButtonBox::Ok)->isEnabled());if(trial==0){const auto out=qEnvironmentVariable("ALLOY_INTERSECT_EVIDENCE");if(!out.isEmpty()){QDir().mkpath(out);call(w,"view.home");call(w,"capture.screenshot",{{"path",out+"/native-intersect-preview.png"}});}QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel),Qt::LeftButton);QCOMPARE(call(w,"document.inspect")["authored"],before["authored"]);QCOMPARE(call(w,"document.inspect")["undo"],before["undo"]);}else QTest::mouseClick(buttons->button(QDialogButtonBox::Ok),Qt::LeftButton);
  }
  const auto after=call(w,"document.inspect");QCOMPARE(after["authored"].toObject()["features"].toArray().last().toObject()["parameters"].toObject()["operation"].toString(),QString("intersect"));QVERIFY(std::abs(after["evaluation"].toObject()["bodies"].toArray()[0].toObject()["volumeMm3"].toDouble()-1800)<1e-5);QTemporaryDir saved;call(w,"document.save",{{"path",saved.path()+"/native-intersect.alloy"}});w.close();
 }
};
QTEST_MAIN(IntersectControls)
#include "intersect_controls.moc"
