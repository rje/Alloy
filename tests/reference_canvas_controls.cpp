// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include "AlloyTheme.h"
#include <QTest>
#include <QApplication>
#include <QTemporaryDir>
#include <QImage>
#include <QPainter>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QWindow>
#include <QTreeWidget>
#include <QProcess>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
class CanvasControlsTest final:public QObject {
 Q_OBJECT
 QJsonObject call(AlloyWindow& w,const QString& command,const QJsonObject& args={}){auto r=w.execute(command,args);if(!r["ok"].toBool())qFatal("%s",QJsonDocument(r).toJson().constData());return r["result"].toObject();}
 QJsonObject session(AlloyWindow& w){return w.state()["activeSession"].toObject();}
 QJsonObject authored(AlloyWindow& w){return call(w,"document.inspect")["authored"].toObject();}
 void capture(AlloyWindow& w,const QString& name){const auto out=qEnvironmentVariable("ALLOY_CANVAS_EVIDENCE");if(out.isEmpty())return;QDir().mkpath(out);call(w,"capture.screenshot",{{"path",out+"/"+name+".png"}});QFile f(out+"/"+name+".json");f.open(QIODevice::WriteOnly);f.write(QJsonDocument(w.state()).toJson());}
 void type(QDoubleSpinBox* input,const QString& value){QTest::mouseClick(input,Qt::LeftButton);QTRY_VERIFY(input->hasFocus());QTest::keyClick(QApplication::focusWidget(),Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(QApplication::focusWidget(),value);QTest::keyClick(QApplication::focusWidget(),Qt::Key_Tab);}
private slots:
 void publicPreviewOmitsDuplicateImagePayloads(){
    AlloyTheme::apply(*qApp);AlloyWindow w;QTemporaryDir temp;
    QImage image(512,192,QImage::Format_RGB32);quint32 seed=0x463ab823;
    for(int y=0;y<image.height();++y)for(int x=0;x<image.width();++x){
        seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;image.setPixel(x,y,0xff000000u|(seed&0xffffffu));
    }
    const auto imagePath=temp.filePath("independent-noise.png");QVERIFY(image.save(imagePath));
    for(const auto& plane:{"XY","XZ"})call(w,"canvas.set",{{"path",imagePath},{"planeSource",QJsonObject{{"plane",plane}}}});
    call(w,"solid.box",{{"width",20},{"depth",30},{"height",10}});
    const auto before=authored(w);const auto body=call(w,"document.inspect")["evaluation"].toObject()["bodies"].toArray()[0].toObject();
    const auto face=body["supportFaces"].toArray()[0].toObject();QJsonObject support;
    for(const auto& key:{"bodyId","producerFeatureId","faceId"})support[key]=face[key];
    auto api=[&](const QString& command,const QJsonObject& args=QJsonObject{}){
        const auto response=w.executeApi(command,args);
        if(!response["ok"].toBool())qFatal("%s",QJsonDocument(response).toJson().constData());
        if(QJsonDocument(response).toJson(QJsonDocument::Compact).size()>=1024*1024)qFatal("Public response exceeds bounded frame");
        return response["result"].toObject();
    };
    const auto opened=api("solid.mirrorDialog",{{"sourceBodyId",body["bodyId"]},{"mirrorPlane",support}});
    QVERIFY(opened["previewed"].toBool());
    auto checkDraft=[&](const QJsonObject& publicDraft){
        const auto full=session(w)["documentDraft"].toObject();
        QCOMPARE(full["canvases"],before["canvases"]);
        auto expected=full;QJsonArray rows;
        for(const auto& value:full["canvases"].toArray()){
            auto row=value.toObject();const auto payload=row.take("pngBase64").toString();
            row["pngBase64Omitted"]=QJsonObject{{"encoding","base64"},{"encodedBytes",payload.size()}};rows.append(row);
        }
        expected["canvases"]=rows;QCOMPARE(publicDraft,expected);
    };
    checkDraft(opened["documentDraft"].toObject());
    const auto internalBytes=QJsonDocument(w.state()).toJson(QJsonDocument::Compact).size();
    QVERIFY(internalBytes>1024*1024);
    const auto publicState=api("app.state");
    checkDraft(publicState["activeSession"].toObject()["documentDraft"].toObject());
    QCOMPARE(publicState["document"].toObject()["authored"].toObject(),before);
    const auto publicBytes=QJsonDocument(publicState).toJson(QJsonDocument::Compact).size();
    qInfo()<<"PUBLIC_PREVIEW_PAYLOAD_OK internalStateBytes"<<internalBytes<<"publicStateBytes"<<publicBytes;
    QVERIFY(publicBytes<1024*1024-4096);
    const auto repeated=api("command.preview",{{"session",opened["id"]}});checkDraft(repeated["documentDraft"].toObject());
    api("command.cancel",{{"session",opened["id"]}});QCOMPARE(authored(w),before);
    const auto replacement=api("solid.mirrorDialog",{{"sourceBodyId",body["bodyId"]},{"mirrorPlane",support}});
    api("command.commit",{{"session",replacement["id"]}});const auto committed=authored(w);
    QCOMPARE(committed["canvases"],before["canvases"]);
    call(w,"history.undo");QCOMPARE(authored(w),before);call(w,"history.redo");QCOMPARE(authored(w),committed);
    const auto saved=temp.filePath("portable.alloy");call(w,"document.save",{{"path",saved}});QVERIFY(QFile::remove(imagePath));
    call(w,"document.open",{{"path",saved},{"discard",true}});QCOMPARE(authored(w),committed);
 }
 void placementCancelAndTwoPointCalibration(){
    AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1400,1000);w.show();QVERIFY(QTest::qWaitForWindowExposed(&w));
    ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;QVERIFY(view);
    // One initial harness activation; later checks use focus produced by commands/input.
#ifdef Q_OS_MACOS
    QProcess activation;activation.start("osascript",{"-e",QString("tell application \"System Events\" to set frontmost of first application process whose unix id is %1 to true").arg(QCoreApplication::applicationPid())});QVERIFY(activation.waitForFinished(5000));QCOMPARE(activation.exitCode(),0);
#endif
    w.raise();w.activateWindow();w.windowHandle()->requestActivate();QApplication::setActiveWindow(&w);QTest::qWait(100);
    QTemporaryDir temp;const auto path=temp.filePath("independent-reference.png");QImage image(400,200,QImage::Format_RGB32);image.fill(Qt::white);QPainter painter(&image);painter.setPen(Qt::black);painter.drawRect(2,2,395,195);painter.fillRect(0,0,50,50,Qt::red);painter.fillRect(350,0,50,50,Qt::blue);painter.drawLine(50,100,350,100);painter.drawText(100,160,"Alloy local reference");painter.end();QVERIFY(image.save(path));const auto blank=authored(w);
    call(w,"canvas.dialog",{{"path",path}});auto* dock=w.findChild<QDockWidget*>("canvasDock");QVERIFY(dock && dock->isVisible());auto* width=dock->findChild<QDoubleSpinBox*>("canvas.widthMm");auto* opacity=dock->findChild<QDoubleSpinBox*>("canvas.opacity");QVERIFY(width && opacity);
    type(width,"160");type(opacity,"35");QCOMPARE(session(w)["canvasDraft"].toObject()["widthMm"].toDouble(),160.0);QVERIFY(std::abs(session(w)["canvasDraft"].toObject()["opacity"].toDouble()-.35)<1e-9);QCOMPARE(authored(w),blank);capture(w,"01-native-placement-preview");
    auto* buttons=dock->findChild<QDialogButtonBox*>();QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel),Qt::LeftButton);QVERIFY(session(w).isEmpty());QCOMPARE(authored(w),blank);
    call(w,"canvas.dialog",{{"path",path},{"widthMm",160},{"opacity",.8}});dock=w.findChild<QDockWidget*>("canvasDock");QVERIFY(dock && dock->isVisible());buttons=dock->findChild<QDialogButtonBox*>();QTest::mouseClick(buttons->button(QDialogButtonBox::Ok),Qt::LeftButton);QVERIFY(session(w).isEmpty());const auto inserted=authored(w);const auto id=inserted["canvases"].toArray()[0].toObject()["id"].toString();
    call(w,"canvas.calibrateDialog",{{"canvasId",id}});QTRY_VERIFY(view->hasFocus());dock=w.findChild<QDockWidget*>("canvasDock");QVERIFY(dock && dock->isVisible());
    auto pixel=[&](double x,double y){auto p=call(w,"canvas.projectPixel",{{"canvasId",id},{"pixelX",x},{"pixelY",y}});return QPoint(qRound(p["x"].toDouble()),qRound(p["y"].toDouble()));};const auto first=pixel(50,100),second=pixel(350,100);
    QVERIFY(view->rect().contains(first));QVERIFY(view->rect().contains(second));QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,first);QCOMPARE(session(w)["calibrationPoints"].toArray().size(),1);QVERIFY(view->hasFocus());
    QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,second);QCOMPARE(session(w)["calibrationPoints"].toArray().size(),2);QVERIFY(view->hasFocus());
    auto* distance=dock->findChild<QDoubleSpinBox*>("canvas.distanceMm");QVERIFY(distance);type(distance,"90");QVERIFY(session(w)["previewed"].toBool());capture(w,"02-native-calibration-preview");
    const auto completedPair=session(w);auto extra=w.execute("canvas.pick",{{"x",first.x()},{"y",first.y()}});QVERIFY(!extra["ok"].toBool());QCOMPARE(session(w),completedPair);
    buttons=dock->findChild<QDialogButtonBox*>();QTest::mouseClick(buttons->button(QDialogButtonBox::Ok),Qt::LeftButton);QVERIFY(session(w).isEmpty());const auto calibrated=authored(w);const auto row=calibrated["canvases"].toArray()[0].toObject();QVERIFY(std::abs(row["widthMm"].toDouble()-120)<1);QTRY_VERIFY(view->hasFocus());
    QTest::keyClick(QApplication::focusWidget(),Qt::Key_Z,Qt::ControlModifier);QCOMPARE(authored(w),inserted);QTest::keyClick(QApplication::focusWidget(),Qt::Key_Z,Qt::ControlModifier|Qt::ShiftModifier);QCOMPARE(authored(w),calibrated);capture(w,"03-native-calibrated");
    auto* tree=w.findChild<QTreeWidget*>("browser.tree");QVERIFY(tree);
    auto canvasItem=[&]()->QTreeWidgetItem* {for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole+2).toString()==id && (*it)->parent() && (*it)->parent()->text(0)=="Canvases")return *it;return nullptr;};
    QVERIFY(canvasItem());tree->scrollToItem(canvasItem());QTest::mouseClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,tree->visualItemRect(canvasItem()).center());QVERIFY(tree->hasFocus());QTest::keyClick(QApplication::focusWidget(),Qt::Key_Space);
    QTRY_VERIFY(!authored(w)["canvases"].toArray()[0].toObject()["visible"].toBool());
    QVERIFY(canvasItem());QTest::mouseClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,tree->visualItemRect(canvasItem()).center());QVERIFY(tree->hasFocus());QTest::keyClick(QApplication::focusWidget(),Qt::Key_Space);
    QTRY_VERIFY(authored(w)["canvases"].toArray()[0].toObject()["visible"].toBool());
    call(w,"canvas.calibrateDialog",{{"canvasId",id}});QVERIFY(!session(w)["previewed"].toBool());auto incomplete=w.execute("command.commit",{{"session",session(w)["id"]}});QVERIFY(!incomplete["ok"].toBool());QCOMPARE(authored(w),calibrated);call(w,"command.cancel",{{"session",session(w)["id"]}});
    
 }
};
QTEST_MAIN(CanvasControlsTest)
#include "reference_canvas_controls.moc"
