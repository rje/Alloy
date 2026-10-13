// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QTest>
#include <QPushButton>
#include <QScrollArea>
#include <QLineEdit>
#include <QMouseEvent>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <cmath>
class CenterlineControlsTest final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& w,const QString& name,const QJsonObject& args={}){const auto r=w.execute(name,args);if(!r["ok"].toBool())qFatal("%s",QJsonDocument(r).toJson().constData());return r["result"].toObject();}
    QJsonObject session(AlloyWindow& w){return w.state()["activeSession"].toObject();}
    QJsonObject draft(AlloyWindow& w){return session(w)["draftSketch"].toObject();}
private slots:
    void centerlineDiameterWorkflow(){
        AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1400,1000);w.show();w.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&w));ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(child))view=v;QVERIFY(view);
        call(w,"sketch.begin",{{"plane","XZ"}});call(w,"sketch.line",{{"x1",0},{"y1",0},{"x2",0},{"y2",150},{"units","mm"}});call(w,"sketch.rectangle",{{"x1",50},{"y1",0},{"x2",82.5},{"y2",100},{"units","mm"}});
        const auto axis=draft(w)["parameters"].toObject()["entities"].toArray()[0].toObject();
        for(const auto key:{"startPointId","endPointId"})call(w,"sketch.constraint",{{"type","fixed"},{"pointIds",QJsonArray{axis[key]}}});
        call(w,"sketch.tool",{{"tool","select"}});call(w,"sketch.selectionSet",{{"entityIds",QJsonArray{axis["id"]}}});
        const auto evidence=qEnvironmentVariable("ALLOY_CENTERLINE_EVIDENCE");if(!evidence.isEmpty())QDir().mkpath(evidence);
        const auto normal=draft(w);auto* convert=w.findChild<QPushButton*>("sketch.centerline.toggle");QVERIFY(convert);for(auto* parent=convert->parentWidget();parent;parent=parent->parentWidget())if(auto* scroll=qobject_cast<QScrollArea*>(parent))scroll->ensureWidgetVisible(convert);QTest::qWait(50);QVERIFY(convert->isVisible());QTest::mouseClick(convert,Qt::LeftButton);QVERIFY(draft(w)["parameters"].toObject()["entities"].toArray()[0].toObject()["centerline"].toBool());
        if(!evidence.isEmpty())call(w,"capture.screenshot",{{"path",evidence+"/centerline-converted-palette.png"}});
        call(w,"history.undo");QCOMPARE(draft(w),normal);call(w,"history.redo");QVERIFY(draft(w)["parameters"].toObject()["entities"].toArray()[0].toObject()["centerline"].toBool());call(w,"sketch.lookAt");
        auto pixel=[&](double u,double v){const auto p=call(w,"view.projectSketchPoint",{{"sketchId",draft(w)["id"]},{"uMm",u},{"vMm",v}});return QPoint(qRound(p["x"].toDouble()),qRound(p["y"].toDouble()));};
        call(w,"sketch.selectionSet",{{"entityIds",QJsonArray{}}});view->setFocus();QTest::keyClick(view,Qt::Key_D);QCOMPARE(session(w)["tool"].toString(),QString("dimension"));
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(0,65));QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(50,60));
        QCOMPARE(session(w)["selection"].toObject()["entityIds"].toArray().size(),2);
        const auto place=pixel(-5,115);QMouseEvent move(QEvent::MouseMove,QPointF(place),QPointF(view->mapToGlobal(place)),Qt::NoButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(view,&move);
        QTRY_COMPARE(session(w)["pendingDimension"].toObject()["kind"].toString(),QString("centerlineDiameter"));QCOMPARE(session(w)["pendingDimension"].toObject()["measuredValueMm"].toDouble(),100.0);
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,place);auto* input=w.findChild<QLineEdit*>("sketch.inlineValue");QVERIFY(input && input->isVisible());input->selectAll();QTest::keyClicks(input,"100 mm");QTest::keyClick(input,Qt::Key_Return);QTRY_VERIFY(!session(w).contains("pendingDimension"));
        auto dims=draft(w)["parameters"].toObject()["dimensions"].toArray();QCOMPARE(dims.size(),1);const auto dim=dims[0].toObject();
        auto layout=view->diagnostics()["sketchAnnotations"].toArray()[0].toObject();QCOMPARE(layout["kind"].toString(),QString("centerlineDiameter"));QCOMPARE(layout["label"].toObject()["text"].toString(),QString::fromUtf8("Ø100"));const auto line=layout["line"].toArray();QVERIFY(std::abs(std::hypot(line[1].toObject()["uMm"].toDouble()-line[0].toObject()["uMm"].toDouble(),line[1].toObject()["vMm"].toDouble()-line[0].toObject()["vMm"].toDouble())-100)<1e-6);
        if(!evidence.isEmpty()){call(w,"capture.screenshot",{{"path",evidence+"/centerline-diameter-100.png"}});}
        call(w,"sketch.dimensionEdit",{{"dimensionId",dim["id"]}});input->selectAll();QTest::keyClicks(input,"110");QTest::keyClick(input,Qt::Key_Return);QTRY_VERIFY(!session(w).contains("pendingDimension"));QCOMPARE(draft(w)["parameters"].toObject()["dimensions"].toArray()[0].toObject()["valueMm"].toDouble(),110.0);
        // Escape cancels numeric editing without changing authored intent.
        const auto beforeCancel=draft(w);call(w,"sketch.dimensionEdit",{{"dimensionId",dim["id"]}});input->selectAll();QTest::keyClicks(input,"120");QTest::keyClick(input,Qt::Key_Escape);QCOMPARE(draft(w),beforeCancel);
        call(w,"sketch.tool",{{"tool","select"}});call(w,"history.undo");QCOMPARE(draft(w)["parameters"].toObject()["dimensions"].toArray()[0].toObject()["valueMm"].toDouble(),100.0);call(w,"history.redo");QCOMPARE(draft(w),beforeCancel);
        auto rows=draft(w)["parameters"].toObject()["entities"].toArray();call(w,"sketch.dimension",{{"kind","centerlineDiameter"},{"entityIds",QJsonArray{axis["id"],rows[2].toObject()["id"]}},{"driving",false},{"annotation",QJsonObject{{"uMm",0},{"vMm",135}}}});dims=draft(w)["parameters"].toObject()["dimensions"].toArray();call(w,"sketch.dimensionEdit",{{"dimensionId",dims[1].toObject()["id"]}});QVERIFY(input->isReadOnly());call(w,"sketch.dimensionCancel");
        if(!evidence.isEmpty()){call(w,"capture.screenshot",{{"path",evidence+"/centerline-diameter-edited-driven.png"}});QFile f(evidence+"/centerline-state.json");QVERIFY(f.open(QIODevice::WriteOnly));f.write(QJsonDocument(w.state()).toJson());}
        const auto finalDraft=draft(w);call(w,"sketch.finish");const auto authored=call(w,"document.inspect")["authored"].toObject();call(w,"history.undo");call(w,"history.redo");QCOMPARE(call(w,"document.inspect")["authored"].toObject(),authored);
        QTemporaryDir dir;call(w,"document.save",{{"path",dir.path()+"/centerline.alloy"}});call(w,"document.open",{{"path",dir.path()+"/centerline.alloy"}});QCOMPARE(call(w,"document.inspect")["authored"].toObject(),authored);QVERIFY(!finalDraft.isEmpty());
        w.close();
    }
};
QTEST_MAIN(CenterlineControlsTest)
#include "centerline_controls.moc"
