// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QJsonDocument>
#include <QLineF>
#include <QMouseEvent>
#include <QPointer>
#include <QPushButton>
#include <QTest>
#include <cmath>

class AssemblyCopiedControlsTest final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& args={}) {
        const auto response=window.execute(command,args);
        if(!response["ok"].toBool())qFatal("%s: %s",qPrintable(command),QJsonDocument(response).toJson().constData());
        return response["result"].toObject();
    }
    static QString definition(const QJsonObject& authored,const QString& name) {
        for(const auto& value:authored["components"].toArray())if(value.toObject()["name"]==name)return value.toObject()["id"].toString();
        return {};
    }
    static QString occurrence(const QJsonObject& authored,const QString& owner) {
        for(const auto& value:authored["occurrences"].toArray())if(value.toObject()["definitionId"]==owner)return value.toObject()["id"].toString();
        return {};
    }
    static QJsonObject eye(const QJsonObject& inspected,const QString& owner,double x,double z) {
        for(const auto& value:inspected["evaluation"].toObject()["bodies"].toArray()) {
            const auto body=value.toObject();if(body["ownerId"]!=owner)continue;
            for(const auto& edgeValue:body["solidEdges"].toArray()) {
                const auto edge=edgeValue.toObject();
                const auto a=edge["startMm"].toArray(),b=edge["endMm"].toArray(),m=edge["midpointMm"].toArray();
                if(a.size()!=3 || b.size()!=3 || m.size()!=3)continue;
                if(std::abs(a[0].toDouble()-b[0].toDouble())>1e-6
                    || std::abs(a[2].toDouble()-b[2].toDouble())>1e-6
                    || std::abs(a[0].toDouble()-m[0].toDouble())<1)continue;
                if(std::abs(a[0].toDouble()-x)<7 && std::abs(m[0].toDouble()-x)<7
                    && std::abs(a[2].toDouble()-z)<7 && std::abs(m[2].toDouble()-z)<7)
                    return {{"bodyId",body["bodyId"]},{"edgeId",edge["edgeId"]}};
            }
        }
        return {};
    }
private slots:
    void copiedOriginAndWholeGraphDrag() {
        const auto source=qEnvironmentVariable("ALLOY_COPIED_SOURCE");
        const auto complete=qEnvironmentVariable("ALLOY_COPIED_COMPLETE");
        QVERIFY(!source.isEmpty() && !complete.isEmpty());
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1510,920);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));QApplication::setActiveWindow(&window);
        ReferenceView* view=nullptr;for(auto* child:window.findChildren<QWidget*>())
            if(auto* found=dynamic_cast<ReferenceView*>(child)){view=found;break;}QVERIFY(view);
        call(window,"document.open",{{"path",source},{"discard",true}});
        const auto initial=call(window,"document.inspect"),authored=initial["authored"].toObject();
        QCOMPARE(authored["joints"].toArray().size(),4);
        const auto driver=authored["joints"].toArray()[0].toObject()["id"].toString();
        const auto parent=occurrence(authored,definition(authored,"Piston subassembly"));
        const auto lower=occurrence(authored,definition(authored,"Lower arm"));
        const auto inner=occurrence(authored,definition(authored,"Inner piston"));
        QVERIFY(!parent.isEmpty() && !lower.isEmpty() && !inner.isEmpty());
        call(window,"joint.angleSet",{{"jointId",driver},{"angleDeg",0}});
        const auto rest=call(window,"document.inspect")["authored"].toObject();
        call(window,"joint.previewAngle",{{"jointId",driver},{"angleDeg",15}});
        const auto placement=QJsonObject{{"axis","Z"},{"angleDeg",0},{"translationMm",QJsonArray{-180,0,0}}};
        const auto refused=window.execute("component.copyOccurrence",{{"occurrenceId",parent},{"placement",placement}});
        QVERIFY(!refused["ok"].toBool());QCOMPARE(refused["error"].toObject()["code"].toString(),"staged_pose");
        QCOMPARE(call(window,"document.inspect")["authored"].toObject(),rest);
        call(window,"position.revert");

        call(window,"component.copyOccurrenceDialog",{{"occurrenceId",parent}});
        QPointer<QDialog> dialog=window.findChild<QDialog*>("component.copy.dialog");QVERIFY(dialog && dialog->isVisible());
        auto* x=dialog->findChild<QDoubleSpinBox*>("component.copy.x");QVERIFY(x);
        QTest::mouseClick(x,Qt::LeftButton);QTest::keyClick(x,Qt::Key_A,Qt::ControlModifier);
        QTest::keyClicks(x,"-180");QTest::keyClick(x,Qt::Key_Tab);QCOMPARE(x->value(),-180.0);
        QTest::mouseClick(dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok),Qt::LeftButton);
        QTRY_VERIFY(!dialog->isVisible());
        const auto copied=call(window,"document.inspect")["authored"].toObject();
        QCOMPARE(copied["joints"].toArray().size(),5);
        QString copiedInner;
        for(const auto& value:copied["occurrences"].toArray()){
            const auto r=value.toObject();
            if(r["definitionId"]==definition(copied,"Inner piston") && r["id"]!=inner)copiedInner=r["id"].toString();
        }
        QVERIFY(!copiedInner.isEmpty());
        call(window,"view.orientation",{{"name","front"}});call(window,"view.home");
        const auto sourceEye=eye(initial,definition(authored,"Inner piston"),90,0);
        QVERIFY2(!sourceEye.isEmpty(),"Copied Inner must retain its original end-eye edge ID");

        call(window,"joint.revoluteDialog");
        auto* dock=window.findChild<QDockWidget*>("revoluteDock");QVERIFY(dock && dock->isVisible());
        auto* first=dock->findChild<QComboBox*>("revolute.component1");
        auto* second=dock->findChild<QComboBox*>("revolute.component2");
        QVERIFY(first && second);
        first->setCurrentIndex(first->findData(lower));
        second->setCurrentIndex(second->findData(copiedInner));
        QVERIFY(second->currentText().contains("Inner piston") && second->currentText().contains("[2]"));
        const auto projected=call(window,"view.projectEdge",{{"bodyId",sourceEye["bodyId"]},
            {"edgeId",sourceEye["edgeId"]},{"occurrenceId",copiedInner}});
        const QPoint center(qRound(projected["x"].toDouble()),qRound(projected["y"].toDouble()));
        QPoint chosen;for(int dx=-14;dx<=14 && chosen.isNull();++dx)for(int dy=-14;dy<=14;++dy){
            const QPoint point=center+QPoint(dx,dy);
            if(!view->rect().contains(point))continue;
            const auto picked=window.execute("selection.edgePick",{{"x",point.x()},{"y",point.y()}});
            if(!picked["ok"].toBool())continue;
            const auto hit=picked["result"].toObject();
            if(hit["edgeId"]==sourceEye["edgeId"] && hit["occurrenceId"]==copiedInner){chosen=point;break;}
        }
        QVERIFY2(!chosen.isNull(),"Find actual visible copied circular edge pixel");
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,chosen);
        QCOMPARE(window.state()["activeSession"].toObject()["inputs"].toObject()["position"].toObject()["occurrenceId"].toString(),copiedInner);
        auto* position=dock->findChild<QComboBox*>("revolute.position");QVERIFY(position);
        QVERIFY(position->currentText().contains("circle"));
        const auto evidence=qEnvironmentVariable("ALLOY_COPIED_NATIVE_EVIDENCE");
        if(!evidence.isEmpty())call(window,"capture.screenshot",{{"path",evidence+"/copied-origin-preview.png"}});
        auto* buttons=dock->findChild<QDialogButtonBox*>("revolute.buttons");QVERIFY(buttons);
        QVERIFY(buttons->button(QDialogButtonBox::Ok)->isEnabled());
        QTest::mouseClick(buttons->button(QDialogButtonBox::Ok),Qt::LeftButton);
        QCOMPARE(call(window,"document.inspect")["authored"].toObject()["joints"].toArray().size(),6);

        call(window,"document.open",{{"path",complete},{"discard",true}});
        const auto final=call(window,"document.inspect")["authored"].toObject();
        QCOMPARE(final["joints"].toArray().size(),7);
        call(window,"view.orientation",{{"name","front"}});call(window,"view.home");
        const auto baseline=window.state()["assembly"].toObject();
        const auto joints=baseline["joints"].toArray();
        QCOMPARE(joints.size(),7);
        const auto hinge=joints[0].toObject();
        const auto frame=hinge["originFrameWorld"].toArray();
        QCOMPARE(frame.size(),12);
        const auto pivot=view->projectWorldPoint({frame[3],frame[7],frame[11]});
        const auto upper=definition(final,"Upper arm");
        QString upperBody;for(const auto& value:call(window,"document.inspect")["evaluation"].toObject()["bodies"].toArray())
            if(value.toObject()["ownerId"]==upper)upperBody=value.toObject()["bodyId"].toString();
        QVERIFY(!upperBody.isEmpty());
        const auto projectedBody=call(window,"view.projectBody",{{"bodyId",upperBody}});
        const QPoint bodyCenter(qRound(projectedBody["x"].toDouble()),qRound(projectedBody["y"].toDouble()));
        QPoint press;double best=0;
        for(int dx=-75;dx<=75;dx+=15)for(int dy=-60;dy<=60;dy+=15){
            const QPoint p=bodyCenter+QPoint(dx,dy);
            if(!view->rect().contains(p) || view->pickBody(p.x(),p.y())!=upperBody)continue;
            const double distance=QLineF(QPointF(p),pivot).length();
            if(distance>best){best=distance;press=p;}
        }
        QVERIFY(best>35);
        const auto radius=QPointF(press)-pivot;
        const auto angle=12*std::acos(-1.0)/180;
        const QPoint target(qRound(pivot.x()+std::cos(angle)*radius.x()-std::sin(angle)*radius.y()),
            qRound(pivot.y()+std::sin(angle)*radius.x()+std::cos(angle)*radius.y()));
        QVERIFY(view->rect().contains(target));
        QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,press);QVERIFY(view->hasFocus());
        QMouseEvent move(QEvent::MouseMove,QPointF(target),QPointF(view->mapToGlobal(target)),
            Qt::NoButton,Qt::LeftButton,Qt::NoModifier);
        QCoreApplication::sendEvent(view,&move);
        QTest::mouseRelease(view,Qt::LeftButton,Qt::NoModifier,target);
        const auto staged=window.state()["assembly"].toObject();
        QVERIFY(std::abs(staged["joints"].toArray()[0].toObject()["coordinateDeg"].toDouble()-15)>1);
        int moved=0;const auto beforePoses=baseline["occurrences"].toArray(),afterPoses=staged["occurrences"].toArray();
        QCOMPARE(beforePoses.size(),afterPoses.size());
        for(int i=0;i<beforePoses.size();++i)if(beforePoses[i].toObject()["worldPose"]!=afterPoses[i].toObject()["worldPose"])++moved;
        QVERIFY(moved>=4);
        QCOMPARE(call(window,"document.inspect")["authored"].toObject(),final);
        if(!evidence.isEmpty())call(window,"capture.screenshot",{{"path",evidence+"/two-piston-native-drag.png"}});
        QCOMPARE(call(window,"position.revert")["occurrences"],baseline["occurrences"]);
        QCOMPARE(call(window,"document.inspect")["authored"].toObject(),final);
        qInfo("Copied circular edge picked; seven-joint drag staged %d leaf poses and Revert restored all",moved);
        call(window,"document.new",{{"discard",true}});window.close();
    }
};
QTEST_MAIN(AssemblyCopiedControlsTest)
#include "assembly_copied_controls.moc"
