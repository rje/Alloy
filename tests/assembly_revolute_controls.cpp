// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QJsonDocument>
#include <QPushButton>
#include <QTest>
#include <cmath>

class AssemblyRevoluteControlsTest final:public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& args={}) {
        const auto response=window.execute(command,args);
        if(!response["ok"].toBool())qFatal("%s: %s",qPrintable(command),QJsonDocument(response).toJson().constData());
        return response["result"].toObject();
    }
private slots:
    void circularCanvasPickTypedAngleCancelAndCommit(){
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1360,880);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));QApplication::setActiveWindow(&window);
        const auto fixture=qEnvironmentVariable("ALLOY_REVOLUTE_FIXTURE");QVERIFY(!fixture.isEmpty());
        call(window,"document.open",{{"path",fixture},{"discard",true}});
        const auto initial=call(window,"document.inspect");const auto authored=initial["authored"].toObject();
        QString lower,upper,lowerOccurrence,upperOccurrence;
        for(const auto& value:authored["components"].toArray()){
            const auto row=value.toObject();if(row["name"]=="Lower arm")lower=row["id"].toString();
            if(row["name"]=="Upper arm")upper=row["id"].toString();}
        for(const auto& value:authored["occurrences"].toArray()){
            const auto row=value.toObject();if(row["definitionId"]==lower)lowerOccurrence=row["id"].toString();
            if(row["definitionId"]==upper)upperOccurrence=row["id"].toString();}
        QVERIFY(!lowerOccurrence.isEmpty() && !upperOccurrence.isEmpty());
        QJsonObject lowerBody;for(const auto& value:initial["evaluation"].toObject()["bodies"].toArray())
            if(value.toObject()["ownerId"]==lower)lowerBody=value.toObject();
        QVERIFY(!lowerBody.isEmpty());
        call(window,"view.orientation",{{"name","front"}});call(window,"view.home");
        QJsonObject selected,pixel;
        for(const auto& value:lowerBody["solidEdges"].toArray()){
            const auto edge=value.toObject();const auto start=edge["startMm"].toArray(),end=edge["endMm"].toArray(),mid=edge["midpointMm"].toArray();
            if(start.size()!=3 || end.size()!=3 || mid.size()!=3 ||
                std::abs(start[0].toDouble())>=5 || std::abs(start[2].toDouble()-80)>=5 ||
                std::abs(start[0].toDouble()-end[0].toDouble())>1e-6 ||
                std::abs(start[2].toDouble()-end[2].toDouble())>1e-6 ||
                std::abs(start[0].toDouble()-mid[0].toDouble())<1)continue;
            const auto projected=window.execute("view.projectEdge",{{"bodyId",lowerBody["bodyId"]},{"edgeId",edge["edgeId"]}});
            if(!projected["ok"].toBool())continue;
            const auto point=projected["result"].toObject();
            const auto hit=window.execute("selection.edgePick",{{"x",point["x"]},{"y",point["y"]}});
            if(hit["ok"].toBool() && hit["result"].toObject()["edgeId"]==edge["edgeId"]) {
                selected=edge;pixel=point;break;}
        }
        QVERIFY2(!selected.isEmpty(),"A visible retained Lower hinge circle must be screen-pickable");
        auto* action=window.findChild<QAction*>("solid.action.joint.revoluteDialog");QVERIFY(action);action->trigger();
        auto* dock=window.findChild<QDockWidget*>("revoluteDock");QVERIFY(dock && dock->isVisible());
        auto* first=dock->findChild<QComboBox*>("revolute.component1");auto* second=dock->findChild<QComboBox*>("revolute.component2");
        auto* position=dock->findChild<QComboBox*>("revolute.position");QVERIFY(first && second && position);
        first->setCurrentIndex(first->findData(upperOccurrence));second->setCurrentIndex(second->findData(lowerOccurrence));
        ReferenceView* viewport=nullptr;for(auto* child:window.findChildren<QWidget*>())if(auto* found=dynamic_cast<ReferenceView*>(child)){viewport=found;break;}QVERIFY(viewport);
        pixel=call(window,"view.projectEdge",{{"bodyId",lowerBody["bodyId"]},{"edgeId",selected["edgeId"]}});
        QTest::mouseClick(viewport,Qt::LeftButton,Qt::NoModifier,QPoint(qRound(pixel["x"].toDouble()),qRound(pixel["y"].toDouble())));
        QCOMPARE(window.state()["activeSession"].toObject()["inputs"].toObject()["position"].toObject()["edgeId"],selected["edgeId"]);
        auto* angle=dock->findChild<QDoubleSpinBox*>("revolute.angleDeg");QVERIFY(angle);
        QTest::mouseClick(angle,Qt::LeftButton);QTest::keyClick(angle,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(angle,"15");QTest::keyClick(angle,Qt::Key_Tab);
        QVERIFY(window.state()["activeSession"].toObject()["previewed"].toBool());
        const auto evidence=qEnvironmentVariable("ALLOY_REVOLUTE_EVIDENCE");
        if(!evidence.isEmpty())call(window,"capture.screenshot",{{"path",evidence+"/revolute-native-preview.png"}});
        auto* buttons=dock->findChild<QDialogButtonBox*>("revolute.buttons");QVERIFY(buttons);
        QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel),Qt::LeftButton);
        QCOMPARE(call(window,"document.inspect")["authored"].toObject(),authored);
        action->trigger();dock=window.findChild<QDockWidget*>("revoluteDock");QVERIFY(dock);
        first=dock->findChild<QComboBox*>("revolute.component1");second=dock->findChild<QComboBox*>("revolute.component2");
        position=dock->findChild<QComboBox*>("revolute.position");
        first->setCurrentIndex(first->findData(upperOccurrence));second->setCurrentIndex(second->findData(lowerOccurrence));
        position->setCurrentIndex(position->findData(QJsonObject{{"bodyId",lowerBody["bodyId"]},{"edgeId",selected["edgeId"]}}));
        angle=dock->findChild<QDoubleSpinBox*>("revolute.angleDeg");
        QTest::mouseClick(angle,Qt::LeftButton);QTest::keyClick(angle,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(angle,"-15");QTest::keyClick(angle,Qt::Key_Tab);
        buttons=dock->findChild<QDialogButtonBox*>("revolute.buttons");QVERIFY(buttons->button(QDialogButtonBox::Ok)->isEnabled());
        QTest::mouseClick(buttons->button(QDialogButtonBox::Ok),Qt::LeftButton);
        const auto joint=call(window,"joint.inspect")["joints"].toArray()[0].toObject();
        QCOMPARE(joint["coordinateDeg"].toDouble(),-15.);
        if(!evidence.isEmpty())call(window,"capture.screenshot",{{"path",evidence+"/revolute-native-accepted.png"}});
        call(window,"document.new",{{"discard",true}});window.close();
    }
};
QTEST_MAIN(AssemblyRevoluteControlsTest)
#include "assembly_revolute_controls.moc"
