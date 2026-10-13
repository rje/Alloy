// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QMouseEvent>
#include <QTest>

class CoincidentSnapControlsTest final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& w,const QString& command,const QJsonObject& args={}) {
        const auto response=w.execute(command,args);
        if(!response["ok"].toBool())qFatal("%s",QJsonDocument(response).toJson().constData());
        return response["result"].toObject();
    }
    QJsonObject session(AlloyWindow& w) {return w.state()["activeSession"].toObject();}
    QJsonObject parameters(AlloyWindow& w) {return session(w)["draftSketch"].toObject()["parameters"].toObject();}
    QPoint pixel(AlloyWindow& w,double u,double v) {
        const auto p=call(w,"view.projectSketchPoint",{{"sketchId",session(w)["draftSketch"].toObject()["id"]},{"uMm",u},{"vMm",v}});
        return {qRound(p["x"].toDouble()),qRound(p["y"].toDouble())};
    }
    QJsonObject xy(const QPoint& p) {return {{"x",p.x()},{"y",p.y()}};}
    ReferenceView* view(AlloyWindow& w) {
        for(auto* child:w.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(child))return v;
        return nullptr;
    }
private slots:
    void linkedOriginRectangleAndAxisThenNativeCircle() {
        AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1300,900);w.show();w.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&w));QApplication::setActiveWindow(&w);
        auto* canvas=view(w);QVERIFY(canvas);canvas->setFocus();
        call(w,"sketch.begin",{{"plane","XY"}});call(w,"view.zoom",{{"factor",0.55}});
        call(w,"sketch.tool",{{"tool","centerRectangle"}});
        QTest::mouseClick(canvas,Qt::LeftButton,Qt::NoModifier,pixel(w,0,0));
        QTest::mouseClick(canvas,Qt::LeftButton,Qt::NoModifier,pixel(w,33,54));
        const auto rectangle=parameters(w);const auto origin=rectangle["originPointId"].toString();
        QVERIFY(!origin.isEmpty());QCOMPARE(rectangle["entities"].toArray().size(),6);
        call(w,"sketch.tool",{{"tool","line"}});
        QTest::mouseClick(canvas,Qt::LeftButton,Qt::NoModifier,pixel(w,-33,0));
        QTest::mouseClick(canvas,Qt::LeftButton,Qt::NoModifier,pixel(w,33,0));
        call(w,"sketch.tool",{{"tool","circle"}});
        const auto before=parameters(w);const auto center=pixel(w,0,0);
        const auto preview=call(w,"sketch.pointerMove",xy(center))["snap"].toObject();
        QCOMPARE(preview["status"].toString(),QString("acquired"));
        QCOMPARE(preview["target"].toObject()["kind"].toString(),QString("origin"));
        QTest::mouseClick(canvas,Qt::LeftButton,Qt::NoModifier,center);
        QCOMPARE(session(w)["firstPoint"].toObject()["target"].toObject()["kind"].toString(),QString("origin"));
        QCOMPARE(parameters(w),before);
        QTest::mouseClick(canvas,Qt::LeftButton,Qt::NoModifier,pixel(w,5,0));
        QCOMPARE(parameters(w)["entities"].toArray().size(),8);
        QCOMPARE(parameters(w)["originPointId"].toString(),origin);
        for(const auto& point:before["points"].toArray())QVERIFY(parameters(w)["points"].toArray().contains(point));
        const auto output=qEnvironmentVariable("ALLOY_COINCIDENT_SNAP_EVIDENCE");
        if(!output.isEmpty()) {
            QDir().mkpath(output);call(w,"capture.screenshot",{{"path",output+"/origin-circle.png"}});
            QFile state(output+"/origin-circle-state.json");QVERIFY(state.open(QIODevice::WriteOnly));
            state.write(QJsonDocument(w.state()).toJson(QJsonDocument::Indented));state.close();
        }
        w.hide();
    }
    void unlinkedEqualCoordinatePointsStayAmbiguous() {
        AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1300,900);w.show();w.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&w));QApplication::setActiveWindow(&w);
        QVERIFY(view(w));call(w,"sketch.begin",{{"plane","XY"}});call(w,"view.zoom",{{"factor",0.55}});
        call(w,"sketch.centerRectangle",{{"centerX",0},{"centerY",0},{"width",66},{"height",108},{"units","mm"}});
        call(w,"sketch.tool",{{"tool","circle"}});
        const auto before=parameters(w);const auto center=pixel(w,0,0);
        const auto preview=call(w,"sketch.pointerMove",xy(center))["snap"].toObject();
        QCOMPARE(preview["status"].toString(),QString("ambiguous"));
        const auto attempted=w.execute("sketch.placePoint",xy(center));
        QVERIFY(!attempted["ok"].toBool());
        QCOMPARE(attempted["error"].toObject()["code"].toString(),QString("ambiguous_snap"));
        QCOMPARE(parameters(w),before);QVERIFY(!session(w).contains("firstPoint"));
        w.hide();
    }
    void emptySupportedFaceOriginKeepsPriority() {
        AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1300,900);w.show();w.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&w));QApplication::setActiveWindow(&w);
        QVERIFY(view(w));call(w,"sketch.begin",{{"plane","XY"}});
        call(w,"sketch.rectangle",{{"x1",0},{"y1",0},{"x2",20},{"y2",20},{"units","mm"}});
        call(w,"sketch.finish");
        auto evaluation=call(w,"document.inspect")["evaluation"].toObject();
        const auto sketch=evaluation["sketches"].toArray().first().toObject();
        call(w,"solid.extrude",{{"sketchId",sketch["sketchId"]},
            {"profileId",sketch["profiles"].toArray().first().toObject()["profileId"]},
            {"distance",2},{"units","mm"}});
        evaluation=call(w,"document.inspect")["evaluation"].toObject();
        const auto body=evaluation["bodies"].toArray().first().toObject();
        QJsonObject face;for(const auto& item:body["supportFaces"].toArray())
            if(item.toObject()["role"]=="extrude.endCap")face=item.toObject();
        QVERIFY(!face.isEmpty());
        QJsonObject support;for(const auto* key:{"bodyId","producerFeatureId","faceId"})support[key]=face[key];
        call(w,"sketch.begin",{{"support",support}});
        QCOMPARE(parameters(w)["entities"].toArray().size(),0);
        QCOMPARE(parameters(w)["originPointId"].toString(),QString{});
        bool projectedAtOrigin=false;
        for(const auto& item:session(w)["projectionContext"].toObject()["lines"].toArray()) {
            const auto line=item.toObject();
            for(const auto* end:{"1","2"}) {
                const QString u=QString("u%1Mm").arg(end),v=QString("v%1Mm").arg(end);
                if(qAbs(line[u].toDouble())<1e-6 && qAbs(line[v].toDouble())<1e-6)projectedAtOrigin=true;
            }
        }
        QVERIFY(projectedAtOrigin);
        call(w,"sketch.tool",{{"tool","circle"}});
        const auto center=pixel(w,0,0);
        const auto preview=call(w,"sketch.pointerMove",xy(center))["snap"].toObject();
        QCOMPARE(preview["status"].toString(),QString("acquired"));
        QCOMPARE(preview["target"].toObject()["kind"].toString(),QString("origin"));
        call(w,"sketch.placePoint",xy(center));
        QCOMPARE(session(w)["firstPoint"].toObject()["target"].toObject()["kind"].toString(),QString("origin"));
        w.hide();
    }
};
QTEST_MAIN(CoincidentSnapControlsTest)
#include "coincident_snap_controls.moc"
