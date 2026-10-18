// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include "SketchGeometry.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QLineEdit>
#include <QProcess>
#include <QPushButton>
#include <QScrollArea>
#include <QTest>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QTemporaryDir>
#include <QWindow>
#include <QUuid>
#include <cmath>

class SketchFixArcControls final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& w,const QString& c,const QJsonObject& a={}) {
        auto r=w.executeApi(c,a);const auto initial=r.value("result").toObject();
        for(int i=0;r.value("completion")=="pending" && i<400;++i){QTest::qWait(25);r=w.executeApi("command.status",{{"session",initial.value("id")},{"generation",initial.value("previewGeneration")}});}
        if(!r.value("ok").toBool() || r.value("completion")=="pending")qFatal("%s: %s",qPrintable(c),QJsonDocument(r).toJson().constData());return r.value("result").toObject();
    }
private slots:
    void ownershipAndSchemaRefusals() {
        const auto blank=SketchGeometry::newFeature(QUuid::createUuid().toString(QUuid::WithoutBraces),"XY");
        const auto arc=SketchGeometry::addArc(blank,{{"centerUMm",10},{"centerVMm",10},{"radiusMm",8},{"startAngleDeg",35},{"sweepAngleDeg",-210}});
        const auto entity=arc.value("parameters").toObject().value("entities").toArray().last().toObject();
        const auto fixed=SketchGeometry::toggleFixed(arc,{{"entityIds",QJsonArray{entity.value("id")}}});QVERIFY2(fixed.ok,qPrintable(fixed.error));
        QCOMPARE(fixed.feature.value("parameters").toObject().value("constraints").toArray().last().toObject().value("type"),QJsonValue("fixedArc"));
        for(int mode=0;mode<6;++mode){auto bad=fixed.feature;auto p=bad.value("parameters").toObject();auto cs=p.value("constraints").toArray();auto relation=cs.last().toObject();
            if(mode==0)relation["radiusMm"]=-1;else if(mode==1)relation["pointIds"]=QJsonArray{entity.value("endPointId"),entity.value("startPointId"),entity.value("centerPointId")};else if(mode==2)relation["extra"]=true;else if(mode==3)relation["sweepAngleDeg"]=360;else if(mode==4)relation.remove("startAngleDeg");else relation["entityIds"]=QJsonArray{QUuid::createUuid().toString(QUuid::WithoutBraces)};
            cs[cs.size()-1]=relation;p["constraints"]=cs;bad["parameters"]=p;QVERIFY(!SketchGeometry::solveDraft(bad).ok);}
        // Rounded-profile arcs are group-owned even when present as authored entities.
        const auto slot=SketchGeometry::addCenterPointSlot(blank,{{"centerUMm",0},{"centerVMm",0},{"endUMm",20},{"endVMm",0},{"widthMm",10}});QVERIFY2(slot.ok,qPrintable(slot.error));const auto rounded=slot.feature;
        for(const auto& row:rounded.value("parameters").toObject().value("entities").toArray())if(row.toObject().value("type")=="arc"){
            const auto refused=SketchGeometry::toggleFixed(rounded,{{"entityIds",QJsonArray{row.toObject().value("id")}}});QVERIFY(!refused.ok);QCOMPARE(refused.feature,rounded);}
    }
    void nativeToggleAndPersistence() {
        AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1500,1000);w.show();QVERIFY(QTest::qWaitForWindowExposed(&w));
        ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(child))view=v;QVERIFY(view);
        auto session=[&]{return w.state().value("activeSession").toObject();};
        auto draft=[&]{return session().value("draftSketch").toObject();};
        auto parameters=[&]{return draft().value("parameters").toObject();};
        auto roles=[&]{return session().value("solverDiagnostics").toObject().value("entityRoles").toArray();};
        auto role=[&](QJsonValue id){for(const auto& row:roles())if(row.toObject().value("entityId")==id)return row.toObject().value("role").toString();return QString{};};
        auto curveLabel=[&](QJsonValue id){for(auto* tree:w.findChildren<QTreeWidget*>())for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole)=="entity" && (*it)->data(0,Qt::UserRole+1).toString()==id.toString())return (*it)->text(0);return QString{};};
        call(w,"sketch.begin",{{"plane","XY"},{"units","mm"},{"name","Arc Fix and UnFix"}});
        call(w,"sketch.addArc",{{"mode","center"},{"centerX",-20},{"centerY",10},{"radius",10},{"startAngleDeg",0},{"sweepAngleDeg",180},{"units","mm"}});const auto arc=parameters().value("entities").toArray().last().toObject();
        call(w,"sketch.dimension",{{"kind","radius"},{"entityId",arc.value("id")},{"value",10},{"units","mm"}});const auto dimension=parameters().value("dimensions").toArray().last().toObject();
        call(w,"sketch.annotationMove",{{"dimensionId",dimension.value("id")},{"uMm",-40},{"vMm",28}});
        call(w,"sketch.addArc",{{"mode","center"},{"centerX",20},{"centerY",-10},{"radius",7},{"startAngleDeg",0},{"sweepAngleDeg",-180},{"units","mm"}});const auto ordinary=parameters().value("entities").toArray().last().toObject();
        call(w,"sketch.tool",{{"tool","select"}});call(w,"sketch.selectionSet");call(w,"sketch.lookAt");QTest::qWait(100);
        auto* fix=w.findChild<QPushButton*>("sketch.constraint.fixed");QVERIFY(fix);QCOMPARE(fix->text(),QString("Fix/UnFix"));
        for(auto* area:w.findChildren<QScrollArea*>())if(area->isAncestorOf(fix))area->ensureWidgetVisible(fix);
#ifdef Q_OS_MACOS
        QProcess activate;activate.start("osascript",{"-e",QString("tell application \"System Events\" to set frontmost of first application process whose unix id is %1 to true").arg(QCoreApplication::applicationPid())});QVERIFY(activate.waitForFinished(5000));QCOMPARE(activate.exitCode(),0);
#endif
        w.raise();w.activateWindow();w.windowHandle()->requestActivate();QApplication::setActiveWindow(&w);QTRY_VERIFY(w.isActiveWindow());
        // Initial harness activation ends here. No later focus or selection repair.
        auto click=[&](double u,double v){const auto p=view->projectSketchPoint(u,v,"XY");const QPoint at(qRound(p.value("x").toDouble()),qRound(p.value("y").toDouble()));QVERIFY(view->rect().contains(at));QVERIFY(!view->childAt(at));QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,at);QVERIFY(w.isActiveWindow());QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QCOMPARE(session().value("tool"),QJsonValue("constraint"));QVERIFY(session().value("selection").toObject().value("entityIds").toArray().isEmpty());};
        const auto baseline=draft();QTest::mouseClick(fix,Qt::LeftButton);QCOMPARE(session().value("constraintType"),QJsonValue("fixed"));
        click(-20,20);const auto fixedFirst=draft();QVERIFY(fixedFirst!=baseline);QCOMPARE(role(arc.value("id")),QString("fixed"));
        const auto relation=parameters().value("constraints").toArray().last().toObject();QCOMPARE(relation.value("type"),QJsonValue("fixedArc"));QCOMPARE(relation.value("pointIds").toArray(),(QJsonArray{arc.value("centerPointId"),arc.value("startPointId"),arc.value("endPointId")}));QCOMPARE(relation.value("radiusMm"),arc.value("radiusMm"));QCOMPARE(relation.value("startAngleDeg"),arc.value("startAngleDeg"));QCOMPARE(relation.value("sweepAngleDeg"),arc.value("sweepAngleDeg"));
        click(20,-17);const auto fixedBoth=draft();QCOMPARE(role(ordinary.value("id")),QString("fixed"));
        QTemporaryDir temporaryOutput;QVERIFY(temporaryOutput.isValid());const auto requestedOutput=qEnvironmentVariable("ALLOY_FIX_ARC_EVIDENCE");const auto output=requestedOutput.isEmpty() ? temporaryOutput.path() : requestedOutput;QJsonArray receipts;if(!output.isEmpty())QDir().mkpath(output);
        auto record=[&](QString name){QTest::qWait(100);receipts.append(QJsonObject{{"step",name},{"state",w.state()}});if(!output.isEmpty())call(w,"capture.screenshot",{{"path",output+"/"+name+".png"}});};
        QCOMPARE(curveLabel(arc.value("id")).count("Fixed"),1);QCOMPARE(curveLabel(ordinary.value("id")).count("Fixed"),1);
        record("native-fixed-arcs");
        for(const auto& g:w.state().value("viewport").toObject().value("gpu").toObject().value("sketchConstraintGlyphs").toArray())QVERIFY(g.toObject().value("kind")!="fixedArc");
        // Repeat native click removes only the first explicit arc owner.
        click(-20,20);const auto secondOnly=draft();QVERIFY(role(arc.value("id"))!="fixed");QCOMPARE(role(ordinary.value("id")),QString("fixed"));
        QTest::keyClick(view,Qt::Key_Escape);QCOMPARE(session().value("tool"),QJsonValue("select"));call(w,"history.undo");QCOMPARE(draft(),fixedBoth);call(w,"history.redo");QCOMPARE(draft(),secondOnly);call(w,"history.undo");QCOMPARE(draft(),fixedBoth);
        const auto history=session().value("draftHistory");QVERIFY(!w.executeApi("sketch.dimensionUpdate",{{"dimensionId",dimension.value("id")},{"value",12},{"units","mm"}}).value("ok").toBool());QCOMPARE(draft(),fixedBoth);QCOMPARE(session().value("draftHistory"),history);
        call(w,"sketch.fixToggle",{{"entityIds",QJsonArray{arc.value("id")}}});call(w,"sketch.dimensionUpdate",{{"dimensionId",dimension.value("id")},{"value",12},{"units","mm"}});QCOMPARE(parameters().value("entities").toArray().first().toObject().value("radiusMm"),QJsonValue(12));
        call(w,"sketch.dimension",{{"kind","arcSweep"},{"entityId",arc.value("id")},{"valueDeg",160}});const auto sweep=parameters().value("dimensions").toArray().last().toObject();call(w,"sketch.annotationMove",{{"dimensionId",sweep.value("id")},{"uMm",-15},{"vMm",35}});
        call(w,"sketch.fixToggle",{{"entityIds",QJsonArray{arc.value("id")}}});const auto fixedSweep=draft();const auto sweepHistory=session().value("draftHistory");
        QVERIFY(!w.executeApi("sketch.dimensionUpdate",{{"dimensionId",sweep.value("id")},{"valueDeg",150}}).value("ok").toBool());QCOMPARE(draft(),fixedSweep);QCOMPARE(session().value("draftHistory"),sweepHistory);
        call(w,"sketch.fixToggle",{{"entityIds",QJsonArray{arc.value("id")}}});call(w,"sketch.dimensionUpdate",{{"dimensionId",sweep.value("id")},{"valueDeg",150}});QVERIFY(std::abs(parameters().value("entities").toArray().first().toObject().value("sweepAngleDeg").toDouble()-150)<1e-6);
        call(w,"sketch.fixToggle",{{"entityIds",QJsonArray{arc.value("id")}}});const auto final=draft();record("native-final");
        const auto sketchId=final.value("id");call(w,"sketch.finish");
        if(!output.isEmpty()){const auto authored=call(w,"document.inspect").value("authored");call(w,"document.save",{{"path",output+"/fixed-arcs.alloy"}});call(w,"document.open",{{"path",output+"/fixed-arcs.alloy"}});QCOMPARE(call(w,"document.inspect").value("authored"),authored);call(w,"sketch.edit",{{"sketchId",sketchId}});QCOMPARE(draft(),final);QTest::qWait(200);record("native-reopened");QFile file(output+"/receipt.json");QVERIFY(file.open(QIODevice::WriteOnly));file.write(QJsonDocument(QJsonObject{{"result","passed"},{"steps",receipts},{"initialRelation",relation}}).toJson());call(w,"sketch.cancel");}
        w.close();
    }
};
QTEST_MAIN(SketchFixArcControls)
#include "sketch_fix_arc_controls.moc"
