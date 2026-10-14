// SPDX-License-Identifier: MIT
#include "AlloyTheme.h"
#include "AlloyWindow.h"
#include <QApplication>
#include <QAbstractItemView>
#include <QComboBox>
#include <QDir>
#include <QDialogButtonBox>
#include <QJsonDocument>
#include <QProcess>
#include <QPushButton>
#include <QTest>
#include <QWindow>

class ClosedRectUnionControls final:public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& args={}) {
        const auto response=window.executeApi(command,args);
        if(!response.value("ok").toBool())qFatal("%s: %s",qPrintable(command),QJsonDocument(response).toJson().constData());
        return response.value("result").toObject();
    }
private slots:
    void displayExplicitUnionAndCancel() {
        const auto design=qEnvironmentVariable("ALLOY_CLOSED_RECT_UNION_DESIGN");
        if(design.isEmpty())QSKIP("Actual three-port design unavailable");
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1500,950);window.show();window.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        call(window,"document.open",{{"path",design}});
        const auto before=call(window,"document.inspect");
        QJsonObject sketch,cut;
        for(const auto& row:before.value("authored").toObject().value("features").toArray()) {
            const auto feature=row.toObject();
            if(feature.value("type")=="sketch" && feature.value("parameters").toObject().value("closedRectOffsets").toArray().size()==3)sketch=feature;
        }
        for(const auto& row:before.value("authored").toObject().value("features").toArray()) {
            const auto feature=row.toObject();
            if(feature.value("type")=="extrude" && feature.value("parameters").toObject().value("sketchId")==sketch.value("id")
                && feature.value("parameters").toObject().value("operation")=="cut")cut=feature;
        }
        QVERIFY(!sketch.isEmpty()&&!cut.isEmpty());const auto p=cut.value("parameters").toObject();
        QVERIFY(!p.value("profileIds").toArray().isEmpty());
        auto begin=[&]{call(window,"command.begin",{{"command","solid.extrude"},{"inputs",QJsonObject{{"featureId",cut.value("id")},
            {"sketchId",sketch.value("id")},{"profileIds",p.value("profileIds")},{"extent","toObject"},
            {"endSupport",p.value("endSupport")},{"operation","cut"},{"targetBodyIds",p.value("targetBodyIds")}}}});};
        begin();
        auto* combo=window.findChild<QComboBox*>("extrude.profile");QVERIFY(combo&&combo->isVisible());
        int aggregate=-1,ordinary=-1;for(int i=0;i<combo->count();++i){const auto data=combo->itemData(i).toJsonObject();
            if(data.contains("profileSelection"))aggregate=i;
            if(ordinary<0 && data.value("sketchId")==sketch.value("id") && data.contains("profileId"))ordinary=i;}
        QVERIFY(aggregate>0&&ordinary>aggregate);
        // One initial harness activation after document/command setup.
#ifdef Q_OS_MACOS
        QProcess activation;activation.start("osascript",{"-e",QString("tell application \"System Events\" to set frontmost of first application process whose unix id is %1 to true").arg(QCoreApplication::applicationPid())});QVERIFY(activation.waitForFinished(5000));QCOMPARE(activation.exitCode(),0);
#endif
        window.raise();window.activateWindow();window.windowHandle()->requestActivate();QApplication::setActiveWindow(&window);QTRY_VERIFY(window.isActiveWindow());
        auto choose=[&](int row){
            QTest::mouseClick(combo,Qt::LeftButton);QTRY_VERIFY(combo->view()->isVisible());
            QTest::keyClick(combo->view(),Qt::Key_Home);
            for(int i=0;i<row;++i)QTest::keyClick(combo->view(),Qt::Key_Down);
            QTest::keyClick(combo->view(),Qt::Key_Return);
            QTRY_COMPARE(combo->currentIndex(),row);
        };
        choose(aggregate);
        QTRY_VERIFY(window.state().value("activeSession").toObject().value("inputs").toObject().contains("profileSelection"));
        QTRY_VERIFY(window.state().value("activeSession").toObject().value("previewed").toBool());
        const auto session=window.state().value("activeSession").toObject();
        QCOMPARE(session.value("inputs").toObject().value("profileSelection").toObject().value("groupIds").toArray().size(),3);
        QCOMPARE(session.value("resolvedProfileIds").toArray().size(),4);
        QCOMPARE(combo->currentIndex(),aggregate);
        const auto evidence=qEnvironmentVariable("ALLOY_CLOSED_RECT_UNION_EVIDENCE");
        if(!evidence.isEmpty()){QDir().mkpath(evidence);call(window,"capture.screenshot",{{"path",evidence+"/native-explicit-union.png"}});}
        choose(ordinary);
        QTRY_VERIFY(!window.state().value("activeSession").toObject().value("inputs").toObject().contains("profileSelection"));
        QTRY_VERIFY(window.state().value("activeSession").toObject().value("inputs").toObject().contains("profileId"));
        choose(aggregate);
        QTRY_VERIFY(window.state().value("activeSession").toObject().value("inputs").toObject().contains("profileSelection"));
        choose(0);
        const auto cleared=window.state().value("activeSession").toObject();
        QVERIFY(!cleared.value("inputs").toObject().contains("profileSelection"));
        QVERIFY(cleared.value("inputs").toObject().value("profileIds").toArray().isEmpty());
        QVERIFY(!cleared.value("previewed").toBool());
        auto* buttons=window.findChild<QDialogButtonBox*>("extrude.buttons");QVERIFY(buttons);
        QVERIFY(!buttons->button(QDialogButtonBox::Ok)->isEnabled());
        QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel),Qt::LeftButton);
        QCOMPARE(call(window,"document.inspect").value("authored"),before.value("authored"));
        begin();combo=window.findChild<QComboBox*>("extrude.profile");QVERIFY(combo);
        choose(aggregate);QTRY_VERIFY(window.state().value("activeSession").toObject().value("previewed").toBool());
        buttons=window.findChild<QDialogButtonBox*>("extrude.buttons");QVERIFY(buttons->button(QDialogButtonBox::Ok)->isEnabled());
        QTest::mouseClick(buttons->button(QDialogButtonBox::Ok),Qt::LeftButton);
        QTRY_VERIFY(window.state().value("activeSession").toObject().isEmpty());
        const auto committed=call(window,"document.inspect");
        bool retained=false;for(const auto& row:committed.value("authored").toObject().value("features").toArray())if(row.toObject().value("id")==cut.value("id"))
            retained=row.toObject().value("parameters").toObject().contains("profileSelection");
        QVERIFY(retained);
        call(window,"history.undo");QCOMPARE(call(window,"document.inspect").value("authored"),before.value("authored"));
        call(window,"history.redo");QCOMPARE(call(window,"document.inspect").value("authored"),committed.value("authored"));
    }
};
QTEST_MAIN(ClosedRectUnionControls)
#include "closed_rect_union_controls.moc"
