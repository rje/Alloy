// SPDX-License-Identifier: MIT
#include "AlloyTheme.h"
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QCheckBox>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QLineEdit>
#include <QTest>
#include <QTemporaryDir>

class RoundedLoopOffsetControls final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& name,const QJsonObject& args={}){
        auto response=window.execute(name,args);
        if(response.value("completion")=="pending") {
            const auto operation=response.value("result").toObject();
            for(int i=0;i<2000 && response.value("completion")=="pending";++i){QTest::qWait(10);response=window.execute("command.status",{{"session",operation.value("id")},{"generation",operation.value("previewGeneration")}});}
        }
        if(!response["ok"].toBool())qFatal("%s",QJsonDocument(response).toJson().constData());
        return response["result"].toObject();
    }
    QJsonObject session(AlloyWindow& window){return window.state()["activeSession"].toObject();}
private slots:
    void generatedProfileDelete(){
        const auto output=qEnvironmentVariable("ALLOY_LOOP_DELETE_EVIDENCE");
        if(output.isEmpty())QSKIP("Set ALLOY_LOOP_DELETE_EVIDENCE to the API fixture directory");
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1500,950);window.show();window.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&window));QApplication::setActiveWindow(&window);
        ReferenceView* view=nullptr;for(auto* child:window.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(child))view=v;QVERIFY(view);
        call(window,"document.open",{{"path",output+"/native-before.alloy"}});
        const auto base=call(window,"document.inspect");const auto features=base.value("authored").toObject().value("features").toArray();const auto target=features[features.size()-2].toObject();
        call(window,"sketch.edit",{{"sketchId",target.value("id")}});call(window,"sketch.lookAt");call(window,"view.fit");call(window,"sketch.tool",{{"tool","select"}});
        const auto original=session(window).value("draftSketch").toObject();
        QTest::qWait(250); // Settle queued sketch-palette layout before the input sequence.
        // Initial harness activation only. The actual click must establish focus and selection.
        view->setFocus(Qt::OtherFocusReason);QVERIFY(view->hasFocus());
        const auto frame=session(window).value("resolvedFrame").toObject();QPoint picked(-1,-1);QString profile;
        for(double u=-65;u<=65 && picked.x()<0;u+=.75)for(double v=-65;v<=65;v+=.5){
            const auto pixel=view->projectSketchPoint(u,v,"face",frame);const QPoint point(qRound(pixel.value("x").toDouble()),qRound(pixel.value("y").toDouble()));
            if(!view->rect().adjusted(10,10,-10,-10).contains(point))continue;
            const auto hit=view->pickSketch(original,point.x(),point.y(),{},frame);
            if(hit.value("kind")=="profile" && QApplication::widgetAt(view->mapToGlobal(point))==view){picked=point;profile=hit.value("profileId").toString();break;}
        }
        QVERIFY(picked.x()>=0);call(window,"capture.screenshot",{{"path",output+"/native-before.png"}});
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,picked);
        QTRY_COMPARE(session(window).value("selection").toObject().value("profileIds").toArray(),QJsonArray{profile});QVERIFY(view->hasFocus());
        call(window,"capture.screenshot",{{"path",output+"/native-selected.png"}});
        const auto historyIndex=session(window).value("draftHistory").toObject().value("index").toInt();
        QTest::keyClick(QApplication::focusWidget(),Qt::Key_Delete);
        QTRY_VERIFY(!session(window).value("draftSketch").toObject().value("parameters").toObject().contains("loopOffsets"));
        QCOMPARE(session(window).value("draftHistory").toObject().value("index").toInt(),historyIndex+1);
        const auto deleted=session(window).value("draftSketch").toObject();QCOMPARE(deleted.value("parameters").toObject().value("support"),original.value("parameters").toObject().value("support"));
        call(window,"capture.screenshot",{{"path",output+"/native-deleted.png"}});
        call(window,"history.undo");QCOMPARE(session(window).value("draftSketch").toObject(),original);call(window,"capture.screenshot",{{"path",output+"/native-undone.png"}});
        call(window,"history.redo");QCOMPARE(session(window).value("draftSketch").toObject(),deleted);call(window,"sketch.finish");
        const auto final=call(window,"document.inspect").value("authored").toObject();const auto path=output+"/native-deleted.alloy";call(window,"document.save",{{"path",path}});
        call(window,"document.open",{{"path",path},{"discard",true}});QCOMPARE(call(window,"document.inspect").value("authored").toObject(),final);
        call(window,"capture.screenshot",{{"path",output+"/native-reopened.png"}});window.close();
    }
    void actualBoundaryAndNativeSignedEntry(){
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1500,950);window.show();window.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&window));QApplication::setActiveWindow(&window);
        ReferenceView* view=nullptr;for(auto* child:window.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;
        QVERIFY(view);
        const auto fixture=QFileInfo(__FILE__).dir().filePath("fixtures/rounded-rectangle-source.alloy");
        call(window,"document.open",{{"path",fixture}});
        const auto source=call(window,"document.inspect");const auto sketch=source["evaluation"].toObject()["sketches"].toArray().first().toObject();
        call(window,"solid.extrude",{{"sketchId",sketch["sketchId"]},{"profileId",sketch["profiles"].toArray().first().toObject()["profileId"]},{"distance",2},{"units","mm"}});
        const auto body=call(window,"document.inspect")["evaluation"].toObject()["bodies"].toArray().first().toObject();
        QJsonObject face;for(const auto& value:body["supportFaces"].toArray())if(value.toObject()["role"]=="extrude.endCap")face=value.toObject();
        QVERIFY(!face.isEmpty());const QJsonObject support{{"bodyId",face["bodyId"]},{"producerFeatureId",face["producerFeatureId"]},{"faceId",face["faceId"]}};
        call(window,"sketch.begin",{{"support",support},{"name","Rounded inner loop"},{"units","mm"}});
        call(window,"sketch.tool",{{"tool","offset"}});
        auto* chain=window.findChild<QCheckBox*>("sketch.offset.chain");auto* field=window.findChild<QLineEdit*>("sketch.offset.distance");
        QVERIFY(chain && chain->isVisible() && chain->isChecked());QVERIFY(field && field->isVisible());
        const auto before=session(window)["draftSketch"].toObject();
        QJsonObject edge;for(const auto& value:body["solidEdges"].toArray())if(value.toObject()["provenance"].toObject()["role"]=="endRim"){
            edge=value.toObject();break;
        }
        QVERIFY(!edge.isEmpty());const auto pixel=call(window,"view.projectEdge",{{"bodyId",body["bodyId"]},{"edgeId",edge["edgeId"]}});
        const QPoint point(qRound(pixel["x"].toDouble()),qRound(pixel["y"].toDouble()));QVERIFY(view->rect().contains(point));
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,point);
        QTRY_VERIFY(session(window)["pendingLoopOffset"].toObject()["valid"].toBool());
        QCOMPARE(session(window)["draftSketch"].toObject(),before);
        QTest::mouseClick(field,Qt::LeftButton);
        QTest::keyClick(QApplication::focusWidget(),Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(QApplication::focusWidget(),"-1.5");
        QTRY_VERIFY(session(window)["pendingLoopOffset"].toObject()["valid"].toBool());
        QCOMPARE(field->text(),QString("-1.5"));
        QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(field));
        const auto output=qEnvironmentVariable("ALLOY_ROUNDED_LOOP_EVIDENCE");
        if(!output.isEmpty()){QDir().mkpath(output);call(window,"capture.screenshot",{{"path",output+"/loop-preview.png"}});}
        QTest::keyClick(QApplication::focusWidget(),Qt::Key_Escape);
        QTRY_VERIFY(!session(window).contains("pendingLoopOffset"));QCOMPARE(session(window)["draftSketch"].toObject(),before);
        call(window,"sketch.tool",{{"tool","offset"}});
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,point);
        QTRY_VERIFY(session(window)["pendingLoopOffset"].toObject()["valid"].toBool());
        QTest::mouseClick(field,Qt::LeftButton);QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(field));
        QTest::keyClick(QApplication::focusWidget(),Qt::Key_Return);
        QTRY_VERIFY(!session(window).contains("pendingLoopOffset"));
        QCOMPARE(session(window)["draftSketch"].toObject()["parameters"].toObject()["loopOffsets"].toArray().size(),1);
        if(!output.isEmpty())call(window,"capture.screenshot",{{"path",output+"/loop-accepted.png"}});
        call(window,"sketch.cancel");QTemporaryDir clean;QVERIFY(clean.isValid());
        call(window,"document.save",{{"path",clean.filePath("native-offset.alloy")}});window.close();
    }
};
QTEST_MAIN(RoundedLoopOffsetControls)
#include "rounded_loop_offset_controls.moc"
