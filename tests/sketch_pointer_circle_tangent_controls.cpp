// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include "SketchGeometry.h"
#include <QApplication>
#include <QDir>
#include <QJsonDocument>
#include <QLabel>
#include <QMouseEvent>
#include <QProcess>
#include <QTest>
#include <QWindow>
#include <cmath>

class SketchPointerCircleTangentControls final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& name,const QJsonObject& args={}){
        const auto response=window.executeApi(name,args);
        if(!response.value("ok").toBool())qFatal("%s",QJsonDocument(response).toJson(QJsonDocument::Compact).constData());
        return response.value("result").toObject();
    }
    QJsonObject session(AlloyWindow& window){return window.state().value("activeSession").toObject();}
    QJsonObject parameters(AlloyWindow& window){return session(window).value("draftSketch").toObject().value("parameters").toObject();}
    QJsonObject byId(const QJsonArray& rows,const QJsonValue& id){for(const auto& value:rows)if(value.toObject().value("id")==id)return value.toObject();return {};}
    bool hasRelation(const QJsonArray& rows,const QString& kind,const QJsonArray& ids){for(const auto& value:rows){const auto row=value.toObject();
        if(row.value("type")==kind && row.value("entityIds").toArray()==ids)return true;}return false;}
private slots:
    void shiftFromAndOrdinaryOnto(){
        const auto out=qEnvironmentVariable("ALLOY_POINTER_CIRCLE_TANGENT_EVIDENCE");QVERIFY(!out.isEmpty());QVERIFY(QDir().mkpath(out));
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1500,950);window.show();QVERIFY(QTest::qWaitForWindowExposed(&window));
        ReferenceView* view=nullptr;for(auto* child:window.findChildren<QWidget*>())if(auto* found=dynamic_cast<ReferenceView*>(child))view=found;QVERIFY(view);
        call(window,"sketch.begin",{{"plane","XY"},{"units","mm"},{"name","Circle contact lines"}});
        call(window,"sketch.addCircle",{{"centerX",-40},{"centerY",0},{"diameter",20},{"units","mm"}});
        call(window,"sketch.addCircle",{{"centerX",40},{"centerY",0},{"diameter",20},{"units","mm"}});
        const auto circles=parameters(window).value("entities").toArray();const auto left=circles[0].toObject().value("id"),right=circles[1].toObject().value("id");
        call(window,"sketch.lookAt");call(window,"view.zoom",{{"factor",0.65}});
#ifdef Q_OS_MACOS
        QProcess activate;activate.start("osascript",{"-e",QString("tell application \"System Events\" to set frontmost of first application process whose unix id is %1 to true").arg(QCoreApplication::applicationPid())});
        QVERIFY(activate.waitForFinished(5000));QCOMPARE(activate.exitCode(),0);
#endif
        window.raise();window.activateWindow();window.windowHandle()->requestActivate();QApplication::setActiveWindow(&window);
        QTRY_VERIFY(window.isActiveWindow());
        call(window,"sketch.tool",{{"tool","line"}});
        auto pixel=[&](double u,double v){const auto projected=call(window,"view.projectSketchPoint",{{"sketchId",session(window).value("draftSketch").toObject().value("id")},{"uMm",u},{"vMm",v}});
            const QPoint p(qRound(projected.value("x").toDouble()),qRound(projected.value("y").toDouble()));
            if(!view->rect().contains(p))qFatal("Projected circle contact is outside the viewport");return p;};
        auto move=[&](QPoint point,Qt::KeyboardModifiers modifiers){const QPoint global=view->mapToGlobal(point);
            QMouseEvent event(QEvent::MouseMove,QPointF(point),QPointF(global),Qt::NoButton,Qt::NoButton,modifiers);
            QApplication::sendEvent(view,&event);QTest::qWait(80);};
        const auto start=pixel(-40,10),rough=pixel(-5,12);
        move(start,Qt::NoModifier);QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,start);
        QCOMPARE(session(window).value("firstPoint").toObject().value("target").toObject().value("entityId"),left);
        QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
        move(rough,Qt::ShiftModifier);
        auto* cue=window.findChild<QLabel*>("sketch.lineLockCue");QVERIFY(cue&&cue->isVisible());QCOMPARE(cue->text(),QString("Tangent"));
        call(window,"capture.screenshot",{{"path",out+"/shift-from-preview.png"}});
        QTest::mouseClick(view,Qt::LeftButton,Qt::ShiftModifier,rough);
        auto p=parameters(window);auto entities=p.value("entities").toArray(),relations=p.value("constraints").toArray();
        QCOMPARE(entities.size(),3);const auto firstLine=entities.last().toObject();
        QVERIFY(hasRelation(relations,"tangent",{left,firstLine.value("id")}));
        const auto firstEnd=byId(p.value("points").toArray(),firstLine.value("endPointId"));QVERIFY(!firstEnd.isEmpty());
        const auto candidates=SketchGeometry::pointerCircleTangents(p,right.toString(),firstEnd.value("uMm").toDouble(),firstEnd.value("vMm").toDouble());
        QCOMPARE(candidates.size(),2);
        auto target=candidates[0].toObject();if(std::hypot(candidates[1].toObject().value("uMm").toDouble()-40,candidates[1].toObject().value("vMm").toDouble()-10)
            <std::hypot(target.value("uMm").toDouble()-40,target.value("vMm").toDouble()-10))target=candidates[1].toObject();
        const auto targetPixel=pixel(target.value("uMm").toDouble(),target.value("vMm").toDouble());
        QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));move(targetPixel,Qt::NoModifier);
        QCOMPARE(session(window).value("snap").toObject().value("target").toObject().value("entityId"),right);
        QVERIFY(cue->isVisible());QCOMPARE(cue->text(),QString("Tangent"));
        call(window,"capture.screenshot",{{"path",out+"/ordinary-onto-preview.png"}});
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,targetPixel);
        p=parameters(window);entities=p.value("entities").toArray();relations=p.value("constraints").toArray();
        QCOMPARE(entities.size(),4);const auto secondLine=entities.last().toObject();
        QVERIFY(hasRelation(relations,"tangent",{right,secondLine.value("id")}));
        bool leftContact=false,rightContact=false;for(const auto& value:relations){const auto row=value.toObject();
            if(row.value("type")!="pointOnCurve")continue;
            if(row.value("entityIds").toArray()==QJsonArray{left} && row.value("pointIds").toArray()==QJsonArray{firstLine.value("startPointId")})leftContact=true;
            if(row.value("entityIds").toArray()==QJsonArray{right} && row.value("pointIds").toArray()==QJsonArray{secondLine.value("endPointId")})rightContact=true;
        }QVERIFY(leftContact&&rightContact);
        call(window,"capture.screenshot",{{"path",out+"/retained-contacts.png"}});
        QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QTest::keyClick(view,Qt::Key_Escape);
        auto* finish=window.findChild<QWidget*>("sketch.finish");QVERIFY(finish);QTest::mouseClick(finish,Qt::LeftButton,Qt::NoModifier,finish->rect().center());
        const auto authored=call(window,"document.inspect").value("authored");
        call(window,"history.undo");QVERIFY(call(window,"document.inspect").value("authored")!=authored);
        call(window,"history.redo");QCOMPARE(call(window,"document.inspect").value("authored"),authored);
        call(window,"document.save",{{"path",out+"/circle-contact-lines.alloy"}});
        call(window,"document.new",{{"discard",false}});call(window,"document.open",{{"path",out+"/circle-contact-lines.alloy"}});
        QCOMPARE(call(window,"document.inspect").value("authored"),authored);
        window.close();
    }
};
QTEST_MAIN(SketchPointerCircleTangentControls)
#include "sketch_pointer_circle_tangent_controls.moc"
