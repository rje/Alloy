// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QTest>
#include <QJsonDocument>
#include <QDir>

class LinkedCircularProjectControls final:public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& args={}){
        const auto result=window.execute(command,args);
        if(!result["ok"].toBool())qFatal("%s: %s",qPrintable(command),QJsonDocument(result).toJson().constData());
        return result["result"].toObject();
    }
    QJsonObject ref(const QJsonObject& face){return {{"producerFeatureId",face["producerFeatureId"]},{"bodyId",face["bodyId"]},{"faceId",face["faceId"]}};}
private slots:
    void nativeHolePickPreviewCancel(){
        const auto fixture=qEnvironmentVariable("ALLOY_ADAPTED_BOARD_FIXTURE");if(fixture.isEmpty())QSKIP("External qualified board fixture unavailable");
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1500,950);window.show();window.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&window));ReferenceView* view=nullptr;
        for(auto* widget:window.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(widget))view=candidate;
        QVERIFY(view);
        const auto imported=call(window,"reference.insert",{{"path",fixture},{"name","Board Reference"}});
        call(window,"reference.move",{{"featureId",imported["featureId"]},{"placement",QJsonObject{{"axis","Z"},{"angleDeg",90},{"translationMm",QJsonArray{28,-42.5,6}}}}});
        call(window,"reference.capture");
        const QString owner=call(window,"component.create",{{"name","Plastic Case"},{"activate",true}})["definitionId"].toString();
        call(window,"solid.box",{{"width",64},{"depth",93},{"height",3.5},{"x",-32},{"y",-46.5},{"z",0},{"units","mm"}});
        const auto bodies=call(window,"document.inspect")["evaluation"].toObject()["bodies"].toArray();QJsonObject pcb,floor,top,edge;
        for(const auto& value:bodies){const auto body=value.toObject();if(body["name"]=="PCB-001")pcb=body;if(body["ownerId"]==owner)floor=body;}
        QVERIFY(!pcb.isEmpty() && !floor.isEmpty());
        for(const auto& value:floor["supportFaces"].toArray())if(value.toObject()["role"]=="box.z1")top=value.toObject();
        for(const auto& value:pcb["solidEdges"].toArray())if(value.toObject()["provenance"].toObject()["sourceEdgeIndex"]==12)edge=value.toObject();
        QVERIFY(!top.isEmpty() && !edge.isEmpty());
        call(window,"view.orientation",{{"name","top"}});call(window,"view.home");
        call(window,"sketch.begin",{{"support",ref(top)},{"name","Mounting Circles"}});
        const QJsonObject original=window.state()["activeSession"].toObject()["draftSketch"].toObject();
        view->setFocus(Qt::OtherFocusReason);QTest::keyClick(view,Qt::Key_P);
        QTRY_COMPARE(window.state()["activeSession"].toObject()["tool"].toString(),QString("project"));
        const auto pixel=call(window,"view.projectSupportEdge",{{"bodyId",pcb["bodyId"]},{"edgeId",edge["edgeId"]}});
        const QPoint target(qRound(pixel["x"].toDouble()),qRound(pixel["y"].toDouble()));QVERIFY(view->rect().contains(target));
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,target);
        auto session=window.state()["activeSession"].toObject();QVERIFY(session.contains("pendingProject"));
        QCOMPARE(session["pendingProject"].toObject()["previewDraft"].toObject()["parameters"].toObject()["circleProjections"].toArray().size(),1);
        const auto evidence=qEnvironmentVariable("ALLOY_CIRCULAR_PROJECT_EVIDENCE");if(!evidence.isEmpty()){
            QDir().mkpath(evidence);call(window,"capture.screenshot",{{"path",evidence+"/native-hole-project-preview.png"}});
            call(window,"capture.dialog",{{"path",evidence+"/native-project-panel.png"}});
        }
        QTest::keyClick(view,Qt::Key_Escape);QTRY_VERIFY(!window.state()["activeSession"].toObject().contains("pendingProject"));
        QCOMPARE(window.state()["activeSession"].toObject()["draftSketch"],original);
        call(window,"document.new",{{"discard",true}});window.close();
    }
};
QTEST_MAIN(LinkedCircularProjectControls)
#include "linked_circular_project_controls.moc"
