// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QJsonDocument>
#include <QProcess>
#include <QPushButton>
#include <QScrollArea>
#include <QTest>
#include <QWindow>

class SketchCollinearControls final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& args={}){
        const auto response=window.executeApi(command,args);
        if(!response["ok"].toBool())qFatal("%s",QJsonDocument(response).toJson(QJsonDocument::Compact).constData());
        return response["result"].toObject();
    }
    QJsonObject session(AlloyWindow& window){return window.state()["activeSession"].toObject();}
private slots:
    void twoTopLinesAndBadge(){
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1500,950);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        ReferenceView* view=nullptr;for(auto* child:window.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;
        QVERIFY(view);
        call(window,"sketch.begin",{{"plane","XY"},{"units","mm"},{"name","Native Collinear plate"}});
        auto addLine=[&](double x1,double y1,double x2,double y2){
            call(window,"sketch.line",{{"x1",x1},{"y1",y1},{"x2",x2},{"y2",y2},{"units","mm"}});
            return session(window)["draftSketch"].toObject()["parameters"].toObject()["entities"].toArray().last().toObject();
        };
        addLine(0,0,60,0);addLine(60,0,60,40);
        const auto first=addLine(60,40,40,40),second=addLine(40,40,0,40.5);
        addLine(0,40.5,0,0);
        call(window,"sketch.lookAt");
        auto* button=window.findChild<QPushButton*>("sketch.constraint.collinear");QVERIFY(button);
        for(auto* area:window.findChildren<QScrollArea*>())if(area->isAncestorOf(button))area->ensureWidgetVisible(button);
#ifdef Q_OS_MACOS
        QProcess activation;activation.start("osascript",{"-e",QString("tell application \"System Events\" to set frontmost of first application process whose unix id is %1 to true").arg(QCoreApplication::applicationPid())});
        QVERIFY(activation.waitForFinished(5000));QCOMPARE(activation.exitCode(),0);
#endif
        window.raise();window.activateWindow();window.windowHandle()->requestActivate();QApplication::setActiveWindow(&window);
        QTRY_VERIFY(window.isActiveWindow());QVERIFY(button->isVisible());
        QTest::mouseClick(button,Qt::LeftButton);QCOMPARE(session(window)["constraintType"].toString(),QString("collinear"));
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
        auto pixel=[&](double u,double v){const auto p=call(window,"view.projectSketchPoint",{{"sketchId",session(window)["draftSketch"].toObject()["id"]},{"uMm",u},{"vMm",v}});
            return QPoint(qRound(p["x"].toDouble()),qRound(p["y"].toDouble()));};
        const auto firstPixel=pixel(50,40),secondPixel=pixel(20,40.25);
        QVERIFY(view->rect().contains(firstPixel));QVERIFY(view->rect().contains(secondPixel));
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,firstPixel);
        QCOMPARE(session(window)["selection"].toObject()["entityIds"].toArray(),QJsonArray{first["id"]});
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,secondPixel);
        const auto relations=session(window)["draftSketch"].toObject()["parameters"].toObject()["constraints"].toArray();
        QCOMPARE(relations.size(),1);QCOMPARE(relations[0].toObject()["type"].toString(),QString("collinear"));
        QCOMPARE(relations[0].toObject()["entityIds"].toArray(),(QJsonArray{first["id"],second["id"]}));
        const auto glyphs=window.state()["viewport"].toObject()["gpu"].toObject()["sketchConstraintGlyphs"].toArray();
        int badges=0;QJsonObject badge;for(const auto& value:glyphs)if(value.toObject()["constraintId"]==relations[0].toObject()["id"]){++badges;badge=value.toObject();}
        QCOMPARE(badges,1);
        const auto evidence=qEnvironmentVariable("ALLOY_SKETCH_COLLINEAR_EVIDENCE");
        if(!evidence.isEmpty()){QDir().mkpath(evidence);call(window,"capture.screenshot",{{"path",evidence+"/native-collinear-plate.png"}});}
        QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QTest::keyClick(view,Qt::Key_Escape);
        const auto bounds=badge["bounds"].toObject();
        const QPoint target(qRound(bounds["x"].toDouble()+bounds["width"].toDouble()/2),
            qRound(bounds["y"].toDouble()+bounds["height"].toDouble()/2));
        QVERIFY(view->rect().contains(target));QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,target);
        QCOMPARE(session(window)["selection"].toObject()["constraintIds"].toArray(),QJsonArray{relations[0].toObject()["id"]});
        QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QTest::keyClick(view,Qt::Key_Delete);
        QCOMPARE(session(window)["draftSketch"].toObject()["parameters"].toObject()["constraints"].toArray().size(),0);
        call(window,"sketch.cancel");window.close();
    }
};
QTEST_MAIN(SketchCollinearControls)
#include "sketch_collinear_controls.moc"
