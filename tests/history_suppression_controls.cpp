// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include <QApplication>
#include <QMenu>
#include <QPushButton>
#include <QTest>
#include <QToolButton>
#include <QJsonDocument>
#include <QElapsedTimer>
#include <QTimer>
#include <QWindow>
#include <QFile>
#include <QDir>
#include <QPointer>
#include <cmath>
#include <iostream>
#include <stdexcept>
static void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
static bool waitFor(const std::function<bool()>& ready){QElapsedTimer t;t.start();while(!ready() && t.elapsed()<2500)QTest::qWait(25);return ready();}
int main(int argc,char** argv){QApplication app(argc,argv);AlloyTheme::apply(app);try{
    AlloyWindow window;window.resize(1400,900);window.show();window.activateWindow();
    // Initial harness activation only. Never repair activation between user steps.
    check(QTest::qWaitForWindowExposed(&window),"Window not exposed");
    window.raise();window.windowHandle()->requestActivate();
    if(!waitFor([&]{return window.isActiveWindow() && window.windowHandle()->isActive() && QApplication::focusWidget();})){
        std::cerr<<"UNQUALIFIED initial native activation unavailable\n";return 77;
    }
    auto call=[&](const QString& id,const QJsonObject& args=QJsonObject{}){auto r=window.execute(id,args);check(r.value("ok").toBool(),QJsonDocument(r).toJson().constData());return r.value("result").toObject();};
    call("solid.box",{{"width",20},{"depth",15},{"height",10},{"units","mm"}});
    const auto stock=call("document.inspect");
    const auto body=stock.value("evaluation").toObject().value("bodies").toArray().first().toObject();
    QJsonObject face;
    for(const auto& v:body.value("supportFaces").toArray())if(v.toObject().value("role")=="box.z1") {
        const auto f=v.toObject();for(const auto* key:{"producerFeatureId","bodyId","faceId"})face.insert(key,f.value(key));
    }
    call("solid.faceOffset",{{"targetBodyId",body.value("bodyId")},{"face",face},{"distanceMm",5}});
    call("view.fit");
    const auto out=qEnvironmentVariable("ALLOY_SUPPRESSION_EVIDENCE");check(!out.isEmpty(),"Evidence directory required");QDir().mkpath(out);
    auto record=[&](const QString& name,const QJsonObject& value){QFile f(out+"/"+name+".json");check(f.open(QIODevice::WriteOnly),"Evidence write failed");f.write(QJsonDocument(value).toJson());};
    auto capture=[&](const QString& name){call("capture.screenshot",{{"path",out+"/"+name+".png"}});};
    call("solid.box",{{"width",8},{"depth",6},{"height",4},{"x",40},{"units","mm"},{"name","Box B"}});call("view.fit");
    const auto before=call("document.inspect");const auto features=before.value("authored").toObject().value("features").toArray();
    const auto id=features.first().toObject().value("id").toString();
    auto nativeAction=[&](const QString& label){
        QTest::qWait(100);QPointer<QToolButton> tile=window.findChild<QToolButton*>("timeline.feature."+id);check(tile,"Timeline tile absent");
        const auto arrowGlobal=tile->mapToGlobal(tile->rect().center());check(QApplication::widgetAt(arrowGlobal)==tile,"Tile actual hit mismatch");
        bool clicked=false,ready=false;
        QTimer::singleShot(150,&window,[&]{if(!tile)return;auto* menu=tile->menu();ready=menu && menu->isVisible() && QApplication::activePopupWidget()==menu;if(!ready)return;
            menu->grab().save(out+"/"+label+"-menu.png");QAction* target=nullptr;for(auto* action:menu->actions())if(action->text()==label)target=action;if(!target)return;
            const auto global=menu->mapToGlobal(menu->actionGeometry(target).center());auto* actual=QApplication::widgetAt(global);if(actual!=menu)return;
            clicked=true;QTest::mouseClick(actual,Qt::LeftButton,Qt::NoModifier,actual->mapFromGlobal(global));});
        QTest::mouseClick(tile,Qt::RightButton,Qt::NoModifier,tile->mapFromGlobal(arrowGlobal));
        check(waitFor([&]{return clicked;}),"Action not clicked");check(ready,"Popup readiness");check(waitFor([&]{return !QApplication::activePopupWidget();}),"Popup closure");
        check(window.isActiveWindow() && QApplication::focusWidget(),"Focus lost");
    };
    capture("native-full");nativeAction("Suppress Features");
    auto suppressed=call("document.inspect");check(suppressed.value("bodies").toInt()==1,"Suppressed body count");
    auto state=call("app.state");const auto tiles=state.value("ui").toObject().value("timeline").toObject().value("features").toArray();
    check(tiles[0].toObject().value("explicitSuppressed").toBool() && tiles[1].toObject().value("effectiveSuppressed").toBool() && !tiles[2].toObject().value("effectiveSuppressed").toBool(),"Dimming state");
    record("suppressed",suppressed);record("suppressed-state",state);capture("native-suppressed");nativeAction("Unsuppress Features");
    const auto restored=call("document.inspect");check(restored.value("bodies").toInt()==2 && restored.value("evaluation").toObject().value("bodies")==before.value("evaluation").toObject().value("bodies"),"Restored geometry");
    record("restored",restored);capture("native-restored");std::cout<<"PASS actual-hit right-click Suppress/Unsuppress Timeline action, active popup/closure and restored geometry\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
