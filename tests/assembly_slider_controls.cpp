// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QJsonDocument>
#include <QPushButton>
#include <QTest>

class AssemblySliderControlsTest final:public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& args={}) {
        const auto response=window.execute(command,args);
        if(!response["ok"].toBool())qFatal("%s: %s",qPrintable(command),QJsonDocument(response).toJson().constData());
        return response["result"].toObject();
    }
private slots:
    void nativeSliderEntryCancelAndCommit(){
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1320,850);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));QApplication::setActiveWindow(&window);
        const auto box=call(window,"component.create",{{"name","Box"},{"activate",true}});
        call(window,"solid.box",{{"width",50},{"depth",80},{"height",40},{"name","Box stock"}});
        const auto lid=call(window,"component.create",{{"name","Lid"},{"activate",true}});
        call(window,"solid.box",{{"width",50},{"depth",80},{"height",8},{"z",40},{"name","Lid stock"}});
        auto authored=call(window,"document.inspect")["authored"].toObject();
        const auto evaluated=call(window,"document.inspect")["evaluation"].toObject()["bodies"].toArray();
        QJsonObject fixedBody;for(const auto& value:evaluated)if(value.toObject()["ownerId"]==box["definitionId"])fixedBody=value.toObject();
        QJsonObject topEdge;for(const auto& value:fixedBody["solidEdges"].toArray()){
            const auto candidate=value.toObject(),provenance=candidate["provenance"].toObject();
            const auto start=provenance["startMm"].toArray(),end=provenance["endMm"].toArray();
            if(start.size()==3 && end.size()==3 && start[2].toDouble()==40 && end[2].toDouble()==40 &&
                start[1].toDouble()==80 && end[1].toDouble()==80){topEdge=candidate;break;}
        }
        QVERIFY(!topEdge.isEmpty());
        auto action=window.findChild<QAction*>("solid.action.joint.sliderDialog");QVERIFY(action);action->trigger();
        auto* dock=window.findChild<QDockWidget*>("sliderDock");QVERIFY(dock && dock->isVisible());
        auto* first=dock->findChild<QComboBox*>("slider.component1");auto* second=dock->findChild<QComboBox*>("slider.component2");
        auto* ledge=dock->findChild<QComboBox*>("slider.position");QVERIFY(first && second && ledge);
        first->setCurrentIndex(first->findData(lid["occurrenceId"].toString()));
        second->setCurrentIndex(second->findData(box["occurrenceId"].toString()));
        QVERIFY(ledge->count()>1);
        call(window,"view.orientation",{{"name","top"}});call(window,"view.home");
        const auto pixel=call(window,"view.projectEdge",{{"bodyId",fixedBody["bodyId"]},{"edgeId",topEdge["edgeId"]}});
        ReferenceView* viewport=nullptr;for(auto* child:window.findChildren<QWidget*>())if(auto* found=dynamic_cast<ReferenceView*>(child)){viewport=found;break;}QVERIFY(viewport);
        QTest::mouseClick(viewport,Qt::LeftButton,Qt::NoModifier,QPoint(qRound(pixel["x"].toDouble()),qRound(pixel["y"].toDouble())));
        QCOMPARE(window.state()["activeSession"].toObject()["inputs"].toObject()["position"].toObject()["edgeId"],topEdge["edgeId"]);
        auto* minEnable=dock->findChild<QCheckBox*>("slider.minimumEnabled");auto* maxEnable=dock->findChild<QCheckBox*>("slider.maximumEnabled");
        auto* maximum=dock->findChild<QDoubleSpinBox*>("slider.maximumMm");QVERIFY(minEnable && maxEnable && maximum);
        QTest::mouseClick(minEnable,Qt::LeftButton);QTest::mouseClick(maxEnable,Qt::LeftButton);
        QTest::mouseClick(maximum,Qt::LeftButton);QTest::keyClick(maximum,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(maximum,"50");QTest::keyClick(maximum,Qt::Key_Tab);
        auto active=window.state()["activeSession"].toObject();QVERIFY(active["previewed"].toBool());
        const auto evidence=qEnvironmentVariable("ALLOY_SLIDER_EVIDENCE");
        if(!evidence.isEmpty())call(window,"capture.screenshot",{{"path",evidence+"/slider-native-preview.png"}});
        auto* buttons=dock->findChild<QDialogButtonBox*>("slider.buttons");QVERIFY(buttons);
        QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel),Qt::LeftButton);
        QCOMPARE(call(window,"document.inspect")["authored"].toObject(),authored);
        action->trigger();dock=window.findChild<QDockWidget*>("sliderDock");QVERIFY(dock);
        first=dock->findChild<QComboBox*>("slider.component1");second=dock->findChild<QComboBox*>("slider.component2");ledge=dock->findChild<QComboBox*>("slider.position");
        first->setCurrentIndex(first->findData(lid["occurrenceId"].toString()));
        second->setCurrentIndex(second->findData(box["occurrenceId"].toString()));ledge->setCurrentIndex(ledge->findData(QJsonObject{{"bodyId",fixedBody["bodyId"]},{"edgeId",topEdge["edgeId"]}}));
        minEnable=dock->findChild<QCheckBox*>("slider.minimumEnabled");maxEnable=dock->findChild<QCheckBox*>("slider.maximumEnabled");
        auto* reverse=dock->findChild<QCheckBox*>("slider.reverseDirection");QVERIFY(reverse && reverse->isVisible());
        QTest::mouseClick(reverse,Qt::LeftButton);
        maximum=dock->findChild<QDoubleSpinBox*>("slider.maximumMm");QTest::mouseClick(minEnable,Qt::LeftButton);QTest::mouseClick(maxEnable,Qt::LeftButton);
        QTest::mouseClick(maximum,Qt::LeftButton);QTest::keyClick(maximum,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(maximum,"50");QTest::keyClick(maximum,Qt::Key_Tab);
        buttons=dock->findChild<QDialogButtonBox*>("slider.buttons");QVERIFY(buttons->button(QDialogButtonBox::Ok)->isEnabled());
        if(!evidence.isEmpty())call(window,"capture.screenshot",{{"path",evidence+"/slider-native-reverse-preview.png"}});
        QTest::mouseClick(buttons->button(QDialogButtonBox::Ok),Qt::LeftButton);
        authored=call(window,"document.inspect")["authored"].toObject();QCOMPARE(authored["joints"].toArray().size(),1);
        const auto joint=authored["joints"].toArray()[0].toObject();
        QCOMPARE(joint["component1OccurrenceId"],lid["occurrenceId"]);QCOMPARE(joint["component2OccurrenceId"],box["occurrenceId"]);
        QCOMPARE(joint["reverseDirection"],true);
        call(window,"joint.positionSet",{{"jointId",joint["id"]},{"positionMm",20}});
        QCOMPARE(call(window,"joint.inspect")["joints"].toArray()[0].toObject()["coordinateMm"].toDouble(),20.);
        const auto occurrences=call(window,"joint.inspect")["occurrences"].toArray();
        for(const auto& value:occurrences)if(value.toObject()["occurrenceId"]==lid["occurrenceId"])
            QCOMPARE(value.toObject()["bodies"].toArray()[0].toObject()["boundsMm"].toObject()["min"].toArray()[1].toDouble(),-20.);
        if(!evidence.isEmpty())call(window,"capture.screenshot",{{"path",evidence+"/slider-native-driven.png"}});
        call(window,"document.new",{{"discard",true}});window.close();
    }
};
QTEST_MAIN(AssemblySliderControlsTest)
#include "assembly_slider_controls.moc"
