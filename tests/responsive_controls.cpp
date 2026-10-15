// SPDX-License-Identifier: MIT
#include "AlloyTheme.h"
#include "AlloyWindow.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
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
};
QTEST_MAIN(ResponsiveControls)
#include "responsive_controls.moc"
