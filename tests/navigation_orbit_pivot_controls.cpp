// SPDX-License-Identifier: MIT
#include "AlloyTheme.h"
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QContextMenuEvent>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QMenu>
#include <QMouseEvent>
#include <QPushButton>
#include <QTest>
#include <QToolButton>
#include <QWindow>
#include <cmath>

class NavigationOrbitPivotControls final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& args={}) {
        const auto response=window.executeApi(command,args);
        if(!response.value("ok").toBool())qFatal("%s",QJsonDocument(response).toJson(QJsonDocument::Compact).constData());
        return response.value("result").toObject();
    }
    static QJsonObject navigation(AlloyWindow& window) {
        return window.state().value("viewport").toObject().value("navigation").toObject();
    }
    static QJsonObject actualCamera(AlloyWindow& window) {
        return window.state().value("viewport").toObject().value("gpu").toObject().value("camera").toObject();
    }
    static double separation(const QJsonArray& a,const QJsonArray& b) {
        double sum=0;for(int i=0;i<3;++i)sum+=std::pow(a[i].toDouble()-b[i].toDouble(),2);return std::sqrt(sum);
    }
    static void pointer(QWidget* view,QEvent::Type type,const QPoint& pos,Qt::MouseButton button,
        Qt::MouseButtons buttons,Qt::KeyboardModifiers modifiers) {
        QMouseEvent event(type,QPointF(pos),QPointF(view->mapToGlobal(pos)),button,buttons,modifiers);
        QApplication::sendEvent(view,&event);
    }
    static void drag(QWidget* view,const QPoint& start,const QPoint& end,Qt::MouseButton button,Qt::KeyboardModifiers mods) {
        pointer(view,QEvent::MouseButtonPress,start,button,button,mods);
        pointer(view,QEvent::MouseMove,end,Qt::NoButton,button,mods);
        pointer(view,QEvent::MouseButtonRelease,end,button,Qt::NoButton,mods);
    }
    static QMenu* blankMenu(ReferenceView* view,const QPoint& at) {
        QContextMenuEvent event(QContextMenuEvent::Mouse,at,view->mapToGlobal(at));QApplication::sendEvent(view,&event);
        auto* menu=qobject_cast<QMenu*>(QApplication::activePopupWidget());
        if(!menu || menu->objectName()!="context.orbitCenter")qFatal("blank-canvas orbit menu unavailable");
        return menu;
    }
    static QAction* action(QMenu* menu,const QString& name) {
        for(auto* row:menu->actions())if(row->objectName()==name)return row;
        return nullptr;
    }
    static void choose(QMenu* menu,QAction* row) {
        if(!row || !row->isEnabled())qFatal("orbit menu action unavailable");
        QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,menu->actionGeometry(row).center());
    }
    static double pointDistance(const QJsonObject& a,const QJsonObject& b) {
        return std::hypot(a.value("x").toDouble()-b.value("x").toDouble(),
            a.value("y").toDouble()-b.value("y").toDouble());
    }
    static void checkLevel(const QJsonObject& camera) {
        const auto d=camera.value("direction").toArray(),u=camera.value("up").toArray();
        if(d.size()!=3 || u.size()!=3)qFatal("camera vectors unavailable");
        const double dot=d[2].toDouble();const double magnitude=std::sqrt(std::max(0.0,1-dot*dot));
        if(!std::isfinite(magnitude) || magnitude<0.08 || u[2].toDouble()<0.07)qFatal("constrained camera hit a pole or flipped horizon");
        const QJsonArray expected{-dot*d[0].toDouble()/magnitude,-dot*d[1].toDouble()/magnitude,
            (1-dot*dot)/magnitude};
        if(separation(u,expected)>1e-4)qFatal("constrained camera up is not projected world Z");
    }
private slots:
    void sharedCameraCommandsWithoutDesktopFocus() {
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1300,820);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        ReferenceView* view=nullptr;for(auto* child:window.findChildren<QWidget*>())
            if(auto* found=dynamic_cast<ReferenceView*>(child))view=found;
        QVERIFY(view);
        call(window,"sketch.begin",{{"plane","XY"},{"units","mm"}});
        call(window,"sketch.rectangle",{{"x1",20},{"y1",10},{"x2",60},{"y2",30},{"units","mm"}});
        call(window,"sketch.finish");
        const auto sketch=call(window,"document.inspect").value("evaluation").toObject().value("sketches").toArray().first().toObject();
        call(window,"solid.extrude",{{"sketchId",sketch.value("sketchId")},
            {"profileId",sketch.value("profiles").toArray().first().toObject().value("profileId")},
            {"distance",12},{"units","mm"}});
        const auto body=call(window,"document.inspect").value("evaluation").toObject().value("bodies").toArray().first().toObject();
        call(window,"view.home");
        const auto document=call(window,"document.inspect");
        const auto projected=call(window,"view.projectBody",{{"bodyId",body.value("bodyId")}});
        const QPoint center(qRound(projected.value("x").toDouble()),qRound(projected.value("y").toDouble()));
        QJsonObject hit;QPoint pixel;
        for(const QPoint delta:{QPoint(42,0),QPoint(-42,0),QPoint(0,24),QPoint(0,-24),QPoint(0,0)}) {
            const auto candidate=center+delta;
            if(candidate.x()<0 || candidate.y()<0 || candidate.x()>=view->width() || candidate.y()>=view->height())continue;
            hit=view->pickOrbitCenter(candidate.x(),candidate.y());
            if(!hit.isEmpty()){pixel=candidate;break;}
        }
        QVERIFY2(!hit.isEmpty(),"visible solid pivot ray was unavailable");
        call(window,"view.orbitCenterBegin");
        call(window,"view.orbitCenterPick",{{"x",pixel.x()},{"y",pixel.y()}});
        QVERIFY(navigation(window).value("customOrbitCenter").toBool());
        call(window,"view.pan",{{"x",23},{"y",-15}});
        // A rolled Free view must level without moving the off-center pivot.
        call(window,"view.orbit",{{"deltaX",83},{"deltaY",57}});
        const auto beforeSwitch=navigation(window).value("pivotScreen").toObject();
        call(window,"view.orbitMode",{{"mode","constrained"}});
        QVERIFY(pointDistance(beforeSwitch,navigation(window).value("pivotScreen").toObject())<1.5);
        checkLevel(actualCamera(window));
        const auto beforeOrbit=navigation(window).value("pivotScreen").toObject();
        call(window,"view.orbit",{{"deltaX",-66},{"deltaY",37}});
        QVERIFY(pointDistance(beforeOrbit,navigation(window).value("pivotScreen").toObject())<1.5);
        checkLevel(actualCamera(window));
        const auto out=qEnvironmentVariable("ALLOY_NAV_PIVOT_EVIDENCE");
        QVERIFY(!out.isEmpty());QVERIFY(QDir().mkpath(out));
        call(window,"capture.screenshot",{{"path",out+"/api-constrained-pivot.png"}});
        QFile state(out+"/api-camera-state.json");QVERIFY(state.open(QIODevice::WriteOnly));
        state.write(QJsonDocument(QJsonObject{{"navigation",navigation(window)},
            {"gpuCamera",actualCamera(window)}}).toJson(QJsonDocument::Indented));state.close();
        const auto preTab=navigation(window);
        const auto tab=call(window,"document.tabs").value("activeTabId").toString();QVERIFY(!tab.isEmpty());
        call(window,"document.tabNew");call(window,"document.tabActivate",{{"tabId",tab}});
        QCOMPARE(navigation(window).value("orbitMode"),preTab.value("orbitMode"));
        QCOMPARE(navigation(window).value("orbitPivot"),preTab.value("orbitPivot"));
        QCOMPARE(navigation(window).value("viewCenter"),preTab.value("viewCenter"));
        call(window,"view.orientation",{{"name","top"}});
        const auto named=navigation(window);
        QCOMPARE(named.value("viewCenter"),QJsonValue(QJsonArray{named.value("centerX"),named.value("centerY"),named.value("centerZ")}));
        QVERIFY(named.value("customOrbitCenter").toBool());
        call(window,"view.orbitCenterReset");
        QVERIFY(!navigation(window).value("customOrbitCenter").toBool());
        QCOMPARE(call(window,"document.inspect").value("authored"),document.value("authored"));
        QCOMPARE(call(window,"document.inspect").value("undo"),document.value("undo"));
        call(window,"document.save",{{"path",out+"/api-orbit-pivot.alloy"}});window.close();
    }
    void actualModeAndOffCenterPivot() {
        const auto out=qEnvironmentVariable("ALLOY_NAV_PIVOT_EVIDENCE");QVERIFY(!out.isEmpty());QVERIFY(QDir().mkpath(out));
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1460,900);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        ReferenceView* view=nullptr;for(auto* child:window.findChildren<QWidget*>())
            if(auto* found=dynamic_cast<ReferenceView*>(child))view=found;
        QVERIFY(view);
        // One initial activation before setup and native inputs; no later focus repair.
        window.raise();window.activateWindow();window.windowHandle()->requestActivate();QTRY_VERIFY(window.isActiveWindow());
        // API geometry setup.
        call(window,"sketch.begin",{{"plane","XY"},{"units","mm"}});
        call(window,"sketch.rectangle",{{"x1",20},{"y1",10},{"x2",60},{"y2",30},{"units","mm"}});
        call(window,"sketch.finish");
        const auto sketch=call(window,"document.inspect").value("evaluation").toObject().value("sketches").toArray().first().toObject();
        call(window,"solid.extrude",{{"sketchId",sketch.value("sketchId")},
            {"profileId",sketch.value("profiles").toArray().first().toObject().value("profileId")},
            {"distance",12},{"units","mm"}});
        const auto body=call(window,"document.inspect").value("evaluation").toObject().value("bodies").toArray().first().toObject();
        call(window,"selection.set",{{"bodyId",body.value("bodyId")}});call(window,"view.home");
        const auto initial=call(window,"document.inspect");
        const auto selection=window.state().value("selection");
        const QPoint middle(view->width()/2,view->height()/2);
        QCOMPARE(QApplication::widgetAt(view->mapToGlobal(middle)),static_cast<QWidget*>(view));
        const auto before=actualCamera(window);
        drag(view,middle,middle+QPoint(75,30),Qt::MiddleButton,Qt::ShiftModifier);
        QVERIFY(separation(before.value("direction").toArray(),actualCamera(window).value("direction").toArray())>0.08);
        QCOMPARE(navigation(window).value("orbitMode"),QJsonValue("free"));
        call(window,"view.home");

        auto* mode=window.findChild<QToolButton*>("view.orbitMode");auto* tool=window.findChild<QPushButton*>("view.orbitTool");
        QVERIFY(mode && mode->menu() && tool);
        QTest::mouseClick(mode,Qt::LeftButton);QTRY_VERIFY(mode->menu()->isVisible());
        choose(mode->menu(),action(mode->menu(),"view.orbitMode.constrained"));
        QTRY_COMPARE(navigation(window).value("orbitMode"),QJsonValue("constrained"));
        QCOMPARE(tool->text(),QString("Constrained Orbit"));
        drag(view,middle,middle+QPoint(82,43),Qt::MiddleButton,Qt::ShiftModifier);
        checkLevel(actualCamera(window));

        const QPoint blank(15,15);QCOMPARE(QApplication::widgetAt(view->mapToGlobal(blank)),static_cast<QWidget*>(view));
        auto* popup=blankMenu(view,blank);
        choose(popup,action(popup,"context.orbitCenter.set"));
        QTRY_VERIFY(window.state().value("viewport").toObject().value("orbitCenterPickPending").toBool());
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,blank);
        QVERIFY(window.state().value("viewport").toObject().value("orbitCenterPickPending").toBool());
        const auto projected=call(window,"view.projectBody",{{"bodyId",body.value("bodyId")}});
        const QPoint center(qRound(projected.value("x").toDouble()),qRound(projected.value("y").toDouble()));
        QPoint hitPixel;QJsonObject hit;
        for(const QPoint delta:{QPoint(42,0),QPoint(-42,0),QPoint(0,24),QPoint(0,-24),QPoint(0,0)}) {
            const auto candidate=center+delta;
            if(QApplication::widgetAt(view->mapToGlobal(candidate))!=view)continue;
            const auto picked=view->pickOrbitCenter(candidate.x(),candidate.y());
            if(!picked.isEmpty()){hitPixel=candidate;hit=picked;break;}
        }
        QVERIFY2(!hit.isEmpty(),"no exact visible model-surface pixel for orbit center");
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,hitPixel);
        QTRY_VERIFY(navigation(window).value("customOrbitCenter").toBool());
        const auto custom=navigation(window);
        const auto picked=custom.value("orbitPivot").toArray();
        QCOMPARE(picked,hit.value("point").toArray());
        QVERIFY(separation(picked,QJsonArray{custom.value("centerX"),custom.value("centerY"),custom.value("centerZ")})>1);
        QVERIFY(!window.state().value("viewport").toObject().value("orbitCenterPickPending").toBool());
        call(window,"capture.screenshot",{{"path",out+"/green-orbit-center.png"}});

        // Keep a real pre-existing pan while checking the off-center pivot.
        drag(view,middle,middle+QPoint(21,-13),Qt::MiddleButton,Qt::NoModifier);
        const auto pivotBefore=navigation(window).value("pivotScreen").toObject();
        const auto viewCenterBefore=navigation(window).value("viewCenter").toArray();
        drag(view,middle,middle+QPoint(-90,37),Qt::MiddleButton,Qt::ShiftModifier);
        const auto orbited=navigation(window);
        QVERIFY(pointDistance(pivotBefore,orbited.value("pivotScreen").toObject())<1.5);
        QVERIFY(separation(viewCenterBefore,orbited.value("viewCenter").toArray())>1);
        checkLevel(actualCamera(window));
        call(window,"capture.screenshot",{{"path",out+"/constrained-offcenter-orbit.png"}});
        call(window,"view.orbit",{{"deltaX",0},{"deltaY",10000}});checkLevel(actualCamera(window));

        QTest::mouseClick(mode,Qt::LeftButton);QTRY_VERIFY(mode->menu()->isVisible());
        choose(mode->menu(),action(mode->menu(),"view.orbitMode.free"));
        QCOMPARE(tool->text(),QString("Free Orbit"));
        const auto freeBefore=actualCamera(window);
        drag(view,middle,middle+QPoint(40,-31),Qt::MiddleButton,Qt::ShiftModifier);
        QVERIFY(separation(freeBefore.value("direction").toArray(),actualCamera(window).value("direction").toArray())>0.05);
        const auto restored=navigation(window);
        const auto tab=call(window,"document.tabs").value("activeTabId").toString();
        QVERIFY(!tab.isEmpty());call(window,"document.tabNew");call(window,"document.tabActivate",{{"tabId",tab}});
        QCOMPARE(navigation(window).value("orbitMode"),QJsonValue("free"));
        QCOMPARE(navigation(window).value("orbitPivot"),restored.value("orbitPivot"));
        QCOMPARE(navigation(window).value("viewCenter"),restored.value("viewCenter"));
        call(window,"view.home");
        QVERIFY(navigation(window).value("customOrbitCenter").toBool());
        QCOMPARE(navigation(window).value("orientation"),QJsonValue("isometric"));
        popup=blankMenu(view,blank);choose(popup,action(popup,"context.orbitCenter.reset"));
        QTRY_VERIFY(!navigation(window).value("customOrbitCenter").toBool());
        const QJsonArray fitCenter{navigation(window).value("centerX"),navigation(window).value("centerY"),
            navigation(window).value("centerZ")};
        QCOMPARE(navigation(window).value("orbitPivot"),QJsonValue(fitCenter));
        QCOMPARE(call(window,"document.inspect").value("authored"),initial.value("authored"));
        QCOMPARE(call(window,"document.inspect").value("undo"),initial.value("undo"));
        QCOMPARE(call(window,"document.inspect").value("revision"),initial.value("revision"));
        QCOMPARE(window.state().value("selection"),selection);
        call(window,"document.save",{{"path",out+"/orbit-pivot.alloy"}});window.close();
    }
};
QTEST_MAIN(NavigationOrbitPivotControls)
#include "navigation_orbit_pivot_controls.moc"
