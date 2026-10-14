// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QDir>
#include <QJsonDocument>
#include <QMouseEvent>
#include <QTest>
#include <QWindow>
#include <cmath>

class SketchExtendControls final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& args={}){
        const auto response=window.executeApi(command,args);
        if(!response.value("ok").toBool())qFatal("%s",QJsonDocument(response).toJson(QJsonDocument::Compact).constData());
        return response.value("result").toObject();
    }
    QJsonObject session(AlloyWindow& window){return window.state().value("activeSession").toObject();}
private slots:
    void extendCornerAndExtrude(){
        const auto out=qEnvironmentVariable("ALLOY_SKETCH_EXTEND_EVIDENCE");QVERIFY(!out.isEmpty());QVERIFY(QDir().mkpath(out));
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1500,950);window.show();QVERIFY(QTest::qWaitForWindowExposed(&window));
        ReferenceView* view=nullptr;for(auto* child:window.findChildren<QWidget*>())if(auto* found=dynamic_cast<ReferenceView*>(child))view=found;QVERIFY(view);
        window.raise();window.activateWindow();window.windowHandle()->requestActivate();QApplication::setActiveWindow(&window);
        QTRY_VERIFY(window.isActiveWindow());
        call(window,"sketch.begin",{{"plane","XY"},{"units","mm"},{"name","Open corner"}});
        for(const auto& line:QVector<QJsonArray>{QJsonArray{0,0,20,0},QJsonArray{20,0,20,8},QJsonArray{20,10,0,10},QJsonArray{0,10,0,0}})
            call(window,"sketch.line",{{"x1",line[0]},{"y1",line[1]},{"x2",line[2]},{"y2",line[3]},{"units","mm"}});
        call(window,"sketch.lookAt");call(window,"view.zoom",{{"factor",1.5}});
        const auto original=session(window).value("draftSketch").toObject();const auto sketchId=original.value("id").toString();
        const auto source=original.value("parameters").toObject().value("entities").toArray()[1].toObject();
        auto pixel=[&](double u,double v){const auto p=call(window,"view.projectSketchPoint",{{"sketchId",sketchId},{"uMm",u},{"vMm",v}});
            return QPoint(qRound(p.value("x").toDouble()),qRound(p.value("y").toDouble()));};
        call(window,"sketch.tool",{{"tool","extend"}});
        const auto pick=pixel(20,7.5);QVERIFY(view->rect().contains(pick));
        QCOMPARE(QApplication::widgetAt(view->mapToGlobal(pick)),static_cast<QWidget*>(view));
        QMouseEvent hover(QEvent::MouseMove,QPointF(pick),QPointF(view->mapToGlobal(pick)),Qt::NoButton,Qt::NoButton,Qt::NoModifier);
        QApplication::sendEvent(view,&hover);QTest::qWait(50); // Let the palette and GPU finish the hover frame.
        const auto preview=session(window).value("pendingExtend").toObject();QVERIFY(!preview.isEmpty());
        const auto segment=preview.value("segment").toObject();QCOMPARE(segment.value("entityId"),source.value("id"));
        QVERIFY(std::abs(segment.value("v1Mm").toDouble()-8)<1e-6 && std::abs(segment.value("v2Mm").toDouble()-10)<1e-6);
        QCOMPARE(session(window).value("draftSketch"),QJsonValue(original));
        call(window,"capture.screenshot",{{"path",out+"/extend-hover.png"}});
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pick);
        const auto edited=session(window);QCOMPARE(edited.value("tool"),QJsonValue("extend"));
        const auto draft=edited.value("draftSketch").toObject();QCOMPARE(draft.value("parameters").toObject().value("entities").toArray()[1].toObject().value("id"),source.value("id"));
        QCOMPARE(call(window,"document.inspect").value("authored").toObject().value("features").toArray().size(),0);
        call(window,"capture.screenshot",{{"path",out+"/extended-profile.png"}});
        QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QTest::keyClick(view,Qt::Key_Return);
        QCOMPARE(session(window).value("tool"),QJsonValue("select"));
        call(window,"sketch.finish");const auto sketch=call(window,"document.inspect").value("evaluation").toObject().value("sketches").toArray().first().toObject();
        const auto profiles=sketch.value("profiles").toArray();QCOMPARE(profiles.size(),1);
        QVERIFY(std::abs(profiles.first().toObject().value("areaMm2").toDouble()-200)<1e-6);
        call(window,"solid.extrude",{{"sketchId",sketchId},{"profileId",profiles.first().toObject().value("profileId")},{"distance",5},{"units","mm"}});
        const auto final=call(window,"document.inspect");const auto bodies=final.value("evaluation").toObject().value("bodies").toArray();QCOMPARE(bodies.size(),1);
        const auto body=bodies.first().toObject();QVERIFY(body.value("valid").toBool() && body.value("closed").toBool());
        QVERIFY(std::abs(body.value("volumeMm3").toDouble()-1000)<1e-5);
        call(window,"capture.screenshot",{{"path",out+"/extended-solid.png"}});
        call(window,"history.undo");QCOMPARE(call(window,"document.inspect").value("authored").toObject().value("features").toArray().size(),1);
        call(window,"history.redo");QCOMPARE(call(window,"document.inspect").value("authored"),final.value("authored"));
        const auto saved=out+"/extended-corner.alloy";call(window,"document.save",{{"path",saved}});
        call(window,"document.open",{{"path",saved},{"discard",true}});
        QCOMPARE(call(window,"document.inspect").value("authored"),final.value("authored"));window.close();
    }
};
QTEST_MAIN(SketchExtendControls)
#include "sketch_extend_controls.moc"
