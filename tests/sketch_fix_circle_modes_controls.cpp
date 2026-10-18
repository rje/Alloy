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

#include <QtMath>
class SketchFixCircleModesControls final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& w,const QString& c,const QJsonObject& a={}) {
        auto r=w.executeApi(c,a);const auto initial=r.value("result").toObject();
        for(int i=0;r.value("completion")=="pending" && i<400;++i){QTest::qWait(25);r=w.executeApi("command.status",{{"session",initial.value("id")},{"generation",initial.value("previewGeneration")}});}
        if(!r.value("ok").toBool() || r.value("completion")=="pending")qFatal("%s: %s",qPrintable(c),QJsonDocument(r).toJson().constData());return r.value("result").toObject();
    }
private slots:
    void nativeModes_data(){QTest::addColumn<QString>("mode");for(const auto& m:{"three-point","two-tangent","three-tangent","center-coincident","center-snap"})QTest::newRow(m)<<QString(m);}
    void nativeModes(){
        QFETCH(QString,mode);AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1500,1000);w.show();QVERIFY(QTest::qWaitForWindowExposed(&w));
        ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(child))view=v;QVERIFY(view);
        auto session=[&]{return w.state().value("activeSession").toObject();};auto draft=[&]{return session().value("draftSketch").toObject();};auto parameters=[&]{return draft().value("parameters").toObject();};
        auto role=[&](QJsonValue id){for(const auto& row:session().value("solverDiagnostics").toObject().value("entityRoles").toArray())if(row.toObject().value("entityId")==id)return row.toObject().value("role").toString();return QString{};};
        call(w,"sketch.begin",{{"plane","XY"},{"units","mm"},{"name",mode+" Fix"}});
        call(w,"sketch.rectangle",{{"x1",0},{"y1",0},{"x2",30},{"y2",30},{"units","mm"}});const auto lines=parameters().value("entities").toArray();const auto bottom=lines[0].toObject(),right=lines[1].toObject(),top=lines[2].toObject();
        call(w,"sketch.constraint",{{"type","fixed"},{"pointIds",QJsonArray{bottom.value("startPointId")}}});
        call(w,"sketch.dimension",{{"kind","horizontal"},{"entityId",bottom.value("id")},{"value",30},{"units","mm"}});
        call(w,"sketch.dimension",{{"kind","vertical"},{"entityId",right.value("id")},{"value",30},{"units","mm"}});const auto dimensions=parameters().value("dimensions").toArray();
        QJsonArray lineIds;for(const auto& l:lines)lineIds.append(l.toObject().value("id"));call(w,"sketch.construction",{{"entityIds",lineIds},{"construction",true}});
        QJsonValue editDimension=dimensions[1].toObject().value("id");double clickU=0,clickV=0;
        if(mode=="three-point"){
            call(w,"sketch.threePointCircle",{{"x1",0},{"y1",0},{"x2",30},{"y2",0},{"x3",30},{"y3",30},{"units","mm"},{"firstSnapTarget",QJsonObject{{"kind","point"},{"pointId",bottom.value("startPointId")}}},{"secondSnapTarget",QJsonObject{{"kind","point"},{"pointId",bottom.value("endPointId")}}},{"thirdSnapTarget",QJsonObject{{"kind","point"},{"pointId",right.value("endPointId")}}}});clickU=15+qSqrt(450.0);clickV=15;
        }else if(mode=="two-tangent"){
            call(w,"sketch.twoTangentCircle",{{"lineIds",QJsonArray{bottom.value("id"),right.value("id")}},{"x",20},{"y",10},{"radius",10},{"units","mm"}});editDimension=dimensions[0].toObject().value("id");clickU=10;clickV=10;
        }else if(mode=="three-tangent"){
            call(w,"sketch.threeTangentCircle",{{"lineIds",QJsonArray{bottom.value("id"),right.value("id"),top.value("id")}}});clickU=0;clickV=15;
        }else{
            call(w,"sketch.addCircle",{{"centerX",mode=="center-coincident"?32:30},{"centerY",mode=="center-coincident"?32:30},{"diameter",20},{"units","mm"}});QJsonObject e;for(const auto& item:parameters().value("entities").toArray())if(item.toObject().value("type")=="circle")e=item.toObject();
            if(mode=="center-coincident")call(w,"sketch.constraint",{{"type","coincident"},{"pointIds",QJsonArray{e.value("centerPointId"),right.value("endPointId")}}});
            call(w,"sketch.dimension",{{"kind","radius"},{"entityId",e.value("id")},{"value",10},{"units","mm"}});clickU=40;clickV=30;
        }
        const auto circle=parameters().value("entities").toArray().last().toObject();call(w,"sketch.tool",{{"tool","select"}});call(w,"sketch.selectionSet");call(w,"sketch.lookAt");QTest::qWait(100);
        auto* fix=w.findChild<QPushButton*>("sketch.constraint.fixed");QVERIFY(fix);for(auto* area:w.findChildren<QScrollArea*>())if(area->isAncestorOf(fix))area->ensureWidgetVisible(fix);
#ifdef Q_OS_MACOS
        QProcess activate;activate.start("osascript",{"-e",QString("tell application \"System Events\" to set frontmost of first application process whose unix id is %1 to true").arg(QCoreApplication::applicationPid())});QVERIFY(activate.waitForFinished(5000));QCOMPARE(activate.exitCode(),0);
#endif
        w.raise();w.activateWindow();w.windowHandle()->requestActivate();QApplication::setActiveWindow(&w);QTRY_VERIFY(w.isActiveWindow());
        // One initial harness activation; no focus/selection repair between user steps.
        auto click=[&]{const auto p=view->projectSketchPoint(clickU,clickV,"XY");const QPoint at(qRound(p.value("x").toDouble()),qRound(p.value("y").toDouble()));QVERIFY(view->rect().contains(at));QVERIFY(!view->childAt(at));QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,at);QVERIFY(w.isActiveWindow());QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QCOMPARE(session().value("tool"),QJsonValue("constraint"));QVERIFY(session().value("selection").toObject().value("entityIds").toArray().isEmpty());};
        const auto baseline=draft();QTest::mouseClick(fix,Qt::LeftButton);click();QCOMPARE(role(circle.value("id")),QString("fixed"));const auto fixed=draft();const auto relation=parameters().value("constraints").toArray().last().toObject();QCOMPARE(relation.value("type"),QJsonValue("fixedCircle"));QCOMPARE(relation.value("radiusMm"),circle.value("radiusMm"));
        const auto requested=qEnvironmentVariable("ALLOY_FIX_CIRCLE_MODES_EVIDENCE");QTemporaryDir temporary;QVERIFY(temporary.isValid());const auto output=(requested.isEmpty()?temporary.path():requested)+"/"+mode;QDir().mkpath(output);
        QTest::qWait(100);call(w,"capture.screenshot",{{"path",output+"/native-fixed.png"}});const auto fixedState=w.state();for(const auto& g:fixedState.value("viewport").toObject().value("gpu").toObject().value("sketchConstraintGlyphs").toArray())QVERIFY(g.toObject().value("kind")!="fixedCircle");
        click();QCOMPARE(draft(),baseline);QTest::keyClick(view,Qt::Key_Escape);QCOMPARE(session().value("tool"),QJsonValue("select"));call(w,"history.undo");QCOMPARE(draft(),fixed);call(w,"history.redo");QCOMPARE(draft(),baseline);call(w,"history.undo");QCOMPARE(draft(),fixed);
        const auto history=session().value("draftHistory");const auto refusal=w.executeApi("sketch.dimensionUpdate",{{"dimensionId",editDimension},{"value",40},{"units","mm"}});QVERIFY2(!refusal.value("ok").toBool(),QJsonDocument(refusal).toJson().constData());QCOMPARE(draft(),fixed);QCOMPARE(session().value("draftHistory"),history);
        call(w,"sketch.fixToggle",{{"entityIds",QJsonArray{circle.value("id")}}});QCOMPARE(draft(),baseline);call(w,"sketch.dimensionUpdate",{{"dimensionId",editDimension},{"value",40},{"units","mm"}});const auto edited=draft();QVERIFY(edited!=baseline);QCOMPARE(parameters().value("constraints"),baseline.value("parameters").toObject().value("constraints"));
        call(w,"sketch.fixToggle",{{"entityIds",QJsonArray{circle.value("id")}}});QCOMPARE(role(circle.value("id")),QString("fixed"));const auto final=draft();call(w,"sketch.finish");const auto authored=call(w,"document.inspect").value("authored");call(w,"document.save",{{"path",output+"/fixed.alloy"}});call(w,"document.open",{{"path",output+"/fixed.alloy"}});QCOMPARE(call(w,"document.inspect").value("authored"),authored);call(w,"sketch.edit",{{"sketchId",final.value("id")}});QCOMPARE(draft(),final);QCOMPARE(role(circle.value("id")),QString("fixed"));QTest::qWait(150);call(w,"capture.screenshot",{{"path",output+"/native-reopened.png"}});
        QFile file(output+"/receipt.json");QVERIFY(file.open(QIODevice::WriteOnly));file.write(QJsonDocument(QJsonObject{{"result","passed"},{"mode",mode},{"baseline",baseline},{"fixed",fixed},{"edited",edited},{"final",final},{"fixedState",fixedState},{"reopenedState",w.state()},{"refusal",refusal}}).toJson());call(w,"sketch.cancel");w.close();
    }
};
QTEST_MAIN(SketchFixCircleModesControls)
#include "sketch_fix_circle_modes_controls.moc"
