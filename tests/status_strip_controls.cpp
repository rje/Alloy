// SPDX-License-Identifier: MIT
#include "AlloyTheme.h"
#include "AlloyWindow.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFontMetrics>
#include <QJsonDocument>
#include <QLabel>
#include <QStatusBar>
#include <QTemporaryDir>
#include <QTest>

class StatusStripControls final : public QObject {
    Q_OBJECT
    static QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& args={}) {
        const auto response=window.executeApi(command,args);
        if(!response.value("ok").toBool())qFatal("%s",QJsonDocument(response).toJson(QJsonDocument::Compact).constData());
        return response.value("result").toObject();
    }
private slots:
    void conciseStatusAndPermanentWarnings() {
        const auto out=qEnvironmentVariable("ALLOY_STATUS_STRIP_EVIDENCE");QVERIFY(!out.isEmpty());QVERIFY(QDir().mkpath(out));
        QTemporaryDir temporary;QVERIFY(temporary.isValid());
        const auto wrongRoot=temporary.filePath(QString(110,'r'));
        QFile ordinaryFile(wrongRoot);QVERIFY(ordinaryFile.open(QIODevice::WriteOnly));ordinaryFile.write("not a recovery directory");ordinaryFile.close();
        const auto oldRoot=qgetenv("ALLOY_RECOVERY_DIR");const bool rootWasSet=qEnvironmentVariableIsSet("ALLOY_RECOVERY_DIR");
        qputenv("ALLOY_RECOVERY_DIR",wrongRoot.toUtf8());
        AlloyTheme::apply(*qApp);
        {
            AlloyWindow window;window.resize(1360,850);window.show();
            QVERIFY(QTest::qWaitForWindowExposed(&window));
            auto* status=window.findChild<QLabel*>("application.status");QVERIFY(status);
            auto* recovery=window.findChild<QLabel*>("document.recovery.warning");QVERIFY(recovery);
            auto* gpu=window.findChild<QLabel*>("application.gpu.warning");QVERIFY(gpu);
            QVERIFY(window.statusBar());
            const auto warning=window.state().value("recovery").toObject().value("warning").toString();
            QVERIFY(warning.contains("Private directory must be a directory"));
            QCOMPARE(recovery->text(),QString("Recovery unavailable"));
            QVERIFY(recovery->isVisible());
            QCOMPARE(recovery->toolTip(),QString("Recovery unavailable: ")+warning);
            QCOMPARE(recovery->accessibleDescription(),recovery->toolTip());
            const auto actualGpu=window.state().value("viewport").toObject().value("gpu").toObject();
            const bool hardware=actualGpu.value("hardwareAccelerated").toBool();
            qInfo("GPU current=%d notice=%d status=%s tooltip=%s",int(hardware),int(gpu->isVisible()),
                qPrintable(status->text()),qPrintable(gpu->toolTip()));
            QTRY_COMPARE(gpu->isVisible(),!hardware);
            window.statusBar()->clearMessage();
            QTRY_VERIFY(status->isVisible());
            if(!hardware) {
                QCOMPARE(gpu->text(),QString("GPU UNQUALIFIED"));
                QVERIFY(gpu->toolTip().contains(actualGpu.value("qualification").toString()));
                QCOMPARE(gpu->accessibleDescription(),gpu->toolTip());
            }
            auto legible=[&] {
                for(auto* label:{status,recovery,gpu}) {
                    if(!label->isVisible())continue;
                    if(!window.statusBar()->rect().contains(label->geometry()))return false;
                    if(label->fontMetrics().horizontalAdvance(label->text())>label->contentsRect().width())return false;
                }
                return true;
            };
            QTRY_VERIFY(legible());
            QVERIFY(status->toolTip().contains("Wheel: zoom"));
            QCOMPARE(status->accessibleDescription(),status->toolTip());
            QVERIFY(status->toolTip().size()>status->text().size());
            call(window,"capture.screenshot",{{"path",out+"/normal-status.png"}});
            window.resize(1040,640);
            QTRY_VERIFY(legible());
            call(window,"view.navigationTool",{{"mode","pan"}});
            QTRY_VERIFY(status->text().contains("Pan"));
            QTRY_VERIFY(legible());
            call(window,"capture.screenshot",{{"path",out+"/minimum-pan-warning.png"}});
            call(window,"view.orbitTool",{{"active",true}});
            QTRY_VERIFY(status->text().contains("Orbit"));
            QTRY_VERIFY(legible());
            const auto priorDetails=recovery->toolTip();
            window.statusBar()->showMessage("Temporary operation status",5000);
            QCOMPARE(window.statusBar()->currentMessage(),QString("Temporary operation status"));
            QVERIFY(recovery->isVisible());
            QCOMPARE(recovery->toolTip(),priorDetails);
            window.statusBar()->clearMessage();
            QTRY_VERIFY(status->isVisible() && legible());
            QCOMPARE(window.state().value("recovery").toObject().value("warning").toString(),warning);
            window.close();
        }
        if(rootWasSet)qputenv("ALLOY_RECOVERY_DIR",oldRoot);else qunsetenv("ALLOY_RECOVERY_DIR");
    }
};
QTEST_MAIN(StatusStripControls)
#include "status_strip_controls.moc"
