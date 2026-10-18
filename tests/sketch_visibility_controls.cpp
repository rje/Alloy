// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QCheckBox>
#include <QLineEdit>
#include <QScrollBar>
#include <QFile>
#include <QJsonDocument>
#include <QProcess>
#include <QPushButton>
#include <QScrollArea>
#include <QTest>
#include <QTreeWidget>
#include <QWindow>

class SketchVisibilityControls final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& w,const QString& c,const QJsonObject& a={}){
        auto r=w.executeApi(c,a);const auto initial=r.value("result").toObject();
        for(int i=0;r.value("completion")=="pending" && i<400;++i){QTest::qWait(25);r=w.executeApi("command.status",{{"session",initial.value("id")},{"generation",initial.value("previewGeneration")}});}
        if(!r.value("ok").toBool() || r.value("completion")=="pending")qFatal("%s",QJsonDocument(r).toJson().constData());return r.value("result").toObject();
    }
private slots:
    void settledReopenCapture(){
        const auto output=qEnvironmentVariable("ALLOY_VISIBILITY_EVIDENCE");
        if(output.isEmpty())QSKIP("Set ALLOY_VISIBILITY_EVIDENCE to existing native evidence with visibility.alloy");
        AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1500,1000);w.show();QVERIFY(QTest::qWaitForWindowExposed(&w));
        call(w,"document.open",{{"path",output+"/visibility.alloy"}});
        const auto doc=call(w,"document.inspect");QJsonObject sketch;
        for(const auto& row:doc.value("authored").toObject().value("features").toArray())if(row.toObject().value("name")=="Mixed entity roles")sketch=row.toObject();
        QVERIFY(!sketch.isEmpty());call(w,"sketch.edit",{{"sketchId",sketch.value("id")}});call(w,"sketch.lookAt");
        auto* name=w.findChild<QLineEdit*>("sketch.name");auto* viewport=w.findChild<QWidget*>("sketchScrollViewport");QVERIFY(name&&viewport);
        // Observe the deferred dock/layout work after edit. No scroll/focus repair.
        QTRY_VERIFY_WITH_TIMEOUT(name->isVisible() && name->mapTo(viewport,QPoint()).y()>=0 && name->mapTo(viewport,QPoint()).y()<60,3000);
        QTest::qWait(100);
        QVERIFY(name->mapTo(viewport,QPoint()).y()<60);
        call(w,"capture.screenshot",{{"path",output+"/all-visible-settled.png"}});
        QFile receipt(output+"/settled-state.json");QVERIFY(receipt.open(QIODevice::WriteOnly));receipt.write(QJsonDocument(QJsonObject{{"nameY",name->mapTo(viewport,QPoint()).y()},{"nameHeight",name->height()},{"viewportHeight",viewport->height()},{"state",w.state()}}).toJson());
        call(w,"sketch.cancel");w.close();
    }
    void independentVisibility(){
        const auto fixture=qEnvironmentVariable("ALLOY_ENTITY_ROLES_FIXTURE");
        if(fixture.isEmpty())QSKIP("Set ALLOY_ENTITY_ROLES_FIXTURE to the retained mixed-roles.alloy from sketch_entity_roles_api.py");
        AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1500,1000);w.show();QVERIFY(QTest::qWaitForWindowExposed(&w));
        call(w,"document.open",{{"path",fixture}});
        const auto doc=call(w,"document.inspect");QJsonObject sketch;
        for(const auto& row:doc.value("authored").toObject().value("features").toArray())if(row.toObject().value("name")=="Mixed entity roles")sketch=row.toObject();
        QVERIFY(!sketch.isEmpty());call(w,"sketch.edit",{{"sketchId",sketch.value("id")}});
        auto session=[&]{return w.state().value("activeSession").toObject();};
        const auto initial=session(),diagnostics=initial.value("solverDiagnostics").toObject();
        QCOMPARE(diagnostics.value("entityRoles").toArray().size(),8);QCOMPARE(diagnostics.value("underconstrainedEntities").toArray().size(),2);
        ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* found=dynamic_cast<ReferenceView*>(child)){view=found;break;}QVERIFY(view);
#ifdef Q_OS_MACOS
        QProcess activation;activation.start("osascript",{"-e",QString("tell application \"System Events\" to set frontmost of first application process whose unix id is %1 to true").arg(QCoreApplication::applicationPid())});
        QVERIFY(activation.waitForFinished(5000));QCOMPARE(activation.exitCode(),0);
#endif
        w.raise();w.activateWindow();w.windowHandle()->requestActivate();QApplication::setActiveWindow(&w);QTRY_VERIFY(w.isActiveWindow());
        // End initial activation. Every following event uses actual produced focus.
        auto toggle=[&](QString key,bool value){
            auto* box=w.findChild<QCheckBox*>("sketch."+key);QVERIFY(box);
            for(auto* area:w.findChildren<QScrollArea*>())if(area->isAncestorOf(box))area->ensureWidgetVisible(box);
            QTest::qWait(25);QVERIFY(box->isVisible());QCOMPARE(box->isChecked(),!value);
            const QPoint at(8,box->height()/2);QVERIFY(!box->childAt(at));QTest::mouseClick(box,Qt::LeftButton,Qt::NoModifier,at);
            QCOMPARE(session().value("displayOptions").toObject().value(key).toBool(),value);QVERIFY(w.isActiveWindow());
        };
        const auto parameters=initial.value("draftSketch").toObject().value("parameters").toObject();
        auto point=[&](double u,double v){const auto p=view->projectSketchPoint(u,v,"face");return QPoint(qRound(p.value("x").toDouble()),qRound(p.value("y").toDouble()));};
        auto hit=[&](double u,double v,QString filter=QString{}){const auto p=point(u,v);return view->pickSketch(session().value("draftSketch").toObject(),p.x(),p.y(),filter);};
        auto click=[&](double u,double v){const auto p=point(u,v);QVERIFY(view->rect().contains(p));QVERIFY(!view->childAt(p));QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,p);};
        call(w,"sketch.lookAt");QTest::qWait(100);
        // One overlap: ordinary line lies exactly over the construction line. With
        // construction hidden the ordinary line remains the actual pick target.
        call(w,"sketch.line",{{"x1",-35},{"y1",8},{"x2",-10},{"y2",8},{"units","mm"}});
        const auto overlap=session().value("draftSketch").toObject().value("parameters").toObject().value("entities").toArray().last().toObject().value("id");
        call(w,"sketch.constraint",{{"type","horizontal"},{"entityIds",QJsonArray{overlap}}});
        const auto baseline=session();
        auto unchanged=[&]{QCOMPARE(session().value("draftSketch"),baseline.value("draftSketch"));QCOMPARE(session().value("draftHistory"),baseline.value("draftHistory"));QCOMPARE(session().value("solverDiagnostics"),baseline.value("solverDiagnostics"));};
        toggle("showConstructionGeometries",false);QCOMPARE(hit(-22,8,"entity").value("entityId"),overlap);click(-22,8);QCOMPARE(session().value("selection").toObject().value("entityIds").toArray(),QJsonArray{overlap});unchanged();
        toggle("showConstructionGeometries",true);unchanged();
        // Point and profile controls do not suppress ordinary boundary curves.
        QVERIFY(hit(-35,20,"point").value("kind")=="point");click(-35,20);
        QVERIFY(!session().value("selection").toObject().value("pointIds").toArray().isEmpty());
        toggle("showPoints",false);QVERIFY(hit(-35,20,"point").isEmpty());QVERIFY(session().value("selection").toObject().value("pointIds").toArray().isEmpty());
        click(-35,20);QVERIFY(session().value("selection").toObject().value("pointIds").toArray().isEmpty());unchanged();toggle("showPoints",true);
        QCOMPARE(hit(20,10).value("kind").toString(),QString("point"));
        QCOMPARE(hit(22,12).value("kind").toString(),QString("profile"));click(22,12);
        QVERIFY(!session().value("selection").toObject().value("profileIds").toArray().isEmpty());
        toggle("showProfile",false);QVERIFY(hit(22,12).isEmpty());QVERIFY(session().value("selection").toObject().value("profileIds").toArray().isEmpty());click(22,12);unchanged();toggle("showProfile",true);
        const auto projection=w.state().value("viewport").toObject().value("gpu").toObject().value("projectedLines").toArray().first().toObject();
        const double pu=(projection.value("u1Mm").toDouble()+projection.value("u2Mm").toDouble())/2,pv=(projection.value("v1Mm").toDouble()+projection.value("v2Mm").toDouble())/2;
        const auto projected=hit(pu,pv,"entity");QCOMPARE(projected.value("kind").toString(),QString("entity"));click(pu,pv);
        toggle("showProjectedGeometries",false);QVERIFY(hit(pu,pv,"entity").isEmpty());click(pu,pv);QVERIFY(session().value("selection").toObject().value("entityIds").toArray().isEmpty());unchanged();toggle("showProjectedGeometries",true);
        QVERIFY(!view->annotations().isEmpty());toggle("showDimensions",false);QVERIFY(view->annotations().isEmpty());QVERIFY(!w.state().value("viewport").toObject().value("gpu").toObject().value("sketchConstraintGlyphs").toArray().isEmpty());unchanged();toggle("showDimensions",true);
        QVERIFY(!w.state().value("viewport").toObject().value("gpu").toObject().value("sketchConstraintGlyphs").toArray().isEmpty());toggle("showConstraints",false);QVERIFY(w.state().value("viewport").toObject().value("gpu").toObject().value("sketchConstraintGlyphs").toArray().isEmpty());QVERIFY(!view->annotations().isEmpty());unchanged();toggle("showConstraints",true);
        click(-22,-8);QVERIFY(!session().value("selection").toObject().value("entityIds").toArray().isEmpty());
        toggle("hideFullyConstrained",true);QVERIFY(hit(-22,-8,"entity").isEmpty());QVERIFY(hit(26,10,"entity").isEmpty());QVERIFY(hit(pu,pv,"entity").isEmpty());QVERIFY(!hit(-22,8,"entity").isEmpty());
        click(-22,-8);QVERIFY(session().value("selection").toObject().value("entityIds").toArray().isEmpty());unchanged();
        const auto output=qEnvironmentVariable("ALLOY_VISIBILITY_EVIDENCE");
        if(!output.isEmpty()){
            call(w,"capture.screenshot",{{"path",output+"/focus.png"}});
            QFile receipt(output+"/native-state.json");QVERIFY(receipt.open(QIODevice::WriteOnly));receipt.write(QJsonDocument(w.state()).toJson());
        }
        toggle("hideFullyConstrained",false);unchanged();
        call(w,"history.undo");QVERIFY(session().value("draftSketch")!=baseline.value("draftSketch"));call(w,"history.redo");QCOMPARE(session().value("draftSketch"),baseline.value("draftSketch"));
        toggle("showPoints",false);call(w,"sketch.finish");
        if(!output.isEmpty()){
            const auto saved=call(w,"document.inspect").value("authored");call(w,"document.save",{{"path",output+"/visibility.alloy"}});
            call(w,"document.open",{{"path",output+"/visibility.alloy"}});QCOMPARE(call(w,"document.inspect").value("authored"),saved);
            call(w,"sketch.edit",{{"sketchId",sketch.value("id")}});QVERIFY(session().value("displayOptions").toObject().value("showPoints").toBool());
            call(w,"sketch.lookAt");
            auto* name=w.findChild<QLineEdit*>("sketch.name");auto* viewport=w.findChild<QWidget*>("sketchScrollViewport");QVERIFY(name&&viewport);
            QTRY_VERIFY_WITH_TIMEOUT(name->isVisible() && name->mapTo(viewport,QPoint()).y()>=0 && name->mapTo(viewport,QPoint()).y()<60,3000);
            call(w,"capture.screenshot",{{"path",output+"/all-visible.png"}});call(w,"sketch.cancel");
        }
        w.close();
    }
};
QTEST_MAIN(SketchVisibilityControls)
#include "sketch_visibility_controls.moc"
