// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include "SketchSpline.h"
#include <QAction>
#include <QApplication>
#include <QDir>
#include <QJsonDocument>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QTest>
#include <array>
#include <cmath>

class SketchMirrorControls final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& w,const QString& id,const QJsonObject& args={}){
        const auto response=w.execute(id,args);
        if(!response["ok"].toBool())qFatal("%s",QJsonDocument(response).toJson().constData());
        return response["result"].toObject();
    }
    QJsonObject active(AlloyWindow& w){return w.state()["activeSession"].toObject();}
    QJsonObject draft(AlloyWindow& w){return active(w)["draftSketch"].toObject();}
private slots:
    void fitSplinePointSelectionPreviewCancelAndAccept(){
        AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1500,950);w.show();w.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&w));QApplication::setActiveWindow(&w);
        ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;
        QVERIFY(view);call(w,"sketch.begin",{{"plane","XY"},{"units","mm"},{"name","Spline Mirror"}});call(w,"sketch.lookAt");
        call(w,"sketch.line",{{"x1",-40},{"y1",0},{"x2",40},{"y2",0},{"units","mm"}});
        const auto axis=draft(w)["parameters"].toObject()["entities"].toArray().last().toObject();
        call(w,"sketch.construction",{{"entityIds",QJsonArray{axis["id"]}},{"construction",true}});
        call(w,"sketch.splineAdd",{{"points",QJsonArray{QJsonObject{{"uMm",-30},{"vMm",14}},QJsonObject{{"uMm",-12},{"vMm",28}},QJsonObject{{"uMm",8},{"vMm",24}},QJsonObject{{"uMm",24},{"vMm",10}}}}});
        const auto source=draft(w)["parameters"].toObject()["entities"].toArray().last().toObject();
        call(w,"sketch.splineTangent",{{"entityId",source["id"]},{"pointId",source["pointIds"].toArray().first()},{"duMm",60},{"dvMm",20}});
        call(w,"sketch.splineTangent",{{"entityId",source["id"]},{"pointId",source["pointIds"].toArray().last()},{"duMm",30},{"dvMm",-30}});
        const auto original=draft(w);const auto curve=SketchSpline::evaluate(original["parameters"].toObject(),original["parameters"].toObject()["entities"].toArray().last().toObject()).curve;
        auto pixel=[&](double u,double v){const auto p=call(w,"view.projectSketchPoint",{{"sketchId",original["id"]},{"uMm",u},{"vMm",v}});return QPoint(qRound(p["x"].toDouble()),qRound(p["y"].toDouble()));};
        auto* action=w.findChild<QAction*>("sketch.action.mirror");QVERIFY(action);
        auto* axisButton=w.findChild<QPushButton*>("sketch.mirror.role.axis");auto* preview=w.findChild<QPushButton*>("sketch.mirror.preview");
        auto* cancel=w.findChild<QPushButton*>("sketch.mirror.cancel");auto* accept=w.findChild<QPushButton*>("sketch.mirror.accept");QVERIFY(axisButton && preview && cancel && accept);
        auto collect=[&]{action->trigger();QTRY_VERIFY(axisButton->isVisible());QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
            const auto p=curve->Value(.18);
            auto hit=[&]{const auto at=pixel(p.X(),p.Y());return view->pickSketch(original,at.x(),at.y(),{},active(w)["resolvedFrame"].toObject());};
            QTRY_COMPARE(hit()["entityId"],source["id"]);QTRY_COMPARE(hit()["kind"].toString(),QString("entity"));
            QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(p.X(),p.Y()));
            QCOMPARE(active(w)["mirrorSelection"].toObject()["sourceEntityIds"].toArray(),QJsonArray{source["id"]});QCOMPARE(draft(w),original);
            QTest::mouseClick(axisButton,Qt::LeftButton);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
            QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(0,0));QCOMPARE(active(w)["mirrorSelection"].toObject()["axisEntityId"],axis["id"]);
            QTest::mouseClick(preview,Qt::LeftButton);QVERIFY(active(w).contains("pendingSketchMirror"));QCOMPARE(draft(w),original);};
        collect();if(QTest::currentTestFailed())return;const auto output=qEnvironmentVariable("ALLOY_SKETCH_MIRROR_CONTROLS_EVIDENCE");
        if(!output.isEmpty()){QDir().mkpath(output);QTest::qWait(40);call(w,"capture.screenshot",{{"path",output+"/spline-mirror-preview.png"}});}
        QTest::mouseClick(cancel,Qt::LeftButton);QVERIFY(!active(w).contains("pendingSketchMirror"));QCOMPARE(draft(w),original);
        collect();if(QTest::currentTestFailed())return;QTest::mouseClick(accept,Qt::LeftButton);QCOMPARE(active(w)["tool"].toString(),QString("select"));
        const auto group=draft(w)["parameters"].toObject()["mirrorGroups"].toArray().first().toObject();QCOMPARE(group["pointPairs"].toArray().size(),4);
        if(!output.isEmpty())call(w,"capture.screenshot",{{"path",output+"/spline-mirror-accepted.png"}});
        call(w,"sketch.cancel");w.close();
    }
    void enclosedSourcesAxisCancelAndAccept(){
        AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1500,950);w.show();w.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&w));QApplication::setActiveWindow(&w);
        ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;
        QVERIFY(view);call(w,"sketch.begin",{{"plane","XY"},{"units","mm"}});call(w,"sketch.lookAt");
        auto addLine=[&](double x1,double y1,double x2,double y2){call(w,"sketch.line",{{"x1",x1},{"y1",y1},{"x2",x2},{"y2",y2},{"units","mm"}});
            return draft(w)["parameters"].toObject()["entities"].toArray().last().toObject()["id"].toString();};
        for(const auto& segment:std::initializer_list<std::array<double,4>>{{-50,-50,50,-50},{50,-50,50,50},{50,50,-50,50},{-50,50,-50,-50}})
            addLine(segment[0],segment[1],segment[2],segment[3]);
        const auto diagonal1=addLine(-45,0,45,45),diagonal2=addLine(-45,45,45,0);
        call(w,"sketch.construction",{{"entityIds",QJsonArray{diagonal1,diagonal2}},{"construction",true}});
        call(w,"sketch.addCircle",{{"centerX",0},{"centerY",0},{"diameter",12},{"units","mm"}});
        QStringList source;
        for(const auto& segment:std::initializer_list<std::array<double,4>>{{-14,10,12,10},{12,10,12,36},{12,36,-14,36},{-14,36,-14,10}})
            source.append(addLine(segment[0],segment[1],segment[2],segment[3]));
        const auto axisId=addLine(-25,-3,25,-3);
        call(w,"sketch.construction",{{"entityIds",QJsonArray{source[1],source[3],axisId}},{"construction",true}});
        for(const auto& side:{-1,1}){
            const double x=side<0 ? -14 : 12,through=x+side*std::sqrt(56.0);
            call(w,"sketch.addArc",{{"mode","threePoint"},{"startX",x},{"startY",side<0 ? 36 : 10},
                {"endX",x},{"endY",side<0 ? 10 : 36},{"throughX",through},{"throughY",23},{"units","mm"}});
            source.append(draft(w)["parameters"].toObject()["entities"].toArray().last().toObject()["id"].toString());
        }
        const auto original=draft(w);QCOMPARE(source.size(),6);
        const auto sketchId=original["id"].toString();
        auto pixel=[&](double u,double v){const auto p=call(w,"view.projectSketchPoint",{{"sketchId",sketchId},{"uMm",u},{"vMm",v}});
            return QPoint(qRound(p["x"].toDouble()),qRound(p["y"].toDouble()));};
        auto* action=w.findChild<QAction*>("sketch.action.mirror");QVERIFY(action);
        auto* axisButton=w.findChild<QPushButton*>("sketch.mirror.role.axis");
        auto* preview=w.findChild<QPushButton*>("sketch.mirror.preview");
        auto* cancel=w.findChild<QPushButton*>("sketch.mirror.cancel");
        auto* accept=w.findChild<QPushButton*>("sketch.mirror.accept");
        QVERIFY(axisButton && preview && cancel && accept);
        auto collect=[&]{
            action->trigger();QCOMPARE(active(w)["tool"].toString(),QString("mirror"));
            QTRY_VERIFY(axisButton->isVisible());
            const auto a=pixel(-24,39),b=pixel(23,7);QVERIFY(view->rect().contains(a));QVERIFY(view->rect().contains(b));
            QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
            QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,a);
            const QPoint middle((a.x()+b.x())/2,(a.y()+b.y())/2);
            QMouseEvent move(QEvent::MouseMove,QPointF(middle),QPointF(view->mapToGlobal(middle)),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);
            QApplication::sendEvent(view,&move);QTest::mouseRelease(view,Qt::LeftButton,Qt::NoModifier,b);
            auto chosen=active(w)["mirrorSelection"].toObject();QCOMPARE(chosen["sourceEntityIds"].toArray().size(),6);
            QCOMPARE(chosen["pointIds"].toArray().size(),6);
            for(const auto& id:chosen["sourceEntityIds"].toArray())QVERIFY(source.contains(id.toString()));
            QCOMPARE(draft(w),original);
            QTest::mouseClick(axisButton,Qt::LeftButton);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
            QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(0,-3));
            chosen=active(w)["mirrorSelection"].toObject();QCOMPARE(chosen["axisEntityId"].toString(),axisId);
            QTest::mouseClick(preview,Qt::LeftButton);QVERIFY(active(w).contains("pendingSketchMirror"));QCOMPARE(draft(w),original);
        };
        collect();
        const auto output=qEnvironmentVariable("ALLOY_SKETCH_MIRROR_CONTROLS_EVIDENCE");
        auto* summary=w.findChild<QLabel*>("sketch.mirror.summary");QVERIFY(summary);
        QVERIFY(summary->text().contains("12 objects"));
        QTRY_VERIFY(summary->height()>=summary->heightForWidth(summary->width()));
        if(!output.isEmpty()){QDir().mkpath(output);QTest::qWait(40);call(w,"capture.screenshot",{{"path",output+"/mirror-window-preview.png"}});}
        QTest::mouseClick(cancel,Qt::LeftButton);QVERIFY(!active(w).contains("pendingSketchMirror"));QCOMPARE(draft(w),original);
        collect();QTest::mouseClick(accept,Qt::LeftButton);
        QCOMPARE(active(w)["tool"].toString(),QString("select"));QVERIFY(!active(w).contains("pendingSketchMirror"));
        QTRY_VERIFY(!w.findChild<QWidget*>("sketch.mirror.controls")->isVisible());
        const auto groups=draft(w)["parameters"].toObject()["mirrorGroups"].toArray();QCOMPARE(groups.size(),1);
        QCOMPARE(groups[0].toObject()["entityPairs"].toArray().size(),6);
        if(!output.isEmpty()){QTest::qWait(40);call(w,"capture.screenshot",{{"path",output+"/mirror-window-accepted.png"}});}
        call(w,"sketch.cancel");w.close();
    }
};
QTEST_MAIN(SketchMirrorControls)
#include "sketch_mirror_controls.moc"
