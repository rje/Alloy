// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QCheckBox>
#include <QContextMenuEvent>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QLineEdit>
#include <QMenu>
#include <QProcess>
#include <QScrollArea>
#include <QTest>
#include <QWindow>

class SketchPointRelationsControls final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& w,const QString& c,const QJsonObject& a={}) {
        auto r=w.executeApi(c,a);const auto initial=r.value("result").toObject();
        for(int i=0;r.value("completion")=="pending" && i<400;++i){QTest::qWait(25);r=w.executeApi("command.status",{{"session",initial.value("id")},{"generation",initial.value("previewGeneration")}});}
        if(!r.value("ok").toBool() || r.value("completion")=="pending")qFatal("%s: %s",qPrintable(c),QJsonDocument(r).toJson().constData());return r.value("result").toObject();
    }
private slots:
    void contextualRevealAndDelete() {
        AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1500,1000);w.show();QVERIFY(QTest::qWaitForWindowExposed(&w));
        ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(child))view=v;QVERIFY(view);
        auto session=[&]{return w.state().value("activeSession").toObject();};
        auto parameters=[&]{return session().value("draftSketch").toObject().value("parameters").toObject();};
        auto glyphs=[&]{return w.state().value("viewport").toObject().value("gpu").toObject().value("sketchConstraintGlyphs").toArray();};
        auto has=[&](QJsonValue id){for(const auto& v:glyphs())if(v.toObject().value("constraintId")==id)return true;return false;};
        call(w,"sketch.begin",{{"plane","XY"},{"units","mm"},{"name","Contextual point relations"}});
        call(w,"sketch.line",{{"x1",-40},{"y1",-10},{"x2",-10},{"y2",-10},{"units","mm"}});const auto a=parameters().value("entities").toArray().last().toObject();
        call(w,"sketch.constraint",{{"type","fixed"},{"pointIds",QJsonArray{a.value("endPointId")}}});
        call(w,"sketch.line",{{"x1",-9},{"y1",-9},{"x2",10},{"y2",15},{"units","mm"}});const auto b=parameters().value("entities").toArray().last().toObject();
        QVERIFY(a.value("endPointId")!=b.value("startPointId"));
        call(w,"sketch.constraint",{{"type","coincident"},{"pointIds",QJsonArray{a.value("endPointId"),b.value("startPointId")}}});const auto pp=parameters().value("constraints").toArray().last().toObject().value("id");
        call(w,"sketch.line",{{"x1",20},{"y1",-20},{"x2",20},{"y2",20},{"units","mm"}});const auto c=parameters().value("entities").toArray().last().toObject();
        call(w,"sketch.line",{{"x1",20},{"y1",5},{"x2",40},{"y2",5},{"units","mm"}});const auto d=parameters().value("entities").toArray().last().toObject();
        call(w,"sketch.constraint",{{"type","pointOnCurve"},{"pointIds",QJsonArray{d.value("startPointId")}},{"entityIds",QJsonArray{c.value("id")}}});const auto pc=parameters().value("constraints").toArray().last().toObject().value("id");
        call(w,"sketch.constraint",{{"type","horizontal"},{"entityIds",QJsonArray{a.value("id")}}});const auto horizontal=parameters().value("constraints").toArray().last().toObject().value("id");
        call(w,"sketch.tool",{{"tool","select"}});call(w,"sketch.selectionSet");call(w,"sketch.lookAt");QTest::qWait(100);
        const auto baseline=session().value("draftSketch");QVERIFY(!has(pp));QVERIFY(has(pc));QVERIFY(has(horizontal));
#ifdef Q_OS_MACOS
        QProcess activate;activate.start("osascript",{"-e",QString("tell application \"System Events\" to set frontmost of first application process whose unix id is %1 to true").arg(QCoreApplication::applicationPid())});QVERIFY(activate.waitForFinished(5000));QCOMPARE(activate.exitCode(),0);
#endif
        w.raise();w.activateWindow();w.windowHandle()->requestActivate();QApplication::setActiveWindow(&w);QTRY_VERIFY(w.isActiveWindow());
        // Initial activation ends here. No inter-step focus or selection repair.
        auto pixel=[&](double u,double v){const auto p=view->projectSketchPoint(u,v,"XY");return QPoint(qRound(p.value("x").toDouble()),qRound(p.value("y").toDouble()));};
        auto click=[&](double u,double v){const auto p=pixel(u,v);QVERIFY(view->rect().contains(p));QVERIFY(!view->childAt(p));QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,p);QVERIFY(w.isActiveWindow());};
        auto* show=w.findChild<QCheckBox*>("sketch.showConstraints");QVERIFY(show);
        for(auto* area:w.findChildren<QScrollArea*>())if(area->isAncestorOf(show))area->ensureWidgetVisible(show);
        QTest::mouseClick(show,Qt::LeftButton,Qt::NoModifier,QPoint(8,show->height()/2));QVERIFY(!session().value("displayOptions").toObject().value("showConstraints").toBool());QVERIFY(glyphs().isEmpty());
        click(-25,-10);QCOMPARE(session().value("selection").toObject().value("entityIds").toArray(),QJsonArray{a.value("id")});QVERIFY(has(horizontal));QVERIFY(!has(pp));QVERIFY(!has(pc));
        click(-10,-10);QVERIFY(!session().value("selection").toObject().value("pointIds").toArray().isEmpty());QVERIFY(has(pp));QVERIFY(!has(pc));QCOMPARE(session().value("draftSketch"),baseline);
        const auto output=qEnvironmentVariable("ALLOY_POINT_RELATIONS_EVIDENCE");if(!output.isEmpty())QDir().mkpath(output);
        QJsonArray receipts;
        auto record=[&](QString name){receipts.append(QJsonObject{{"step",name},{"state",w.state()}});if(!output.isEmpty())call(w,"capture.screenshot",{{"path",output+"/"+name+".png"}});};
        record("point-point-reveal");
        auto remove=[&](double u,double v,QJsonValue id){
            const auto p=pixel(u,v);QVERIFY(!view->childAt(p));QContextMenuEvent event(QContextMenuEvent::Mouse,p,view->mapToGlobal(p));QApplication::sendEvent(view,&event);
            QTRY_VERIFY(QApplication::activePopupWidget()!=nullptr);auto* menu=qobject_cast<QMenu*>(QApplication::activePopupWidget());QVERIFY(menu);QTRY_VERIFY(menu->isActiveWindow());
            QAction* action=nullptr;for(auto* item:menu->actions())if(item->objectName()=="sketch.action.deleteCoincident" && item->data().toString()==id.toString())action=item;QVERIFY(action);
            record(id==pp ? "point-point-menu" : "point-curve-menu");QVERIFY(menu->isActiveWindow());
            const auto at=menu->actionGeometry(action).center();QVERIFY(!menu->childAt(at));QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,at);
            QTRY_VERIFY(QApplication::activePopupWidget()==nullptr);QTRY_VERIFY(w.isActiveWindow());
            QTRY_VERIFY(!has(id));
            for(const auto& relation:parameters().value("constraints").toArray())QVERIFY(relation.toObject().value("id")!=id);
        };
        remove(-10,-10,pp);const auto withoutPP=session().value("draftSketch");QVERIFY(withoutPP!=baseline);record("point-point-deleted");
        call(w,"history.undo");QCOMPARE(session().value("draftSketch"),baseline);call(w,"history.redo");QCOMPARE(session().value("draftSketch"),withoutPP);call(w,"history.undo");QCOMPARE(session().value("draftSketch"),baseline);
        click(20,5);QCOMPARE(session().value("selection").toObject().value("pointIds").toArray(),QJsonArray{d.value("startPointId")});QVERIFY(has(pc));QVERIFY(!has(pp));record("point-curve-reveal");
        // Stable ID authority refuses unrelated and stale deletions atomically.
        const auto before=session();QVERIFY(!w.executeApi("sketch.deleteCoincident",{{"constraintId",pp}}).value("ok").toBool());QCOMPARE(session().value("draftSketch"),before.value("draftSketch"));
        remove(20,5,pc);const auto withoutPC=session().value("draftSketch");record("point-curve-deleted");
        QVERIFY(!w.executeApi("sketch.deleteCoincident",{{"constraintId",pc}}).value("ok").toBool());QCOMPARE(session().value("draftSketch"),withoutPC);
        call(w,"history.undo");QCOMPARE(session().value("draftSketch"),baseline);call(w,"history.redo");QCOMPARE(session().value("draftSketch"),withoutPC);call(w,"history.undo");QCOMPARE(session().value("draftSketch"),baseline);
        // Existing badge picks/delete also work for contextually revealed relations.
        click(-25,-10);QVERIFY(has(horizontal));QJsonObject badge;for(const auto& v:glyphs())if(v.toObject().value("constraintId")==horizontal)badge=v.toObject();const auto bounds=badge.value("bounds").toObject();
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,QPoint(qRound(bounds.value("x").toDouble()+bounds.value("width").toDouble()/2),qRound(bounds.value("y").toDouble()+bounds.value("height").toDouble()/2)));
        QCOMPARE(session().value("selection").toObject().value("constraintIds").toArray(),QJsonArray{horizontal});QVERIFY(has(horizontal));QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QTest::keyClick(view,Qt::Key_Backspace);QVERIFY(!has(horizontal));call(w,"history.undo");QCOMPARE(session().value("draftSketch"),baseline);
        const auto sketchId=session().value("draftSketch").toObject().value("id");call(w,"sketch.finish");
        if(!output.isEmpty()){
            const auto authored=call(w,"document.inspect").value("authored");call(w,"document.save",{{"path",output+"/point-relations.alloy"}});call(w,"document.open",{{"path",output+"/point-relations.alloy"}});QCOMPARE(call(w,"document.inspect").value("authored"),authored);
            call(w,"sketch.edit",{{"sketchId",sketchId}});call(w,"sketch.selectionSet");QVERIFY(!has(pp));QVERIFY(has(pc));
            auto* name=w.findChild<QLineEdit*>("sketch.name");auto* scroll=w.findChild<QWidget*>("sketchScrollViewport");QVERIFY(name&&scroll);
            QTRY_VERIFY_WITH_TIMEOUT(name->isVisible() && name->mapTo(scroll,QPoint()).y()>=0 && name->mapTo(scroll,QPoint()).y()<60,3000);
            QTest::qWait(100);QVERIFY(name->mapTo(scroll,QPoint()).y()<60);record("reopened");
            QFile file(output+"/receipt.json");QVERIFY(file.open(QIODevice::WriteOnly));file.write(QJsonDocument(QJsonObject{{"steps",receipts},{"pointPointRelationId",pp},{"pointCurveRelationId",pc},{"baselineDraft",baseline}}).toJson());call(w,"sketch.cancel");
        }
        w.close();
    }
};
QTEST_MAIN(SketchPointRelationsControls)
#include "sketch_point_relations_controls.moc"
