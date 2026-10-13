// SPDX-License-Identifier: MIT
#include "AlloyTheme.h"
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include "SketchSpline.h"
#include <QApplication>
#include <QAction>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTest>

class FitSplineOffsetControls final:public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& name,const QJsonObject& arguments={}){
        const auto response=window.execute(name,arguments);
        if(!response["ok"].toBool())qFatal("%s",QJsonDocument(response).toJson().constData());
        return response["result"].toObject();
    }
    QJsonObject session(AlloyWindow& window){return window.state()["activeSession"].toObject();}
    QJsonObject draft(AlloyWindow& window){return session(window)["draftSketch"].toObject();}
private slots:
    void actualSelectionSignedPreviewCancelAndAccept(){
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1500,950);window.show();window.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&window));QApplication::setActiveWindow(&window);
        ReferenceView* view=nullptr;for(auto* child:window.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;
        QVERIFY(view);
        call(window,"sketch.create",{{"plane","XY"}});
        call(window,"sketch.splineAdd",{{"points",QJsonArray{
            QJsonObject{{"uMm",0},{"vMm",-20}},QJsonObject{{"uMm",8},{"vMm",20}},
            QJsonObject{{"uMm",-6},{"vMm",60}},QJsonObject{{"uMm",4},{"vMm",100}}}}});
        call(window,"view.zoom",{{"factor",0.4}});
        const auto before=draft(window),source=before["parameters"].toObject()["entities"].toArray().first().toObject();
        const auto fit=SketchSpline::evaluate(before["parameters"].toObject(),source);
        const auto onCurve=fit.curve->Value(.4);const auto pixel=view->projectSketchPoint(onCurve.X(),onCurve.Y(),"XY");
        const QPoint pick(qRound(pixel["x"].toDouble()),qRound(pixel["y"].toDouble()));QVERIFY(view->rect().contains(pick));
        auto* action=window.findChild<QAction*>("sketch.tool.offset");QVERIFY(action);action->trigger();
        QCOMPARE(session(window)["tool"].toString(),QString("offset"));QVERIFY(view->hasFocus());
        auto* field=window.findChild<QLineEdit*>("sketch.offset.distance");QVERIFY(field && field->isVisible());
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pick);
        QTRY_VERIFY(session(window)["pendingSplineOffset"].toObject()["valid"].toBool());
        QCOMPARE(draft(window),before);QCOMPARE(session(window)["pendingSplineOffset"].toObject()["sourceEntityId"],source["id"]);
        QTest::mouseClick(field,Qt::LeftButton);QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(field));
        QTest::keyClick(QApplication::focusWidget(),Qt::Key_A,Qt::ControlModifier);
        QTest::keyClicks(QApplication::focusWidget(),"5");
        QTRY_VERIFY(session(window)["pendingSplineOffset"].toObject()["valid"].toBool());
        QCOMPARE(session(window)["pendingSplineOffset"].toObject()["distanceMm"].toDouble(),5.0);
        auto* flip=window.findChild<QPushButton*>("sketch.offset.flip");QVERIFY(flip);QTest::mouseClick(flip,Qt::LeftButton);
        QTRY_COMPARE(session(window)["pendingSplineOffset"].toObject()["distanceMm"].toDouble(),-5.0);
        const auto previewDraft=session(window)["pendingSplineOffset"].toObject()["previewDraft"].toObject();
        QCOMPARE(previewDraft["parameters"].toObject()["entities"].toArray().size(),2);
        QCOMPARE(draft(window),before);
        QTRY_COMPARE(view->diagnostics()["draftNormalEdgeCount"].toInt(),2);
        QTest::qWait(80);
        auto* info=window.findChild<QLabel*>("sketch.info");auto* profile=window.findChild<QLabel*>("sketch.profileStatus");
        QVERIFY(info && profile);qInfo()<<"offset labels"<<info->geometry()<<profile->geometry();
        QVERIFY(info->geometry().bottom()<profile->geometry().top());
        const auto evidence=qEnvironmentVariable("ALLOY_FIT_SPLINE_OFFSET_EVIDENCE");
        if(!evidence.isEmpty()){QDir().mkpath(evidence);call(window,"capture.screenshot",{{"path",evidence+"/native-preview.png"}});}
        auto* cancel=window.findChild<QPushButton*>("sketch.offset.cancel");QVERIFY(cancel);QTest::mouseClick(cancel,Qt::LeftButton);
        QTRY_VERIFY(!session(window).contains("pendingSplineOffset"));QCOMPARE(draft(window),before);
        QTRY_COMPARE(view->diagnostics()["draftNormalEdgeCount"].toInt(),1);
        action->trigger();QVERIFY(view->hasFocus());QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pick);
        QTRY_VERIFY(session(window)["pendingSplineOffset"].toObject()["valid"].toBool());
        QTest::mouseClick(field,Qt::LeftButton);QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(field));
        QTest::keyClick(QApplication::focusWidget(),Qt::Key_A,Qt::ControlModifier);
        QTest::keyClicks(QApplication::focusWidget(),"-5");QTest::keyClick(QApplication::focusWidget(),Qt::Key_Return);
        QTRY_VERIFY(!session(window).contains("pendingSplineOffset"));
        const auto entities=draft(window)["parameters"].toObject()["entities"].toArray();
        QCOMPARE(entities.size(),2);QCOMPARE(entities.last().toObject()["type"].toString(),QString("fitSplineOffset"));
        QCOMPARE(entities.last().toObject()["signedDistanceMm"].toDouble(),-5.0);
        QTRY_COMPARE(view->diagnostics()["draftNormalEdgeCount"].toInt(),2);
        if(!evidence.isEmpty()){
            call(window,"capture.screenshot",{{"path",evidence+"/native-accepted.png"}});
            QFile state(evidence+"/native-state.json");QVERIFY(state.open(QIODevice::WriteOnly));state.write(QJsonDocument(window.state()).toJson(QJsonDocument::Indented));
        }
        call(window,"sketch.cancel");window.close();
    }
};
QTEST_MAIN(FitSplineOffsetControls)
#include "fit_spline_offset_controls.moc"
