// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QLabel>
#include <QProcess>
#include <QPushButton>
#include <QScrollArea>
#include <QTemporaryDir>
#include <QTest>
#include <QWindow>

class SketchConstraintPairFeedbackControls final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& w,const QString& command,const QJsonObject& args={}){
        auto response=w.executeApi(command,args);
        const auto initial=response["result"].toObject();QElapsedTimer timer;timer.start();
        while(response["completion"]=="pending"){
            if(timer.elapsed()>30000)qFatal("Timed out waiting for %s",qPrintable(command));
            QTest::qWait(10);response=w.executeApi("command.status",{{"session",initial["id"]},{"generation",initial["previewGeneration"]}});
        }
        if(!response["ok"].toBool())qFatal("%s",QJsonDocument(response).toJson(QJsonDocument::Compact).constData());
        return response["result"].toObject();
    }
    QJsonObject session(AlloyWindow& w){return w.state()["activeSession"].toObject();}
private slots:
    void recoverablePairsAndRetainedBadge(){
        AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1500,950);w.show();QVERIFY(QTest::qWaitForWindowExposed(&w));
        ReferenceView* view=nullptr;for(auto* widget:w.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(widget))view=candidate;
        QVERIFY(view);
        call(w,"sketch.begin",{{"plane","XY"},{"units","mm"},{"name","Constraint eligibility"}});
        auto last=[&]{return session(w)["draftSketch"].toObject()["parameters"].toObject()["entities"].toArray().last().toObject().value("id");};
        call(w,"sketch.line",{{"x1",-40},{"y1",-15},{"x2",-40},{"y2",15},{"units","mm"}});const auto line=last();
        call(w,"sketch.line",{{"x1",-25},{"y1",-15},{"x2",-25},{"y2",15},{"units","mm"}});const auto otherLine=last();
        call(w,"sketch.addCircle",{{"centerX",0},{"centerY",0},{"diameter",16},{"units","mm"}});const auto circle=last();
        call(w,"sketch.addArc",{{"mode","center"},{"centerX",30},{"centerY",0},{"radius",8},{"startAngleDeg",-90},{"sweepAngleDeg",180},{"units","mm"}});const auto arc=last();
        call(w,"sketch.addArc",{{"mode","center"},{"centerX",50},{"centerY",0},{"radius",8},{"startAngleDeg",-90},{"sweepAngleDeg",180},{"units","mm"}});const auto otherArc=last();
        call(w,"sketch.lookAt");
#ifdef Q_OS_MACOS
        QProcess activation;activation.start("osascript",{"-e",QString("tell application \"System Events\" to set frontmost of first application process whose unix id is %1 to true").arg(QCoreApplication::applicationPid())});
        QVERIFY(activation.waitForFinished(5000));QCOMPARE(activation.exitCode(),0);
#endif
        w.raise();w.activateWindow();w.windowHandle()->requestActivate();QApplication::setActiveWindow(&w);QTRY_VERIFY(w.isActiveWindow());
        auto pixel=[&](double u,double v){const auto p=call(w,"view.projectSketchPoint",{{"sketchId",session(w)["draftSketch"].toObject()["id"]},{"uMm",u},{"vMm",v}});return QPoint(qRound(p["x"].toDouble()),qRound(p["y"].toDouble()));};
        auto click=[&](const QPoint& p){QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,p);};
        auto activate=[&](const QString& type){
            auto* button=w.findChild<QPushButton*>("sketch.constraint."+type);QVERIFY(button);
            for(auto* area:w.findChildren<QScrollArea*>())if(area->isAncestorOf(button))area->ensureWidgetVisible(button);
            QTest::mouseClick(button,Qt::LeftButton);QCOMPARE(session(w)["constraintType"].toString(),type);
            QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
        };
        const auto lp=pixel(-40,0),op=pixel(-25,0),cp=pixel(0,8),ap=pixel(38,0),bp=pixel(58,0);
        struct Row {QString type;QJsonValue first,second;QPoint firstPixel,secondPixel;QString status;};
        const QList<Row> rows{
            {"concentric",circle,line,cp,lp,"wrong_type"},
            {"collinear",line,circle,lp,cp,"wrong_type"},
            {"symmetry",line,arc,lp,ap,"unsupported_pair"},
            {"symmetry",arc,circle,ap,cp,"unsupported_pair"},
            {"tangent",line,otherLine,lp,op,"unsupported_pair"},
            {"tangent",arc,circle,ap,cp,"unsupported_pair"},
            {"tangent",circle,arc,cp,ap,"unsupported_pair"},
            {"tangent",arc,otherArc,ap,bp,"unsupported_pair"}};
        QJsonArray receipt;
        const auto evidence=qEnvironmentVariable("ALLOY_CONSTRAINT_PAIR_EVIDENCE");if(!evidence.isEmpty())QDir().mkpath(evidence);
        for(const auto& row:rows){
            call(w,"sketch.tool",{{"tool","select"}});call(w,"sketch.selectionSet");activate(row.type);
            QCOMPARE(session(w)["constraintPickStatus"].toString(),QString("pending_first"));
            click(row.firstPixel);QCOMPARE(session(w)["selection"].toObject()["entityIds"].toArray(),QJsonArray{row.first});
            QCOMPARE(session(w)["constraintPickStatus"].toString(),QString("pending_next"));
            const auto before=session(w),document=call(w,"document.inspect");const auto viewport=w.state()["viewport"].toObject();
            click(row.secondPixel);
            QCOMPARE(session(w)["selection"],before["selection"]);QCOMPARE(session(w)["draftSketch"],before["draftSketch"]);
            QCOMPARE(session(w)["draftHistory"],before["draftHistory"]);QCOMPARE(session(w)["solverDiagnostics"],before["solverDiagnostics"]);
            QCOMPARE(call(w,"document.inspect"),document);
            QCOMPARE(w.state()["viewport"].toObject()["gpu"].toObject()["sketchConstraintGlyphs"],viewport["gpu"].toObject()["sketchConstraintGlyphs"]);
            QCOMPARE(session(w)["constraintPickStatus"].toString(),row.status);
            const auto prompt=session(w)["constraintPrompt"].toString();QVERIFY(prompt.contains(row.status=="wrong_type" ? "Wrong type" : "Unsupported pair"));
            auto* info=w.findChild<QLabel*>("sketch.info");QVERIFY(info);QVERIFY(info->text().contains(prompt));
            auto* error=w.findChild<QLabel*>("sketch.error");QVERIFY(error);QVERIFY(error->isVisible());
            QVERIFY(error->text().contains(row.status=="wrong_type" ? "Wrong type" : "Unsupported pair"));
            QTRY_VERIFY(error->height()>=error->heightForWidth(error->contentsRect().width()));
            QVERIFY(w.rect().contains(QRect(error->mapTo(&w,QPoint{}),error->size())));
            QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
            receipt.append(QJsonObject{{"type",row.type},{"first",row.first},{"second",row.second},{"status",row.status},{"prompt",prompt},{"unchanged",true}});
            if(row.type=="tangent" && row.first==arc && row.second==circle && !evidence.isEmpty())call(w,"capture.screenshot",{{"path",evidence+"/native-unsupported-pair.png"}});
        }
        // Concentric recovers from the refused line without repicking the circle.
        call(w,"sketch.tool",{{"tool","select"}});call(w,"sketch.selectionSet");activate("concentric");click(cp);click(lp);
        QCOMPARE(session(w)["selection"].toObject()["entityIds"].toArray(),QJsonArray{circle});click(ap);
        auto constraints=session(w)["draftSketch"].toObject()["parameters"].toObject()["constraints"].toArray();QCOMPARE(constraints.size(),1);
        const auto relation=constraints[0].toObject();QCOMPARE(relation["type"].toString(),QString("concentric"));QCOMPARE(session(w)["constraintPickStatus"].toString(),QString("applied"));
        auto glyphs=w.state()["viewport"].toObject()["gpu"].toObject()["sketchConstraintGlyphs"].toArray();QJsonObject badge;
        for(const auto& value:glyphs)if(value.toObject()["constraintId"]==relation["id"])badge=value.toObject();QVERIFY(!badge.isEmpty());
        if(!evidence.isEmpty())call(w,"capture.screenshot",{{"path",evidence+"/native-recovered-concentric.png"}});
        QTest::keyClick(view,Qt::Key_Escape);QVERIFY(!session(w).contains("constraintPickStatus"));
        const auto bounds=badge["bounds"].toObject();click(QPoint(qRound(bounds["x"].toDouble()+bounds["width"].toDouble()/2),qRound(bounds["y"].toDouble()+bounds["height"].toDouble()/2)));
        QCOMPARE(session(w)["selection"].toObject()["constraintIds"].toArray(),QJsonArray{relation["id"]});
        QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QTest::keyClick(view,Qt::Key_Delete);
        QCOMPARE(session(w)["draftSketch"].toObject()["parameters"].toObject()["constraints"].toArray().size(),0);
        call(w,"history.undo");QCOMPARE(session(w)["draftSketch"].toObject()["parameters"].toObject()["constraints"].toArray(),constraints);
        call(w,"history.redo");QCOMPARE(session(w)["draftSketch"].toObject()["parameters"].toObject()["constraints"].toArray().size(),0);
        call(w,"history.undo");const auto draft=session(w)["draftSketch"].toObject();call(w,"sketch.finish");
        QTemporaryDir temp;QVERIFY(temp.isValid());const auto path=evidence.isEmpty() ? temp.path()+"/recovered.alloy" : evidence+"/recovered.alloy";
        call(w,"document.save",{{"path",path}});const auto authored=call(w,"document.inspect").value("authored");
        call(w,"document.new",{{"discard",false}});call(w,"document.open",{{"path",path}});QCOMPARE(call(w,"document.inspect")["authored"],authored);
        call(w,"sketch.edit",{{"sketchId",draft["id"]}});QCOMPARE(session(w)["draftSketch"],draft);
        if(!evidence.isEmpty()){QFile file(evidence+"/native-receipt.json");QVERIFY(file.open(QIODevice::WriteOnly));file.write(QJsonDocument(QJsonObject{{"refusals",receipt},{"gpu",w.state()["viewport"].toObject()["gpu"]},{"relation",relation},{"authored",authored},{"badgeDeleteUndoRedoReopen",true}}).toJson());}
        call(w,"sketch.cancel");w.close();
    }
};
QTEST_MAIN(SketchConstraintPairFeedbackControls)
#include "sketch_constraint_pair_feedback_controls.moc"
