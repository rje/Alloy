// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QJsonDocument>
#include <QProcess>
#include <QPushButton>
#include <QScrollArea>
#include <QTest>
#include <QWindow>

class SketchSymmetryControls final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& args={}){
        const auto response=window.executeApi(command,args);
        if(!response["ok"].toBool())qFatal("%s",QJsonDocument(response).toJson(QJsonDocument::Compact).constData());
        return response["result"].toObject();
    }
    QJsonObject session(AlloyWindow& window){return window.state()["activeSession"].toObject();}
private slots:
    void twoLinesThenConstructionAxis_data(){QTest::addColumn<bool>("arcs");QTest::newRow("lines")<<false;QTest::newRow("arcs")<<true;}
    void twoLinesThenConstructionAxis(){
        QFETCH(bool,arcs);
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1500,950);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        ReferenceView* view=nullptr;for(auto* child:window.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;
        QVERIFY(view);
        const auto started=call(window,"sketch.begin",{{"plane","XY"},{"units","mm"},{"name","Native symmetry"}});
        auto addLine=[&](double x1,double y1,double x2,double y2){
            call(window,"sketch.line",{{"x1",x1},{"y1",y1},{"x2",x2},{"y2",y2},{"units","mm"}});
            return session(window)["draftSketch"].toObject()["parameters"].toObject()["entities"].toArray().last().toObject();
        };
        auto addArc=[&](double x,double angle){call(window,"sketch.addArc",{{"mode","center"},{"centerX",x},{"centerY",0},{"radius",10},{"startAngleDeg",angle},{"sweepAngleDeg",180},{"units","mm"}});
            return session(window)["draftSketch"].toObject()["parameters"].toObject()["entities"].toArray().last().toObject();};
        const auto left=arcs ? addArc(-20,90) : addLine(-30,0,-30,30);
        const auto right=arcs ? addArc(20,270) : addLine(30,30,30,0);
        if(arcs){addLine(-20,-10,20,-10);addLine(20,10,-20,10);}
        const auto axis=addLine(0,-20,0,35);
        const auto other=addLine(-10,0,10,0);
        call(window,"sketch.construction",{{"entityIds",QJsonArray{axis["id"]}},{"construction",true}});
        call(window,"sketch.lookAt");
        auto* button=window.findChild<QPushButton*>("sketch.constraint.symmetry");QVERIFY(button);
        for(auto* area:window.findChildren<QScrollArea*>())if(area->isAncestorOf(button))area->ensureWidgetVisible(button);
#ifdef Q_OS_MACOS
        QProcess activation;activation.start("osascript",{"-e",QString("tell application \"System Events\" to set frontmost of first application process whose unix id is %1 to true").arg(QCoreApplication::applicationPid())});
        QVERIFY(activation.waitForFinished(5000));QCOMPARE(activation.exitCode(),0);
#endif
        window.raise();window.activateWindow();window.windowHandle()->requestActivate();QApplication::setActiveWindow(&window);
        QTRY_VERIFY(window.isActiveWindow());QVERIFY(button->isVisible());
        QTest::mouseClick(button,Qt::LeftButton);QCOMPARE(session(window)["constraintType"].toString(),QString("symmetry"));
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
        auto pixel=[&](double u,double v){const auto p=call(window,"view.projectSketchPoint",{{"sketchId",session(window)["draftSketch"].toObject()["id"]},{"uMm",u},{"vMm",v}});
            return QPoint(qRound(p["x"].toDouble()),qRound(p["y"].toDouble()));};
        const auto first=arcs ? pixel(-30,0) : pixel(-30,15),second=arcs ? pixel(30,0) : pixel(30,15),wrong=pixel(5,0),third=pixel(0,25);
        for(const auto& p:{first,second,wrong,third})QVERIFY(view->rect().contains(p));
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,first);
        QCOMPARE(session(window)["selection"].toObject()["entityIds"].toArray(),QJsonArray{left["id"]});
        if(arcs){
            const auto before=session(window)["draftSketch"].toObject();
            QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,wrong);
            QCOMPARE(session(window)["selection"].toObject()["entityIds"].toArray(),QJsonArray{left["id"]});
            QCOMPARE(session(window)["draftSketch"].toObject(),before);
            QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
        }
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,second);
        QCOMPARE(session(window)["selection"].toObject()["entityIds"].toArray(),(QJsonArray{left["id"],right["id"]}));
        QVERIFY(session(window)["constraintPrompt"].toString().contains("construction axis"));
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,wrong);
        QCOMPARE(session(window)["selection"].toObject()["entityIds"].toArray(),(QJsonArray{left["id"],right["id"]}));
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,third);
        const auto constraints=session(window)["draftSketch"].toObject()["parameters"].toObject()["constraints"].toArray();
        QCOMPARE(constraints.size(),1);QCOMPARE(constraints[0].toObject()["type"].toString(),QString("symmetry"));
        QCOMPARE(constraints[0].toObject()["entityIds"].toArray(),(QJsonArray{left["id"],right["id"],axis["id"]}));
        const auto glyphs=window.state()["viewport"].toObject()["gpu"].toObject()["sketchConstraintGlyphs"].toArray();
        int badges=0;for(const auto& value:glyphs)if(value.toObject()["constraintId"]==constraints[0].toObject()["id"])++badges;
        QCOMPARE(badges,1);
        const auto evidence=qEnvironmentVariable("ALLOY_SKETCH_SYMMETRY_EVIDENCE");
        if(!evidence.isEmpty()){QDir().mkpath(evidence);call(window,"capture.screenshot",{{"path",evidence+(arcs ? "/native-arc-symmetry-badge.png" : "/native-symmetry-badge.png")}});}
        QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
        QTest::keyClick(view,Qt::Key_Escape);QCOMPARE(session(window)["tool"].toString(),QString("select"));
        const auto badge=glyphs.last().toObject()["bounds"].toObject();
        const QPoint badgePixel(qRound(badge["x"].toDouble()+badge["width"].toDouble()/2),qRound(badge["y"].toDouble()+badge["height"].toDouble()/2));
        QVERIFY(view->rect().contains(badgePixel));QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,badgePixel);
        QCOMPARE(session(window)["selection"].toObject()["constraintIds"].toArray(),QJsonArray{constraints[0].toObject()["id"]});
        QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
        QTest::keyClick(view,Qt::Key_Delete);
        QCOMPARE(session(window)["draftSketch"].toObject()["parameters"].toObject()["constraints"].toArray().size(),0);
        Q_UNUSED(other);Q_UNUSED(started);call(window,"sketch.cancel");window.close();
    }
};
QTEST_MAIN(SketchSymmetryControls)
#include "sketch_symmetry_controls.moc"
