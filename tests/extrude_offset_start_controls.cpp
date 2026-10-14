// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QJsonDocument>
#include <QLineEdit>
#include <QPushButton>
#include <QTest>

class ExtrudeOffsetStartControlsTest final:public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& args={}) {
        const auto response=window.execute(command,args);
        if(!response["ok"].toBool())qFatal("%s: %s",qPrintable(command),QJsonDocument(response).toJson().constData());
        return response["result"].toObject();
    }
private slots:
    void offsetInputAndEdit() {
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1300,850);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));QApplication::setActiveWindow(&window);
        call(window,"document.parameterAdd",{{"name","StartGap"},{"value",7},{"units","mm"}});
        call(window,"sketch.begin",{{"plane","XY"}});
        call(window,"sketch.rectangle",{{"x1",0},{"y1",0},{"x2",20},{"y2",10},{"units","mm"}});
        const auto sketchId=window.state()["activeSession"].toObject()["draftSketch"].toObject()["id"].toString();
        call(window,"sketch.finish");
        const auto profileId=call(window,"document.inspect")["evaluation"].toObject()["sketches"].toArray()[0].toObject()["profiles"].toArray()[0].toObject()["profileId"].toString();
        auto session=call(window,"solid.extrudeDialog",{{"sketchId",sketchId},{"profileId",profileId}});
        auto* dock=window.findChild<QDockWidget*>("extrudeDock");QVERIFY(dock);
        auto* start=dock->findChild<QComboBox*>("extrude.start");auto* offset=dock->findChild<QDoubleSpinBox*>("extrude.startOffset");auto* distance=dock->findChild<QDoubleSpinBox*>("extrude.distance");
        QVERIFY(start && offset && distance);QVERIFY(!offset->isVisible());
        start->setCurrentIndex(start->findData("offset"));QVERIFY(offset->isVisible());
        QTest::mouseClick(offset,Qt::LeftButton);QTest::keyClick(offset,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(offset,"-10");QTest::keyClick(offset,Qt::Key_Tab);
        QTest::mouseClick(distance,Qt::LeftButton);QTest::keyClick(distance,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(distance,"5");QTest::keyClick(distance,Qt::Key_Tab);
        QCOMPARE(window.state()["activeSession"].toObject()["inputs"].toObject()["startOffset"].toDouble(),-10.);
        QVERIFY(window.state()["activeSession"].toObject()["previewed"].toBool());
        const auto evidence=qEnvironmentVariable("ALLOY_EXTRUDE_OFFSET_EVIDENCE");
        if(!evidence.isEmpty())call(window,"capture.screenshot",{{"path",evidence+"/offset-preview.png"}});
        call(window,"command.cancel",{{"session",session["id"]}});
        QCOMPARE(call(window,"document.inspect")["authored"].toObject()["features"].toArray().size(),1);
        session=call(window,"solid.extrudeDialog",{{"sketchId",sketchId},{"profileId",profileId}});
        dock=window.findChild<QDockWidget*>("extrudeDock");start=dock->findChild<QComboBox*>("extrude.start");offset=dock->findChild<QDoubleSpinBox*>("extrude.startOffset");distance=dock->findChild<QDoubleSpinBox*>("extrude.distance");
        start->setCurrentIndex(start->findData("offset"));
        QTest::mouseClick(offset,Qt::LeftButton);QTest::keyClick(offset,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(offset,"UnknownGap");QTest::keyClick(offset,Qt::Key_Tab);
        QVERIFY(window.state()["activeSession"].toObject().contains("invalidNamedInput"));
        QVERIFY(!dock->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->isEnabled());
        QVERIFY(!window.execute("command.commit",{{"session",session["id"]}})["ok"].toBool());
        QTest::mouseClick(offset,Qt::LeftButton);QTest::keyClick(offset,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(offset,"StartGap");QTest::keyClick(offset,Qt::Key_Tab);
        QCOMPARE(window.state()["activeSession"].toObject()["inputs"].toObject()["startOffsetParameter"].toString(),QString("StartGap"));
        if(!evidence.isEmpty())call(window,"capture.screenshot",{{"path",evidence+"/offset-bound-name.png"}});
        call(window,"command.update",{{"session",session["id"]},{"inputs",QJsonObject{{"distance",5}}}});
        call(window,"command.preview",{{"session",session["id"]}});
        call(window,"command.commit",{{"session",session["id"]}});
        auto authored=call(window,"document.inspect")["authored"].toObject();const auto feature=authored["features"].toArray().last().toObject();
        QVERIFY(feature["parameters"].toObject().contains("startOffsetParameterId"));
        session=call(window,"feature.editDialog",{{"featureId",feature["id"]}});
        dock=window.findChild<QDockWidget*>("extrudeDock");offset=dock->findChild<QDoubleSpinBox*>("extrude.startOffset");QVERIFY(offset);
        QVERIFY(offset->findChild<QLineEdit*>()->text().contains("StartGap"));
        QTest::mouseClick(offset,Qt::LeftButton);QTest::keyClick(offset,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(offset,"7");QTest::keyClick(offset,Qt::Key_Tab);
        QVERIFY(!window.state()["activeSession"].toObject()["inputs"].toObject().contains("startOffsetParameter"));
        call(window,"command.commit",{{"session",session["id"]}});
        authored=call(window,"document.inspect")["authored"].toObject();
        QVERIFY(!authored["features"].toArray().last().toObject()["parameters"].toObject().contains("startOffsetParameterId"));
        session=call(window,"feature.editDialog",{{"featureId",feature["id"]}});
        offset=window.findChild<QDockWidget*>("extrudeDock")->findChild<QDoubleSpinBox*>("extrude.startOffset");QVERIFY(offset);
        QCOMPARE(offset->value(),7.);
        call(window,"command.cancel",{{"session",session["id"]}});
        call(window,"document.new",{{"discard",true}});window.close();
    }
};
QTEST_MAIN(ExtrudeOffsetStartControlsTest)
#include "extrude_offset_start_controls.moc"
