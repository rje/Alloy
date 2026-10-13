// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QAction>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QTemporaryDir>
#include <QTest>

// Injected Qt events run against the real Alloy window and GPU sketch view.
class TwoPointRectangleControlsTest final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& name,const QJsonObject& args={}) {
        const auto response=window.execute(name,args);
        if(!response["ok"].toBool())qFatal("%s",QJsonDocument(response).toJson().constData());
        return response["result"].toObject();
    }
    QJsonObject session(AlloyWindow& window) {return window.state()["activeSession"].toObject();}
    QJsonObject draft(AlloyWindow& window) {return session(window)["draftSketch"].toObject();}
private slots:
    void sourceRectangle() {
        QTemporaryDir temporary;auto output=qEnvironmentVariable("ALLOY_RECTANGLE_EVIDENCE");
        if(output.isEmpty())output=temporary.path();QDir().mkpath(output);
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1400,900);window.show();window.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&window));QApplication::setActiveWindow(&window);
        ReferenceView* view=nullptr;for(auto* child:window.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;
        QVERIFY(view);
        auto* action=window.findChild<QAction*>("solid.action.sketch.twoPointRectangle");QVERIFY(action);action->trigger();
        QCOMPARE(session(window)["command"].toString(),QString("sketch.plane"));
        QCOMPARE(session(window)["pendingTool"].toString(),QString("rectangle"));
        auto* plane=window.findChild<QWidget*>("sketch.plane.XY");QVERIFY(plane);
        QTest::mouseClick(plane,Qt::LeftButton);QTRY_COMPARE(session(window)["tool"].toString(),QString("rectangle"));
        call(window,"sketch.lookAt");
        const auto sketchId=draft(window)["id"];
        auto pixel=[&](double u,double v){const auto p=call(window,"view.projectSketchPoint",{{"sketchId",sketchId},{"uMm",u},{"vMm",v}});
            return QPoint(qRound(p["x"].toDouble()),qRound(p["y"].toDouble()));};
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(0,0));QVERIFY(session(window).contains("firstPoint"));
        const auto directionPixel=pixel(20,-10);QVERIFY(view->rect().contains(directionPixel));
        QMouseEvent motion(QEvent::MouseMove,QPointF(directionPixel),QPointF(view->mapToGlobal(directionPixel)),
            Qt::NoButton,Qt::NoButton,Qt::NoModifier);
        QApplication::sendEvent(view,&motion);
        QTRY_VERIFY(session(window)["pointerPoint"].toObject().contains("uMm"));
        QVERIFY(session(window)["pointerPoint"].toObject()["uMm"].toDouble()>0);
        QVERIFY(session(window)["pointerPoint"].toObject()["vMm"].toDouble()<0);
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
        const auto before=draft(window);QTest::keyClick(QApplication::focusWidget(),Qt::Key_6);
        auto* width=window.findChild<QLineEdit*>("sketch.inlineValue");
        auto* height=window.findChild<QLineEdit*>("sketch.centerRectangle.height");
        auto* widthLabel=window.findChild<QLabel*>("sketch.twoPointRectangle.widthLabel");
        auto* heightLabel=window.findChild<QLabel*>("sketch.twoPointRectangle.heightLabel");
        QVERIFY(width && height && width->isVisible() && height->isVisible());
        QVERIFY(widthLabel && heightLabel && widthLabel->isVisible() && heightLabel->isVisible());
        QCOMPARE(width->placeholderText(),QString("Width"));QCOMPARE(height->placeholderText(),QString("Height"));
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(width));
        QTest::keyClick(QApplication::focusWidget(),Qt::Key_0);
        QTest::keyClick(QApplication::focusWidget(),Qt::Key_Tab);
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(height));
        QTest::keyClicks(QApplication::focusWidget(),"30");
        QCOMPARE(session(window)["pendingTwoPointRectangle"].toObject()["uSign"].toInt(),1);
        QCOMPARE(session(window)["pendingTwoPointRectangle"].toObject()["vSign"].toInt(),-1);
        QCOMPARE(draft(window),before);
        QVERIFY(view->diagnostics()["cursorPreviewVisible"].toBool());
        call(window,"capture.screenshot",{{"path",output+"/rectangle-preview.png"}});
        QTest::keyClick(QApplication::focusWidget(),Qt::Key_Return);
        QVERIFY(!session(window).contains("pendingTwoPointRectangle"));
        QCOMPARE(session(window)["tool"].toString(),QString("rectangle"));
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
        const auto parameters=draft(window)["parameters"].toObject();
        QCOMPARE(parameters["entities"].toArray().size(),4);QCOMPARE(parameters["dimensions"].toArray().size(),2);
        QCOMPARE(parameters["dimensions"].toArray()[0].toObject()["valueMm"].toDouble(),60.0);
        QCOMPARE(parameters["dimensions"].toArray()[1].toObject()["valueMm"].toDouble(),30.0);
        call(window,"capture.screenshot",{{"path",output+"/rectangle-committed.png"}});
        QTest::keyClick(QApplication::focusWidget(),Qt::Key_Return);
        QCOMPARE(session(window)["tool"].toString(),QString("select"));
        call(window,"history.undo");QCOMPARE(draft(window),before);
        call(window,"history.redo");QCOMPARE(draft(window)["parameters"].toObject(),parameters);
        call(window,"sketch.tool",{{"tool","circle"}});call(window,"sketch.placePoint",{{"x",pixel(0,0).x()},{"y",pixel(0,0).y()}});
        call(window,"sketch.circleDiameterPreview",{{"text","2"}});
        QVERIFY(!widthLabel->isVisible() && !heightLabel->isVisible() && !width->isReadOnly());
        call(window,"sketch.circleDiameterCancel");
        call(window,"sketch.tool",{{"tool","line"}});call(window,"sketch.placePoint",{{"x",pixel(0,0).x()},{"y",pixel(0,0).y()}});
        call(window,"sketch.lineLengthPreview",{{"text","5"}});
        QVERIFY(!widthLabel->isVisible() && !heightLabel->isVisible() && !width->isReadOnly());
        call(window,"sketch.lineLengthCancel");call(window,"sketch.tool",{{"tool","select"}});
        call(window,"sketch.finish");const auto authored=call(window,"document.inspect").value("authored");
        const auto path=output+"/two-point-rectangle.alloy";call(window,"document.save",{{"path",path}});
        call(window,"document.open",{{"path",path}});QCOMPARE(call(window,"document.inspect").value("authored"),authored);
        QFile state(output+"/state.json");QVERIFY(state.open(QIODevice::WriteOnly));state.write(QJsonDocument(window.state()).toJson(QJsonDocument::Indented));
        window.close();
    }
};
QTEST_MAIN(TwoPointRectangleControlsTest)
#include "two_point_rectangle_controls.moc"
