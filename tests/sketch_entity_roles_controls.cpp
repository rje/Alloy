// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QFile>
#include <QJsonDocument>
#include <QProcess>
#include <QPushButton>
#include <QScrollArea>
#include <QTest>
#include <QTreeWidget>
#include <QWindow>

class SketchEntityRolesControls final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& w,const QString& c,const QJsonObject& a={}){
        auto r=w.executeApi(c,a);const auto initial=r.value("result").toObject();
        for(int i=0;r.value("completion")=="pending" && i<400;++i){QTest::qWait(25);r=w.executeApi("command.status",{{"session",initial.value("id")},{"generation",initial.value("previewGeneration")}});}
        if(!r.value("ok").toBool() || r.value("completion")=="pending")qFatal("%s",QJsonDocument(r).toJson().constData());return r.value("result").toObject();
    }
private slots:
    void liveListNativeSelection(){
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
        auto* find=w.findChild<QPushButton*>("sketch.findUnderconstrained");auto* compute=w.findChild<QPushButton*>("sketch.compute");
        auto* list=w.findChild<QTreeWidget*>("sketch.underconstrained");QVERIFY(find&&compute&&list);
        for(auto* area:w.findChildren<QScrollArea*>())if(area->isAncestorOf(find))area->ensureWidgetVisible(find);
#ifdef Q_OS_MACOS
        QProcess activation;activation.start("osascript",{"-e",QString("tell application \"System Events\" to set frontmost of first application process whose unix id is %1 to true").arg(QCoreApplication::applicationPid())});
        QVERIFY(activation.waitForFinished(5000));QCOMPARE(activation.exitCode(),0);
#endif
        w.raise();w.activateWindow();w.windowHandle()->requestActivate();QApplication::setActiveWindow(&w);QTRY_VERIFY(w.isActiveWindow());
        // Initial activation ends here. No later step repairs focus or selection.
        QTest::mouseClick(find,Qt::LeftButton);QVERIFY(list->isVisible());QVERIFY(session().value("findUnderconstrained").toBool());
        for(auto* area:w.findChildren<QScrollArea*>())if(area->isAncestorOf(list))area->ensureWidgetVisible(list);
        auto* first=list->topLevelItem(0);QVERIFY(first&&!first->isHidden());const auto id=first->data(0,Qt::UserRole).toString();
        const auto beforeCamera=w.state().value("viewport").toObject().value("navigation");
        const auto target=list->visualItemRect(first).center();QCOMPARE(list->viewport()->childAt(target),nullptr);
        QTest::mouseClick(list->viewport(),Qt::LeftButton,Qt::NoModifier,target);
        QCOMPARE(session().value("selection").toObject().value("entityIds").toArray(),QJsonArray{id});
        QTRY_VERIFY(list->hasFocus());QVERIFY(w.state().value("viewport").toObject().value("navigation")!=beforeCamera);
        QCOMPARE(session().value("draftSketch"),initial.value("draftSketch"));QCOMPARE(session().value("draftHistory"),initial.value("draftHistory"));
        QTest::keyClick(list,Qt::Key_Down);
        const auto secondId=list->topLevelItem(1)->data(0,Qt::UserRole).toString();
        QCOMPARE(session().value("selection").toObject().value("entityIds").toArray(),QJsonArray{secondId});QVERIFY(list->hasFocus());
        QTest::mouseClick(compute,Qt::LeftButton);QCOMPARE(session().value("solverDiagnostics"),diagnostics);
        QCOMPARE(session().value("draftHistory"),initial.value("draftHistory"));
        const auto output=qEnvironmentVariable("ALLOY_ENTITY_ROLES_EVIDENCE");
        if(!output.isEmpty()){
            call(w,"sketch.lookAt");call(w,"capture.screenshot",{{"path",output+"/native-list.png"}});
            QFile receipt(output+"/native-state.json");QVERIFY(receipt.open(QIODevice::WriteOnly));receipt.write(QJsonDocument(w.state()).toJson());
        }
        QTest::mouseClick(find,Qt::LeftButton);QVERIFY(!list->isVisible());
        call(w,"sketch.cancel");QCOMPARE(call(w,"document.inspect").value("authored"),doc.value("authored"));
        w.close();
    }
};
QTEST_MAIN(SketchEntityRolesControls)
#include "sketch_entity_roles_controls.moc"
