// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QTest>
#include <QAction>
#include <QMenu>
#include <QToolButton>
#include <QTimer>
#include <QLineEdit>
#include <QPushButton>
#include <QMouseEvent>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <cmath>
class StandalonePointControlsTest final:public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& w,const QString& command,const QJsonObject& args={}) {
        const auto r=w.execute(command,args);if(!r["ok"].toBool())qFatal("%s",QJsonDocument(r).toJson().constData());return r["result"].toObject();
    }
    QJsonObject session(AlloyWindow& w){return w.state()["activeSession"].toObject();}
private slots:
    void pointPlacementThenTwoDimensionsWithoutFocusRepair() {
        AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1280,860);w.show();w.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&w));
        ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(child))view=v;QVERIFY(view);
        // API setup is separate from the demonstrated native Point/D sequence.
        call(w,"sketch.begin",{{"plane","XY"}});call(w,"sketch.centerRectangle",{{"centerX",0},{"centerY",0},{"width",75},{"height",50},{"units","mm"}});call(w,"sketch.finish");
        const auto base=call(w,"document.inspect")["evaluation"].toObject()["sketches"].toArray()[0].toObject();
        call(w,"solid.extrude",{{"sketchId",base["sketchId"]},{"profileId",base["profiles"].toArray()[0].toObject()["profileId"]},{"distance",10},{"units","mm"}});
        auto body=call(w,"document.inspect")["evaluation"].toObject()["bodies"].toArray()[0].toObject();QJsonArray corners;
        for(const auto& e:body["solidEdges"].toArray())if(e.toObject()["provenance"].toObject()["role"]=="sideCorner")corners.append(e.toObject()["edgeId"]);
        call(w,"solid.fillet",{{"targetBodyId",body["bodyId"]},{"edgeIds",corners},{"radiusMm",5}});
        body=call(w,"document.inspect")["evaluation"].toObject()["bodies"].toArray()[0].toObject();QJsonObject support;
        for(const auto& f:body["supportFaces"].toArray())if(f.toObject()["role"]=="extrude.endCap")for(auto key:{"bodyId","producerFeatureId","faceId"})support[key]=f.toObject()[key];
        call(w,"sketch.begin",{{"support",support}});call(w,"view.orientation",{{"name","top"}});
        const auto sketchId=session(w)["draftSketch"].toObject()["id"].toString();
        auto pixel=[&](double u,double v){const auto p=call(w,"view.projectSketchPoint",{{"sketchId",sketchId},{"uMm",u},{"vMm",v}});return QPoint(qRound(p["x"].toDouble()),qRound(p["y"].toDouble()));};
        view->setFocus();QTest::qWait(100); // Initial harness activation only.
        auto* action=w.findChild<QAction*>("sketch.tool.point");QVERIFY(action);
        auto* create=w.findChild<QToolButton*>("sketch.menu.create");QVERIFY(create);auto* menu=create->menu();QVERIFY(menu);
        bool menuVisible=false;
        // InstantPopup enters QMenu's nested event loop during mouse press.
        // Queue the actual menu click so it executes inside that loop.
        QTimer::singleShot(100,&w,[&]{menuVisible=menu->isVisible();if(menuVisible)QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,menu->actionGeometry(action).center());});
        QTest::mouseClick(create,Qt::LeftButton);QVERIFY(menuVisible);
        QCOMPARE(session(w)["tool"].toString(),QString("point"));QVERIFY(view->hasFocus());
        QTest::mouseMove(view,pixel(-26,14));QCOMPARE(session(w)["draftSketch"].toObject()["parameters"].toObject()["entities"].toArray().size(),0);
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(-26,14));
        const auto entity=session(w)["draftSketch"].toObject()["parameters"].toObject()["entities"].toArray().first().toObject();const auto pointId=entity["pointId"].toString();
        QCOMPARE(entity["type"].toString(),QString("point"));QVERIFY(!pointId.isEmpty());QVERIFY(view->hasFocus());QVERIFY(!session(w).contains("firstPoint"));
        QCOMPARE(session(w)["solverDiagnostics"].toObject()["dof"].toInt(-1),2);
        QTest::keyClick(view,Qt::Key_D);QTRY_COMPARE(session(w)["tool"].toString(),QString("dimension"));QVERIFY(view->hasFocus());
        auto current=[&]{for(const auto& p:session(w)["draftSketch"].toObject()["parameters"].toObject()["points"].toArray())if(p.toObject()["id"]==pointId)return QPointF(p.toObject()["uMm"].toDouble(),p.toObject()["vMm"].toDouble());return QPointF{};};
        for(int i=0;i<2;++i) {
            const auto c=current();QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(c.x(),c.y()));
            QCOMPARE(session(w)["selection"].toObject()["pointIds"].toArray(),QJsonArray{pointId});
            QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,i==0 ? pixel(-12,25) : pixel(-37.5,4));
            QCOMPARE(session(w)["selection"].toObject()["pointIds"].toArray().size(),1);QCOMPARE(session(w)["selection"].toObject()["entityIds"].toArray().size(),1);
            const auto place=i==0 ? pixel(-33,20) : pixel(-32,8);
            QMouseEvent move(QEvent::MouseMove,QPointF(place),QPointF(view->mapToGlobal(place)),Qt::NoButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(view,&move);
            QTRY_COMPARE(session(w)["pendingDimension"].toObject()["phase"].toString(),QString("placing"));QCOMPARE(session(w)["pendingDimension"].toObject()["kind"].toString(),QString("pointLineDistance"));
            QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,place);auto* editor=w.findChild<QLineEdit*>("sketch.inlineValue");QVERIFY(editor);QVERIFY(editor->isVisible());QVERIFY(editor->hasFocus());
            editor->selectAll();QTest::keyClicks(editor,"10");QTest::keyClick(editor,Qt::Key_Return);QTRY_VERIFY(!session(w).contains("pendingDimension"));QVERIFY(view->hasFocus());
            QCOMPARE(session(w)["solverDiagnostics"].toObject()["dof"].toInt(-1),1-i);
        }
        const auto c=current();QVERIFY(std::abs(c.x()+27.5)<1e-6 && std::abs(c.y()-15)<1e-6);
        const auto constrained=session(w)["draftSketch"].toObject();QCOMPARE(constrained["parameters"].toObject()["entities"].toArray().size(),1);QCOMPARE(session(w)["profileSummaries"].toArray().size(),0);
        const auto evidence=qEnvironmentVariable("ALLOY_STANDALONE_POINT_EVIDENCE");
        if(!evidence.isEmpty()){QDir().mkpath(evidence);call(w,"capture.screenshot",{{"path",evidence+"/native-two-edge-point.png"}});QFile state(evidence+"/native-state.json");QVERIFY(state.open(QIODevice::WriteOnly));state.write(QJsonDocument(w.state()).toJson(QJsonDocument::Indented));}
        auto* finish=w.findChild<QPushButton*>("sketch.finish");QVERIFY(finish);QTest::mouseClick(finish,Qt::LeftButton);QTRY_VERIFY(session(w).isEmpty());
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(-27.5,15));
        const auto selected=w.state()["selection"].toObject();QCOMPARE(selected["kind"].toString(),QString("sketchPoint"));QCOMPARE(selected["pointId"].toString(),pointId);QCOMPARE(selected["entityId"],entity["id"]);QCOMPARE(selected["sketchId"].toString(),sketchId);
        if(!evidence.isEmpty()){call(w,"capture.screenshot",{{"path",evidence+"/native-finished-selected-point.png"}});QFile state(evidence+"/finished-state.json");QVERIFY(state.open(QIODevice::WriteOnly));state.write(QJsonDocument(w.state()).toJson(QJsonDocument::Indented));}
        // Tool Escape discards its cursor; accepted points remain in the draft.
        call(w,"sketch.edit",{{"sketchId",sketchId}});action->trigger();QVERIFY(view->hasFocus());QTest::keyClick(view,Qt::Key_Escape);QCOMPARE(session(w)["tool"].toString(),QString("select"));QCOMPARE(session(w)["draftSketch"].toObject(),constrained);
        call(w,"sketch.cancel");w.hide();
    }
};
QTEST_MAIN(StandalonePointControlsTest)
#include "standalone_point_controls.moc"
