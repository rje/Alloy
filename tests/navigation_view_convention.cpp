// SPDX-License-Identifier: MIT
#include "AlloyTheme.h"
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QLabel>
#include <QMenu>
#include <QContextMenuEvent>
#include <QTest>
#include <QTimer>
#include <QTreeWidget>
#include <cmath>

class NavigationViewConvention final : public QObject {
    Q_OBJECT
    static QJsonObject call(AlloyWindow& window,const QString& name,const QJsonObject& args={}) {
        const auto response=window.executeApi(name,args);
        if(!response.value("ok").toBool())qFatal("%s",QJsonDocument(response).toJson(QJsonDocument::Compact).constData());
        return response.value("result").toObject();
    }
    static QJsonArray direction(AlloyWindow& window) {
        return window.state().value("viewport").toObject().value("gpu").toObject()
            .value("camera").toObject().value("direction").toArray();
    }
    static QJsonArray up(AlloyWindow& window) {
        return window.state().value("viewport").toObject().value("gpu").toObject()
            .value("camera").toObject().value("up").toArray();
    }
    static void nearVector(const QJsonArray& actual,const QJsonArray& expected,double sign=1) {
        QCOMPARE(actual.size(),3);QCOMPARE(expected.size(),3);
        for(int i=0;i<3;++i)QVERIFY(std::abs(actual[i].toDouble()-sign*expected[i].toDouble())<1e-6);
    }
private slots:
    void documentConventionAndCube() {
        const auto out=qEnvironmentVariable("ALLOY_NAV_CONVENTION_EVIDENCE");
        QVERIFY(!out.isEmpty());QVERIFY(QDir().mkpath(out));
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1360,850);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        ReferenceView* view=nullptr;for(auto* child:window.findChildren<QWidget*>())
            if(auto* found=dynamic_cast<ReferenceView*>(child))view=found;
        QVERIFY(view);
        auto* upLabel=view->findChild<QLabel*>("cube.upAxis");QVERIFY(upLabel);
        call(window,"solid.box",{{"width",47},{"depth",26},{"height",15},{"units","mm"}});
        const auto before=call(window,"document.inspect");
        const auto bodyId=before.value("evaluation").toObject().value("bodies").toArray().first().toObject().value("bodyId").toString();
        QVERIFY(!bodyId.isEmpty());
        call(window,"view.home");call(window,"view.homeSet");
        const auto savedHome=direction(window),savedHomeUp=up(window);
        const auto named=call(window,"view.namedCreate",{{"name","Reference Pose"}});
        const auto namedId=named.value("id").toString();QVERIFY(!namedId.isEmpty());
        const auto beforeUp=direction(window),beforeUpVector=up(window);
        call(window,"view.conventionUp",{{"axis","Y"}});
        QCOMPARE(direction(window),beforeUp);QCOMPARE(up(window),beforeUpVector);
        QCOMPARE(upLabel->text(),QString("UP Y"));
        const auto yFrame=call(window,"view.conventionInspect");
        QCOMPARE(yFrame.value("defaultUp"),QJsonValue("Y"));
        for(const auto* face:{"top","bottom","front","back","left","right"}){
            call(window,"view.orientation",{{"name",face}});
            const auto local=call(window,"view.conventionInspect");
            const auto d=direction(window);
            if(QString::fromUtf8(face)=="front")nearVector(d,local.value("back").toArray());
            if(QString::fromUtf8(face)=="top")nearVector(d,local.value("up").toArray(),-1);
            QCOMPARE(window.state().value("viewport").toObject().value("navigation").toObject()
                .value("resolvedProjection"),QJsonValue("orthographic"));
        }
        call(window,"capture.screenshot",{{"path",out+"/y-up-cube.png"}});
        call(window,"view.orbitMode",{{"mode","constrained"}});
        call(window,"view.orientation",{{"name","isometric"}});
        call(window,"view.orbit",{{"deltaX",12},{"deltaY",8}});
        const auto levelUp=up(window),frameUp=yFrame.value("up").toArray();
        double alignment=0;for(int i=0;i<3;++i)alignment+=levelUp[i].toDouble()*frameUp[i].toDouble();
        QVERIFY(alignment>0);
        call(window,"view.orbitMode",{{"mode","free"}});
        call(window,"view.orientation",{{"name","isometric"}});
        call(window,"view.orbit",{{"deltaX",27},{"deltaY",13}});
        const auto obliqueDirection=direction(window),obliqueUp=up(window);
        const auto authoredBefore=call(window,"document.inspect").value("authored").toObject().value("features");
        auto* tree=window.findChild<QTreeWidget*>("browser.tree");QVERIFY(tree);
        QTreeWidgetItem* frontRow=nullptr;
        for(auto* item:tree->findItems("Front",Qt::MatchExactly|Qt::MatchRecursive))
            if(item->parent() && item->parent()->text(0)=="Named Views")frontRow=item;
        QVERIFY(frontRow);tree->scrollToItem(frontRow);
        const auto browserPixel=tree->visualItemRect(frontRow).center();
        QVERIFY(tree->viewport()->rect().contains(browserPixel));
        bool menuHit=false;
        QTimer::singleShot(50,&window,[&]{
            auto* menu=qobject_cast<QMenu*>(QApplication::activePopupWidget());if(!menu)return;
            QAction* action=nullptr;for(auto* row:menu->actions())if(row->objectName()=="context.conventionSetFront")action=row;
            if(!action || !action->isEnabled())return;
            const auto hit=menu->actionGeometry(action).center();
            menuHit=QApplication::widgetAt(menu->mapToGlobal(hit))==menu;
            if(menuHit)QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,hit);
        });
        QTimer::singleShot(2000,&window,[]{if(auto* popup=QApplication::activePopupWidget())popup->close();});
        QContextMenuEvent menuEvent(QContextMenuEvent::Mouse,browserPixel,tree->viewport()->mapToGlobal(browserPixel));
        QApplication::sendEvent(tree->viewport(),&menuEvent);
        QVERIFY(menuHit);QVERIFY(QApplication::activePopupWidget()==nullptr);
        nearVector(direction(window),obliqueDirection);nearVector(up(window),obliqueUp);
        QCOMPARE(upLabel->text(),QString("UP CUSTOM"));
        const auto custom=call(window,"view.conventionInspect");
        QVERIFY(custom!=yFrame);
        call(window,"view.orientation",{{"name","front"}});
        nearVector(direction(window),obliqueDirection);nearVector(up(window),obliqueUp);
        call(window,"capture.screenshot",{{"path",out+"/custom-front-cube.png"}});
        QPoint cubeFace(-1,-1);QString cubeRegion;
        for(int y=2;y<std::min(view->height(),180)&&cubeFace.x()<0;y+=3)
            for(int x=std::max(0,view->width()-180);x<view->width()&&cubeFace.x()<0;x+=3){
                const auto hit=view->cubeHit(x,y);
                if(hit.value("kind")=="face" && QApplication::widgetAt(view->mapToGlobal(QPoint(x,y)))==view){
                    cubeFace={x,y};cubeRegion=hit.value("region").toString();
                }
            }
        QVERIFY(cubeFace.x()>=0);QVERIFY(!cubeRegion.isEmpty());
        QCOMPARE(QApplication::widgetAt(view->mapToGlobal(cubeFace)),static_cast<QWidget*>(view));
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,cubeFace);
        const auto clicked=direction(window);
        call(window,"view.cubeOrient",{{"region",cubeRegion}});nearVector(direction(window),clicked);
        call(window,"view.home");nearVector(direction(window),savedHome);nearVector(up(window),savedHomeUp);
        call(window,"view.namedRestore",{{"viewId",namedId}});
        nearVector(direction(window),savedHome);nearVector(up(window),savedHomeUp);
        call(window,"view.orientation",{{"name","isometric"}});call(window,"view.orbit",{{"deltaX",9},{"deltaY",7}});
        const auto topDirection=direction(window),topUp=up(window);
        call(window,"view.conventionSet",{{"face","top"}});
        nearVector(direction(window),topDirection);nearVector(up(window),topUp);
        const auto topFrame=call(window,"view.conventionInspect");
        call(window,"view.orientation",{{"name","top"}});
        nearVector(direction(window),topDirection);nearVector(up(window),topUp);
        const auto projected=call(window,"view.projectBody",{{"bodyId",bodyId}});
        QCOMPARE(view->pickBody(qRound(projected.value("x").toDouble()),qRound(projected.value("y").toDouble())),bodyId);
        QCOMPARE(call(window,"document.inspect").value("authored").toObject().value("features"),authoredBefore);
        const auto file=out+"/convention.alloy";call(window,"document.save",{{"path",file}});
        call(window,"view.conventionReset");QCOMPARE(call(window,"view.conventionInspect"),yFrame);
        call(window,"history.undo");QCOMPARE(call(window,"view.conventionInspect"),topFrame);
        call(window,"history.redo");QCOMPARE(call(window,"view.conventionInspect"),yFrame);
        call(window,"document.open",{{"path",file},{"discard",true}});
        QCOMPARE(call(window,"view.conventionInspect"),topFrame);
        QFile source(file);QVERIFY(source.open(QIODevice::ReadOnly));
        auto malformed=QJsonDocument::fromJson(source.readAll()).object();source.close();
        auto broken=malformed.value("viewConvention").toObject();broken["right"]=QJsonArray{-1,0,0};
        malformed["viewConvention"]=broken;
        const auto bad=out+"/reflected-convention.alloy";QFile destination(bad);QVERIFY(destination.open(QIODevice::WriteOnly));
        destination.write(QJsonDocument(malformed).toJson());destination.close();
        QVERIFY(!window.executeApi("document.open",{{"path",bad},{"discard",true}}).value("ok").toBool());
        QCOMPARE(call(window,"view.conventionInspect"),topFrame);
        window.close();
    }
};
QTEST_MAIN(NavigationViewConvention)
#include "navigation_view_convention.moc"
