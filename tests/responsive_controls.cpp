// SPDX-License-Identifier: MIT
#include "AlloyTheme.h"
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QListWidget>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QTest>
#include <QTimer>
#include <QToolButton>
#include <cmath>

class ResponsiveControls final : public QObject {
    Q_OBJECT
    static QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& args={}) {
        const auto response=window.executeApi(command,args);
        if(!response.value("ok").toBool())qFatal("%s",QJsonDocument(response).toJson(QJsonDocument::Compact).constData());
        return response.value("result").toObject();
    }
    static void choose(QToolButton* opener,const QString& actionName) {
        QVERIFY(opener && opener->isVisible());
        QCOMPARE(QApplication::widgetAt(opener->mapToGlobal(opener->rect().center())),static_cast<QWidget*>(opener));
        auto* menu=opener->menu();QVERIFY(menu);
        bool active=false,hit=false,clicked=false,watchdogClosed=false;
        QTimer::singleShot(50,opener,[&]{
            active=QApplication::activePopupWidget()==menu;
            QAction* action=nullptr;
            for(auto* candidate:menu->actions())if(candidate->objectName()==actionName)action=candidate;
            if(!active || !action || !action->isEnabled())return;
            const auto pixel=menu->actionGeometry(action).center();
            hit=QApplication::widgetAt(menu->mapToGlobal(pixel))==menu;
            if(hit){QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,pixel);clicked=true;}
        });
        QTimer watchdog;watchdog.setSingleShot(true);
        QObject::connect(&watchdog,&QTimer::timeout,opener,[&]{if(menu->isVisible()){watchdogClosed=true;menu->close();}});
        watchdog.start(2000);
        QTest::mouseClick(opener,Qt::LeftButton);
        watchdog.stop();
        QVERIFY(active);QVERIFY(hit);QVERIFY(clicked);
        QVERIFY(!watchdogClosed);
        QTRY_VERIFY(QApplication::activePopupWidget()==nullptr);
    }
private slots:
    void compactMenusAndNavigation() {
        const auto out=qEnvironmentVariable("ALLOY_RESPONSIVE_EVIDENCE");QVERIFY(!out.isEmpty());QVERIFY(QDir().mkpath(out));
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1920,900);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto* ribbon=window.findChild<QWidget*>("modelingRibbon");QVERIFY(ribbon);
        auto* nav=window.findChild<QToolButton*>("view.compact.navigation");QVERIFY(nav);
        auto* viewMenu=window.findChild<QToolButton*>("view.compact.view");QVERIFY(viewMenu);
        QTRY_VERIFY(window.findChild<QWidget*>("solid.icons.create")->isVisible());
        call(window,"capture.screenshot",{{"path",out+"/wide-idle.png"}});
        window.resize(1360,850);
        call(window,"capture.screenshot",{{"path",out+"/normal-idle.png"}});
        window.resize(1040,640);
        QTRY_VERIFY(!window.findChild<QToolButton*>("command.primitive.box")->isVisible());
        QVERIFY(window.findChild<QToolButton*>("command.sketch.create")->isVisible());
        QTRY_VERIFY(nav->isVisible() && viewMenu->isVisible());
        QCOMPARE(ribbon->height(),94);
        call(window,"capture.screenshot",{{"path",out+"/minimum-idle.png"}});
        ReferenceView* canvas=nullptr;for(auto* child:window.findChildren<QWidget*>())
            if(auto* candidate=dynamic_cast<ReferenceView*>(child))canvas=candidate;
        QVERIFY(canvas);
        choose(nav,"view.compact.pan");
        QCOMPARE(window.state().value("viewport").toObject().value("navigation").toObject().value("navigationTool"),QJsonValue("pan"));
        call(window,"capture.screenshot",{{"path",out+"/minimum-pan-active.png"}});
        const auto before=window.state().value("viewport").toObject().value("navigation").toObject();
        const QPoint start(canvas->width()/2,canvas->height()/2);
        QCOMPARE(QApplication::widgetAt(canvas->mapToGlobal(start)),static_cast<QWidget*>(canvas));
        QTest::mousePress(canvas,Qt::LeftButton,Qt::NoModifier,start);
        QTest::mouseMove(canvas,start+QPoint(31,14));
        QTest::mouseRelease(canvas,Qt::LeftButton,Qt::NoModifier,start+QPoint(31,14));
        QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(canvas));
        const auto after=window.state().value("viewport").toObject().value("navigation").toObject();
        QVERIFY(after.value("panX")!=before.value("panX") || after.value("panY")!=before.value("panY"));
        QTest::keyClick(canvas,Qt::Key_Escape);
        QCOMPARE(window.state().value("viewport").toObject().value("navigation").toObject().value("navigationTool"),QJsonValue("select"));
        choose(window.findChild<QToolButton*>("solid.menu.create"),"solid.action.primitive.box");
        QCOMPARE(window.state().value("activeSession").toObject().value("command"),QJsonValue("solid.box"));
        call(window,"capture.screenshot",{{"path",out+"/minimum-box-dock.png"}});
        call(window,"command.cancel",{{"session",window.state().value("activeSession").toObject().value("id")}});
        window.close();
    }
    void primitiveDockActionsStayReachable() {
        const auto out=qEnvironmentVariable("ALLOY_PRIMITIVE_DOCK_EVIDENCE");QVERIFY(!out.isEmpty());QVERIFY(QDir().mkpath(out));
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1360,850);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        call(window,"primitive.box");
        auto* dock=window.findChild<QDockWidget*>("primitiveDock");QVERIFY(dock);
        auto* panel=dock->findChild<QWidget*>("primitivePanel");QVERIFY(panel);
        auto* scroll=panel->findChild<QScrollArea*>("primitive.formScroll");QVERIFY(scroll);
        auto* buttons=panel->findChild<QDialogButtonBox*>();QVERIFY(buttons);
        auto* ok=buttons->button(QDialogButtonBox::Ok),*cancel=buttons->button(QDialogButtonBox::Cancel);
        QPushButton* preview=nullptr;for(auto* button:buttons->buttons())if(button->text()=="Preview")preview=qobject_cast<QPushButton*>(button);
        QVERIFY(ok && cancel && preview);
        int lastContentWidth=-1,lastViewportWidth=-1;
        auto fieldsFit=[&]()->bool{
            auto* contents=scroll->widget();if(!contents)return false;
            if(contents->width()!=lastContentWidth || scroll->viewport()->width()!=lastViewportWidth){
                lastContentWidth=contents->width();lastViewportWidth=scroll->viewport()->width();
                qInfo("primitive form widths: contents=%d viewport=%d panel=%d window=%d",
                    lastContentWidth,lastViewportWidth,panel->width(),window.width());
            }
            if(contents->width()>scroll->viewport()->width())return false;
            for(auto* field:contents->findChildren<QDoubleSpinBox*>()){
                const QRect bounds(field->mapTo(scroll->viewport(),QPoint{}),field->size());
                if(bounds.left()<0 || bounds.right()>=scroll->viewport()->width())return false;
            }
            return true;
        };
        QTRY_VERIFY(fieldsFit());
        for(auto* button:{preview,ok,cancel}){
            QVERIFY(button->isVisible());
            QCOMPARE(QApplication::widgetAt(button->mapToGlobal(button->rect().center())),static_cast<QWidget*>(button));
        }
        call(window,"capture.screenshot",{{"path",out+"/normal-box.png"}});
        window.resize(1040,640);
        QTRY_VERIFY(window.findChild<QToolButton*>("view.compact.navigation")->isVisible());
        QTRY_VERIFY(scroll->verticalScrollBar()->maximum()>0);
        QTRY_VERIFY(fieldsFit());
        auto* z=panel->findChild<QDoubleSpinBox*>("primitive.z");QVERIFY(z);
        auto* name=panel->findChild<QLineEdit*>("primitive.name");QVERIFY(name);
        QCOMPARE(QApplication::widgetAt(name->mapToGlobal(name->rect().center())),static_cast<QWidget*>(name));
        QTest::mouseClick(name,Qt::LeftButton);QVERIFY(name->hasFocus());
        auto* editor=z->findChild<QLineEdit*>();QVERIFY(editor);
        for(int i=0;i<10 && !editor->hasFocus();++i){
            auto* focused=QApplication::focusWidget();QVERIFY(focused);
            QTest::keyClick(focused,Qt::Key_Tab);
        }
        QVERIFY(editor->hasFocus());
        QVERIFY(scroll->verticalScrollBar()->value()>0);
        const QRect visibleZ(z->mapTo(scroll->viewport(),QPoint{}),z->size());
        QVERIFY(scroll->viewport()->rect().contains(visibleZ));
        for(auto* button:{preview,ok,cancel}){
            QVERIFY(button->isVisible());
            QCOMPARE(QApplication::widgetAt(button->mapToGlobal(button->rect().center())),static_cast<QWidget*>(button));
        }
        call(window,"capture.screenshot",{{"path",out+"/minimum-box-scrolled.png"}});
        QCOMPARE(QApplication::widgetAt(editor->mapToGlobal(editor->rect().center())),static_cast<QWidget*>(editor));
        QTest::mouseClick(editor,Qt::LeftButton);QVERIFY(editor->hasFocus());
        QTest::keyClick(editor,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(editor,"5");QTest::keyClick(editor,Qt::Key_Tab);
        QCOMPARE(window.state().value("activeSession").toObject().value("inputs").toObject().value("z"),QJsonValue(5));
        QTest::mouseClick(preview,Qt::LeftButton);
        QVERIFY(window.state().value("activeSession").toObject().value("previewed").toBool());
        QCOMPARE(call(window,"document.inspect").value("authored").toObject().value("features").toArray().size(),0);
        QTest::mouseClick(ok,Qt::LeftButton);
        QTRY_VERIFY(window.state().value("activeSession").toObject().isEmpty());
        const auto committed=call(window,"document.inspect");
        const auto features=committed.value("authored").toObject().value("features").toArray();QCOMPARE(features.size(),1);
        QCOMPARE(features.first().toObject().value("parameters").toObject().value("zMm"),QJsonValue(5));
        QCOMPARE(committed.value("evaluation").toObject().value("bodies").toArray().first().toObject().value("volumeMm3"),QJsonValue(6000));
        call(window,"history.undo");
        QCOMPARE(call(window,"document.inspect").value("authored").toObject().value("features").toArray().size(),0);
        const auto beforeCylinder=call(window,"document.inspect");
        call(window,"primitive.cylinder");
        dock=window.findChild<QDockWidget*>("primitiveDock");QVERIFY(dock);
        panel=dock->findChild<QWidget*>("primitivePanel");QVERIFY(panel);
        buttons=panel->findChild<QDialogButtonBox*>();QVERIFY(buttons);
        cancel=buttons->button(QDialogButtonBox::Cancel);QVERIFY(cancel && cancel->isVisible());
        QCOMPARE(QApplication::widgetAt(cancel->mapToGlobal(cancel->rect().center())),static_cast<QWidget*>(cancel));
        call(window,"capture.screenshot",{{"path",out+"/minimum-cylinder.png"}});
        QTest::mouseClick(cancel,Qt::LeftButton);
        QTRY_VERIFY(window.state().value("activeSession").toObject().isEmpty());
        const auto afterCylinder=call(window,"document.inspect");
        QCOMPARE(afterCylinder.value("authored"),beforeCylinder.value("authored"));
        QCOMPARE(afterCylinder.value("undo"),beforeCylinder.value("undo"));
        call(window,"document.save",{{"path",out+"/empty-after-undo.alloy"}});window.close();
    }
    void extrudeDockSmallWindowAudit() {
        const auto out=qEnvironmentVariable("ALLOY_REL024_EXTRUDE_EVIDENCE");QVERIFY(!out.isEmpty());QVERIFY(QDir().mkpath(out));
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1360,850);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        const auto sketch=call(window,"sketch.begin",{{"plane","XY"}}).value("draftSketch").toObject();
        call(window,"sketch.rectangle",{{"x1",0},{"y1",0},{"x2",20},{"y2",12},{"units","mm"}});
        call(window,"sketch.finish");
        const auto evaluated=call(window,"document.inspect").value("evaluation").toObject();
        const auto sketchRow=evaluated.value("sketches").toArray().first().toObject();
        const auto profile=sketchRow.value("profiles").toArray().first().toObject();
        const auto before=call(window,"document.inspect");
        // The incomplete To Object variant exposes the longest form and must give user guidance.
        call(window,"solid.extrudeDialog",{{"sketchId",sketch.value("id")},{"profileId",profile.value("profileId")},{"extent","toObject"}});
        QCOMPARE(window.state().value("activeSession").toObject().value("inputs").toObject().value("extent"),QJsonValue("toObject"));
        auto* dock=window.findChild<QDockWidget*>("extrudeDock");QVERIFY(dock);
        auto* scroll=dock->findChild<QScrollArea*>("extrude.formScroll");QVERIFY(scroll);
        auto* buttons=dock->findChild<QDialogButtonBox*>("extrude.buttons");QVERIFY(buttons);
        auto* ok=buttons->button(QDialogButtonBox::Ok),*cancel=buttons->button(QDialogButtonBox::Cancel);
        QVERIFY(ok && cancel);
        auto* error=dock->findChild<QLabel*>("extrude.error");QVERIFY(error);
        auto* endObject=dock->findChild<QPushButton*>("extrude.endObject");QVERIFY(endObject);
        auto fieldsFit=[&]()->bool{
            if(!dock->isVisible() || !scroll->widget() || scroll->widget()->width()>scroll->viewport()->width())return false;
            for(auto* field:scroll->widget()->findChildren<QWidget*>()){
                if(!field->isVisible() || field->parentWidget()!=scroll->widget())continue;
                const QRect bounds(field->mapTo(scroll->viewport(),QPoint{}),field->size());
                if(bounds.left()<0 || bounds.right()>=scroll->viewport()->width())return false;
            }
            return true;
        };
        auto footerHit=[&]()->bool{
            for(auto* button:{ok,cancel}){
                if(!button->isVisible() || QApplication::widgetAt(button->mapToGlobal(button->rect().center()))!=button)return false;
            }
            return true;
        };
        QTRY_VERIFY(fieldsFit() && footerHit());
        QCOMPARE(error->text(),QString("Select an end face for the extrusion."));
        QVERIFY(!ok->isEnabled());
        QVERIFY(endObject->isVisible());
        scroll->ensureWidgetVisible(error);
        call(window,"capture.screenshot",{{"path",out+"/normal-to-object.png"}});
        window.resize(1040,640);
        QTRY_VERIFY(window.findChild<QToolButton*>("view.compact.navigation")->isVisible());
        QTRY_VERIFY(fieldsFit() && footerHit());
        QVERIFY(scroll->verticalScrollBar()->maximum()>0);
        scroll->ensureWidgetVisible(error);
        QTRY_VERIFY(scroll->viewport()->rect().contains(QRect(error->mapTo(scroll->viewport(),QPoint{}),error->size())));
        qInfo("extrude form: content=%d viewport=%d panel=%d minimum-window=%d",scroll->widget()->width(),scroll->viewport()->width(),
            dock->findChild<QWidget*>("extrudePanel")->width(),window.width());
        call(window,"capture.screenshot",{{"path",out+"/minimum-to-object.png"}});
        QTest::mouseClick(cancel,Qt::LeftButton);
        QTRY_VERIFY(window.state().value("activeSession").toObject().isEmpty());
        const auto afterCancel=call(window,"document.inspect");
        QCOMPARE(afterCancel.value("authored"),before.value("authored"));
        QCOMPARE(afterCancel.value("undo"),before.value("undo"));
        call(window,"solid.extrudeDialog",{{"sketchId",sketch.value("id")},{"profileId",profile.value("profileId")},
            {"extent","distance"},{"distance",8}});
        dock=window.findChild<QDockWidget*>("extrudeDock");QVERIFY(dock);
        scroll=dock->findChild<QScrollArea*>("extrude.formScroll");QVERIFY(scroll);
        buttons=dock->findChild<QDialogButtonBox*>("extrude.buttons");QVERIFY(buttons);
        ok=buttons->button(QDialogButtonBox::Ok);cancel=buttons->button(QDialogButtonBox::Cancel);QVERIFY(ok && cancel);
        QTRY_VERIFY(fieldsFit() && footerHit() && ok->isEnabled());
        QCOMPARE(window.state().value("activeSession").toObject().value("inputs").toObject().value("extent"),QJsonValue("distance"));
        call(window,"capture.screenshot",{{"path",out+"/minimum-distance-ready.png"}});
        QTest::mouseClick(ok,Qt::LeftButton);
        QTRY_VERIFY(window.state().value("activeSession").toObject().isEmpty());
        const auto committed=call(window,"document.inspect");
        const auto features=committed.value("authored").toObject().value("features").toArray();
        QCOMPARE(features.size(),before.value("authored").toObject().value("features").toArray().size()+1);
        QCOMPARE(features.last().toObject().value("type"),QJsonValue("extrude"));
        QVERIFY(std::abs(committed.value("evaluation").toObject().value("bodies").toArray().first().toObject().value("volumeMm3").toDouble()-1920.0)<1e-6);
        call(window,"view.orientation",{{"name","isometric"}});
        call(window,"capture.screenshot",{{"path",out+"/committed-solid.png"}});
        call(window,"document.save",{{"path",out+"/extrude-dock-model.alloy"}});window.close();
    }
    void filletCombineSmallWindowAudit() {
        const auto out=qEnvironmentVariable("ALLOY_REL013_FILLET_COMBINE_EVIDENCE");QVERIFY(!out.isEmpty());QVERIFY(QDir().mkpath(out));
        const auto source=QFileInfo(QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("../examples/gauge-block.alloy")).canonicalFilePath();
        QVERIFY(!source.isEmpty());
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1360,850);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        call(window,"document.open",{{"path",source},{"discard",true}});
        call(window,"view.home");
        const auto before=call(window,"document.inspect");
        QString boxId;
        for(const auto& value:before.value("authored").toObject().value("features").toArray()) {
            const auto feature=value.toObject();if(feature.value("type")=="box" && feature.value("name")=="Gauge block")boxId=feature.value("bodyId").toString();
        }
        QVERIFY(!boxId.isEmpty());
        auto writeCapture=[&](const QString& name,QDockWidget* dock,QDialogButtonBox* buttons,
                              const QString& errorObjectName)->bool {
            if(!dock || !buttons || !dock->isVisible())return false;
            QApplication::processEvents();
            const auto windowGlobal=window.mapToGlobal(QPoint{}),dockGlobal=dock->mapToGlobal(QPoint{});
            QJsonObject snapshot{{"fixture",source},{"windowSize",QJsonArray{window.width(),window.height()}},
                {"windowGlobal",QJsonArray{windowGlobal.x(),windowGlobal.y()}},
                {"dockGeometryGlobal",QJsonArray{dockGlobal.x(),dockGlobal.y(),dock->width(),dock->height()}}};
            auto* panel=dock->widget();
            if(panel)snapshot["panelGeometry"]=QJsonArray{panel->x(),panel->y(),panel->width(),panel->height()};
            auto* error=dock->findChild<QLabel*>(errorObjectName);
            auto* ok=buttons->button(QDialogButtonBox::Ok);auto* cancel=buttons->button(QDialogButtonBox::Cancel);
            const auto session=window.state().value("activeSession").toObject();
            const auto previewError=session.value("previewError").toObject();
            snapshot["failureState"]=QJsonObject{{"label",error ? error->text() : QString{}},
                {"code",previewError.value("code").toString()},{"message",previewError.value("message").toString()},
                {"okEnabled",ok && ok->isEnabled()}};
            QJsonArray widgets;
            auto addWidget=[&](const QString& controlName,QWidget* widget){
                if(!widget){widgets.append(QJsonObject{{"name",controlName},{"present",false}});return;}
                const auto topLeft=widget->mapToGlobal(QPoint{});const QRect bounds(topLeft,widget->size());
                auto* owner=QApplication::widgetAt(bounds.center());
                QStringList ownerPath;for(auto* item=owner;item;item=item->parentWidget()){
                    ownerPath.append(QString::fromLatin1(item->metaObject()->className())+":"+item->objectName());
                    if(item==widget)break;
                }
                widgets.append(QJsonObject{{"name",controlName},{"class",widget->metaObject()->className()},
                    {"visible",widget->isVisible()},{"enabled",widget->isEnabled()},
                    {"geometryGlobal",QJsonArray{bounds.x(),bounds.y(),bounds.width(),bounds.height()}},
                    {"hitOwner",owner ? QString::fromLatin1(owner->metaObject()->className())+":"+owner->objectName() : QString("none")},
                    {"hitOwnerPath",QJsonArray::fromStringList(ownerPath)},
                    {"hitBelongsToTarget",owner && (owner==widget || widget->isAncestorOf(owner))}});
            };
            addWidget("dock",dock);addWidget("panel",panel);addWidget("buttons",buttons);
            addWidget("ok",ok);addWidget("cancel",cancel);addWidget("error",error);
            if(dock->objectName()=="filletDock") {
                addWidget("fillet.groups",dock->findChild<QListWidget*>("fillet.groups"));
                addWidget("fillet.addGroup",dock->findChild<QPushButton*>("fillet.addGroup"));
                addWidget("fillet.removeGroup",dock->findChild<QPushButton*>("fillet.removeGroup"));
                addWidget("filletSelection",dock->findChild<QLabel*>("filletSelection"));
                addWidget("fillet.clearGroup",dock->findChild<QPushButton*>("fillet.clearGroup"));
                addWidget("filletRadius",dock->findChild<QDoubleSpinBox*>("filletRadius"));
            } else {
                auto* scroll=dock->findChild<QScrollArea*>();
                addWidget("combine.scroll",scroll);addWidget("combine.targetBody",dock->findChild<QComboBox*>("combine.targetBody"));
                addWidget("combine.pickTarget",dock->findChild<QPushButton*>("combine.pickTarget"));
                addWidget("combine.toolBodies",dock->findChild<QListWidget*>("combine.toolBodies"));
                addWidget("combine.pickTools",dock->findChild<QPushButton*>("combine.pickTools"));
                addWidget("combine.operation",dock->findChild<QComboBox*>("combine.operation"));
                addWidget("combine.keepTools",dock->findChild<QCheckBox*>("combine.keepTools"));
                snapshot["errorReveal"]="Programmatic QScrollArea::ensureWidgetVisible(combine.error)";
                if(scroll)snapshot["scrollState"]=QJsonObject{{"viewport",QJsonArray{scroll->viewport()->width(),scroll->viewport()->height()}},
                    {"verticalValue",scroll->verticalScrollBar()->value()},{"verticalMaximum",scroll->verticalScrollBar()->maximum()}};
            }
            snapshot["widgets"]=widgets;
            const auto response=window.executeApi("capture.screenshot",{{"path",out+"/"+name+".png"}});
            if(!response.value("ok").toBool())return false;
            QFile file(out+"/"+name+"-geometry.json");if(!file.open(QIODevice::WriteOnly))return false;
            const auto bytes=QJsonDocument(snapshot).toJson(QJsonDocument::Indented);const bool written=file.write(bytes)==bytes.size();file.close();
            if(!written)return false;
            qInfo().noquote()<<name<<QJsonDocument(snapshot).toJson(QJsonDocument::Compact);
            return true;
        };
        const auto fillet=call(window,"solid.filletDialog",{{"targetBodyId",boxId}});
        QCOMPARE(fillet.value("inputs").toObject().value("edgeIds").toArray().size(),0);
        QCOMPARE(window.state().value("activeSession").toObject().value("previewError").toObject().value("code"),QJsonValue("edges_required"));
        auto* filletDock=window.findChild<QDockWidget*>("filletDock");QVERIFY(filletDock);
        auto* filletButtons=filletDock->findChild<QDialogButtonBox*>();QVERIFY(filletButtons);
        QVERIFY(writeCapture("fillet-normal-1360x850",filletDock,filletButtons,"filletError"));
        window.resize(1040,640);QTRY_COMPARE(window.size(),QSize(1040,640));QApplication::processEvents();
        QVERIFY(writeCapture("fillet-minimum-1040x640",filletDock,filletButtons,"filletError"));
        call(window,"command.cancel",{{"session",window.state().value("activeSession").toObject().value("id")}});
        QTRY_VERIFY(window.state().value("activeSession").toObject().isEmpty());
        const auto afterFilletCancel=call(window,"document.inspect");
        QCOMPARE(afterFilletCancel.value("authored"),before.value("authored"));
        QCOMPARE(afterFilletCancel.value("undo"),before.value("undo"));
        call(window,"solid.combineDialog",{{"targetBodyId",boxId},{"toolBodyIds",QJsonArray{}}});
        QCOMPARE(window.state().value("activeSession").toObject().value("previewError").toObject().value("code"),QJsonValue("tools_required"));
        auto* combineDock=window.findChild<QDockWidget*>("combineDock");QVERIFY(combineDock);
        auto* combineButtons=combineDock->findChild<QDialogButtonBox*>();QVERIFY(combineButtons);
        auto* combineError=combineDock->findChild<QLabel*>("combine.error");QVERIFY(combineError);
        if(auto* scroll=combineDock->findChild<QScrollArea*>())scroll->ensureWidgetVisible(combineError,0,16);
        QApplication::processEvents();
        QVERIFY(writeCapture("combine-minimum-1040x640",combineDock,combineButtons,"combine.error"));
        call(window,"command.cancel",{{"session",window.state().value("activeSession").toObject().value("id")}});
        QTRY_VERIFY(window.state().value("activeSession").toObject().isEmpty());
        const auto afterCombineCancel=call(window,"document.inspect");
        QCOMPARE(afterCombineCancel.value("authored"),before.value("authored"));
        QCOMPARE(afterCombineCancel.value("undo"),before.value("undo"));
        window.close();
    }
    void dimensionedSketchPaletteReadabilityAudit() {
        const auto out=qEnvironmentVariable("ALLOY_REL013_SKETCH_PALETTE_EVIDENCE");QVERIFY(!out.isEmpty());QVERIFY(QDir().mkpath(out));
        const auto source=QFileInfo(QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("../examples/dimensioned-rectangle.alloy")).canonicalFilePath();
        QVERIFY(!source.isEmpty());
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1360,850);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        call(window,"document.open",{{"path",source},{"discard",true}});
        const auto before=call(window,"document.inspect");
        const auto features=before.value("authored").toObject().value("features").toArray();QCOMPARE(features.size(),1);
        const auto sketch=features.first().toObject();QCOMPARE(sketch.value("type"),QJsonValue("sketch"));
        const auto sketchId=sketch.value("id").toString();QVERIFY(!sketchId.isEmpty());
        const auto parameters=sketch.value("parameters").toObject();
        QCOMPARE(parameters.value("dimensions").toArray().size(),3);
        QCOMPARE(parameters.value("constraints").toArray().size(),5);
        QCOMPARE(parameters.value("entities").toArray().size(),4);
        const auto lineId=parameters.value("entities").toArray().first().toObject().value("id").toString();QVERIFY(!lineId.isEmpty());
        call(window,"sketch.edit",{{"sketchId",sketchId}});
        QCOMPARE(window.state().value("activeSession").toObject().value("command"),QJsonValue("sketch.edit"));
        call(window,"sketch.lookAt");
        auto* dock=window.findChild<QDockWidget*>("sketchDock");QVERIFY(dock);
        auto* panel=window.findChild<QWidget*>("sketchPanel");QVERIFY(panel);
        auto* scroll=panel->findChild<QScrollArea*>();QVERIFY(scroll);
        auto* content=scroll->widget();QVERIFY(content);
        auto* finish=window.findChild<QPushButton*>("sketch.finish");auto* cancel=window.findChild<QPushButton*>("sketch.cancel");
        QVERIFY(finish && cancel);
        auto* paletteFinish=panel->findChild<QPushButton*>("sketch.palette.finish");
        auto* paletteCancel=panel->findChild<QPushButton*>("sketch.cancel");QVERIFY(paletteFinish && paletteCancel);
        ReferenceView* canvas=nullptr;for(auto* child:window.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(child))canvas=candidate;
        QVERIFY(canvas);
        auto writeCapture=[&](const QString& name)->bool {
            QApplication::processEvents();
            QJsonObject snapshot{{"fixture",source},{"sketchId",sketchId},{"selectedLineId",lineId},
                {"windowSize",QJsonArray{window.width(),window.height()}},
                {"canvasGeometry",QJsonArray{canvas->x(),canvas->y(),canvas->width(),canvas->height()}},
                {"dockGeometry",QJsonArray{dock->x(),dock->y(),dock->width(),dock->height()}},
                {"paletteGeometry",QJsonArray{panel->x(),panel->y(),panel->width(),panel->height()}},
                {"scrollContentWidth",content->width()},
                {"scrollViewport",QJsonArray{scroll->viewport()->width(),scroll->viewport()->height()}},
                {"verticalScroll",QJsonArray{scroll->verticalScrollBar()->value(),scroll->verticalScrollBar()->maximum()}},
                {"authoredDimensions",parameters.value("dimensions").toArray().size()},
                {"authoredConstraints",parameters.value("constraints").toArray().size()}};
            QJsonArray toolbarActions,paletteActions;
            auto addAction=[&](QJsonArray& actions,QWidget* action) {
                if(!action)return;
                const QPoint top=action->mapToGlobal(QPoint{});const QRect global(top,action->size());
                auto* hit=QApplication::widgetAt(global.center());
                actions.append(QJsonObject{{"objectName",action->objectName()},{"text",qobject_cast<QAbstractButton*>(action)->text()},
                    {"visible",action->isVisible()},{"enabled",action->isEnabled()},
                    {"geometryGlobal",QJsonArray{global.x(),global.y(),global.width(),global.height()}},
                    {"hitOwner",hit ? QString("%1:%2").arg(QString::fromLatin1(hit->metaObject()->className()),hit->objectName()) : QString("none")}});
            };
            addAction(toolbarActions,finish);addAction(toolbarActions,cancel);
            addAction(paletteActions,paletteFinish);addAction(paletteActions,paletteCancel);
            QJsonArray labels;
            const QRect viewportRect=scroll->viewport()->rect();
            for(auto* label:panel->findChildren<QLabel*>()) {
                if(!scroll->viewport()->isAncestorOf(label))continue;
                const QPoint origin=label->mapTo(scroll->viewport(),QPoint{});const QRect bounds(origin,label->size());
                const QRect clipped=bounds.intersected(viewportRect);
                labels.append(QJsonObject{{"text",label->text()},{"visible",label->isVisible()},
                    {"boundsInScrollViewport",QJsonArray{bounds.x(),bounds.y(),bounds.width(),bounds.height()}},
                    {"intersectionInScrollViewport",QJsonArray{clipped.x(),clipped.y(),clipped.width(),clipped.height()}},
                    {"fullyInsideViewport",!label->isVisible() || viewportRect.contains(bounds)}});
            }
            snapshot["toolbarActions"] = toolbarActions;snapshot["paletteActions"] = paletteActions;snapshot["paletteLabels"] = labels;
            const auto response=window.executeApi("capture.screenshot",{{"path",out+"/"+name+".png"}});
            if(!response.value("ok").toBool())return false;
            QFile file(out+"/"+name+"-geometry.json");if(!file.open(QIODevice::WriteOnly))return false;
            const auto bytes=QJsonDocument(snapshot).toJson(QJsonDocument::Indented);const bool written=file.write(bytes)==bytes.size();file.close();
            if(written)qInfo().noquote()<<name<<QJsonDocument(snapshot).toJson(QJsonDocument::Compact);
            return written;
        };
        QTRY_VERIFY(dock->isVisible() && finish->isVisible() && cancel->isVisible());
        QVERIFY(writeCapture("normal-1360x850"));
        window.resize(1040,640);QTRY_COMPARE(window.size(),QSize(1040,640));QApplication::processEvents();
        QVERIFY(writeCapture("minimum-1040x640"));
        call(window,"sketch.selectionSet",{{"entityIds",QJsonArray{lineId}}});
        QCOMPARE(window.state().value("activeSession").toObject().value("selection").toObject().value("entityIds").toArray(),QJsonArray{lineId});
        QVERIFY(writeCapture("minimum-selected-1040x640"));
        call(window,"sketch.cancel");QTRY_VERIFY(window.state().value("activeSession").toObject().isEmpty());
        const auto after=call(window,"document.inspect");
        QCOMPARE(after.value("authored"),before.value("authored"));
        QCOMPARE(after.value("undo"),before.value("undo"));
        window.close();
    }
};
QTEST_MAIN(ResponsiveControls)
#include "responsive_controls.moc"
