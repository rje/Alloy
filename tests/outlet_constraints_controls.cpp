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
#include <cmath>
#include <numbers>

class OutletConstraintsControlsTest final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& w,const QString& name,const QJsonObject& args={}){
        const auto response=w.execute(name,args);if(!response["ok"].toBool())qFatal("%s",QJsonDocument(response).toJson().constData());return response["result"].toObject();
    }
    QJsonObject session(AlloyWindow& w){return w.state()["activeSession"].toObject();}
    QJsonObject draft(AlloyWindow& w){return session(w)["draftSketch"].toObject();}
    QJsonObject entity(AlloyWindow& w,int index){return draft(w)["parameters"].toObject()["entities"].toArray()[index].toObject();}
    QJsonObject point(AlloyWindow& w,const QJsonValue& id){for(const auto& value:draft(w)["parameters"].toObject()["points"].toArray())if(value.toObject()["id"]==id)return value.toObject();return {};}
private slots:
    void nativeRelations(){
        AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1440,1000);w.show();w.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&w));
        ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;QVERIFY(view);
        call(w,"sketch.begin",{{"plane","XY"}});
        call(w,"sketch.rectangle",{{"x1",-16},{"y1",40},{"x2",15},{"y2",13},{"units","mm"}});
        const auto leftSide=entity(w,3),rightSide=entity(w,1);
        call(w,"sketch.addCircle",{{"centerX",0},{"centerY",0},{"diameter",10},{"units","mm"}});
        const auto circle=entity(w,4);
        call(w,"sketch.addArc",{{"mode","threePoint"},{"startX",-16},{"startY",40},{"throughX",-25},{"throughY",26.5},{"endX",-16},{"endY",13},{"units","mm"}});
        call(w,"sketch.addArc",{{"mode","threePoint"},{"startX",15},{"startY",13},{"throughX",24},{"throughY",26.5},{"endX",15},{"endY",40},{"units","mm"}});
        const auto leftArc=entity(w,5),rightArc=entity(w,6);
        call(w,"sketch.lookAt");
        auto pixel=[&](double u,double v){const auto p=call(w,"view.projectSketchPoint",{{"sketchId",draft(w)["id"]},{"uMm",u},{"vMm",v}});return QPoint(qRound(p["x"].toDouble()),qRound(p["y"].toDouble()));};
        auto hover=[&](const QPoint& p){QMouseEvent move(QEvent::MouseMove,QPointF(p),QPointF(view->mapToGlobal(p)),Qt::NoButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(view,&move);QApplication::processEvents();};
        call(w,"sketch.tool",{{"tool","line"}});
        hover(pixel(-16,26.5));QTRY_COMPARE(session(w)["snap"].toObject()["target"].toObject()["kind"].toString(),QString("midpoint"));
        QCOMPARE(view->diagnostics()["sketchSnapCue"].toObject()["marker"].toString(),QString("midpoint-triangle"));
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(-16,26.5));
        hover(pixel(15,26.5));QTRY_COMPARE(session(w)["snap"].toObject()["target"].toObject()["kind"].toString(),QString("midpoint"));
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(15,26.5));
        const auto bridge=entity(w,7);QVERIFY(bridge["type"]=="line");
        const auto constraints=draft(w)["parameters"].toObject()["constraints"].toArray();
        int midpoint=0;for(const auto& value:constraints)if(value.toObject()["type"]=="midpoint")++midpoint;QCOMPARE(midpoint,2);
        call(w,"sketch.tool",{{"tool","select"}});
        call(w,"sketch.selectionSet",{{"entityIds",QJsonArray{leftSide["id"],rightSide["id"],bridge["id"]}}});
        call(w,"sketch.constructionToggle");
        QCOMPARE(entity(w,7)["construction"].toBool(),true);
        call(w,"sketch.selectionSet",{{"pointIds",QJsonArray{}},{"entityIds",QJsonArray{}}});
        call(w,"sketch.constraintTool",{{"type","concentric"}});
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(-25,26.5));
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(24,26.5));
        QTRY_VERIFY(draft(w)["parameters"].toObject()["constraints"].toArray().last().toObject()["type"]=="concentric");
        call(w,"sketch.constraintTool",{{"type","horizontalVertical"}});
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(0,0));
        const auto center=point(w,leftArc["centerPointId"]);QVERIFY(!center.isEmpty());
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(center["uMm"].toDouble(),center["vMm"].toDouble()));
        QTRY_VERIFY(draft(w)["parameters"].toObject()["constraints"].toArray().last().toObject()["type"]=="vertical");
        call(w,"sketch.constraintTool",{{"type","equal"}});
        const auto a=entity(w,5),b=entity(w,6);const auto ac=point(w,a["centerPointId"]),bc=point(w,b["centerPointId"]);
        const double aa=(a["startAngleDeg"].toDouble()+a["sweepAngleDeg"].toDouble()/2)*std::numbers::pi/180.0;
        const double ba=(b["startAngleDeg"].toDouble()+b["sweepAngleDeg"].toDouble()/2)*std::numbers::pi/180.0;
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(ac["uMm"].toDouble()+a["radiusMm"].toDouble()*std::cos(aa),ac["vMm"].toDouble()+a["radiusMm"].toDouble()*std::sin(aa)));
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(bc["uMm"].toDouble()+b["radiusMm"].toDouble()*std::cos(ba),bc["vMm"].toDouble()+b["radiusMm"].toDouble()*std::sin(ba)));
        QTRY_VERIFY(draft(w)["parameters"].toObject()["constraints"].toArray().last().toObject()["type"]=="equal");
        const auto output=qEnvironmentVariable("ALLOY_OUTLET_CONSTRAINTS_EVIDENCE");
        if(!output.isEmpty()){QDir().mkpath(output);call(w,"sketch.tool",{{"tool","select"}});call(w,"capture.screenshot",{{"path",output+"/outlet-constraints-native.png"}});QFile file(output+"/outlet-constraints-state.json");QVERIFY(file.open(QIODevice::WriteOnly));file.write(QJsonDocument(w.state()).toJson());}
        call(w,"command.cancel",{{"session",session(w)["id"]}});w.close();
    }
};
QTEST_MAIN(OutletConstraintsControlsTest)
#include "outlet_constraints_controls.moc"
