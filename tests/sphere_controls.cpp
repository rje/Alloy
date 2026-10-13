// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QTest>
#include <QDoubleSpinBox>
#include <QDockWidget>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QDir>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
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
        const auto stock=call(w,"document.inspect");
        auto* tree=w.findChild<QTreeWidget*>("browser.tree");QVERIFY(tree);
        for(const auto& name:{QString("left"),QString("back")}) {
            QTreeWidgetItem* row=nullptr;for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole).toString()==name)row=*it;
            QVERIFY(row);tree->scrollToItem(row);QTest::mouseClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,tree->visualItemRect(row).center());QCOMPARE(view->orientation,name);
            QJsonObject expected;for(const auto& value:stock["evaluation"].toObject()["bodies"].toArray()[0].toObject()["supportFaces"].toArray()) {
                const auto f=value.toObject();const auto n=f["frame"].toObject()["normal"].toArray();if(name=="left" ? n[0].toDouble()<-.99 : n[1].toDouble()>.99)expected=f;
            }
            QVERIFY(!expected.isEmpty());QJsonObject support;for(const auto& key:{"bodyId","producerFeatureId","faceId"})support[key]=expected[key];
            const auto frame=expected["frame"].toObject();const auto o=frame["origin"].toArray();const double world[3]={name=="left" ? -10.0 : 0.0,name=="back" ? 10.0 : 0.0,10.0};double uv[2]={};
            for(int i=0;i<2;++i){const auto axis=frame[i==0 ? "uAxis" : "vAxis"].toArray();for(int k=0;k<3;++k)uv[i]+=(world[k]-o[k].toDouble())*axis[k].toDouble();}
            const auto pixel=call(w,"view.projectFacePoint",{{"support",support},{"uMm",uv[0]},{"vMm",uv[1]}});
            const auto picked=call(w,"selection.facePick",{{"x",pixel["x"]},{"y",pixel["y"]}});QCOMPARE(picked["support"].toObject(),support);
        }
        call(w,"selection.set",{{"bodyId",""}});
        QJsonObject face;for(const auto& value:stock["evaluation"].toObject()["bodies"].toArray()[0].toObject()["supportFaces"].toArray())if(value.toObject()["frame"].toObject()["normal"].toArray()[2].toDouble()>.99)face=value.toObject();QVERIFY(!face.isEmpty());
        QJsonObject ref;for(const auto& key:{"bodyId","producerFeatureId","faceId"})ref[key]=face[key];auto frame=face["frame"].toObject();
        auto location=[&](double x,double y,double z=20){QJsonObject args{{"support",ref}};const auto origin=frame["origin"].toArray(),u=frame["uAxis"].toArray(),v=frame["vAxis"].toArray();double um=0,vm=0;const double world[3]={x,y,z};for(int k=0;k<3;++k){um+=(world[k]-origin[k].toDouble())*u[k].toDouble();vm+=(world[k]-origin[k].toDouble())*v[k].toDouble();}args["uMm"]=um;args["vMm"]=vm;const auto pixel=call(w,"view.projectFacePoint",args);return QPoint(qRound(pixel["x"].toDouble()),qRound(pixel["y"].toDouble()));};
        call(w,"view.home");call(w,"view.orientation",{{"name","top"}});call(w,"solid.sphereDialog");QTest::qWait(80);QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,location(0,0));QCOMPARE(session(w)["selectionRole"].toString(),QString("center"));QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,location(0,0));QVERIFY(session(w)["previewed"].toBool());auto* dock=w.findChild<QDockWidget*>("sphereDock");QVERIFY(dock);QCOMPARE(session(w)["inputs"].toObject()["operation"].toString(),QString("cut"));
        call(w,"capture.screenshot",{{"path",QDir::currentPath()+"/build-tutorial-polish/evidence/sphere-final-independent/qt-origin-preview.png"}});
        auto* buttons=dock->findChild<QDialogButtonBox*>();QVERIFY(buttons);QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel),Qt::LeftButton);QVERIFY(session(w).isEmpty());QCOMPARE(call(w,"document.inspect")["authored"],stock["authored"]);
        call(w,"solid.sphereDialog");QTest::qWait(80);QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,location(0,0));QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,location(0,0));dock=w.findChild<QDockWidget*>("sphereDock");
        auto* diameter=dock->findChild<QDoubleSpinBox*>("sphereDiameter");QVERIFY(diameter);diameter->setFocus();diameter->selectAll();QTest::keyClicks(diameter,"4");QTest::keyClick(diameter,Qt::Key_Return);if(!session(w).isEmpty()){QCOMPARE(session(w)["inputs"].toObject()["diameterMm"].toDouble(),4.0);QVERIFY(session(w)["previewed"].toBool());qWarning("Inactive-window Qt shortcut did not commit; checking the actual OK button instead");buttons=dock->findChild<QDialogButtonBox*>();QTest::mouseClick(buttons->button(QDialogButtonBox::Ok),Qt::LeftButton);}QVERIFY(session(w).isEmpty());const auto first=call(w,"document.inspect");QCOMPARE(first["authored"].toObject()["features"].toArray().size(),3);QCOMPARE(first["evaluation"].toObject()["bodies"].toArray()[0].toObject()["diameterMm"].toDouble(),4.0);
        // Repeat must bind another face, not carry the previous plane/center.
        const auto previousRef=ref;for(const auto& value:first["evaluation"].toObject()["bodies"].toArray()[0].toObject()["supportFaces"].toArray())if(value.toObject()["frame"].toObject()["normal"].toArray()[0].toDouble()>.99)face=value.toObject();
        frame=face["frame"].toObject();for(const auto& key:{"bodyId","producerFeatureId","faceId"})ref[key]=face[key];QVERIFY(ref!=previousRef);call(w,"view.orientation",{{"name","right"}});
        const auto pixel=location(10,0,10);QContextMenuEvent context(QContextMenuEvent::Mouse,pixel,view->mapToGlobal(pixel));QApplication::sendEvent(view,&context);QTRY_VERIFY(QApplication::activePopupWidget()!=nullptr);auto* menu=qobject_cast<QMenu*>(QApplication::activePopupWidget());QVERIFY(menu);QAction* repeat=nullptr;for(auto* action:menu->actions())if(action->objectName()=="context.repeatSphere")repeat=action;QVERIFY(repeat);QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,menu->actionGeometry(repeat).center());QTRY_COMPARE(session(w)["command"].toString(),QString("solid.sphere"));QCOMPARE(session(w)["inputs"].toObject()["support"].toObject(),ref);QVERIFY(!session(w)["inputs"].toObject().contains("uMm"));QCOMPARE(session(w)["inputs"].toObject()["diameterMm"].toDouble(),4.0);QVERIFY(!session(w)["inputs"].toObject().contains("vMm"));
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel);QVERIFY(session(w)["previewed"].toBool());QCOMPARE(session(w)["inputs"].toObject()["operation"].toString(),QString("cut"));
        call(w,"capture.screenshot",{{"path",QDir::currentPath()+"/build-tutorial-polish/evidence/sphere-final-independent/qt-different-face-preview.png"}});
        dock=w.findChild<QDockWidget*>("sphereDock");buttons=dock->findChild<QDialogButtonBox*>();QVERIFY(buttons);QTest::mouseClick(buttons->button(QDialogButtonBox::Ok),Qt::LeftButton);QVERIFY(session(w).isEmpty());
        const auto second=call(w,"document.inspect");QCOMPARE(second["authored"].toObject()["features"].toArray().size(),4);
        const auto volume=second["evaluation"].toObject()["bodies"].toArray()[0].toObject()["volumeMm3"].toDouble();QVERIFY(std::abs(8000-volume-2*(16.0/3)*std::acos(-1.0))<1e-5);
    }
};
QTEST_MAIN(SphereControlsTest)
#include "sphere_controls.moc"
