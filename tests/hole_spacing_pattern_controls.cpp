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
#include <QSpinBox>
#include <QLineEdit>
#include <QWindow>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QMouseEvent>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <cmath>
#include <numbers>
class HoleSpacingPatternControlsTest final:public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& w,const QString& command,const QJsonObject& args={}) {
        const auto r=w.execute(command,args);if(!r["ok"].toBool())qFatal("%s",QJsonDocument(r).toJson().constData());return r["result"].toObject();
    }
    QJsonObject session(AlloyWindow& w){return w.state()["activeSession"].toObject();}
private slots:
    void nativeHoleSeedEdgesSpacingCancelCommitAndEdit() {
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
        call(w,"solid.hole",{{"sourceSketchId",sketchId},{"sourceEntityId",point["id"]},{"sourcePointId",point["pointId"]},{"targetBodyId",target},{"placement","fromSketch"},{"holeType","countersink"},{"extent","all"},{"boreDiameterMm",4},{"sinkDiameterMm",10},{"includedAngleDeg",90}});
        const auto before=call(w,"document.inspect")["authored"].toObject();const auto holeId=before["features"].toArray().last().toObject()["id"].toString();
        body=call(w,"document.inspect")["evaluation"].toObject()["bodies"].toArray()[0].toObject();
        QString edgeIds[2];double signs[2]={0,0};
        for(const auto& value:body["solidEdges"].toArray()){
            const auto edge=value.toObject(),provenance=edge["provenance"].toObject();if(provenance["role"]!="endRim")continue;
            const auto start=provenance["startMm"].toArray(),end=provenance["endMm"].toArray();if(start.size()!=3||end.size()!=3)continue;
            const double dx=end[0].toDouble()-start[0].toDouble(),dy=end[1].toDouble()-start[1].toDouble();
            if(edgeIds[0].isEmpty() && std::abs(dx)>20 && std::abs(dy)<1e-7){edgeIds[0]=edge["edgeId"].toString();signs[0]=dx>0 ? 1 : -1;}
            if(edgeIds[1].isEmpty() && std::abs(dy)>20 && std::abs(dx)<1e-7){edgeIds[1]=edge["edgeId"].toString();signs[1]=dy<0 ? 1 : -1;}
        }
        QVERIFY(!edgeIds[0].isEmpty());QVERIFY(!edgeIds[1].isEmpty());
        w.activateWindow();QApplication::setActiveWindow(&w);view->setFocus();if(view->windowHandle())view->windowHandle()->requestActivate();QTRY_VERIFY(view->hasFocus()); // Initial-only harness activation.
        auto launch=[&]{auto* create=w.findChild<QToolButton*>("solid.menu.create");QVERIFY(create);auto* menu=create->menu();auto* action=w.findChild<QAction*>("solid.action.solid.rectangularPatternDialog");QVERIFY(action);bool visible=false;
            QTimer::singleShot(100,&w,[&]{visible=menu->isVisible();if(visible)QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,menu->actionGeometry(action).center());});QTest::mouseClick(create,Qt::LeftButton);QVERIFY(visible);QTRY_COMPARE(session(w)["command"].toString(),QString("solid.rectangularPattern"));};
        auto chooseSeed=[&]{QToolButton* timeline=nullptr;for(auto* button:w.findChildren<QToolButton*>())if(button->property("featureId").toString()==holeId)timeline=button;QVERIFY(timeline);QTest::mouseClick(timeline,Qt::LeftButton);QCOMPARE(session(w)["inputs"].toObject()["sourceFeatureId"].toString(),holeId);
            QCOMPARE(w.findChild<QComboBox*>("patternDistribution")->currentText(),QString("Spacing"));QCOMPARE(w.findChild<QComboBox*>("patternCompute")->currentText(),QString("Adjust"));};
        auto typeValue=[&](QAbstractSpinBox* spin,const QString& value){QVERIFY(spin);auto* editor=spin->findChild<QLineEdit*>();QVERIFY(editor);QTest::mouseClick(editor,Qt::LeftButton);QVERIFY(editor->hasFocus());QTest::keyClick(editor,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(editor,value);QTest::keyClick(editor,Qt::Key_Tab);};
        auto axes=[&]{for(int i=0;i<2;++i){auto* button=w.findChild<QPushButton*>("patternAxis"+QString::number(i));QVERIFY(button);QTest::mouseClick(button,Qt::LeftButton);QCOMPARE(session(w)["activeAxis"].toInt(),i);
                const auto pixel=call(w,"view.projectEdge",{{"bodyId",target},{"edgeId",edgeIds[i]}});QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,QPoint(qRound(pixel["x"].toDouble()),qRound(pixel["y"].toDouble())));
                QCOMPARE(session(w)["inputs"].toObject()["axes"].toArray()[i].toObject()["edgeId"].toString(),edgeIds[i]);
                typeValue(w.findChild<QSpinBox*>("patternQuantity"+QString::number(i)),"3");typeValue(w.findChild<QDoubleSpinBox*>("patternExtent"+QString::number(i)),QString::number((i==0 ? 27.5 : 15)*signs[i]));}
            QVERIFY(session(w)["previewed"].toBool());QCOMPARE(session(w)["geometry"].toObject()["instanceCount"].toInt(),9);};
        launch();chooseSeed();axes();auto* flip=w.findChild<QPushButton*>("patternFlip0");QVERIFY(flip);QTest::mouseClick(flip,Qt::LeftButton);QCOMPARE(session(w)["inputs"].toObject()["axes"].toArray()[0].toObject()["spacingMm"].toDouble(),-27.5*signs[0]);QTest::mouseClick(flip,Qt::LeftButton);QVERIFY(session(w)["previewed"].toBool());const auto evidence=qEnvironmentVariable("ALLOY_PATTERN_CONTROLS_EVIDENCE");
        if(!evidence.isEmpty()){QDir().mkpath(evidence);call(w,"capture.screenshot",{{"path",evidence+"/native-pattern-preview.png"}});QFile f(evidence+"/native-preview.json");QVERIFY(f.open(QIODevice::WriteOnly));f.write(QJsonDocument(w.state()).toJson());}
        auto* buttons=w.findChild<QDialogButtonBox*>("patternButtons");QVERIFY(buttons);QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel),Qt::LeftButton);QTRY_VERIFY(session(w).isEmpty());QCOMPARE(call(w,"document.inspect")["authored"].toObject(),before);
        launch();chooseSeed();axes();buttons=w.findChild<QDialogButtonBox*>("patternButtons");QVERIFY(buttons->button(QDialogButtonBox::Ok)->isEnabled());QTest::mouseClick(buttons->button(QDialogButtonBox::Ok),Qt::LeftButton);QTRY_VERIFY(session(w).isEmpty());
        const auto created=call(w,"document.inspect");const auto pattern=created["authored"].toObject()["features"].toArray().last().toObject();const auto patternId=pattern["id"].toString();QCOMPARE(pattern["parameters"].toObject()["distribution"].toString(),QString("spacing"));
        QVERIFY(std::abs(created["evaluation"].toObject()["bodies"].toArray()[0].toObject()["volumeMm3"].toDouble()-(36500-353*std::numbers::pi))<1e-5);
        QToolButton* timeline=nullptr;for(auto* button:w.findChildren<QToolButton*>())if(button->property("featureId").toString()==patternId)timeline=button;QVERIFY(timeline);QTest::mouseClick(timeline,Qt::LeftButton);QTRY_COMPARE(session(w)["inputs"].toObject()["featureId"].toString(),patternId);
        typeValue(w.findChild<QDoubleSpinBox*>("patternExtent0"),QString::number(27*signs[0]));QVERIFY(session(w)["previewed"].toBool());typeValue(w.findChild<QDoubleSpinBox*>("patternExtent0"),QString::number(27.5*signs[0]));
        buttons=w.findChild<QDialogButtonBox*>("patternButtons");QTest::mouseClick(buttons->button(QDialogButtonBox::Ok),Qt::LeftButton);QTRY_VERIFY(session(w).isEmpty());QCOMPARE(call(w,"document.inspect")["authored"].toObject()["features"].toArray().last().toObject()["id"].toString(),patternId);
        if(!evidence.isEmpty()){call(w,"view.orientation",{{"name","isometric"}});call(w,"capture.screenshot",{{"path",evidence+"/native-pattern-final.png"}});QFile f(evidence+"/native-final.json");QVERIFY(f.open(QIODevice::WriteOnly));f.write(QJsonDocument(w.state()).toJson());}
        w.hide();
    }
};
QTEST_MAIN(HoleSpacingPatternControlsTest)
#include "hole_spacing_pattern_controls.moc"
