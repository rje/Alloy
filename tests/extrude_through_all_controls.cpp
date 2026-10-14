#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QJsonDocument>
#include <QPushButton>
#include <QTest>

class ThroughAllControls final:public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& args={}) {
        const auto result=window.execute(command,args);
        if(!result["ok"].toBool())qFatal("%s",QJsonDocument(result).toJson().constData());
        return result["result"].toObject();
    }
private slots:
    void extentFlipCancel(){
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1360,860);window.show();
        QApplication::setActiveWindow(&window);window.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&window));
        call(window,"solid.box",{{"width",20},{"depth",20},{"height",10},{"units","mm"}});
        const QString target=call(window,"document.inspect")["evaluation"].toObject()["bodies"].toArray().first().toObject()["bodyId"].toString();
        const QString sketch=call(window,"sketch.begin",{{"plane","XY"}})["draftSketch"].toObject()["id"].toString();
        call(window,"sketch.addCircle",{{"centerX",10},{"centerY",10},{"diameter",4},{"units","mm"}});call(window,"sketch.finish");
        const auto baseline=call(window,"document.inspect");
        const QString profile=baseline["evaluation"].toObject()["sketches"].toArray().last().toObject()["profiles"].toArray().first().toObject()["profileId"].toString();
        const auto session=call(window,"solid.extrudeDialog",{{"sketchId",sketch},{"profileId",profile},{"distance",1},{"operation","cut"},{"targetBodyIds",QJsonArray{target}}});
        auto* dock=window.findChild<QDockWidget*>("extrudeDock");QVERIFY(dock);
        auto* extent=dock->findChild<QComboBox*>("extrude.extent");auto* flip=dock->findChild<QCheckBox*>("extrude.flip");
        auto* buttons=dock->findChild<QDialogButtonBox*>();QVERIFY(extent && flip && buttons);
        extent->setFocus();QTest::keyClick(extent,Qt::Key_End);
        QTRY_COMPARE(extent->currentData().toString(),QString("all"));
        QTRY_VERIFY(window.state()["activeSession"].toObject()["previewed"].toBool());
        QVERIFY(!dock->findChild<QWidget*>("extrude.distance")->isVisible());
        QVERIFY(flip->isVisible() && !flip->isChecked());
        const auto evidence=qEnvironmentVariable("ALLOY_THROUGH_ALL_EVIDENCE");
        if(!evidence.isEmpty())call(window,"capture.dialog",{{"path",evidence+"/native-all-preview.png"}});
        QTest::mouseClick(flip,Qt::LeftButton,Qt::NoModifier,QPoint(10,flip->height()/2));
        QVERIFY(flip->isChecked());QTRY_VERIFY(!buttons->button(QDialogButtonBox::Ok)->isEnabled());
        QVERIFY(!window.state()["activeSession"].toObject()["previewed"].toBool());
        QTest::qWait(100);
        if(!evidence.isEmpty())call(window,"capture.dialog",{{"path",evidence+"/native-flipped-refusal.png"}});
        QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel),Qt::LeftButton);
        QTRY_VERIFY(window.state()["activeSession"].toObject().isEmpty());
        QCOMPARE(call(window,"document.inspect")["authored"],baseline["authored"]);
        QCOMPARE(call(window,"document.inspect")["revision"],baseline["revision"]);
        Q_UNUSED(session)
    }
};
QTEST_MAIN(ThroughAllControls)
#include "extrude_through_all_controls.moc"
