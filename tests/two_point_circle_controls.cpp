// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QTemporaryDir>
#include <QTest>
#include <cmath>
#include <numbers>

// Injected Qt input runs through Alloy's real window and GPU view.
class TwoPointCircleControlsTest final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& w,const QString& command,const QJsonObject& args={}) {
        const auto response=w.execute(command,args);
        if(!response["ok"].toBool())qFatal("%s",QJsonDocument(response).toJson().constData());
        return response["result"].toObject();
    }
    QJsonObject active(AlloyWindow& w) {return w.state()["activeSession"].toObject();}
    QJsonObject draft(AlloyWindow& w) {return active(w)["draftSketch"].toObject();}
    static QJsonObject point(const QJsonObject& sketch,const QJsonValue& id) {
        for(const auto& value:sketch["parameters"].toObject()["points"].toArray())
            if(value.toObject()["id"]==id)return value.toObject();
        return {};
    }
private slots:
    void settledPalettePreview() {
        const auto output=qEnvironmentVariable("ALLOY_TWO_POINT_CIRCLE_EVIDENCE");
        QVERIFY(!output.isEmpty());
        AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1300,900);w.show();w.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&w));
        QApplication::setActiveWindow(&w);
        ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())
            if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;
        QVERIFY(view);view->setFocus();
        call(w,"sketch.begin",{{"plane","XY"}});
        call(w,"sketch.rectangle",{{"x1",0},{"y1",0},{"x2",30},{"y2",20},{"units","mm"}});
        call(w,"sketch.finish");
        auto evaluation=call(w,"document.inspect")["evaluation"].toObject();
        const auto floorSketch=evaluation["sketches"].toArray().first().toObject();
        call(w,"solid.extrude",{{"sketchId",floorSketch["sketchId"]},
            {"profileId",floorSketch["profiles"].toArray().first().toObject()["profileId"]},
            {"distance",2},{"units","mm"}});
        evaluation=call(w,"document.inspect")["evaluation"].toObject();
        const auto body=evaluation["bodies"].toArray().first().toObject();
        QJsonObject face;for(const auto& value:body["supportFaces"].toArray())
            if(value.toObject()["role"]=="extrude.endCap")face=value.toObject();
        QVERIFY(!face.isEmpty());QJsonObject support;
        for(const auto key:{"bodyId","producerFeatureId","faceId"})support[key]=face[key];
        call(w,"selection.faceSet",{{"support",support}});
        call(w,"sketch.twoPointCircle");
        const auto sketchId=draft(w).value("id");
        auto pixel=[&](double u,double v){const auto p=call(w,"view.projectSketchPoint",{{"sketchId",sketchId},{"uMm",u},{"vMm",v}});
            return QPoint(qRound(p["x"].toDouble()),qRound(p["y"].toDouble()));};
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(16,8));
        const QPoint direction=pixel(20,8);
        QMouseEvent move(QEvent::MouseMove,QPointF(direction),QPointF(view->mapToGlobal(direction)),Qt::NoButton,Qt::NoButton,Qt::NoModifier);
        QApplication::sendEvent(view,&move);
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
        QTest::keyClick(view,Qt::Key_4);
        auto* input=w.findChild<QLineEdit*>("sketch.inlineValue");QVERIFY(input && input->isVisible());
        auto* info=w.findChild<QLabel*>("sketch.info");auto* profile=w.findChild<QLabel*>("sketch.profileStatus");
        QVERIFY(info && profile);
        auto measure=[](QLabel* label){const auto g=label->geometry();return QJsonObject{{"text",label->text()},
            {"x",g.x()},{"y",g.y()},{"width",g.width()},{"height",g.height()},
            {"sizeHintHeight",label->sizeHint().height()},{"heightForWidth",label->heightForWidth(g.width())},
            {"minimumHeight",label->minimumHeight()}};};
        QJsonObject metrics{{"beforeInfo",measure(info)},{"beforeProfile",measure(profile)}};
        QApplication::processEvents();QTest::qWait(80);QApplication::processEvents();
        metrics["afterInfo"]=measure(info);metrics["afterProfile"]=measure(profile);
        QVERIFY(profile->geometry().top()>info->geometry().bottom());
        QFile file(output+"/two-point-circle-palette-layout.json");QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(QJsonDocument(metrics).toJson(QJsonDocument::Indented));file.close();
        call(w,"capture.screenshot",{{"path",output+"/two-point-circle-preview-settled.png"}});
        w.hide();
    }
    void diameterEndpointOnFaceAndCut() {
        QTemporaryDir scratch;QVERIFY(scratch.isValid());
        auto output=qEnvironmentVariable("ALLOY_TWO_POINT_CIRCLE_EVIDENCE");
        if(output.isEmpty())output=scratch.path();QDir().mkpath(output);
        AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1300,900);w.show();w.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&w));
        QApplication::setActiveWindow(&w);
        ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())
            if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;
        QVERIFY(view);view->setFocus();
        call(w,"sketch.begin",{{"plane","XY"}});
        call(w,"sketch.rectangle",{{"x1",0},{"y1",0},{"x2",30},{"y2",20},{"units","mm"}});
        call(w,"sketch.finish");
        auto evaluation=call(w,"document.inspect")["evaluation"].toObject();
        const auto floorSketch=evaluation["sketches"].toArray().first().toObject();
        call(w,"solid.extrude",{{"sketchId",floorSketch["sketchId"]},
            {"profileId",floorSketch["profiles"].toArray().first().toObject()["profileId"]},
            {"distance",2},{"units","mm"}});
        evaluation=call(w,"document.inspect")["evaluation"].toObject();
        const auto body=evaluation["bodies"].toArray().first().toObject();
        QJsonObject face;for(const auto& value:body["supportFaces"].toArray())
            if(value.toObject()["role"]=="extrude.endCap")face=value.toObject();
        QVERIFY(!face.isEmpty());QJsonObject support;
        for(const auto key:{"bodyId","producerFeatureId","faceId"})support[key]=face[key];
        call(w,"selection.faceSet",{{"support",support}});
        call(w,"sketch.twoPointCircle");
        QCOMPARE(active(w)["tool"].toString(),QString("twoPointCircle"));
        QCOMPARE(active(w)["inputs"].toObject()["support"].toObject(),support);
        QCOMPARE(active(w)["resolvedFrame"].toObject(),face["frame"].toObject());
        const auto sketchId=draft(w).value("id");
        auto pixel=[&](double u,double v){const auto p=call(w,"view.projectSketchPoint",{{"sketchId",sketchId},{"uMm",u},{"vMm",v}});
            return QPoint(qRound(p["x"].toDouble()),qRound(p["y"].toDouble()));};
        auto pointerMove=[&](double u,double v){const QPoint p=pixel(u,v);
            QMouseEvent event(QEvent::MouseMove,QPointF(p),QPointF(view->mapToGlobal(p)),Qt::NoButton,Qt::NoButton,Qt::NoModifier);
            QApplication::sendEvent(view,&event);};
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(8,8));
        QVERIFY(active(w).contains("firstPoint"));
        const auto before=draft(w);
        pointerMove(12,8);
        QVERIFY(active(w).contains("pointerPoint"));
        QVERIFY(view->diagnostics()["cursorPreviewVisible"].toBool());
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
        QTest::keyClick(view,Qt::Key_4);
        auto* input=w.findChild<QLineEdit*>("sketch.inlineValue");QVERIFY(input && input->isVisible());
        QCOMPARE(w.state()["inlineEditor"].toObject()["kind"].toString(),QString("twoPointCircleDiameter"));
        QCOMPARE(draft(w),before);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(input));
        QApplication::processEvents();QTest::qWait(80);QApplication::processEvents();
        call(w,"capture.screenshot",{{"path",output+"/two-point-circle-preview.png"}});
        QTest::keyClick(input,Qt::Key_Return);
        QVERIFY(!active(w).contains("pendingCircleDiameter"));
        const auto placed=draft(w),parameters=placed["parameters"].toObject();
        QCOMPARE(parameters["entities"].toArray().size(),1);
        const auto circle=parameters["entities"].toArray().first().toObject();
        QCOMPARE(circle["type"].toString(),QString("circle"));
        QVERIFY(!circle["diameterStartPointId"].toString().isEmpty());
        QVERIFY(std::abs(circle["radiusMm"].toDouble()-2)<1e-6);
        const auto start=point(placed,circle["diameterStartPointId"]),center=point(placed,circle["centerPointId"]);
        QVERIFY(!start.isEmpty() && !center.isEmpty());
        QVERIFY(std::abs(start["uMm"].toDouble()-8)<0.2);
        QVERIFY(std::abs(start["vMm"].toDouble()-8)<0.2);
        QVERIFY(std::abs(std::hypot(center["uMm"].toDouble()-start["uMm"].toDouble(),
            center["vMm"].toDouble()-start["vMm"].toDouble())-2)<1e-6);
        const double secondU=start["uMm"].toDouble()+4*circle["diameterDirectionU"].toDouble();
        const double secondV=start["vMm"].toDouble()+4*circle["diameterDirectionV"].toDouble();
        QVERIFY(std::abs(std::hypot(secondU-start["uMm"].toDouble(),secondV-start["vMm"].toDouble())-4)<1e-6);
        QCOMPARE(parameters["dimensions"].toArray().first().toObject()["valueMm"].toDouble(),4.0);
        const auto dimensionId=parameters["dimensions"].toArray().first().toObject().value("id");
        call(w,"sketch.dimensionUpdate",{{"dimensionId",dimensionId},{"value",6},{"units","mm"}});
        QCOMPARE(draft(w)["parameters"].toObject()["entities"].toArray().first().toObject()["radiusMm"].toDouble(),3.0);
        QCOMPARE(point(draft(w),circle["diameterStartPointId"]),start);
        call(w,"sketch.tool",{{"tool","select"}});
        call(w,"history.undo");QCOMPARE(draft(w)["parameters"].toObject(),parameters);
        call(w,"history.redo");
        call(w,"history.undo");QCOMPARE(draft(w)["parameters"].toObject(),parameters);
        call(w,"sketch.movePoint",{{"pointId",circle["diameterStartPointId"]},{"x",9},{"y",9},{"units","mm"}});
        QVERIFY(std::abs(point(draft(w),circle["centerPointId"])["uMm"].toDouble()-center["uMm"].toDouble()-(9-start["uMm"].toDouble()))<1e-6);
        QVERIFY(std::abs(point(draft(w),circle["centerPointId"])["vMm"].toDouble()-center["vMm"].toDouble()-(9-start["vMm"].toDouble()))<1e-6);
        call(w,"history.undo");QCOMPARE(draft(w)["parameters"].toObject(),parameters);

        call(w,"sketch.twoPointCircle");
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(16,8));
        pointerMove(20,8);
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
        QTest::keyClick(view,Qt::Key_0);QVERIFY(input->isVisible());
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(input));
        const auto invalid=w.execute("sketch.circleDiameterAccept");
        QVERIFY(!invalid["ok"].toBool());
        QCOMPARE(invalid["error"].toObject()["code"].toString(),QString("invalid_circle_diameter"));
        QCOMPARE(draft(w)["parameters"].toObject(),parameters);
        QTest::keyClick(input,Qt::Key_Escape);
        QCOMPARE(draft(w)["parameters"].toObject(),parameters);
        call(w,"sketch.tool",{{"tool","select"}});call(w,"sketch.finish");
        const auto authored=call(w,"document.inspect").value("authored");
        const auto path=output+"/two-point-circle.alloy";call(w,"document.save",{{"path",path}});
        call(w,"document.open",{{"path",path}});
        QCOMPARE(call(w,"document.inspect").value("authored"),authored);
        evaluation=call(w,"document.inspect")["evaluation"].toObject();
        const auto holeSketch=evaluation["sketches"].toArray().last().toObject();
        call(w,"solid.extrude",{{"sketchId",holeSketch["sketchId"]},
            {"profileId",holeSketch["profiles"].toArray().first().toObject()["profileId"]},
            {"distance",-3},{"units","mm"},{"operation","cut"},{"targetBodyIds",QJsonArray{body["bodyId"]}}});
        const auto cutBody=call(w,"document.inspect")["evaluation"].toObject()["bodies"].toArray().first().toObject();
        QVERIFY(std::abs(cutBody["volumeMm3"].toDouble()-(1200-8*std::numbers::pi))<1e-3);
        call(w,"history.undo");call(w,"history.redo");
        call(w,"document.save",{{"path",output+"/two-point-circle-cut.alloy"}});
        QFile evidence(output+"/two-point-circle-state.json");
        QVERIFY(evidence.open(QIODevice::WriteOnly));evidence.write(QJsonDocument(w.state()).toJson(QJsonDocument::Indented));
        w.close();
    }
};
QTEST_MAIN(TwoPointCircleControlsTest)
#include "two_point_circle_controls.moc"
