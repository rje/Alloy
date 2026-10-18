// SPDX-License-Identifier: MIT
#include "AlloyTheme.h"
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QAction>
#include <QContextMenuEvent>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonDocument>
#include <QLabel>
#include <QMenu>
#include <QProcess>
#include <QTest>
#include <QWindow>

class SketchOverlapSelectionControls final:public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& w,const QString& name,const QJsonObject& args={}) {
        auto r=w.execute(name,args);const auto initial=r.value("result").toObject();QElapsedTimer timer;timer.start();
        while(r.value("completion")=="pending") {
            if(timer.elapsed()>30000)qFatal("Timeout: %s",qPrintable(name));
            QTest::qWait(20);r=w.execute("command.status",{{"session",initial.value("id")},{"generation",initial.value("previewGeneration")}});
        }
        if(!r.value("ok").toBool())qFatal("%s: %s",qPrintable(name),QJsonDocument(r).toJson().constData());
        return r.value("result").toObject();
    }
private slots:
    void overlappingCurves() {
        const auto fixture=qEnvironmentVariable("ALLOY_OVERLAP_DESIGN"),out=qEnvironmentVariable("ALLOY_OVERLAP_EVIDENCE");
        if(fixture.isEmpty() || out.isEmpty())QSKIP("Set ALLOY_OVERLAP_DESIGN and ALLOY_OVERLAP_EVIDENCE");
        QDir().mkpath(out);AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1500,950);w.show();QVERIFY(QTest::qWaitForWindowExposed(&w));
#ifdef Q_OS_MACOS
        QProcess activation;activation.start("osascript",{"-e",QString("tell application \"System Events\" to set frontmost of first application process whose unix id is %1 to true").arg(QCoreApplication::applicationPid())});QVERIFY(activation.waitForFinished(5000));QCOMPARE(activation.exitCode(),0);
#endif
        w.raise();w.activateWindow();w.windowHandle()->requestActivate();QApplication::setActiveWindow(&w);QTRY_VERIFY(w.isActiveWindow());
        ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;QVERIFY(view);
        call(w,"document.open",{{"path",fixture}});QJsonObject sketch;
        for(const auto& value:call(w,"document.inspect").value("authored").toObject().value("features").toArray())if(value.toObject().value("type")=="sketch")sketch=value.toObject();
        QVERIFY(!sketch.isEmpty());const auto sketchId=sketch.value("id");call(w,"sketch.edit",{{"sketchId",sketchId}});
        const auto session=[&]{return w.state().value("activeSession").toObject();};const auto draft=[&]{return session().value("draftSketch").toObject();};
        const auto context=session().value("projectionContext").toObject();
        const auto line=context.value("lines").toArray()[0].toObject(),circle=context.value("circles").toArray()[0].toObject();
        call(w,"sketch.line",{{"x1",line.value("u1Mm")},{"y1",line.value("v1Mm")},{"x2",line.value("u2Mm")},{"y2",line.value("v2Mm")},{"units","mm"}});
        const auto authoredLine=draft().value("parameters").toObject().value("entities").toArray().last().toObject().value("id");
        call(w,"sketch.addCircle",{{"centerX",circle.value("uMm")},{"centerY",circle.value("vMm")},{"diameter",2*circle.value("radiusMm").toDouble()},{"units","mm"}});
        const auto authoredCircle=draft().value("parameters").toObject().value("entities").toArray().last().toObject().value("id");
        call(w,"sketch.finish");call(w,"document.save",{{"path",out+"/coincident-before.alloy"}});
        const auto sources=call(w,"document.inspect").value("evaluation").toObject().value("bodies");
        call(w,"sketch.edit",{{"sketchId",sketchId}});call(w,"sketch.lookAt");call(w,"view.fit");call(w,"sketch.tool",{{"tool","select"}});
        const auto original=draft();
        const auto pixel=[&](double u,double v){const auto p=call(w,"view.projectSketchPoint",{{"sketchId",sketchId},{"uMm",u},{"vMm",v}});return QPoint(qRound(p.value("x").toDouble()),qRound(p.value("y").toDouble()));};
        const QPoint linePixel=pixel((line.value("u1Mm").toDouble()+line.value("u2Mm").toDouble())/2,(line.value("v1Mm").toDouble()+line.value("v2Mm").toDouble())/2);
        // Avoid the intrinsic projected-circle diameter annotation at the right rim.
        const QPoint circlePixel=pixel(circle.value("uMm").toDouble(),circle.value("vMm").toDouble()-circle.value("radiusMm").toDouble());
        QTRY_VERIFY(view->hasFocus());QTest::qWait(200);
        const auto candidates=[&](QPoint p){return call(w,"sketch.selectionCandidates",{{"x",p.x()},{"y",p.y()}}).value("candidates").toArray();};
        QCOMPARE(candidates(linePixel).size(),2);QCOMPARE(candidates(circlePixel).size(),2);
        for(const auto& p:{linePixel,circlePixel})for(const auto& value:candidates(p)) {
            const auto row=value.toObject();QVERIFY(!row.value("entityId").toString().isEmpty());
            if(row.value("projected").toBool())QVERIFY(!row.value("source").toObject().isEmpty());
        }
        // An ordinary quick click remains deterministic and creates no history.
        const auto history=session().value("draftHistory");QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,linePixel);
        QVERIFY(!session().value("selection").toObject().value("entityIds").toArray().isEmpty());QCOMPARE(session().value("draftHistory"),history);QVERIFY(!QApplication::activePopupWidget());QVERIFY(view->hasFocus());
        // Holding and cancelling does not mutate even the existing selection.
        const auto selection=session().value("selection");QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,linePixel);QTest::qWait(600);
        auto* cancelMenu=view->findChild<QMenu*>("sketch.selectOther");QVERIFY(cancelMenu);QTRY_COMPARE(QApplication::activePopupWidget(),cancelMenu);
        QTest::mouseRelease(cancelMenu,Qt::LeftButton,Qt::NoModifier,QPoint(2,2));QTest::keyClick(cancelMenu,Qt::Key_Escape);QTRY_VERIFY(!QApplication::activePopupWidget());
        QCOMPARE(session().value("selection"),selection);QCOMPARE(draft(),original);QVERIFY(view->hasFocus());
        QJsonArray results;
        struct Target {QString name;QJsonValue id;QPoint pixel;bool projected;bool contextDelete;};
        const QList<Target> targets{{"authored-line",authoredLine,linePixel,false,true},{"projected-line",line.value("id"),linePixel,true,false},
            {"authored-circle",authoredCircle,circlePixel,false,true},{"projected-circle",circle.value("id"),circlePixel,true,false}};
        for(const auto& target:targets) {
            // Stage a readable local view before this independent input workflow.
            call(w,"view.zoom",{{"factor",target.name.contains("circle") ? 6.0 : 2.0},{"x",target.pixel.x()},{"y",target.pixel.y()}});
            QTest::qWait(100);
            const QPoint targetPixel=target.name.contains("circle") ? pixel(circle.value("uMm").toDouble(),circle.value("vMm").toDouble()-circle.value("radiusMm").toDouble()) : pixel((line.value("u1Mm").toDouble()+line.value("u2Mm").toDouble())/2,(line.value("v1Mm").toDouble()+line.value("v2Mm").toDouble())/2);
            QCOMPARE(candidates(targetPixel).size(),2);
            QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,targetPixel);QTest::qWait(600);
            auto* menu=view->findChild<QMenu*>("sketch.selectOther");QVERIFY(menu);QTRY_COMPARE(QApplication::activePopupWidget(),menu);
            QAction* chosen=nullptr;for(auto* action:menu->actions())if(action->data().toJsonObject().value("entityId")==target.id)chosen=action;
            QVERIFY(chosen);QVERIFY(chosen->text().contains(target.projected ? "Projected" : "Authored"));QVERIFY(chosen->text().contains(target.id.toString().first(8)));
            menu->grab().save(out+"/"+target.name+"-chooser.png");
            QTest::mouseRelease(menu,Qt::LeftButton,Qt::NoModifier,QPoint(2,2));
            QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,menu->actionGeometry(chosen).center());
            QTRY_VERIFY(!QApplication::activePopupWidget());QTRY_COMPARE(session().value("selection").toObject().value("entityIds").toArray(),QJsonArray{target.id});
            QVERIFY(view->hasFocus());auto* cue=w.findChild<QLabel*>("sketch.selectionIdentity");QVERIFY(cue&&cue->isVisible());QVERIFY(cue->text().contains(target.id.toString()));
            QVERIFY(cue->text().contains(target.projected ? "Projected" : "Authored"));call(w,"capture.screenshot",{{"path",out+"/"+target.name+"-selected.png"}});
            const auto beforeHistory=session().value("draftHistory").toObject().value("index").toInt();
            if(target.contextDelete) {
                QContextMenuEvent event(QContextMenuEvent::Mouse,targetPixel,view->mapToGlobal(targetPixel));QApplication::sendEvent(view,&event);
                auto* contextMenu=view->findChild<QMenu*>("context.sketchGeometry");QVERIFY(contextMenu);QTRY_COMPARE(QApplication::activePopupWidget(),contextMenu);
                // A right click must preserve the deliberately chosen authored target.
                QCOMPARE(session().value("selection").toObject().value("entityIds").toArray(),QJsonArray{target.id});
                auto* remove=contextMenu->findChild<QAction*>("sketch.action.deleteGeometry");QVERIFY(remove);QTest::mouseClick(contextMenu,Qt::LeftButton,Qt::NoModifier,contextMenu->actionGeometry(remove).center());
                QTRY_VERIFY(!QApplication::activePopupWidget());
            } else QTest::keyClick(view,Qt::Key_Delete);
            QTRY_VERIFY(draft()!=original);const auto deleted=draft();QCOMPARE(session().value("draftHistory").toObject().value("index").toInt(),beforeHistory+1);
            const auto p=deleted.value("parameters").toObject(),originalP=original.value("parameters").toObject();
            auto expected=originalP;const auto key=target.projected ? (target.name.contains("circle") ? "circleProjections" : "projections") : "entities";
            QJsonArray kept;for(const auto& row:expected.value(key).toArray())if(row.toObject().value("id")!=target.id)kept.append(row);
            if(kept.isEmpty())expected.remove(key);else expected[key]=kept;
            // Deleting an authored curve also retires only its orphan points.
            if(!target.projected) {
                const auto entity=[&]{for(const auto& v:originalP.value("entities").toArray())if(v.toObject().value("id")==target.id)return v.toObject();return QJsonObject{};}();
                QSet<QString> removed;for(const auto& field:{"startPointId","endPointId","centerPointId"})if(entity.contains(field))removed.insert(entity.value(field).toString());
                QJsonArray points;for(const auto& row:originalP.value("points").toArray())if(!removed.contains(row.toObject().value("id").toString()))points.append(row);expected["points"]=points;
            }
            // The existing Delete authority canonicalizes absent annotation arrays.
            if(!target.projected && !expected.contains("constraints"))expected["constraints"]=QJsonArray{};
            QCOMPARE(p,expected);call(w,"capture.screenshot",{{"path",out+"/"+target.name+"-deleted.png"}});
            call(w,"history.undo");QCOMPARE(draft(),original);QCOMPARE(session().value("projectionContext"),context);call(w,"history.redo");QCOMPARE(draft(),deleted);
            call(w,"sketch.finish");const auto inspection=call(w,"document.inspect");QCOMPARE(inspection.value("evaluation").toObject().value("bodies"),sources);
            const auto path=out+"/"+target.name+"-deleted.alloy";call(w,"document.save",{{"path",path}});call(w,"document.open",{{"path",path},{"discard",true}});
            QCOMPARE(call(w,"document.inspect").value("authored"),inspection.value("authored"));QCOMPARE(call(w,"document.inspect").value("evaluation").toObject().value("bodies"),sources);
            call(w,"capture.screenshot",{{"path",out+"/"+target.name+"-reopened.png"}});
            results.append(QJsonObject{{"target",target.name},{"id",target.id},{"exactSurvivors",true},{"exactUndoRedoReopen",true},{"sourceBodiesExact",true}});
            // Reload only between complete workflows, never between user input steps.
            call(w,"document.open",{{"path",out+"/coincident-before.alloy"},{"discard",true}});call(w,"sketch.edit",{{"sketchId",sketchId}});call(w,"sketch.lookAt");call(w,"view.fit");QVERIFY(view->hasFocus());
        }
        // Hidden projections must not remain chooser candidates.
        call(w,"sketch.displayOptions",{{"showProjectedGeometries",false}});QCOMPARE(candidates(linePixel).size(),1);QCOMPARE(candidates(circlePixel).size(),1);
        call(w,"sketch.displayOptions",{{"showProjectedGeometries",true}});
        const auto gpu=w.state().value("viewport").toObject().value("gpu").toObject();QVERIFY(gpu.value("hardwareAccelerated").toBool());
        QFile receipt(out+"/receipt.json");QVERIFY(receipt.open(QIODevice::WriteOnly));receipt.write(QJsonDocument(QJsonObject{{"passed",true},{"targets",results},{"gpu",gpu},{"original",original},{"projectionContext",context},{"shortClickAndEscape",true},{"hiddenProjectionExcluded",true}}).toJson());
        call(w,"sketch.cancel");call(w,"document.new",{{"discard",true}});w.close();
    }
};
int main(int argc,char** argv){QApplication app(argc,argv);SketchOverlapSelectionControls test;return QTest::qExec(&test,argc,argv);}
#include "sketch_overlap_selection_controls.moc"
