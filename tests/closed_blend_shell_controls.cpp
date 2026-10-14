// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include <QApplication>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QJsonDocument>
#include <QPushButton>
#include <QTest>
#include <QToolButton>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QUuid>

class ClosedBlendShellControlsTest final:public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& args={}){
        const auto response=window.execute(command,args);
        if(!response["ok"].toBool())qFatal("%s: %s",qPrintable(command),QJsonDocument(response).toJson().constData());
        return response["result"].toObject();
    }
private slots:
    void centeredPrismBodySelectorAndCancel(){
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1300,850);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));window.activateWindow();
        call(window,"sketch.begin",{{"plane","XY"}});
        call(window,"sketch.centerRectangle",{{"centerX",0},{"centerY",0},{"width",93},{"height",64},{"units","mm"},
            {"centerSnapTarget",QJsonObject{{"kind","origin"}}}});
        const auto sketch=window.state()["activeSession"].toObject()["draftSketch"].toObject();
        call(window,"sketch.finish");
        const auto profile=call(window,"document.inspect")["evaluation"].toObject()["sketches"].toArray()[0].toObject()
            ["profiles"].toArray()[0].toObject()["profileId"].toString();
        call(window,"solid.extrude",{{"sketchId",sketch["id"]},{"profileId",profile},{"distance",30},{"units","mm"}});
        const auto stock=call(window,"document.inspect")["evaluation"].toObject()["bodies"].toArray()[0].toObject();
        QJsonArray vertical,rims;
        for(const auto& value:stock["solidEdges"].toArray()){
            const auto edge=value.toObject();const auto role=edge["provenance"].toObject()["role"].toString();
            (role=="sideCorner"?vertical:rims).append(edge["edgeId"]);
        }
        QCOMPARE(vertical.size(),4);QCOMPARE(rims.size(),8);
        const auto group=[](const QJsonArray& edges,double radius){return QJsonObject{
            {"id",QUuid::createUuid().toString(QUuid::WithoutBraces)},
            {"edgeIds",edges},{"radiusMm",radius},{"tangentChain",true}};};
        call(window,"solid.fillet",{{"targetBodyId",stock["bodyId"]},
            {"groups",QJsonArray{group(vertical,5),group(rims,2.5)}}});
        const auto before=call(window,"document.inspect").value("authored").toObject();
        auto* launch=window.findChild<QToolButton*>("command.solid.shellDialog");QVERIFY(launch);
        QTest::mouseClick(launch,Qt::LeftButton);
        QCOMPARE(window.state()["activeSession"].toObject()["command"].toString(),QString("solid.shell"));
        auto* tree=window.findChild<QTreeWidget*>("browser.tree");QVERIFY(tree);
        QTreeWidgetItem* item=nullptr;
        for(QTreeWidgetItemIterator it(tree);*it;++it)
            if((*it)->data(0,Qt::UserRole+1).toString()==stock["bodyId"].toString()){item=*it;break;}
        QVERIFY(item);for(auto* parent=item->parent();parent;parent=parent->parent())parent->setExpanded(true);
        tree->scrollToItem(item);QApplication::processEvents();
        const auto rect=tree->visualItemRect(item);QVERIFY(rect.isValid());
        QTest::mouseClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,rect.center());
        auto* dock=window.findChild<QDockWidget*>("shellDock");QVERIFY(dock);
        auto* thickness=dock->findChild<QDoubleSpinBox*>("shellThickness");QVERIFY(thickness);
        QTest::mouseClick(thickness,Qt::LeftButton);QVERIFY(thickness->hasFocus());thickness->selectAll();
        QTest::keyClicks(thickness,"3.5");QTest::keyClick(thickness,Qt::Key_Tab);
        QTRY_VERIFY(window.state()["activeSession"].toObject()["geometryEvaluated"].toBool());
        QCOMPARE(window.state()["activeSession"].toObject()["geometry"].toObject()
            ["topology"].toObject()["shells"].toInt(),2);
        const auto evidence=qEnvironmentVariable("ALLOY_CLOSED_BLEND_EVIDENCE");
        if(!evidence.isEmpty())call(window,"capture.screenshot",{{"path",evidence+"/native-rounded-shell.png"}});
        auto* buttons=dock->findChild<QDialogButtonBox*>();QVERIFY(buttons);
        QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel),Qt::LeftButton);
        QCOMPARE(call(window,"document.inspect").value("authored").toObject(),before);
        QVERIFY(window.state()["activeSession"].toObject().isEmpty());
        call(window,"document.new",{{"discard",true}});
        window.close();
    }
};
QTEST_MAIN(ClosedBlendShellControlsTest)
#include "closed_blend_shell_controls.moc"
