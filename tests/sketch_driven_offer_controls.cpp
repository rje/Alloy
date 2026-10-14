// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTest>
#include <QWindow>
#include <cmath>

class SketchDrivenOfferControls final:public QObject {
    Q_OBJECT
    static QJsonObject call(AlloyWindow& window,const QString& name,const QJsonObject& args={}){
        const auto response=window.executeApi(name,args);
        if(!response.value("ok").toBool())qFatal("%s: %s",qPrintable(name),qPrintable(QString::fromUtf8(QJsonDocument(response).toJson())));
        return response.value("result").toObject();
    }
    static QJsonObject session(const AlloyWindow& window){return window.state().value("activeSession").toObject();}
    static QJsonObject draft(const AlloyWindow& window){return session(window).value("draftSketch").toObject();}
    static void save(const QString& path,const QJsonObject& value){QFile file(path);QVERIFY(file.open(QIODevice::WriteOnly));file.write(QJsonDocument(value).toJson(QJsonDocument::Indented));}
private slots:
    void explicitDrivenChoice(){
        const auto output=qEnvironmentVariable("ALLOY_DRIVEN_OFFER_EVIDENCE");QVERIFY(!output.isEmpty());QVERIFY(QDir().mkpath(output));
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1450,900);window.show();QVERIFY(QTest::qWaitForWindowExposed(&window));
        window.raise();window.activateWindow();window.windowHandle()->requestActivate();QTRY_VERIFY(window.isActiveWindow());
        call(window,"sketch.begin",{{"plane","XY"},{"units","mm"}});
        call(window,"sketch.rectangle",{{"x1",0},{"y1",0},{"x2",40},{"y2",25},{"units","mm"}});
        const auto entities=draft(window).value("parameters").toObject().value("entities").toArray();
        const auto bottom=entities[0].toObject(),right=entities[1].toObject();
        call(window,"sketch.constraint",{{"type","fixed"},{"pointIds",QJsonArray{bottom.value("startPointId")}}});
        call(window,"sketch.dimension",{{"kind","horizontal"},{"entityId",bottom.value("id")},{"value",40},{"units","mm"}});
        const auto widthId=draft(window).value("parameters").toObject().value("dimensions").toArray().last().toObject().value("id").toString();
        call(window,"sketch.dimension",{{"kind","vertical"},{"entityId",right.value("id")},{"value",25},{"units","mm"}});
        const auto before=draft(window),journal=session(window).value("draftHistory").toObject();
        const QJsonObject duplicate{{"kind","horizontal"},{"entityId",bottom.value("id")},{"value",40},{"units","mm"}};
        const auto first=call(window,"sketch.dimension",duplicate);
        QVERIFY(first.value("pending").toBool());QVERIFY(!first.value("offerId").toString().isEmpty());
        QCOMPARE(draft(window),before);QCOMPARE(session(window).value("draftHistory").toObject(),journal);
        auto* panel=window.findChild<QWidget*>("sketch.dimension.offer");
        auto* accept=window.findChild<QPushButton*>("sketch.dimension.offerAccept");
        auto* cancel=window.findChild<QPushButton*>("sketch.dimension.offerCancel");
        QVERIFY(panel&&accept&&cancel);QTRY_VERIFY(panel->isVisible());QVERIFY(accept->isEnabled());
        auto* text=window.findChild<QLabel*>("sketch.dimension.offerText");QVERIFY(text&&text->text().contains("read-only Driven"));
        call(window,"capture.screenshot",{{"path",output+"/driven-offer.png"}});
        QVERIFY(QApplication::widgetAt(cancel->mapToGlobal(cancel->rect().center()))==cancel);
        QTest::mouseClick(cancel,Qt::LeftButton);
        QVERIFY(!session(window).contains("pendingDrivenOffer"));QCOMPARE(draft(window),before);
        QCOMPARE(session(window).value("draftHistory").toObject(),journal);
        const auto conflict=window.executeApi("sketch.dimension",{{"kind","horizontal"},{"entityId",bottom.value("id")},{"value",41},{"units","mm"}});
        QVERIFY(!conflict.value("ok").toBool());QVERIFY(!session(window).contains("pendingDrivenOffer"));
        QCOMPARE(draft(window),before);QCOMPARE(session(window).value("draftHistory").toObject(),journal);
        call(window,"sketch.selectionSet",{{"entityIds",QJsonArray{bottom.value("id")}}});
        const auto pixel=call(window,"view.projectSketchPoint",{{"sketchId",draft(window).value("id")},{"uMm",20},{"vMm",-8}});
        call(window,"sketch.dimensionPlace",{{"x",pixel.value("x")},{"y",pixel.value("y")}});
        auto* inlineValue=window.findChild<QLineEdit*>("sketch.inlineValue");
        QVERIFY(inlineValue && inlineValue->isVisible());
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(inlineValue));
        QTest::keyClick(QApplication::focusWidget(),Qt::Key_A,Qt::ControlModifier);
        QTest::keyClicks(QApplication::focusWidget(),"40");
        QTest::keyClick(QApplication::focusWidget(),Qt::Key_Return);
        const auto offered=session(window).value("pendingDrivenOffer").toObject();
        QVERIFY(!offered.value("id").toString().isEmpty());
        QTRY_VERIFY(panel->isVisible());QVERIFY(QApplication::widgetAt(accept->mapToGlobal(accept->rect().center()))==accept);
        QTest::mouseClick(accept,Qt::LeftButton);
        QVERIFY(!session(window).contains("pendingDrivenOffer"));
        auto dimensions=draft(window).value("parameters").toObject().value("dimensions").toArray();
        QCOMPARE(dimensions.size(),3);
        const auto measured=dimensions.last().toObject();
        QCOMPARE(measured.value("id"),offered.value("dimensionId"));
        QVERIFY(!measured.value("driving").toBool());QVERIFY(!measured.contains("valueMm"));
        QCOMPARE(session(window).value("draftHistory").toObject().value("count").toInt(),journal.value("count").toInt()+1);
        const auto acceptedDraft=draft(window);
        call(window,"sketch.tool",{{"tool","select"}});
        call(window,"history.undo");QCOMPARE(draft(window),before);
        call(window,"history.redo");QCOMPARE(draft(window),acceptedDraft);
        const auto edit=call(window,"sketch.dimensionEdit",{{"dimensionId",measured.value("id")}});
        QVERIFY(!edit.value("driving").toBool());
        auto* value=window.findChild<QLineEdit*>("sketch.inlineValue");QVERIFY(value && value->isReadOnly());
        call(window,"sketch.dimensionCancel");
        call(window,"sketch.dimensionUpdate",{{"dimensionId",widthId},{"value",50},{"units","mm"}});
        bool measured50=false;
        for(const auto& item:session(window).value("solverDiagnostics").toObject().value("dimensionValues").toArray())
            if(item.toObject().value("id")==measured.value("id"))
                measured50=std::abs(item.toObject().value("valueMm").toDouble()-50)<1e-5;
        QVERIFY(measured50);
        save(output+"/accepted-session.json",window.state());
        call(window,"capture.screenshot",{{"path",output+"/driven-after-width-edit.png"}});
        call(window,"sketch.finish");
        const auto committed=call(window,"document.inspect").value("authored").toObject();
        call(window,"history.undo");
        QVERIFY(call(window,"document.inspect").value("authored").toObject()!=committed);
        call(window,"history.redo");
        QCOMPARE(call(window,"document.inspect").value("authored").toObject(),committed);
        const auto saved=output+"/driven-rectangle.alloy";call(window,"document.save",{{"path",saved}});
        const auto authored=call(window,"document.inspect").value("authored").toObject();
        call(window,"document.open",{{"path",saved}});
        QCOMPARE(call(window,"document.inspect").value("authored").toObject(),authored);
        window.close();
    }
};
QTEST_MAIN(SketchDrivenOfferControls)
#include "sketch_driven_offer_controls.moc"
