// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QJsonDocument>
#include <QLabel>
#include <QAbstractItemView>
#include <QPushButton>
#include <QTest>
#include <QToolButton>
#include <cmath>

class PathEndpointPlaneControlsTest final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& args={}){
        const auto response=window.execute(command,args);
        if(!response["ok"].toBool())qFatal("%s: %s",qPrintable(command),QJsonDocument(response).toJson().constData());
        return response["result"].toObject();
    }
private slots:
    void canvasSplineAndEndSelection(){
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1400,880);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));window.activateWindow();
        call(window,"sketch.begin",{{"plane","YZ"},{"name","Shoehorn spine"}});
        call(window,"sketch.splineAdd",{{"points",QJsonArray{QJsonObject{{"uMm",0},{"vMm",0}},
            QJsonObject{{"uMm",50},{"vMm",16}},QJsonObject{{"uMm",130},{"vMm",0}}}}});
        const auto draft=window.state()["activeSession"].toObject()["draftSketch"].toObject();
        const auto sketchId=draft["id"].toString();
        const auto entityId=draft["parameters"].toObject()["entities"].toArray()[0].toObject()["id"].toString();
        call(window,"sketch.finish");
        ReferenceView* view=nullptr;for(auto* child:window.findChildren<QWidget*>())
            if(auto* found=dynamic_cast<ReferenceView*>(child)){view=found;break;}
        QVERIFY(view);
        auto* launch=window.findChild<QToolButton*>("command.construction.pathPlane");QVERIFY(launch);
        auto run=[&](bool accept){
            QTest::mouseClick(launch,Qt::LeftButton);
            QTRY_COMPARE(window.state()["activeSession"].toObject()["command"].toString(),QString("construction.pathPlane"));
            auto* dock=window.findChild<QDockWidget*>("pathPlaneDock");QVERIFY(dock && dock->isVisible());
            auto* buttons=dock->findChild<QDialogButtonBox*>("pathPlane.buttons");QVERIFY(buttons);
            auto* endpoint=dock->findChild<QComboBox*>("pathPlane.endpoint");QVERIFY(endpoint);
            QVERIFY(!buttons->button(QDialogButtonBox::Ok)->isEnabled());
            call(window,"view.home");call(window,"view.orientation",{{"name","right"}});
            const auto point=call(window,"view.projectSketchPoint",{{"plane","YZ"},{"uMm",50},{"vMm",16}});
            QVERIFY(view->hasFocus());
            QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,
                QPoint(qRound(point["x"].toDouble()),qRound(point["y"].toDouble())));
            const auto selected=window.state()["activeSession"].toObject()["inputs"].toObject()["path"].toObject();
            QCOMPARE(selected["sketchId"].toString(),sketchId);
            QCOMPARE(selected["entityId"].toString(),entityId);
            QTRY_VERIFY(buttons->button(QDialogButtonBox::Ok)->isEnabled());
            QTest::mouseClick(endpoint,Qt::LeftButton);
            QVERIFY(endpoint->view()->isVisible());
            auto* popupFocus=QApplication::focusWidget();
            QVERIFY(popupFocus && (popupFocus==endpoint->view() || endpoint->view()->isAncestorOf(popupFocus)));
            QTest::keyClick(popupFocus,Qt::Key_End);
            QTest::keyClick(popupFocus,Qt::Key_Return);
            QTRY_VERIFY(QApplication::activePopupWidget()==nullptr);
            QTRY_COMPARE(window.state()["activeSession"].toObject()["inputs"].toObject()["path"].toObject()["endpoint"].toInt(),1);
            const auto evidence=qEnvironmentVariable("ALLOY_PATH_PLANE_EVIDENCE");
            if(!evidence.isEmpty()){call(window,"view.orientation",{{"name","isometric"}});
                call(window,"capture.screenshot",{{"path",evidence+(accept?"/path-end-accepted.png":"/path-end-cancel.png")}});}
            QTest::mouseClick(buttons->button(accept?QDialogButtonBox::Ok:QDialogButtonBox::Cancel),Qt::LeftButton);
            QTRY_VERIFY(window.state()["activeSession"].toObject().isEmpty());
        };
        const auto authored=call(window,"document.inspect")["authored"].toObject();
        run(false);QCOMPARE(call(window,"document.inspect")["authored"].toObject(),authored);
        run(true);
        const auto inspected=call(window,"document.inspect");
        const auto planes=inspected["evaluation"].toObject()["constructionPlanes"].toArray();
        QCOMPARE(planes.size(),1);
        const auto origin=planes[0].toObject()["frame"].toObject()["origin"].toArray();
        QVERIFY(std::abs(origin[1].toDouble()-130)<1e-6);
    }
};
QTEST_MAIN(PathEndpointPlaneControlsTest)
#include "path_endpoint_plane_controls.moc"
