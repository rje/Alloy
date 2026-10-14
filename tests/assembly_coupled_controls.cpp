// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QMouseEvent>
#include <QPushButton>
#include <QTest>
#include <cmath>

class AssemblyCoupledControlsTest final:public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& args={}) {
        const auto response=window.execute(command,args);
        if(!response["ok"].toBool())qFatal("%s: %s",qPrintable(command),QJsonDocument(response).toJson().constData());
        return response["result"].toObject();
    }
    static QString definition(const QJsonObject& authored,const QString& name){
        for(const auto& value:authored["components"].toArray())if(value.toObject()["name"]==name)return value.toObject()["id"].toString();
        return {};
    }
    static QString occurrence(const QJsonObject& authored,const QString& owner){
        for(const auto& value:authored["occurrences"].toArray())if(value.toObject()["definitionId"]==owner)return value.toObject()["id"].toString();
        return {};
    }
private slots:
    void additionalRevoluteAndUpperArmDrag(){
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1360,880);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));QApplication::setActiveWindow(&window);
        ReferenceView* view=nullptr;for(auto* child:window.findChildren<QWidget*>())
            if(auto* found=dynamic_cast<ReferenceView*>(child)){view=found;break;}QVERIFY(view);
        const auto intermediate=qEnvironmentVariable("ALLOY_COUPLED_INTERMEDIATE");
        const auto complete=qEnvironmentVariable("ALLOY_COUPLED_FIXTURE");
        QVERIFY(!intermediate.isEmpty() && !complete.isEmpty());
        call(window,"document.open",{{"path",intermediate},{"discard",true}});
        const auto before=call(window,"document.inspect"),authored=before["authored"].toObject();
        QCOMPARE(authored["joints"].toArray().size(),2);
        const auto lower=definition(authored,"Lower arm"),inner=definition(authored,"Inner piston");
        const auto lowerOccurrence=occurrence(authored,lower),innerOccurrence=occurrence(authored,inner);
        QVERIFY(!lowerOccurrence.isEmpty() && !innerOccurrence.isEmpty());
        QJsonObject position;
        for(const auto& value:before["evaluation"].toObject()["bodies"].toArray()){
            const auto body=value.toObject();if(body["ownerId"]!=lower)continue;
            for(const auto& edgeValue:body["solidEdges"].toArray()){
                const auto edge=edgeValue.toObject();
                const auto a=edge["startMm"].toArray(),b=edge["endMm"].toArray(),m=edge["midpointMm"].toArray();
                if(a.size()!=3 || b.size()!=3 || m.size()!=3)continue;
                if(std::abs(a[0].toDouble()-b[0].toDouble())>1e-6 || std::abs(a[2].toDouble()-b[2].toDouble())>1e-6)continue;
                if(std::abs((a[0].toDouble()+m[0].toDouble())/2-90)>1e-4 ||
                   std::abs((a[2].toDouble()+m[2].toDouble())/2)>1e-4)continue;
                position={{"bodyId",body["bodyId"]},{"edgeId",edge["edgeId"]}};break;
            }
        }
        QVERIFY2(!position.isEmpty(),"Actual Lower +90 mm circular eye must be retained");
        call(window,"joint.revoluteDialog");
        auto* dock=window.findChild<QDockWidget*>("revoluteDock");QVERIFY(dock && dock->isVisible());
        QVERIFY2(!window.state()["activeSession"].toObject()["inputs"].toObject().contains("jointId"),
            "A new As-Built Joint must not reopen the first Revolute");
        auto* first=dock->findChild<QComboBox*>("revolute.component1");
        auto* second=dock->findChild<QComboBox*>("revolute.component2");
        auto* edge=dock->findChild<QComboBox*>("revolute.position");
        QVERIFY(first && second && edge);
        first->setCurrentIndex(first->findData(innerOccurrence));
        second->setCurrentIndex(second->findData(lowerOccurrence));
        const int positionIndex=edge->findData(position);QVERIFY(positionIndex>0);edge->setCurrentIndex(positionIndex);
        auto* buttons=dock->findChild<QDialogButtonBox*>("revolute.buttons");QVERIFY(buttons);
        QVERIFY(buttons->button(QDialogButtonBox::Ok)->isEnabled());
        QTest::mouseClick(buttons->button(QDialogButtonBox::Ok),Qt::LeftButton);
        const auto after=call(window,"document.inspect")["authored"].toObject();
        QCOMPARE(after["joints"].toArray().size(),3);
        QCOMPARE(after["joints"].toArray()[0],authored["joints"].toArray()[0]);
        QCOMPARE(after["joints"].toArray()[1],authored["joints"].toArray()[1]);

        call(window,"document.open",{{"path",complete},{"discard",true}});
        const auto saved=call(window,"document.inspect")["authored"].toObject();
        call(window,"view.orientation",{{"name","front"}});call(window,"view.home");
        const auto baseline=window.state()["assembly"].toObject();const auto joints=baseline["joints"].toArray();
        QCOMPARE(joints.size(),4);const auto driver=joints[0].toObject();
        QVERIFY(driver["coordinateEditable"].toBool() && driver["pinAuthored"].toBool());
        const auto frame=driver["originFrameWorld"].toArray();QCOMPARE(frame.size(),12);
        const auto pivot=view->projectWorldPoint({frame[3],frame[7],frame[11]});
        const auto upper=definition(saved,"Upper arm");QString upperBody;
        for(const auto& value:call(window,"document.inspect")["evaluation"].toObject()["bodies"].toArray())
            if(value.toObject()["ownerId"]==upper)upperBody=value.toObject()["bodyId"].toString();
        QVERIFY(!upperBody.isEmpty());
        QPoint press;double best=0;
        const auto projected=call(window,"view.projectBody",{{"bodyId",upperBody}});
        const QPoint bodyCenter(qRound(projected["x"].toDouble()),qRound(projected["y"].toDouble()));
        for(int x=-75;x<=75;x+=15)for(int y=-60;y<=60;y+=15){
            const QPoint candidate=bodyCenter+QPoint(x,y);if(candidate.x()<5 || candidate.y()<5 || candidate.x()>=view->width()-5 || candidate.y()>=view->height()-5)continue;
            if(view->pickBody(candidate.x(),candidate.y())!=upperBody)continue;
            const double distance=QLineF(QPointF(candidate),pivot).length();if(distance>best){best=distance;press=candidate;}
        }
        QVERIFY2(best>35,"Find a visible UpperArm pixel away from the retained hinge");
        const auto radius=QPointF(press)-pivot;const double radians=12*std::acos(-1.0)/180;
        const QPoint target(qRound(pivot.x()+std::cos(radians)*radius.x()-std::sin(radians)*radius.y()),
            qRound(pivot.y()+std::sin(radians)*radius.x()+std::cos(radians)*radius.y()));
        QVERIFY(view->rect().contains(target));
        QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,press);
        QVERIFY(view->hasFocus());
        QElapsedTimer latency;latency.start();
        QMouseEvent move(QEvent::MouseMove,QPointF(target),QPointF(view->mapToGlobal(target)),
            Qt::NoButton,Qt::LeftButton,Qt::NoModifier);
        QCoreApplication::sendEvent(view,&move);
        const auto dragMilliseconds=latency.elapsed();
        QTest::mouseRelease(view,Qt::LeftButton,Qt::NoModifier,target);
        const auto staged=window.state()["assembly"].toObject();
        QVERIFY2(std::abs(staged["joints"].toArray()[0].toObject()["coordinateDeg"].toDouble())>1,
            "Pointer drag must stage a meaningful coupled Revolute angle");
        int moved=0;const auto beforePoses=baseline["occurrences"].toArray(),afterPoses=staged["occurrences"].toArray();
        QCOMPARE(afterPoses.size(),beforePoses.size());
        for(int i=0;i<beforePoses.size();++i)if(beforePoses[i].toObject()["worldPose"]!=afterPoses[i].toObject()["worldPose"])++moved;
        QVERIFY2(moved>=2,"The drag must stage multiple solved occurrence poses");
        QCOMPARE(call(window,"document.inspect")["authored"].toObject(),saved);
        const auto evidence=qEnvironmentVariable("ALLOY_COUPLED_EVIDENCE");
        if(!evidence.isEmpty())call(window,"capture.screenshot",{{"path",evidence+"/coupled-native-drag.png"}});
        const auto reverted=call(window,"position.revert");
        QCOMPARE(reverted["occurrences"],baseline["occurrences"]);
        QCOMPARE(call(window,"document.inspect")["authored"].toObject(),saved);
        qInfo("UpperArm Qt pointer drag command latency %lld ms; %d occurrence poses changed",
            static_cast<long long>(dragMilliseconds),moved);
        call(window,"document.new",{{"discard",true}});window.close();
    }
};
QTEST_MAIN(AssemblyCoupledControlsTest)
#include "assembly_coupled_controls.moc"
