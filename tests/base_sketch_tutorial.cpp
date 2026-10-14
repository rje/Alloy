// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include "SketchGeometry.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMouseEvent>
#include <QProcess>
#include <QPushButton>
#include <QTest>
#include <QTimer>
#include <QToolButton>
#include <QWindow>
#include <cmath>
#include <stdexcept>

class BaseSketchTutorial final : public QObject {
    Q_OBJECT
    AlloyWindow* w_=nullptr;ReferenceView* view_=nullptr;QString out_;bool recording_=false;QPoint cursor_;
    static void require(bool value,const QString& why){if(!value)throw std::runtime_error(why.toStdString());}
    QJsonObject call(const QString& name,const QJsonObject& args={}){const auto r=w_->executeApi(name,args);require(r.value("ok").toBool(),name+": "+QString::fromUtf8(QJsonDocument(r).toJson()));return r.value("result").toObject();}
    QJsonObject session(){return w_->state().value("activeSession").toObject();}
    QJsonObject draft(){return session().value("draftSketch").toObject();}
    QJsonObject parameters(){return draft().value("parameters").toObject();}
    QJsonObject byId(const QJsonArray& rows,const QJsonValue& id){for(const auto& row:rows)if(row.toObject().value("id")==id)return row.toObject();return {};}
    void step(const QString& text){qInfo().noquote()<<text;if(recording_){call("recording.step",{{"text",text}});QTest::qWait(900);}}
    void move(QWidget* widget,QPoint target,Qt::KeyboardModifiers modifiers=Qt::NoModifier,Qt::MouseButtons buttons=Qt::NoButton){
        const QPoint end=widget->mapToGlobal(target),start=cursor_.isNull()?end:cursor_;const int steps=recording_?8:1;
        for(int i=1;i<=steps;++i){const QPoint global=start+(end-start)*i/steps;const QPoint local=widget->mapFromGlobal(global);
            QMouseEvent e(QEvent::MouseMove,QPointF(local),QPointF(global),Qt::NoButton,buttons,modifiers);QApplication::sendEvent(widget,&e);if(recording_)QTest::qWait(60);}
        cursor_=end;
    }
    void click(QWidget* widget,QPoint point,Qt::KeyboardModifiers modifiers=Qt::NoModifier){require(widget && widget->isVisible(),"Click target visible");move(widget,point,modifiers);QTest::mouseClick(widget,Qt::LeftButton,modifiers,point);QTest::qWait(recording_?250:30);}
    void key(Qt::Key key,Qt::KeyboardModifiers modifiers=Qt::NoModifier){require(QApplication::focusWidget()==view_,"Viewport naturally focused before key");QTest::keyClick(view_,key,modifiers);QTest::qWait(recording_?250:20);}
    QPoint pixel(double u,double v){const auto p=call("view.projectSketchPoint",{{"sketchId",draft().value("id")},{"uMm",u},{"vMm",v}});const QPoint result(qRound(p.value("x").toDouble()),qRound(p.value("y").toDouble()));require(view_->rect().contains(result),"Sketch point inside viewport");return result;}
    void menu(const QString& menuId,const QString& actionId,const QString& submenuId={}){
        auto* button=w_->findChild<QToolButton*>("sketch.menu."+menuId);auto* top=w_->findChild<QMenu*>("sketch.commands."+menuId);auto* action=w_->findChild<QAction*>(actionId);
        require(button&&top&&action,"Menu and action exist");
        // This macOS QToolButton enters native QMenu::exec during Qt mousePress,
        // so QTest cannot deliver mouseRelease. Open the same menu nonblockingly;
        // the submenu hover and action click remain delivered to real Qt widgets.
        top->popup(button->mapToGlobal(QPoint(0,button->height())));
        require(QTest::qWaitFor([&]{return top->isVisible();},2000),"Command menu visible");
        QMenu* target=top;
        if(!submenuId.isEmpty()){
            target=w_->findChild<QMenu*>("sketch.commands."+submenuId);
            require(target,"Submenu exists");
            move(top,top->actionGeometry(target->menuAction()).center());
            require(QTest::qWaitFor([&]{return target->isVisible();},2000),"Hover opens submenu");
        }
        require(target->actionGeometry(action).isValid(),"Action belongs to visible menu");
        click(target,target->actionGeometry(action).center());
        require(!target->isVisible(),"Action click closes menu");
        require(session().value("command")=="sketch.edit","Menu leaves sketch active");
    }
    QPoint point(const QJsonValue& id){const auto p=byId(parameters().value("points").toArray(),id);require(!p.isEmpty(),"Retained point exists");return pixel(p.value("uMm").toDouble(),p.value("vMm").toDouble());}
    QJsonObject lastEntity(){return parameters().value("entities").toArray().last().toObject();}
    void capture(const QString& name){call("capture.screenshot",{{"path",out_+"/"+name+".png"}});}
    void ontoCircle(const QJsonValue& circle,double nearU,double nearV){
        const auto endpoint=byId(parameters().value("points").toArray(),lastEntity().value("endPointId"));
        const auto candidates=SketchGeometry::pointerCircleTangents(parameters(),circle.toString(),endpoint.value("uMm").toDouble(),endpoint.value("vMm").toDouble());
        require(candidates.size()==2,"Two finite circle tangent candidates");QJsonObject chosen;double best=1e100;
        for(const auto& value:candidates){const auto p=value.toObject();const double distance=std::hypot(p.value("uMm").toDouble()-nearU,p.value("vMm").toDouble()-nearV);if(distance<best){best=distance;chosen=p;}}
        const auto pick=pixel(chosen.value("uMm").toDouble(),chosen.value("vMm").toDouble());move(view_,pick);QTest::qWait(40);
        auto* cue=w_->findChild<QLabel*>("sketch.lineLockCue");require(cue&&cue->isVisible()&&cue->text()=="Tangent","Ordinary circle endpoint shows Tangent cue");capture("circle-contact-preview");click(view_,pick);
    }
private slots:
    void constructionSteps(){
        out_=qEnvironmentVariable("ALLOY_BASE_SKETCH_EVIDENCE");QVERIFY(!out_.isEmpty());QVERIFY(QDir().mkpath(out_));
        AlloyTheme::apply(*qApp);AlloyWindow window;w_=&window;window.resize(1500,950);window.show();QVERIFY(QTest::qWaitForWindowExposed(&window));
        for(auto* child:window.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(child))view_=v;QVERIFY(view_);
#ifdef Q_OS_MACOS
        QProcess activate;activate.start("osascript",{"-e",QString("tell application \"System Events\" to set frontmost of first application process whose unix id is %1 to true").arg(QCoreApplication::applicationPid())});QVERIFY(activate.waitForFinished(5000));QCOMPARE(activate.exitCode(),0);
#endif
        window.raise();window.activateWindow();window.windowHandle()->requestActivate();QApplication::setActiveWindow(&window);QTRY_VERIFY(window.isActiveWindow());
        try{
            QVERIFY(call("document.inspect").value("authored").toObject().value("features").toArray().isEmpty());
            if(qEnvironmentVariableIntValue("ALLOY_BASE_SKETCH_RECORD")){call("recording.start",{{"path",out_+"/base-sketch-construction.mp4"},{"fps",15}});recording_=true;}
            step("Blank design · Create Sketch on XY");auto* create=window.findChild<QToolButton*>("command.sketch.create");QVERIFY(create);click(create,create->rect().center());
            auto* plane=window.findChild<QWidget*>("sketch.plane.XY");QVERIFY(plane);click(plane,plane->rect().center());
            QCOMPARE(session().value("command").toString(),QString("sketch.edit"));
            // View configuration and coordinate projection guide native picks; no geometry is seeded.
            call("view.zoom",{{"factor",0.65}});const auto center=pixel(35,-5);call("view.pan",{{"x",view_->width()/2-center.x()},{"y",view_->height()/2-center.y()}});
            step("1 · Center Diameter Circle at origin · roughly 30 mm");menu("create","sketch.tool.circle","circle");click(view_,pixel(0,0));click(view_,pixel(15,0));key(Qt::Key_Escape);const auto left=lastEntity().value("id");
            step("2 · Second rough circle · position and size are not constrained yet");menu("create","sketch.tool.circle","circle");click(view_,pixel(70,7));click(view_,pixel(79,7));key(Qt::Key_Escape);const auto right=lastEntity().value("id");
            QCOMPARE(parameters().value("entities").toArray().size(),2);capture("two-circles");
            step("3 · Line from left circle · Shift for tangent direction");menu("create","sketch.tool.line");
            click(view_,pixel(-6,-std::sqrt(15.0*15-36)));move(view_,pixel(38,-33),Qt::ShiftModifier);
            auto* cue=window.findChild<QLabel*>("sketch.lineLockCue");QVERIFY(cue&&cue->isVisible());QCOMPARE(cue->text(),QString("Tangent"));capture("lower-tangent-preview");
            click(view_,pixel(38,-33),Qt::ShiftModifier);const auto lowerFirst=lastEntity();
            step("3 · Ordinary click onto right circle's tangent contact · Escape");ontoCircle(right,77,0);key(Qt::Key_Escape);QCOMPARE(parameters().value("entities").toArray().size(),4);
            QJsonArray remoteRelations;for(const auto& value:parameters().value("constraints").toArray()) {
                const auto row=value.toObject();if(row.value("type")=="tangent" || row.value("type")=="pointOnCurve")remoteRelations.append(row);
            }QCOMPARE(remoteRelations.size(),4);
            step("4 · Fillet · click the line junction · type 10 mm · Enter");menu("modify","sketch.tool.fillet");click(view_,point(lowerFirst.value("endPointId")));
            auto* radius=window.findChild<QLineEdit*>("sketch.fillet.radius");QVERIFY(radius&&radius->isVisible());QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(radius));
            QTest::keyClick(radius,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(radius,"10",Qt::NoModifier,recording_?100:-1);QTest::qWait(60);
            QVERIFY(session().value("pendingFillet").toObject().contains("previewDraft"));capture("r10-preview");QTest::keyClick(radius,Qt::Key_Return);QTest::qWait(60);
            QVERIFY(!session().contains("pendingFillet"));QCOMPARE(parameters().value("entities").toArray().size(),5);
            const auto fillet=lastEntity();QCOMPARE(fillet.value("type").toString(),QString("arc"));QVERIFY(std::abs(fillet.value("radiusMm").toDouble()-10)<1e-6);
            for(const auto& row:remoteRelations)QCOMPARE(byId(parameters().value("constraints").toArray(),row.toObject().value("id")),row.toObject());
            capture("r10-accepted");
            step("5 · Start upper boundary on left circle · Shift for tangent Line");menu("create","sketch.tool.line");
            // Read current circle geometry: prior solves are allowed to move underconstrained curves.
            const auto circle=byId(parameters().value("entities").toArray(),left);const auto c=byId(parameters().value("points").toArray(),circle.value("centerPointId"));
            const double r=circle.value("radiusMm").toDouble();click(view_,pixel(c.value("uMm").toDouble()+r*0.55,c.value("vMm").toDouble()+r*std::sqrt(1-0.55*0.55)));
            click(view_,pixel(27,0),Qt::ShiftModifier);const auto upperLine=lastEntity();const auto arcStart=point(upperLine.value("endPointId"));const auto beforeDrag=parameters();
            step("5 · Hold at the last endpoint · drag a tangent Arc · release to create");move(view_,arcStart);QTest::mousePress(view_,Qt::LeftButton,Qt::NoModifier,arcStart);
            const auto arcEnd=pixel(49,0);move(view_,arcEnd,Qt::NoModifier,Qt::LeftButton);QTest::qWait(recording_?800:60);capture("tangent-arc-preview");
            QCOMPARE(parameters(),beforeDrag);QTest::mouseRelease(view_,Qt::LeftButton,Qt::NoModifier,arcEnd);QTest::qWait(60);
            QCOMPARE(session().value("tool").toString(),QString("line"));const auto arc=lastEntity();QCOMPARE(arc.value("type").toString(),QString("arc"));QCOMPARE(arc.value("startPointId"),upperLine.value("endPointId"));
            step("5 · Continue Line onto right circle · completed shaded outline");ontoCircle(right,64,14);key(Qt::Key_Escape);
            const auto outgoing=lastEntity();bool linked=outgoing.value("startPointId")==arc.value("endPointId");
            for(const auto& value:parameters().value("constraints").toArray()) {const auto row=value.toObject();const auto ids=row.value("pointIds").toArray();
                if(row.value("type")=="coincident" && ids.contains(outgoing.value("startPointId")) && ids.contains(arc.value("endPointId")))linked=true;
            }QVERIFY(linked);capture("construction-complete");const auto construction=draft();
            QFile savedDraft(out_+"/construction-draft.json");QVERIFY(savedDraft.open(QIODevice::WriteOnly));savedDraft.write(QJsonDocument(construction).toJson());savedDraft.close();
            step("Alloy check · Small circle size edit · inspect retained contacts");
            const auto currentRight=byId(parameters().value("entities").toArray(),right);const double oldDiameter=2*currentRight.value("radiusMm").toDouble();
            call("sketch.dimension",{{"kind","diameter"},{"entityId",right},{"value",oldDiameter+1},{"units","mm"}});
            const auto changed=draft();QVERIFY(changed!=construction);
            const auto editedGeometry=call("command.preview",{{"session",session().value("id")}}).value("sketchGeometry").toObject();
            bool editedBridge=false;for(const auto& value:editedGeometry.value("profiles").toArray()) {const auto row=value.toObject();QVERIFY(row.value("valid").toBool());
                const auto ids=row.value("sourceEntityIds").toArray();if(ids.contains(arc.value("id")) && ids.contains(fillet.value("id")))editedBridge=true;
            }QVERIFY(editedBridge);
            QCOMPARE(changed.value("parameters").toObject().value("constraints"),construction.value("parameters").toObject().value("constraints"));
            const auto editedCircle=byId(changed.value("parameters").toObject().value("entities").toArray(),right);
            QVERIFY(std::abs(2*editedCircle.value("radiusMm").toDouble()-(oldDiameter+1))<1e-6);capture("circle-edit-preserves-closure");
            call("history.undo");QCOMPARE(draft(),construction);call("history.redo");QCOMPARE(draft(),changed);
            // Restore the exact undimensioned source checkpoint for the next tutorial.
            call("history.undo");QCOMPARE(draft(),construction);
            step("Alloy check · Finish, save and reopen the editable construction");auto* finish=window.findChild<QPushButton*>("sketch.finish");QVERIFY(finish);click(finish,finish->rect().center());
            const auto inspection=call("document.inspect");const auto sketches=inspection.value("evaluation").toObject().value("sketches").toArray();QCOMPARE(sketches.size(),1);
            const auto profiles=sketches.first().toObject().value("profiles").toArray();QVERIFY(!profiles.isEmpty());for(const auto& p:profiles)QVERIFY(p.toObject().value("areaMm2").toDouble()>0);
            bool connector=false;double totalArea=0;
            for(const auto& value:profiles){const auto row=value.toObject();totalArea+=row.value("areaMm2").toDouble();
                const auto sources=row.value("sourceEntityIds").toArray();if(sources.contains(arc.value("id")) && sources.contains(fillet.value("id")))connector=true;
            }QVERIFY(connector);
            const auto constructionEntities=construction.value("parameters").toObject().value("entities").toArray();
            const double lr=byId(constructionEntities,left).value("radiusMm").toDouble(),rr=byId(constructionEntities,right).value("radiusMm").toDouble();
            QVERIFY(totalArea>std::acos(-1.0)*(lr*lr+rr*rr)+100);
            // Bridge cell boundary must use both arcs and add substantial area beyond the two circles.
            // Exact cell count is not sourced. Independent visual review must confirm the connected silhouette.
            call("document.save",{{"path",out_+"/base-sketch-construction.alloy"}});const auto authored=inspection.value("authored");call("document.new",{{"discard",false}});call("document.open",{{"path",out_+"/base-sketch-construction.alloy"}});QCOMPARE(call("document.inspect").value("authored"),authored);
            step("Construction checkpoint · editable contacts and tangent arc");if(recording_){QTest::qWait(1800);call("recording.stop");recording_=false;}
            QFile result(out_+"/result.json");QVERIFY(result.open(QIODevice::WriteOnly));result.write(QJsonDocument(QJsonObject{{"status","passed-pending-independent-silhouette-review"},{"authored",authored},{"evaluation",inspection.value("evaluation")},{"editedDraft",changed},{"editedGeometry",editedGeometry},{"input","Native Qt source-order geometry and radius; harness nonblocking menu opener; API viewport/projection, inspection, bounded edit/history/storage extension"}}).toJson());window.close();
        }catch(const std::exception& e){if(recording_){window.executeApi("recording.stop",{});recording_=false;}window.executeApi("capture.screenshot",{{"path",out_+"/failure.png"}});QFAIL(e.what());}
    }
};
QTEST_MAIN(BaseSketchTutorial)
#include "base_sketch_tutorial.moc"
