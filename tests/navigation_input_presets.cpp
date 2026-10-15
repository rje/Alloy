// SPDX-License-Identifier: MIT
#include "AlloyTheme.h"
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QDir>
#include <QJsonDocument>
#include <QMouseEvent>
#include <QMenu>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>
#include <QWheelEvent>
#include <cmath>

class NavigationInputPresets final : public QObject {
    Q_OBJECT
    static QJsonObject call(AlloyWindow& w,const QString& name,const QJsonObject& args={}) {
        const auto response=w.executeApi(name,args);
        if(!response.value("ok").toBool())qFatal("%s",QJsonDocument(response).toJson(QJsonDocument::Compact).constData());
        return response.value("result").toObject();
    }
    static QJsonObject camera(AlloyWindow& w) {
        return w.state().value("viewport").toObject().value("navigation").toObject();
    }
    static void drag(ReferenceView* view,Qt::MouseButton button,const QPoint& start,const QPoint& end) {
        const auto modifiers=Qt::ShiftModifier|Qt::AltModifier;
        QTest::mousePress(view,button,modifiers,start);
        QVERIFY(view->hasFocus());
        QMouseEvent motion(QEvent::MouseMove,QPointF(end),QPointF(view->mapToGlobal(end)),
            Qt::NoButton,button,modifiers);
        QApplication::sendEvent(view,&motion);
        QTest::mouseRelease(view,button,modifiers,end);
    }
private slots:
    void alternateChordsAndRestart() {
        const auto out=qEnvironmentVariable("ALLOY_NAV_INPUT_EVIDENCE");
        QVERIFY(!out.isEmpty());QVERIFY(QDir().mkpath(out));
        QTemporaryDir settings;QVERIFY(settings.isValid());
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,settings.path());
        qApp->setOrganizationName("AlloyNavigationInputTest");qApp->setApplicationName("AlloyNavigationInputTest");
        AlloyTheme::apply(*qApp);
        {
            AlloyWindow window;window.resize(1360,850);window.show();window.activateWindow();
            QVERIFY(QTest::qWaitForWindowExposed(&window));
            QTRY_VERIFY(window.isActiveWindow());
            ReferenceView* view=nullptr;
            for(auto* child:window.findChildren<QWidget*>())if(auto* found=dynamic_cast<ReferenceView*>(child))view=found;
            QVERIFY(view);
            call(window,"solid.box",{{"width",44},{"depth",27},{"height",13},{"units","mm"}});
            call(window,"view.home");
            const auto original=call(window,"document.inspect");
            QCOMPARE(call(window,"view.inputPresetInspect").value("preset"),QJsonValue("default"));
            call(window,"view.inputPreset",{{"preset","shiftAlt"}});
            QCOMPARE(window.state().value("navigationPreferences").toObject().value("preset"),QJsonValue("shiftAlt"));
            const QPoint p(view->width()/2,view->height()/2);
            const auto first=camera(window);
            drag(view,Qt::MiddleButton,p,p+QPoint(46,-17));
            QVERIFY(std::abs(camera(window).value("panX").toDouble()-first.value("panX").toDouble()-46)<1e-6);
            const auto afterPan=camera(window);
            drag(view,Qt::LeftButton,p,p+QPoint(38,21));
            QVERIFY(camera(window).value("customDirection")!=afterPan.value("customDirection"));
            const auto afterOrbit=camera(window);
            drag(view,Qt::RightButton,p,p+QPoint(0,-30));
            QVERIFY(camera(window).value("zoom").toDouble()>afterOrbit.value("zoom").toDouble());
            QCOMPARE(call(window,"document.inspect").value("authored"),original.value("authored"));
            QCOMPARE(call(window,"document.inspect").value("undo"),original.value("undo"));
            call(window,"capture.screenshot",{{"path",out+"/alternate-navigation.png"}});
            QMenu* menu=nullptr;
            for(auto* child:window.findChildren<QMenu*>())if(child->title()=="Navigation Controls")menu=child;
            QVERIFY(menu);
            menu->popup(window.mapToGlobal(QPoint(145,55)));
            QTRY_COMPARE(QApplication::activePopupWidget(),static_cast<QWidget*>(menu));
            QAction* alternate=nullptr;
            for(auto* action:menu->actions())if(action->objectName()=="view.inputPreset.shiftAlt")alternate=action;
            QVERIFY(alternate && alternate->isChecked());
            call(window,"capture.screenshot",{{"path",out+"/navigation-controls-menu.png"}});
            const auto row=menu->actionGeometry(alternate).center();
            QCOMPARE(QApplication::widgetAt(menu->mapToGlobal(row)),static_cast<QWidget*>(menu));
            QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,row);
            QTRY_VERIFY(QApplication::activePopupWidget()==nullptr);
            QCOMPARE(call(window,"view.inputPresetInspect").value("preset"),QJsonValue("shiftAlt"));
            call(window,"document.save",{{"path",out+"/navigation-input.alloy"}});
            window.close();
        }
        {
            AlloyWindow reopened;reopened.show();QVERIFY(QTest::qWaitForWindowExposed(&reopened));
            QCOMPARE(call(reopened,"view.inputPresetInspect").value("preset"),QJsonValue("shiftAlt"));
            const auto before=call(reopened,"document.inspect");
            call(reopened,"view.inputPresetReset");
            QCOMPARE(call(reopened,"view.inputPresetInspect").value("preset"),QJsonValue("default"));
            QCOMPARE(call(reopened,"document.inspect").value("authored"),before.value("authored"));
            QCOMPARE(call(reopened,"document.inspect").value("undo"),before.value("undo"));
            reopened.close();
        }
        {
            AlloyWindow reset;QCOMPARE(call(reset,"view.inputPresetInspect").value("preset"),QJsonValue("default"));
        }
    }
};
QTEST_MAIN(NavigationInputPresets)
#include "navigation_input_presets.moc"
