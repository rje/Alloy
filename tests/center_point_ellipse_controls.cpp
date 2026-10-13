#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QAction>
#include <QApplication>
#include <QDir>
#include <QJsonDocument>
#include <QLineEdit>
#include <QMouseEvent>
#include <QTest>
#include <cmath>

class CenterPointEllipseControlsTest final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& name,const QJsonObject& arguments={}){
        const auto response=window.execute(name,arguments);
        if(!response.value("ok").toBool())qFatal("%s",QJsonDocument(response).toJson().constData());
        return response.value("result").toObject();
    }
    QJsonObject active(AlloyWindow& window){return window.state().value("activeSession").toObject();}
    QJsonObject draft(AlloyWindow& window){return active(window).value("draftSketch").toObject();}
private slots:
    void nativeEllipseGesture(){
        const auto output=qEnvironmentVariable("ALLOY_ELLIPSE_EVIDENCE");QVERIFY(!output.isEmpty());QVERIFY(QDir().mkpath(output));
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1500,950);window.show();window.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&window));QApplication::setActiveWindow(&window);
        ReferenceView* view=nullptr;for(auto* child:window.findChildren<QWidget*>())
            if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;
        QVERIFY(view);view->setFocus();
        auto* ellipse=window.findChild<QAction*>("sketch.tool.centerPointEllipse");QVERIFY(ellipse);
        auto pixel=[&](double u,double v){const auto projected=call(window,"view.projectSketchPoint",
            {{"sketchId",draft(window).value("id")},{"uMm",u},{"vMm",v}});
            return QPoint(qRound(projected.value("x").toDouble()),qRound(projected.value("y").toDouble()));};
        auto motion=[&](const QPoint& p){QMouseEvent move(QEvent::MouseMove,QPointF(p),QPointF(view->mapToGlobal(p)),
            Qt::NoButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(view,&move);};

        // Actual typed full-axis/Tab-angle/second-axis controls, then cancel.
        call(window,"sketch.begin",{{"plane","XY"}});call(window,"view.zoom",{{"factor",0.4}});ellipse->trigger();
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(0,0));
        QTest::keyClick(view,Qt::Key_1);
        auto* first=window.findChild<QLineEdit*>("sketch.ellipse.firstLength");QVERIFY(first && first->isVisible());
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(first));
        QTest::keyClicks(first,"20");QTest::keyClick(first,Qt::Key_Tab);
        auto* angle=window.findChild<QLineEdit*>("sketch.ellipse.angle");QVERIFY(angle && angle->isVisible());
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(angle));
        angle->selectAll();QTest::keyClicks(angle,"15");QTest::keyClick(angle,Qt::Key_Return);
        QTRY_VERIFY(active(window).contains("secondPoint"));
        auto* second=window.findChild<QLineEdit*>("sketch.ellipse.secondLength");QVERIFY(second && second->isVisible());
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(second));
        QTest::keyClicks(second,"50");QTest::keyClick(second,Qt::Key_Return);
        QCOMPARE(draft(window).value("parameters").toObject().value("entities").toArray().last().toObject().value("type").toString(),QString("ellipse"));
        QCOMPARE(active(window).value("solverDiagnostics").toObject().value("dof").toInt(-1),3);
        QApplication::processEvents();QTest::qWait(80);call(window,"capture.screenshot",{{"path",output+"/typed-ellipse.png"}});
        call(window,"sketch.cancel");

        // Source-like center, rough first axis and oblique contour pick.
        call(window,"sketch.begin",{{"plane","XY"}});call(window,"view.zoom",{{"factor",0.4}});ellipse->trigger();
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(0,0));
        motion(pixel(130,8));QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(130,8));
        motion(pixel(78,45));QApplication::processEvents();QTest::qWait(70);
        call(window,"capture.screenshot",{{"path",output+"/ellipse-preview.png"}});
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(78,45));
        QCOMPARE(active(window).value("solverDiagnostics").toObject().value("dof").toInt(-1),3);
        auto parameters=draft(window).value("parameters").toObject();
        const auto authored=parameters.value("entities").toArray().last().toObject();
        QVERIFY(authored.value("type")=="ellipse");QCOMPARE(parameters.value("dimensions").toArray().size(),0);
        QTest::keyClick(view,Qt::Key_Escape);QCOMPARE(active(window).value("tool").toString(),QString("select"));
        auto* horizontal=window.findChild<QAction*>("sketch.action.constraint.horizontal");QVERIFY(horizontal);horizontal->trigger();
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(65,4));
        QTRY_COMPARE(active(window).value("solverDiagnostics").toObject().value("dof").toInt(-1),2);
        QTest::keyClick(view,Qt::Key_Escape);
        for(int index=0;index<2;++index){
            QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QTest::keyClick(view,Qt::Key_D);
            QCOMPARE(active(window).value("tool").toString(),QString("dimension"));
            const auto target=index==0 ? pixel(70,0) : pixel(0,25);
            QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,target);
            QTRY_COMPARE(active(window).value("selection").toObject().value("entityIds").toArray().size(),1);
            const auto selected=active(window).value("selection").toObject().value("entityIds").toArray();
            QCOMPARE(selected.at(0),authored.value(index==0 ? "firstAxisId" : "secondAxisId"));
            const auto label=index==0 ? pixel(55,80) : pixel(60,30);
            motion(label);QApplication::processEvents();
            QTRY_COMPARE(active(window).value("pendingDimension").toObject().value("phase").toString(),QString("placing"));
            QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,label);
            auto* editor=window.findChild<QLineEdit*>("sketch.inlineValue");QVERIFY(editor && editor->isVisible());
            QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(editor));
            editor->selectAll();QTest::keyClicks(editor,index==0 ? "275" : "100");QTest::keyClick(editor,Qt::Key_Return);
            QTRY_VERIFY(!active(window).contains("pendingDimension"));
            QCOMPARE(active(window).value("solverDiagnostics").toObject().value("dof").toInt(-1),1-index);
            QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QTest::keyClick(view,Qt::Key_Escape);
        }
        parameters=draft(window).value("parameters").toObject();QCOMPARE(parameters.value("dimensions").toArray().size(),2);
        QCOMPARE(parameters.value("entities").toArray().last().toObject(),authored);
        QTRY_COMPARE(active(window).value("profileSummaries").toArray().size(),1);
        QApplication::processEvents();QTest::qWait(80);call(window,"capture.screenshot",{{"path",output+"/defined-ellipse.png"}});
        call(window,"sketch.cancel");window.close();
    }
};
QTEST_MAIN(CenterPointEllipseControlsTest)
#include "center_point_ellipse_controls.moc"
