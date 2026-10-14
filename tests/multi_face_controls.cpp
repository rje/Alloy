// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include "multi_face_fixtures.h"
#include <QApplication>
#include <QTest>
#include <QAction>
#include <QToolButton>
#include <QPushButton>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QComboBox>
#include <QListWidget>
#include <QLabel>
#include <QScrollArea>
#include <QLayout>
#include <QMap>
#include <QMouseEvent>
#include <QFile>
#include <QDir>
#include <QTemporaryDir>
#include <QJsonDocument>
#include <cmath>
// Focused synthetic Qt input against the real application/GPU, not OS input.
class MultiFaceControlsTest final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& w,const QString& name,const QJsonObject& a={}){const auto r=w.execute(name,a);if(!r["ok"].toBool())qFatal("%s",QJsonDocument(r).toJson().constData());return r["result"].toObject();}
    QJsonObject session(AlloyWindow& w){return w.state()["activeSession"].toObject();}
    QJsonObject inspect(AlloyWindow& w){return call(w,"document.inspect");}
    void motion(ReferenceView* view,const QPoint& point,Qt::MouseButtons buttons=Qt::NoButton){QMouseEvent e(QEvent::MouseMove,QPointF(point),QPointF(view->mapToGlobal(point)),Qt::NoButton,buttons,Qt::NoModifier);QApplication::sendEvent(view,&e);}
    void capture(AlloyWindow& w,const QString& folder,const QString& name){QApplication::processEvents();QTest::qWait(40);call(w,"capture.screenshot",{{"path",folder+"/"+name+".png"}});QFile file(folder+"/"+name+".json");if(!file.open(QIODevice::WriteOnly))qFatal("Evidence file");auto state=w.state();QJsonObject layout;auto bounds=[](const QRect& r){return QJsonObject{{"x",r.x()},{"y",r.y()},{"width",r.width()},{"height",r.height()}};};for(const auto& objectName:QStringList{"sketch.info","sketch.name","sketchScrollContent"})if(auto* widget=w.findChild<QWidget*>(objectName)){QJsonObject info{{"geometry",bounds(widget->geometry())},{"sizeHint",QJsonArray{widget->sizeHint().width(),widget->sizeHint().height()}},{"minimumSizeHint",QJsonArray{widget->minimumSizeHint().width(),widget->minimumSizeHint().height()}},{"heightForWidth",widget->heightForWidth(widget->width())}};if(widget->layout())info["layoutGeometry"]=bounds(widget->layout()->geometry());layout[objectName]=info;}state["validationLayout"]=layout;file.write(QJsonDocument(state).toJson(QJsonDocument::Indented));}
private slots:
    void sideInclinedSketchJoinAndSupportRecovery() {
        QTemporaryDir scratch;QVERIFY(scratch.isValid());auto fixtures=qEnvironmentVariable("ALLOY_MULTI_FACE_FIXTURES"),evidence=qEnvironmentVariable("ALLOY_MULTI_FACE_EVIDENCE");if(fixtures.isEmpty()){fixtures=scratch.path()+"/fixtures";MultiFaceFixtures::create(fixtures);}if(evidence.isEmpty())evidence=scratch.path()+"/evidence";QDir().mkpath(evidence);QFile file(fixtures+"/fixtures.json");QVERIFY(file.open(QIODevice::ReadOnly));const auto metadata=QJsonDocument::fromJson(file.readAll()).object();
        AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1360,860);w.show();w.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&w));ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(child))view=v;QVERIFY(view);
        const auto selectedCase=qEnvironmentVariable("ALLOY_MULTI_FACE_CASE");
        for(const auto& name:QStringList{"plate40","inclined"}) {
            if(!selectedCase.isEmpty() && name!=selectedCase)continue;
            call(w,"document.open",{{"path",fixtures+"/"+name+".alloy"}});const auto before=inspect(w);const auto body=before["evaluation"].toObject()["bodies"].toArray()[0].toObject();QJsonObject face;for(const auto& f:body["supportFaces"].toArray()){const auto n=f.toObject()["frame"].toObject()["normal"].toArray();if(name=="plate40" ? n[1].toDouble()<-.99 : std::abs(n[0].toDouble())>.1 && std::abs(n[2].toDouble())>.1)face=f.toObject();}QVERIFY(!face.isEmpty());QJsonObject ref;for(auto key:{"bodyId","producerFeatureId","faceId"})ref[key]=face[key];
            call(w,"view.home");if(name=="plate40")call(w,"view.orientation",{{"name","front"}});const auto p=call(w,"view.projectFacePoint",{{"support",ref},{"uMm",14},{"vMm",2.5}});const QPoint hit(qRound(p["x"].toDouble()),qRound(p["y"].toDouble()));motion(view,hit);QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,hit);QCOMPARE(w.state()["selection"].toObject()["support"].toObject(),ref);auto* create=w.findChild<QToolButton*>("command.sketch.create");QVERIFY(create);QTest::mouseClick(create,Qt::LeftButton);QTRY_COMPARE(session(w)["command"].toString(),QString("sketch.edit"));QCOMPARE(session(w)["inputs"].toObject()["support"].toObject(),ref);const auto sketchFrame=session(w)["resolvedFrame"].toObject();QCOMPARE(sketchFrame["uAxis"],face["frame"].toObject()["uAxis"]);QCOMPARE(sketchFrame["vAxis"],face["frame"].toObject()["vAxis"]);const auto oldOrigin=face["frame"].toObject()["origin"].toArray(),newOrigin=sketchFrame["origin"].toArray(),uAxis=sketchFrame["uAxis"].toArray(),vAxis=sketchFrame["vAxis"].toArray();double uShift=0,vShift=0;for(int k=0;k<3;++k){const double delta=oldOrigin[k].toDouble()-newOrigin[k].toDouble();uShift+=delta*uAxis[k].toDouble();vShift+=delta*vAxis[k].toDouble();}
            auto pixel=[&](double u,double v){const auto q=call(w,"view.projectSketchPoint",{{"sketchId",session(w)["draftSketch"].toObject()["id"]},{"uMm",u},{"vMm",v}});return QPoint(qRound(q["x"].toDouble()),qRound(q["y"].toDouble()));};
            auto* rectangle=w.findChild<QAction*>("sketch.tool.rectangle");QVERIFY(rectangle);rectangle->trigger();const double width=name=="plate40" ? 8:6;const auto start=pixel(10+uShift,1+vShift),end=pixel(10+width+uShift,4+vShift);const auto expected=view->sketchPoint(start.x(),start.y(),"face",sketchFrame);QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,start);QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,end);const auto initial=session(w)["draftSketch"].toObject();QCOMPARE(initial["parameters"].toObject()["entities"].toArray().size(),4);const auto first=initial["parameters"].toObject()["points"].toArray()[0].toObject();QVERIFY(std::abs(first["uMm"].toDouble()-expected["uMm"].toDouble())<1e-8 && std::abs(first["vMm"].toDouble()-expected["vMm"].toDouble())<1e-8);auto* look=w.findChild<QToolButton*>("command.sketch.lookAt");QVERIFY(look);QTest::mouseClick(look,Qt::LeftButton);
            for(int axis=0;axis<2;++axis){view->setFocus();QTest::keyClick(view,Qt::Key_D);QTRY_COMPARE(session(w)["tool"].toString(),QString("dimension"));const auto parameters=session(w)["draftSketch"].toObject()["parameters"].toObject();QMap<QString,QPointF> points;for(const auto& p:parameters["points"].toArray())points[p.toObject()["id"].toString()]={p.toObject()["uMm"].toDouble(),p.toObject()["vMm"].toDouble()};const auto edge=parameters["entities"].toArray()[axis].toObject();const auto middle=(points[edge["startPointId"].toString()]+points[edge["endPointId"].toString()])/2;const auto location=pixel(middle.x(),middle.y());QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,location);if(session(w)["selection"].toObject()["entityIds"].toArray().size()!=1)capture(w,evidence,name+"-dimension-selection-error");QCOMPARE(session(w)["selection"].toObject()["entityIds"].toArray().size(),1);const auto label=axis==0 ? pixel(middle.x(),middle.y()+1.5):pixel(middle.x()+1.5,middle.y());motion(view,label);QTRY_COMPARE(session(w)["pendingDimension"].toObject()["phase"].toString(),QString("placing"));QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,label);auto* edit=w.findChild<QLineEdit*>("sketch.inlineValue");QVERIFY(edit && edit->isVisible());edit->selectAll();QTest::keyClicks(edit,axis==0 ? QString::number(width):QString("3"));QTest::keyClick(edit,Qt::Key_Return);QTRY_VERIFY(!session(w).contains("pendingDimension"));}
            const auto sketch=session(w)["draftSketch"].toObject();QCOMPARE(sketch["parameters"].toObject()["dimensions"].toArray().size(),2);const auto geometry=call(w,"command.preview",{{"session",session(w)["id"]}})["sketchGeometry"].toObject();QVERIFY(std::abs(geometry["profiles"].toArray()[0].toObject()["areaMm2"].toDouble()-width*3)<1e-6);capture(w,evidence,name+"-rectangle");auto* finish=w.findChild<QPushButton*>("sketch.finish");QVERIFY(finish);QTest::mouseClick(finish,Qt::LeftButton);QTRY_VERIFY(session(w).isEmpty());
            auto* extrude=w.findChild<QToolButton*>("command.solid.extrudeDialog");QVERIFY(extrude);QTest::mouseClick(extrude,Qt::LeftButton);QTRY_COMPARE(session(w)["command"].toString(),QString("solid.extrude"));const auto profilePoint=call(w,"view.projectSketchPoint",{{"sketchId",sketch["id"]},{"uMm",10+width/2+uShift},{"vMm",2.5+vShift}});QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,QPoint(qRound(profilePoint["x"].toDouble()),qRound(profilePoint["y"].toDouble())));QCOMPARE(session(w)["inputs"].toObject()["sketchId"],sketch["id"]);auto* dock=w.findChild<QDockWidget*>("extrudeDock");QVERIFY(dock);auto* operation=dock->findChild<QComboBox*>("extrude.operation");QVERIFY(operation);operation->setFocus();QTest::keyClick(operation,Qt::Key_Home);QTest::keyClick(operation,Qt::Key_Down);QCOMPARE(operation->currentData().toString(),QString("join"));auto* target=dock->findChild<QComboBox*>("extrude.targetBody");QVERIFY(target && target->isVisible());target->setFocus();QTest::keyClick(target,Qt::Key_Home);QTest::keyClick(target,Qt::Key_Down);QCOMPARE(target->currentData().toJsonArray(),QJsonArray{body["bodyId"]});auto* distance=dock->findChild<QDoubleSpinBox*>("extrude.distance");QVERIFY(distance);distance->setFocus();QTest::keyClick(distance,Qt::Key_Up);QTRY_VERIFY(session(w)["previewed"].toBool());QCOMPARE(session(w)["inputs"].toObject()["targetBodyIds"].toArray(),QJsonArray{body["bodyId"]});capture(w,evidence,name+"-join-preview");auto* buttons=dock->findChild<QDialogButtonBox*>();QVERIFY(buttons && buttons->button(QDialogButtonBox::Ok)->isEnabled());const int depth=name=="plate40" ? 4:2;for(int step=1;step<depth;++step)QTest::keyClick(distance,Qt::Key_Up);QCOMPARE(session(w)["inputs"].toObject()["distance"].toDouble(),double(depth));capture(w,evidence,name+"-join-preview");QTest::mouseClick(buttons->button(QDialogButtonBox::Ok),Qt::LeftButton);QTRY_VERIFY(session(w).isEmpty());const auto after=inspect(w);QCOMPARE(after["evaluation"].toObject()["bodies"].toArray().size(),1);const auto result=after["evaluation"].toObject()["bodies"].toArray()[0].toObject();QCOMPARE(result["bodyId"],body["bodyId"]);QVERIFY(std::abs(result["volumeMm3"].toDouble()-body["volumeMm3"].toDouble()-(name=="plate40" ? 96:36))<1e-5);call(w,"view.home");capture(w,evidence,name+"-joined");call(w,"document.save",{{"path",evidence+"/"+name+"-native.alloy"}});
        }
        if(qEnvironmentVariable("ALLOY_MULTI_FACE_CASE")=="recovery") {
        call(w,"document.open",{{"path",fixtures+"/support-failure.alloy"}});const auto failure=metadata["failure"].toObject();const auto before=inspect(w);
        for(auto key:{"splitBodyId","deleteBodyId"}){call(w,"feature.editDialog",{{"featureId",failure["combineFeatureId"]}});auto* tools=w.findChild<QListWidget*>("combine.toolBodies");QVERIFY(tools);auto toggle=[&](const QString& id) {
                auto findItem=[&]() -> QListWidgetItem* {
                    for(int i=0;i<tools->count();++i)
                        if(tools->item(i)->data(Qt::UserRole).toString()==id)return tools->item(i);
                    return nullptr;
                };
                auto* item=findItem();if(!item)return false;
                const bool expected=item->checkState()!=Qt::Checked;
                for(auto* area:w.findChildren<QScrollArea*>())
                    if(area->isAncestorOf(tools))area->ensureWidgetVisible(tools);
                tools->scrollToItem(item);QApplication::processEvents();
                // Use the ordinary current-row/Space gesture. Hand-built style
                // options do not establish the real delegate checkbox hit box.
                QTest::mouseClick(tools->viewport(),Qt::LeftButton,Qt::NoModifier,tools->visualItemRect(item).center());
                if(!tools->currentItem() || tools->currentItem()->data(Qt::UserRole).toString()!=id)return false;
                tools->setFocus();QTest::keyClick(tools,Qt::Key_Space);
                for(int wait=0;wait<50;++wait) {
                    QTest::qWait(20);item=findItem(); // Preview refresh rebuilds the list.
                    if(item && (item->checkState()==Qt::Checked)==expected &&
                        session(w)["inputs"].toObject()["toolBodyIds"].toArray().contains(id)==expected)return true;
                }
                capture(w,evidence,QString(key)+"-toggle-failed");return false;
            };QVERIFY(toggle(failure["notchBodyId"].toString()));QVERIFY(toggle(failure[key].toString()));capture(w,evidence,QString(key)+"-selection");QTRY_COMPARE_WITH_TIMEOUT(session(w)["previewError"].toObject()["diagnostics"].toObject()["code"].toString(),QString("support_unavailable"),3000);QCOMPARE(session(w)["previewError"].toObject()["diagnostics"].toObject()["featureId"],failure["sketchId"]);auto* label=w.findChild<QLabel*>("combine.error");QVERIFY(label && label->isVisible() && !label->text().isEmpty());QVERIFY(!w.findChild<QPushButton*>("combine.ok")->isEnabled());for(auto* area:w.findChildren<QScrollArea*>())if(area->isAncestorOf(label))area->ensureWidgetVisible(label,0,16);QCOMPARE(inspect(w)["authored"],before["authored"]);QCOMPARE(inspect(w)["revision"],before["revision"]);QCOMPARE(inspect(w)["undo"],before["undo"]);capture(w,evidence,QString(key)+"-visible-error");
            if(QString(key)=="splitBodyId"){auto* cancel=w.findChild<QPushButton*>("combine.cancel");QVERIFY(cancel);QTest::mouseClick(cancel,Qt::LeftButton);}else{QVERIFY(toggle(failure[key].toString()));QVERIFY(toggle(failure["notchBodyId"].toString()));QTRY_VERIFY(session(w)["previewed"].toBool());auto* ok=w.findChild<QPushButton*>("combine.ok");QVERIFY(ok && ok->isEnabled());QTest::mouseClick(ok,Qt::LeftButton);}QTRY_VERIFY(session(w).isEmpty());QCOMPARE(inspect(w)["authored"],before["authored"]);call(w,"view.home");capture(w,evidence,QString(key)+"-recovered");}
        }
        // Reopen the already-completed native artifacts; settle deferred Timeline layout.
        for(const auto& name:QStringList{"plate40","inclined"})if(QFile::exists(evidence+"/"+name+"-native.alloy")){call(w,"document.open",{{"path",evidence+"/"+name+"-native.alloy"}});call(w,"view.home");capture(w,evidence,name+"-final-visible-timeline");}
    }
};
QTEST_MAIN(MultiFaceControlsTest)
#include "multi_face_controls.moc"
