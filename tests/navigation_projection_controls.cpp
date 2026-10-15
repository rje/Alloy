// SPDX-License-Identifier: MIT
#include "AlloyTheme.h"
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QDir>
#include <QJsonDocument>
#include <QMenu>
#include <QTest>
#include <QToolButton>
#include <cmath>

class NavigationProjectionControls final : public QObject {
    Q_OBJECT
    static QJsonObject call(AlloyWindow& window,const QString& name,const QJsonObject& args={}) {
        const auto response=window.executeApi(name,args);
        if(!response.value("ok").toBool())qFatal("%s",QJsonDocument(response).toJson(QJsonDocument::Compact).constData());
        return response.value("result").toObject();
    }
    static QJsonObject camera(AlloyWindow& window) {
        return window.state().value("viewport").toObject().value("navigation").toObject();
    }
    static QJsonValue actualProjection(AlloyWindow& window) {
        return window.state().value("viewport").toObject().value("gpu").toObject()
            .value("camera").toObject().value("projection");
    }
private slots:
    void policiesAndActualCamera() {
        const auto output=qEnvironmentVariable("ALLOY_NAV_PROJECTION_EVIDENCE");
        QVERIFY(!output.isEmpty());QVERIFY(QDir().mkpath(output));
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1360,850);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto* menuButton=window.findChild<QToolButton*>("view.projection.menuButton");
        auto* menu=window.findChild<QMenu*>("view.projection.menu");
        QVERIFY(menuButton && menu && menuButton->isVisible());
        call(window,"solid.box",{{"width",40},{"depth",27},{"height",19},{"units","mm"}});
        const auto body=call(window,"document.inspect").value("evaluation").toObject().value("bodies").toArray().first().toObject();
        const auto bodyId=body.value("bodyId").toString();QVERIFY(!bodyId.isEmpty());
        QJsonObject top;
        for(const auto& value:body.value("supportFaces").toArray()) {
            const auto candidate=value.toObject();const auto n=candidate.value("frame").toObject().value("normal").toArray();
            if(n.size()==3 && n[2].toDouble()>0.99)top=candidate;
        }
        QVERIFY(!top.isEmpty());
        const QJsonObject support{{"bodyId",top.value("bodyId")},{"faceId",top.value("faceId")},
            {"producerFeatureId",top.value("producerFeatureId")}};
        call(window,"view.home");
        const auto authored=call(window,"document.inspect").value("authored");
        const auto undo=call(window,"document.inspect").value("undo");
        const auto original=camera(window);
        QCOMPARE(original.value("projectionPolicy"),QJsonValue("orthographic"));
        QCOMPARE(original.value("resolvedProjection"),QJsonValue("orthographic"));
        QCOMPARE(actualProjection(window),QJsonValue("orthographic"));
        const auto pointBefore=call(window,"view.projectFacePoint",{{"support",support},{"uMm",8},{"vMm",7}});

        menu->popup(menuButton->mapToGlobal(QPoint(0,menuButton->height())));
        QTRY_VERIFY(menu->isVisible());
        QTRY_COMPARE(QApplication::activePopupWidget(),static_cast<QWidget*>(menu));
        auto* perspective=window.findChild<QAction*>("view.projection.perspective");QVERIFY(perspective);
        const auto row=menu->actionGeometry(perspective).center();
        QCOMPARE(menu->actionAt(row),perspective);
        QCOMPARE(QApplication::widgetAt(menu->mapToGlobal(row)),static_cast<QWidget*>(menu));
        call(window,"capture.screenshot",{{"path",output+"/camera-menu.png"}});
        QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,row);
        QTRY_VERIFY(!menu->isVisible());
        QTRY_VERIFY(QApplication::activePopupWidget()==nullptr);
        QCOMPARE(camera(window).value("projectionPolicy"),QJsonValue("perspective"));
        QCOMPARE(camera(window).value("resolvedProjection"),QJsonValue("perspective"));
        QCOMPARE(actualProjection(window),QJsonValue("perspective"));
        QCOMPARE(camera(window).value("viewCenter"),original.value("viewCenter"));
        const auto pointAfter=call(window,"view.projectFacePoint",{{"support",support},{"uMm",8},{"vMm",7}});
        QVERIFY(std::abs(pointAfter.value("x").toDouble()-pointBefore.value("x").toDouble())<50);
        QVERIFY(std::abs(pointAfter.value("y").toDouble()-pointBefore.value("y").toDouble())<50);
        const auto picked=call(window,"selection.facePick",{{"x",pointAfter.value("x")},{"y",pointAfter.value("y")}});
        QVERIFY(!picked.isEmpty());
        QCOMPARE(window.state().value("selection").toObject().value("support").toObject().value("bodyId"),QJsonValue(bodyId));
        QCOMPARE(window.state().value("selection").toObject().value("support").toObject().value("faceId"),top.value("faceId"));
        call(window,"capture.screenshot",{{"path",output+"/perspective.png"}});
        call(window,"view.projection",{{"mode","hybrid"}});
        QCOMPARE(camera(window).value("resolvedProjection"),QJsonValue("perspective"));
        for(const auto* face:{"top","bottom","front","back","right","left"}) {
            call(window,"view.orientation",{{"name",face}});
            QCOMPARE(camera(window).value("resolvedProjection"),QJsonValue("orthographic"));
            QCOMPARE(actualProjection(window),QJsonValue("orthographic"));
        }
        call(window,"view.orientation",{{"name","isometric"}});
        QCOMPARE(camera(window).value("resolvedProjection"),QJsonValue("perspective"));
        call(window,"view.orientation",{{"name","top"}});
        call(window,"view.orbit",{{"deltaX",1},{"deltaY",0}});
        QCOMPARE(camera(window).value("resolvedProjection"),QJsonValue("orthographic"));
        call(window,"view.orbit",{{"deltaX",3},{"deltaY",0}});
        QCOMPARE(camera(window).value("resolvedProjection"),QJsonValue("perspective"));
        call(window,"capture.screenshot",{{"path",output+"/hybrid-orbit.png"}});
        const auto originalTab=call(window,"document.tabs").value("activeTabId");
        call(window,"document.tabNew");
        QCOMPARE(camera(window).value("projectionPolicy"),QJsonValue("orthographic"));
        call(window,"document.tabActivate",{{"tabId",originalTab}});
        QCOMPARE(camera(window).value("projectionPolicy"),QJsonValue("hybrid"));
        QCOMPARE(camera(window).value("resolvedProjection"),QJsonValue("perspective"));
        call(window,"view.projection",{{"mode","orthographic"}});
        QCOMPARE(camera(window).value("resolvedProjection"),QJsonValue("orthographic"));
        call(window,"view.orientation",{{"name","isometric"}});
        QCOMPARE(camera(window).value("resolvedProjection"),QJsonValue("orthographic"));
        call(window,"capture.screenshot",{{"path",output+"/orthographic.png"}});
        QCOMPARE(call(window,"document.inspect").value("authored"),authored);
        QCOMPARE(call(window,"document.inspect").value("undo"),undo);
        call(window,"document.save",{{"path",output+"/projection.alloy"}});
        window.close();
    }
};
QTEST_MAIN(NavigationProjectionControls)
#include "navigation_projection_controls.moc"
