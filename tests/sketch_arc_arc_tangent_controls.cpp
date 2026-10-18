// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QProcess>
#include <QPushButton>
#include <QScrollArea>
#include <QTest>
#include <QWindow>
#include <tuple>
#include <cmath>
#include <utility>

class SketchArcArcTangentControls final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& args={}){
        const auto response=window.executeApi(command,args);
        if(!response["ok"].toBool())qFatal("%s",QJsonDocument(response).toJson(QJsonDocument::Compact).constData());
        return response["result"].toObject();
    }
    QJsonObject session(AlloyWindow& window){return window.state()["activeSession"].toObject();}
private slots:
    void eitherPickOrderAndBadge(){
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1500,950);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        ReferenceView* view=nullptr;for(auto* child:window.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;
        QVERIFY(view);
        call(window,"sketch.begin",{{"plane","XY"},{"units","mm"},{"name","Native arc-arc tangent"}});
        call(window,"sketch.addArc",{{"mode","center"},{"centerX",-10},{"centerY",10},{"radius",10},{"startAngleDeg",-90},{"sweepAngleDeg",180},{"units","mm"}});
        const auto line=session(window)["draftSketch"].toObject()["parameters"].toObject()["entities"].toArray().last().toObject();
        call(window,"sketch.addArc",{{"mode","center"},{"centerX",4},{"centerY",10},{"radius",4},{"startAngleDeg",90},{"sweepAngleDeg",180},{"units","mm"}});
        const auto circle=session(window)["draftSketch"].toObject()["parameters"].toObject()["entities"].toArray().last().toObject();
        call(window,"sketch.lookAt");
        auto* button=window.findChild<QPushButton*>("sketch.constraint.tangent");QVERIFY(button);
        for(auto* area:window.findChildren<QScrollArea*>())if(area->isAncestorOf(button))area->ensureWidgetVisible(button);
#ifdef Q_OS_MACOS
        QProcess activation;activation.start("osascript",{"-e",QString("tell application \"System Events\" to set frontmost of first application process whose unix id is %1 to true").arg(QCoreApplication::applicationPid())});
        QVERIFY(activation.waitForFinished(5000));QCOMPARE(activation.exitCode(),0);
#endif
        window.raise();window.activateWindow();window.windowHandle()->requestActivate();QApplication::setActiveWindow(&window);
        QTRY_VERIFY(window.isActiveWindow());QVERIFY(button->isVisible());
        QTest::mouseClick(button,Qt::LeftButton);QCOMPARE(session(window)["constraintType"].toString(),QString("tangent"));
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
        auto pixel=[&](double u,double v){const auto p=call(window,"view.projectSketchPoint",{{"sketchId",session(window)["draftSketch"].toObject()["id"]},{"uMm",u},{"vMm",v}});
            return QPoint(qRound(p["x"].toDouble()),qRound(p["y"].toDouble()));};
        auto targets=[&]{const auto params=session(window)["draftSketch"].toObject()["parameters"].toObject();
            QMap<QString,QJsonObject> points;for(const auto& value:params["points"].toArray())points[value.toObject()["id"].toString()]=value.toObject();
            const auto center=points[circle["centerPointId"].toString()],other=points[line["centerPointId"].toString()];
            auto radius=[&](const QJsonObject& e){for(const auto& v:params["entities"].toArray())if(v.toObject()["id"]==e["id"])return v.toObject()["radiusMm"].toDouble();return 0.0;};
            return std::pair{pixel(center["uMm"].toDouble()-radius(circle)*std::sqrt(.5),center["vMm"].toDouble()+radius(circle)*std::sqrt(.5)),
                pixel(other["uMm"].toDouble()+radius(line)*std::sqrt(.5),other["vMm"].toDouble()+radius(line)*std::sqrt(.5))};};
        auto [circlePixel,linePixel]=targets();QVERIFY(view->rect().contains(circlePixel));QVERIFY(view->rect().contains(linePixel));
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,circlePixel);
        QCOMPARE(session(window)["selection"].toObject()["entityIds"].toArray(),QJsonArray{circle["id"]});
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,linePixel);
        auto relations=session(window)["draftSketch"].toObject()["parameters"].toObject()["constraints"].toArray();
        QCOMPARE(relations.size(),1);QCOMPARE(relations[0].toObject()["type"].toString(),QString("arcArcTangent"));
        QVERIFY(relations[0].toObject()["entityIds"].toArray().contains(circle["id"]));
        auto glyphs=window.state()["viewport"].toObject()["gpu"].toObject()["sketchConstraintGlyphs"].toArray();
        auto badge=[&](const QJsonArray& all,const QJsonValue& id){QJsonObject result;for(const auto& value:all)if(value.toObject()["constraintId"]==id)result=value.toObject();return result;};
        auto pickedBadge=badge(glyphs,relations[0].toObject()["id"]);QVERIFY(!pickedBadge.isEmpty());
        auto clickBadge=[&](const QJsonObject& value){const auto bounds=value["bounds"].toObject();
            const QPoint target(qRound(bounds["x"].toDouble()+bounds["width"].toDouble()/2),
                qRound(bounds["y"].toDouble()+bounds["height"].toDouble()/2));
            QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,target);};
        QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QTest::keyClick(view,Qt::Key_Escape);
        clickBadge(pickedBadge);
        QCOMPARE(session(window)["selection"].toObject()["constraintIds"].toArray(),QJsonArray{relations[0].toObject()["id"]});
        QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QTest::keyClick(view,Qt::Key_Delete);
        QCOMPARE(session(window)["draftSketch"].toObject()["parameters"].toObject()["constraints"].toArray().size(),0);
        QTest::mouseClick(button,Qt::LeftButton);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
        std::tie(circlePixel,linePixel)=targets();
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,linePixel);
        QCOMPARE(session(window)["selection"].toObject()["entityIds"].toArray(),QJsonArray{line["id"]});
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,circlePixel);
        relations=session(window)["draftSketch"].toObject()["parameters"].toObject()["constraints"].toArray();
        QCOMPARE(relations.size(),1);QCOMPARE(relations[0].toObject()["type"].toString(),QString("arcArcTangent"));
        glyphs=window.state()["viewport"].toObject()["gpu"].toObject()["sketchConstraintGlyphs"].toArray();
        pickedBadge=badge(glyphs,relations[0].toObject()["id"]);QVERIFY(!pickedBadge.isEmpty());
        const auto evidence=qEnvironmentVariable("ALLOY_SKETCH_ARC_ARC_EVIDENCE");
        if(!evidence.isEmpty()){
            QDir().mkpath(evidence);call(window,"capture.screenshot",{{"path",evidence+"/native-arc-arc-tangent.png"}});
            QFile file(evidence+"/native-receipt.json");QVERIFY(file.open(QIODevice::WriteOnly));
            file.write(QJsonDocument(QJsonObject{{"bothPickOrders",true},{"badgeSelectedAndDeleted",true},
                {"relation",relations[0]},{"gpu",window.state()["viewport"].toObject()["gpu"]}}).toJson());
        }
        call(window,"sketch.cancel");window.close();
    }
};
QTEST_MAIN(SketchArcArcTangentControls)
#include "sketch_arc_arc_tangent_controls.moc"
