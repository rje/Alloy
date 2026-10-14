// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
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

class SketchExpressionTutorial final : public QObject {
    Q_OBJECT
    AlloyWindow* w_=nullptr;ReferenceView* view_=nullptr;QString out_;bool recording_=false;QPoint cursor_;
    static void require(bool value,const QString& why){if(!value)throw std::runtime_error(why.toStdString());}
    QJsonObject call(const QString& name,const QJsonObject& args={}){const auto r=w_->executeApi(name,args);require(r.value("ok").toBool(),name+": "+QString::fromUtf8(QJsonDocument(r).toJson()));return r.value("result").toObject();}
    QJsonObject session(){return w_->state().value("activeSession").toObject();}
    QJsonObject draft(){return session().value("draftSketch").toObject();}
    QJsonObject parameters(){return draft().value("parameters").toObject();}
    QJsonObject byId(const QJsonArray& rows,const QJsonValue& id){for(const auto& row:rows)if(row.toObject().value("id")==id)return row.toObject();return {};}
    void step(const QString& text){qInfo().noquote()<<text;if(recording_){call("recording.step",{{"text",text}});QTest::qWait(900);}}
    void move(QWidget* widget,QPoint target){
        const QPoint end=widget->mapToGlobal(target),start=cursor_.isNull()?end:cursor_;const int steps=recording_?8:1;
        for(int i=1;i<=steps;++i){const QPoint global=start+(end-start)*i/steps;const QPoint local=widget->mapFromGlobal(global);
            QMouseEvent e(QEvent::MouseMove,QPointF(local),QPointF(global),Qt::NoButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(widget,&e);if(recording_)QTest::qWait(60);}
        cursor_=end;
    }
    void click(QWidget* widget,QPoint point,Qt::KeyboardModifiers modifiers=Qt::NoModifier){require(widget && widget->isVisible(),"Click target visible");move(widget,point);QTest::mouseClick(widget,Qt::LeftButton,modifiers,point);QTest::qWait(recording_?250:30);}
    void key(Qt::Key key,Qt::KeyboardModifiers modifiers=Qt::NoModifier){require(QApplication::focusWidget()==view_,"Viewport naturally focused before key");QTest::keyClick(view_,key,modifiers);QTest::qWait(recording_?250:20);}
    QPoint pixel(double u,double v){const auto p=call("view.projectSketchPoint",{{"sketchId",draft().value("id")},{"uMm",u},{"vMm",v}});const QPoint result(qRound(p.value("x").toDouble()),qRound(p.value("y").toDouble()));require(view_->rect().contains(result),"Sketch point inside viewport");return result;}
    QPoint middle(const QJsonObject& line){const auto p=parameters().value("points").toArray();const auto a=byId(p,line.value("startPointId")),b=byId(p,line.value("endPointId"));require(!a.isEmpty()&&!b.isEmpty(),"Line endpoints retained");return pixel((a.value("uMm").toDouble()+b.value("uMm").toDouble())/2,(a.value("vMm").toDouble()+b.value("vMm").toDouble())/2);}
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
    void constraint(const QString& type,const QJsonObject& first,const QJsonObject& second={}){
        click(view_,middle(first));
        require(session().value("selection").toObject().value("entityIds").toArray().contains(first.value("id")),
            "First native constraint pick selected its retained line");
        if(!second.isEmpty()){
            click(view_,middle(second),Qt::ShiftModifier);
            require(session().value("selection").toObject().value("entityIds").toArray().contains(second.value("id")),
                "Second native constraint pick selected its retained line");
        }
        menu("constraints","sketch.action.constraint."+type);key(Qt::Key_Escape);
    }
    void enter(const QString& text){auto* editor=w_->findChild<QLineEdit*>("sketch.inlineValue");require(editor&&editor->isVisible()&&QApplication::focusWidget()==editor,"Inline editor naturally focused");QTest::keyClick(editor,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(editor,text,Qt::NoModifier,recording_?100:-1);if(recording_)QTest::qWait(600);QTest::keyClick(editor,Qt::Key_Return);QTest::qWait(recording_?400:30);require(!session().contains("pendingDimension"),"Dimension accepted");}
    void dimension(QPoint target,QPoint annotation,const QString& text){click(view_,target);move(view_,annotation);QTest::qWait(30);require(session().value("pendingDimension").toObject().value("phase")=="placing","Dimension preview present");click(view_,annotation);enter(text);}
private slots:
    void completeSourceExercise(){
        out_=qEnvironmentVariable("ALLOY_EXPRESSION_TUTORIAL_EVIDENCE");QVERIFY(!out_.isEmpty());QVERIFY(QDir().mkpath(out_));
        AlloyTheme::apply(*qApp);AlloyWindow window;w_=&window;window.resize(1500,950);window.show();QVERIFY(QTest::qWaitForWindowExposed(&window));
        for(auto* child:window.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(child))view_=v;QVERIFY(view_);
#ifdef Q_OS_MACOS
        QProcess activate;activate.start("osascript",{"-e",QString("tell application \"System Events\" to set frontmost of first application process whose unix id is %1 to true").arg(QCoreApplication::applicationPid())});QVERIFY(activate.waitForFinished(5000));QCOMPARE(activate.exitCode(),0);
#endif
        window.raise();window.activateWindow();window.windowHandle()->requestActivate();QApplication::setActiveWindow(&window);QTRY_VERIFY(window.isActiveWindow());
        try{
            QVERIFY(call("document.inspect").value("authored").toObject().value("features").toArray().isEmpty());
            if(qEnvironmentVariableIntValue("ALLOY_EXPRESSION_TUTORIAL_RECORD")){call("recording.start",{{"path",out_+"/sketch-expression-tutorial.mp4"},{"fps",15}});recording_=true;}
            step("Blank design · Create Sketch on XY");auto* create=window.findChild<QToolButton*>("command.sketch.create");QVERIFY(create);click(create,create->rect().center());
            auto* plane=window.findChild<QWidget*>("sketch.plane.XY");QVERIFY(plane);click(plane,plane->rect().center());
            QCOMPARE(session().value("command").toString(),QString("sketch.edit"));
            // Initial viewport configuration only; no modeling command seeds the exercise.
            call("view.zoom",{{"factor",0.35}});QTest::qWait(100);
            step("1 · Center Diameter Circle · center, then circumference");menu("create","sketch.tool.circle","circle");click(view_,pixel(-110,90));click(view_,pixel(-90,90));key(Qt::Key_Escape);
            step("2 · 2-Point Rectangle · opposite corners");menu("create","sketch.tool.rectangle","rectangle");click(view_,pixel(-120,30));click(view_,pixel(-70,60));key(Qt::Key_Escape);
            QCOMPARE(parameters().value("dimensions").toArray().size(),0);
            step("3 · Center to Center Slot · first center, second center, width");menu("create","sketch.tool.centerToCenterSlot","slot");click(view_,pixel(-90,-30));click(view_,pixel(-48,-22));click(view_,pixel(-70,-18));key(Qt::Key_Escape);
            auto p=parameters();QCOMPARE(p.value("dimensions").toArray().size(),0);const auto slot=p.value("slotGroups").toArray().first().toObject();
            const auto side=byId(p.value("entities").toArray(),slot.value("sideIds").toArray().first());
            step("4 · Select a straight slot side · Horizontal/Vertical");constraint("horizontalVertical",side);
            step("5–6 · D · place 45 mm, then R5 without restarting Dimension");key(Qt::Key_D);dimension(middle(side),pixel(-68,-2),"45");
            p=parameters();const auto cap=byId(p.value("entities").toArray(),slot.value("arcIds").toArray().first());const auto center=byId(p.value("points").toArray(),cap.value("centerPointId"));
            dimension(pixel(center.value("uMm").toDouble()+cap.value("radiusMm").toDouble(),center.value("vMm").toDouble()),pixel(-30,-10),"5");key(Qt::Key_Escape);
            QCOMPARE(parameters().value("dimensions").toArray().size(),2);
            const auto slotDims=parameters().value("dimensions").toArray();
            QCOMPARE(slotDims[0].toObject().value("name").toString(),QString("d1"));
            QCOMPARE(slotDims[0].toObject().value("kind").toString(),QString("horizontal"));
            QCOMPARE(slotDims[0].toObject().value("valueMm").toDouble(),45.0);
            QCOMPARE(slotDims[0].toObject().value("pointIds"),QJsonValue(QJsonArray{side.value("startPointId"),side.value("endPointId")}));
            QCOMPARE(slotDims[1].toObject().value("name").toString(),QString("d2"));
            QCOMPARE(slotDims[1].toObject().value("kind").toString(),QString("radius"));
            QCOMPARE(slotDims[1].toObject().value("valueMm").toDouble(),5.0);
            QCOMPARE(slotDims[1].toObject().value("entityId"),cap.value("id"));
            step("7 · Line · sloping side, bottom, right side, then close the loop");menu("create","sketch.tool.line");const int start=parameters().value("entities").toArray().size();
            for(const auto& q:{QPointF(-50,75),QPointF(0,0),QPointF(100,0),QPointF(100,100),QPointF(-50,75)}) {
                click(view_,pixel(q.x(),q.y()));
                if(q==QPointF(0,0)) {
                    move(view_,pixel(100,0));
                    auto* cue=window.findChild<QLabel*>("sketch.lineLockCue");
                    require(cue && cue->isVisible() && cue->text()=="H","Horizontal preview cue visible before bottom click");
                    call("capture.screenshot",{{"path",out_+"/horizontal-preview-cue.png"}});
                }
            }key(Qt::Key_Escape);
            auto entities=parameters().value("entities").toArray();QCOMPARE(entities.size(),start+4);
            const auto bottom=entities[start+1].toObject(),right=entities[start+2].toObject(),top=entities[start+3].toObject();
            bool bottomHorizontal=false,rightVertical=false;
            for(const auto& value:parameters().value("constraints").toArray()) {
                const auto relation=value.toObject();
                if(relation.value("type")=="horizontal" && relation.value("entityIds").toArray()==QJsonArray{bottom.value("id")})bottomHorizontal=true;
                if(relation.value("type")=="vertical" && relation.value("entityIds").toArray()==QJsonArray{right.value("id")})rightVertical=true;
            }
            QVERIFY(bottomHorizontal && rightVertical);
            step("8 · Select top and right · Perpendicular");constraint("perpendicular",top,right);
            step("9 · Select bottom and right · Equal");constraint("equal",bottom,right);
            step("10 · D · vertical 100 mm, then top d3 + 50");key(Qt::Key_D);dimension(middle(right),pixel(125,50),"100");
            const auto driver=parameters().value("dimensions").toArray().last().toObject();QCOMPARE(driver.value("name").toString(),QString("d3"));
            dimension(middle(top),pixel(25,135),"d3 + 50");key(Qt::Key_Escape);
            const auto dependent=parameters().value("dimensions").toArray().last().toObject();QCOMPARE(dependent.value("expression").toString(),QString("d3 + 50"));QVERIFY(std::abs(dependent.value("valueMm").toDouble()-150)<1e-6);
            call("capture.screenshot",{{"path",out_+"/source-100-150.png"}});
            step("Edit d3 · 100 → 120 mm · dependent top becomes 170 mm");const auto label=view_->dimensionLayout(draft(),driver,100,"mm").value("label").toObject();QVERIFY(!label.isEmpty());
            const QPoint pos(qRound(label.value("x").toDouble()),qRound(label.value("y").toDouble()));move(view_,pos);QTest::mouseDClick(view_,Qt::LeftButton,Qt::NoModifier,pos);enter("120");
            const auto updated=byId(parameters().value("dimensions").toArray(),dependent.value("id"));QVERIFY(std::abs(updated.value("valueMm").toDouble()-170)<1e-6);QCOMPARE(updated.value("referenceIds"),QJsonValue(QJsonArray{driver.value("id")}));
            call("capture.screenshot",{{"path",out_+"/source-120-170.png"}});
            const auto before=draft();step("Alloy check · Undo and redo the driver edit");call("history.undo");QVERIFY(std::abs(byId(parameters().value("dimensions").toArray(),dependent.value("id")).value("valueMm").toDouble()-150)<1e-6);call("history.redo");QCOMPARE(draft(),before);
            step("Alloy check · Finish, save and reopen the editable sketch");auto* finish=window.findChild<QPushButton*>("sketch.finish");QVERIFY(finish);click(finish,finish->rect().center());
            const auto inspection=call("document.inspect");const auto sketches=inspection.value("evaluation").toObject().value("sketches").toArray();QCOMPARE(sketches.size(),1);
            const auto profiles=sketches.first().toObject().value("profiles").toArray();QCOMPARE(profiles.size(),4);
            bool slotArea=false;for(const auto& value:profiles)if(std::abs(value.toObject().value("areaMm2").toDouble()-(450+25*std::acos(-1.0)))<1e-4)slotArea=true;QVERIFY(slotArea);
            double largest=0;for(const auto& value:profiles)largest=std::max(largest,value.toObject().value("areaMm2").toDouble());QVERIFY(std::abs(largest-17400)<1e-4);
            call("document.save",{{"path",out_+"/sketch-expression-tutorial.alloy"}});const auto authored=inspection.value("authored");call("document.new",{{"discard",false}});call("document.open",{{"path",out_+"/sketch-expression-tutorial.alloy"}});QCOMPARE(call("document.inspect").value("authored"),authored);
            step("Complete · Four closed profiles · editable d3 + 50 dependency");if(recording_){QTest::qWait(1800);call("recording.stop");recording_=false;}
            QFile result(out_+"/result.json");QVERIFY(result.open(QIODevice::WriteOnly));result.write(QJsonDocument(QJsonObject{{"status","passed"},{"authored",authored},{"evaluation",inspection.value("evaluation")},{"input","Real QMenu popup opened by harness; Qt submenu/item/pointer/key input; shared API viewport configuration, inspection, history and storage checks"}}).toJson());
            window.close();
        }catch(const std::exception& e){if(recording_){window.executeApi("recording.stop",{});recording_=false;}window.executeApi("capture.screenshot",{{"path",out_+"/failure.png"}});QFAIL(e.what());}
    }
};
QTEST_MAIN(SketchExpressionTutorial)
#include "sketch_expression_tutorial.moc"
