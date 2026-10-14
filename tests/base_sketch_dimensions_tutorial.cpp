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
#include <QMouseEvent>
#include <QProcess>
#include <QPushButton>
#include <QTest>
#include <QWindow>
#include <cmath>
#include <stdexcept>

class BaseSketchDimensionsTutorial final : public QObject {
    Q_OBJECT
    AlloyWindow* window_=nullptr;
    ReferenceView* view_=nullptr;
    QString output_;
    bool recording_=false;
    QJsonObject call(const QString& command,const QJsonObject& args={}){
        const auto response=window_->executeApi(command,args);
        require(response.value("ok").toBool(),command+": "+QString::fromUtf8(QJsonDocument(response).toJson()));
        return response.value("result").toObject();
    }
    static void require(bool condition,const QString& why){if(!condition)throw std::runtime_error(why.toStdString());}
    QJsonObject session() const{return window_->state().value("activeSession").toObject();}
    QJsonObject draft() const{return session().value("draftSketch").toObject();}
    QJsonObject parameters() const{return draft().value("parameters").toObject();}
    static QJsonObject row(const QJsonArray& rows,const QJsonValue& id){for(const auto& value:rows)if(value.toObject().value("id")==id)return value.toObject();return {};}
    QJsonObject entity(int index) const{return parameters().value("entities").toArray().at(index).toObject();}
    QJsonObject point(const QJsonValue& id) const{return row(parameters().value("points").toArray(),id);}
    QPoint pixel(double u,double v){const auto result=call("view.projectSketchPoint",{{"sketchId",draft().value("id")},{"uMm",u},{"vMm",v}});
        const QPoint p(qRound(result.value("x").toDouble()),qRound(result.value("y").toDouble()));require(view_->rect().contains(p),"Projected point is within viewport");return p;}
    QPoint pointPixel(const QJsonValue& id){const auto p=point(id);require(!p.isEmpty(),"Authored point remains available");return pixel(p.value("uMm").toDouble(),p.value("vMm").toDouble());}
    void move(QPoint p){const QPoint global=view_->mapToGlobal(p);QMouseEvent event(QEvent::MouseMove,QPointF(p),QPointF(global),Qt::NoButton,Qt::NoButton,Qt::NoModifier);
        QApplication::sendEvent(view_,&event);QTest::qWait(25);}
    void click(QPoint p){require(QApplication::widgetAt(view_->mapToGlobal(p))==view_,"Viewport owns native click");move(p);QTest::mouseClick(view_,Qt::LeftButton,Qt::NoModifier,p);QTest::qWait(25);}
    void capture(const QString& name){call("capture.screenshot",{{"path",output_+"/"+name+".png"}});}
    void step(const QString& description){qInfo().noquote()<<description;if(recording_)call("recording.step",{{"text",description}});}
    void tangentAudit(){
        const auto before=parameters().value("constraints").toArray();int count=0;for(const auto& value:before)
            if(QStringList{"tangent","arcTangent"}.contains(value.toObject().value("type").toString()))++count;
        require(count==7,"Accepted construction has exactly seven Tangent relations");
        // The eighth illustrated location is the upper Arc/outgoing Line contact.
        const auto arc=entity(6),line=entity(7),center=point(arc.value("centerPointId"));
        require(arc.value("type")=="arc" && line.value("type")=="line" && line.value("startPointId")==arc.value("endPointId"),
            "Upper Arc/outgoing Line share the intended endpoint");
        auto* button=window_->findChild<QPushButton*>("sketch.constraint.tangent");require(button&&button->isVisible(),"Tangent control visible");
        QTest::mouseClick(button,Qt::LeftButton);require(session().value("constraintType")=="tangent","Native Tangent tool active");
        const double angle=(arc.value("startAngleDeg").toDouble()+arc.value("sweepAngleDeg").toDouble()*0.5)*std::acos(-1.0)/180;
        const double radius=arc.value("radiusMm").toDouble();click(pixel(center.value("uMm").toDouble()+radius*std::cos(angle),center.value("vMm").toDouble()+radius*std::sin(angle)));
        const auto a=point(line.value("startPointId")),b=point(line.value("endPointId"));
        click(pixel(a.value("uMm").toDouble()*0.45+b.value("uMm").toDouble()*0.55,a.value("vMm").toDouble()*0.45+b.value("vMm").toDouble()*0.55));
        const auto after=parameters().value("constraints").toArray();int tangents=0;bool joined=false;
        for(const auto& value:after){const auto c=value.toObject();if(QStringList{"tangent","arcTangent"}.contains(c.value("type").toString()))++tangents;
            const auto ids=c.value("entityIds").toArray();if(c.value("type")=="arcTangent" && ids.contains(arc.value("id")) && ids.contains(line.value("id")))joined=true;}
        require(tangents==8 && joined,"Native eighth Tangent is retained at the upper Arc/Line contact");
        capture("eight-tangents");
        require(QApplication::focusWidget()==view_,"Viewport naturally focused before Escape");QTest::keyClick(view_,Qt::Key_Escape);
    }
    void dimensionTool(){require(QApplication::focusWidget()==view_,"Viewport naturally focused before D");QTest::keyClick(view_,Qt::Key_D);
        require(session().value("tool")=="dimension","D leaves Dimension active");}
    QPoint curvePixel(const QJsonObject& curve){
        const auto center=point(curve.value("centerPointId"));const double radius=curve.value("radiusMm").toDouble();
        const double angle=curve.value("type")=="arc" ? (curve.value("startAngleDeg").toDouble()+0.5*curve.value("sweepAngleDeg").toDouble())*std::acos(-1.0)/180
            : curve.value("id")==entity(0).value("id") ? std::acos(-1.0) : 0;
        return pixel(center.value("uMm").toDouble()+radius*std::cos(angle),center.value("vMm").toDouble()+radius*std::sin(angle));
    }
    QJsonObject acceptPlaced(const QPoint& placement,const QString& value){
        move(placement);require(session().value("pendingDimension").toObject().value("phase")=="placing","Native Dimension placement preview ready");
        click(placement);auto* editor=window_->findChild<QLineEdit*>("sketch.inlineValue");require(editor&&editor->isVisible(),"Inline dimension editor visible");
        require(QApplication::focusWidget()==editor,"Inline dimension value naturally focused");
        if(!value.isEmpty()){QTest::keyClick(editor,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(editor,value);}
        QTest::keyClick(editor,Qt::Key_Return);require(!session().contains("pendingDimension"),"Dimension accepted atomically");
        const auto rows=parameters().value("dimensions").toArray();require(!rows.isEmpty(),"Dimension retained");return rows.last().toObject();
    }
    QJsonObject curveDimension(int index,double placeU,double placeV,const QString& value,const QString& kind){
        const auto before=parameters().value("dimensions").toArray().size();click(curvePixel(entity(index)));
        const auto result=acceptPlaced(pixel(placeU,placeV),value);
        require(parameters().value("dimensions").toArray().size()==before+1 && result.value("kind")==kind,"Native curve dimension kind and count");return result;
    }
    QJsonObject centerDimension(int first,int second,bool vertical,double offset){
        const auto a=entity(first),b=entity(second);const auto c1=point(a.value("centerPointId")),c2=point(b.value("centerPointId"));
        const auto before=parameters().value("dimensions").toArray().size();click(pointPixel(c1.value("id")));
        require(session().value("selection").toObject().value("pointIds").toArray().contains(c1.value("id")),"First native center pick retained");
        click(pointPixel(c2.value("id")));
        const auto picked=session().value("selection").toObject().value("pointIds").toArray();require(picked.contains(c1.value("id"))&&picked.contains(c2.value("id")),"Both native center picks retained");
        const double u=vertical ? offset>0 ? std::min(c1.value("uMm").toDouble(),c2.value("uMm").toDouble())-offset
            : std::max(c1.value("uMm").toDouble(),c2.value("uMm").toDouble())-offset
            : (c1.value("uMm").toDouble()+c2.value("uMm").toDouble())*.5;
        const double v=vertical ? (c1.value("vMm").toDouble()+c2.value("vMm").toDouble())*.5
            : offset>0 ? std::min(c1.value("vMm").toDouble(),c2.value("vMm").toDouble())-offset
            : std::max(c1.value("vMm").toDouble(),c2.value("vMm").toDouble())-offset;
        const auto result=acceptPlaced(pixel(u,v),{});require(parameters().value("dimensions").toArray().size()==before+1,"Point-pair dimension retained");
        require(result.value("kind")==(vertical ? "vertical" : "horizontal"),"Native label placement selects intended orientation");
        require(result.value("pointIds").toArray()==QJsonArray{c1.value("id"),c2.value("id")},"Center references remain exact and ordered");return result;
    }
    void editDimension(const QString& id,const QString& text){
        QJsonObject layout;for(const auto& value:view_->annotations())if(value.toObject().value("dimensionId")==id){layout=value.toObject();break;}
        require(!layout.isEmpty(),"Canvas dimension annotation is visible");const auto label=layout.value("label").toObject();
        const QPoint p(qRound(label.value("x").toDouble()),qRound(label.value("y").toDouble()));
        require(view_->rect().contains(p),"Dimension label is in viewport");move(p);QTest::mouseDClick(view_,Qt::LeftButton,Qt::NoModifier,p);
        auto* editor=window_->findChild<QLineEdit*>("sketch.inlineValue");require(editor&&editor->isVisible(),"Double-click opens dimension editor");
        require(QApplication::focusWidget()==editor,"Dimension editor naturally focused");QTest::keyClick(editor,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(editor,text);QTest::keyClick(editor,Qt::Key_Return);
        require(!session().contains("pendingDimension"),"Dimension edit accepted");
        const auto changed=row(parameters().value("dimensions").toArray(),id);require(std::abs(changed.value("valueMm").toDouble()-text.toDouble())<1e-6,"Dimension value matches published target");
    }
private slots:
    void nativeContinuation(){
        output_=qEnvironmentVariable("ALLOY_BASE_SKETCH_DIMENSIONS_EVIDENCE");QVERIFY(!output_.isEmpty());QVERIFY(QDir().mkpath(output_));
        const QString input=qEnvironmentVariable("ALLOY_BASE_SKETCH_INPUT");QVERIFY(!input.isEmpty());
        AlloyTheme::apply(*qApp);AlloyWindow window;window_=&window;window.resize(1500,950);window.show();QVERIFY(QTest::qWaitForWindowExposed(&window));
        for(auto* child:window.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(child))view_=candidate;QVERIFY(view_);
#ifdef Q_OS_MACOS
        QProcess activate;activate.start("osascript",{"-e",QString("tell application \"System Events\" to set frontmost of first application process whose unix id is %1 to true").arg(QCoreApplication::applicationPid())});QVERIFY(activate.waitForFinished(5000));QCOMPARE(activate.exitCode(),0);
#endif
        window.raise();window.activateWindow();window.windowHandle()->requestActivate();QApplication::setActiveWindow(&window);QTRY_VERIFY(window.isActiveWindow());
        try{
            call("document.open",{{"path",input},{"discard",true}});const auto sketch=call("document.inspect").value("authored").toObject().value("features").toArray().first().toObject();
            call("sketch.edit",{{"sketchId",sketch.value("id")}});call("sketch.lookAt");call("view.zoom",{{"factor",0.65}});
            const auto centered=pixel(35,-5);call("view.pan",{{"x",view_->width()/2-centered.x()},{"y",view_->height()/2-centered.y()}});
            if(qEnvironmentVariableIntValue("ALLOY_BASE_SKETCH_DIMENSIONS_RECORD")){
                call("recording.start",{{"path",output_+"/base-sketch-dimensions.mp4"},{"fps",15}});recording_=true;
            }
            step("6 · Audit seven existing tangencies; add the missing upper Arc/Line contact");tangentAudit();dimensionTool();
            step("7 · Place Ø31 on the left circle, Ø20 on the right, and R30 on the upper Arc");
            const auto left=curveDimension(0,-27,12,"31","diameter");
            const auto right=curveDimension(1,97,12,"20","diameter");
            const auto upper=curveDimension(6,37,35,"30","radius");
            const auto lower=parameters().value("dimensions").toArray().first().toObject();
            require(lower.value("kind")=="radius" && std::abs(lower.value("valueMm").toDouble()-10)<1e-6,"Earlier R10 is retained and already annotated");
            capture("curve-dimensions");
            step("7 · Place vertical and horizontal center dimensions without changing rough values");
            const auto lowerV=centerDimension(0,4,true,28),lowerH=centerDimension(0,4,false,25);
            const auto upperV=centerDimension(0,6,true,45),upperH=centerDimension(0,6,false,-34);
            const auto rightV=centerDimension(0,1,true,-22),rightH=centerDimension(0,1,false,-48);
            capture("six-center-dimensions");
            step("8 · Edit six placed labels in the published order");
            for(const auto& edit:QList<QPair<QString,QString>>{{rightV.value("id").toString(),"3.82"},{upperV.value("id").toString(),"29.101"},
                 {lowerV.value("id").toString(),"20.181"},{upperH.value("id").toString(),"40.357"},{rightH.value("id").toString(),"71.699"},
                 {lowerH.value("id").toString(),"48.12"}})editDimension(edit.first,edit.second);
            capture("dimensioned-sketch");
            const auto finalDraft=draft();const auto before=call("document.inspect").value("authored");
            QFile audit(output_+"/dimensioned-draft.json");require(audit.open(QIODevice::WriteOnly),"Can write draft audit");audit.write(QJsonDocument(finalDraft).toJson());audit.close();
            auto* finish=window.findChild<QPushButton*>("sketch.finish");require(finish&&finish->isVisible(),"Finish Sketch visible");QTest::mouseClick(finish,Qt::LeftButton);
            const auto final=call("document.inspect");call("history.undo");call("history.redo");const auto restored=call("document.inspect");
            require(restored.value("authored")==final.value("authored"),"Undo/redo restores authored dimensions and Tangents");
            require(restored.value("evaluation").toObject().value("sketches")==final.value("evaluation").toObject().value("sketches"),
                "Undo/redo restores evaluated sketch geometry");
            const auto saved=output_+"/base-sketch-dimensioned.alloy";call("document.save",{{"path",saved}});call("document.open",{{"path",saved}});
            require(call("document.inspect").value("authored")==final.value("authored"),"Saved dimensioned sketch reopens exactly");
            QFile result(output_+"/result.json");require(result.open(QIODevice::WriteOnly),"Can write result");result.write(QJsonDocument(QJsonObject{{"status","passed-pending-independent-review"},{"authored",final.value("authored")},
                {"evaluation",final.value("evaluation")},{"input","Saved TUT025 fixture; actual Qt Tangent and D selections, placements, typed values and label double-click edits; API open/view projection/inspection/save/history"},
                {"sourceInput",input},{"initialAuthored",before},{"curveDimensionIds",QJsonArray{left.value("id"),right.value("id"),upper.value("id"),lower.value("id")}}}).toJson());
            if(recording_){call("recording.stop");recording_=false;}window.close();
        }catch(const std::exception& error){if(recording_){window.executeApi("recording.stop",{});recording_=false;}
            window.executeApi("capture.screenshot",{{"path",output_+"/failure.png"}});QFAIL(error.what());}
    }
};
QTEST_MAIN(BaseSketchDimensionsTutorial)
#include "base_sketch_dimensions_tutorial.moc"
