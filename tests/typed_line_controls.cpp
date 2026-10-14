// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QTemporaryDir>
#include <QTest>
#include <cmath>

// Delivered Qt widget events; physical OS input is a separate qualification.
class TypedLineControlsTest final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& w,const QString& command,const QJsonObject& args={}) {
        const auto response=w.execute(command,args);
        if(!response["ok"].toBool())qFatal("%s",QJsonDocument(response).toJson().constData());
        return response["result"].toObject();
    }
    QJsonObject active(AlloyWindow& w) {return w.state()["activeSession"].toObject();}
    QJsonObject draft(AlloyWindow& w) {return active(w)["draftSketch"].toObject();}
private slots:
    void angledLineRetainsHelperAndTabFocus() {
        AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1250,850);w.show();w.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&w));
        ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;
        QVERIFY(view);call(w,"sketch.begin",{{"plane","XY"},{"units","mm"}});call(w,"sketch.lookAt");
        const auto sketchId=draft(w)["id"];
        auto pixel=[&](double u,double v){const auto p=call(w,"view.projectSketchPoint",{{"sketchId",sketchId},{"uMm",u},{"vMm",v}});
            return QPoint(qRound(p["x"].toDouble()),qRound(p["y"].toDouble()));};
        // Establish the private test window once; subsequent focus must follow
        // the user's key sequence without harness repair between steps.
        QApplication::setActiveWindow(&w);view->setFocus();QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
        QTest::keyClick(QApplication::focusWidget(),Qt::Key_L);QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(0,0));
        const auto baseline=draft(w);QTest::keyClick(view,Qt::Key_4);
        auto* length=w.findChild<QLineEdit*>("sketch.inlineValue");auto* angle=w.findChild<QLineEdit*>("sketch.lineAngleValue");
        QVERIFY(length);QVERIFY(angle);QVERIFY(length->isVisible());
        QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(length));
        QTest::keyClick(QApplication::focusWidget(),Qt::Key_Tab);
        QCOMPARE(active(w)["pendingLineLength"].toObject()["phase"].toString(),QString("locked"));
        QVERIFY(angle->isVisible());QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(angle));QCOMPARE(draft(w),baseline);
        QTest::keyClicks(QApplication::focusWidget(),"55");QCOMPARE(active(w)["pendingLineLength"].toObject()["angleDeg"].toDouble(),55.0);
        QTest::keyClick(QApplication::focusWidget(),Qt::Key_Tab);QVERIFY(active(w)["pendingLineLength"].toObject()["angleLocked"].toBool());QCOMPARE(draft(w),baseline);
        QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
        const QPoint upperRight=pixel(3,3);QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,upperRight);
        const auto p=draft(w)["parameters"].toObject();QCOMPARE(p["entities"].toArray().size(),2);
        const auto helper=p["entities"].toArray().first().toObject(),edge=p["entities"].toArray().last().toObject();
        QVERIFY(helper["construction"].toBool());QCOMPARE(helper["startPointId"],edge["startPointId"]);
        const auto dimensions=p["dimensions"].toArray();QCOMPARE(dimensions.size(),2);
        QCOMPARE(dimensions[0].toObject()["kind"].toString(),QString("linear"));QCOMPARE(dimensions[0].toObject()["valueMm"].toDouble(),4.0);
        QCOMPARE(dimensions[1].toObject()["kind"].toString(),QString("angle"));QCOMPARE(dimensions[1].toObject()["valueDeg"].toDouble(),55.0);
        QJsonObject start,end;for(const auto& row:p["points"].toArray()){
            if(row.toObject()["id"]==edge["startPointId"])start=row.toObject();
            if(row.toObject()["id"]==edge["endPointId"])end=row.toObject();}
        QVERIFY(std::abs(std::hypot(end["uMm"].toDouble()-start["uMm"].toDouble(),end["vMm"].toDouble()-start["vMm"].toDouble())-4)<1e-6);
        QCOMPARE(active(w)["firstPoint"].toObject()["target"].toObject()["pointId"],edge["endPointId"]);
        call(w,"sketch.tool",{{"tool","select"}});call(w,"history.undo");
        QCOMPARE(draft(w),baseline);call(w,"history.redo");QCOMPARE(draft(w)["parameters"],p);
        call(w,"sketch.cancel");w.close();
    }
    void cancelWholeSketchClearsCue() {
        AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1100,800);w.show();w.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&w));
        ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;
        QVERIFY(view);const auto baseline=call(w,"document.inspect").value("authored");
        const auto begun=call(w,"sketch.begin",{{"plane","XY"},{"units","mm"}});
        const auto sketchId=begun["draftSketch"]["id"];
        auto pixel=[&](double u,double v){const auto p=call(w,"view.projectSketchPoint",{{"sketchId",sketchId},{"uMm",u},{"vMm",v}});
            return QJsonObject{{"x",p["x"]},{"y",p["y"]}};};
        call(w,"sketch.tool",{{"tool","line"}});call(w,"sketch.placePoint",pixel(0,0));
        call(w,"sketch.lineLengthPreview",{{"text","12"}});call(w,"sketch.lineLengthLock");
        call(w,"sketch.pointerMove",pixel(0,10));
        auto* cue=w.findChild<QLabel*>("sketch.lineLockCue");QVERIFY(cue && cue->isVisible());
        QVERIFY(cue->text().contains("12 mm"));
        call(w,"sketch.cancel");QVERIFY(!cue->isVisible());QVERIFY(active(w).isEmpty());
        QCOMPARE(call(w,"document.inspect").value("authored"),baseline);
        w.close();
    }
    void chainedTypedLengths() {
        QTemporaryDir temporary;auto output=qEnvironmentVariable("ALLOY_TYPED_LINE_EVIDENCE");
        if(output.isEmpty())output=temporary.path();QDir().mkpath(output);
        AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1400,900);w.show();w.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&w));
        ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;
        QVERIFY(view);call(w,"sketch.begin",{{"plane","XY"},{"units","mm"}});call(w,"sketch.lookAt");
        const auto sketchId=draft(w)["id"];
        auto pixel=[&](double u,double v){const auto p=call(w,"view.projectSketchPoint",{{"sketchId",sketchId},{"uMm",u},{"vMm",v}});
            return QPoint(qRound(p["x"].toDouble()),qRound(p["y"].toDouble()));};
        auto move=[&](QPoint p){QMouseEvent event(QEvent::MouseMove,QPointF(p),QPointF(view->mapToGlobal(p)),Qt::NoButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(view,&event);};
        view->setFocus();QTest::keyClick(view,Qt::Key_L);QCOMPARE(active(w)["tool"].toString(),QString("line"));
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(0,0));QVERIFY(active(w).contains("firstPoint"));
        auto* input=w.findChild<QLineEdit*>("sketch.inlineValue");
        QTest::keyClick(view,Qt::Key_0);input=w.findChild<QLineEdit*>("sketch.inlineValue");QVERIFY(input && input->isVisible());
        const auto empty=draft(w);QTest::keyClick(input,Qt::Key_Tab);
        QCOMPARE(active(w)["pendingLineLength"].toObject()["phase"].toString(),QString("editing"));
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(0,20));QCOMPARE(draft(w),empty);
        input->selectAll();QTest::keyClicks(input,"40");QTest::keyClick(input,Qt::Key_Tab);
        QCOMPARE(active(w)["pendingLineLength"].toObject()["phase"].toString(),QString("locked"));
        const double expected[7][4]={{0,40,0,20},{14,40,10,40},{14,-20,14,0},{3,-20,8,-20},{3,24,3,0},{11,24,8,24},{11,0,11,10}};
        const char* values[]={"40","14","60","11","44","8","24"};
        for(int i=0;i<7;++i) {
            if(i>0){view->setFocus();QTest::keyClick(view,Qt::Key_1);QVERIFY(input->isVisible());input->selectAll();QTest::keyClicks(input,values[i]);QTest::keyClick(input,Qt::Key_Tab);
                QCOMPARE(active(w)["pendingLineLength"].toObject()["phase"].toString(),QString("locked"));}
            const auto pointer=pixel(expected[i][2],expected[i][3]);move(pointer);
            const auto preview=active(w)["pointerPoint"].toObject();QVERIFY(!preview["direction"].toString().isEmpty());
            QVERIFY(view->diagnostics()["cursorPreviewVisible"].toBool());
            QApplication::processEvents();QTest::qWait(50);
            if(i==0 || i==4){call(w,"capture.screenshot",{{"path",output+QString("/locked-%1.png").arg(i+1)}});}
            QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pointer);
            QVERIFY(!active(w).contains("pendingLineLength"));
            const auto p=draft(w)["parameters"].toObject(),line=p["entities"].toArray().last().toObject();
            QJsonObject end;for(const auto& value:p["points"].toArray())if(value.toObject()["id"]==line["endPointId"]){end=value.toObject();break;}
            QVERIFY(std::abs(end["uMm"].toDouble()-expected[i][0])<1e-6);
            QVERIFY(std::abs(end["vMm"].toDouble()-expected[i][1])<1e-6);
            QCOMPARE(p["dimensions"].toArray().size(),i+1);QCOMPARE(p["dimensions"].toArray().last().toObject()["valueMm"].toDouble(),QString(values[i]).toDouble());
        }
        const auto authoredDraft=draft(w);QCOMPARE(authoredDraft["parameters"].toObject()["entities"].toArray().size(),7);
        QTest::keyClick(view,Qt::Key_Escape);QCOMPARE(active(w)["tool"].toString(),QString("select"));
        call(w,"history.undo");QCOMPARE(draft(w)["parameters"].toObject()["entities"].toArray().size(),6);
        call(w,"history.redo");QCOMPARE(draft(w),authoredDraft);
        // Existing typed Circle and fillet tool entry still use the same sketch session.
        call(w,"sketch.tool",{{"tool","circle"}});QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(0,0));
        view->setFocus();QTest::keyClick(view,Qt::Key_2);QVERIFY(input->isVisible());
        QTest::keyClick(input,Qt::Key_Return);QCOMPARE(draft(w)["parameters"].toObject()["entities"].toArray().size(),8);
        call(w,"sketch.tool",{{"tool","select"}});call(w,"history.undo");QCOMPARE(draft(w),authoredDraft);
        call(w,"sketch.tool",{{"tool","fillet"}});QCOMPARE(active(w)["tool"].toString(),QString("fillet"));
        call(w,"sketch.tool",{{"tool","select"}});
        call(w,"sketch.finish");const auto authored=call(w,"document.inspect").value("authored");
        const auto path=output+"/typed-line.alloy";call(w,"document.save",{{"path",path}});
        call(w,"document.open",{{"path",path}});QCOMPARE(call(w,"document.inspect").value("authored"),authored);
        QFile file(output+"/state.json");QVERIFY(file.open(QIODevice::WriteOnly));file.write(QJsonDocument(w.state()).toJson(QJsonDocument::Indented));
        w.close();
    }
};
QTEST_MAIN(TypedLineControlsTest)
#include "typed_line_controls.moc"
