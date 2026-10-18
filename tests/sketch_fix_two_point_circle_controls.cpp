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

class SketchFixTwoPointCircleControls final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& w,const QString& c,const QJsonObject& a={}) {
        auto r=w.executeApi(c,a);const auto initial=r.value("result").toObject();
        for(int i=0;r.value("completion")=="pending" && i<400;++i){QTest::qWait(25);r=w.executeApi("command.status",{{"session",initial.value("id")},{"generation",initial.value("previewGeneration")}});}
        if(!r.value("ok").toBool() || r.value("completion")=="pending")qFatal("%s: %s",qPrintable(c),QJsonDocument(r).toJson().constData());return r.value("result").toObject();
    }
private slots:
    void retainedSourceAndSchema() {
        const auto blank=SketchGeometry::newFeature(QUuid::createUuid().toString(QUuid::WithoutBraces),"XY");
        const auto two=SketchGeometry::addTwoPointCircle(blank,{{"startUMm",-26},{"startVMm",2},{"directionU",3},{"directionV",4},{"diameterMm",20}});QVERIFY(two.ok);
        const auto entity=two.feature.value("parameters").toObject().value("entities").toArray().last().toObject();
        const auto fixed=SketchGeometry::toggleFixed(two.feature,{{"entityIds",QJsonArray{entity.value("id")}}});QVERIFY2(fixed.ok,qPrintable(fixed.error));
        const auto mixed=SketchGeometry::solveDraft(SketchGeometry::addCircle(fixed.feature,{{"centerUMm",20},{"centerVMm",-10},{"diameterMm",14}}));QVERIFY(mixed.ok);QVERIFY(mixed.diagnostics.value("dof").toInt()>0);
        QVERIFY(mixed.diagnostics.value("pointFreedom").toObject().value(entity.value("diameterStartPointId").toString()).toObject().value("fullyConstrained").toBool());
        for(int mode=0;mode<6;++mode){
            auto bad=fixed.feature;auto p=bad.value("parameters").toObject();auto cs=p.value("constraints").toArray();auto relation=cs.last().toObject();
            if(mode==0)relation.remove("startUMm");
            else if(mode==1)relation["pointIds"]=QJsonArray{entity.value("centerPointId")};
            else if(mode==2)relation["diameterDirectionU"]=-0.6;
            else if(mode==3)relation["startUMm"]=-25;
            else if(mode==4){auto es=p.value("entities").toArray();auto e=es.last().toObject();e["diameterDirectionU"]=-0.6;es[es.size()-1]=e;p["entities"]=es;}
            else relation["radiusMm"]=-1;
            cs[cs.size()-1]=relation;p["constraints"]=cs;bad["parameters"]=p;
            const auto refused=SketchGeometry::solveDraft(bad);QVERIFY2(!refused.ok,qPrintable(QString("Malformed source mode %1 accepted").arg(mode)));
        }
        const auto unfixed=SketchGeometry::toggleFixed(fixed.feature,{{"entityIds",QJsonArray{entity.value("id")}}});QVERIFY(unfixed.ok);auto expected=two.feature;auto expectedParameters=expected.value("parameters").toObject();expectedParameters["constraints"]=QJsonArray{};expected["parameters"]=expectedParameters;QCOMPARE(unfixed.feature,expected);
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
        call(w,"sketch.begin",{{"plane","XY"},{"units","mm"},{"name","Two-Point Circle Fix and UnFix"}});
        call(w,"sketch.addTwoPointCircle",{{"startX",-26},{"startY",2},{"endX",-14},{"endY",18},{"diameter",20},{"units","mm"}});const auto circle=parameters().value("entities").toArray().last().toObject();
        const auto dimension=parameters().value("dimensions").toArray().last().toObject();
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
        const auto relation=parameters().value("constraints").toArray().last().toObject();QCOMPARE(relation.value("type"),QJsonValue("fixedCircle"));QCOMPARE(relation.value("pointIds").toArray(),(QJsonArray{circle.value("centerPointId"),circle.value("diameterStartPointId")}));QCOMPARE(relation.value("radiusMm"),circle.value("radiusMm"));QCOMPARE(relation.value("startUMm"),QJsonValue(-26));QCOMPARE(relation.value("startVMm"),QJsonValue(2));QCOMPARE(relation.value("diameterDirectionU"),QJsonValue(0.6));QCOMPARE(relation.value("diameterDirectionV"),QJsonValue(0.8));
        click(27,-10);const auto fixedBoth=draft();QCOMPARE(role(ordinary.value("id")),QString("fixed"));
        QTemporaryDir temporaryOutput;QVERIFY(temporaryOutput.isValid());const auto requestedOutput=qEnvironmentVariable("ALLOY_FIX_TWO_POINT_CIRCLE_EVIDENCE");const auto output=requestedOutput.isEmpty() ? temporaryOutput.path() : requestedOutput;QJsonArray receipts;if(!output.isEmpty())QDir().mkpath(output);
        auto record=[&](QString name){QTest::qWait(100);receipts.append(QJsonObject{{"step",name},{"state",w.state()}});if(!output.isEmpty())call(w,"capture.screenshot",{{"path",output+"/"+name+".png"}});};
        QCOMPARE(curveLabel(circle.value("id")).count("Fixed"),1);QCOMPARE(curveLabel(ordinary.value("id")).count("Fixed"),1);
        record("native-fixed-two-point-circle");
        for(const auto& g:w.state().value("viewport").toObject().value("gpu").toObject().value("sketchConstraintGlyphs").toArray())QVERIFY(g.toObject().value("kind")!="fixedCircle");
        // Repeat native click removes only the first explicit circle owner.
        click(-10,10);const auto secondOnly=draft();QVERIFY(role(circle.value("id"))!="fixed");QCOMPARE(role(ordinary.value("id")),QString("fixed"));
        QTest::keyClick(view,Qt::Key_Escape);QCOMPARE(session().value("tool"),QJsonValue("select"));call(w,"history.undo");QCOMPARE(draft(),fixedBoth);call(w,"history.redo");QCOMPARE(draft(),secondOnly);call(w,"history.undo");QCOMPARE(draft(),fixedBoth);
        const auto history=session().value("draftHistory");QVERIFY(!w.executeApi("sketch.dimensionUpdate",{{"dimensionId",dimension.value("id")},{"value",24},{"units","mm"}}).value("ok").toBool());QCOMPARE(draft(),fixedBoth);QCOMPARE(session().value("draftHistory"),history);
        call(w,"sketch.fixToggle",{{"entityIds",QJsonArray{circle.value("id")}}});call(w,"sketch.dimensionUpdate",{{"dimensionId",dimension.value("id")},{"value",24},{"units","mm"}});QCOMPARE(parameters().value("entities").toArray().first().toObject().value("radiusMm"),QJsonValue(12));call(w,"sketch.movePoint",{{"pointId",circle.value("diameterStartPointId")},{"x",-30},{"y",0},{"units","mm"}});for(const auto& row:parameters().value("points").toArray()){const auto point=row.toObject();if(point.value("id")==circle.value("centerPointId")){QVERIFY(qAbs(point.value("uMm").toDouble()+22.8)<1e-6);QVERIFY(qAbs(point.value("vMm").toDouble()-9.6)<1e-6);}}
        call(w,"sketch.fixToggle",{{"entityIds",QJsonArray{circle.value("id")}}});const auto final=draft();record("native-final");
        const auto sketchId=final.value("id");call(w,"sketch.finish");
        if(!output.isEmpty()){const auto authored=call(w,"document.inspect").value("authored");call(w,"document.save",{{"path",output+"/fixed-two-point-circle.alloy"}});call(w,"document.open",{{"path",output+"/fixed-two-point-circle.alloy"}});QCOMPARE(call(w,"document.inspect").value("authored"),authored);call(w,"sketch.edit",{{"sketchId",sketchId}});QCOMPARE(draft(),final);QTest::qWait(200);record("native-reopened");QFile file(output+"/receipt.json");QVERIFY(file.open(QIODevice::WriteOnly));file.write(QJsonDocument(QJsonObject{{"result","passed"},{"steps",receipts},{"initialRelation",relation}}).toJson());call(w,"sketch.cancel");}
        w.close();
    }
};
QTEST_MAIN(SketchFixTwoPointCircleControls)
#include "sketch_fix_two_point_circle_controls.moc"
