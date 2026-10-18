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

class LinkedProjectionDeleteControls final:public QObject {
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
    void linkedProjectionDelete() {
        const auto design=qEnvironmentVariable("ALLOY_PROJECTION_DELETE_DESIGN");if(design.isEmpty())QSKIP("Set projection fixture");
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1500,950);window.show();QVERIFY(QTest::qWaitForWindowExposed(&window));
#ifdef Q_OS_MACOS
        QProcess activation;activation.start("osascript",{"-e",QString("tell application \"System Events\" to set frontmost of first application process whose unix id is %1 to true").arg(QCoreApplication::applicationPid())});QVERIFY(activation.waitForFinished(5000));QCOMPARE(activation.exitCode(),0);
#endif
        window.raise();window.activateWindow();window.windowHandle()->requestActivate();QApplication::setActiveWindow(&window);QTRY_VERIFY(window.isActiveWindow());
        ReferenceView* view=nullptr;for(auto* widget:window.findChildren<QWidget*>())if(auto* c=dynamic_cast<ReferenceView*>(widget))view=c;QVERIFY(view);
        call(window,"document.open",{{"path",design}});QJsonObject sketch;
        for(const auto& row:call(window,"document.inspect").value("authored").toObject().value("features").toArray())if(row.toObject().value("type")=="sketch")sketch=row.toObject();
        QVERIFY(!sketch.isEmpty());call(window,"sketch.edit",{{"sketchId",sketch.value("id")}});call(window,"sketch.lookAt");call(window,"view.fit");
        const auto session=[&]{return window.state().value("activeSession").toObject();};const auto draft=[&]{return session().value("draftSketch").toObject();};
        const auto original=draft(),p=original.value("parameters").toObject(),context=session().value("projectionContext").toObject();
        const auto line=context.value("lines").toArray()[0].toObject(),circle=context.value("circles").toArray()[0].toObject();
        const auto point=p.value("pointProjections").toArray()[0].toObject().value("id");
        const auto pixel=[&](double u,double v){const auto pos=call(window,"view.projectSketchPoint",{{"sketchId",sketch.value("id")},{"uMm",u},{"vMm",v}});return QPoint(qRound(pos.value("x").toDouble()),qRound(pos.value("y").toDouble()));};
        const auto evidence=qEnvironmentVariable("ALLOY_PROJECTION_DELETE_EVIDENCE");QDir().mkpath(evidence);
        QTRY_VERIFY(view->hasFocus());QTest::qWait(200);
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel((line.value("u1Mm").toDouble()+line.value("u2Mm").toDouble())/2,(line.value("v1Mm").toDouble()+line.value("v2Mm").toDouble())/2));
        QCOMPARE(session().value("selection").toObject().value("entityIds").toArray(),QJsonArray{line.value("id")});QVERIFY(view->hasFocus());
        call(window,"capture.screenshot",{{"path",evidence+"/native-line-selected.png"}});QTest::keyClick(view,Qt::Key_Delete);
        QTRY_COMPARE(draft().value("parameters").toObject().value("projections").toArray().size(),3);const auto lineDeleted=draft();
        auto* status=window.findChild<QLabel*>("sketch.profileStatus");auto* guidance=window.findChild<QLabel*>("sketch.selectionGuidance");QVERIFY(status&&guidance);
        const auto profileTextFits=[&] {const int textHeight=status->fontMetrics().boundingRect(QRect(0,0,status->contentsRect().width(),100000),Qt::TextWordWrap,status->text()).height();return status->height()>=textHeight && status->geometry().bottom()<guidance->geometry().top();};
        QVERIFY2(profileTextFits(),"Deleted projection diagnostics must fit above selection guidance in the same refresh");
        call(window,"capture.screenshot",{{"path",evidence+"/native-line-deleted.png"}});call(window,"history.undo");QCOMPARE(draft(),original);QVERIFY(profileTextFits());call(window,"capture.screenshot",{{"path",evidence+"/native-profile-restored.png"}});call(window,"history.redo");QCOMPARE(draft(),lineDeleted);call(window,"history.undo");QCOMPARE(draft(),original);
        const auto rim=pixel(circle.value("uMm").toDouble()+circle.value("radiusMm").toDouble(),circle.value("vMm").toDouble());
        QContextMenuEvent contextEvent(QContextMenuEvent::Mouse,rim,view->mapToGlobal(rim));QApplication::sendEvent(view,&contextEvent);
        auto* menu=view->findChild<QMenu*>("context.sketchGeometry");QVERIFY(menu);QTRY_COMPARE(QApplication::activePopupWidget(),menu);
        QCOMPARE(session().value("selection").toObject().value("entityIds").toArray(),QJsonArray{circle.value("id")});
        auto* remove=menu->findChild<QAction*>("sketch.action.deleteGeometry");QVERIFY(remove);QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,menu->actionGeometry(remove).center());
        QTRY_VERIFY(QApplication::activePopupWidget()==nullptr);QTRY_VERIFY(!draft().value("parameters").toObject().contains("circleProjections"));
        call(window,"capture.screenshot",{{"path",evidence+"/native-circle-deleted.png"}});call(window,"history.undo");QCOMPARE(draft(),original);
        auto* tree=window.findChild<QTreeWidget*>("sketch.selection");QVERIFY(tree);QTreeWidgetItem* row=nullptr;
        for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole+1).toString()==point.toString())row=*it;
        QVERIFY(row);QVERIFY(!row->isHidden());tree->scrollToItem(row);const auto rowPoint=tree->visualItemRect(row).center();QVERIFY(tree->viewport()->childAt(rowPoint)==nullptr);
        QTest::mouseClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,rowPoint);QCOMPARE(session().value("selection").toObject().value("pointIds").toArray(),QJsonArray{point});QVERIFY(tree->hasFocus());
        call(window,"capture.screenshot",{{"path",evidence+"/native-point-list-selected.png"}});QTest::keyClick(tree,Qt::Key_Backspace);QTRY_VERIFY(!draft().value("parameters").toObject().contains("pointProjections"));
        const auto changed=draft();call(window,"history.undo");QCOMPARE(draft(),original);call(window,"history.redo");QCOMPARE(draft(),changed);
        call(window,"sketch.finish");const auto authored=call(window,"document.inspect").value("authored");call(window,"document.save",{{"path",evidence+"/native-deleted.alloy"}});call(window,"document.open",{{"path",evidence+"/native-deleted.alloy"},{"discard",true}});QCOMPARE(call(window,"document.inspect").value("authored"),authored);
        call(window,"capture.screenshot",{{"path",evidence+"/native-reopened.png"}});const auto gpu=window.state().value("viewport").toObject().value("gpu").toObject();QVERIFY(gpu.value("hardwareAccelerated").toBool());
        QFile receipt(evidence+"/receipt.json");QVERIFY(receipt.open(QIODevice::WriteOnly));receipt.write(QJsonDocument(QJsonObject{{"gpu",gpu},{"original",original},{"deleted",changed},{"canvasLineDelete",true},{"contextCircleDelete",true},{"listPointBackspace",true},{"exactUndoRedoReopen",true},{"immediateProfileGuidanceSeparation",true}}).toJson());
        call(window,"document.new",{{"discard",true}});call(window,"sketch.begin",{{"plane","XY"},{"name","Ordinary profile palette"}});call(window,"sketch.addCircle",{{"centerX",0},{"centerY",0},{"diameter",20},{"units","mm"}});
        auto* ordinaryStatus=window.findChild<QLabel*>("sketch.profileStatus");auto* ordinaryGuidance=window.findChild<QLabel*>("sketch.selectionGuidance");QVERIFY(ordinaryStatus&&ordinaryGuidance);
        QVERIFY(ordinaryStatus->text().contains("1 selectable region"));QVERIFY(ordinaryStatus->geometry().bottom()<ordinaryGuidance->geometry().top());
        call(window,"capture.screenshot",{{"path",evidence+"/native-ordinary-profile.png"}});call(window,"sketch.cancel");call(window,"document.new",{{"discard",true}});window.close();
    }
};
int main(int argc,char** argv){QApplication app(argc,argv);LinkedProjectionDeleteControls test;return QTest::qExec(&test,argc,argv);}
#include "linked_projection_delete_controls.moc"
