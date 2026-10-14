// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QDir>
#include <QJsonDocument>
#include <QMouseEvent>
#include <QProcess>
#include <QTest>
#include <QWindow>

class SketchLineTangentArcControls final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& name,const QJsonObject& args={}){
        const auto response=window.executeApi(name,args);
        if(!response.value("ok").toBool())qFatal("%s",QJsonDocument(response).toJson(QJsonDocument::Compact).constData());
        return response.value("result").toObject();
    }
    QJsonObject session(AlloyWindow& window){return window.state().value("activeSession").toObject();}
    QJsonObject parameters(AlloyWindow& window){return session(window).value("draftSketch").toObject().value("parameters").toObject();}
private slots:
    void dragCommitAndContinue(){
        const auto out=qEnvironmentVariable("ALLOY_LINE_TANGENT_ARC_EVIDENCE");QVERIFY(!out.isEmpty());QVERIFY(QDir().mkpath(out));
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1500,950);window.show();QVERIFY(QTest::qWaitForWindowExposed(&window));
        ReferenceView* view=nullptr;for(auto* child:window.findChildren<QWidget*>())if(auto* found=dynamic_cast<ReferenceView*>(child))view=found;QVERIFY(view);
        call(window,"sketch.begin",{{"plane","XY"},{"units","mm"},{"name","Tangent arc chain"}});
        call(window,"sketch.lookAt");call(window,"view.zoom",{{"factor",0.65}});
#ifdef Q_OS_MACOS
        QProcess activate;activate.start("osascript",{"-e",QString("tell application \"System Events\" to set frontmost of first application process whose unix id is %1 to true").arg(QCoreApplication::applicationPid())});
        QVERIFY(activate.waitForFinished(5000));QCOMPARE(activate.exitCode(),0);
#endif
        window.raise();window.activateWindow();window.windowHandle()->requestActivate();QApplication::setActiveWindow(&window);
        QTRY_VERIFY(window.isActiveWindow());
        call(window,"sketch.tool",{{"tool","line"}});
        auto pixel=[&](double u,double v){const auto p=call(window,"view.projectSketchPoint",{{"sketchId",session(window).value("draftSketch").toObject().value("id")},{"uMm",u},{"vMm",v}});
            const QPoint point(qRound(p.value("x").toDouble()),qRound(p.value("y").toDouble()));
            if(!view->rect().contains(point))qFatal("Projected tangent arc point outside viewport");return point;};
        auto move=[&](QPoint point,Qt::MouseButtons buttons){const QPoint global=view->mapToGlobal(point);
            QMouseEvent event(QEvent::MouseMove,QPointF(point),QPointF(global),Qt::NoButton,buttons,Qt::NoModifier);
            QApplication::sendEvent(view,&event);QTest::qWait(70);};
        const auto p0=pixel(0,0),p1=pixel(40,0),p2=pixel(70,-20),p3=pixel(100,-10);
        QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,p0);
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,p1);
        auto entities=parameters(window).value("entities").toArray();QCOMPARE(entities.size(),1);
        const auto incoming=entities.last().toObject();
        QCOMPARE(session(window).value("firstPoint").toObject().value("target").toObject().value("pointId"),incoming.value("endPointId"));
        QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
        QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,p1);
        QTest::mouseRelease(view,Qt::LeftButton,Qt::NoModifier,p1);
        QVERIFY(session(window).value("pendingLineArc").toObject().isEmpty());
        QCOMPARE(parameters(window).value("entities").toArray().size(),1);
        QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,p1);
        move(p2,Qt::LeftButton);
        QVERIFY(session(window).value("pendingLineArc").toObject().value("valid").toBool());
        QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QTest::keyClick(view,Qt::Key_Escape);
        QVERIFY(session(window).value("pendingLineArc").toObject().isEmpty());
        QTest::mouseRelease(view,Qt::LeftButton,Qt::NoModifier,p2);
        QCOMPARE(parameters(window).value("entities").toArray().size(),1);
        QCOMPARE(session(window).value("tool"),QJsonValue("line"));
        QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,p1);
        QVERIFY(!session(window).value("pendingLineArc").toObject().isEmpty());
        QCOMPARE(parameters(window).value("entities").toArray().size(),1);
        move(p2,Qt::LeftButton);
        QVERIFY(session(window).value("pendingLineArc").toObject().value("valid").toBool());
        QCOMPARE(parameters(window).value("entities").toArray().size(),1);
        call(window,"capture.screenshot",{{"path",out+"/held-arc-preview.png"}});
        QTest::mouseRelease(view,Qt::LeftButton,Qt::NoModifier,p2);
        entities=parameters(window).value("entities").toArray();QCOMPARE(entities.size(),2);
        const auto arc=entities.last().toObject();QCOMPARE(arc.value("type"),QJsonValue("arc"));
        QCOMPARE(arc.value("startPointId"),incoming.value("endPointId"));
        QCOMPARE(session(window).value("tool"),QJsonValue("line"));
        QCOMPARE(session(window).value("firstPoint").toObject().value("target").toObject().value("pointId"),arc.value("endPointId"));
        int incomingTangents=0;for(const auto& value:parameters(window).value("constraints").toArray()){
            const auto relation=value.toObject();if(relation.value("type")=="arcTangent"
                && relation.value("entityIds").toArray()==QJsonArray{arc.value("id"),incoming.value("id")})++incomingTangents;
        }QCOMPARE(incomingTangents,1);
        move(p3,Qt::NoButton);QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,p3);
        entities=parameters(window).value("entities").toArray();QCOMPARE(entities.size(),3);
        const auto outgoing=entities.last().toObject();QCOMPARE(outgoing.value("type"),QJsonValue("line"));
        QCOMPARE(outgoing.value("startPointId"),arc.value("endPointId"));
        call(window,"capture.screenshot",{{"path",out+"/connected-chain.png"}});
        QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QTest::keyClick(view,Qt::Key_Escape);
        call(window,"sketch.finish");const auto authored=call(window,"document.inspect").value("authored");
        call(window,"history.undo");QVERIFY(call(window,"document.inspect").value("authored")!=authored);
        call(window,"history.redo");QCOMPARE(call(window,"document.inspect").value("authored"),authored);
        call(window,"document.save",{{"path",out+"/tangent-arc-chain.alloy"}});
        call(window,"document.new",{{"discard",false}});call(window,"document.open",{{"path",out+"/tangent-arc-chain.alloy"}});
        QCOMPARE(call(window,"document.inspect").value("authored"),authored);
        window.close();
    }
};
QTEST_MAIN(SketchLineTangentArcControls)
#include "sketch_line_tangent_arc_controls.moc"
