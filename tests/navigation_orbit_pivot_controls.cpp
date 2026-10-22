// SPDX-License-Identifier: MIT
#include "AlloyTheme.h"
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QMenu>
#include <QMouseEvent>
#include <QPushButton>
#include <QScopeGuard>
#include <QTabBar>
#include <QTimer>
#include <QTest>
#include <QToolButton>
#include <QWindow>
#include <cmath>
#ifdef Q_OS_MACOS
#include <CoreGraphics/CGEvent.h>
#include <dlfcn.h>
#endif

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
        QCOMPARE(QApplication::widgetAt(view->mapToGlobal(start)),view);
        QCOMPARE(QApplication::widgetAt(view->mapToGlobal(end)),view);
        pointer(view,QEvent::MouseButtonPress,start,button,button,mods);
        pointer(view,QEvent::MouseMove,end,Qt::NoButton,button,mods);
        pointer(view,QEvent::MouseButtonRelease,end,button,Qt::NoButton,mods);
    }
    // Observation only: no AppKit/Qt focus repair after initial activation.
    static QJsonObject readiness(AlloyWindow& window) {
        QJsonObject r{{"qtActive",window.isActiveWindow()},
            {"applicationActive",QGuiApplication::applicationState()==Qt::ApplicationActive},
            {"focusWindowEditor",QGuiApplication::focusWindow()==window.windowHandle()},
            {"processId",QCoreApplication::applicationPid()}};
#ifdef Q_OS_MACOS
        const auto selector=reinterpret_cast<void*(*)(const char*)>(dlsym(RTLD_DEFAULT,"sel_registerName"));
        const auto send=reinterpret_cast<void*(*)(void*,void*)>(dlsym(RTLD_DEFAULT,"objc_msgSend"));
        const auto boolean=reinterpret_cast<bool(*)(void*,void*)>(dlsym(RTLD_DEFAULT,"objc_msgSend"));
        const auto cls=reinterpret_cast<void*(*)(const char*)>(dlsym(RTLD_DEFAULT,"objc_getClass"));
        const auto app=send(cls("NSApplication"),selector("sharedApplication"));
        const auto native=send(reinterpret_cast<void*>(window.winId()),selector("window"));
        r.insert("appKitActive",boolean(app,selector("isActive")));
        r.insert("appKitKeyWindowEditor",send(app,selector("keyWindow"))==native);
        if(auto* popup=QApplication::activePopupWidget())
            r.insert("appKitKeyWindowPopup",send(app,selector("keyWindow"))==send(reinterpret_cast<void*>(popup->winId()),selector("window")));
#endif
        return r;
    }
    static bool ready(AlloyWindow& w) {
        const auto r=readiness(w);
        for(const auto& key:{"qtActive","applicationActive","focusWindowEditor","appKitActive","appKitKeyWindowEditor"})
            if(!r.value(key).toBool(true))return false;
        return true;
    }
    static void click(QWidget* widget,const QPoint& pixel,Qt::MouseButton button=Qt::LeftButton) {
        QVERIFY(widget && widget->isVisible());
        auto* hit=QApplication::widgetAt(widget->mapToGlobal(pixel));
        QVERIFY(hit && (hit==widget || widget->isAncestorOf(hit)));
        QTest::mouseClick(hit,button,Qt::NoModifier,hit->mapFromGlobal(widget->mapToGlobal(pixel)));
    }
    static QAction* action(QMenu* menu,const QString& name) {
        for(auto* row:menu->actions())if(row->objectName()==name)return row;
        return nullptr;
    }
    static void choose(AlloyWindow& window,QMenu* menu,const QString& name) {
        QVERIFY(menu);QTRY_COMPARE(QApplication::activePopupWidget(),static_cast<QWidget*>(menu));
        const auto native=readiness(window);
        QVERIFY(native.value("appKitActive").toBool(true));
        QVERIFY(native.value("appKitKeyWindowEditor").toBool(true) || native.value("appKitKeyWindowPopup").toBool());
        auto* row=action(menu,name);QVERIFY(row && row->isEnabled());
        click(menu,menu->actionGeometry(row).center());
        QTRY_VERIFY(!QApplication::activePopupWidget());
    }
    static bool nativeContextClick(AlloyWindow& window,ReferenceView* view,const QPoint& pixel) {
        if(!ready(window) || QApplication::widgetAt(view->mapToGlobal(pixel))!=view)return false;
#ifdef Q_OS_MACOS
        // PID-directed OS events exercise Cocoa's context-menu generation. Never
        // post globally or target another application. Qt-only input is separate.
        const auto create=reinterpret_cast<CGEventRef(*)(CGEventSourceRef,CGEventType,CGPoint,CGMouseButton)>(dlsym(RTLD_DEFAULT,"CGEventCreateMouseEvent"));
        const auto post=reinterpret_cast<void(*)(pid_t,CGEventRef)>(dlsym(RTLD_DEFAULT,"CGEventPostToPid"));
        if(!create || !post)return false;
        const auto p=view->mapToGlobal(pixel);
        // QT_SCALE_FACTOR changes Qt global coordinates, not Cocoa screen points.
        bool ok=false;const auto configured=qEnvironmentVariable("QT_SCALE_FACTOR").toDouble(&ok);
        const double factor=ok ? configured : 1.;
        for(const auto type:{kCGEventRightMouseDown,kCGEventRightMouseUp}) {
            auto event=create(nullptr,type,CGPointMake(p.x()*factor,p.y()*factor),kCGMouseButtonRight);
            if(!event)return false;
            post(static_cast<pid_t>(QCoreApplication::applicationPid()),event);CFRelease(event);QTest::qWait(60);
        }
        return true;
#else
        Q_UNUSED(view);Q_UNUSED(pixel);return false;
#endif
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
    void captureSingleModeIndicator() {
        const auto out=qEnvironmentVariable("ALLOY_NAV_PIVOT_EVIDENCE");
        const auto source=qEnvironmentVariable("ALLOY_NAV_PIVOT_SOURCE");
        QVERIFY(!out.isEmpty() && !source.isEmpty());QVERIFY(QDir().mkpath(out));
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1300,820);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        call(window,"document.open",{{"path",source}});
        call(window,"view.orbitMode",{{"mode","constrained"}});
        call(window,"capture.screenshot",{{"path",out+"/single-mode-indicator.png"}});
        window.close();
    }
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
        QJsonArray steps;QString phase="initial activation";
        const auto receipt=qScopeGuard([&]{
            QFile file(out+"/native-report.json");
            if(file.open(QIODevice::WriteOnly))file.write(QJsonDocument(QJsonObject{
                {"passed",!QTest::currentTestFailed()},{"phase",phase},{"readiness",readiness(window)},
                {"steps",steps},{"state",window.state()}}).toJson());
            window.close();
        });
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        ReferenceView* view=nullptr;for(auto* child:window.findChildren<QWidget*>())
            if(auto* found=dynamic_cast<ReferenceView*>(child))view=found;
        QVERIFY(view);
        // One initial harness activation only. No setActiveWindow bookkeeping.
        window.raise();window.activateWindow();window.windowHandle()->requestActivate();
        QTRY_VERIFY_WITH_TIMEOUT(ready(window),3000);
        auto record=[&](const QString& name) {
            phase=name;auto* focus=QApplication::focusWidget();
            steps.append(QJsonObject{{"step",name},{"readiness",readiness(window)},
                {"focusClass",focus?focus->metaObject()->className():"none"},
                {"focusObject",focus?focus->objectName():"none"},{"state",window.state()}});
            qInfo().noquote()<<name;
        };
        call(window,"solid.box",{{"width",40},{"depth",20},{"height",12},{"x",20},{"y",10},{"z",0},{"units","mm"}});
        const auto body=call(window,"document.inspect").value("evaluation").toObject().value("bodies").toArray().first().toObject();
        call(window,"selection.set",{{"bodyId",body.value("bodyId")}});call(window,"view.home");
        call(window,"document.save",{{"path",out+"/orbit-pivot.alloy"}});
        const auto initial=call(window,"document.inspect");const auto selection=window.state().value("selection");
        const QPoint middle(view->width()/2,view->height()/2),blank(15,15);
        auto* mode=window.findChild<QToolButton*>("view.orbitMode");auto* tool=window.findChild<QPushButton*>("view.orbitTool");
        QVERIFY(mode && mode->menu() && tool);
        auto chooseMode=[&](const QString& name) {
            QVERIFY(ready(window));phase="mode popup "+name;
            // QToolButton executes a nested menu loop; observe it after dispatch.
            QTimer chooser;chooser.setSingleShot(true);
            connect(&chooser,&QTimer::timeout,mode,[&,name]{record("mode popup "+name);choose(window,mode->menu(),"view.orbitMode."+name);});
            bool timedOut=false;QTimer watchdog;watchdog.setSingleShot(true);
            connect(&watchdog,&QTimer::timeout,mode,[&]{timedOut=true;mode->menu()->close();});
            chooser.start(80);watchdog.start(3000);click(mode,mode->rect().center());
            chooser.stop();watchdog.stop();QVERIFY(!timedOut);
            QTRY_VERIFY(!QApplication::activePopupWidget());QTRY_VERIFY(ready(window));
            QCOMPARE(navigation(window).value("orbitMode"),QJsonValue(name));record("mode "+name);
        };
        auto context=[&](const QString& name) {
            phase="OS context "+name;QVERIFY(ready(window));
            QVERIFY2(nativeContextClick(window,view,blank),"PID-directed native context click unavailable");
            QMenu* menu=nullptr;QTRY_VERIFY_WITH_TIMEOUT((menu=qobject_cast<QMenu*>(QApplication::activePopupWidget())),2000);
            QCOMPARE(menu->objectName(),QString("context.orbitCenter"));
            QVERIFY(readiness(window).value("appKitActive").toBool(true));
            record("context popup "+name);choose(window,menu,"context.orbitCenter."+name);
            QTRY_VERIFY(ready(window));record("context closed "+name);
        };
        auto escape=[&] {
            QVERIFY(ready(window));auto* focus=QApplication::focusWidget();QVERIFY(focus);
            // Deliver to natural focus, never repair it to the view.
            QTest::keyClick(focus,Qt::Key_Escape);QTRY_VERIFY(ready(window));
        };
        auto orbit=[&](const QPoint& delta) {
            QVERIFY(ready(window));phase="visible Orbit tool click";click(tool,tool->rect().center());
            QTRY_VERIFY(tool->isChecked());QVERIFY(ready(window));
            QCOMPARE(QApplication::widgetAt(view->mapToGlobal(middle)),static_cast<QWidget*>(view));
            const auto before=actualCamera(window);drag(view,middle,middle+delta,Qt::LeftButton,Qt::NoModifier);
            QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QVERIFY(ready(window));
            QVERIFY(separation(before.value("direction").toArray(),actualCamera(window).value("direction").toArray())>0.05);
            escape();QVERIFY(!tool->isChecked());record("left drag orbit and Escape");
        };
        record("setup");chooseMode("free");QVERIFY(!QTest::currentTestFailed());orbit({75,30});QVERIFY(!QTest::currentTestFailed());
        chooseMode("constrained");QVERIFY(!QTest::currentTestFailed());orbit({82,43});QVERIFY(!QTest::currentTestFailed());checkLevel(actualCamera(window));
        context("set");QVERIFY(!QTest::currentTestFailed());QVERIFY(window.state().value("viewport").toObject().value("orbitCenterPickPending").toBool());
        click(view,blank);QVERIFY(window.state().value("viewport").toObject().value("orbitCenterPickPending").toBool());
        QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
        const auto beforeCancel=navigation(window);escape();
        QVERIFY(!window.state().value("viewport").toObject().value("orbitCenterPickPending").toBool());
        QCOMPARE(navigation(window),beforeCancel);record("pending pick Escape");
        context("set");QVERIFY(!QTest::currentTestFailed());
        const auto projected=call(window,"view.projectBody",{{"bodyId",body.value("bodyId")}});
        const QPoint center(qRound(projected.value("x").toDouble()),qRound(projected.value("y").toDouble()));
        QPoint hitPixel;QJsonObject hit;
        for(const QPoint delta:{QPoint(42,0),QPoint(-42,0),QPoint(0,24),QPoint(0,-24),QPoint(0,0)}) {
            const auto p=center+delta;if(QApplication::widgetAt(view->mapToGlobal(p))!=view)continue;
            hit=view->pickOrbitCenter(p.x(),p.y());if(!hit.isEmpty()){hitPixel=p;break;}
        }
        QVERIFY2(!hit.isEmpty(),"no exact visible model-surface pixel for orbit center");click(view,hitPixel);
        QTRY_VERIFY(navigation(window).value("customOrbitCenter").toBool());
        const auto custom=navigation(window);const auto picked=custom.value("orbitPivot").toArray();
        QCOMPARE(picked,hit.value("point").toArray());
        QVERIFY(separation(picked,QJsonArray{custom.value("centerX"),custom.value("centerY"),custom.value("centerZ")})>1);
        QVERIFY(!window.state().value("viewport").toObject().value("orbitCenterPickPending").toBool());
        QVERIFY(ready(window));record("picked visible surface");
        call(window,"capture.screenshot",{{"path",out+"/green-orbit-center.png"}});
        drag(view,middle,middle+QPoint(21,-13),Qt::MiddleButton,Qt::NoModifier);
        QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QVERIFY(ready(window));
        const auto pivotBefore=navigation(window).value("pivotScreen").toObject();
        const auto viewCenterBefore=navigation(window).value("viewCenter").toArray();
        orbit({-90,37});QVERIFY(!QTest::currentTestFailed());const auto orbited=navigation(window);
        QVERIFY(pointDistance(pivotBefore,orbited.value("pivotScreen").toObject())<1.5);
        QVERIFY(separation(viewCenterBefore,orbited.value("viewCenter").toArray())>1);checkLevel(actualCamera(window));
        call(window,"capture.screenshot",{{"path",out+"/constrained-offcenter-orbit.png"}});
        chooseMode("free");QVERIFY(!QTest::currentTestFailed());orbit({40,-31});QVERIFY(!QTest::currentTestFailed());const auto restored=navigation(window);
        // Create the second tab as fixture setup, then return through its actual tab.
        const auto tab=call(window,"document.tabs").value("activeTabId").toString();QVERIFY(!tab.isEmpty());
        call(window,"document.tabNew");auto* tabs=window.findChild<QTabBar*>("document.tabs");QVERIFY(tabs && tabs->count()==2);
        click(tabs,tabs->tabRect(0).center());QTRY_VERIFY(ready(window));
        QCOMPARE(call(window,"document.tabs").value("activeTabId"),QJsonValue(tab));
        QCOMPARE(navigation(window).value("orbitMode"),QJsonValue("free"));
        QCOMPARE(navigation(window).value("orbitPivot"),restored.value("orbitPivot"));
        QCOMPARE(navigation(window).value("viewCenter"),restored.value("viewCenter"));record("actual tab return");
        context("reset");QVERIFY(!QTest::currentTestFailed());QVERIFY(!navigation(window).value("customOrbitCenter").toBool());
        const auto reset=navigation(window);
        QCOMPARE(reset.value("orbitPivot"),QJsonValue(QJsonArray{reset.value("centerX"),reset.value("centerY"),reset.value("centerZ")}));
        QCOMPARE(call(window,"document.inspect"),initial);QCOMPARE(window.state().value("selection"),selection);
        record("unchanged authored history selection");call(window,"capture.screenshot",{{"path",out+"/reset-orbit-center.png"}});
        phase="complete";
    }
};
QTEST_MAIN(NavigationOrbitPivotControls)
#include "navigation_orbit_pivot_controls.moc"
