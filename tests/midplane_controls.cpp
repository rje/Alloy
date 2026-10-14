// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QJsonDocument>
#include <QLabel>
#include <QPushButton>
#include <QTest>
#include <QToolButton>
#include <cmath>

class MidplaneControlsTest final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& arguments={}) {
        const auto response=window.execute(command,arguments);
        if(!response["ok"].toBool())qFatal("%s: %s",qPrintable(command),QJsonDocument(response).toJson().constData());
        return response["result"].toObject();
    }
    static QJsonObject ref(const QJsonObject& face){return {{"producerFeatureId",face["producerFeatureId"]},
        {"bodyId",face["bodyId"]},{"faceId",face["faceId"]}};}
    static QJsonObject face(const QJsonObject& inspected,const QString& role){
        for(const auto& value:inspected["evaluation"].toObject()["bodies"].toArray()[0].toObject()["supportFaces"].toArray())
            if(value.toObject()["role"]==role)return value.toObject();
        return {};
    }
    QPoint center(AlloyWindow& window,const QJsonObject& face){
        const auto frame=face["frame"].toObject();
        const auto origin=frame["origin"].toArray();
        const double world[3]={25,40,face["role"]=="box.z1" ? 40.0:0.0};
        const auto u=frame["uAxis"].toArray(),v=frame["vAxis"].toArray();double um=0,vm=0;
        for(int axis=0;axis<3;++axis){um+=(world[axis]-origin[axis].toDouble())*u[axis].toDouble();
            vm+=(world[axis]-origin[axis].toDouble())*v[axis].toDouble();}
        const auto point=call(window,"view.projectFacePoint",{{"support",ref(face)},{"uMm",um},{"vMm",vm}});
        return {qRound(point["x"].toDouble()),qRound(point["y"].toDouble())};
    }
private slots:
    void canvasFacesPreviewCancelCommitEdit(){
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1320,850);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));QApplication::setActiveWindow(&window);
        call(window,"solid.box",{{"width",50},{"depth",80},{"height",40},{"name","Enclosure"}});
        const auto before=call(window,"document.inspect"),top=face(before,"box.z1"),bottom=face(before,"box.z0");
        QVERIFY(!top.isEmpty() && !bottom.isEmpty());
        ReferenceView* view=nullptr;for(auto* child:window.findChildren<QWidget*>())if(auto* found=dynamic_cast<ReferenceView*>(child)){view=found;break;}
        QVERIFY(view);
        auto* launch=window.findChild<QToolButton*>("command.construction.midplane");QVERIFY(launch);
        auto run=[&](bool accept){
            QTest::mouseClick(launch,Qt::LeftButton);QTRY_COMPARE(window.state()["activeSession"].toObject()["command"].toString(),QString("construction.midplane"));
            auto* dock=window.findChild<QDockWidget*>("midplaneDock");QVERIFY(dock && dock->isVisible());
            auto* buttons=dock->findChild<QDialogButtonBox*>("midplane.buttons");QVERIFY(buttons);
            QVERIFY(!buttons->button(QDialogButtonBox::Ok)->isEnabled());
            call(window,"view.home");call(window,"view.orientation",{{"name","top"}});
            QVERIFY(view->hasFocus());QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,center(window,top));
            QCOMPARE(window.state()["activeSession"].toObject()["inputs"].toObject()["faces"].toArray(),QJsonArray{ref(top)});
            QVERIFY(!buttons->button(QDialogButtonBox::Ok)->isEnabled());
            auto* status=dock->findChild<QLabel*>("midplane.status");QVERIFY(status && status->text().contains("1 of 2"));
            call(window,"view.home");call(window,"view.orientation",{{"name","bottom"}});
            const auto bottomPoint=center(window,bottom);
            QVERIFY(view->hasFocus());QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,bottomPoint);
            QTRY_VERIFY(buttons->button(QDialogButtonBox::Ok)->isEnabled());
            QCOMPARE(window.state()["activeSession"].toObject()["inputs"].toObject()["faces"].toArray(),(QJsonArray{ref(top),ref(bottom)}));
            const auto evidence=qEnvironmentVariable("ALLOY_MIDPLANE_EVIDENCE");
            if(!evidence.isEmpty()){call(window,"view.orientation",{{"name","isometric"}});
                call(window,"capture.screenshot",{{"path",evidence+(accept ? "/midplane-native-commit-preview.png":"/midplane-native-cancel-preview.png")}});}
            QTest::mouseClick(buttons->button(accept ? QDialogButtonBox::Ok:QDialogButtonBox::Cancel),Qt::LeftButton);
            QTRY_VERIFY(window.state()["activeSession"].toObject().isEmpty());
        };
        run(false);QCOMPARE(call(window,"document.inspect")["authored"].toObject(),before["authored"].toObject());
        run(true);
        const auto after=call(window,"document.inspect");const auto plane=after["authored"].toObject()["features"].toArray().last().toObject();
        QCOMPARE(plane["parameters"].toObject()["source"].toObject()["faces"].toArray(),(QJsonArray{ref(top),ref(bottom)}));
        QCOMPARE(after["evaluation"].toObject()["constructionPlanes"].toArray().size(),1);
        const auto z=after["evaluation"].toObject()["constructionPlanes"].toArray()[0].toObject()["frame"].toObject()["origin"].toArray()[2].toDouble();
        QVERIFY(std::abs(z-20)<1e-7);
        call(window,"feature.editDialog",{{"featureId",plane["id"]}});
        const auto edit=window.state()["activeSession"].toObject();
        QCOMPARE(edit["command"].toString(),QString("construction.midplane"));
        QCOMPARE(edit["inputs"].toObject()["faces"].toArray(),(QJsonArray{ref(top),ref(bottom)}));
        auto* dock=window.findChild<QDockWidget*>("midplaneDock");QVERIFY(dock);
        QTest::mouseClick(dock->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Cancel),Qt::LeftButton);
        QCOMPARE(call(window,"document.inspect")["authored"].toObject(),after["authored"].toObject());
        call(window,"document.new",{{"discard",true}});window.close();
    }
};
QTEST_MAIN(MidplaneControlsTest)
#include "midplane_controls.moc"
