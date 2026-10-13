// SPDX-License-Identifier: MIT
#include "AlloyTheme.h"
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QDir>
#include <QJsonDocument>
#include <QLineEdit>
#include <QMouseEvent>
#include <QTest>
class ParallelLineControlsTest final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& w,const QString& command,const QJsonObject& input={}){
        const auto response=w.execute(command,input);if(!response["ok"].toBool())qFatal("%s",QJsonDocument(response).toJson().constData());return response["result"].toObject();
    }
    QJsonObject session(AlloyWindow& w){return w.state()["activeSession"].toObject();}
    QJsonObject draft(AlloyWindow& w){return session(w)["draftSketch"].toObject();}
private slots:
    void twoNativeDistances(){
        AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1400,1000);w.show();w.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&w));
        ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(child))view=v;QVERIFY(view);
        qInfo("native step: begin");call(w,"sketch.begin",{{"plane","XY"}});
        for(const auto y:{0.,11.,39.})call(w,"sketch.line",{{"x1",0},{"y1",y},{"x2",50},{"y2",y},{"units","mm"}});
        qInfo("native step: lines");const auto entities=draft(w)["parameters"].toObject()["entities"].toArray();QCOMPARE(entities.size(),3);
        for(const auto& line:entities)call(w,"sketch.constraint",{{"type","horizontal"},{"entityIds",QJsonArray{line.toObject()["id"]}}});
        const auto base=entities[0].toObject();call(w,"sketch.constraint",{{"type","fixed"},{"pointIds",QJsonArray{base["startPointId"]}}});
        qInfo("native step: constrained");call(w,"sketch.lookAt");auto pixel=[&](double u,double v){const auto p=call(w,"view.projectSketchPoint",{{"sketchId",draft(w)["id"]},{"uMm",u},{"vMm",v}});return QPoint(qRound(p["x"].toDouble()),qRound(p["y"].toDouble()));};
        const auto evidence=qEnvironmentVariable("ALLOY_PARALLEL_EVIDENCE");if(!evidence.isEmpty())QDir().mkpath(evidence);
        qInfo("native step: D");view->setFocus();QTest::keyClick(view,Qt::Key_D);QCOMPARE(session(w)["tool"].toString(),QString("dimension"));
        auto enter=[&](double firstY,double secondY,double annotationY,const char* text){
            qInfo("native step: first pick");QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(20,firstY));
            qInfo("native step: second pick");
            QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(20,secondY));
            QCOMPARE(session(w)["selection"].toObject()["entityIds"].toArray().size(),2);
            qInfo("native step: place");const auto place=pixel(30,annotationY);QMouseEvent move(QEvent::MouseMove,QPointF(place),QPointF(view->mapToGlobal(place)),Qt::NoButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(view,&move);
            QTRY_COMPARE(session(w)["pendingDimension"].toObject()["kind"].toString(),QString("parallelLineDistance"));
            QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,place);
            auto* input=w.findChild<QLineEdit*>("sketch.inlineValue");QVERIFY(input && input->isVisible() && input->hasFocus());
            qInfo("native step: type");input->selectAll();QTest::keyClicks(input,text);QTest::keyClick(input,Qt::Key_Return);QTRY_VERIFY(!session(w).contains("pendingDimension"));
        };
        enter(0,11,5,"11");enter(11,39,25,"28");
        const auto dimensions=draft(w)["parameters"].toObject()["dimensions"].toArray();QCOMPARE(dimensions.size(),2);
        QCOMPARE(dimensions[0].toObject()["valueMm"].toDouble(),11.);QCOMPARE(dimensions[1].toObject()["valueMm"].toDouble(),28.);
        const auto layouts=view->diagnostics()["sketchAnnotations"].toArray();QCOMPARE(layouts.size(),2);
        for(const auto& layout:layouts){const auto item=layout.toObject();QCOMPARE(item["kind"].toString(),QString("parallelLineDistance"));QCOMPARE(item["witnesses"].toArray().size(),2);}
        if(!evidence.isEmpty())call(w,"capture.screenshot",{{"path",evidence+"/parallel-line-d-11-28.png"}});
        call(w,"sketch.cancel");w.close();
    }
};
QTEST_MAIN(ParallelLineControlsTest)
#include "parallel_line_controls.moc"
