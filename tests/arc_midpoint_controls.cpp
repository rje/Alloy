// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QJsonDocument>
#include <QLabel>
#include <QPushButton>
#include <QTest>
#include <cmath>

class ArcMidpointControlsTest final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& args={}){
        const auto response=window.execute(command,args);
        if(!response["ok"].toBool())qFatal("%s: %s",qPrintable(command),QJsonDocument(response).toJson().constData());
        return response["result"].toObject();
    }
private slots:
    void arcThenAttachedOrigin(){
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1440,1000);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QApplication::setActiveWindow(&window);window.activateWindow();
        call(window,"sketch.begin",{{"plane","YZ"},{"name","Shoehorn spine"}});
        call(window,"sketch.splineAdd",{{"points",QJsonArray{QJsonObject{{"uMm",0},{"vMm",0}},
            QJsonObject{{"uMm",50},{"vMm",16}},QJsonObject{{"uMm",130},{"vMm",0}}}}});
        const auto path=window.state()["activeSession"].toObject()["draftSketch"].toObject();
        const auto spline=path["parameters"].toObject()["entities"].toArray()[0].toObject();
        call(window,"sketch.finish");
        const auto plane=call(window,"construction.pathPlane",{{"path",QJsonObject{{"sketchId",path["id"]},
            {"entityId",spline["id"]},{"endpoint",0}}},{"name","Start Plane"}})["planeId"].toString();
        call(window,"sketch.begin",{{"planeId",plane},{"name","Front arc"}});
        call(window,"sketch.addArc",{{"mode","threePoint"},{"startX",-25},{"startY",10},
            {"throughX",0},{"throughY",0},{"endX",25},{"endY",10},{"units","mm"}});
        const auto draft=window.state()["activeSession"].toObject()["draftSketch"].toObject();
        const auto arc=draft["parameters"].toObject()["entities"].toArray()[0].toObject();
        ReferenceView* view=nullptr;for(auto* child:window.findChildren<QWidget*>())
            if(auto* found=dynamic_cast<ReferenceView*>(child)){view=found;break;}
        QVERIFY(view);
        auto* button=window.findChild<QPushButton*>("sketch.constraint.midpoint");
        QVERIFY(button && button->isVisible());
        QTest::mouseClick(button,Qt::LeftButton);
        QTRY_COMPARE(window.state()["activeSession"].toObject()["constraintType"].toString(),QString("midpoint"));
        QVERIFY(view->hasFocus());
        auto pixel=[&](double u,double v){const auto projected=call(window,"view.projectSketchPoint",
            {{"sketchId",draft["id"]},{"uMm",u},{"vMm",v}});
            return QPoint(qRound(projected["x"].toDouble()),qRound(projected["y"].toDouble()));};
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(-15,3.25));
        QTRY_COMPARE(window.state()["activeSession"].toObject()["selection"].toObject()["entityIds"].toArray(),QJsonArray{arc["id"]});
        QVERIFY(view->hasFocus());
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(0,0));
        const auto accepted=window.state()["activeSession"].toObject()["draftSketch"].toObject();
        const auto parameters=accepted["parameters"].toObject();
        const auto origin=parameters["originPointId"].toString();QVERIFY(!origin.isEmpty());
        bool retained=false;for(const auto& value:parameters["constraints"].toArray()){
            const auto relation=value.toObject();if(relation["type"]=="midpoint" &&
                relation["entityIds"].toArray()==QJsonArray{arc["id"]} &&
                relation["pointIds"].toArray()==QJsonArray{origin})retained=true;}
        QVERIFY(retained);
        bool badge=false;for(const auto& value:view->diagnostics()["sketchConstraintGlyphs"].toArray())
            if(value.toObject()["kind"]=="midpoint" && value.toObject()["entityIds"].toArray()==QJsonArray{arc["id"]})badge=true;
        QVERIFY(badge);
        QTest::qWait(100);
        auto* info=window.findChild<QLabel*>("sketch.info");
        auto* profile=window.findChild<QLabel*>("sketch.profileStatus");
        QVERIFY(info && profile);
        QVERIFY(info->geometry().bottom()<profile->geometry().top());
        const auto output=qEnvironmentVariable("ALLOY_ARC_MIDPOINT_EVIDENCE");
        if(!output.isEmpty())call(window,"capture.screenshot",{{"path",output+"/arc-midpoint-native.png"}});
        call(window,"sketch.dimension",{{"kind","horizontal"},
            {"pointIds",QJsonArray{arc["startPointId"],arc["endPointId"]}},{"value",50},{"units","mm"}});
        for(const auto& endpoint:{"startPointId","endPointId"})
            call(window,"sketch.dimension",{{"kind","vertical"},
                {"pointIds",QJsonArray{origin,arc[endpoint]}},{"value",10},{"units","mm"}});
        QCOMPARE(window.state()["activeSession"].toObject()["solverDiagnostics"].toObject()["dof"].toInt(),0);
        call(window,"sketch.tool",{{"tool","select"}});
        QTest::qWait(100);
        if(!output.isEmpty())call(window,"capture.screenshot",{{"path",output+"/arc-midpoint-sized.png"}});
        call(window,"command.cancel",{{"session",window.state()["activeSession"].toObject()["id"]}});
    }
};
QTEST_MAIN(ArcMidpointControlsTest)
#include "arc_midpoint_controls.moc"
