#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QAction>
#include <QApplication>
#include <QListWidget>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QJsonDocument>
#include <QPushButton>
#include <QTest>

class GroupedFilletControlsTest final:public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& args={}){
        const auto response=window.execute(command,args);
        if(!response["ok"].toBool())qFatal("%s: %s",qPrintable(command),QJsonDocument(response).toJson().constData());
        return response["result"].toObject();
    }
private slots:
    void nativeSelectionRowsAndRadii(){
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1320,850);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));window.activateWindow();
        call(window,"solid.box",{{"width",50},{"depth",80},{"height",40},{"name","Enclosure stock"}});
        const auto source=call(window,"document.inspect");
        const auto body=source["evaluation"].toObject()["bodies"].toArray()[0].toObject();
        QJsonObject vertical,top;
        for(const auto& value:body["solidEdges"].toArray()){
            const auto edge=value.toObject();const auto semantic=edge["provenance"].toObject()["boxEdge"].toString();
            if(semantic=="x1y0z*")vertical=edge;
            if(semantic=="x*y0z1")top=edge;
        }
        QVERIFY(!vertical.isEmpty() && !top.isEmpty());
        ReferenceView* viewport=nullptr;for(auto* child:window.findChildren<QWidget*>())
            if(auto* found=dynamic_cast<ReferenceView*>(child)){viewport=found;break;}QVERIFY(viewport);
        const auto clickEdge=[&](const QJsonObject& edge){
            const auto pixel=call(window,"view.projectEdge",{{"bodyId",body["bodyId"]},{"edgeId",edge["edgeId"]}});
            QTest::mouseClick(viewport,Qt::LeftButton,Qt::NoModifier,QPoint(qRound(pixel["x"].toDouble()),qRound(pixel["y"].toDouble())));
        };
        auto action=window.findChild<QAction*>("solid.action.solid.filletDialog");QVERIFY(action);action->trigger();
        auto* dock=window.findChild<QDockWidget*>("filletDock");QVERIFY(dock && dock->isVisible());
        auto* radius=dock->findChild<QDoubleSpinBox*>("filletRadius");
        auto* add=dock->findChild<QPushButton*>("fillet.addGroup");
        auto* remove=dock->findChild<QPushButton*>("fillet.removeGroup");
        auto* groups=dock->findChild<QListWidget*>("fillet.groups");
        QVERIFY(radius && add && remove && groups);
        QTest::mouseClick(radius,Qt::LeftButton);QTest::keyClick(radius,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(radius,"5");QTest::keyClick(radius,Qt::Key_Tab);
        clickEdge(vertical);
        auto inputs=window.state()["activeSession"].toObject()["inputs"].toObject();
        QCOMPARE(inputs["edgeIds"].toArray().size(),1);
        QTest::mouseClick(add,Qt::LeftButton);
        QCOMPARE(groups->count(),2);QCOMPARE(groups->currentRow(),1);
        QTest::mouseClick(radius,Qt::LeftButton);QTest::keyClick(radius,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(radius,"2.5");QTest::keyClick(radius,Qt::Key_Tab);
        clickEdge(top);
        auto session=window.state()["activeSession"].toObject();QVERIFY(session["previewed"].toBool());
        auto rows=session["inputs"].toObject()["groups"].toArray();QCOMPARE(rows.size(),2);
        QCOMPARE(rows[0].toObject()["radiusMm"].toDouble(),5.);QCOMPARE(rows[1].toObject()["radiusMm"].toDouble(),2.5);
        QCOMPARE(rows[1].toObject()["edgeIds"].toArray().size(),1);
        const auto secondId=rows[1].toObject()["id"];
        QTest::mouseClick(add,Qt::LeftButton);QCOMPARE(groups->count(),3);
        QTest::mouseClick(remove,Qt::LeftButton);QCOMPARE(groups->count(),2);
        QCOMPARE(window.state()["activeSession"].toObject()["inputs"].toObject()["groups"].toArray()[1].toObject()["id"],secondId);
        const auto evidence=qEnvironmentVariable("ALLOY_GROUPED_FILLET_EVIDENCE");
        if(!evidence.isEmpty())call(window,"capture.screenshot",{{"path",evidence+"/grouped-native-preview.png"}});
        auto* buttons=dock->findChild<QDialogButtonBox*>();QVERIFY(buttons);
        QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel),Qt::LeftButton);
        QCOMPARE(call(window,"document.inspect")["authored"].toObject(),source["authored"].toObject());
        const auto two=call(window,"solid.filletDialog",{{"targetBodyId",body["bodyId"]},{"groups",rows}});
        Q_UNUSED(two);
        dock=window.findChild<QDockWidget*>("filletDock");QVERIFY(dock);
        buttons=dock->findChild<QDialogButtonBox*>();QVERIFY(buttons->button(QDialogButtonBox::Ok)->isEnabled());
        QTest::mouseClick(buttons->button(QDialogButtonBox::Ok),Qt::LeftButton);
        const auto authored=call(window,"document.inspect")["authored"].toObject();
        QCOMPARE(authored["features"].toArray().last().toObject()["parameters"].toObject()["groups"].toArray(),rows);
        const auto featureId=authored["features"].toArray().last().toObject()["id"];
        call(window,"solid.filletDialog",{{"featureId",featureId}});
        dock=window.findChild<QDockWidget*>("filletDock");QVERIFY(dock);
        groups=dock->findChild<QListWidget*>("fillet.groups");radius=dock->findChild<QDoubleSpinBox*>("filletRadius");
        QVERIFY(groups && radius);QCOMPARE(groups->count(),2);QCOMPARE(radius->value(),5.);
        const auto secondRow=groups->visualItemRect(groups->item(1)).center();
        QTest::mouseClick(groups->viewport(),Qt::LeftButton,Qt::NoModifier,secondRow);
        QCOMPARE(groups->currentRow(),1);QCOMPARE(radius->value(),2.5);
        buttons=dock->findChild<QDialogButtonBox*>();
        QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel),Qt::LeftButton);
        QCOMPARE(call(window,"document.inspect")["authored"].toObject(),authored);
        call(window,"document.new",{{"discard",true}});window.close();
    }
};
QTEST_MAIN(GroupedFilletControlsTest)
#include "grouped_fillet_controls.moc"
