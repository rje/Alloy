// SPDX-License-Identifier: MIT
#include "AlloyTheme.h"
#include "AlloyWindow.h"
#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QJsonDocument>
#include <QTest>

class ClosedRectUnionControls final:public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& args={}) {
        const auto response=window.execute(command,args);
        if(!response.value("ok").toBool())qFatal("%s: %s",qPrintable(command),QJsonDocument(response).toJson().constData());
        return response.value("result").toObject();
    }
private slots:
    void displayExplicitUnionAndCancel() {
        const auto design=qEnvironmentVariable("ALLOY_CLOSED_RECT_UNION_DESIGN");
        if(design.isEmpty())QSKIP("Actual three-port design unavailable");
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1500,950);window.show();window.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&window));QApplication::setActiveWindow(&window);QTRY_VERIFY(window.isActiveWindow());
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
        call(window,"command.begin",{{"command","solid.extrude"},{"inputs",QJsonObject{{"featureId",cut.value("id")},
            {"sketchId",sketch.value("id")},{"profileIds",p.value("profileIds")},{"extent","toObject"},
            {"endSupport",p.value("endSupport")},{"operation","cut"},{"targetBodyIds",p.value("targetBodyIds")}}}});
        auto* combo=window.findChild<QComboBox*>("extrude.profile");QVERIFY(combo&&combo->isVisible());
        int aggregate=-1;for(int i=0;i<combo->count();++i)if(combo->itemData(i).toJsonObject().contains("profileSelection"))aggregate=i;
        QVERIFY(aggregate>0);
        // The visible row uses the same command as the API. Qt popup delivery
        // on this desktop is separately qualified; no input parity is claimed.
        const auto id=window.state().value("activeSession").toObject().value("id");
        call(window,"command.update",{{"session",id},{"inputs",combo->itemData(aggregate).toJsonObject()}});
        call(window,"command.preview",{{"session",id}});
        QTRY_VERIFY(window.state().value("activeSession").toObject().value("inputs").toObject().contains("profileSelection"));
        QTRY_VERIFY(window.state().value("activeSession").toObject().value("previewed").toBool());
        const auto session=window.state().value("activeSession").toObject();
        QCOMPARE(session.value("inputs").toObject().value("profileSelection").toObject().value("groupIds").toArray().size(),3);
        QCOMPARE(session.value("resolvedProfileIds").toArray().size(),4);
        QCOMPARE(combo->currentIndex(),aggregate);
        const auto evidence=qEnvironmentVariable("ALLOY_CLOSED_RECT_UNION_EVIDENCE");
        if(!evidence.isEmpty()){QDir().mkpath(evidence);call(window,"capture.screenshot",{{"path",evidence+"/native-explicit-union.png"}});}
        call(window,"command.cancel",{{"session",session.value("id")}});
        QCOMPARE(call(window,"document.inspect").value("authored"),before.value("authored"));
        window.close();
    }
};
QTEST_MAIN(ClosedRectUnionControls)
#include "closed_rect_union_controls.moc"
