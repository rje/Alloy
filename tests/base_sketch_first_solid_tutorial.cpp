// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFile>
#include <QJsonDocument>
#include <QMenu>
#include <QMouseEvent>
#include <QProcess>
#include <QPushButton>
#include <QTest>
#include <QToolButton>
#include <QWindow>
#include <cmath>
#include <stdexcept>

class BaseSketchFirstSolidTutorial final : public QObject {
    Q_OBJECT
    AlloyWindow* window_=nullptr;
    ReferenceView* view_=nullptr;
    QString output_;
    bool recording_=false;
    static void require(bool good,const QString& message){if(!good)throw std::runtime_error(message.toStdString());}
    QJsonObject call(const QString& command,const QJsonObject& args={}){
        const auto response=window_->executeApi(command,args);
        require(response.value("ok").toBool(),command+": "+QString::fromUtf8(QJsonDocument(response).toJson()));
        return response.value("result").toObject();
    }
    QJsonObject state() const{return window_->state();}
    QJsonObject session() const{return state().value("activeSession").toObject();}
    QJsonObject inspect(){return call("document.inspect");}
    void step(const QString& label){qInfo().noquote()<<label;if(recording_){call("recording.step",{{"text",label}});QTest::qWait(700);}}
    void capture(const QString& name){call("capture.screenshot",{{"path",output_+"/"+name+".png"}});}
    void move(QWidget* widget,QPoint local){const auto global=widget->mapToGlobal(local);
        QMouseEvent event(QEvent::MouseMove,QPointF(local),QPointF(global),Qt::NoButton,Qt::NoButton,Qt::NoModifier);
        QApplication::sendEvent(widget,&event);QTest::qWait(recording_?120:25);}
    void click(QWidget* widget,QPoint local){require(widget && widget->isVisible(),"Native click target visible");
        auto* actual=QApplication::widgetAt(widget->mapToGlobal(local));
        require(actual==widget || widget->isAncestorOf(actual),"Native click target owns pixel");
        move(widget,local);QTest::mouseClick(widget,Qt::LeftButton,Qt::NoModifier,local);QTest::qWait(recording_?200:25);}
    QPoint pixel(const QString& sketchId,double u,double v){const auto result=call("view.projectSketchPoint",{{"sketchId",sketchId},{"uMm",u},{"vMm",v}});
        const QPoint point(qRound(result.value("x").toDouble()),qRound(result.value("y").toDouble()));
        require(view_->rect().contains(point),"Projected source region is within viewport");return point;}
    void openPressPull(){
        auto* button=window_->findChild<QToolButton*>("solid.menu.modify");
        auto* menu=window_->findChild<QMenu*>("solid.commands.modify");
        auto* action=window_->findChild<QAction*>("solid.action.solid.pressPullDialog");
        require(button&&menu&&action&&menu->actions().contains(action),"Modify > Press Pull exists");
        // InstantPopup enters QMenu::exec on the mouse press in this Qt test. Open the
        // same menu nonblockingly; the Press Pull item receives an actual Qt click.
        menu->popup(button->mapToGlobal(QPoint(0,button->height())));
        require(QTest::qWaitFor([&]{return menu->isVisible();},2000),"Modify menu visible");
        click(menu,menu->actionGeometry(action).center());
        require(session().value("command")=="solid.extrude","Press Pull opens extrusion session");
    }
    QJsonArray pickThree(const QString& sketchId,const QJsonArray& expected){
        // Pick inside each cell, away from its authored center point and edge.
        const QList<QPair<double,double>> locations{{-7,5},{76.5,7},{35,-10}};
        QJsonArray selected;
        for(int i=0;i<locations.size();++i){
            require(session().value("command")=="solid.extrude","Press Pull owns profile click");
            const auto [u,v]=locations.at(i);click(view_,pixel(sketchId,u,v));
            const auto inputs=session().value("inputs").toObject();selected=inputs.value("profileIds").toArray();
            if(selected.isEmpty() && inputs.contains("profileId"))selected.append(inputs.value("profileId"));
            require(selected.size()==i+1,"Each native region click collects one distinct profile (pick "+QString::number(i+1)+")");
            require(expected.contains(selected.last()),"Native region belongs to the three retained sketch cells");
        }
        for(const auto& id:expected)require(selected.contains(id),"All three exact profile IDs selected");
        return selected;
    }
    void typeDistance(double mm){
        auto* spin=window_->findChild<QDoubleSpinBox*>("extrude.distance");require(spin&&spin->isVisible(),"Distance input visible");
        click(spin,spin->rect().center());require(QApplication::focusWidget()==spin || spin->isAncestorOf(QApplication::focusWidget()),
            "Distance input naturally focused");
        QTest::keyClick(spin,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(spin,QString::number(mm,'f',2),Qt::NoModifier,recording_?85:-1);
        QTest::keyClick(spin,Qt::Key_Tab);require(std::abs(session().value("inputs").toObject().value("distance").toDouble()-mm)<1e-6,
            "Native numeric entry reaches shared distance authority");
        require(session().value("previewed").toBool(),"Three-profile solid preview is valid");
    }
    void button(QDialogButtonBox::StandardButton which){
        auto* box=window_->findChild<QDialogButtonBox*>("extrude.buttons");require(box&&box->isVisible(),"Press Pull buttons visible");
        auto* target=box->button(which);require(target&&target->isEnabled(),"Press Pull target button enabled");click(target,target->rect().center());
        require(session().isEmpty(),"Press Pull closes after button click");
    }
    static double volume(const QJsonObject& state){return state.value("evaluation").toObject().value("bodies").toArray().first().toObject().value("volumeMm3").toDouble();}
private slots:
    void firstSolid(){
        output_=qEnvironmentVariable("ALLOY_BASE_SKETCH_SOLID_EVIDENCE");QVERIFY(!output_.isEmpty());QVERIFY(QDir().mkpath(output_));
        const QString input=qEnvironmentVariable("ALLOY_BASE_SKETCH_SOLID_INPUT");QVERIFY(!input.isEmpty());
        AlloyTheme::apply(*qApp);AlloyWindow window;window_=&window;window.resize(1500,950);window.show();QVERIFY(QTest::qWaitForWindowExposed(&window));
        for(auto* child:window.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(child))view_=candidate;QVERIFY(view_);
#ifdef Q_OS_MACOS
        QProcess activate;activate.start("osascript",{"-e",QString("tell application \"System Events\" to set frontmost of first application process whose unix id is %1 to true").arg(QCoreApplication::applicationPid())});QVERIFY(activate.waitForFinished(5000));QCOMPARE(activate.exitCode(),0);
#endif
        window.raise();window.activateWindow();window.windowHandle()->requestActivate();QApplication::setActiveWindow(&window);QTRY_VERIFY(window.isActiveWindow());
        try{
            call("document.open",{{"path",input},{"discard",true}});const auto initial=inspect();
            const auto sketch=initial.value("evaluation").toObject().value("sketches").toArray().first().toObject();
            const QString sketchId=sketch.value("sketchId").toString();const auto profiles=sketch.value("profiles").toArray();
            require(profiles.size()==3,"Accepted dimensioned checkpoint has exactly three source cells");
            QJsonArray expected;double area=0;for(const auto& value:profiles){expected.append(value.toObject().value("profileId"));area+=value.toObject().value("areaMm2").toDouble();}
            call("sketch.edit",{{"sketchId",sketchId}});call("sketch.lookAt");
            call("view.zoom",{{"factor",0.65}});const auto center=pixel(sketchId,35,-5);
            call("view.pan",{{"x",view_->width()/2-center.x()},{"y",view_->height()/2-center.y()}});
            if(qEnvironmentVariableIntValue("ALLOY_BASE_SKETCH_SOLID_RECORD")){
                call("recording.start",{{"path",output_+"/base-sketch-first-solid.mp4"},{"fps",15}});recording_=true;
            }
            step("Saved dimensioned Base Sketch · Stop Sketch");
            auto* finish=window.findChild<QPushButton*>("sketch.finish");require(finish&&finish->isVisible(),"Stop Sketch control visible");
            click(finish,finish->rect().center());require(session().isEmpty(),"Sketch finished without geometry changes");
            require(inspect().value("authored")==initial.value("authored"),"Editing and finishing unchanged sketch preserves authored geometry");
            step("Modify > Press Pull · select three retained sketch regions");openPressPull();const auto selected=pickThree(sketchId,expected);
            {int highlighted=0;for(const auto& row:state().value("viewport").toObject().value("gpu").toObject().value("finishedProfiles").toArray())
                if(row.toObject().value("selected").toBool())++highlighted;
                require(highlighted==3,"All three selected profile fills are highlighted before depth entry");}
            capture("three-selected-profiles");
            auto* extent=window.findChild<QComboBox*>("extrude.extent");auto* operation=window.findChild<QComboBox*>("extrude.operation");
            require(extent&&operation&&extent->currentData()=="distance"&&operation->currentData()=="newBody", "One Side Distance / New Body control state");
            step("48.25 mm · preview the full three-region solid");typeDistance(48.25);
            const auto preview=session().value("geometry").toObject();require(preview.value("volumeMm3").toDouble()>0,"Preview has positive kernel volume");
            capture("solid-preview");
            const auto beforeCancel=inspect();step("Alloy continuity · Cancel preview without authored change");button(QDialogButtonBox::Cancel);
            const auto canceled=inspect();require(canceled.value("authored")==beforeCancel.value("authored")&&canceled.value("undo")==beforeCancel.value("undo"),
                "Cancel leaves authored document and history unchanged");
            for(const auto& row:state().value("viewport").toObject().value("gpu").toObject().value("finishedProfiles").toArray())
                require(!row.toObject().value("selected").toBool(),"Cancel clears disposable blue profile selection");
            capture("selection-cleared-after-cancel");
            step("Modify > Press Pull · reselect three regions and accept 48.25 mm");openPressPull();require(pickThree(sketchId,expected)==selected,"Reselection preserves ordered source IDs");typeDistance(48.25);button(QDialogButtonBox::Ok);
            const auto accepted=inspect();const auto features=accepted.value("authored").toObject().value("features").toArray();
            require(features.size()==2 && features.last().toObject().value("type")=="extrude","One retained editable Extrude follows sketch");
            const auto feature=features.last().toObject(),inputs=feature.value("parameters").toObject();
            const auto retainedProfiles=inputs.value("profileIds").toArray();
            require(retainedProfiles.size()==selected.size() && [&]{for(const auto& id:selected)if(!retainedProfiles.contains(id))return false;return true;}()
                && std::abs(inputs.value("distanceMm").toDouble()-48.25)<1e-6,
                "Feature retains exact three profile IDs and depth");
            require(inputs.value("operation").toString("newBody")=="newBody" && inputs.value("extent").toString("distance")=="distance",
                "Feature retains New Body / Distance through its explicit-or-default fields");
            const auto bodies=accepted.value("evaluation").toObject().value("bodies").toArray();require(bodies.size()==1,"Three adjacent regions produce one solid body");
            const auto body=bodies.first().toObject();require(body.value("valid").toBool()&&body.value("closed").toBool()&&body.value("volumeMm3").toDouble()>0,
                "Kernel validates one positive closed solid");
            const double expectedVolume=area*48.25;require(std::abs(volume(accepted)-expectedVolume)<std::max(1e-4,expectedVolume*1e-7),
                "Solid volume agrees with current accepted three-cell sketch area times depth");
            call("view.home");capture("first-solid");
            step("Alloy continuity · edit distance, restore, undo/redo, save and reopen");
            const auto edit=call("feature.editDialog",{{"featureId",feature.value("id")}});
            call("command.update",{{"session",edit.value("id")},{"inputs",QJsonObject{{"distance",49.25}}}});
            call("command.preview",{{"session",edit.value("id")}});call("command.commit",{{"session",edit.value("id")}});
            require(std::abs(volume(inspect())-area*49.25)<std::max(1e-4,area*49.25*1e-7),"Distance edit recomputes solid");
            call("history.undo");require(inspect().value("authored")==accepted.value("authored"),"Undo restores 48.25 mm");
            call("history.redo");require(std::abs(volume(inspect())-area*49.25)<1e-3,"Redo restores edited distance");
            call("history.undo");require(inspect().value("authored")==accepted.value("authored"),"Restored source checkpoint before save");
            const QString saved=output_+"/base-sketch-first-solid.alloy";call("document.save",{{"path",saved}});
            call("document.open",{{"path",saved},{"discard",true}});const auto reopened=inspect();
            require(reopened.value("authored")==accepted.value("authored"),"Reopen preserves feature/profile references");
            require(std::abs(volume(reopened)-expectedVolume)<std::max(1e-4,expectedVolume*1e-7),"Reopen preserves solid volume");
            QFile result(output_+"/result.json");require(result.open(QIODevice::WriteOnly),"Can write result");
            result.write(QJsonDocument(QJsonObject{{"status","passed-pending-independent-review"},{"sourceInput",input},
                {"input","Actual Stop Sketch, Qt Modify menu item and three viewport profile picks, typed 48.25, Cancel/OK; harness opens real QMenu nonblockingly; API inspection/view/history/storage/edit"},
                {"selectedProfileIds",selected},{"sketchAreaMm2",area},{"expectedVolumeMm3",expectedVolume},
                {"authored",reopened.value("authored")},{"evaluation",reopened.value("evaluation")}}).toJson());result.close();
            if(recording_){QTest::qWait(1100);call("recording.stop");recording_=false;}window.close();
        }catch(const std::exception& error){if(recording_){window.executeApi("recording.stop",{});recording_=false;}
            window.executeApi("capture.screenshot",{{"path",output_+"/failure.png"}});QFAIL(error.what());}
    }
};
QTEST_MAIN(BaseSketchFirstSolidTutorial)
#include "base_sketch_first_solid_tutorial.moc"
