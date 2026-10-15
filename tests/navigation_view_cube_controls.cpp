// SPDX-License-Identifier: MIT
#include "AlloyTheme.h"
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QMouseEvent>
#include <QSet>
#include <QTest>
#include <QWindow>
#include <algorithm>
#include <cmath>

class NavigationViewCubeControls final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& name,const QJsonObject& args={}) {
        const auto response=window.executeApi(name,args);
        if(!response.value("ok").toBool())qFatal("%s",QJsonDocument(response).toJson(QJsonDocument::Compact).constData());
        return response.value("result").toObject();
    }
    static QJsonObject nav(AlloyWindow& window) {
        return window.state().value("viewport").toObject().value("navigation").toObject();
    }
    static QJsonArray direction(AlloyWindow& window) {
        return window.state().value("viewport").toObject().value("gpu").toObject()
            .value("camera").toObject().value("direction").toArray();
    }
private slots:
    void regionsAndSharedCamera() {
        const auto output=qEnvironmentVariable("ALLOY_NAV_CUBE_EVIDENCE");
        QVERIFY(!output.isEmpty());QVERIFY(QDir().mkpath(output));
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1360,850);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        ReferenceView* view=nullptr;for(auto* child:window.findChildren<QWidget*>())
            if(auto* found=dynamic_cast<ReferenceView*>(child))view=found;
        QVERIFY(view);
        call(window,"sketch.begin",{{"plane","XY"},{"units","mm"}});
        call(window,"sketch.rectangle",{{"x1",12},{"y1",6},{"x2",58},{"y2",33},{"units","mm"}});
        call(window,"sketch.finish");
        const auto sketch=call(window,"document.inspect").value("evaluation").toObject().value("sketches").toArray().first().toObject();
        call(window,"solid.extrude",{{"sketchId",sketch.value("sketchId")},
            {"profileId",sketch.value("profiles").toArray().first().toObject().value("profileId")},
            {"distance",13},{"units","mm"}});
        const auto body=call(window,"document.inspect").value("evaluation").toObject().value("bodies").toArray().first().toObject();
        call(window,"selection.set",{{"bodyId",body.value("bodyId")}});
        call(window,"view.home");call(window,"view.pan",{{"x",41},{"y",-18}});
        call(window,"view.zoom",{{"factor",1.15}});
        const auto projected=call(window,"view.projectBody",{{"bodyId",body.value("bodyId")}});
        call(window,"view.orbitCenterBegin");
        bool pivotFound=false;
        const QPoint approximate(qRound(projected.value("x").toDouble()),qRound(projected.value("y").toDouble()));
        for(int dy=-70;dy<=70&&!pivotFound;dy+=14)for(int dx=-70;dx<=70&&!pivotFound;dx+=14) {
            const QPoint candidate=approximate+QPoint(dx,dy);
            if(view->rect().contains(candidate)&&!view->pickOrbitCenter(candidate.x(),candidate.y()).isEmpty()) {
                call(window,"view.orbitCenterPick",{{"x",candidate.x()},{"y",candidate.y()}});pivotFound=true;
            }
        }
        QVERIFY(pivotFound);
        const auto pivot=nav(window).value("orbitPivot");
        const auto before=call(window,"document.inspect");
        const auto selection=window.state().value("selection");
        const auto zoom=nav(window).value("zoom"),panX=nav(window).value("panX"),panY=nav(window).value("panY");
        const auto regions=ReferenceView::cubeRegions();QCOMPARE(regions.size(),26);
        QStringList poses;
        for(const auto& region:regions) {
            const auto state=call(window,"view.cubeOrient",{{"region",region}});
            QCOMPARE(state.value("orientation"),QJsonValue("custom"));
            QCOMPARE(state.value("orbitPivot"),pivot);
            QCOMPARE(nav(window).value("zoom"),zoom);
            QCOMPARE(nav(window).value("panX"),panX);
            QCOMPARE(nav(window).value("panY"),panY);
            const auto vector=direction(window);QCOMPARE(vector.size(),3);
            for(const auto& coordinate:vector)QVERIFY(std::isfinite(coordinate.toDouble()));
            poses.append(QString::number(vector[0].toDouble(),'f',4)+","+
                QString::number(vector[1].toDouble(),'f',4)+","+
                QString::number(vector[2].toDouble(),'f',4));
        }
        QSet<QString> unique;for(const auto& pose:poses)unique.insert(pose);
        QCOMPARE(unique.size(),26);
        auto hitFor=[&](const QString& kind) {
            QPoint chosen(-1,-1);
            for(int y=2;y<std::min(view->height(),180)&&chosen.x()<0;y+=3)
                for(int x=std::max(0,view->width()-180);x<view->width()&&chosen.x()<0;x+=3)
                    if(view->cubeHit(x,y).value("kind")==kind)chosen=QPoint(x,y);
            return chosen;
        };
        for(const auto& kind:{"face","edge","corner"}) {
            const QPoint pixel=hitFor(kind);QVERIFY2(pixel.x()>=0,qPrintable(QString("missing cube ")+kind));
            const auto hit=view->cubeHit(pixel.x(),pixel.y());
            const auto expected=hit.value("region").toString();QVERIFY(!expected.isEmpty());
            if(QString::fromUtf8(kind)=="face") {
                QMouseEvent hover(QEvent::MouseMove,QPointF(pixel),view->mapToGlobal(pixel),
                    Qt::NoButton,Qt::NoButton,Qt::NoModifier);
                QApplication::sendEvent(view,&hover);
                QCOMPARE(view->cursor().shape(),Qt::PointingHandCursor);
                QCOMPARE(view->cubeHit(pixel.x(),pixel.y()).value("region"),QJsonValue(expected));
                call(window,"capture.screenshot",{{"path",output+"/cube-face-hover.png"}});
            }
            QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel);
            const auto clicked=direction(window);
            call(window,"view.cubeOrient",{{"region",expected}});
            QCOMPARE(direction(window),clicked);
            QCOMPARE(nav(window).value("orbitPivot"),pivot);
        }
        QCOMPARE(call(window,"document.inspect").value("authored"),before.value("authored"));
        QCOMPARE(call(window,"document.inspect").value("undo"),before.value("undo"));
        QCOMPARE(call(window,"document.inspect").value("revision"),before.value("revision"));
        QCOMPARE(window.state().value("selection"),selection);
        call(window,"capture.screenshot",{{"path",output+"/cube-after-corner.png"}});
        // A cube press becomes an orientation click only on release. Once the
        // pointer crosses Qt's drag threshold it uses the shared orbit command
        // about the ordinary view target, leaving Set Orbit Center stored.
        const auto face=hitFor("face");QVERIFY(face.x()>=0);
        const auto beforeDrag=nav(window);
        const auto beforeDirection=direction(window);
        const auto center=beforeDrag.value("viewCenter").toArray();
        const auto storedPivot=beforeDrag.value("orbitPivot").toArray();
        QVERIFY(center.size()==3 && storedPivot.size()==3);
        double pivotSeparation=0;
        for(int i=0;i<3;++i)pivotSeparation+=std::pow(center[i].toDouble()-storedPivot[i].toDouble(),2);
        QVERIFY(pivotSeparation>1);
        QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,face);
        QVERIFY(view->cubeGestureActive());
        QCOMPARE(direction(window),beforeDirection);
        auto dragMove=[&](QPoint local){
            QMouseEvent move(QEvent::MouseMove,QPointF(local),view->mapToGlobal(local),
                Qt::NoButton,Qt::LeftButton,Qt::NoModifier);
            QApplication::sendEvent(view,&move);
        };
        dragMove(face+QPoint(27,15));dragMove(face+QPoint(49,29));
        const auto dragged=direction(window);QVERIFY(dragged!=beforeDirection);
        QCOMPARE(nav(window).value("viewCenter"),QJsonValue(center));
        QCOMPARE(nav(window).value("orbitPivot"),QJsonValue(storedPivot));
        QTest::mouseRelease(view,Qt::LeftButton,Qt::NoModifier,face+QPoint(49,29));
        QVERIFY(!view->cubeGestureActive());QCOMPARE(direction(window),dragged);
        view->sync();QCOMPARE(direction(window),dragged);
        const auto tab=call(window,"document.tabs").value("activeTabId").toString();QVERIFY(!tab.isEmpty());
        call(window,"document.tabNew");call(window,"document.tabActivate",{{"tabId",tab}});
        QCOMPARE(direction(window),dragged);
        QCOMPARE(nav(window).value("orbitPivot"),QJsonValue(storedPivot));
        call(window,"capture.screenshot",{{"path",output+"/cube-dragged.png"}});
        const QJsonObject geometry{{"viewWidth",view->width()},{"viewHeight",view->height()},
            {"viewDevicePixelRatio",view->devicePixelRatioF()},
            {"nativeWidth",view->windowHandle() ? view->windowHandle()->width() : 0},
            {"nativeHeight",view->windowHandle() ? view->windowHandle()->height() : 0},
            {"windowWidth",window.width()},{"windowHeight",window.height()}};
        QFile geometryFile(output+"/viewport-geometry.json");QVERIFY(geometryFile.open(QIODevice::WriteOnly));
        geometryFile.write(QJsonDocument(geometry).toJson(QJsonDocument::Indented));geometryFile.close();
        call(window,"view.orbit",{{"deltaX",17},{"deltaY",8}});
        QVERIFY(nav(window).value("viewCenter")!=QJsonValue(center));
        QCOMPARE(nav(window).value("orbitPivot"),QJsonValue(storedPivot));
        QCOMPARE(call(window,"document.inspect").value("authored"),before.value("authored"));
        QCOMPARE(call(window,"document.inspect").value("undo"),before.value("undo"));
        QCOMPARE(window.state().value("selection"),selection);
        call(window,"document.save",{{"path",output+"/view-cube.alloy"}});window.close();
    }
};
QTEST_MAIN(NavigationViewCubeControls)
#include "navigation_view_cube_controls.moc"
