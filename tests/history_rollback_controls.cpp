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
    const auto out=qEnvironmentVariable("ALLOY_ROLLBACK_EVIDENCE");check(!out.isEmpty(),"Evidence directory required");QDir().mkpath(out);
    auto record=[&](const QString& name,const QJsonObject& value){QFile f(out+"/"+name+".json");check(f.open(QIODevice::WriteOnly),"Evidence write failed");f.write(QJsonDocument(value).toJson());};
    auto capture=[&](const QString& name){call("capture.screenshot",{{"path",out+"/"+name+".png"}});};
    std::cout<<"CHECKPOINT fixture ready"<<std::endl;
    const auto before=call("document.inspect");const auto features=before.value("authored").toObject().value("features").toArray();
    const auto full=before.value("evaluation").toObject().value("bodies").toArray().first().toObject();
    check(std::abs(full.value("volumeMm3").toDouble()-4500)<1e-6,"Latest geometry mismatch");
    record("before",before);capture("native-latest");
    const auto id=features.last().toObject().value("id").toString();QTest::qWait(100);
    QPointer<QToolButton> tile=window.findChild<QToolButton*>("timeline.feature."+id);check(tile,"Timeline tile absent");
    const auto arrowGlobal=tile->mapToGlobal(QPoint(tile->width()-6,tile->height()/2));
    auto* hit=QApplication::widgetAt(arrowGlobal);check(hit==tile,"Timeline arrow actual hit mismatch");
    // Opening a Qt popup can enter its event loop until the queued action click.
    bool actionClicked=false;bool popupReady=false;
    QTimer::singleShot(150,&window,[&]{if(!tile)return;auto* menu=tile->menu();popupReady=menu && menu->isVisible() && QApplication::activePopupWidget()==menu;
        if(!popupReady)return;menu->grab().save(out+"/native-popup.png");QAction* target=nullptr;for(auto* a:menu->actions())if(a->text()=="Roll History Marker Here")target=a;
        if(!target)return;const auto point=menu->actionGeometry(target).center();const auto global=menu->mapToGlobal(point);auto* actual=QApplication::widgetAt(global);
        if(actual!=menu)return;std::cout<<"CHECKPOINT popup active; action actual hit"<<std::endl;actionClicked=true;QTest::mouseClick(actual,Qt::LeftButton,Qt::NoModifier,actual->mapFromGlobal(global));});
    // The press opens a nested popup loop. Its action rebuilds the Timeline,
    // deleting the original tile before the outer click's release. Deliver via
    // the surviving top-level QWindow so Qt routes the release to the current
    // actual hit, rather than retaining a deleted QWidget between events.
    QTest::mouseClick(window.windowHandle(),Qt::LeftButton,Qt::NoModifier,window.windowHandle()->mapFromGlobal(arrowGlobal));
    std::cout<<"CHECKPOINT outer click returned; original tile alive="<<bool(tile)<<std::endl;
    check(waitFor([&]{return actionClicked;}),"Popup not natively ready or action actual hit unavailable");
    check(popupReady,"Popup activation not observed");
    check(waitFor([&]{return !QApplication::activePopupWidget();}),"Popup did not close");
    check(window.isActiveWindow() && QApplication::focusWidget(),"Native return activation/focus unavailable");
    std::cout<<"CHECKPOINT native menu action and popup closure"<<std::endl;
    auto rolled=call("document.inspect");check(rolled.value("rollback").toObject().value("active").toBool(),"Native marker action failed");
    check(rolled.value("authored")==before.value("authored") && rolled.value("bodies").toInt()==1,"Native marker changed authored data/prefix wrong");
    const auto prefix=rolled.value("evaluation").toObject().value("bodies").toArray().first().toObject();
    check(prefix.value("bodyId")==body.value("bodyId") && std::abs(prefix.value("volumeMm3").toDouble()-3000)<1e-6,"Prefix geometry mismatch");
    for(const auto* key:{"revision","dirty","undo"})check(rolled.value(key)==before.value(key),"Rollback changed authoring/history state");
    const auto state=call("app.state");const auto tiles=state.value("ui").toObject().value("timeline").toObject().value("features").toArray();
    check(tiles.size()==2 && tiles.first().toObject().value("evaluated").toBool() && !tiles.last().toObject().value("evaluated").toBool(),"Inactive tile state wrong");
    const auto gpu=state.value("viewport").toObject().value("gpu").toObject();
    check(gpu.value("hardwareAccelerated").toBool() && gpu.value("selectedSupportFace").toObject().isEmpty(),"GPU or selection state wrong");
    record("prefix",rolled);record("prefix-state",state);capture("native-prefix");
    const auto refusal=window.execute("feature.edit",{{"featureId",features.first().toObject().value("id")},{"parameters",QJsonObject{{"widthMm",24}}}});
    check(!refusal.value("ok").toBool() && refusal.value("error").toObject().value("code")=="history_rollback_read_only","Read-only refusal absent");record("refusal",refusal);
    call("view.zoom",{{"factor",1.1}});
    const auto navigated=call("app.state");
    check(navigated.value("viewport").toObject().value("gpu").toObject().value("camera")!=gpu.value("camera"),"Camera navigation did not move");
    record("navigated-state",navigated);call("view.home");
    check(call("document.inspect").value("authored")==before.value("authored"),"Navigation/refusal changed authoring");
    check(window.isActiveWindow() && window.windowHandle()->isActive() && QApplication::focusWidget(),"Focus/activation lost before Move to End");
    auto* end=window.findChild<QPushButton*>("timeline.returnToLatest");check(end,"Move to End absent");
    const auto global=end->mapToGlobal(end->rect().center());auto* actual=QApplication::widgetAt(global);check(actual==end,"Move to End actual hit mismatch");
    QTest::mouseClick(actual,Qt::LeftButton,Qt::NoModifier,actual->mapFromGlobal(global));
    auto restored=call("document.inspect");check(!restored.value("rollback").toObject().value("active").toBool() && restored.value("bodies").toInt()==1,"Native Move to End failed");
    check(restored.value("authored")==before.value("authored") && restored.value("undo")==before.value("undo"),"Native marker changed history");
    check(restored.value("evaluation")==before.value("evaluation"),"Full geometry identity not restored");
    for(const auto* key:{"revision","dirty"})check(restored.value(key)==before.value(key),"Return changed authoring state");
    record("restored",restored);const auto finalState=call("app.state");
    check(finalState.value("viewport").toObject().value("gpu").toObject().value("selectedSupportFace").toObject().isEmpty(),"Stale face highlight after return");
    record("restored-state",finalState);capture("native-restored");
    std::cout<<"PASS actual-hit Timeline arrow/action and Move to End; popup activation/closure; authored/history preserved\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
