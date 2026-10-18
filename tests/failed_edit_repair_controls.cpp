// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include "failed_edit_repair_fixtures.h"
#include <QApplication>
#include <QDockWidget>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTest>

// Qt-delivered input only. This does not establish active AppKit/physical input.
class FailedEditRepairTest final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& w,const QString& name,const QJsonObject& args={}) {
        auto result=w.execute(name,args);const auto initial=result.value("result").toObject();
        for(int wait=0;result.value("completion")=="pending" && wait<500;++wait) {
            QTest::qWait(10);result=w.execute("command.status",{{"session",initial.value("id")}});
        }
        if(!result.value("ok").toBool())qFatal("%s",QJsonDocument(result).toJson().constData());
        return result.value("result").toObject();
    }
    QJsonObject inspect(AlloyWindow& w){return call(w,"document.inspect");}
private slots:
    void cachedSelectionRefusalCancelAndInvalidation() {
        QTemporaryDir scratch;const auto folder=qEnvironmentVariable("ALLOY_FAILED_EDIT_EVIDENCE",scratch.path());QDir().mkpath(folder);
        const auto metadata=FailedEditRepairFixtures::create(folder+"/fixtures");const auto ids=metadata.value("failure").toObject();
        AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1360,860);w.show();QVERIFY(QTest::qWaitForWindowExposed(&w));
        call(w,"document.open",{{"path",folder+"/fixtures/support-failure.alloy"}});
        call(w,"selection.set",{{"bodyId",ids.value("targetBodyId")}});
        const auto base=inspect(w);const auto selection=w.state().value("selection");const auto camera=w.state().value("viewport").toObject().value("navigation");
        const auto profile=base.value("evaluation").toObject().value("sketches").toArray().last().toObject().value("profiles").toArray().first().toObject().value("profileId");
        QJsonObject args{{"featureId",ids.value("combineFeatureId")},{"targetBodyId",ids.value("targetBodyId")},
            {"toolBodyIds",QJsonArray{ids.value("splitBodyId")}},{"operation","cut"},{"keepTools",true}};
        // Existing Combine preview exposes an explicit inspection action.
        call(w,"solid.combineDialog",args);
        auto* begin=w.findChild<QPushButton*>("combine.inspectFailedEdit");QVERIFY(begin && begin->isVisible());
        QTest::mouseClick(begin,Qt::LeftButton);
        auto repair=call(w,"history.failedEditInspect");QCOMPARE(repair.value("status").toString(),QString("active"));
        QCOMPARE(repair.value("baseDocumentRevision"),base.value("revision"));
        QCOMPARE(repair.value("diagnostics").toObject().value("code").toString(),QString("support_unavailable"));
        QCOMPARE(repair.value("impactedSketchIds").toArray(),QJsonArray{ids.value("sketchId")});
        QVERIFY(!repair.value("validPrefix").toObject().value("featureIds").toArray().contains(ids.value("sketchId")));
        QCOMPARE(inspect(w),base);
        auto* banner=w.findChild<QLabel*>("failedEditRepair.banner");QVERIFY(banner && banner->isVisible());
        QVERIFY(banner->text().contains("CACHED ACCEPTED SKETCH"));
        auto* profiles=w.findChild<QListWidget*>("failedEditRepair.profiles");QVERIFY(profiles && profiles->count()>0);
        const auto rect=profiles->visualItemRect(profiles->item(0));
        QCOMPARE(QApplication::widgetAt(profiles->viewport()->mapToGlobal(rect.center())),profiles->viewport());
        QTest::mouseClick(profiles->viewport(),Qt::LeftButton,Qt::NoModifier,rect.center());
        const auto selected=w.state().value("activeSession").toObject();
        QCOMPARE(selected.value("selection").toObject().value("profileIds").toArray(),QJsonArray{profile});
        auto* remove=w.findChild<QPushButton*>("failedEditRepair.delete");QVERIFY(remove && remove->isVisible());
        QTest::mouseClick(remove,Qt::LeftButton);
        QVERIFY(!w.findChild<QLabel*>("failedEditRepair.actionError")->text().isEmpty());
        QCOMPARE(w.state().value("activeSession").toObject(),selected);QCOMPARE(inspect(w),base);
        const auto refusal=w.execute("sketch.deleteSelection");QVERIFY(!refusal.value("ok").toBool());
        QCOMPARE(refusal.value("error").toObject().value("code").toString(),QString("support_unavailable"));
        QCOMPARE(w.state().value("activeSession").toObject(),selected);
        QTest::qWait(20);call(w,"capture.screenshot",{{"path",folder+"/cached-selected.png"}});
        auto* cancel=w.findChild<QPushButton*>("failedEditRepair.cancel");QVERIFY(cancel && cancel->isVisible());
        QTest::mouseClick(cancel,Qt::LeftButton);
        QVERIFY(w.state().value("activeSession").toObject().isEmpty());QCOMPARE(inspect(w),base);
        QCOMPARE(w.state().value("selection"),selection);QCOMPARE(w.state().value("viewport").toObject().value("navigation"),camera);
        // A second modeling edit invalidates the cache before any later result.
        args["toolBodyIds"]=QJsonArray{ids.value("deleteBodyId")};
        const auto failed=w.execute("solid.combine",args);QVERIFY(!failed.value("ok").toBool());
        repair=call(w,"history.failedEditInspect");QVERIFY(!repair.isEmpty());const auto oldId=repair.value("id");
        const auto forbidden=w.execute("sketch.finish");QCOMPARE(forbidden.value("error").toObject().value("code").toString(),QString("repair_read_only"));QCOMPARE(inspect(w),base);
        call(w,"feature.edit",{{"featureId",ids.value("combineFeatureId")},{"parameters",QJsonObject{{"toolBodyIds",QJsonArray{ids.value("notchBodyId")}}}}});
        QVERIFY(call(w,"history.failedEditInspect").isEmpty());QVERIFY(w.state().value("activeSession").toObject().isEmpty());
        QVERIFY(!w.execute("command.cancel",{{"session",oldId}}).value("ok").toBool());
        QCOMPARE(inspect(w).value("authored"),base.value("authored"));
        const auto gpu=w.state().value("viewport").toObject().value("gpu").toObject();QVERIFY(gpu.value("hardwareAccelerated").toBool());
        QFile receipt(folder+"/qt-receipt.json");QVERIFY(receipt.open(QIODevice::WriteOnly));receipt.write(QJsonDocument(QJsonObject{{"status","pass"},{"input","Qt-delivered only; locked console; physical input unqualified"},{"gpu",gpu},{"refusal",refusal},{"repair",repair}}).toJson(QJsonDocument::Indented));
    }
};
QTEST_MAIN(FailedEditRepairTest)
#include "failed_edit_repair_controls.moc"
