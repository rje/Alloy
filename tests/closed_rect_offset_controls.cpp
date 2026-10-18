// SPDX-License-Identifier: MIT
#include "AlloyTheme.h"
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QAction>
#include <QContextMenuEvent>
#include <QFile>
#include <QElapsedTimer>
#include <QMenu>
#include <QProcess>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QWindow>
#include <QCheckBox>
#include <QDir>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QTest>

class ClosedRectOffsetControls final:public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& name,const QJsonObject& args={}) {
        auto response=window.execute(name,args);
        const auto initial=response.value("result").toObject();QElapsedTimer timer;timer.start();
        while(response.value("completion")=="pending") {
            if(timer.elapsed()>30000)qFatal("Command timed out: %s",qPrintable(name));
            QTest::qWait(20);response=window.execute("command.status",{{"session",initial.value("id")},{"generation",initial.value("previewGeneration")}});
        }
        if(!response["ok"].toBool())qFatal("%s: %s",qPrintable(name),QJsonDocument(response).toJson().constData());
        return response["result"].toObject();
    }
private slots:
    void nativeRectangleOwnerDelete() {
        const auto design=qEnvironmentVariable("ALLOY_RECT_DELETE_DESIGN");
        if(design.isEmpty())QSKIP("Rectangle Delete fixture unavailable");
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1500,950);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
#ifdef Q_OS_MACOS
        QProcess activation;activation.start("osascript",{"-e",QString("tell application \"System Events\" to set frontmost of first application process whose unix id is %1 to true").arg(QCoreApplication::applicationPid())});
        QVERIFY(activation.waitForFinished(5000));QCOMPARE(activation.exitCode(),0);
#endif
        window.raise();window.activateWindow();window.windowHandle()->requestActivate();QApplication::setActiveWindow(&window);QTRY_VERIFY(window.isActiveWindow());
        ReferenceView* view=nullptr;for(auto* widget:window.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(widget))view=candidate;QVERIFY(view);
        call(window,"document.open",{{"path",design}});
        QJsonObject sketch;for(const auto& row:call(window,"document.inspect").value("authored").toObject().value("features").toArray())if(row.toObject().value("type")=="sketch")sketch=row.toObject();
        QVERIFY(!sketch.isEmpty());call(window,"sketch.edit",{{"sketchId",sketch.value("id")}});call(window,"sketch.lookAt");call(window,"view.fit");
        const auto session=[&]{return window.state().value("activeSession").toObject();};
        const auto draft=[&]{return session().value("draftSketch").toObject();};
        const auto original=draft(),p=original.value("parameters").toObject();const auto groups=p.value("closedRectOffsets").toArray();QCOMPARE(groups.size(),2);
        const auto first=groups[0].toObject(),second=groups[1].toObject();
        const auto pixel=[&](const QJsonObject& group){
            const auto ids=group.value("pointIds").toArray();QJsonObject a,b;
            for(const auto& row:p.value("points").toArray()){const auto point=row.toObject();if(point.value("id")==ids[0])a=point;if(point.value("id")==ids[1])b=point;}
            const auto projected=call(window,"view.projectSketchPoint",{{"sketchId",sketch.value("id")},{"uMm",(a.value("uMm").toDouble()+b.value("uMm").toDouble())/2},{"vMm",(a.value("vMm").toDouble()+b.value("vMm").toDouble())/2}});
            return QPoint(qRound(projected.value("x").toDouble()),qRound(projected.value("y").toDouble()));
        };
        const auto evidence=qEnvironmentVariable("ALLOY_RECT_DELETE_EVIDENCE");QDir().mkpath(evidence);
        // Initial application activation is above. These steps observe the
        // focus and stable selection produced by the real receiving widget.
        QTRY_VERIFY(view->hasFocus());QTest::qWait(200);QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(first));
        QCOMPARE(session().value("selection").toObject().value("entityIds").toArray(),QJsonArray{first.value("entityIds").toArray()[0]});QVERIFY(view->hasFocus());
        call(window,"capture.screenshot",{{"path",evidence+"/native-canvas-selected.png"}});QTest::keyClick(view,Qt::Key_Delete);
        QTRY_COMPARE(draft().value("parameters").toObject().value("closedRectOffsets").toArray(),QJsonArray{second});const auto changed=draft();
        call(window,"history.undo");QCOMPARE(draft(),original);call(window,"history.redo");QCOMPARE(draft(),changed);
        // Context Delete on the second owner follows the remaining visible edge.
        const auto pick=pixel(second);QContextMenuEvent context(QContextMenuEvent::Mouse,pick,view->mapToGlobal(pick));QApplication::sendEvent(view,&context);
        auto* menu=view->findChild<QMenu*>("context.sketchGeometry");QVERIFY(menu);QTRY_COMPARE(QApplication::activePopupWidget(),menu);
        QCOMPARE(session().value("selection").toObject().value("entityIds").toArray(),QJsonArray{second.value("entityIds").toArray()[0]});
        auto* remove=menu->findChild<QAction*>("sketch.action.deleteGeometry");QVERIFY(remove);QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,menu->actionGeometry(remove).center());
        QTRY_VERIFY(QApplication::activePopupWidget()==nullptr);QTRY_VERIFY(!draft().value("parameters").toObject().contains("closedRectOffsets"));
        call(window,"history.undo");QCOMPARE(draft(),changed);
        auto* tree=window.findChild<QTreeWidget*>("sketch.selection");QVERIFY(tree);QTreeWidgetItem* row=nullptr;
        const auto target=second.value("entityIds").toArray().at(1);
        for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole+1).toString()==target.toString())row=*it;
        QVERIFY(row);tree->scrollToItem(row);const auto rowPoint=tree->visualItemRect(row).center();
        auto* receiver=tree->viewport()->childAt(rowPoint);QVERIFY(receiver==nullptr);QTest::mouseClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,rowPoint);
        QCOMPARE(session().value("selection").toObject().value("entityIds").toArray(),QJsonArray{target});QVERIFY(tree->hasFocus());QTest::keyClick(tree,Qt::Key_Delete);
        QTRY_VERIFY(!draft().value("parameters").toObject().contains("closedRectOffsets"));
        for(const auto& projection:draft().value("parameters").toObject().value("projections").toArray())QVERIFY(projection.toObject().value("profile").toBool());
        call(window,"capture.screenshot",{{"path",evidence+"/native-list-deleted.png"}});
        call(window,"history.undo");QCOMPARE(draft(),changed);call(window,"history.redo");const auto final=draft();
        call(window,"sketch.finish");const auto authored=call(window,"document.inspect").value("authored");
        call(window,"document.save",{{"path",evidence+"/native-deleted.alloy"}});call(window,"document.open",{{"path",evidence+"/native-deleted.alloy"},{"discard",true}});QCOMPARE(call(window,"document.inspect").value("authored"),authored);
        const auto gpu=window.state().value("viewport").toObject().value("gpu").toObject();QVERIFY(gpu.value("hardwareAccelerated").toBool());
        call(window,"capture.screenshot",{{"path",evidence+"/native-reopened.png"}});
        QFile receipt(evidence+"/native-receipt.json");QVERIFY(receipt.open(QIODevice::WriteOnly));receipt.write(QJsonDocument(QJsonObject{{"gpu",gpu},{"original",original},{"afterFirstDelete",changed},{"afterBothDelete",final},{"canvasListContextDelete",true},{"exactUndoRedoReopen",true}}).toJson());window.close();
    }
    void nativeProjectedRectangleOffset() {
        const auto design=qEnvironmentVariable("ALLOY_CLOSED_RECT_DESIGN");
        if(design.isEmpty())QSKIP("Actual projected connector design unavailable");
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1500,950);window.show();window.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&window));QApplication::setActiveWindow(&window);QTRY_VERIFY(window.isActiveWindow());
        ReferenceView* view=nullptr;for(auto* widget:window.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(widget))view=candidate;
        QVERIFY(view);
        call(window,"document.open",{{"path",design}});
        const auto features=call(window,"document.inspect")["authored"].toObject()["features"].toArray();
        QString sketchId;for(const auto& value:features)if(value.toObject()["name"]=="Three linked port clearances")sketchId=value.toObject()["id"].toString();
        QVERIFY(!sketchId.isEmpty());call(window,"sketch.edit",{{"sketchId",sketchId}});call(window,"sketch.lookAt");
        const auto original=window.state()["activeSession"].toObject()["draftSketch"].toObject();
        const auto context=window.state()["activeSession"].toObject()["projectionContext"].toObject();
        const auto lines=context["lines"].toArray();QCOMPARE(lines.size(),12);
        const auto line=lines.first().toObject();const double u=(line["u1Mm"].toDouble()+line["u2Mm"].toDouble())/2;
        const double v=(line["v1Mm"].toDouble()+line["v2Mm"].toDouble())/2;
        const auto pixel=call(window,"view.projectSketchPoint",{{"sketchId",sketchId},{"uMm",u},{"vMm",v}});
        const QPoint point(qRound(pixel["x"].toDouble()),qRound(pixel["y"].toDouble()));QVERIFY(view->rect().contains(point));
        call(window,"sketch.tool",{{"tool","offset"}});QTRY_VERIFY(view->hasFocus());
        auto* chain=window.findChild<QCheckBox*>("sketch.offset.chain");QVERIFY(chain&&chain->isVisible()&&chain->isChecked());
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,point);
        QTRY_VERIFY(window.state()["activeSession"].toObject()["pendingClosedRectOffset"].toObject()["valid"].toBool());
        QCOMPARE(window.state()["activeSession"].toObject()["draftSketch"].toObject(),original);
        auto* field=window.findChild<QLineEdit*>("sketch.offset.distance");QVERIFY(field&&field->isVisible());
        QTest::mouseClick(field,Qt::LeftButton);QVERIFY(field->hasFocus());field->selectAll();QTest::keyClicks(field,"1");
        QTRY_VERIFY(window.state()["activeSession"].toObject()["pendingClosedRectOffset"].toObject()["valid"].toBool());
        const auto pending=window.state()["activeSession"].toObject()["pendingClosedRectOffset"].toObject();
        QCOMPARE(pending["sourceEntityIds"].toArray().size(),4);QCOMPARE(pending["distanceMm"].toDouble(),1.0);
        auto* title=window.findChild<QLabel*>("sketch.offset.title");QVERIFY(title&&title->isVisible()&&title->text().contains("rectangle",Qt::CaseInsensitive));
        const auto evidence=qEnvironmentVariable("ALLOY_CLOSED_RECT_EVIDENCE");if(!evidence.isEmpty()){
            QDir().mkpath(evidence);call(window,"capture.screenshot",{{"path",evidence+"/native-rectangle-offset-preview.png"}});
        }
        QTest::keyClick(field,Qt::Key_Escape);QTRY_VERIFY(!window.state()["activeSession"].toObject().contains("pendingClosedRectOffset"));
        QCOMPARE(window.state()["activeSession"].toObject()["draftSketch"].toObject(),original);
        call(window,"sketch.cancel");window.close();
    }
};
QTEST_MAIN(ClosedRectOffsetControls)
#include "closed_rect_offset_controls.moc"
