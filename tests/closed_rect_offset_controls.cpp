// SPDX-License-Identifier: MIT
#include "AlloyTheme.h"
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QCheckBox>
#include <QDir>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QTest>

class ClosedRectOffsetControls final:public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& name,const QJsonObject& args={}) {
        const auto response=window.execute(name,args);
        if(!response["ok"].toBool())qFatal("%s: %s",qPrintable(name),QJsonDocument(response).toJson().constData());
        return response["result"].toObject();
    }
private slots:
    void nativeProjectedRectangleOffset() {
        const auto design=qEnvironmentVariable("ALLOY_CLOSED_RECT_DESIGN");
        if(design.isEmpty())QSKIP("Actual projected connector design unavailable");
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1500,950);window.show();window.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&window));QApplication::setActiveWindow(&window);QTRY_VERIFY(window.isActiveWindow());
        ReferenceView* view=nullptr;for(auto* widget:window.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(widget))view=candidate;
        QVERIFY(view);
        call(window,"document.open",{{"path",design}});
        const auto features=call(window,"document.inspect")["authored"].toObject()["features"].toArray();
        QString sketchId;for(const auto& value:features)if(value.toObject()["name"]=="Three linked port clearances")sketchId=value.toObject()["id"].toString();
        QVERIFY(!sketchId.isEmpty());call(window,"sketch.edit",{{"sketchId",sketchId}});call(window,"sketch.lookAt");
        const auto original=window.state()["activeSession"].toObject()["draftSketch"].toObject();
        const auto context=window.state()["activeSession"].toObject()["projectionContext"].toObject();
        const auto lines=context["lines"].toArray();QCOMPARE(lines.size(),12);
        const auto line=lines.first().toObject();const double u=(line["u1Mm"].toDouble()+line["u2Mm"].toDouble())/2;
        const double v=(line["v1Mm"].toDouble()+line["v2Mm"].toDouble())/2;
        const auto pixel=call(window,"view.projectSketchPoint",{{"sketchId",sketchId},{"uMm",u},{"vMm",v}});
        const QPoint point(qRound(pixel["x"].toDouble()),qRound(pixel["y"].toDouble()));QVERIFY(view->rect().contains(point));
        call(window,"sketch.tool",{{"tool","offset"}});QTRY_VERIFY(view->hasFocus());
        auto* chain=window.findChild<QCheckBox*>("sketch.offset.chain");QVERIFY(chain&&chain->isVisible()&&chain->isChecked());
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,point);
        QTRY_VERIFY(window.state()["activeSession"].toObject()["pendingClosedRectOffset"].toObject()["valid"].toBool());
        QCOMPARE(window.state()["activeSession"].toObject()["draftSketch"].toObject(),original);
        auto* field=window.findChild<QLineEdit*>("sketch.offset.distance");QVERIFY(field&&field->isVisible());
        QTest::mouseClick(field,Qt::LeftButton);QVERIFY(field->hasFocus());field->selectAll();QTest::keyClicks(field,"1");
        QTRY_VERIFY(window.state()["activeSession"].toObject()["pendingClosedRectOffset"].toObject()["valid"].toBool());
        const auto pending=window.state()["activeSession"].toObject()["pendingClosedRectOffset"].toObject();
        QCOMPARE(pending["sourceEntityIds"].toArray().size(),4);QCOMPARE(pending["distanceMm"].toDouble(),1.0);
        auto* title=window.findChild<QLabel*>("sketch.offset.title");QVERIFY(title&&title->isVisible()&&title->text().contains("rectangle",Qt::CaseInsensitive));
        const auto evidence=qEnvironmentVariable("ALLOY_CLOSED_RECT_EVIDENCE");if(!evidence.isEmpty()){
            QDir().mkpath(evidence);call(window,"capture.screenshot",{{"path",evidence+"/native-rectangle-offset-preview.png"}});
        }
        QTest::keyClick(field,Qt::Key_Escape);QTRY_VERIFY(!window.state()["activeSession"].toObject().contains("pendingClosedRectOffset"));
        QCOMPARE(window.state()["activeSession"].toObject()["draftSketch"].toObject(),original);
        call(window,"sketch.cancel");window.close();
    }
};
QTEST_MAIN(ClosedRectOffsetControls)
#include "closed_rect_offset_controls.moc"
