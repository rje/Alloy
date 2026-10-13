// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QTest>
#include <QDoubleSpinBox>
#include <QDockWidget>
#include <QContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QJsonDocument>
#include <cmath>
// Synthesized Qt input against actual GPU application; physical OS input separate.
class SphereControlsTest final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& w,const QString& command,const QJsonObject& args={}){const auto r=w.execute(command,args);if(!r["ok"].toBool())qFatal("%s",QJsonDocument(r).toJson().constData());return r["result"].toObject();}
    QJsonObject session(AlloyWindow& w){return w.state()["activeSession"].toObject();}
private slots:
    void faceCenterDiameterEnterRepeatCancel() {
        AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1180,760);w.show();w.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&w));ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(child))view=v;QVERIFY(view);
        call(w,"sketch.begin",{{"plane","XY"}});call(w,"sketch.rectangle",{{"x1",-10},{"y1",-10},{"x2",10},{"y2",10},{"units","mm"}});call(w,"sketch.finish");const auto sketch=call(w,"document.inspect")["evaluation"].toObject()["sketches"].toArray()[0].toObject();call(w,"solid.extrude",{{"sketchId",sketch["sketchId"]},{"profileId",sketch["profiles"].toArray()[0].toObject()["profileId"]},{"distance",20},{"units","mm"}});
        const auto stock=call(w,"document.inspect");QJsonObject face;for(const auto& value:stock["evaluation"].toObject()["bodies"].toArray()[0].toObject()["supportFaces"].toArray())if(value.toObject()["frame"].toObject()["normal"].toArray()[2].toDouble()>.99)face=value.toObject();QVERIFY(!face.isEmpty());
        QJsonObject ref;for(const auto& key:{"bodyId","producerFeatureId","faceId"})ref[key]=face[key];const auto frame=face["frame"].toObject();
        auto location=[&](double x,double y){QJsonObject args{{"support",ref}};const auto origin=frame["origin"].toArray(),u=frame["uAxis"].toArray(),v=frame["vAxis"].toArray();double um=0,vm=0;const double world[3]={x,y,20};for(int k=0;k<3;++k){um+=(world[k]-origin[k].toDouble())*u[k].toDouble();vm+=(world[k]-origin[k].toDouble())*v[k].toDouble();}args["uMm"]=um;args["vMm"]=vm;const auto pixel=call(w,"view.projectFacePoint",args);return QPoint(qRound(pixel["x"].toDouble()),qRound(pixel["y"].toDouble()));};
        call(w,"view.orientation",{{"name","top"}});call(w,"view.home");call(w,"solid.sphereDialog");QTest::qWait(80);QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,location(0,0));QCOMPARE(session(w)["selectionRole"].toString(),QString("center"));QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,location(0,0));QVERIFY(session(w)["previewed"].toBool());auto* dock=w.findChild<QDockWidget*>("sphereDock");QVERIFY(dock);auto* diameter=dock->findChild<QDoubleSpinBox*>("sphereDiameter");QVERIFY(diameter);diameter->setFocus();diameter->selectAll();QTest::keyClicks(diameter,"4");QTest::keyClick(diameter,Qt::Key_Return);QTRY_VERIFY(session(w).isEmpty());const auto first=call(w,"document.inspect");QCOMPARE(first["authored"].toObject()["features"].toArray().size(),3);QCOMPARE(first["evaluation"].toObject()["bodies"].toArray()[0].toObject()["diameterMm"].toDouble(),4.0);
        const auto pixel=location(5,5);QContextMenuEvent context(QContextMenuEvent::Mouse,pixel,view->mapToGlobal(pixel));QApplication::sendEvent(view,&context);QTRY_VERIFY(QApplication::activePopupWidget()!=nullptr);auto* menu=qobject_cast<QMenu*>(QApplication::activePopupWidget());QVERIFY(menu);QAction* repeat=nullptr;for(auto* action:menu->actions())if(action->objectName()=="context.repeatSphere")repeat=action;QVERIFY(repeat);QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,menu->actionGeometry(repeat).center());QTRY_COMPARE(session(w)["command"].toString(),QString("solid.sphere"));QCOMPARE(session(w)["inputs"].toObject()["support"].toObject(),ref);QVERIFY(!session(w)["inputs"].toObject().contains("uMm"));QCOMPARE(session(w)["inputs"].toObject()["diameterMm"].toDouble(),4.0);view->setFocus();QTest::keyClick(view,Qt::Key_Escape);QTRY_VERIFY(session(w).isEmpty());QCOMPARE(call(w,"document.inspect")["authored"].toObject(),first["authored"].toObject());
    }
};
QTEST_MAIN(SphereControlsTest)
#include "sphere_controls.moc"
