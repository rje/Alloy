// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QTest>
#include <QJsonDocument>
#include <QDir>
#include <QFile>
#include <cmath>
class LinkedPlanarProjectControls final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& args={}){
        const auto result=window.execute(command,args);
        if(!result["ok"].toBool())qFatal("%s",QJsonDocument(result).toJson().constData());
        return result["result"].toObject();
    }
    QJsonObject ref(const QJsonObject& face){return {{"producerFeatureId",face["producerFeatureId"]},{"bodyId",face["bodyId"]},{"faceId",face["faceId"]}};}
private slots:
    void nativeProjectedInnerEdge(){
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1360,860);window.show();window.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&window));ReferenceView* view=nullptr;
        for(auto* widget:window.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(widget))view=candidate;
        QVERIFY(view);
        call(window,"solid.box",{{"width",50},{"depth",80},{"height",40},{"x",0},{"y",0},{"z",0},{"units","mm"}});
        auto body=call(window,"document.inspect")["evaluation"].toObject()["bodies"].toArray()[0].toObject();
        QJsonObject top;for(const auto& entry:body["supportFaces"].toArray())if(entry.toObject()["role"]=="box.z1")top=entry.toObject();
        QVERIFY(!top.isEmpty());call(window,"solid.shell",{{"targetBodyId",body["bodyId"]},{"removedFace",ref(top)},{"thicknessMm",4}});
        body=call(window,"document.inspect")["evaluation"].toObject()["bodies"].toArray()[0].toObject();
        QJsonObject front,innerEdge;for(const auto& entry:body["supportFaces"].toArray()){
            const auto face=entry.toObject();if(face["role"]=="box.y0")front=face;
            if(face["role"]!="shell-inner")continue;
            for(const auto& candidate:face["edges"].toArray()){
                const auto edge=candidate.toObject();const auto a=edge["startMm"].toArray(),b=edge["endMm"].toArray();
                if(a.size()==3 && b.size()==3 && a[0].toDouble()>40 && a[2].toDouble()>39
                    && std::abs(a[0].toDouble()-b[0].toDouble())<1e-6
                    && std::abs(a[2].toDouble()-b[2].toDouble())<1e-6 && std::abs(a[1].toDouble()-b[1].toDouble())>1)
                    innerEdge=edge;
            }
        }
        QVERIFY(!front.isEmpty() && !innerEdge.isEmpty());
        call(window,"sketch.begin",{{"support",ref(front)}});
        call(window,"view.home");QApplication::processEvents();view->setFocus(Qt::OtherFocusReason);
        QTest::keyClick(view,Qt::Key_P);QTRY_COMPARE(window.state()["activeSession"].toObject()["tool"].toString(),QString("project"));
        const auto pixel=call(window,"view.projectSupportEdge",{{"bodyId",body["bodyId"]},{"edgeId",innerEdge["edgeId"]}});
        const QPoint target(qRound(pixel["x"].toDouble()),qRound(pixel["y"].toDouble()));
        QVERIFY(view->rect().contains(target));QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,target);
        const auto session=window.state()["activeSession"].toObject();
        const auto pending=session["pendingProject"].toObject();QVERIFY(!pending.isEmpty());
        QCOMPARE(pending["sources"].toArray().size(),1);
        QCOMPARE(pending["previewDraft"].toObject()["parameters"].toObject()["pointProjections"].toArray().size(),1);
        auto evidence=qEnvironmentVariable("ALLOY_LINKED_PROJECT_EVIDENCE");if(!evidence.isEmpty()){
            QDir().mkpath(evidence);call(window,"capture.screenshot",{{"path",evidence+"/native-project-point.png"}});
            QFile file(evidence+"/native-project-point.json");QVERIFY(file.open(QIODevice::WriteOnly));file.write(QJsonDocument(window.state()).toJson());
        }
        QTest::keyClick(view,Qt::Key_Escape);QTRY_VERIFY(!window.state()["activeSession"].toObject().contains("pendingProject"));
    }
};
QTEST_MAIN(LinkedPlanarProjectControls)
#include "linked_planar_project_controls.moc"
