// SPDX-License-Identifier: MIT
#include "AlloyTheme.h"
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QDir>
#include <QJsonDocument>
#include <QTest>
#include <QToolButton>
#include <cmath>

class NavigationSelectionLookAtControls final : public QObject {
    Q_OBJECT
    static QJsonObject call(AlloyWindow& window,const QString& name,const QJsonObject& args={}) {
        const auto response=window.executeApi(name,args);
        if(!response.value("ok").toBool())qFatal("%s",QJsonDocument(response).toJson(QJsonDocument::Compact).constData());
        return response.value("result").toObject();
    }
    static QJsonObject camera(AlloyWindow& window) {
        return window.state().value("viewport").toObject().value("navigation").toObject();
    }
private slots:
    void selectedShapeAndPlanarLookAt() {
        const auto out=qEnvironmentVariable("ALLOY_NAV_LOOK_AT_EVIDENCE");
        QVERIFY(!out.isEmpty());QVERIFY(QDir().mkpath(out));
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1360,850);window.show();window.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&window));QApplication::setActiveWindow(&window);
        QTRY_VERIFY(window.isActiveWindow());
        auto* fit=window.findChild<QToolButton*>("command.view.fitSelection");
        auto* look=window.findChild<QToolButton*>("command.view.lookAt");
        QVERIFY(fit && look && fit->isVisible() && look->isVisible());
        call(window,"solid.box",{{"width",46},{"depth",27},{"height",13},{"units","mm"}});
        const auto body=call(window,"document.inspect").value("evaluation").toObject().value("bodies").toArray().first().toObject();
        const auto bodyId=body.value("bodyId").toString();QVERIFY(!bodyId.isEmpty());
        QJsonObject top;
        for(const auto& value:body.value("supportFaces").toArray()){
            const auto candidate=value.toObject();
            const auto normal=candidate.value("frame").toObject().value("normal").toArray();
            if(normal.size()==3 && normal[2].toDouble()>0.99)top=candidate;
        }
        QVERIFY(!top.isEmpty());
        const QJsonObject support{{"bodyId",top.value("bodyId")},{"producerFeatureId",top.value("producerFeatureId")},{"faceId",top.value("faceId")}};
        call(window,"view.home");call(window,"view.orbit",{{"deltaX",45},{"deltaY",18}});
        call(window,"view.pan",{{"x",38},{"y",-20}});call(window,"view.zoom",{{"factor",1.5}});
        call(window,"selection.set",{{"bodyId",bodyId}});
        const auto beforeBody=call(window,"document.inspect"),pose=camera(window);
        QTest::mouseClick(fit,Qt::LeftButton);
        const auto fitted=camera(window);
        QCOMPARE(fitted.value("customDirection"),pose.value("customDirection"));
        QCOMPARE(fitted.value("customUp"),pose.value("customUp"));
        QCOMPARE(fitted.value("zoom"),QJsonValue(1));
        QCOMPARE(fitted.value("panX"),QJsonValue(0));
        QVERIFY(std::abs(fitted.value("centerZ").toDouble()-6.5)<1e-7);
        QCOMPARE(window.state().value("selection").toObject().value("bodyId"),QJsonValue(bodyId));
        QCOMPARE(call(window,"document.inspect").value("authored"),beforeBody.value("authored"));
        QCOMPARE(call(window,"document.inspect").value("undo"),beforeBody.value("undo"));
        call(window,"capture.screenshot",{{"path",out+"/fit-selected-body.png"}});
        const auto refusal=window.executeApi("view.lookAt");
        QVERIFY(!refusal.value("ok").toBool());
        QCOMPARE(refusal.value("error").toObject().value("code"),QJsonValue("unsupported_selection"));
        QCOMPARE(camera(window).value("customDirection"),fitted.value("customDirection"));

        call(window,"selection.faceSet",{{"support",support}});
        const auto beforeFace=call(window,"document.inspect");
        QTest::mouseClick(look,Qt::LeftButton);
        const auto faceCamera=camera(window);
        QCOMPARE(faceCamera.value("orientation"),QJsonValue("custom"));
        const auto normal=top.value("frame").toObject().value("normal").toArray();
        const auto direction=faceCamera.value("customDirection").toArray();
        for(int i=0;i<3;++i)QVERIFY(std::abs(direction[i].toDouble()+normal[i].toDouble())<1e-7);
        QVERIFY(std::abs(faceCamera.value("centerZ").toDouble()-13)<1e-7);
        QCOMPARE(window.state().value("selection").toObject().value("support"),QJsonValue(support));
        QCOMPARE(call(window,"document.inspect").value("authored"),beforeFace.value("authored"));
        call(window,"capture.screenshot",{{"path",out+"/look-at-top-face.png"}});

        call(window,"sketch.begin",{{"support",support},{"name","Offset cap sketch"},{"units","mm"}});
        call(window,"sketch.rectangle",{{"x1",5},{"y1",5},{"x2",15},{"y2",11},{"units","mm"}});
        const auto activeBefore=window.state().value("activeSession").toObject();
        QTest::mouseClick(look,Qt::LeftButton);
        QCOMPARE(window.state().value("activeSession").toObject().value("draftSketch"),activeBefore.value("draftSketch"));
        QCOMPARE(window.state().value("activeSession").toObject().value("command"),QJsonValue("sketch.edit"));
        call(window,"sketch.finish");
        const auto sketches=call(window,"document.inspect").value("evaluation").toObject().value("sketches").toArray();
        QVERIFY(!sketches.isEmpty());const auto sketchRow=sketches.last().toObject();
        const auto sketchId=sketchRow.value("sketchId").toString();
        QVERIFY(!sketchId.isEmpty());
        call(window,"selection.sketchSet",{{"sketchId",sketchId}});
        const auto beforeSketch=call(window,"document.inspect");
        QCOMPARE(window.state().value("selection").toObject().value("sketchId"),QJsonValue(sketchId));
        const auto sketchFit=call(window,"view.fitSelection");
        QVERIFY(sketchFit.value("scale").toDouble()<20);
        call(window,"view.home");
        const auto cameraBeforeSketch=camera(window);
        QTest::mouseClick(look,Qt::LeftButton);
        const auto sketchCamera=camera(window);
        qInfo().noquote()<<"selected sketch camera before"<<QJsonDocument(cameraBeforeSketch).toJson(QJsonDocument::Compact)
            <<"after"<<QJsonDocument(sketchCamera).toJson(QJsonDocument::Compact);
        QCOMPARE(sketchCamera.value("orientation"),QJsonValue("custom"));
        QCOMPARE(call(window,"document.inspect").value("authored"),beforeSketch.value("authored"));
        QCOMPARE(call(window,"document.inspect").value("undo"),beforeSketch.value("undo"));
        call(window,"capture.screenshot",{{"path",out+"/look-at-finished-sketch.png"}});
        call(window,"document.save",{{"path",out+"/selected-look-at.alloy"}});window.close();
        const auto frame=cameraBeforeSketch.value("sketchFrame").toObject();
        const auto origin=frame.value("origin").toArray(),u=frame.value("uAxis").toArray(),v=frame.value("vAxis").toArray();
        QCOMPARE(origin.size(),3);QCOMPARE(u.size(),3);QCOMPARE(v.size(),3);
        const double expectedX=origin[0].toDouble()+10*u[0].toDouble()+8*v[0].toDouble();
        const double expectedY=origin[1].toDouble()+10*u[1].toDouble()+8*v[1].toDouble();
        const double expectedZ=origin[2].toDouble()+10*u[2].toDouble()+8*v[2].toDouble();
        QVERIFY(std::abs(sketchFit.value("centerX").toDouble()-expectedX)<1e-5);
        QVERIFY(std::abs(sketchFit.value("centerY").toDouble()-expectedY)<1e-5);
        QVERIFY(std::abs(sketchCamera.value("centerX").toDouble()-expectedX)<1e-5);
        QVERIFY(std::abs(sketchCamera.value("centerY").toDouble()-expectedY)<1e-5);
        QVERIFY(std::abs(sketchCamera.value("centerZ").toDouble()-expectedZ)<1e-5);
        QVERIFY2(sketchCamera.value("scale").toDouble()<20,"Look At must frame the visible 10 x 6 mm sketch, not the hidden support projections");
    }
};
QTEST_MAIN(NavigationSelectionLookAtControls)
#include "navigation_selection_look_at_controls.moc"
