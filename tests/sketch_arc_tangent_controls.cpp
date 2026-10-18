// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <tuple>
#include <utility>
#include <QJsonDocument>
#include <QProcess>
#include <QPushButton>
#include <QScrollArea>
#include <QTest>
#include <QWindow>
#include <cmath>

class SketchArcTangentControls final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& args={}){
        const auto response=window.executeApi(command,args);
        if(!response["ok"].toBool())qFatal("%s",QJsonDocument(response).toJson(QJsonDocument::Compact).constData());
        return response["result"].toObject();
    }
    QJsonObject session(AlloyWindow& window){return window.state()["activeSession"].toObject();}
private slots:
    void arcAndLineInEitherOrder(){
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1500,950);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        ReferenceView* view=nullptr;for(auto* child:window.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;
        QVERIFY(view);
        call(window,"sketch.begin",{{"plane","XY"},{"units","mm"},{"name","Native arc tangent"}});
        auto addLine=[&](double x1,double y1,double x2,double y2){
            call(window,"sketch.line",{{"x1",x1},{"y1",y1},{"x2",x2},{"y2",y2},{"units","mm"}});
            return session(window)["draftSketch"].toObject()["parameters"].toObject()["entities"].toArray().last().toObject();
        };
        const auto right=addLine(60,0,60,35);
        call(window,"sketch.addArc",{{"mode","center"},{"centerX",55},{"centerY",35},{"radius",5},
            {"startAngleDeg",0},{"sweepAngleDeg",90},{"units","mm"}});
        const auto arc=session(window)["draftSketch"].toObject()["parameters"].toObject()["entities"].toArray().last().toObject();
        const auto top=addLine(55,40,0,40),unrelated=addLine(15,15,25,15);
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
        const auto rightPixel=pixel(60,18),arcPixel=pixel(55+5/std::sqrt(2.0),35+5/std::sqrt(2.0)),
            topPixel=pixel(28,40),wrongPixel=pixel(20,15);
        for(const auto& p:{rightPixel,arcPixel,topPixel,wrongPixel})QVERIFY(view->rect().contains(p));
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,rightPixel);
        QCOMPARE(session(window)["selection"].toObject()["entityIds"].toArray(),QJsonArray{right["id"]});
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,arcPixel);
        auto relations=session(window)["draftSketch"].toObject()["parameters"].toObject()["constraints"].toArray();
        QCOMPARE(relations.size(),1);QCOMPARE(relations[0].toObject()["type"].toString(),QString("arcTangent"));
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,arcPixel);
        QCOMPARE(session(window)["selection"].toObject()["entityIds"].toArray(),QJsonArray{arc["id"]});
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,wrongPixel);
        QCOMPARE(session(window)["selection"].toObject()["entityIds"].toArray(),QJsonArray{arc["id"]});
        QCOMPARE(session(window)["draftSketch"].toObject()["parameters"].toObject()["constraints"].toArray().size(),1);
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,topPixel);
        relations=session(window)["draftSketch"].toObject()["parameters"].toObject()["constraints"].toArray();
        QCOMPARE(relations.size(),2);QCOMPARE(relations[1].toObject()["type"].toString(),QString("arcTangent"));
        const auto glyphs=window.state()["viewport"].toObject()["gpu"].toObject()["sketchConstraintGlyphs"].toArray();
        int badges=0;QJsonObject badge;for(const auto& value:glyphs)if(value.toObject()["constraintId"]==relations[1].toObject()["id"]){++badges;badge=value.toObject();}
        QCOMPARE(badges,1);
        const auto evidence=qEnvironmentVariable("ALLOY_SKETCH_ARC_TANGENT_EVIDENCE");
        if(!evidence.isEmpty()){QDir().mkpath(evidence);call(window,"capture.screenshot",{{"path",evidence+"/native-arc-line-tangent.png"}});}
        QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QTest::keyClick(view,Qt::Key_Escape);
        const auto bounds=badge["bounds"].toObject();
        const QPoint target(qRound(bounds["x"].toDouble()+bounds["width"].toDouble()/2),
            qRound(bounds["y"].toDouble()+bounds["height"].toDouble()/2));
        QVERIFY(view->rect().contains(target));QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,target);
        QCOMPARE(session(window)["selection"].toObject()["constraintIds"].toArray(),QJsonArray{relations[1].toObject()["id"]});
        QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QTest::keyClick(view,Qt::Key_Delete);
        QCOMPARE(session(window)["draftSketch"].toObject()["parameters"].toObject()["constraints"].toArray().size(),1);
        Q_UNUSED(unrelated);Q_UNUSED(top);call(window,"sketch.cancel");window.close();
    }
    void interiorPickOrdersAndBadge(){
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1500,950);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        ReferenceView* view=nullptr;for(auto* child:window.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;
        QVERIFY(view);
        call(window,"sketch.begin",{{"plane","XY"},{"units","mm"},{"name","Native circle tangent"}});
        call(window,"sketch.line",{{"x1",-20},{"y1",11},{"x2",20},{"y2",11},{"units","mm"}});
        const auto line=session(window)["draftSketch"].toObject()["parameters"].toObject()["entities"].toArray().last().toObject();
        call(window,"sketch.addArc",{{"mode","center"},{"centerX",0},{"centerY",0},{"radius",10},
            {"startAngleDeg",0},{"sweepAngleDeg",180},{"units","mm"}});
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
            const auto center=points[circle["centerPointId"].toString()],start=points[line["startPointId"].toString()],end=points[line["endPointId"].toString()];
            QJsonObject arc;for(const auto& v:params["entities"].toArray())if(v.toObject()["id"]==circle["id"])arc=v.toObject();
            const double angle=(arc["startAngleDeg"].toDouble()+arc["sweepAngleDeg"].toDouble()*0.25)*std::acos(-1.0)/180;
            return std::pair{pixel(center["uMm"].toDouble()+arc["radiusMm"].toDouble()*std::cos(angle),center["vMm"].toDouble()+arc["radiusMm"].toDouble()*std::sin(angle)),
                pixel(start["uMm"].toDouble()*0.75+end["uMm"].toDouble()*0.25,start["vMm"].toDouble()*0.75+end["vMm"].toDouble()*0.25)};};
        auto [circlePixel,linePixel]=targets();QVERIFY(view->rect().contains(circlePixel));QVERIFY(view->rect().contains(linePixel));
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,circlePixel);
        QCOMPARE(session(window)["selection"].toObject()["entityIds"].toArray(),QJsonArray{circle["id"]});
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,linePixel);
        auto relations=session(window)["draftSketch"].toObject()["parameters"].toObject()["constraints"].toArray();
        QCOMPARE(relations.size(),1);QCOMPARE(relations[0].toObject()["type"].toString(),QString("arcTangent"));
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
        QCOMPARE(relations.size(),1);QCOMPARE(relations[0].toObject()["type"].toString(),QString("arcTangent"));
        glyphs=window.state()["viewport"].toObject()["gpu"].toObject()["sketchConstraintGlyphs"].toArray();
        pickedBadge=badge(glyphs,relations[0].toObject()["id"]);QVERIFY(!pickedBadge.isEmpty());
        const auto evidence=qEnvironmentVariable("ALLOY_SKETCH_INTERIOR_ARC_EVIDENCE");
        if(!evidence.isEmpty()){QDir().mkpath(evidence);call(window,"capture.screenshot",{{"path",evidence+"/native-interior-arc-tangent.png"}});
            QFile state(evidence+"/native-state.json");QVERIFY(state.open(QIODevice::WriteOnly));state.write(QJsonDocument(window.state()).toJson());}
        call(window,"sketch.cancel");window.close();
    }
};
QTEST_MAIN(SketchArcTangentControls)
#include "sketch_arc_tangent_controls.moc"
