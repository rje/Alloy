// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QFile>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPushButton>
#include <QTableWidget>
#include <QTest>
#include <QMap>
#include <cmath>

class NamedLengthParametersControlsTest final:public QObject{
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& args={}){
        const auto result=window.executeApi(command,args);if(!result["ok"].toBool())qFatal("%s: %s",qPrintable(command),QJsonDocument(result).toJson().constData());return result["result"].toObject();
    }
private slots:
    void ownedDialogCaptureAndPendingEdits() {
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1400,900);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));QApplication::setActiveWindow(&window);
        call(window,"document.parametersDialog");
        auto* dialog=window.findChild<QDialog*>("parameters.dialog");QVERIFY(dialog && dialog->isVisible());
        call(window,"document.parametersDialog");QCOMPARE(window.findChildren<QDialog*>("parameters.dialog").size(),1);
        auto* name=dialog->findChild<QLineEdit*>("parameters.name");auto* value=dialog->findChild<QLineEdit*>("parameters.value");
        auto* table=dialog->findChild<QTableWidget*>("parameters.table");QVERIFY(name && value && table);
        name->setText("Uncommitted");value->setText("123");
        call(window,"document.parameterAdd",{{"name","Length"},{"value",75},{"units","mm"}});
        table->setCurrentCell(0,0);const auto selected=table->item(0,0)->data(Qt::UserRole);
        call(window,"document.parameterAdd",{{"name","Width"},{"value",50},{"units","mm"}});
        call(window,"document.parameterAdd",{{"name","Thickness"},{"value",10},{"units","mm"}});
        QCOMPARE(table->rowCount(),3);QCOMPARE(name->text(),QString("Uncommitted"));QCOMPARE(value->text(),QString("123"));
        QCOMPARE(table->item(table->currentRow(),0)->data(Qt::UserRole),selected);
        QCOMPARE(table->item(2,0)->text(),QString("Thickness"));QCOMPARE(table->item(2,2)->text(),QString("10"));
        const auto before=call(window,"document.inspect")["authored"].toObject();
        call(window,"document.parametersDialog",{{"visible",false}});QVERIFY(!dialog->isVisible());
        QCOMPARE(call(window,"document.inspect")["authored"].toObject(),before);
        call(window,"document.parametersDialog");auto* reopened=window.findChildren<QDialog*>("parameters.dialog").last();QVERIFY(reopened->isVisible());
        call(window,"document.parametersDialog",{{"visible",false}});call(window,"document.new",{{"discard",true}});window.close();
    }

    void nativeCreateName(){
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1300,900);window.show();window.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&window));QApplication::setActiveWindow(&window);
        ReferenceView* view=nullptr;for(auto* child:window.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;
        QVERIFY(view);
        call(window,"document.parameterAdd",{{"name","Length"},{"value",75},{"units","mm"}});
        const auto lengthId=call(window,"document.inspect")["userParameters"].toArray()[0].toObject()["id"];
        call(window,"sketch.begin",{{"plane","XY"},{"units","mm"}});call(window,"sketch.lookAt");call(window,"view.zoom",{{"factor",0.65}});
        call(window,"sketch.tool",{{"tool","centerRectangle"}});
        const auto sketchId=window.state()["activeSession"].toObject()["draftSketch"].toObject()["id"];
        auto pixel=[&](double u,double v){const auto p=call(window,"view.projectSketchPoint",{{"sketchId",sketchId},{"uMm",u},{"vMm",v}});
            return QPoint(qRound(p["x"].toDouble()),qRound(p["y"].toDouble()));};
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(0,0));
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(33,24));
        const auto rough=window.state()["activeSession"].toObject()["draftSketch"].toObject()["parameters"].toObject();
        QCOMPARE(rough["dimensions"].toArray().size(),0);
        const auto edge=rough["entities"].toArray()[2].toObject();QMap<QString,QPointF> points;
        for(const auto& entry:rough["points"].toArray()){const auto p=entry.toObject();points[p["id"].toString()]={p["uMm"].toDouble(),p["vMm"].toDouble()};}
        const auto middle=(points[edge["startPointId"].toString()]+points[edge["endPointId"].toString()])/2;
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QTest::keyClick(QApplication::focusWidget(),Qt::Key_D);
        QCOMPARE(window.state()["activeSession"].toObject()["tool"].toString(),QString("dimension"));
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(middle.x(),middle.y()));
        QCOMPARE(window.state()["activeSession"].toObject()["selection"].toObject()["entityIds"].toArray().size(),1);
        const auto label=pixel(middle.x(),middle.y()+8);
        QMouseEvent move(QEvent::MouseMove,QPointF(label),QPointF(view->mapToGlobal(label)),Qt::NoButton,Qt::NoButton,Qt::NoModifier);
        QApplication::sendEvent(view,&move);
        QTRY_COMPARE(window.state()["activeSession"].toObject()["pendingDimension"].toObject()["phase"].toString(),QString("placing"));
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,label);
        auto* inlineValue=window.findChild<QLineEdit*>("sketch.inlineValue");QVERIFY(inlineValue && inlineValue->isVisible());
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(inlineValue));
        QTest::keyClick(QApplication::focusWidget(),Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(QApplication::focusWidget(),"Length");
        QTest::keyClick(QApplication::focusWidget(),Qt::Key_Return);
        const auto draft=window.state()["activeSession"].toObject()["draftSketch"].toObject();const auto dimensions=draft["parameters"].toObject()["dimensions"].toArray();
        QCOMPARE(dimensions.size(),1);QCOMPARE(dimensions[0].toObject()["parameterId"],lengthId);QCOMPARE(dimensions[0].toObject()["valueMm"].toDouble(),75.);
        QApplication::processEvents();QTest::qWait(80);
        auto* info=window.findChild<QLabel*>("sketch.info");auto* profileStatus=window.findChild<QLabel*>("sketch.profileStatus");
        QVERIFY(info && profileStatus);QTRY_VERIFY(profileStatus->y()>=info->y()+info->height());
        const auto evidence=qEnvironmentVariable("ALLOY_PARAMETER_EVIDENCE");if(!evidence.isEmpty())call(window,"capture.screenshot",{{"path",evidence+"/named-length-native-create.png"}});
        call(window,"sketch.cancel");call(window,"document.new",{{"discard",true}});window.close();
    }
    void nativeAndSharedCommands(){
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1400,900);window.show();window.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&window));QApplication::setActiveWindow(&window);
        call(window,"document.parametersDialog");auto* dialog=window.findChild<QDialog*>();QVERIFY(dialog && dialog->isVisible());
        auto* name=dialog->findChild<QLineEdit*>("parameters.name");auto* value=dialog->findChild<QLineEdit*>("parameters.value");
        auto* table=dialog->findChild<QTableWidget*>("parameters.table");auto* add=dialog->findChild<QPushButton*>("parameters.add");
        auto* update=dialog->findChild<QPushButton*>("parameters.update");QVERIFY(name && value && table && add && update);
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(name));
        QTest::keyClicks(QApplication::focusWidget(),"Length");QTest::keyClick(QApplication::focusWidget(),Qt::Key_Tab);
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(value));
        QTest::keyClicks(QApplication::focusWidget(),"75");QTest::mouseClick(add,Qt::LeftButton);
        QCOMPARE(table->rowCount(),1);const auto first=call(window,"document.inspect")["userParameters"].toArray()[0].toObject();
        QCOMPARE(first["name"].toString(),QString("Length"));QCOMPARE(first["valueMm"].toDouble(),75.);
        QTest::mouseClick(table->viewport(),Qt::LeftButton,Qt::NoModifier,table->visualItemRect(table->item(0,0)).center());
        QTest::mouseClick(value,Qt::LeftButton);QTest::keyClick(QApplication::focusWidget(),Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(QApplication::focusWidget(),"76");
        QTest::mouseClick(update,Qt::LeftButton);QCOMPARE(call(window,"document.inspect")["userParameters"].toArray()[0].toObject()["valueMm"].toDouble(),76.);
        QTest::mouseClick(dialog->findChild<QPushButton*>("parameters.update"),Qt::LeftButton);QVERIFY(dialog->isVisible());
        dialog->close();
        call(window,"document.parameterAdd",{{"name","Width"},{"value",50},{"units","mm"}});
        call(window,"document.parameterAdd",{{"name","Thickness"},{"value",10},{"units","mm"}});
        call(window,"sketch.begin",{{"plane","XY"},{"units","mm"}});
        call(window,"sketch.rectangle",{{"x1",-30},{"y1",-20},{"x2",30},{"y2",20},{"units","mm"}});
        const auto session=window.state()["activeSession"].toObject();const auto sketch=session["draftSketch"].toObject();
        const auto points=sketch["parameters"].toObject()["points"].toArray(),entities=sketch["parameters"].toObject()["entities"].toArray();
        call(window,"sketch.constraint",{{"type","fixed"},{"pointIds",QJsonArray{points[0].toObject()["id"]}}});
        call(window,"sketch.dimension",{{"kind","horizontal"},{"entityId",entities[0].toObject()["id"]},{"value",76}});
        const auto horizontal=window.state()["activeSession"].toObject()["draftSketch"].toObject()["parameters"].toObject()["dimensions"].toArray().first().toObject();
        call(window,"sketch.dimensionEdit",{{"dimensionId",horizontal["id"]}});
        auto* inlineValue=window.findChild<QLineEdit*>("sketch.inlineValue");QVERIFY(inlineValue && inlineValue->isVisible());
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(inlineValue));
        QTest::keyClick(QApplication::focusWidget(),Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(QApplication::focusWidget(),"Length");
        QTest::keyClick(QApplication::focusWidget(),Qt::Key_Return);
        QTRY_VERIFY(window.state()["activeSession"].toObject()["draftSketch"].toObject()["parameters"].toObject()["dimensions"].toArray().first().toObject().contains("parameterId"));
        call(window,"sketch.dimension",{{"kind","vertical"},{"entityId",entities[1].toObject()["id"]},{"parameterName","Width"}});
        call(window,"sketch.finish");
        const auto inspected=call(window,"document.inspect");const auto sketchSummary=inspected["evaluation"].toObject()["sketches"].toArray()[0].toObject();
        call(window,"solid.extrudeDialog",{{"sketchId",sketchSummary["sketchId"]},{"profileId",sketchSummary["profiles"].toArray()[0].toObject()["profileId"]}});
        auto* dock=window.findChild<QDockWidget*>("extrudeDock");QVERIFY(dock);
        auto* distance=dock->findChild<QDoubleSpinBox*>("extrude.distance");auto* buttons=dock->findChild<QDialogButtonBox*>();QVERIFY(distance && buttons);
        auto* distanceInput=distance->findChild<QLineEdit*>();QVERIFY(distanceInput);
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(distance));
        QTest::keyClick(QApplication::focusWidget(),Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(QApplication::focusWidget(),"UnknownLength");QTest::keyClick(QApplication::focusWidget(),Qt::Key_Tab);
        QVERIFY(!window.state()["activeSession"].toObject()["previewed"].toBool());
        QVERIFY(dock->findChild<QLabel*>("extrude.error")->text().contains("unavailable"));
        QCOMPARE(call(window,"document.inspect")["evaluation"].toObject()["bodies"].toArray().size(),0);
        QTest::mouseClick(distance,Qt::LeftButton);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(distance));
        QTest::keyClick(QApplication::focusWidget(),Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(QApplication::focusWidget(),"Thickness");QTest::keyClick(QApplication::focusWidget(),Qt::Key_Tab);
        QTRY_VERIFY(window.state()["activeSession"].toObject()["previewed"].toBool());
        QCOMPARE(window.state()["activeSession"].toObject()["inputs"].toObject()["distanceParameter"].toString(),QString("Thickness"));
        const auto evidence=qEnvironmentVariable("ALLOY_PARAMETER_EVIDENCE");if(!evidence.isEmpty())call(window,"capture.screenshot",{{"path",evidence+"/named-length-extrude-preview.png"}});
        QTest::mouseClick(buttons->button(QDialogButtonBox::Ok),Qt::LeftButton);
        QTRY_VERIFY(window.state()["activeSession"].toObject().isEmpty());
        const auto finalState=call(window,"document.inspect");QCOMPARE(finalState["evaluation"].toObject()["bodies"].toArray().size(),1);
        QVERIFY(std::abs(finalState["evaluation"].toObject()["bodies"].toArray()[0].toObject()["volumeMm3"].toDouble()-38000)<1e-5);
        if(!evidence.isEmpty()){call(window,"capture.screenshot",{{"path",evidence+"/named-length-solid.png"}});QFile file(evidence+"/named-length-state.json");QVERIFY(file.open(QIODevice::WriteOnly));file.write(QJsonDocument(finalState).toJson());}
        const auto authored=finalState["authored"].toObject(),solid=authored["features"].toArray().last().toObject();
        call(window,"feature.editDialog",{{"featureId",solid["id"]}});
        auto* editDistance=window.findChild<QDoubleSpinBox*>("extrude.distance");QVERIFY(editDistance);
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(editDistance));
        QVERIFY(editDistance->findChild<QLineEdit*>()->text().contains("Thickness"));
        QTest::keyClick(QApplication::focusWidget(),Qt::Key_Return);
        QTRY_VERIFY(window.state()["activeSession"].toObject().isEmpty());
        const auto unchanged=call(window,"document.inspect")["authored"].toObject()["features"].toArray().last().toObject()["parameters"].toObject();
        QVERIFY(unchanged.contains("distanceParameterId"));
        call(window,"feature.editDialog",{{"featureId",solid["id"]}});
        editDistance=window.findChild<QDoubleSpinBox*>("extrude.distance");QVERIFY(editDistance);
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(editDistance));
        QTest::keyClick(QApplication::focusWidget(),Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(QApplication::focusWidget(),"10");QTest::keyClick(QApplication::focusWidget(),Qt::Key_Tab);
        QVERIFY(!window.state()["activeSession"].toObject()["inputs"].toObject().contains("distanceParameter"));
        call(window,"command.commit",{{"session",window.state()["activeSession"].toObject()["id"]}});
        QVERIFY(!call(window,"document.inspect")["authored"].toObject()["features"].toArray().last().toObject()["parameters"].toObject().contains("distanceParameterId"));
        call(window,"solid.extrude",{{"featureId",solid["id"]},{"sketchId",sketchSummary["sketchId"]},{"profileId",sketchSummary["profiles"].toArray()[0].toObject()["profileId"]},{"distance",12},{"units","mm"}});
        const auto literal=call(window,"document.inspect");
        QVERIFY(!literal["authored"].toObject()["features"].toArray().last().toObject()["parameters"].toObject().contains("distanceParameterId"));
        QVERIFY(std::abs(literal["evaluation"].toObject()["bodies"].toArray()[0].toObject()["volumeMm3"].toDouble()-45600)<1e-5);
        call(window,"document.new",{{"discard",true}});window.close();
    }
    void nativeFormulaCell(){
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1280,820);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        call(window,"document.parameterAdd",{{"name","Width"},{"value",40},{"units","mm"}});
        call(window,"document.parameterAdd",{{"name","Length"},{"value",80},{"units","mm"}});
        call(window,"document.parametersDialog");
        auto* dialog=window.findChild<QDialog*>("parameters.dialog");QVERIFY(dialog && dialog->isVisible());
        auto* table=dialog->findChild<QTableWidget*>("parameters.table");QVERIFY(table);
        QApplication::setActiveWindow(dialog);QTRY_VERIFY(dialog->isActiveWindow());
        auto* expression=table->item(1,3);QVERIFY(expression);
        table->scrollToItem(expression);
        const auto cell=table->visualItemRect(expression).center();
        QTest::mouseClick(table->viewport(),Qt::LeftButton,Qt::NoModifier,cell);
        QTest::mouseDClick(table->viewport(),Qt::LeftButton,Qt::NoModifier,cell);
        QTRY_VERIFY(qobject_cast<QLineEdit*>(QApplication::focusWidget()));
        QTest::keyClick(QApplication::focusWidget(),Qt::Key_A,Qt::ControlModifier);
        QTest::keyClicks(QApplication::focusWidget(),"Width*2");
        QTest::keyClick(QApplication::focusWidget(),Qt::Key_Return);
        QTRY_COMPARE(table->item(1,3)->text(),QString("Width*2"));
        QCOMPARE(table->item(1,2)->text(),QString("80"));
        const auto state=call(window,"document.inspect");
        QCOMPARE(state["userParameters"].toArray()[1].toObject()["expression"],QJsonValue("Width*2"));
        const auto evidence=qEnvironmentVariable("ALLOY_EXPRESSION_EVIDENCE");
        if(!evidence.isEmpty())call(window,"capture.dialog",{{"path",evidence+"/native-formula-cell.png"}});
        dialog->close();call(window,"document.new",{{"discard",true}});window.close();
    }
};
QTEST_MAIN(NamedLengthParametersControlsTest)
#include "named_length_parameters_controls.moc"
