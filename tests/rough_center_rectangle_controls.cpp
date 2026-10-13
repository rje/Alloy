// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QAction>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QLineEdit>
#include <QMap>
#include <QMouseEvent>
#include <QTemporaryDir>
#include <QTest>
#include <cmath>

class RoughCenterRectangleControlsTest final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& w,const QString& command,const QJsonObject& args={}) {
        const auto response=w.execute(command,args);
        if(!response["ok"].toBool())qFatal("%s",QJsonDocument(response).toJson().constData());
        return response["result"].toObject();
    }
    QJsonObject active(AlloyWindow& w) {return w.state()["activeSession"].toObject();}
    QJsonObject draft(AlloyWindow& w) {return active(w)["draftSketch"].toObject();}
private slots:
    void pointerRoughThenNativeDimensions() {
        QTemporaryDir scratch;QVERIFY(scratch.isValid());
        auto output=qEnvironmentVariable("ALLOY_ROUGH_CENTER_RECTANGLE_EVIDENCE");
        if(output.isEmpty())output=scratch.path();QDir().mkpath(output);
        AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1300,900);w.show();w.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&w));QApplication::setActiveWindow(&w);
        ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())
            if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;
        QVERIFY(view);view->setFocus();
        call(w,"sketch.begin",{{"plane","XY"}});
        call(w,"view.zoom",{{"factor",0.65}});
        auto* tool=w.findChild<QAction*>("sketch.tool.centerRectangle");QVERIFY(tool);tool->trigger();
        QCOMPARE(active(w)["tool"].toString(),QString("centerRectangle"));
        const auto sketchId=draft(w).value("id");
        auto pixel=[&](double u,double v){const auto p=call(w,"view.projectSketchPoint",{{"sketchId",sketchId},{"uMm",u},{"vMm",v}});
            return QPoint(qRound(p["x"].toDouble()),qRound(p["y"].toDouble()));};
        auto motion=[&](const QPoint& p){QMouseEvent event(QEvent::MouseMove,QPointF(p),QPointF(view->mapToGlobal(p)),
            Qt::NoButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(view,&event);};
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(0,0));
        QVERIFY(active(w).contains("firstPoint"));
        motion(pixel(34,55));QVERIFY(active(w).contains("pointerPoint"));
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(34,55));
        auto parameters=draft(w)["parameters"].toObject();
        QCOMPARE(parameters["entities"].toArray().size(),6);
        QCOMPARE(parameters["dimensions"].toArray().size(),0);
        QCOMPARE(active(w)["solverDiagnostics"].toObject()["dof"].toInt(-1),2);
        QCOMPARE(parameters["entities"].toArray()[4].toObject()["construction"].toBool(),true);
        QCOMPARE(parameters["entities"].toArray()[5].toObject()["construction"].toBool(),true);
        QApplication::processEvents();QTest::qWait(40);
        call(w,"capture.screenshot",{{"path",output+"/rough-center-rectangle.png"}});
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
        QTest::keyClick(view,Qt::Key_Escape);
        QCOMPARE(active(w)["tool"].toString(),QString("select"));
        const auto entities=parameters["entities"].toArray();
        QMap<QString,QPointF> points;for(const auto& value:parameters["points"].toArray()) {
            const auto p=value.toObject();points[p["id"].toString()]={p["uMm"].toDouble(),p["vMm"].toDouble()};
        }
        for(int axis=0;axis<2;++axis) {
            QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
            QTest::keyClick(view,Qt::Key_D);
            QCOMPARE(active(w)["tool"].toString(),QString("dimension"));
            const auto edge=entities[axis==0 ? 2 : 1].toObject();
            const auto middle=(points[edge["startPointId"].toString()]+points[edge["endPointId"].toString()])/2;
            QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(middle.x(),middle.y()));
            QCOMPARE(active(w)["selection"].toObject()["entityIds"].toArray().size(),1);
            const auto label=axis==0 ? pixel(middle.x(),middle.y()+8):pixel(middle.x()+8,middle.y());
            motion(label);
            QTRY_COMPARE(active(w)["pendingDimension"].toObject()["phase"].toString(),QString("placing"));
            QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,label);
            auto* editor=w.findChild<QLineEdit*>("sketch.inlineValue");QVERIFY(editor && editor->isVisible());
            QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(editor));
            editor->selectAll();QTest::keyClicks(editor,axis==0 ? "70" : "114");QTest::keyClick(editor,Qt::Key_Return);
            QTRY_VERIFY(!active(w).contains("pendingDimension"));
            QCOMPARE(active(w)["solverDiagnostics"].toObject()["dof"].toInt(-1),1-axis);
            QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
            QTest::keyClick(view,Qt::Key_Escape);
            QCOMPARE(active(w)["tool"].toString(),QString("select"));
        }
        parameters=draft(w)["parameters"].toObject();
        const auto dimensions=parameters["dimensions"].toArray();QCOMPARE(dimensions.size(),2);
        for(const auto& value:dimensions)QVERIFY(value.toObject()["driving"].toBool());
        QApplication::processEvents();QTest::qWait(40);
        call(w,"capture.screenshot",{{"path",output+"/defined-center-rectangle.png"}});
        call(w,"history.undo");QCOMPARE(active(w)["solverDiagnostics"].toObject()["dof"].toInt(-1),1);
        call(w,"history.redo");QCOMPARE(active(w)["solverDiagnostics"].toObject()["dof"].toInt(-1),0);
        const auto authoredSketch=draft(w);call(w,"sketch.finish");
        const auto authored=call(w,"document.inspect").value("authored");
        const auto path=output+"/rough-center-rectangle.alloy";call(w,"document.save",{{"path",path}});
        call(w,"document.open",{{"path",path}});
        QCOMPARE(call(w,"document.inspect").value("authored"),authored);
        const auto reopened=call(w,"document.inspect")["evaluation"].toObject()["sketches"].toArray().first().toObject();
        QCOMPARE(reopened["sketchId"],authoredSketch["id"]);
        QCOMPARE(reopened["profiles"].toArray().size(),1);
        QFile evidence(output+"/rough-center-rectangle-state.json");QVERIFY(evidence.open(QIODevice::WriteOnly));
        evidence.write(QJsonDocument(w.state()).toJson(QJsonDocument::Indented));evidence.close();
        w.close();
    }
};
QTEST_MAIN(RoughCenterRectangleControlsTest)
#include "rough_center_rectangle_controls.moc"
