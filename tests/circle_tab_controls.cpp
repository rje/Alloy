// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QLineEdit>
#include <QMouseEvent>
#include <QTemporaryDir>
#include <QTest>
#include <cmath>

// Injected Qt events exercise the documented keys on Alloy's real GPU view.
class CircleTabControlsTest final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& w,const QString& command,const QJsonObject& args={}) {
        const auto response=w.execute(command,args);
        if(!response["ok"].toBool())qFatal("%s",QJsonDocument(response).toJson().constData());
        return response["result"].toObject();
    }
    QJsonObject active(AlloyWindow& w) {return w.state()["activeSession"].toObject();}
    QJsonObject draft(AlloyWindow& w) {return active(w)["draftSketch"].toObject();}
private slots:
    void sourceTabEnterAndCancel() {
        QTemporaryDir temporary;auto output=qEnvironmentVariable("ALLOY_CIRCLE_TAB_EVIDENCE");
        if(output.isEmpty())output=temporary.path();QDir().mkpath(output);
        AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1300,900);w.show();w.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&w));
        QApplication::setActiveWindow(&w);
        ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;
        QVERIFY(view);call(w,"sketch.begin",{{"plane","XZ"},{"units","mm"}});call(w,"sketch.lookAt");
        call(w,"view.zoom",{{"factor",8}});
        const auto sketchId=draft(w)["id"];
        auto pixel=[&](double u,double v){const auto p=call(w,"view.projectSketchPoint",{{"sketchId",sketchId},{"uMm",u},{"vMm",v}});
            return QPoint(qRound(p["x"].toDouble()),qRound(p["y"].toDouble()));};
        view->setFocus();QTest::keyClick(view,Qt::Key_C);QCOMPARE(active(w)["tool"].toString(),QString("circle"));
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(0,0));QVERIFY(active(w).contains("firstPoint"));
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
        const auto before=draft(w);QTest::keyClick(view,Qt::Key_2);
        auto* input=w.findChild<QLineEdit*>("sketch.inlineValue");QVERIFY(input && input->isVisible());
        QCOMPARE(active(w)["pendingCircleDiameter"].toObject()["phase"].toString(),QString("editing"));
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(input));
        QTest::keyClick(input,Qt::Key_Tab);
        QCOMPARE(active(w)["pendingCircleDiameter"].toObject()["phase"].toString(),QString("locked"));
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(input));
        QCOMPARE(draft(w),before);QVERIFY(input->isVisible() && input->isReadOnly());
        QVERIFY(view->diagnostics()["cursorPreviewVisible"].toBool());
        QVERIFY(w.state()["inlineEditor"].toObject()["visible"].toBool());
        QApplication::processEvents();QTest::qWait(40);
        call(w,"capture.screenshot",{{"path",output+"/circle-tab-locked.png"}});
        QTest::keyClick(QApplication::focusWidget(),Qt::Key_Return);
        QVERIFY(!active(w).contains("pendingCircleDiameter"));QVERIFY(!input->isVisible());
        QCOMPARE(active(w)["tool"].toString(),QString("circle"));
        const auto placed=draft(w)["parameters"].toObject();QCOMPARE(placed["entities"].toArray().size(),1);
        const auto circle=placed["entities"].toArray()[0].toObject();QCOMPARE(circle["type"].toString(),QString("circle"));
        QCOMPARE(circle["radiusMm"].toDouble(),1.0);QCOMPARE(placed["dimensions"].toArray().size(),1);
        QCOMPARE(placed["dimensions"].toArray()[0].toObject()["valueMm"].toDouble(),2.0);
        const auto centerId=circle["centerPointId"];
        for(const auto& value:placed["points"].toArray())if(value.toObject()["id"]==centerId){QCOMPARE(value.toObject()["uMm"].toDouble(),0.0);QCOMPARE(value.toObject()["vMm"].toDouble(),0.0);}
        call(w,"capture.screenshot",{{"path",output+"/circle-committed.png"}});

        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(5,0));QVERIFY(active(w).contains("firstPoint"));
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
        QTest::keyClick(view,Qt::Key_3);QVERIFY(input->isVisible() && !input->isReadOnly());QTest::keyClick(input,Qt::Key_Tab);
        QCOMPARE(active(w)["pendingCircleDiameter"].toObject()["phase"].toString(),QString("locked"));
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(input));
        QTest::keyClick(QApplication::focusWidget(),Qt::Key_Escape);
        QVERIFY(!active(w).contains("pendingCircleDiameter"));QVERIFY(!input->isVisible());
        QVERIFY(!input->isReadOnly());
        QCOMPARE(draft(w)["parameters"].toObject(),placed);
        call(w,"sketch.tool",{{"tool","select"}});call(w,"sketch.finish");
        const auto authored=call(w,"document.inspect").value("authored");
        const auto path=output+"/circle-tab.alloy";call(w,"document.save",{{"path",path}});
        call(w,"document.open",{{"path",path}});QCOMPARE(call(w,"document.inspect").value("authored"),authored);
        QFile evidence(output+"/state.json");QVERIFY(evidence.open(QIODevice::WriteOnly));evidence.write(QJsonDocument(w.state()).toJson(QJsonDocument::Indented));
        w.close();
    }
};
QTEST_MAIN(CircleTabControlsTest)
#include "circle_tab_controls.moc"
