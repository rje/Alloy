// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "RecoveryStore.h"
#include <QApplication>
#include <QDialog>
#include <QDir>
#include <QFileDialog>
#include <QJsonDocument>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QWindow>

class RecoveryControls final : public QObject {
    Q_OBJECT
    static QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& args={}) {
        const auto response=window.executeApi(command,args);
        if(!response.value("ok").toBool())qFatal("%s: %s",qPrintable(command),qPrintable(QString::fromUtf8(QJsonDocument(response).toJson())));
        return response.value("result").toObject();
    }
private slots:
    void recoverAndCancelSaveAs() {
        QTemporaryDir temporary;QVERIFY(temporary.isValid());
        const auto root=QDir(temporary.path()).filePath("recovery");
        qputenv("ALLOY_RECOVERY_DIR",root.toUtf8());
        QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
        const auto output=qEnvironmentVariable("ALLOY_RECOVERY_CONTROLS_EVIDENCE");
        QVERIFY(!output.isEmpty());QVERIFY(QDir().mkpath(output));
        AlloyTheme::apply(*qApp);
        QJsonObject authored;
        {
            AlloyWindow setup;
            call(setup,"solid.box",{{"width",20},{"depth",12},{"height",5},{"units","mm"}});
            authored=call(setup,"document.inspect").value("authored").toObject();
            call(setup,"document.save",{{"path",temporary.path()+"/setup.alloy"}});
            setup.close();
        }
        {
            RecoveryStore interrupted(root);
            QVERIFY(interrupted.error().isEmpty());
            QVERIFY(interrupted.write("22222222-2222-4222-8222-222222222222",authored,{}).isEmpty());
        }
        AlloyWindow window;window.resize(1180,760);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        window.raise();window.activateWindow();window.windowHandle()->requestActivate();
        QTRY_VERIFY(window.isActiveWindow());
        QCOMPARE(call(window,"document.recoveryList").value("count").toInt(),1);
        bool inspected=false,clicked=false;
        QTimer::singleShot(0,&window,[&]{
            auto* dialog=window.findChild<QDialog*>("document.recovery.dialog");
            QVERIFY(dialog && dialog->isVisible());
            QCOMPARE(QApplication::activeModalWidget(),static_cast<QWidget*>(dialog));
            auto* list=dialog->findChild<QListWidget*>("document.recovery.candidates");
            auto* recover=dialog->findChild<QPushButton*>("document.recovery.recover");
            QVERIFY(list && recover);QCOMPARE(list->count(),1);
            QVERIFY(recover->isEnabled());
            QVERIFY(dialog->grab().save(output+"/recover-work-dialog.png"));
            inspected=true;
            QVERIFY(QApplication::widgetAt(recover->mapToGlobal(recover->rect().center()))==recover);
            QTest::mouseClick(recover,Qt::LeftButton);
            clicked=true;
        });
        call(window,"document.recoveryDialog");
        QVERIFY(inspected && clicked);
        const auto recovered=call(window,"document.inspect");
        QCOMPARE(recovered.value("authored").toObject(),authored);
        QVERIFY(recovered.value("dirty").toBool());QVERIFY(recovered.value("path").toString().isEmpty());

        // Save from the close prompt, then cancel Save As. The clean history of
        // a recovered document must not authorize losing its pathless snapshot.
        bool saveChosen=false,cancelledSaveAs=false;
        QTimer watcher;
        int ticks=0;
        connect(&watcher,&QTimer::timeout,&window,[&]{
            if(++ticks>250)qFatal("Save As stuck: message=%d cancel=%d",saveChosen,cancelledSaveAs);
            if(auto* message=qobject_cast<QMessageBox*>(QApplication::activeModalWidget());message && !saveChosen) {
                auto* save=message->button(QMessageBox::Save);
                QVERIFY(save);saveChosen=true;QTest::mouseClick(save,Qt::LeftButton);
            }else if(saveChosen && !cancelledSaveAs) {
                for(auto* widget:QApplication::topLevelWidgets())
                    if(auto* file=qobject_cast<QFileDialog*>(widget);file && file->windowTitle()=="Save Design") {
                        cancelledSaveAs=true;file->reject();break;
                    }
            }
        });
        watcher.start(20);
        window.close();watcher.stop();
        QVERIFY(saveChosen && cancelledSaveAs);
        QVERIFY(window.isVisible());
        QVERIFY(call(window,"document.inspect").value("dirty").toBool());
        QCOMPARE(call(window,"document.recoveryList").value("count").toInt(),0);
        const auto saved=call(window,"document.save",{{"path",output+"/recovered.alloy"}});
        QVERIFY(!saved.value("dirty").toBool());
        window.close();
    }
};
QTEST_MAIN(RecoveryControls)
#include "recovery_controls.moc"
