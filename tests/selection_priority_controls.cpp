// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QDir>
#include <QJsonDocument>
#include <QMenu>
#include <QTest>
#include <QTimer>
#include <QToolButton>
#include <cmath>

class SelectionPriorityControlsTest final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& name,const QJsonObject& args={}) {
        const auto response=window.execute(name,args);
        if(!response["ok"].toBool())qFatal("%s",QJsonDocument(response).toJson().constData());
        return response["result"].toObject();
    }
private slots:
    void menuAndBoxEdges() {
        const auto evidence=qEnvironmentVariable("ALLOY_SELECTION_PRIORITY_EVIDENCE");
        QVERIFY(!evidence.isEmpty());QDir().mkpath(evidence);
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1440,900);window.show();window.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        ReferenceView* view=nullptr;for(auto* child:window.findChildren<QWidget*>())
            if(auto* found=dynamic_cast<ReferenceView*>(child))view=found;
        QVERIFY(view);
        call(window,"solid.box",{{"width",50},{"depth",80},{"height",40},{"x",0},{"y",0},{"z",0},{"units","mm"}});
        const auto initial=call(window,"document.inspect");
        const auto body=initial["evaluation"].toObject()["bodies"].toArray().first().toObject();
        const auto bodyId=body["bodyId"].toString();
        call(window,"view.home");call(window,"view.orientation",{{"name","front"}});
        QString topId,bottomId;
        for(const auto& value:body["solidEdges"].toArray()){
            const auto edge=value.toObject();
            const auto a=edge["startMm"].toArray(),b=edge["endMm"].toArray();
            if(a.size()!=3 || b.size()!=3 || std::abs(a[1].toDouble())>1e-5 || std::abs(b[1].toDouble())>1e-5
                || std::abs(a[2].toDouble()-b[2].toDouble())>1e-5 || std::abs(a[0].toDouble()-b[0].toDouble())<40)continue;
            if(std::abs(a[2].toDouble()-40)<1e-5)topId=edge["edgeId"].toString();
            if(std::abs(a[2].toDouble())<1e-5)bottomId=edge["edgeId"].toString();
        }
        QVERIFY(!topId.isEmpty() && !bottomId.isEmpty());
        auto* select=window.findChild<QToolButton*>("solid.menu.select");
        auto* menu=window.findChild<QMenu*>("solid.commands.select");
        auto* priority=window.findChild<QMenu*>("solid.commands.selectionPriority");
        QVERIFY(select && menu && priority);
        auto choose=[&](const QString& mode){
            QAction* submenuAction=nullptr;for(auto* item:menu->actions())if(item->menu()==priority)submenuAction=item;
            QVERIFY(submenuAction);
            auto* action=window.findChild<QAction*>("solid.action.selectionPriority."+mode);QVERIFY(action);
            bool menuVisible=false,priorityVisible=false;
            // InstantPopup runs a nested menu event loop during mousePress.
            QTimer::singleShot(80,&window,[&]{
                menuVisible=menu->isVisible();if(!menuVisible)return;
                QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,menu->actionGeometry(submenuAction).center());
                QTimer::singleShot(80,&window,[&]{
                    priorityVisible=priority->isVisible();
                    if(priorityVisible)QTest::mouseClick(priority,Qt::LeftButton,Qt::NoModifier,priority->actionGeometry(action).center());
                });
            });
            QTest::mouseClick(select,Qt::LeftButton);
            QVERIFY(menuVisible && priorityVisible);
            QCOMPARE(action->isChecked(),window.state()["ui"].toObject()["selectionPriority"]==mode);
        };
        choose("edge");
        QCOMPARE(window.state()["ui"].toObject()["selectionPriority"].toString(),QString("edge"));
        for(const auto& id:{topId,bottomId}){
            const auto pixel=call(window,"view.projectEdge",{{"bodyId",bodyId},{"edgeId",id}});
            QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,QPoint(qRound(pixel["x"].toDouble()),qRound(pixel["y"].toDouble())));
            QTRY_COMPARE(window.state()["selection"].toObject()["edgeId"].toString(),id);
            QCOMPARE(window.state()["selection"].toObject()["kind"].toString(),QString("edge"));
        }
        call(window,"capture.screenshot",{{"path",evidence+"/native-edge-priority.png"}});
        choose("face");
        QCOMPARE(window.state()["ui"].toObject()["selectionPriority"].toString(),QString("face"));
        const auto front=body["supportFaces"].toArray();QJsonObject support;
        for(const auto& value:front)if(value.toObject()["role"]=="box.y0"){
            const auto row=value.toObject();support={{"bodyId",row["bodyId"]},{"producerFeatureId",row["producerFeatureId"]},{"faceId",row["faceId"]}};
        }
        QVERIFY(!support.isEmpty());
        const auto interior=call(window,"view.projectFacePoint",{{"support",support},{"uMm",25},{"vMm",20}});
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,QPoint(qRound(interior["x"].toDouble()),qRound(interior["y"].toDouble())));
        QTRY_COMPARE(window.state()["selection"].toObject()["kind"].toString(),QString("face"));
        choose("face");
        QCOMPARE(window.state()["ui"].toObject()["selectionPriority"].toString(),QString("auto"));
        QCOMPARE(call(window,"document.inspect")["authored"],initial["authored"]);
        window.hide();
    }
};
QTEST_MAIN(SelectionPriorityControlsTest)
#include "selection_priority_controls.moc"
