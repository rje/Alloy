// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QTest>
#include <QAction>
#include <QMenu>
#include <QToolButton>
#include <QTimer>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QWindow>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QAbstractItemView>
#include <QMouseEvent>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <cmath>
#include <numbers>
class SimpleHoleControlsTest final:public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& w,const QString& command,const QJsonObject& args={}) {
        const auto r=w.execute(command,args);if(!r["ok"].toBool())qFatal("%s",QJsonDocument(r).toJson().constData());return r["result"].toObject();
    }
    QJsonObject session(AlloyWindow& w){return w.state()["activeSession"].toObject();}
private slots:
    void nativePointTargetFieldsPreviewCancelCommitAndEdit() {
        AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1400,920);w.show();w.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&w));
        ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(child))view=v;QVERIFY(view);
        // Shared API fixture setup is separate from the native Hole ingredient.
        call(w,"sketch.begin",{{"plane","XY"}});call(w,"sketch.centerRectangle",{{"centerX",0},{"centerY",0},{"width",75},{"height",50},{"units","mm"}});call(w,"sketch.finish");
        const auto base=call(w,"document.inspect")["evaluation"].toObject()["sketches"].toArray()[0].toObject();
        call(w,"solid.extrude",{{"sketchId",base["sketchId"]},{"profileId",base["profiles"].toArray()[0].toObject()["profileId"]},{"distance",10},{"units","mm"}});
        auto body=call(w,"document.inspect")["evaluation"].toObject()["bodies"].toArray()[0].toObject();const auto target=body["bodyId"].toString();QJsonArray corners;
        for(const auto& e:body["solidEdges"].toArray())if(e.toObject()["provenance"].toObject()["role"]=="sideCorner")corners.append(e.toObject()["edgeId"]);
        call(w,"solid.fillet",{{"targetBodyId",target},{"edgeIds",corners},{"radiusMm",5}});
        body=call(w,"document.inspect")["evaluation"].toObject()["bodies"].toArray()[0].toObject();QJsonObject support;
        for(const auto& f:body["supportFaces"].toArray())if(f.toObject()["role"]=="extrude.endCap")for(auto key:{"bodyId","producerFeatureId","faceId"})support[key]=f.toObject()[key];
        call(w,"sketch.begin",{{"support",support}});call(w,"sketch.addPoint",{{"x",-27.5},{"y",15},{"units","mm"}});
        const auto pointSketch=session(w)["draftSketch"].toObject();const auto sketchId=pointSketch["id"].toString();const auto point=pointSketch["parameters"].toObject()["entities"].toArray()[0].toObject();
        for(const auto& value:session(w)["projectionContext"].toObject()["lines"].toArray()) {
            const auto l=value.toObject();if((std::abs(l["u1Mm"].toDouble()+37.5)<1e-6 && std::abs(l["u2Mm"].toDouble()+37.5)<1e-6)
                || (std::abs(l["v1Mm"].toDouble()-25)<1e-6 && std::abs(l["v2Mm"].toDouble()-25)<1e-6))
                call(w,"sketch.dimension",{{"kind","pointLineDistance"},{"pointIds",QJsonArray{point["pointId"]}},{"entityId",l["id"]},{"value",10},{"units","mm"}});
        }
        call(w,"sketch.finish");call(w,"view.orientation",{{"name","top"}});call(w,"view.home");
        auto pixel=[&]{const auto p=call(w,"view.projectSketchPoint",{{"sketchId",sketchId},{"uMm",-27.5},{"vMm",15}});return QPoint(qRound(p["x"].toDouble()),qRound(p["y"].toDouble()));};
        const auto before=call(w,"document.inspect")["authored"].toObject();
        w.activateWindow();QApplication::setActiveWindow(&w);view->setFocus();if(view->windowHandle())view->windowHandle()->requestActivate();
        QTRY_VERIFY(view->hasFocus()); // Sole initial activation, after API setup.
        qInfo().noquote()<<QJsonDocument(QJsonObject{{"step","initial native activation"},{"focus",w.state()["focus"]}}).toJson(QJsonDocument::Compact);
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel());QCOMPARE(w.state()["selection"].toObject()["kind"].toString(),QString("sketchPoint"));
        auto launch=[&]{
            auto* create=w.findChild<QToolButton*>("solid.menu.create");QVERIFY(create);auto* menu=create->menu();QVERIFY(menu);auto* action=w.findChild<QAction*>("solid.action.solid.holeDialog");QVERIFY(action);
            bool ready=false;QTimer::singleShot(100,&w,[&]{ready=menu->isVisible() && menu->isActiveWindow();if(ready)QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,menu->actionGeometry(action).center());});
            QTest::mouseClick(create,Qt::LeftButton);QVERIFY(ready);QTRY_VERIFY(!menu->isVisible());QTRY_COMPARE(session(w)["command"].toString(),QString("solid.hole"));
        };
        launch();qInfo().noquote()<<QJsonDocument(QJsonObject{{"step","after native Hole menu"},{"focus",w.state()["focus"]}}).toJson(QJsonDocument::Compact);
        const auto inputs=session(w)["inputs"].toObject();QCOMPARE(inputs["sourcePointId"],point["pointId"]);QCOMPARE(inputs["sourceEntityId"],point["id"]);QCOMPARE(inputs["sourceSketchId"].toString(),sketchId);QCOMPARE(inputs["targetBodyId"].toString(),target);
        const QJsonObject labels{{"hole.placement","From Sketch"},{"hole.type","Countersink"},{"hole.tap","Simple"},{"hole.extent","All"}};
        for(auto i=labels.begin();i!=labels.end();++i) {auto* combo=w.findChild<QComboBox*>(i.key());QVERIFY(combo);QCOMPARE(combo->currentText(),i.value().toString());}
        QVERIFY(session(w)["previewed"].toBool());QVERIFY(std::abs(session(w)["geometry"].toObject()["sinkDepthMm"].toDouble()-3)<1e-7);QCOMPARE(view->diagnostics()["cutPreviewToolCount"].toInt(),1);
        auto* targetButton=w.findChild<QPushButton*>("hole.selectTarget");QVERIFY(targetButton);QTest::mouseClick(targetButton,Qt::LeftButton);QCOMPARE(session(w)["selectionRole"].toString(),QString("target"));
        QJsonArray focusObservations;focusObservations.append(QJsonObject{{"step","after native Select Target"},{"focus",w.state()["focus"]}});
        const auto bp=call(w,"view.projectBody",{{"bodyId",target}});QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,QPoint(qRound(bp["x"].toDouble()),qRound(bp["y"].toDouble())));
        QCOMPARE(session(w)["selectionRole"].toString(),QString("ready"));
        QCOMPARE(session(w)["inputs"].toObject()["sourcePointId"],point["pointId"]);QCOMPARE(session(w)["inputs"].toObject()["targetBodyId"].toString(),target);
        focusObservations.append(QJsonObject{{"step","after actual target viewport click"},{"focus",w.state()["focus"]}});
        auto typeValue=[&](const char* field,const QString& text){auto* spin=w.findChild<QDoubleSpinBox*>(field);QVERIFY(spin);auto* editor=spin->findChild<QLineEdit*>();QVERIFY(editor);QTest::mouseClick(editor,Qt::LeftButton);
            const auto observed=w.state()["focus"].toObject();qInfo().noquote()<<QJsonDocument(QJsonObject{{"step",QString("native field click ")+field},{"focus",observed},{"visible",editor->isVisible()},{"editorHasFocus",editor->hasFocus()},{"editorEnabled",editor->isEnabled()}}).toJson(QJsonDocument::Compact);
            QVERIFY(editor->hasFocus());focusObservations.append(QJsonObject{{"step",QString("before typing ")+field},{"focus",w.state()["focus"]}});QTest::keyClick(editor,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(editor,text);QTest::keyClick(editor,Qt::Key_Tab);};
        auto* type=w.findChild<QComboBox*>("hole.type");QVERIFY(type);
        auto* extent=w.findChild<QComboBox*>("hole.extent");QVERIFY(extent);
        auto choose=[&](QComboBox* combo,int index) {
            QTest::mouseClick(combo,Qt::LeftButton);
            QTRY_VERIFY(combo->view()->isVisible());
            QTRY_VERIFY(combo->view()->window()->isActiveWindow());
            auto* receiver=QApplication::focusWidget();QVERIFY(receiver);
            QVERIFY(receiver==combo->view() || combo->view()->isAncestorOf(receiver));
            QTest::keyClick(receiver,index==0 ? Qt::Key_Home : Qt::Key_End);
            QTRY_COMPARE(combo->view()->currentIndex().row(),index);
            QTest::keyClick(receiver,Qt::Key_Return);
            QTRY_VERIFY(!combo->view()->isVisible());
        };
        choose(type,0);
        QCOMPARE(session(w)["inputs"].toObject()["holeType"].toString(),QString("simple"));
        choose(extent,1);
        QCOMPARE(session(w)["inputs"].toObject()["extent"].toString(),QString("distance"));
        typeValue("hole.distanceMm","6");typeValue("hole.boreDiameterMm","5");
        QVERIFY(session(w)["previewed"].toBool());
        QVERIFY(std::abs(session(w)["geometry"].toObject()["removedVolumeMm3"].toDouble()-37.5*std::numbers::pi)<1e-5);
        const auto evidence=qEnvironmentVariable("ALLOY_SIMPLE_HOLE_EVIDENCE");
        if(!evidence.isEmpty()){QDir().mkpath(evidence);call(w,"capture.screenshot",{{"path",evidence+"/native-hole-preview.png"}});QFile state(evidence+"/native-preview.json");QVERIFY(state.open(QIODevice::WriteOnly));state.write(QJsonDocument(w.state()).toJson(QJsonDocument::Indented));QFile observations(evidence+"/native-focus.json");QVERIFY(observations.open(QIODevice::WriteOnly));observations.write(QJsonDocument(focusObservations).toJson(QJsonDocument::Indented));}
        auto* buttons=w.findChild<QDialogButtonBox*>("hole.buttons");QVERIFY(buttons);QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel),Qt::LeftButton);QTRY_VERIFY(session(w).isEmpty());QCOMPARE(call(w,"document.inspect")["authored"].toObject(),before);QCOMPARE(view->diagnostics()["cutPreviewToolCount"].toInt(),0);
        // Fresh command-first creation selects the actual Point; no hidden setup.
        launch();auto* pointButton=w.findChild<QPushButton*>("hole.selectPoint");QVERIFY(pointButton);QTest::mouseClick(pointButton,Qt::LeftButton);QCOMPARE(session(w)["selectionRole"].toString(),QString("point"));QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel());
        QCOMPARE(session(w)["selectionRole"].toString(),QString("ready"));QCOMPARE(session(w)["inputs"].toObject()["sourcePointId"],point["pointId"]);QVERIFY(session(w)["previewed"].toBool());
        type=w.findChild<QComboBox*>("hole.type");choose(type,0);
        buttons=w.findChild<QDialogButtonBox*>("hole.buttons");QVERIFY(buttons);QVERIFY(buttons->button(QDialogButtonBox::Ok)->isEnabled());QTest::mouseClick(buttons->button(QDialogButtonBox::Ok),Qt::LeftButton);QTRY_VERIFY(session(w).isEmpty());
        const auto created=call(w,"document.inspect");const auto hole=created["authored"].toObject()["features"].toArray().last().toObject();QCOMPARE(hole["type"].toString(),QString("hole"));QVERIFY(!hole.contains("bodyId"));
        QVERIFY(std::abs(created["evaluation"].toObject()["bodies"].toArray()[0].toObject()["volumeMm3"].toDouble()-(36500+210*std::numbers::pi))<1e-5);
        // Actual Timeline button reopens the retained command and its input IDs.
        QToolButton* timeline=nullptr;for(auto* button:w.findChildren<QToolButton*>())if(button->property("featureId").toString()==hole["id"].toString())timeline=button;
        QVERIFY(timeline);QTest::mouseClick(timeline,Qt::LeftButton);QTRY_COMPARE(session(w)["command"].toString(),QString("solid.hole"));QCOMPARE(session(w)["inputs"].toObject()["featureId"],hole["id"]);
        typeValue("hole.boreDiameterMm","5");QVERIFY(session(w)["previewed"].toBool());QVERIFY(std::abs(session(w)["geometry"].toObject()["removedVolumeMm3"].toDouble()-62.5*std::numbers::pi)<1e-5);
        buttons=w.findChild<QDialogButtonBox*>("hole.buttons");QTest::mouseClick(buttons->button(QDialogButtonBox::Ok),Qt::LeftButton);QTRY_VERIFY(session(w).isEmpty());QCOMPARE(call(w,"document.inspect")["authored"].toObject()["features"].toArray().last().toObject()["id"],hole["id"]);
        if(!evidence.isEmpty()){call(w,"view.orientation",{{"name","isometric"}});call(w,"capture.screenshot",{{"path",evidence+"/native-edited-hole.png"}});QFile state(evidence+"/native-final.json");QVERIFY(state.open(QIODevice::WriteOnly));state.write(QJsonDocument(w.state()).toJson(QJsonDocument::Indented));}
        w.hide();
    }
};
QTEST_MAIN(SimpleHoleControlsTest)
#include "simple_hole_controls.moc"
