// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QTest>
#include <QToolButton>
#include <QPushButton>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QMouseEvent>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <numbers>
#include <cmath>
// Real Qt pointer input, shared application authority and native GPU. No OS-input claim.
class RevolveControlsTest final:public QObject{
    Q_OBJECT
    QJsonObject call(AlloyWindow& w,const QString& c,const QJsonObject& a={}){const auto r=w.execute(c,a);if(!r["ok"].toBool())qFatal("%s",QJsonDocument(r).toJson().constData());return r["result"].toObject();}
    QJsonObject session(AlloyWindow& w){return w.state()["activeSession"].toObject();}
    void capture(AlloyWindow& w,const QString& folder,const QString& name){QApplication::processEvents();QTest::qWait(40);call(w,"capture.screenshot",{{"path",folder+"/"+name+".png"}});QFile file(folder+"/"+name+".json");if(!file.open(QIODevice::WriteOnly))qFatal("evidence");file.write(QJsonDocument(w.state()).toJson(QJsonDocument::Indented));}
private slots:
    void autoManualAngularCancelFullEdit(){
        QTemporaryDir tmp;auto output=qEnvironmentVariable("ALLOY_REVOLVE_EVIDENCE");if(output.isEmpty())output=tmp.path();QDir().mkpath(output);AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1400,900);w.show();w.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&w));ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(child))view=v;QVERIFY(view);
        call(w,"sketch.create",{{"plane","XZ"},{"name","Radial fixture"}});call(w,"sketch.rectangle",{{"x1",10},{"y1",0},{"x2",20},{"y2",15},{"units","mm"}});call(w,"sketch.line",{{"x1",0},{"y1",-5},{"x2",0},{"y2",20},{"units","mm"}});const auto draft=session(w)["draftSketch"].toObject();const auto axis=draft["parameters"].toObject()["entities"].toArray().last().toObject();call(w,"sketch.centerline",{{"entityIds",QJsonArray{axis["id"]}},{"centerline",true}});call(w,"sketch.finish");call(w,"view.home");const auto before=call(w,"document.inspect");auto* launch=w.findChild<QToolButton*>("command.solid.revolveDialog");QVERIFY(launch);QTest::mouseClick(launch,Qt::LeftButton);QTRY_VERIFY(session(w)["previewed"].toBool());QCOMPARE(session(w)["inputs"].toObject()["axisEntityId"],axis["id"]);QCOMPARE(session(w)["selectionRole"].toString(),QString("ready"));capture(w,output,"automatic-full");
        // Explicit role buttons and actual finished profile/line pointer picking.
        auto* clear=w.findChild<QPushButton*>("revolve.profile.clear");QVERIFY(clear);QTest::mouseClick(clear,Qt::LeftButton);QCOMPARE(session(w)["selectionRole"].toString(),QString("profile"));const auto region=call(w,"view.projectSketchPoint",{{"sketchId",draft["id"]},{"uMm",15},{"vMm",7}});QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,QPoint(qRound(region["x"].toDouble()),qRound(region["y"].toDouble())));QTRY_COMPARE(session(w)["selectionRole"].toString(),QString("axis"));const auto axisPoint=call(w,"view.projectSketchPoint",{{"sketchId",draft["id"]},{"uMm",0},{"vMm",15}});QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,QPoint(qRound(axisPoint["x"].toDouble()),qRound(axisPoint["y"].toDouble())));QTRY_VERIFY(session(w)["previewed"].toBool());QCOMPARE(session(w)["inputs"].toObject()["axisEntityId"],axis["id"]);capture(w,output,"manual-profile-axis");
        auto* extent=w.findChild<QComboBox*>("revolve.extent");QVERIFY(extent);extent->setFocus();QTest::keyClick(extent,Qt::Key_End);QTRY_COMPARE(session(w)["inputs"].toObject()["extent"].toString(),QString("partial"));QApplication::processEvents();QTest::qWait(60);capture(w,output,"partial-before-drag");const auto cue=view->revolveCueState();QVERIFY(cue["dragAvailable"].toBool());auto point=[](const QJsonObject& p){return QPointF(p["x"].toDouble(),p["y"].toDouble());};const auto center=point(cue["centerScreen"].toObject()),u=point(cue["basisUScreen"].toObject())-center,v=point(cue["basisVScreen"].toObject())-center;const double angle=70*std::numbers::pi/180.;const QPoint start=point(cue["endScreen"].toObject()).toPoint(),target=(center+u*std::cos(angle)+v*std::sin(angle)).toPoint();QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,start);QMouseEvent move(QEvent::MouseMove,QPointF(target),QPointF(view->mapToGlobal(target)),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(view,&move);QTest::mouseRelease(view,Qt::LeftButton,Qt::NoModifier,target);QTRY_VERIFY(session(w)["previewed"].toBool());capture(w,output,"after-drag-diagnostics");const auto measured=session(w)["inputs"].toObject()["angleDeg"].toDouble();qInfo()<<"Derived drag angle"<<measured<<"start"<<start<<"target"<<target;QVERIFY(std::abs(measured-70)<1.5);QCOMPARE(session(w)["angularDrag"].toObject()["phase"].toString(),QString("end"));const auto afterCue=view->revolveCueState();for(const auto& key:{"centerScreen","basisUScreen","basisVScreen"})QCOMPARE(afterCue[key],cue[key]);QVERIFY(std::abs(session(w)["geometry"].toObject()["volumeMm3"].toDouble()-4500*std::numbers::pi*measured/360)<1e-5);capture(w,output,"native-angular-partial");
        auto* box=w.findChild<QDockWidget*>("revolveDock")->findChild<QDialogButtonBox*>();QTest::mouseClick(box->button(QDialogButtonBox::Cancel),Qt::LeftButton);QTRY_VERIFY(session(w).isEmpty());QCOMPARE(call(w,"document.inspect")["authored"],before["authored"]);QCOMPARE(call(w,"document.inspect")["undo"],before["undo"]);
        QTest::mouseClick(launch,Qt::LeftButton);QTRY_VERIFY(session(w)["previewed"].toBool());QCOMPARE(session(w)["inputs"].toObject()["angleDeg"].toDouble(),360.);box=w.findChild<QDockWidget*>("revolveDock")->findChild<QDialogButtonBox*>();QTest::mouseClick(box->button(QDialogButtonBox::Cancel),Qt::LeftButton);QTRY_VERIFY(session(w).isEmpty());QCOMPARE(call(w,"document.inspect")["authored"],before["authored"]);QCOMPARE(call(w,"document.inspect")["undo"],before["undo"]);
        QTest::mouseClick(launch,Qt::LeftButton);QTRY_VERIFY(session(w)["previewed"].toBool());extent=w.findChild<QComboBox*>("revolve.extent");extent->setFocus();QTest::keyClick(extent,Qt::Key_End);QTest::keyClick(extent,Qt::Key_Home);QCOMPARE(session(w)["inputs"].toObject()["angleDeg"].toDouble(),360.);box=w.findChild<QDockWidget*>("revolveDock")->findChild<QDialogButtonBox*>();QVERIFY(box->button(QDialogButtonBox::Ok)->isEnabled());QTest::mouseClick(box->button(QDialogButtonBox::Ok),Qt::LeftButton);QTRY_VERIFY(session(w).isEmpty());const auto accepted=call(w,"document.inspect");QCOMPARE(accepted["authored"].toObject()["features"].toArray().size(),2);QVERIFY(std::abs(accepted["evaluation"].toObject()["bodies"].toArray()[0].toObject()["volumeMm3"].toDouble()-4500*std::numbers::pi)<1e-5);capture(w,output,"native-full-committed");
        const auto feature=accepted["authored"].toObject()["features"].toArray().last().toObject();call(w,"feature.editDialog",{{"featureId",feature["id"]}});QTRY_VERIFY(session(w)["previewed"].toBool());capture(w,output,"edit-full");box=w.findChild<QDockWidget*>("revolveDock")->findChild<QDialogButtonBox*>();QTest::mouseClick(box->button(QDialogButtonBox::Cancel),Qt::LeftButton);call(w,"history.undo");QCOMPARE(call(w,"document.inspect")["authored"],before["authored"]);call(w,"history.redo");QCOMPARE(call(w,"document.inspect")["authored"],accepted["authored"]);const auto path=output+"/native-revolve.alloy";call(w,"document.save",{{"path",path}});call(w,"document.open",{{"path",path}});QCOMPARE(call(w,"document.inspect")["authored"],accepted["authored"]);
    }
};
QTEST_MAIN(RevolveControlsTest)
#include "revolve_controls.moc"
