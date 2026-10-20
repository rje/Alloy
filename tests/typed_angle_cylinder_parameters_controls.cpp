// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QJsonDocument>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTest>

class TypedAngleCylinderControls final:public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& w,const QString& cmd,const QJsonObject& args={}){auto r=w.execute(cmd,args);if(!r["ok"].toBool())qFatal("%s: %s",qPrintable(cmd),QJsonDocument(r).toJson().constData());return r["result"].toObject();}
    void type(QWidget* field,const QString& text){QVERIFY(field);QTest::mouseClick(field,Qt::LeftButton);QTRY_VERIFY(QApplication::focusWidget()==field || QApplication::focusWidget()==field->parentWidget());QTest::keyClick(field,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(field,text);QTest::keyClick(field,Qt::Key_Return);}
private slots:
    void nativeTypedFields() {
        AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1400,950);w.show();QVERIFY(QTest::qWaitForWindowExposed(&w));w.activateWindow();QApplication::setActiveWindow(&w);
        call(w,"document.parametersDialog");auto* dialog=w.findChild<QDialog*>("parameters.dialog");QVERIFY(dialog);
        QTRY_VERIFY(dialog->isActiveWindow());
        auto* unit=dialog->findChild<QComboBox*>("parameters.units");QVERIFY(unit);QVERIFY(unit->findText("deg")>=0);unit->setCurrentText("deg");
        auto* name=dialog->findChild<QLineEdit*>("parameters.name");auto* value=dialog->findChild<QLineEdit*>("parameters.value");
        QTest::mouseClick(name,Qt::LeftButton);QTest::keyClicks(name,"Opening");QTest::mouseClick(value,Qt::LeftButton);QTest::keyClicks(value,"30");
        QTest::mouseClick(dialog->findChild<QPushButton*>("parameters.add"),Qt::LeftButton);
        auto* table=dialog->findChild<QTableWidget*>("parameters.table");QTRY_COMPARE(table->rowCount(),1);QCOMPARE(table->item(0,1)->text(),QString("deg"));QCOMPARE(table->item(0,2)->text(),QString("30"));
        call(w,"document.parametersDialog",{{"visible",false}});QTRY_VERIFY(!dialog->isVisible());
        call(w,"document.new",{{"discard",true}});w.close();
    }
    void nativeBindingFields() {
        AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1400,950);w.show();QVERIFY(QTest::qWaitForWindowExposed(&w));w.activateWindow();QApplication::setActiveWindow(&w);
        call(w,"document.parameterAdd",{{"name","Opening"},{"value",30},{"units","deg"}});
        call(w,"document.parameterAdd",{{"name","Height"},{"value",10},{"units","mm"}});
        const auto users=call(w,"document.inspect")["userParameters"].toArray();const auto aid=users[0].toObject().value("id"),hid=users[1].toObject().value("id");
        auto session=call(w,"primitive.cylinder");auto* height=w.findChild<QDoubleSpinBox*>("primitive.height");QVERIFY(height);
        auto* input=height->findChild<QLineEdit*>();QVERIFY(input);type(input,"Height");
        QTRY_COMPARE(call(w,"app.state")["activeSession"].toObject()["inputs"].toObject()["heightParameter"].toString(),QString("Height"));
        call(w,"command.commit",{{"session",session["id"]}});auto stock=call(w,"document.inspect")["authored"].toObject()["features"].toArray()[0].toObject();QCOMPARE(stock["parameters"].toObject()["heightParameterId"],hid);
        session=call(w,"feature.editDialog",{{"featureId",stock["id"]}});height=w.findChild<QDoubleSpinBox*>("primitive.height");input=height->findChild<QLineEdit*>();QVERIFY(input->text()=="Height" || input->text()=="Height mm");
        type(input,"10");call(w,"command.commit",{{"session",session["id"]}});QVERIFY(!call(w,"document.inspect")["authored"].toObject()["features"].toArray()[0].toObject()["parameters"].toObject().contains("heightParameterId"));
        call(w,"sketch.begin",{{"plane","XY"}});call(w,"sketch.line",{{"x1",0},{"y1",0},{"x2",30},{"y2",0}});call(w,"sketch.line",{{"x1",0},{"y1",0},{"x2",26},{"y2",15}});
        auto draft=call(w,"app.state")["activeSession"].toObject()["draftSketch"].toObject();auto lines=draft["parameters"].toObject()["entities"].toArray();
        call(w,"sketch.dimension",{{"kind","angle"},{"entityIds",QJsonArray{lines[0].toObject()["id"],lines[1].toObject()["id"]}},{"valueDeg",30},{"annotation",QJsonObject{{"uMm",12},{"vMm",5}}}});
        draft=call(w,"app.state")["activeSession"].toObject()["draftSketch"].toObject();const auto did=draft["parameters"].toObject()["dimensions"].toArray().last().toObject().value("id");
        call(w,"sketch.dimensionEdit",{{"dimensionId",did}});input=w.findChild<QLineEdit*>("sketch.inlineValue");QVERIFY(input && input->isVisible());QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(input));
        QTest::keyClicks(input,"Opening");QTest::keyClick(input,Qt::Key_Return);
        QTRY_VERIFY(!call(w,"app.state")["activeSession"].toObject().contains("pendingDimension"));
        draft=call(w,"app.state")["activeSession"].toObject()["draftSketch"].toObject();QCOMPARE(draft["parameters"].toObject()["dimensions"].toArray().last().toObject()["parameterId"],aid);
        call(w,"sketch.dimensionEdit",{{"dimensionId",did}});QCOMPARE(input->text(),QString("Opening"));QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(input));QTest::keyClick(input,Qt::Key_Return);
        draft=call(w,"app.state")["activeSession"].toObject()["draftSketch"].toObject();QCOMPARE(draft["parameters"].toObject()["dimensions"].toArray().last().toObject()["parameterId"],aid);
        call(w,"sketch.dimensionEdit",{{"dimensionId",did}});QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(input));QTest::keyClicks(input,"30");QTest::keyClick(input,Qt::Key_Return);
        draft=call(w,"app.state")["activeSession"].toObject()["draftSketch"].toObject();QVERIFY(!draft["parameters"].toObject()["dimensions"].toArray().last().toObject().contains("parameterId"));
        call(w,"sketch.cancel");call(w,"document.new",{{"discard",true}});w.close();
    }
};
QTEST_MAIN(TypedAngleCylinderControls)
#include "typed_angle_cylinder_parameters_controls.moc"
