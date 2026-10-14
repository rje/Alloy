#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QJsonDocument>
#include <QLineEdit>
#include <QPushButton>
#include <QTest>
class ToObjectControls final:public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& w,const QString& command,const QJsonObject& args={}) {
        const auto response=w.execute(command,args);if(!response["ok"].toBool())qFatal("%s",QJsonDocument(response).toJson().constData());return response["result"].toObject();
    }
    QJsonObject ref(const QJsonObject& f){return {{"producerFeatureId",f["producerFeatureId"]},{"bodyId",f["bodyId"]},{"faceId",f["faceId"]}};}
private slots:
    void selectAndClearance(){
        AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1360,860);w.show();w.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&w));
        ReferenceView* view=nullptr;for(auto* widget:w.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(widget))view=v;QVERIFY(view);
        call(w,"solid.box",{{"width",50},{"depth",80},{"height",40},{"units","mm"}});
        auto body=call(w,"document.inspect")["evaluation"].toObject()["bodies"].toArray().first().toObject();QJsonObject top,back;
        for(const auto& entry:body["supportFaces"].toArray())if(entry.toObject()["role"]=="box.z1")top=entry.toObject();
        call(w,"solid.shell",{{"targetBodyId",body["bodyId"]},{"removedFace",ref(top)},{"thicknessMm",4}});
        body=call(w,"document.inspect")["evaluation"].toObject()["bodies"].toArray().first().toObject();
        for(const auto& entry:body["supportFaces"].toArray()) {const auto f=entry.toObject();if(f["role"]=="shell-inner" && std::abs(f["frame"].toObject()["origin"].toArray()[1].toDouble()-76)<1e-5)back=f;}
        QVERIFY(!back.isEmpty());call(w,"document.parameterAdd",{{"name","Clearance"},{"value",0.25},{"units","mm"}});
        auto sketch=call(w,"sketch.begin",{{"plane","XZ"}});const QJsonValue sketchId=sketch["draftSketch"].toObject()["id"];
        call(w,"sketch.rectangle",{{"x1",5},{"y1",38},{"x2",45},{"y2",42},{"units","mm"}});call(w,"sketch.finish");
        const auto baseline=call(w,"document.inspect");const QJsonValue profile=baseline["evaluation"].toObject()["sketches"].toArray().last().toObject()["profiles"].toArray().first().toObject()["profileId"];
        auto session=call(w,"solid.extrudeDialog",{{"sketchId",sketchId},{"profileId",profile}});
        auto* dock=w.findChild<QDockWidget*>("extrudeDock");QVERIFY(dock);auto* extent=dock->findChild<QComboBox*>("extrude.extent");QVERIFY(extent);
        extent->setFocus();QTest::keyClick(extent,Qt::Key_End);QCOMPARE(extent->currentData().toString(),QString("toObject"));
        QVERIFY(!dock->findChild<QDoubleSpinBox*>("extrude.distance")->isVisible());
        call(w,"view.home");QApplication::processEvents();
        const auto frame=back["frame"].toObject();const auto origin=frame["origin"].toArray(),u=frame["uAxis"].toArray(),v=frame["vAxis"].toArray();double localU=0,localV=0;const double point[]{25,76,30};
        for(int i=0;i<3;++i){localU+=(point[i]-origin[i].toDouble())*u[i].toDouble();localV+=(point[i]-origin[i].toDouble())*v[i].toDouble();}
        const auto pixel=call(w,"view.projectFacePoint",{{"support",ref(back)},{"uMm",localU},{"vMm",localV}});
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,QPoint(qRound(pixel["x"].toDouble()),qRound(pixel["y"].toDouble())));
        QCOMPARE(w.state()["activeSession"].toObject()["inputs"].toObject()["endSupport"].toObject(),ref(back));
        auto* offset=dock->findChild<QLineEdit*>("extrude.endOffset");QVERIFY(offset);
        QTest::mouseClick(offset,Qt::LeftButton);QTest::keyClick(offset,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(offset,"Unknown");QTest::keyClick(offset,Qt::Key_Tab);
        QVERIFY2(!dock->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->isEnabled(),qPrintable("Unaccepted offset text: "+offset->text()));QVERIFY(!w.execute("command.commit",{{"session",session["id"]}})["ok"].toBool());
        QTest::mouseClick(offset,Qt::LeftButton);QTest::keyClick(offset,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(offset,"Clearance");QTest::keyClick(offset,Qt::Key_Tab);
        QVERIFY(w.state()["activeSession"].toObject()["previewed"].toBool());QCOMPARE(w.state()["activeSession"].toObject()["resolvedDistanceMm"].toDouble(),-75.75);
        QCOMPARE(call(w,"document.inspect"),baseline);
        const auto evidence=qEnvironmentVariable("ALLOY_TO_OBJECT_EVIDENCE");if(!evidence.isEmpty())call(w,"capture.screenshot",{{"path",evidence+"/native-to-object.png"}});
        QTest::mouseClick(dock->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok),Qt::LeftButton);
        const auto result=call(w,"document.inspect");QCOMPARE(result["evaluation"].toObject()["bodies"].toArray().size(),2);
        const auto feature=result["authored"].toObject()["features"].toArray().last().toObject();QCOMPARE(feature["parameters"].toObject()["endSupport"].toObject(),ref(back));
        session=call(w,"feature.editDialog",{{"featureId",feature["id"]}});QCOMPARE(session["inputs"].toObject()["endOffsetParameter"].toString(),QString("Clearance"));
        call(w,"command.cancel",{{"session",session["id"]}});QCOMPARE(call(w,"document.inspect")["authored"],result["authored"]);
    }
};
QTEST_MAIN(ToObjectControls)
#include "extrude_to_object_controls.moc"
