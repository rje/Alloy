// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
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
            QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view_));
            QTest::keyClick(view_,Qt::Key_C);
            require(session().value("tool")=="circle","Circle tool active after C");
            auto circleAt=[&](double u,double v,const QString& diameter){
                click(view_,sketchPixel(draft().value("id").toString(),u,v));
                require(session().contains("firstPoint"),"Native center click starts circle");
                QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view_));
                QTest::keyClick(view_,Qt::Key(diameter[0].digitValue()+Qt::Key_0));
                auto* input=window.findChild<QLineEdit*>("sketch.inlineValue");require(input&&input->isVisible(),"Diameter editor appears");
                QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(input));
                for(int index=1;index<diameter.size();++index)QTest::keyClick(input,Qt::Key(diameter[index].digitValue()+Qt::Key_0));
                QTest::keyClick(input,Qt::Key_Return);
                require(!session().contains("firstPoint"),"Native diameter commits circle");
            };
            circleAt(71.699,3.82,"10");writeJson("right-circle-draft",draft());capture("right-circle");
            step("Circle · lower arc center · diameter 8 mm");circleAt(48.12,-20.181,"8");
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
            call("sketch.finish");const auto beforeSourceEdit=inspect();
            auto worldCenter=[&](const QJsonObject& inspection){
                for(const auto& entry:inspection.value("evaluation").toObject().value("sketches").toArray()){
                    const auto sketch=entry.toObject();if(sketch.value("featureId")!=holeSketchId && sketch.value("id")!=holeSketchId)continue;
                    for(const auto& point:sketch.value("worldPoints").toArray())if(point.toObject().value("id")==rightCenterId)
                        return point.toObject().value("positionMm").toArray();
                }
                return QJsonArray{};
            };
            const auto originalCenter=worldCenter(beforeSourceEdit);require(originalCenter.size()==3,"Evaluated referenced center exists");
            const auto source=beforeSourceEdit.value("authored").toObject().value("features").toArray().first().toObject();
            QJsonObject movementDimension;
            for(const auto& value:source.value("parameters").toObject().value("dimensions").toArray())
                if(value.toObject().value("name")=="d10")movementDimension=value.toObject();
            require(!movementDimension.isEmpty(),"Source boss horizontal dimension available");
            call("sketch.edit",{{"sketchId",source.value("id")}});
            call("sketch.dimensionUpdate",{{"dimensionId",movementDimension.value("id")},{"value",72.699},{"units","mm"}});
            call("sketch.finish");const auto afterSourceEdit=inspect();writeJson("source-center-edit",afterSourceEdit);
            const auto movedCenter=worldCenter(afterSourceEdit);
            require(movedCenter.size()==3 && std::abs(movedCenter[0].toDouble()-originalCenter[0].toDouble()-1)<1e-5,
                "Retained right center follows source geometry edit by 1 mm");
            call("history.undo");require(inspect().value("authored")==beforeSourceEdit.value("authored"),"Source edit undo restores authored sketch");
            call("document.save",{{"path",output_+"/face-hole-sketch.alloy"}});
            window.close();
        }catch(const std::exception& error){if(recording_){window.executeApi("recording.stop",{});recording_=false;}
            window.executeApi("capture.screenshot",{{"path",output_+"/failure.png"}});QFAIL(error.what());}
    }
};
QTEST_MAIN(BaseSketchFaceHolesTutorial)
#include "base_sketch_face_holes_tutorial.moc"
