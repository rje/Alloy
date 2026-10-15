// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QDir>
#include <QJsonDocument>
#include <QLineEdit>
#include <QMouseEvent>
#include <QTest>
#include <cmath>

class TangentOriginDimensionControlsTest final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& w,const QString& name,const QJsonObject& args={}) {
        const auto response=w.execute(name,args);
        if(!response["ok"].toBool())qFatal("%s: %s",qPrintable(name),QJsonDocument(response).toJson().constData());
        return response["result"].toObject();
    }
    QJsonObject active(AlloyWindow& w) {return w.state()["activeSession"].toObject();}
    QJsonObject draft(AlloyWindow& w) {return active(w)["draftSketch"].toObject();}
private slots:
    void circleOriginAndDrivingDistance() {
        const auto source=qEnvironmentVariable("ALLOY_TANGENT_SOURCE"),out=qEnvironmentVariable("ALLOY_TANGENT_ORIGIN_EVIDENCE");
        QVERIFY(!source.isEmpty() && !out.isEmpty());QDir().mkpath(out);
        AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1380,880);w.show();QVERIFY(QTest::qWaitForWindowExposed(&w));
        ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;
        QVERIFY(view);call(w,"document.open",{{"path",source},{"discard",true}});
        const auto initial=call(w,"document.inspect"),body=initial["evaluation"].toObject()["bodies"].toArray().last().toObject();
        QVERIFY(!body.isEmpty());call(w,"component.activate",{{"definitionId",body["ownerId"]}});
        const auto face=body["supportFaces"].toArray().first().toObject(),edge=body["solidEdges"].toArray()[1].toObject();
        const QJsonObject tangent{{"face",QJsonObject{{"bodyId",body["bodyId"]},{"faceId",face["faceId"]},{"producerFeatureId",face["producerFeatureId"]}}},
            {"point",QJsonObject{{"bodyId",body["bodyId"]},{"edgeId",edge["edgeId"]},{"endpoint",0}}}};
        const auto plane=call(w,"construction.tangentPlane",{{"tangent",tangent},{"name","Endpoint tangent"}});
        const auto planeId=plane["planeId"].toString();QVERIFY(!planeId.isEmpty());
        call(w,"sketch.create",{{"planeId",planeId},{"name","Diameter 5 at 10"}});
        QCOMPARE(active(w)["command"].toString(),QString("sketch.edit"));
        QVERIFY(!draft(w)["parameters"].toObject().contains("originPointId"));
        call(w,"sketch.lookAt");call(w,"view.zoom",{{"factor",3}});
        const auto sketchId=draft(w)["id"].toString();
        auto pixel=[&](double u,double v){const auto p=call(w,"view.projectSketchPoint",{{"sketchId",sketchId},{"uMm",u},{"vMm",v}});
            return QPoint(qRound(p["x"].toDouble()),qRound(p["y"].toDouble()));};
        auto hitClick=[&](const QPoint& point){
            auto* recipient=QApplication::widgetAt(view->mapToGlobal(point));
            if(recipient!=view)return false;
            QTest::mouseClick(recipient,Qt::LeftButton,Qt::NoModifier,recipient->mapFromGlobal(view->mapToGlobal(point)));
            return true;
        };
        const auto center=pixel(0,10),origin=pixel(0,0);QVERIFY(view->rect().contains(center) && view->rect().contains(origin));
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
        QTest::keyClick(view,Qt::Key_C);QCOMPARE(active(w)["tool"].toString(),QString("circle"));
        QVERIFY(hitClick(center));QVERIFY(active(w).contains("firstPoint"));
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QTest::keyClick(view,Qt::Key_5);
        auto* inlineValue=w.findChild<QLineEdit*>("sketch.inlineValue");QVERIFY(inlineValue && inlineValue->isVisible());
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(inlineValue));QTest::keyClick(inlineValue,Qt::Key_Tab);
        QCOMPARE(active(w)["pendingCircleDiameter"].toObject()["phase"].toString(),QString("locked"));
        QTest::keyClick(QApplication::focusWidget(),Qt::Key_Return);QTRY_VERIFY(!active(w).contains("pendingCircleDiameter"));
        const auto circle=draft(w)["parameters"].toObject()["entities"].toArray().last().toObject();
        QCOMPARE(circle["type"].toString(),QString("circle"));QCOMPARE(circle["radiusMm"].toDouble(),2.5);
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QTest::keyClick(view,Qt::Key_D);
        QTRY_COMPARE(active(w)["tool"].toString(),QString("dimension"));
        QVERIFY(hitClick(center));
        QCOMPARE(active(w)["selection"].toObject()["pointIds"].toArray(),QJsonArray{circle["centerPointId"]});
        QVERIFY(hitClick(origin));
        const auto prepared=draft(w)["parameters"].toObject();const auto originId=prepared["originPointId"].toString();
        QVERIFY(!originId.isEmpty());QCOMPARE(prepared["entities"].toArray().size(),1);
        QCOMPARE(active(w)["selection"].toObject()["pointIds"].toArray(),(QJsonArray{circle["centerPointId"],originId}));
        const auto label=pixel(3,5);
        QMouseEvent move(QEvent::MouseMove,QPointF(label),QPointF(view->mapToGlobal(label)),
            Qt::NoButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(view,&move);
        QTRY_COMPARE(active(w)["pendingDimension"].toObject()["phase"].toString(),QString("placing"));
        QCOMPARE(active(w)["pendingDimension"].toObject()["kind"].toString(),QString("vertical"));
        call(w,"capture.screenshot",{{"path",out+"/origin-dimension-preview.png"}});
        QVERIFY(hitClick(label));
        QVERIFY(inlineValue->isVisible());QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(inlineValue));
        inlineValue->selectAll();QTest::keyClicks(inlineValue,"10");QTest::keyClick(inlineValue,Qt::Key_Return);
        QTRY_VERIFY(!active(w).contains("pendingDimension"));
        const auto dimensioned=draft(w)["parameters"].toObject();
        QCOMPARE(dimensioned["entities"].toArray().size(),1);
        bool located=false;for(const auto& value:dimensioned["dimensions"].toArray()){
            const auto row=value.toObject();if(row["kind"]=="vertical" && row["driving"].toBool()
                && row["pointIds"].toArray().contains(originId) && row["pointIds"].toArray().contains(circle["centerPointId"]) ) {
                located=std::abs(row["valueMm"].toDouble()-10)<1e-7;break;
            }
        }QVERIFY(located);
        call(w,"capture.screenshot",{{"path",out+"/origin-dimension-committed.png"}});
        call(w,"sketch.constraint",{{"type","horizontalVertical"},{"pointIds",QJsonArray{originId,circle["centerPointId"]}}});
        const auto finalDraft=draft(w)["parameters"].toObject();bool axis=false;
        for(const auto& value:finalDraft["constraints"].toArray()){const auto row=value.toObject();
            if(row["type"]=="vertical" && row["pointIds"].toArray()==QJsonArray{originId,circle["centerPointId"]})axis=true;
        }QVERIFY(axis);
        call(w,"sketch.finish");const auto finished=call(w,"document.inspect");
        const auto output=out+"/tangent-origin-dimension.alloy";call(w,"document.save",{{"path",output}});
        call(w,"document.new",{{"discard",true}});call(w,"document.open",{{"path",output},{"discard",true}});
        QCOMPARE(call(w,"document.inspect")["authored"],finished["authored"]);call(w,"app.quit");
    }
};
QTEST_MAIN(TangentOriginDimensionControlsTest)
#include "tangent_origin_dimension_controls.moc"
