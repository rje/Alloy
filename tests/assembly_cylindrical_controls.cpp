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

class AssemblyCylindricalControlsTest final:public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& args={}) {
        const auto response=window.execute(command,args);
        if(!response["ok"].toBool())qFatal("%s: %s",qPrintable(command),QJsonDocument(response).toJson().constData());
        return response["result"].toObject();
    }
private slots:
    void nestedPistonCircularPickAndTwoCoordinateEdit(){
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1360,880);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));QApplication::setActiveWindow(&window);
        const auto fixture=qEnvironmentVariable("ALLOY_CYLINDRICAL_FIXTURE");QVERIFY(!fixture.isEmpty());
        call(window,"document.open",{{"path",fixture},{"discard",true}});
        const auto initial=call(window,"document.inspect");const auto authored=initial["authored"].toObject();
        QString inner,outer,innerOccurrence,outerOccurrence;
        for(const auto& value:authored["components"].toArray()){
            const auto row=value.toObject();if(row["name"]=="Inner piston")inner=row["id"].toString();
            if(row["name"]=="Outer piston")outer=row["id"].toString();}
        for(const auto& value:authored["occurrences"].toArray()){
            const auto row=value.toObject();if(row["definitionId"]==inner)innerOccurrence=row["id"].toString();
            if(row["definitionId"]==outer)outerOccurrence=row["id"].toString();}
        QVERIFY(!innerOccurrence.isEmpty() && !outerOccurrence.isEmpty());
        QJsonObject outerBody;for(const auto& value:initial["evaluation"].toObject()["bodies"].toArray())
            if(value.toObject()["ownerId"]==outer)outerBody=value.toObject();
        QVERIFY(!outerBody.isEmpty());
        auto* action=window.findChild<QAction*>("solid.action.joint.cylindricalDialog");QVERIFY(action);action->trigger();
        auto* dock=window.findChild<QDockWidget*>("cylindricalDock");QVERIFY(dock && dock->isVisible());
        auto* first=dock->findChild<QComboBox*>("cylindrical.component1");
        auto* second=dock->findChild<QComboBox*>("cylindrical.component2");
        QVERIFY(first && second);first->setCurrentIndex(first->findData(innerOccurrence));
        second->setCurrentIndex(second->findData(outerOccurrence));
        ReferenceView* viewport=nullptr;for(auto* child:window.findChildren<QWidget*>())
            if(auto* found=dynamic_cast<ReferenceView*>(child)){viewport=found;break;}QVERIFY(viewport);
        QJsonObject selected,pixel;
        for(const auto& orientation:{"top","bottom","isometric"}){
            call(window,"view.orientation",{{"name",orientation}});call(window,"view.home");
            for(const auto& value:outerBody["solidEdges"].toArray()){
                const auto edge=value.toObject();const auto a=edge["startMm"].toArray(),b=edge["endMm"].toArray(),m=edge["midpointMm"].toArray();
                if(a.size()!=3 || b.size()!=3 || m.size()!=3 || std::abs(a[2].toDouble()-b[2].toDouble())>1e-6
                    || std::abs(a[2].toDouble()-m[2].toDouble())>1e-6
                    || std::abs(a[0].toDouble()-b[0].toDouble())>1e-6
                    || std::abs(a[1].toDouble()-b[1].toDouble())>1e-6
                    || std::abs(a[0].toDouble()-m[0].toDouble())<1)continue;
                const auto projected=window.execute("view.projectEdge",{{"bodyId",outerBody["bodyId"]},{"edgeId",edge["edgeId"]}});
                if(!projected["ok"].toBool())continue;const auto point=projected["result"].toObject();
                const auto hit=window.execute("selection.edgePick",{{"x",point["x"]},{"y",point["y"]}});
                if(hit["ok"].toBool() && hit["result"].toObject()["edgeId"]==edge["edgeId"]){selected=edge;pixel=point;break;}
            }
            if(!selected.isEmpty())break;
        }
        QVERIFY2(!selected.isEmpty(),"A visible retained Outer tube circle must be screen-pickable");
        QTest::mouseClick(viewport,Qt::LeftButton,Qt::NoModifier,QPoint(qRound(pixel["x"].toDouble()),qRound(pixel["y"].toDouble())));
        QCOMPARE(window.state()["activeSession"].toObject()["inputs"].toObject()["position"].toObject()["edgeId"],selected["edgeId"]);
        auto* buttons=dock->findChild<QDialogButtonBox*>("cylindrical.buttons");QVERIFY(buttons);
        QVERIFY(buttons->button(QDialogButtonBox::Ok)->isEnabled());
        const auto evidence=qEnvironmentVariable("ALLOY_CYLINDRICAL_EVIDENCE");
        if(!evidence.isEmpty())call(window,"capture.screenshot",{{"path",evidence+"/cylindrical-native-selected.png"}});
        QTest::mouseClick(buttons->button(QDialogButtonBox::Ok),Qt::LeftButton);
        auto after=call(window,"document.inspect")["authored"].toObject();
        const auto joint=after["joints"].toArray().last().toObject();QCOMPARE(joint["type"],"asBuiltCylindrical");
        call(window,"joint.cylindricalDialog",{{"jointId",joint["id"]}});
        dock=window.findChild<QDockWidget*>("cylindricalDock");QVERIFY(dock);
        auto* rotation=dock->findChild<QDoubleSpinBox*>("cylindrical.rotationDeg");
        auto* slide=dock->findChild<QDoubleSpinBox*>("cylindrical.slideMm");QVERIFY(rotation && slide);
        QTest::mouseClick(rotation,Qt::LeftButton);QTest::keyClick(rotation,Qt::Key_A,Qt::ControlModifier);
        QTest::keyClicks(rotation,"15");QTest::keyClick(rotation,Qt::Key_Tab);
        QTest::mouseClick(slide,Qt::LeftButton);QTest::keyClick(slide,Qt::Key_A,Qt::ControlModifier);
        QTest::keyClicks(slide,"5");QTest::keyClick(slide,Qt::Key_Tab);
        QVERIFY(window.state()["activeSession"].toObject()["previewed"].toBool());
        if(!evidence.isEmpty())call(window,"capture.screenshot",{{"path",evidence+"/cylindrical-native-preview.png"}});
        buttons=dock->findChild<QDialogButtonBox*>("cylindrical.buttons");
        QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel),Qt::LeftButton);
        QCOMPARE(call(window,"document.inspect")["authored"].toObject(),after);
        call(window,"document.new",{{"discard",true}});window.close();
    }
};
QTEST_MAIN(AssemblyCylindricalControlsTest)
#include "assembly_cylindrical_controls.moc"
