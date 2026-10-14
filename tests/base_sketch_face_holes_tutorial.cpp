// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "CadDocument.h"
#include "ReferenceView.h"
#include <BRepClass3d_SolidClassifier.hxx>
#include <TopAbs_State.hxx>
#include <gp_Pnt.hxx>
#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFile>
#include <QJsonDocument>
#include <QLineEdit>
#include <QMenu>
#include <QMouseEvent>
#include <QProcess>
#include <QPushButton>
#include <QTest>
#include <QToolButton>
#include <QWindow>
#include <cmath>
#include <stdexcept>

class BaseSketchFaceHolesTutorial final : public QObject {
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
    QJsonObject draft() const{return session().value("draftSketch").toObject();}
    QJsonObject inspect(){return call("document.inspect");}
    void step(const QString& label){qInfo().noquote()<<label;if(recording_){call("recording.step",{{"text",label}});QTest::qWait(700);}}
    void capture(const QString& name){call("capture.screenshot",{{"path",output_+"/"+name+".png"}});}
    void move(QWidget* widget,QPoint local){const auto global=widget->mapToGlobal(local);
        QMouseEvent event(QEvent::MouseMove,QPointF(local),QPointF(global),Qt::NoButton,Qt::NoButton,Qt::NoModifier);
        QApplication::sendEvent(widget,&event);QTest::qWait(recording_?120:25);}
    void click(QWidget* widget,QPoint local){require(widget&&widget->isVisible(),"Native target visible");
        auto* actual=QApplication::widgetAt(widget->mapToGlobal(local));
        require(actual==widget || widget->isAncestorOf(actual),"Native target owns pixel");
        move(widget,local);QTest::mouseClick(widget,Qt::LeftButton,Qt::NoModifier,local);QTest::qWait(recording_?200:25);}
    QPoint sketchPixel(const QString& sketchId,double u,double v){const auto point=call("view.projectSketchPoint",{{"sketchId",sketchId},{"uMm",u},{"vMm",v}});
        const QPoint result(qRound(point.value("x").toDouble()),qRound(point.value("y").toDouble()));
        require(view_->rect().contains(result),"Projected sketch point inside viewport");return result;}
    void writeJson(const QString& name,const QJsonObject& object){QFile file(output_+"/"+name+".json");require(file.open(QIODevice::WriteOnly),"Can save diagnostic JSON");file.write(QJsonDocument(object).toJson(QJsonDocument::Indented));}
    void openPressPull(){
        auto* button=window_->findChild<QToolButton*>("solid.menu.modify");
        auto* menu=window_->findChild<QMenu*>("solid.commands.modify");
        auto* action=window_->findChild<QAction*>("solid.action.solid.pressPullDialog");
        require(button&&menu&&action&&menu->actions().contains(action),"Modify > Press Pull action exists");
        // QTest mousePress enters the menu's nested event loop on macOS. Open the
        // real menu nonblockingly; the item receives a native Qt widget click.
        menu->popup(button->mapToGlobal(QPoint(0,button->height())));
        require(QTest::qWaitFor([&]{return menu->isVisible();},2000),"Modify menu visible");
        click(menu,menu->actionGeometry(action).center());
        require(session().value("command")=="solid.extrude","Press Pull session active");
    }
    void openSketchCircle(){
        auto* button=window_->findChild<QToolButton*>("sketch.menu.create");
        auto* top=window_->findChild<QMenu*>("sketch.commands.create");
        auto* circle=window_->findChild<QMenu*>("sketch.commands.circle");
        auto* action=window_->findChild<QAction*>("sketch.tool.circle");
        require(button&&top&&circle&&action,"Sketch > Circle > Center Diameter route exists");
        // The real QMenu is opened nonblockingly to avoid the QToolButton press
        // loop in QtTest; submenu hover and action click are native Qt input.
        top->popup(button->mapToGlobal(QPoint(0,button->height())));
        require(QTest::qWaitFor([&]{return top->isVisible();},2000),"Sketch Create menu visible");
        move(top,top->actionGeometry(circle->menuAction()).center());
        require(QTest::qWaitFor([&]{return circle->isVisible();},2000),"Circle submenu visible");
        click(circle,circle->actionGeometry(action).center());
        require(session().value("tool")=="circle","Center Diameter Circle tool active");
    }
    void chooseCombo(QComboBox* combo,const QString& data){
        require(combo&&combo->isVisible(),"Native option combo visible");
        int index=-1;for(int i=0;i<combo->count();++i)if(combo->itemData(i)==data){index=i;break;}
        require(index>=0,"Required native option exists: "+data);
        click(combo,combo->rect().center());
        require(QTest::qWaitFor([&]{return combo->view()->isVisible();},2000),"Option popup visible");
        auto* focused=QApplication::focusWidget();
        require(focused&&(focused==combo || focused==combo->view() || combo->view()->window()->isAncestorOf(focused)),
            "Option popup owns keyboard focus without repair");
        QTest::keyClick(focused,Qt::Key_Home);
        for(int i=0;i<index;++i)QTest::keyClick(focused,Qt::Key_Down);
        QTest::keyClick(focused,Qt::Key_Return);
        require(combo->currentData()==data,"Native option selection persisted: "+data);
    }
private slots:
    void faceHoles(){
        output_=qEnvironmentVariable("ALLOY_BASE_SKETCH_HOLES_EVIDENCE");QVERIFY(!output_.isEmpty());QVERIFY(QDir().mkpath(output_));
        const auto input=qEnvironmentVariable("ALLOY_BASE_SKETCH_HOLES_INPUT");QVERIFY(!input.isEmpty());
        AlloyTheme::apply(*qApp);AlloyWindow window;window_=&window;window.resize(1500,950);window.show();QVERIFY(QTest::qWaitForWindowExposed(&window));
        for(auto* child:window.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(child))view_=candidate;QVERIFY(view_);
#ifdef Q_OS_MACOS
        QProcess activate;activate.start("osascript",{"-e",QString("tell application \"System Events\" to set frontmost of first application process whose unix id is %1 to true").arg(QCoreApplication::applicationPid())});QVERIFY(activate.waitForFinished(5000));QCOMPARE(activate.exitCode(),0);
#endif
        window.raise();window.activateWindow();window.windowHandle()->requestActivate();QApplication::setActiveWindow(&window);QTRY_VERIFY(window.isActiveWindow());
        try{
            call("document.open",{{"path",input},{"discard",true}});const auto initial=inspect();
            if(qEnvironmentVariableIntValue("ALLOY_BASE_SKETCH_HOLES_RECORD")){
                call("recording.start",{{"path",output_+"/base-sketch-face-holes.mp4"},{"fps",15}});recording_=true;
            }
            const auto body=initial.value("evaluation").toObject().value("bodies").toArray().first().toObject();
            QJsonObject face;for(const auto& value:body.value("supportFaces").toArray())if(value.toObject().value("role")=="extrude.endCap")face=value.toObject();
            require(!face.isEmpty(),"Retained end cap available");QJsonObject reference;
            for(const auto& key:{"producerFeatureId","bodyId","faceId"})reference[key]=face.value(key);
            call("view.home");call("view.orientation",{{"name","top"}});
            const auto projected=call("view.projectFacePoint",{{"support",reference},{"uMm",35},{"vMm",10}});
            writeJson("cap-projected-point",projected);
            const QPoint facePoint(qRound(projected.value("x").toDouble()),qRound(projected.value("y").toDouble()));
            step("Create Sketch · choose the retained extrusion end cap");
            auto* create=window.findChild<QToolButton*>("command.sketch.create");require(create,"Create Sketch control exists");click(create,create->rect().center());
            require(session().value("command")=="sketch.plane","Create Sketch requests a support face");
            click(view_,facePoint);writeJson("cap-selection",state().value("selection").toObject());writeJson("after-face-session",session());
            require(session().value("command")=="sketch.edit"&&session().value("inputs").toObject().value("support").toObject()==reference,
                "Face sketch starts from actual cap selection");
            call("sketch.lookAt");call("view.zoom",{{"factor",0.65}});
            const auto context=session().value("projectionContext").toObject();writeJson("face-projection-context",context);
            capture("face-sketch-start");
            require(context.value("points").toArray().size()>=3,"Circular support centers available for native snaps");
            step("Circle · right boss center · diameter 10 mm");
            openSketchCircle();
            auto circleAt=[&](double u,double v,const QString& diameter,bool secondReturn=false){
                click(view_,sketchPixel(draft().value("id").toString(),u,v));
                require(session().contains("firstPoint"),"Native center click starts circle");
                QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view_));
                QTest::keyClick(view_,Qt::Key(diameter[0].digitValue()+Qt::Key_0));
                auto* input=window.findChild<QLineEdit*>("sketch.inlineValue");require(input&&input->isVisible(),"Diameter editor appears");
                QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(input));
                for(int index=1;index<diameter.size();++index)QTest::keyClick(input,Qt::Key(diameter[index].digitValue()+Qt::Key_0));
                QTest::keyClick(input,Qt::Key_Return);
                require(!session().contains("firstPoint"),"Native diameter commits circle");
                if(secondReturn){
                    auto* focused=QApplication::focusWidget();
                    writeJson("before-second-return-state",state());
                    require(focused&&focused->isVisible(),"Second documented Return has a native focused recipient");
                    QTest::keyClick(focused,Qt::Key_Return);
                    writeJson("after-second-return-session",session());
                    writeJson("after-second-return-draft",draft());
                    require(session().value("tool")=="select" && !session().contains("firstPoint")
                        && draft().value("parameters").toObject().value("entities").toArray().size()==1,
                        "Second Return exits Circle while keeping the first source-referenced circle");
                }
            };
            circleAt(71.699,3.82,"10",true);writeJson("right-circle-draft",draft());capture("right-circle");
            step("Circle · lower arc center · diameter 8 mm");openSketchCircle();circleAt(48.12,-20.181,"8");
            step("Circle · left boss center · diameter 8 mm");circleAt(0,0,"8");
            writeJson("all-three-circles-draft",draft());capture("three-circle-centers");
            const auto parameters=draft().value("parameters").toObject();
            require(parameters.value("entities").toArray().size()==3,"Three authored circle profiles");
            require(parameters.value("pointReferences").toArray().size()==3,"All three native centers retain source references");
            const auto dimensions=parameters.value("dimensions").toArray();
            require(dimensions.size()==3 && dimensions[0].toObject().value("valueMm")==10
                && dimensions[1].toObject().value("valueMm")==8 && dimensions[2].toObject().value("valueMm")==8,
                "Source circle diameter order is retained");
            const auto holeSketchId=draft().value("id").toString();
            const auto rightCenterId=parameters.value("entities").toArray()[0].toObject().value("centerPointId").toString();
            const auto leftSourceId=parameters.value("pointReferences").toArray()[2].toObject().value("source").toObject().value("sourceEntityId").toString();
            QString projectedLeftSourceId;
            for(const auto& entry:context.value("points").toArray()){
                const auto point=entry.toObject();
                if(std::hypot(point.value("uMm").toDouble(),point.value("vMm").toDouble())<1e-6)
                    projectedLeftSourceId=point.value("source").toObject().value("sourceEntityId").toString();
            }
            require(!projectedLeftSourceId.isEmpty() && leftSourceId==projectedLeftSourceId,
                "Origin-overlapping circle binds the exact retained left source center");
            step("Stop Sketch · retain the three circular regions");
            auto* finish=window.findChild<QPushButton*>("sketch.finish");require(finish&&finish->isVisible(),"Stop Sketch button visible");
            click(finish,finish->rect().center());require(session().isEmpty(),"Native Stop Sketch accepted");
            const auto finishedSketch=inspect();
            auto worldCenter=[&](const QJsonObject& inspection){
                for(const auto& entry:inspection.value("evaluation").toObject().value("sketches").toArray()){
                    const auto sketch=entry.toObject();if(sketch.value("featureId")!=holeSketchId && sketch.value("id")!=holeSketchId)continue;
                    for(const auto& point:sketch.value("worldPoints").toArray())if(point.toObject().value("id")==rightCenterId)
                        return point.toObject().value("positionMm").toArray();
                }
                return QJsonArray{};
            };
            require(worldCenter(finishedSketch).size()==3,"Evaluated referenced center exists");
            call("document.save",{{"path",output_+"/face-hole-sketch.alloy"}});
            step("Modify > Press Pull · select the three circle profiles");
            call("view.home");call("view.orientation",{{"name","top"}});openPressPull();
            const QList<QPair<double,double>> profileInterior{{73.699,3.82},{50.12,-20.181},{2,0}};
            QJsonArray selected;
            for(int index=0;index<profileInterior.size();++index){
                const auto [u,v]=profileInterior.at(index);click(view_,sketchPixel(holeSketchId,u,v));
                const auto inputs=session().value("inputs").toObject();selected=inputs.value("profileIds").toArray();
                if(selected.isEmpty()&&inputs.contains("profileId"))selected.append(inputs.value("profileId"));
                require(selected.size()==index+1,"Each native profile click collects one circle");
            }
            capture("three-hole-profiles-selected");
            auto* operation=window.findChild<QComboBox*>("extrude.operation");
            auto* extent=window.findChild<QComboBox*>("extrude.extent");
            auto* target=window.findChild<QComboBox*>("extrude.targetBody");
            auto* flip=window.findChild<QCheckBox*>("extrude.flip");
            step("Cut · All · Flip");chooseCombo(operation,"cut");chooseCombo(extent,"all");
            const auto bodyId=body.value("bodyId").toString();
            require(target&&target->currentData().toJsonArray()==QJsonArray{bodyId}
                && session().value("inputs").toObject().value("targetBodyIds").toArray()==QJsonArray{bodyId},
                "Unique supported solid is resolved without an extra target click");
            require(flip&&flip->isVisible()&&!flip->isChecked(),"Flip starts clear");
            click(flip,QPoint(10,flip->height()/2));
            writeJson("cut-preview-session",session());
            require(flip->isChecked()&&session().value("previewed").toBool(),"Three-hole through-cut preview valid");
            capture("three-hole-cut-preview");
            auto* buttons=window.findChild<QDialogButtonBox*>("extrude.buttons");
            require(buttons&&buttons->button(QDialogButtonBox::Ok)->isEnabled(),"Cut OK enabled");
            click(buttons->button(QDialogButtonBox::Ok),buttons->button(QDialogButtonBox::Ok)->rect().center());
            require(session().isEmpty(),"Native Cut committed");
            const auto fullCut=inspect();writeJson("cut-final-inspect",fullCut);capture("three-through-holes-top");
            click(view_,QPoint(30,30));call("view.home");capture("three-through-holes-isometric");
            step("Three through-openings · retained Cut All Flip");
            if(recording_){call("recording.stop");recording_=false;}
            const auto cutFeature=fullCut.value("authored").toObject().value("features").toArray().last().toObject();
            const auto cutInputs=cutFeature.value("parameters").toObject();
            require(cutFeature.value("type")=="extrude" && cutInputs.value("operation")=="cut"
                && cutInputs.value("extent")=="all" && cutInputs.value("flip").toBool()
                && cutInputs.value("targetBodyIds").toArray()==QJsonArray{bodyId}
                && cutInputs.value("profileIds").toArray().size()==3,
                "One Cut retains exact All, Flip, target and three profile IDs");
            const auto cutBody=fullCut.value("evaluation").toObject().value("bodies").toArray().first().toObject();
            const double expectedRemoval=std::acos(-1.0)*57*48.25;
            const double beforeVolume=body.value("volumeMm3").toDouble();
            require(cutBody.value("valid").toBool()&&cutBody.value("closed").toBool()
                && std::abs(beforeVolume-cutBody.value("volumeMm3").toDouble()-expectedRemoval)<1e-3,
                "Three through cylinders remove the analytically expected volume");
            call("document.save",{{"path",output_+"/three-through-holes.alloy"}});
            const auto editPreview=call("solid.extrudeDialog",{{"featureId",cutFeature.value("id")}});
            writeJson("cut-edit-preview-session",session());
            require(editPreview.value("command")=="solid.extrude" && session().value("previewed").toBool(),
                "Retained Cut edit has a valid disposable preview");
            call("command.cancel",{{"session",session().value("id")}});
            require(session().isEmpty() && inspect().value("authored")==fullCut.value("authored"),
                "Cancel leaves the committed three-hole Cut unchanged");
            CadDocument originalKernel,cutKernel;
            require(originalKernel.open(input).isEmpty() && cutKernel.open(output_+"/three-through-holes.alloy").isEmpty()
                && originalKernel.bodies().size()==1 && cutKernel.bodies().size()==1,
                "Saved original and Cut kernel solids reopen");
            const QList<QPair<double,double>> centers{{71.699,3.82},{48.12,-20.181},{0,0}};
            const QList<double> radii{5,4,4};
            for(int i=0;i<centers.size();++i){
                const auto [cx,cy]=centers[i];
                for(const double z:{1.0,24.125,47.25}){
                    BRepClass3d_SolidClassifier before(originalKernel.bodies().front().shape,gp_Pnt(cx,cy,z),1e-7);
                    BRepClass3d_SolidClassifier after(cutKernel.bodies().front().shape,gp_Pnt(cx,cy,z),1e-7);
                    require(before.State()==TopAbs_IN && after.State()==TopAbs_OUT,
                        "Each referenced axis crosses original material and remains open through all depth");
                }
                bool neighboringMaterial=false;
                for(int step=0;step<8;++step){
                    const double angle=std::acos(-1.0)*step/4;
                    const gp_Pnt neighbor(cx+(radii[i]+2)*std::cos(angle),cy+(radii[i]+2)*std::sin(angle),24.125);
                    BRepClass3d_SolidClassifier before(originalKernel.bodies().front().shape,neighbor,1e-7);
                    BRepClass3d_SolidClassifier after(cutKernel.bodies().front().shape,neighbor,1e-7);
                    if(before.State()==TopAbs_IN && after.State()==TopAbs_IN)neighboringMaterial=true;
                }
                require(neighboringMaterial,"Material survives beside each through-opening");
            }
            step("Alloy continuity · source boss center edit, undo, redo and reopen");
            writeJson("before-source-edit-session",session());
            require(session().isEmpty(),"Source edit begins with no active command");
            const auto originalCenter=worldCenter(fullCut);
            const auto source=fullCut.value("authored").toObject().value("features").toArray().first().toObject();
            QJsonObject movementDimension;
            for(const auto& value:source.value("parameters").toObject().value("dimensions").toArray())
                if(value.toObject().value("name")=="d10")movementDimension=value.toObject();
            require(!movementDimension.isEmpty(),"Source boss horizontal dimension available");
            call("sketch.edit",{{"sketchId",source.value("id")}});
            call("sketch.dimensionUpdate",{{"dimensionId",movementDimension.value("id")},{"value",72.699},{"units","mm"}});
            call("sketch.finish");const auto afterSourceEdit=inspect();writeJson("source-center-edit",afterSourceEdit);
            const auto movedCenter=worldCenter(afterSourceEdit);
            require(movedCenter.size()==3 && std::abs(movedCenter[0].toDouble()-originalCenter[0].toDouble()-1)<1e-5,
                "Retained right center follows source geometry edit by 1 mm after Cut");
            call("history.undo");require(inspect().value("authored")==fullCut.value("authored"),"Source edit undo restores full through-Cut");
            call("history.redo");require(inspect().value("authored")==afterSourceEdit.value("authored"),"Redo restores source edit and linked Cut");
            call("history.undo");require(inspect().value("authored")==fullCut.value("authored"),"Final undo restores tutorial Cut");
            step("Alloy continuity · edit lower hole diameter 8 to 9 mm, then restore");
            call("sketch.edit",{{"sketchId",holeSketchId}});
            call("sketch.dimensionUpdate",{{"dimensionId",dimensions[1].toObject().value("id")},{"value",9},{"units","mm"}});
            call("sketch.finish");const auto diameterEdit=inspect();writeJson("lower-hole-diameter-edit",diameterEdit);
            const auto enlarged=diameterEdit.value("evaluation").toObject().value("bodies").toArray().first().toObject();
            const double diameterRemoval=std::acos(-1.0)*(25+4.5*4.5+16)*48.25;
            require(enlarged.value("valid").toBool() && enlarged.value("closed").toBool()
                && std::abs(beforeVolume-enlarged.value("volumeMm3").toDouble()-diameterRemoval)<1e-3,
                "Lower hole diameter changes the retained through-Cut by the expected volume");
            call("history.undo");require(inspect().value("authored")==fullCut.value("authored"),"Diameter undo restores tutorial Cut");
            call("history.redo");require(inspect().value("authored")==diameterEdit.value("authored"),"Diameter redo restores enlarged Cut");
            call("history.undo");require(inspect().value("authored")==fullCut.value("authored"),"Final diameter undo restores tutorial Cut");
            call("document.open",{{"path",output_+"/three-through-holes.alloy"},{"discard",true}});
            require(inspect().value("authored")==fullCut.value("authored"),"Saved tutorial Cut reopens exactly");
            window.close();
        }catch(const std::exception& error){if(recording_){window.executeApi("recording.stop",{});recording_=false;}
            window.executeApi("capture.screenshot",{{"path",output_+"/failure.png"}});QFAIL(error.what());}
    }
};
QTEST_MAIN(BaseSketchFaceHolesTutorial)
#include "base_sketch_face_holes_tutorial.moc"
