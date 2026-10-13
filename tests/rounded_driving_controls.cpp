// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include <QApplication>
#include <QJsonDocument>
#include <QLineEdit>
#include <QTest>
#include <cmath>

class RoundedDrivingControlsTest final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& w,const QString& name,const QJsonObject& args={}) {
        const auto r=w.execute(name,args);if(!r["ok"].toBool())qFatal("%s",QJsonDocument(r).toJson().constData());return r["result"].toObject();
    }
    QJsonObject draft(AlloyWindow& w){return w.state()["activeSession"].toObject()["draftSketch"].toObject();}
    double value(AlloyWindow& w,const QString& id){for(const auto& v:draft(w)["parameters"].toObject()["dimensions"].toArray())if(v.toObject()["id"]==id)return v.toObject()["valueMm"].toDouble();return -1;}
private slots:
    void editCancelAndAccept(){
        AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1400,900);w.show();w.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&w));QApplication::setActiveWindow(&w);
        call(w,"sketch.twoPointRectangle");call(w,"sketch.begin",{{"plane","XY"},{"units","mm"}});call(w,"sketch.lookAt");
        const auto id=draft(w)["id"].toString();
        auto pixel=[&](double u,double v){const auto p=call(w,"view.projectSketchPoint",{{"sketchId",id},{"uMm",u},{"vMm",v}});return QJsonObject{{"x",p["x"]},{"y",p["y"]}};};
        call(w,"sketch.placePoint",pixel(0,0));call(w,"sketch.pointerMove",pixel(20,-10));
        call(w,"sketch.twoPointRectanglePreview",{{"widthText","60"},{"heightText","30"}});call(w,"sketch.twoPointRectangleAccept");
        call(w,"view.zoom",{{"factor",.7}});
        const auto center=pixel(30,-15),viewport=w.state()["viewport"].toObject();
        call(w,"view.pan",{{"x",viewport["width"].toDouble()/2-center["x"].toDouble()},
                            {"y",viewport["height"].toDouble()/2-center["y"].toDouble()}});
        const auto source=draft(w)["parameters"].toObject();const auto es=source["entities"].toArray(),ds=source["dimensions"].toArray();
        QStringList lines;for(const auto& e:es)lines<<e.toObject()["id"].toString();
        const auto width=ds[0].toObject()["id"].toString();
        for(int i=0;i<4;++i)call(w,"sketch.fillet",{{"corners",QJsonArray{QJsonObject{{"entityIds",QJsonArray{lines[i],lines[(i+1)%4]}}}}},{"value",6},{"units","mm"}});
        QString radius;for(const auto& v:draft(w)["parameters"].toObject()["dimensions"].toArray())if(v.toObject()["kind"]=="radius")radius=v.toObject()["id"].toString();
        QVERIFY(!radius.isEmpty());
        call(w,"sketch.dimensionEdit",{{"dimensionId",width}});
        auto* input=w.findChild<QLineEdit*>("sketch.inlineValue");QVERIFY(input && input->isVisible() && !input->isReadOnly());
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(input));
        QTest::keyClick(input,Qt::Key_Escape);QCOMPARE(value(w,width),60.0);
        call(w,"sketch.dimensionEdit",{{"dimensionId",radius}});QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(input));
        QTest::keyClicks(input,"5.5 mm");QTest::keyClick(input,Qt::Key_Return);QCOMPARE(value(w,radius),5.5);
        call(w,"sketch.dimensionEdit",{{"dimensionId",width}});QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(input));
        QTest::keyClicks(input,"70 mm");QTest::keyClick(input,Qt::Key_Return);QCOMPARE(value(w,width),70.0);
        const auto evidence=qEnvironmentVariable("ALLOY_ROUNDED_DRIVING_EVIDENCE");
        if(!evidence.isEmpty())call(w,"capture.screenshot",{{"path",evidence}});
    }
};
QTEST_MAIN(RoundedDrivingControlsTest)
#include "rounded_driving_controls.moc"
