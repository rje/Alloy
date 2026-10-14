// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include <QApplication>
#include <QDir>
#include <QJsonDocument>
#include <QMouseEvent>
#include <QTest>
#include <QToolButton>

class HistoryReorderControlsTest final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& name,const QJsonObject& args={}) {
        const auto response=window.execute(name,args);
        if(!response["ok"].toBool())qFatal("%s",QJsonDocument(response).toJson().constData());
        return response["result"].toObject();
    }
    static void movePressedTile(QToolButton* source,const QPoint& global){
        const auto local=source->mapFromGlobal(global);
        QMouseEvent move(QEvent::MouseMove,QPointF(local),QPointF(global),
            Qt::NoButton,Qt::LeftButton,Qt::NoModifier);
        QApplication::sendEvent(source,&move);
    }
private slots:
    void bottomFilletAfterMirror() {
        const auto fixture=qEnvironmentVariable("ALLOY_REORDER_FIXTURE");
        const auto evidence=qEnvironmentVariable("ALLOY_REORDER_EVIDENCE");
        QVERIFY(!fixture.isEmpty() && !evidence.isEmpty());QDir().mkpath(evidence);
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1440,900);window.show();window.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        call(window,"document.open",{{"path",fixture}});
        const auto before=call(window,"document.inspect")["authored"].toObject();
        const auto features=before["features"].toArray();QVERIFY(features.size()>=2);
        const auto sourceId=features[features.size()-2].toObject()["id"].toString();
        const auto mirrorId=features.last().toObject()["id"].toString();
        QCOMPARE(features[features.size()-2].toObject()["type"].toString(),QString("fillet"));
        QCOMPARE(features.last().toObject()["type"].toString(),QString("featureMirror"));
        auto* source=window.findChild<QToolButton*>("timeline.feature."+sourceId);
        auto* anchor=window.findChild<QToolButton*>("timeline.feature."+mirrorId);
        QVERIFY(source && anchor);QTRY_VERIFY(source->isVisible() && anchor->isVisible());
        const auto drop=anchor->mapToGlobal(QPoint(anchor->width()/2,anchor->height()/2));
        auto gesture=[&]{
            QTest::mousePress(source,Qt::LeftButton,Qt::NoModifier,QPoint(12,12));
            movePressedTile(source,drop);
            QTRY_VERIFY(window.state()["ui"].toObject()["timeline"].toObject()["insertionMarkerVisible"].toBool());
            QCOMPARE(window.state()["ui"].toObject()["timeline"].toObject()["insertionAfterFeatureId"].toString(),mirrorId);
        };
        gesture();
        auto* focus=QApplication::focusWidget();QVERIFY(focus);
        QTest::keyClick(focus,Qt::Key_Escape);
        QVERIFY(!window.state()["ui"].toObject()["timeline"].toObject()["insertionMarkerVisible"].toBool());
        QTest::mouseRelease(source,Qt::LeftButton,Qt::NoModifier,source->mapFromGlobal(drop));
        QVERIFY(window.state()["activeSession"].toObject().isEmpty());
        QCOMPARE(call(window,"document.inspect")["authored"].toObject(),before);
        gesture();
        call(window,"capture.screenshot",{{"path",evidence+"/native-insertion-marker.png"}});
        QTest::mouseRelease(source,Qt::LeftButton,Qt::NoModifier,source->mapFromGlobal(drop));
        QTRY_VERIFY(window.state()["activeSession"].toObject().isEmpty());
        const auto after=call(window,"document.inspect")["authored"].toObject();
        QCOMPARE(after["features"].toArray().last().toObject()["id"].toString(),sourceId);
        QCOMPARE(after["features"].toArray()[after["features"].toArray().size()-2].toObject()["id"].toString(),mirrorId);
        call(window,"capture.screenshot",{{"path",evidence+"/native-reordered.png"}});
        window.hide();
    }
};
QTEST_MAIN(HistoryReorderControlsTest)
#include "history_reorder_controls.moc"
