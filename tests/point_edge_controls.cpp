// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QTest>
#include <QLineEdit>
#include <QMouseEvent>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <cmath>
// Synthetic Qt events against real application/GPU, distinct from physical input.
class PointEdgeControlsTest final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& w,const QString& command,const QJsonObject& args={}){const auto r=w.execute(command,args);if(!r["ok"].toBool())qFatal("%s",QJsonDocument(r).toJson().constData());return r["result"].toObject();}
    QJsonObject session(AlloyWindow& w){return w.state()["activeSession"].toObject();}
private slots:
    void toyCenterEdgePlacementAndValue() {
        AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1280,860);w.show();w.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&w));ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(child))view=v;QVERIFY(view);
        call(w,"sketch.begin",{{"plane","XY"}});call(w,"sketch.rectangle",{{"x1",0},{"y1",0},{"x2",31.8},{"y2",15.8},{"units","mm"}});call(w,"sketch.finish");const auto sketch=call(w,"document.inspect")["evaluation"].toObject()["sketches"].toArray()[0].toObject();call(w,"solid.extrude",{{"sketchId",sketch["sketchId"]},{"profileId",sketch["profiles"].toArray()[0].toObject()["profileId"]},{"distance",9.6},{"units","mm"}});
        QJsonObject face;for(const auto& f:call(w,"document.inspect")["evaluation"].toObject()["bodies"].toArray()[0].toObject()["supportFaces"].toArray())if(f.toObject()["role"]=="extrude.endCap")face=f.toObject();QVERIFY(!face.isEmpty());QJsonObject ref;for(auto key:{"bodyId","producerFeatureId","faceId"})ref[key]=face[key];
        call(w,"sketch.begin",{{"support",ref}});call(w,"sketch.addCircle",{{"centerX",8},{"centerY",8},{"diameter",5},{"units","mm"}});call(w,"sketch.tool",{{"tool","select"}});call(w,"view.orientation",{{"name","top"}});call(w,"view.home");
        auto pixel=[&](double u,double v){const auto p=call(w,"view.projectSketchPoint",{{"sketchId",session(w)["draftSketch"].toObject()["id"]},{"uMm",u},{"vMm",v}});return QPoint(qRound(p["x"].toDouble()),qRound(p["y"].toDouble()));};
        const auto circle=session(w)["draftSketch"].toObject()["parameters"].toObject()["entities"].toArray()[0].toObject();const auto centerId=circle["centerPointId"].toString();
        auto current=[&]{for(const auto& p:session(w)["draftSketch"].toObject()["parameters"].toObject()["points"].toArray())if(p.toObject()["id"]==centerId)return QPointF(p.toObject()["uMm"].toDouble(),p.toObject()["vMm"].toDouble());return QPointF{};};
        view->setFocus();QTest::qWait(100);QTest::keyClick(view,Qt::Key_D);QTRY_COMPARE(session(w)["tool"].toString(),QString("dimension"));
        for(int i=0;i<2;++i){const auto c=current();QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(c.x(),c.y()));QCOMPARE(session(w)["selection"].toObject()["pointIds"].toArray(),QJsonArray{centerId});QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,i==0 ? pixel(20,15.8):pixel(0,8));QCOMPARE(session(w)["selection"].toObject()["pointIds"].toArray().size(),1);QCOMPARE(session(w)["selection"].toObject()["entityIds"].toArray().size(),1);
            const auto place=i==0 ? pixel(1,12):pixel(4,2);QVERIFY(view->rect().contains(place));QMouseEvent move(QEvent::MouseMove,QPointF(place),QPointF(view->mapToGlobal(place)),Qt::NoButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(view,&move);QTRY_COMPARE(session(w)["pendingDimension"].toObject()["phase"].toString(),QString("placing"));QCOMPARE(session(w)["pendingDimension"].toObject()["kind"].toString(),QString("pointLineDistance"));QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,place);QCOMPARE(session(w)["pendingDimension"].toObject()["phase"].toString(),QString("editing"));auto* editor=w.findChild<QLineEdit*>("sketch.inlineValue");if(!editor)for(auto* input:w.findChildren<QLineEdit*>())if(input->isVisible() && input->hasFocus())editor=input;QVERIFY(editor);editor->selectAll();QTest::keyClicks(editor,"3.9");QTest::keyClick(editor,Qt::Key_Return);QTRY_VERIFY(!session(w).contains("pendingDimension"));
        }
        const auto c=current();QVERIFY(std::abs(c.x()-3.9)<1e-6 && std::abs(c.y()-11.9)<1e-6);const auto draft=session(w)["draftSketch"].toObject();QCOMPARE(draft["parameters"].toObject()["entities"].toArray()[0].toObject()["radiusMm"].toDouble(),2.5);QCOMPARE(draft["parameters"].toObject()["dimensions"].toArray().size(),2);
        // Selection and Escape must not create a partial equation or projection.
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(c.x(),c.y()));QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(31.8,8));QTest::mouseMove(view,pixel(29,8));view->setFocus();QTest::keyClick(view,Qt::Key_Escape);QCOMPARE(session(w)["draftSketch"].toObject(),draft);
        // Drag one real annotation label; only placement changes, never circle geometry.
        call(w,"sketch.tool",{{"tool","select"}});const auto beforeDrag=session(w)["draftSketch"].toObject();const auto layouts=view->diagnostics()["sketchAnnotations"].toArray();QVERIFY(layouts.size()==2);const auto label=layouts[0].toObject()["label"].toObject();const QPoint labelPixel(qRound(label["x"].toDouble()),qRound(label["y"].toDouble()));const auto dragTo=labelPixel+QPoint(24,12);QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,labelPixel);QMouseEvent drag(QEvent::MouseMove,QPointF(dragTo),QPointF(view->mapToGlobal(dragTo)),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(view,&drag);QTest::mouseRelease(view,Qt::LeftButton,Qt::NoModifier,dragTo);const auto afterDrag=session(w)["draftSketch"].toObject();QCOMPARE(afterDrag["parameters"].toObject()["points"],beforeDrag["parameters"].toObject()["points"]);QCOMPARE(afterDrag["parameters"].toObject()["entities"],beforeDrag["parameters"].toObject()["entities"]);QVERIFY(afterDrag["parameters"].toObject()["dimensions"].toArray()[0].toObject()["annotation"]!=beforeDrag["parameters"].toObject()["dimensions"].toArray()[0].toObject()["annotation"]);
        const auto evidence=qEnvironmentVariable("ALLOY_POINT_EDGE_EVIDENCE");if(!evidence.isEmpty()){QDir().mkpath(evidence);call(w,"capture.screenshot",{{"path",evidence+"/toy-point-edge.png"}});QFile state(evidence+"/toy-point-edge.json");QVERIFY(state.open(QIODevice::WriteOnly));state.write(QJsonDocument(w.state()).toJson(QJsonDocument::Indented));}
        call(w,"sketch.finish");
    }
};
QTEST_MAIN(PointEdgeControlsTest)
#include "point_edge_controls.moc"
