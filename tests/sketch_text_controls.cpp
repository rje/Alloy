// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QJsonDocument>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPushButton>
#include <QTest>
#include <algorithm>
#include <cmath>

class SketchTextControlsTest final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& w,const QString& name,const QJsonObject& args={}){
        const auto r=w.execute(name,args);if(!r["ok"].toBool())qFatal("%s",QJsonDocument(r).toJson().constData());
        return r["result"].toObject();
    }
    QJsonObject session(AlloyWindow& w){return w.state()["activeSession"].toObject();}
    QJsonObject draft(AlloyWindow& w){return session(w)["draftSketch"].toObject();}
private slots:
    void typeMoveRotateCancelAndEdit(){
        AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1300,900);w.show();w.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&w));QApplication::setActiveWindow(&w);
        ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(child))view=v;
        QVERIFY(view);call(w,"sketch.begin",{{"plane","XY"},{"units","mm"}});call(w,"sketch.lookAt");
        const auto original=draft(w);const auto sketchId=original["id"].toString();
        call(w,"sketch.text");auto* dialog=w.findChild<QDialog*>("sketch.text.dialog");
        QVERIFY(dialog && dialog->isVisible());
        auto* content=dialog->findChild<QLineEdit*>("sketch.text.content");
        auto* font=dialog->findChild<QComboBox*>("sketch.text.font");
        auto* height=dialog->findChild<QDoubleSpinBox*>("sketch.text.height");
        auto* buttons=dialog->findChild<QDialogButtonBox*>();
        QVERIFY(content && font && height && buttons);QVERIFY(font->count()==2);
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(content));
        QTest::keyClick(QApplication::focusWidget(),Qt::Key_Backspace);
        QTest::keyClicks(QApplication::focusWidget(),"DSB");
        QCOMPARE(session(w)["pendingText"].toObject()["text"].toString(),QString("DSB"));
        QCOMPARE(draft(w),original);
        QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel),Qt::LeftButton);
        QCOMPARE(draft(w),original);QVERIFY(!dialog->isVisible());
        call(w,"sketch.text");QVERIFY(dialog->isVisible());
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(content));
        QTest::keyClick(QApplication::focusWidget(),Qt::Key_Backspace);
        QTest::keyClicks(QApplication::focusWidget(),"DSB");
        QTest::mouseClick(font,Qt::LeftButton);QTest::keyClick(font,Qt::Key_Down);
        QTest::mouseClick(height,Qt::LeftButton);
        QTest::keyClick(QApplication::focusWidget(),Qt::Key_A,Qt::ControlModifier);
        QTest::keyClicks(QApplication::focusWidget(),"14");
        QTest::keyClick(QApplication::focusWidget(),Qt::Key_Tab);
        QCOMPARE(session(w)["pendingText"].toObject()["fontId"].toString(),QString("dejavu-sans-2.37-bold"));
        QVERIFY(std::abs(session(w)["pendingText"].toObject()["heightMm"].toDouble()-14)<.01);
        auto pixel=[&](double u,double v){const auto p=call(w,"view.projectSketchPoint",{{"sketchId",sketchId},{"uMm",u},{"vMm",v}});
            return QPoint(qRound(p["x"].toDouble()),qRound(p["y"].toDouble()));};
        const auto start=pixel(0,0),end=pixel(20,-8);
        QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,start);
        QMouseEvent motion(QEvent::MouseMove,QPointF(end),QPointF(view->mapToGlobal(end)),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);
        QApplication::sendEvent(view,&motion);QTest::mouseRelease(view,Qt::LeftButton,Qt::NoModifier,end);
        auto text=session(w)["pendingText"].toObject();
        QVERIFY(std::abs(text["uMm"].toDouble()-20)<.3 && std::abs(text["vMm"].toDouble()+8)<.3);
        const double arm=std::max(8.,text["heightMm"].toDouble()*1.35);
        const auto rotate=pixel(text["uMm"].toDouble()+arm,text["vMm"].toDouble());
        const auto rotated=pixel(text["uMm"].toDouble(),text["vMm"].toDouble()+arm);
        QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,rotate);
        QMouseEvent turning(QEvent::MouseMove,QPointF(rotated),QPointF(view->mapToGlobal(rotated)),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);
        QApplication::sendEvent(view,&turning);QTest::mouseRelease(view,Qt::LeftButton,Qt::NoModifier,rotated);
        QVERIFY(std::abs(session(w)["pendingText"].toObject()["angleDeg"].toDouble()-90)<2);
        QCOMPARE(draft(w),original);
        QTest::mouseClick(buttons->button(QDialogButtonBox::Ok),Qt::LeftButton);
        auto authored=draft(w)["parameters"].toObject()["texts"].toArray();QCOMPARE(authored.size(),1);
        QCOMPARE(authored[0].toObject()["text"].toString(),QString("DSB"));
        call(w,"sketch.finish");
        call(w,"sketch.edit",{{"sketchId",sketchId}});call(w,"sketch.textDialog");
        QVERIFY(dialog->isVisible());
        QCOMPARE(dialog->findChild<QLineEdit*>("sketch.text.content")->text(),QString("DSB"));
        QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel),Qt::LeftButton);
        call(w,"sketch.cancel");
    }
};
QTEST_MAIN(SketchTextControlsTest)
#include "sketch_text_controls.moc"
