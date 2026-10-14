// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QDir>
#include <QJsonDocument>
#include <QLineEdit>
#include <QProcess>
#include <QTest>
#include <QWindow>
#include <cmath>

class SingleCornerFilletControls final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& w,const QString& name,const QJsonObject& args={}) {
        const auto response=w.executeApi(name,args);if(!response.value("ok").toBool())qFatal("%s",QJsonDocument(response).toJson().constData());return response.value("result").toObject();
    }
private slots:
    void singleCancelAndTwoCornerBatch() {
        AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1500,950);w.show();QVERIFY(QTest::qWaitForWindowExposed(&w));
        ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(child))view=v;QVERIFY(view);
#ifdef Q_OS_MACOS
        QProcess activate;activate.start("osascript",{"-e",QString("tell application \"System Events\" to set frontmost of first application process whose unix id is %1 to true").arg(QCoreApplication::applicationPid())});QVERIFY(activate.waitForFinished(5000));QCOMPARE(activate.exitCode(),0);
#endif
        w.raise();w.activateWindow();w.windowHandle()->requestActivate();QApplication::setActiveWindow(&w);QTRY_VERIFY(w.isActiveWindow());
        call(w,"sketch.begin",{{"plane","XY"},{"units","mm"}});
        const QPointF points[]={{0,0},{60,0},{60,40},{0,40},{0,0}};
        for(int i=0;i<4;++i)call(w,"sketch.line",{{"x1",points[i].x()},{"y1",points[i].y()},{"x2",points[i+1].x()},{"y2",points[i+1].y()},{"units","mm"}});
        auto session=[&]{return w.state().value("activeSession").toObject();};auto draft=[&]{return session().value("draftSketch").toObject();};
        call(w,"sketch.lookAt");call(w,"view.zoom",{{"factor",0.65}});
        auto pixel=[&](double u,double v){const auto p=call(w,"view.projectSketchPoint",{{"sketchId",draft().value("id")},{"uMm",u},{"vMm",v}});return QPoint(qRound(p.value("x").toDouble()),qRound(p.value("y").toDouble()));};
        const auto before=draft();call(w,"sketch.tool",{{"tool","fillet"}});QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(60,0));
        auto* radius=w.findChild<QLineEdit*>("sketch.fillet.radius");QVERIFY(radius);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(radius));
        QCOMPARE(session().value("pendingFillet").toObject().value("corners").toArray().size(),1);QVERIFY(session().value("pendingFillet").toObject().contains("previewDraft"));QCOMPARE(draft(),before);
        QTest::keyClick(radius,Qt::Key_Escape);QTest::qWait(30);QCOMPARE(draft(),before);QVERIFY(!session().contains("pendingFillet"));
        call(w,"sketch.tool",{{"tool","fillet"}});QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(60,0));
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(radius));QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(0,40));
        QCOMPARE(session().value("pendingFillet").toObject().value("corners").toArray().size(),2);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(radius));
        QTest::keyClick(radius,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(radius,"6");QTest::keyClick(radius,Qt::Key_Return);QTest::qWait(30);QVERIFY(!session().contains("pendingFillet"));
        int arcs=0;for(const auto& value:draft().value("parameters").toObject().value("entities").toArray())if(value.toObject().value("type")=="arc"){++arcs;QVERIFY(std::abs(value.toObject().value("radiusMm").toDouble()-6)<1e-6);}QCOMPARE(arcs,2);
        const auto out=qEnvironmentVariable("ALLOY_SINGLE_FILLET_EVIDENCE");if(!out.isEmpty()){QVERIFY(QDir().mkpath(out));call(w,"capture.screenshot",{{"path",out+"/two-corner-batch.png"}});}
        call(w,"sketch.cancel");w.close();
    }
};
QTEST_MAIN(SingleCornerFilletControls)
#include "single_corner_fillet_controls.moc"
