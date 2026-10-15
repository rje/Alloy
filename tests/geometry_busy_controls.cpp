// SPDX-License-Identifier: MIT
#include "AlloyTheme.h"
#include "AlloyWindow.h"
#include <QApplication>
#include <QDir>
#include <QJsonDocument>
#include <QStatusBar>
#include <QTest>

class GeometryBusyControls final : public QObject {
    Q_OBJECT
private slots:
    void feedbackAndCleanup() {
        const auto out=qEnvironmentVariable("ALLOY_BUSY_EVIDENCE");
        QVERIFY(!out.isEmpty());QVERIFY(QDir().mkpath(out));
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1360,850);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        int starts=0;bool waitCursorObserved=false,captureSaved=false;
        connect(window.statusBar(),&QStatusBar::messageChanged,&window,[&](const QString& message){
            if(!message.startsWith("Computing "))return;
            ++starts;
            waitCursorObserved=QApplication::overrideCursor() && QApplication::overrideCursor()->shape()==Qt::WaitCursor;
            if(starts==1) {
                QVERIFY(message.contains("Input resumes when finished"));
                QVERIFY(message.contains("Cancellation unavailable"));
                // Capture actual Qt chrome synchronously while it contains the
                // busy message. No event pumping, synthetic sleep or geometry hook.
                captureSaved=window.statusBar()->grab().save(out+"/computing-status.png");
            }
        });
        QApplication::setOverrideCursor(Qt::CrossCursor);
        const auto made=window.executeApi("solid.box",{{"width",40},{"depth",30},{"height",10},{"units","mm"}});
        QVERIFY2(made.value("ok").toBool(),QJsonDocument(made).toJson().constData());
        QCOMPARE(starts,1);QVERIFY(waitCursorObserved);QVERIFY(captureSaved);
        QVERIFY(QApplication::overrideCursor());QCOMPARE(QApplication::overrideCursor()->shape(),Qt::CrossCursor);
        auto work=window.state().value("geometryWork").toObject();
        QVERIFY(!work.value("active").toBool());QVERIFY(!work.value("cancellable").toBool());
        QVERIFY(work.value("last").toObject().value("succeeded").toBool());
        QCOMPARE(work.value("last").toObject().value("command"),QJsonValue("solid.box"));
        QVERIFY(work.value("last").toObject().value("elapsedMs").toDouble()>=0);
        const auto before=window.executeApi("document.inspect").value("result").toObject();
        const auto refused=window.executeApi("feature.edit",{{"featureId","00000000-0000-4000-8000-000000000001"},
            {"parameters",QJsonObject{{"widthMm",42}}}});
        QVERIFY(!refused.value("ok").toBool());QCOMPARE(starts,2);
        work=window.state().value("geometryWork").toObject();
        QVERIFY(!work.value("active").toBool());QVERIFY(!work.value("last").toObject().value("succeeded").toBool());
        QCOMPARE(QApplication::overrideCursor()->shape(),Qt::CrossCursor);
        QVERIFY(!window.statusBar()->currentMessage().startsWith("Computing "));
        const auto after=window.executeApi("document.inspect").value("result").toObject();
        QCOMPARE(after.value("authored"),before.value("authored"));QCOMPARE(after.value("undo"),before.value("undo"));
        QVERIFY(window.executeApi("view.home").value("ok").toBool());QCOMPARE(starts,2);
        const auto body=before.value("evaluation").toObject().value("bodies").toArray().first().toObject();
        const auto edge=body.value("solidEdges").toArray().first().toObject().value("edgeId");
        const auto preview=window.executeApi("solid.filletDialog",{{"targetBodyId",body.value("bodyId")},
            {"edgeIds",QJsonArray{edge}},{"radiusMm",1.0}});
        QVERIFY2(preview.value("ok").toBool(),QJsonDocument(preview).toJson().constData());
        QCOMPARE(starts,3);
        const auto session=window.state().value("activeSession").toObject();
        QVERIFY(session.value("previewed").toBool());
        QVERIFY(window.executeApi("command.cancel",{{"session",session.value("id")}}).value("ok").toBool());
        QCOMPARE(window.executeApi("document.inspect").value("result").toObject().value("authored"),before.value("authored"));
        QCOMPARE(QApplication::overrideCursor()->shape(),Qt::CrossCursor);
        QApplication::restoreOverrideCursor();
        QVERIFY(window.executeApi("capture.screenshot",{{"path",out+"/completed-box.png"}}).value("ok").toBool());
        QVERIFY(window.executeApi("document.save",{{"path",out+"/busy-box.alloy"}}).value("ok").toBool());
        window.close();
    }
};
QTEST_MAIN(GeometryBusyControls)
#include "geometry_busy_controls.moc"
