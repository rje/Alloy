// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QAction>
#include <QApplication>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QJsonDocument>
#include <QLabel>
#include <QPushButton>
#include <QTest>
#include <QTreeWidget>
#include <cmath>

class SplitBodyControlsTest final:public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& args={}){
        const auto response=window.execute(command,args);
        if(!response["ok"].toBool())qFatal("%s: %s",qPrintable(command),QJsonDocument(response).toJson().constData());
        return response["result"].toObject();
    }
    static QJsonObject ref(const QJsonObject& face){return {{"producerFeatureId",face["producerFeatureId"]},
        {"bodyId",face["bodyId"]},{"faceId",face["faceId"]}};}
private slots:
    void nativeBodyThenBrowserPlaneCancelAndCommit(){
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1320,850);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));window.activateWindow();QApplication::setActiveWindow(&window);
        call(window,"solid.box",{{"width",50},{"depth",80},{"height",40},{"name","Enclosure"}});
        const auto inspected=call(window,"document.inspect");const auto body=inspected["evaluation"].toObject()["bodies"].toArray()[0].toObject();
        QJsonObject top,bottom;for(const auto& value:body["supportFaces"].toArray()){
            const auto face=value.toObject();if(face["role"]=="box.z1")top=ref(face);if(face["role"]=="box.z0")bottom=ref(face);
        }
        QVERIFY(!top.isEmpty()&&!bottom.isEmpty());
        const auto plane=call(window,"construction.midplane",{{"faces",QJsonArray{top,bottom}},{"name","Case Midplane"}})["planeId"].toString();
        const auto before=call(window,"document.inspect")["authored"].toObject();
        ReferenceView* view=nullptr;for(auto* child:window.findChildren<QWidget*>())if(auto* found=dynamic_cast<ReferenceView*>(child)){view=found;break;}
        QVERIFY(view);auto* tree=window.findChild<QTreeWidget*>("browser.tree");QVERIFY(tree);
        auto* launch=window.findChild<QAction*>("solid.action.solid.splitBodyDialog");QVERIFY(launch);
        auto pickPlane=[&]{
            QTreeWidgetItem* item=nullptr;
            for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole+2).toString()==plane){item=*it;break;}
            QVERIFY(item);tree->scrollToItem(item);const auto point=tree->visualItemRect(item).center();
            QTest::mouseClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,point);
        };
        auto run=[&](bool accept){
            launch->trigger();QTRY_COMPARE(window.state()["activeSession"].toObject()["command"].toString(),QString("solid.splitBody"));
            auto* dock=window.findChild<QDockWidget*>("splitBodyDock");QVERIFY(dock&&dock->isVisible());
            auto* buttons=dock->findChild<QDialogButtonBox*>("split.buttons");QVERIFY(buttons);
            QVERIFY(!buttons->button(QDialogButtonBox::Ok)->isEnabled());
            const auto pixel=call(window,"view.projectBody",{{"bodyId",body["bodyId"]}});
            const QPoint bodyPoint(qRound(pixel["x"].toDouble()),qRound(pixel["y"].toDouble()));
            QVERIFY(view->hasFocus());QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,bodyPoint);
            auto session=window.state()["activeSession"].toObject();
            QCOMPARE(session["inputs"].toObject()["targetBodyId"],body["bodyId"]);
            QCOMPARE(session["selectionRole"].toString(),QString("plane"));
            pickPlane();QTRY_VERIFY(buttons->button(QDialogButtonBox::Ok)->isEnabled());
            session=window.state()["activeSession"].toObject();
            QCOMPARE(session["inputs"].toObject()["planeId"].toString(),plane);QVERIFY(session["previewed"].toBool());
            QCOMPARE(dock->findChild<QLabel*>("split.body")->text(),QString("Body to Split: Enclosure"));
            QCOMPARE(dock->findChild<QLabel*>("split.plane")->text(),QString("Splitting Tool: Case Midplane"));
            auto* clearPlane=dock->findChild<QPushButton*>("split.clearPlane");QVERIFY(clearPlane);
            QTest::mouseClick(clearPlane,Qt::LeftButton);QVERIFY(!buttons->button(QDialogButtonBox::Ok)->isEnabled());
            QCOMPARE(window.state()["activeSession"].toObject()["inputs"].toObject()["planeId"].toString(),QString());
            pickPlane();QTRY_VERIFY(buttons->button(QDialogButtonBox::Ok)->isEnabled());
            const auto evidence=qEnvironmentVariable("ALLOY_SPLIT_BODY_EVIDENCE");
            if(!evidence.isEmpty())call(window,"capture.screenshot",{{"path",evidence+(accept?"/split-native-commit-preview.png":"/split-native-cancel-preview.png")}});
            QTest::mouseClick(buttons->button(accept?QDialogButtonBox::Ok:QDialogButtonBox::Cancel),Qt::LeftButton);
            QTRY_VERIFY(window.state()["activeSession"].toObject().isEmpty());
        };
        run(false);QCOMPARE(call(window,"document.inspect")["authored"].toObject(),before);
        run(true);const auto after=call(window,"document.inspect");
        QCOMPARE(after["evaluation"].toObject()["bodies"].toArray().size(),2);
        const auto split=after["authored"].toObject()["features"].toArray().last().toObject();
        QCOMPARE(split["type"].toString(),QString("splitBody"));
        const auto edit=call(window,"feature.editDialog",{{"featureId",split["id"]}});
        QVERIFY(edit["previewed"].toBool());
        call(window,"command.cancel",{{"session",edit["id"]}});
        QCOMPARE(call(window,"document.inspect")["authored"].toObject(),after["authored"].toObject());
        call(window,"document.new",{{"discard",true}});window.close();
    }
};
QTEST_MAIN(SplitBodyControlsTest)
#include "split_body_controls.moc"
