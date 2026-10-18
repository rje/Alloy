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

class SketchFixCircleControls final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& w,const QString& c,const QJsonObject& a={}) {
        auto r=w.executeApi(c,a);const auto initial=r.value("result").toObject();
        for(int i=0;r.value("completion")=="pending" && i<400;++i){QTest::qWait(25);r=w.executeApi("command.status",{{"session",initial.value("id")},{"generation",initial.value("previewGeneration")}});}
        if(!r.value("ok").toBool() || r.value("completion")=="pending")qFatal("%s: %s",qPrintable(c),QJsonDocument(r).toJson().constData());return r.value("result").toObject();
    }
private slots:
    void ownershipAndSchemaRefusals() {
        auto uuid=[] {return QUuid::createUuid().toString(QUuid::WithoutBraces);};
        const auto blank=SketchGeometry::newFeature(uuid(),"XY");
        auto two=SketchGeometry::addTwoPointCircle(blank,{{"startUMm",10},{"startVMm",10},{"directionU",1},{"directionV",0},{"diameterMm",8}});QVERIFY(two.ok);
        const auto twoId=two.feature.value("parameters").toObject().value("entities").toArray().last().toObject().value("id");
        const auto fixedTwo=SketchGeometry::toggleFixed(two.feature,{{"entityIds",QJsonArray{twoId}}});QVERIFY2(fixedTwo.ok,qPrintable(fixedTwo.error));SketchGeometry::SolveResult refused;
        auto circle=SketchGeometry::addCircle(blank,{{"centerUMm",10},{"centerVMm",10},{"diameterMm",8}});const auto id=circle.value("parameters").toObject().value("entities").toArray().last().toObject().value("id");
        const auto offset=SketchGeometry::addCircleOffset(circle,{{"sourceEntityId",id},{"distanceMm",2}});QVERIFY2(offset.ok,qPrintable(offset.error));
        const auto resultId=offset.feature.value("parameters").toObject().value("entities").toArray().last().toObject().value("id");
        for(const auto& target:QJsonArray{id,resultId}){refused=SketchGeometry::toggleFixed(offset.feature,{{"entityIds",QJsonArray{target}}});QVERIFY(!refused.ok);QCOMPARE(refused.feature,offset.feature);}
        auto projected=blank;auto pp=projected.value("parameters").toObject();pp["plane"]="construction";pp["planeId"]=uuid();const auto projectionId=uuid();pp["circleProjections"]=QJsonArray{QJsonObject{{"id",projectionId},{"profile",true},{"source",QJsonObject{{"bodyId",uuid()},{"edgeId",uuid()},{"occurrenceId",uuid()}}}}};projected["parameters"]=pp;QSet<QString> authoredIds;const auto projectionSchemaError=SketchGeometry::validateParameters(pp,authoredIds);QVERIFY2(projectionSchemaError.isEmpty(),qPrintable(projectionSchemaError));
        refused=SketchGeometry::toggleFixed(projected,{{"entityIds",QJsonArray{projectionId}}});QVERIFY(!refused.ok);QCOMPARE(refused.feature,projected);QVERIFY(refused.error.contains("target no longer exists"));
        const auto fixed=SketchGeometry::toggleFixed(circle,{{"entityIds",QJsonArray{id}}});QVERIFY(fixed.ok);
        for(int mode=0;mode<3;++mode){auto bad=fixed.feature;auto p=bad.value("parameters").toObject();auto cs=p.value("constraints").toArray();auto relation=cs.last().toObject();if(mode==0)relation["radiusMm"]=-1;else if(mode==1)relation["pointIds"]=QJsonArray{uuid()};else relation["extra"]=true;cs[cs.size()-1]=relation;p["constraints"]=cs;bad["parameters"]=p;QVERIFY(!SketchGeometry::solveDraft(bad).ok);}
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
        call(w,"sketch.begin",{{"plane","XY"},{"units","mm"},{"name","Circle Fix and UnFix"}});
        call(w,"sketch.addCircle",{{"centerX",-20},{"centerY",10},{"diameter",20},{"units","mm"}});const auto circle=parameters().value("entities").toArray().last().toObject();
        call(w,"sketch.dimension",{{"kind","radius"},{"entityId",circle.value("id")},{"value",10},{"units","mm"}});const auto dimension=parameters().value("dimensions").toArray().last().toObject();
        call(w,"sketch.addCircle",{{"centerX",20},{"centerY",-10},{"diameter",14},{"units","mm"}});const auto ordinary=parameters().value("entities").toArray().last().toObject();
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
        click(-10,10);const auto fixedFirst=draft();QVERIFY(fixedFirst!=baseline);QCOMPARE(role(circle.value("id")),QString("fixed"));
        const auto relation=parameters().value("constraints").toArray().last().toObject();QCOMPARE(relation.value("type"),QJsonValue("fixedCircle"));QCOMPARE(relation.value("pointIds").toArray(),QJsonArray{circle.value("centerPointId")});QCOMPARE(relation.value("radiusMm"),circle.value("radiusMm"));
        click(27,-10);const auto fixedBoth=draft();QCOMPARE(role(ordinary.value("id")),QString("fixed"));
        QTemporaryDir temporaryOutput;QVERIFY(temporaryOutput.isValid());const auto requestedOutput=qEnvironmentVariable("ALLOY_FIX_CIRCLE_EVIDENCE");const auto output=requestedOutput.isEmpty() ? temporaryOutput.path() : requestedOutput;QJsonArray receipts;if(!output.isEmpty())QDir().mkpath(output);
        auto record=[&](QString name){QTest::qWait(100);receipts.append(QJsonObject{{"step",name},{"state",w.state()}});if(!output.isEmpty())call(w,"capture.screenshot",{{"path",output+"/"+name+".png"}});};
        QCOMPARE(curveLabel(circle.value("id")).count("Fixed"),1);QCOMPARE(curveLabel(ordinary.value("id")).count("Fixed"),1);
        record("native-fixed-circles");
        for(const auto& g:w.state().value("viewport").toObject().value("gpu").toObject().value("sketchConstraintGlyphs").toArray())QVERIFY(g.toObject().value("kind")!="fixedCircle");
        // Repeat native click removes only the first explicit circle owner.
        click(-10,10);const auto secondOnly=draft();QVERIFY(role(circle.value("id"))!="fixed");QCOMPARE(role(ordinary.value("id")),QString("fixed"));
        QTest::keyClick(view,Qt::Key_Escape);QCOMPARE(session().value("tool"),QJsonValue("select"));call(w,"history.undo");QCOMPARE(draft(),fixedBoth);call(w,"history.redo");QCOMPARE(draft(),secondOnly);call(w,"history.undo");QCOMPARE(draft(),fixedBoth);
        const auto history=session().value("draftHistory");QVERIFY(!w.executeApi("sketch.dimensionUpdate",{{"dimensionId",dimension.value("id")},{"value",12},{"units","mm"}}).value("ok").toBool());QCOMPARE(draft(),fixedBoth);QCOMPARE(session().value("draftHistory"),history);
        call(w,"sketch.fixToggle",{{"entityIds",QJsonArray{circle.value("id")}}});call(w,"sketch.dimensionUpdate",{{"dimensionId",dimension.value("id")},{"value",12},{"units","mm"}});QCOMPARE(parameters().value("entities").toArray().first().toObject().value("radiusMm"),QJsonValue(12));
        call(w,"sketch.fixToggle",{{"entityIds",QJsonArray{circle.value("id")}}});const auto final=draft();record("native-final");
        const auto sketchId=final.value("id");call(w,"sketch.finish");
        if(!output.isEmpty()){const auto authored=call(w,"document.inspect").value("authored");call(w,"document.save",{{"path",output+"/fixed-circles.alloy"}});call(w,"document.open",{{"path",output+"/fixed-circles.alloy"}});QCOMPARE(call(w,"document.inspect").value("authored"),authored);call(w,"sketch.edit",{{"sketchId",sketchId}});QCOMPARE(draft(),final);QTest::qWait(200);record("native-reopened");QFile file(output+"/receipt.json");QVERIFY(file.open(QIODevice::WriteOnly));file.write(QJsonDocument(QJsonObject{{"result","passed"},{"steps",receipts},{"initialRelation",relation}}).toJson());call(w,"sketch.cancel");}
        w.close();
    }
};
QTEST_MAIN(SketchFixCircleControls)
#include "sketch_fix_circle_controls.moc"
