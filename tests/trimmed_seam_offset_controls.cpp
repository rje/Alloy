// SPDX-License-Identifier: MIT
#include "AlloyTheme.h"
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QCheckBox>
#include <QDir>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QTest>
#include <QWindow>
#include <QSet>
#include <cmath>

class TrimmedSeamOffsetControls final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& name,const QJsonObject& args={}) {
        const auto response=window.executeApi(name,args);
        if(!response.value("ok").toBool())qFatal("%s: %s",qPrintable(name),QJsonDocument(response).toJson().constData());
        return response.value("result").toObject();
    }
    QJsonObject session(AlloyWindow& window){return window.state().value("activeSession").toObject();}
private slots:
    void selectedSeamInsetAndCancel() {
        const auto source=qEnvironmentVariable("ALLOY_TRIMMED_SEAM_SOURCE");
        const auto out=qEnvironmentVariable("ALLOY_TRIMMED_SEAM_EVIDENCE");
        QVERIFY(!source.isEmpty() && !out.isEmpty());QVERIFY(QDir().mkpath(out));
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1500,950);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        ReferenceView* view=nullptr;for(auto* child:window.findChildren<QWidget*>())
            if(auto* found=dynamic_cast<ReferenceView*>(child))view=found;
        QVERIFY(view);
        const auto opened=call(window,"document.open",{{"path",source}});
        Q_UNUSED(opened);
        const auto inspected=call(window,"document.inspect");
        const auto bodies=inspected.value("evaluation").toObject().value("bodies").toArray();
        QJsonObject body,face;
        for(const auto& item:bodies)for(const auto& support:item.toObject().value("supportFaces").toArray())
            if(support.toObject().value("role")=="split.negative.cap") {
                body=item.toObject();face=support.toObject();
            }
        QVERIFY(!body.isEmpty() && !face.isEmpty());
        QString owner;
        for(const auto& item:call(window,"component.inspect").value("bodies").toArray())
            if(item.toObject().value("bodyId")==body.value("bodyId"))owner=item.toObject().value("ownerId").toString();
        QVERIFY(!owner.isEmpty());call(window,"component.activate",{{"definitionId",owner}});
        const QJsonObject support{{"producerFeatureId",face.value("producerFeatureId")},
            {"bodyId",face.value("bodyId")},{"faceId",face.value("faceId")}};
        call(window,"sketch.begin",{{"support",support},{"name","Trimmed seam inset"},{"units","mm"}});
        call(window,"sketch.lookAt");call(window,"sketch.tool",{{"tool","offset"}});
        auto* field=window.findChild<QLineEdit*>("sketch.offset.distance");
        auto* chain=window.findChild<QCheckBox*>("sketch.offset.chain");
        QVERIFY(field && field->isVisible() && chain && chain->isChecked());
        const auto before=session(window).value("draftSketch").toObject();
        QSet<QString> faceEdgeIds;
        for(const auto& item:face.value("edges").toArray())faceEdgeIds.insert(item.toObject().value("edgeId").toString());
        QVERIFY(faceEdgeIds.size()==18);
        QJsonObject selectedEdge;QPoint pick;
        for(const auto& item:body.value("solidEdges").toArray()) {
            const auto edge=item.toObject();
            if(!faceEdgeIds.contains(edge.value("edgeId").toString()))continue;
            const auto response=window.executeApi("view.projectEdge",
                {{"bodyId",body.value("bodyId")},{"edgeId",edge.value("edgeId")}});
            if(!response.value("ok").toBool())continue;
            const auto pixel=response.value("result").toObject();
            const QPoint candidate(qRound(pixel.value("x").toDouble()),qRound(pixel.value("y").toDouble()));
            if(!view->rect().contains(candidate) || QApplication::widgetAt(view->mapToGlobal(candidate))!=view)continue;
            const auto hit=window.executeApi("selection.edgePick",{{"x",candidate.x()},{"y",candidate.y()}});
            if(!hit.value("ok").toBool() || hit.value("result").toObject().value("edgeId")!=edge.value("edgeId"))continue;
            selectedEdge=edge;pick=candidate;break;
        }
        QVERIFY2(!selectedEdge.isEmpty(),"no retained source edge has an unambiguous visible viewport hit");
        // Initial app activation occurs after API-only fixture setup and before
        // the demonstrated native edge/field/Escape/Return sequence.
        window.raise();window.activateWindow();window.windowHandle()->requestActivate();
        QTRY_VERIFY(window.isActiveWindow());
        QCOMPARE(QApplication::widgetAt(view->mapToGlobal(pick)),static_cast<QWidget*>(view));
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pick);
        const auto afterClick=session(window);
        QVERIFY2(afterClick.value("pendingLoopOffset").toObject().value("valid").toBool(),
            "native viewport click did not create the required pending inset");
        QVERIFY(window.findChild<QLabel*>("sketch.offset.title")->text().contains("Interior inset"));
        QVERIFY(window.findChild<QLabel*>("sketch.info")->text().contains("Interior inset preview"));
        call(window,"capture.screenshot",{{"path",out+"/after-first-click.png"}});
        QTRY_VERIFY(session(window).value("pendingLoopOffset").toObject().value("valid").toBool());
        QCOMPARE(session(window).value("pendingLoopOffset").toObject().value("kind"),QJsonValue("planarInsetV1"));
        QCOMPARE(session(window).value("draftSketch"),QJsonValue(before));
        QTest::mouseClick(field,Qt::LeftButton);
        qInfo()<<"fieldFocus"<<QApplication::focusWidget()<<"active"<<window.isActiveWindow()
            <<"fieldVisible"<<field->isVisible()<<"fieldEnabled"<<field->isEnabled();
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(field));
        QTest::keyClick(field,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(field,"-1.25");
        qInfo()<<"inset field entered";
        QTRY_VERIFY(session(window).value("pendingLoopOffset").toObject().value("valid").toBool());
        const auto preview=session(window).value("pendingLoopOffset").toObject();
        QCOMPARE(preview.value("distanceMm").toDouble(),-1.25);
        const auto geometry=preview.value("previewGeometry").toObject();
        QVERIFY(std::abs(geometry.value("profiles").toArray().first().toObject().value("areaMm2").toDouble()
            -233.8576962999306)<1e-5);
        call(window,"capture.screenshot",{{"path",out+"/inset-preview.png"}});
        qInfo()<<"inset preview captured";
        const auto invalid=window.executeApi("sketch.loopOffsetPreview",{{"flip",true}});
        QVERIFY(!invalid.value("ok").toBool());
        QVERIFY(!session(window).value("pendingLoopOffset").toObject().value("valid").toBool());
        QVERIFY(!window.executeApi("sketch.loopOffsetAccept").value("ok").toBool());
        qInfo()<<"invalid side refused";
        QCOMPARE(session(window).value("draftSketch"),QJsonValue(before));
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pick);
        QTRY_VERIFY(session(window).value("pendingLoopOffset").toObject().value("valid").toBool());
        QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QTest::keyClick(view,Qt::Key_Escape);
        QTRY_VERIFY(!session(window).contains("pendingLoopOffset"));
        qInfo()<<"native Escape canceled preview";
        QCOMPARE(session(window).value("draftSketch"),QJsonValue(before));
        call(window,"sketch.tool",{{"tool","offset"}});
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pick);
        QTRY_VERIFY(session(window).value("pendingLoopOffset").toObject().value("valid").toBool());
        QTest::mouseClick(field,Qt::LeftButton);QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(field));
        QTest::keyClick(field,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(field,"-1.25");
        QTRY_VERIFY(session(window).value("pendingLoopOffset").toObject().value("valid").toBool());
        QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(field));QTest::keyClick(field,Qt::Key_Return);
        QTRY_VERIFY(!session(window).contains("pendingLoopOffset"));
        qInfo()<<"native Return accepted inset";
        const auto authored=session(window).value("draftSketch").toObject();
        QCOMPARE(authored.value("parameters").toObject().value("loopOffsets").toArray().first().toObject().value("kind"),
            QJsonValue("planarInsetV1"));
        call(window,"capture.screenshot",{{"path",out+"/inset-accepted.png"}});
        // Camera-only second view exposes the actual split seam and port while
        // the top-view capture remains the native inset-input evidence.
        call(window,"view.orientation",{{"name","rearIsometric"}});
        call(window,"capture.screenshot",{{"path",out+"/inset-rear-isometric.png"}});
        call(window,"sketch.finish");
        call(window,"document.save",{{"path",out+"/retained-inset.alloy"}});
        window.close();
    }
};
QTEST_MAIN(TrimmedSeamOffsetControls)
#include "trimmed_seam_offset_controls.moc"
