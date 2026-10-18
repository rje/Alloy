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
#include <QWindow>

class SketchFixUnfixControls final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& w,const QString& c,const QJsonObject& a={}) {
        auto r=w.executeApi(c,a);const auto initial=r.value("result").toObject();
        for(int i=0;r.value("completion")=="pending" && i<400;++i){QTest::qWait(25);r=w.executeApi("command.status",{{"session",initial.value("id")},{"generation",initial.value("previewGeneration")}});}
        if(!r.value("ok").toBool() || r.value("completion")=="pending")qFatal("%s: %s",qPrintable(c),QJsonDocument(r).toJson().constData());return r.value("result").toObject();
    }
private slots:
    void nativeToggleAndPersistence() {
        AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1500,1000);w.show();QVERIFY(QTest::qWaitForWindowExposed(&w));
        ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(child))view=v;QVERIFY(view);
        auto session=[&]{return w.state().value("activeSession").toObject();};
        auto draft=[&]{return session().value("draftSketch").toObject();};
        auto parameters=[&]{return draft().value("parameters").toObject();};
        auto roles=[&]{return session().value("solverDiagnostics").toObject().value("entityRoles").toArray();};
        auto role=[&](QJsonValue id){for(const auto& row:roles())if(row.toObject().value("entityId")==id)return row.toObject().value("role").toString();return QString{};};
        call(w,"sketch.begin",{{"plane","XY"},{"units","mm"},{"name","Fix and UnFix"}});
        call(w,"sketch.line",{{"x1",-35},{"y1",-10},{"x2",5},{"y2",-10},{"units","mm"}});const auto line=parameters().value("entities").toArray().last().toObject();
        call(w,"sketch.constraint",{{"type","horizontal"},{"entityIds",QJsonArray{line.value("id")}}});
        call(w,"sketch.addPoint",{{"x",30},{"y",15},{"units","mm"}});const auto point=parameters().value("entities").toArray().last().toObject();
        call(w,"sketch.addCircle",{{"centerX",30},{"centerY",-15},{"diameter",12},{"units","mm"}});const auto circle=parameters().value("entities").toArray().last().toObject();
        call(w,"sketch.tool",{{"tool","select"}});call(w,"sketch.selectionSet");call(w,"sketch.lookAt");QTest::qWait(100);
        auto* fix=w.findChild<QPushButton*>("sketch.constraint.fixed");QVERIFY(fix);QCOMPARE(fix->text(),QString("Fix/UnFix"));
        for(auto* area:w.findChildren<QScrollArea*>())if(area->isAncestorOf(fix))area->ensureWidgetVisible(fix);
#ifdef Q_OS_MACOS
        QProcess activate;activate.start("osascript",{"-e",QString("tell application \"System Events\" to set frontmost of first application process whose unix id is %1 to true").arg(QCoreApplication::applicationPid())});QVERIFY(activate.waitForFinished(5000));QCOMPARE(activate.exitCode(),0);
#endif
        w.raise();w.activateWindow();w.windowHandle()->requestActivate();QApplication::setActiveWindow(&w);QTRY_VERIFY(w.isActiveWindow());
        // Initial harness activation ends here. No later focus or selection repair.
        auto click=[&](double u,double v){const auto p=view->projectSketchPoint(u,v,"XY");const QPoint at(qRound(p.value("x").toDouble()),qRound(p.value("y").toDouble()));QVERIFY(view->rect().contains(at));QVERIFY(!view->childAt(at));QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,at);QVERIFY(w.isActiveWindow());QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));};
        const auto baseline=draft();QTest::mouseClick(fix,Qt::LeftButton);QCOMPARE(session().value("constraintType"),QJsonValue("fixed"));
        click(-15,-10);const auto fixedLine=draft();QVERIFY(fixedLine!=baseline);QCOMPARE(role(line.value("id")),QString("fixed"));
        const auto lineRelation=parameters().value("constraints").toArray().last().toObject();QCOMPARE(lineRelation.value("type"),QJsonValue("fixedLine"));QCOMPARE(lineRelation.value("entityIds").toArray(),QJsonArray{line.value("id")});QCOMPARE(lineRelation.value("pointIds").toArray(),(QJsonArray{line.value("startPointId"),line.value("endPointId")}));
        click(30,15);const auto fixedBoth=draft();QCOMPARE(role(point.value("id")),QString("fixed"));const auto pointRelation=parameters().value("constraints").toArray().last().toObject();QCOMPARE(pointRelation.value("type"),QJsonValue("fixed"));QCOMPARE(pointRelation.size(),5);
        const auto output=qEnvironmentVariable("ALLOY_FIX_UNFIX_EVIDENCE");QJsonArray receipts;if(!output.isEmpty())QDir().mkpath(output);
        auto record=[&](QString name){receipts.append(QJsonObject{{"step",name},{"state",w.state()}});if(!output.isEmpty())call(w,"capture.screenshot",{{"path",output+"/"+name+".png"}});};
        record("native-fixed-line-point");
        const auto glyphs=w.state().value("viewport").toObject().value("gpu").toObject().value("sketchConstraintGlyphs").toArray();for(const auto& g:glyphs){QVERIFY(g.toObject().value("constraintId")!=lineRelation.value("id"));QVERIFY(g.toObject().value("constraintId")!=pointRelation.value("id"));}
        click(-15,-10);const auto pointOnly=draft();QVERIFY(pointOnly!=fixedBoth);QVERIFY(role(line.value("id"))!="fixed");QCOMPARE(role(point.value("id")),QString("fixed"));
        QTest::keyClick(view,Qt::Key_Escape);QCOMPARE(session().value("tool"),QJsonValue("select"));
        call(w,"history.undo");QCOMPARE(draft(),fixedBoth);call(w,"history.redo");QCOMPARE(draft(),pointOnly);call(w,"history.undo");QCOMPARE(draft(),fixedBoth);
        QTest::mouseClick(fix,Qt::LeftButton);QCOMPARE(session().value("constraintType"),QJsonValue("fixed"));
        click(30,15);QCOMPARE(draft(),fixedLine);QTest::keyClick(view,Qt::Key_Escape);QCOMPARE(session().value("tool"),QJsonValue("select"));call(w,"history.undo");QCOMPARE(draft(),fixedBoth);call(w,"history.redo");QCOMPARE(draft(),fixedLine);call(w,"history.undo");QCOMPARE(draft(),fixedBoth);
        // Unsupported and stale API targets, including mixed target families, refuse atomically.
        for(const auto& args:QList<QJsonObject>{{{"entityIds",QJsonArray{circle.value("id")}}},{{"entityIds",QJsonArray{"00000000-0000-0000-0000-000000000000"}}},{{"entityIds",QJsonArray{line.value("id")}},{"pointIds",QJsonArray{point.value("pointId")}}}}){const auto before=session().value("draftHistory");QVERIFY(!w.executeApi("sketch.fixToggle",args).value("ok").toBool());QCOMPARE(draft(),fixedBoth);QCOMPARE(session().value("draftHistory"),before);}
        // Both endpoint coordinates remain fixed through attempted moves.
        for(const auto& id:QJsonArray{line.value("startPointId"),line.value("endPointId"),point.value("pointId")}){w.executeApi("sketch.movePoint",{{"pointId",id},{"x",60},{"y",40},{"units","mm"}});QCOMPARE(draft(),fixedBoth);}
        // A conflicting driver is refused instead of distorting the fixed line.
        const auto rejected=SketchGeometry::addDimension(fixedBoth,{{"kind","linear"},{"pointIds",QJsonArray{line.value("startPointId"),line.value("endPointId")}},{"valueMm",45},{"driving",true}});QVERIFY(!rejected.ok);QCOMPARE(rejected.feature,fixedBoth);
        const auto originDraft=SketchGeometry::withOriginReference(fixedBoth);
        const auto origin=originDraft.value("parameters").toObject().value("originPointId");
        const auto originRefusal=SketchGeometry::toggleFixed(originDraft,{{"pointIds",QJsonArray{origin}}});QVERIFY(!originRefusal.ok);QCOMPARE(originRefusal.feature,originDraft);
        const auto ellipse=SketchGeometry::addCenterPointEllipse(SketchGeometry::newFeature(fixedBoth.value("ownerId").toString(),"XY"),
            {{"centerUMm",0},{"centerVMm",0},{"firstUMm",10},{"firstVMm",0},{"contourUMm",0},{"contourVMm",5}});QVERIFY(ellipse.ok);
        const auto ellipseEntity=ellipse.feature.value("parameters").toObject().value("entities").toArray().last().toObject();
        const auto axisRefusal=SketchGeometry::toggleFixed(ellipse.feature,{{"entityIds",QJsonArray{ellipseEntity.value("firstAxisId")}}});QVERIFY(!axisRefusal.ok);QCOMPARE(axisRefusal.feature,ellipse.feature);
        // Removal by exact stable ID is also authoritative and undoable.
        call(w,"sketch.deleteSelection",{{"constraintIds",QJsonArray{lineRelation.value("id")}}});QCOMPARE(draft(),pointOnly);call(w,"history.undo");QCOMPARE(draft(),fixedBoth);
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QTest::keyClick(view,Qt::Key_Escape);QCOMPARE(session().value("tool"),QJsonValue("select"));record("fixed-final");
        const auto sketchId=draft().value("id");call(w,"sketch.finish");
        if(!output.isEmpty()) {const auto authored=call(w,"document.inspect").value("authored");call(w,"document.save",{{"path",output+"/fix-unfix.alloy"}});call(w,"document.open",{{"path",output+"/fix-unfix.alloy"}});QCOMPARE(call(w,"document.inspect").value("authored"),authored);call(w,"sketch.edit",{{"sketchId",sketchId}});QCOMPARE(draft(),fixedBoth);auto* name=w.findChild<QLineEdit*>("sketch.name");auto* scroll=w.findChild<QWidget*>("sketchScrollViewport");QVERIFY(name&&scroll);QTRY_VERIFY_WITH_TIMEOUT(name->isVisible() && name->mapTo(scroll,QPoint()).y()>=0 && name->mapTo(scroll,QPoint()).y()<60,3000);QTest::qWait(100);record("reopened");QFile file(output+"/receipt.json");QVERIFY(file.open(QIODevice::WriteOnly));file.write(QJsonDocument(QJsonObject{{"steps",receipts},{"lineRelation",lineRelation},{"pointRelation",pointRelation}}).toJson());call(w,"sketch.cancel");}
        w.close();
    }
};
QTEST_MAIN(SketchFixUnfixControls)
#include "sketch_fix_unfix_controls.moc"
