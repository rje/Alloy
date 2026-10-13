#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QAccessible>
#include <QAction>
#include <QMenu>
#include <QToolButton>
#include <QCheckBox>
#include <QTemporaryDir>
#include <QImage>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QDialogButtonBox>
#include <QDialog>
#include <QDir>
#include <QContextMenuEvent>
#include <QTimer>
#include <QScrollArea>
#include <QScrollBar>
#include <QDockWidget>
#include <QJsonDocument>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QStyle>
#include <QStyleOptionViewItem>
#include <QTest>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <cmath>

// Synthesized QWidget events against the same compiled application core.
// Requires a native graphics session; this is not physical OS input evidence.
class WidgetInteractions final : public QObject {
    Q_OBJECT
    QByteArray error_;
    bool run(AlloyWindow& window, const QString& command, const QJsonObject& args = {}) {
        const auto response = window.execute(command,args);
        error_ = QJsonDocument(response).toJson(QJsonDocument::Compact);
        return response.value("ok").toBool();
    }
    static QJsonObject session(AlloyWindow& window) {return window.state()["activeSession"].toObject();}
    static ReferenceView* viewport(AlloyWindow& window) {
        for (auto* child : window.findChildren<QWidget*>())
            if (auto* view = dynamic_cast<ReferenceView*>(child)) return view;
        return nullptr;
    }
    static QPoint sketchPixel(ReferenceView* view,double u,double v) {
        const auto p = view->projectSketchPoint(u,v,"XY");
        return {qRound(p["x"].toDouble()),qRound(p["y"].toDouble())};
    }
    static QPoint checkbox(QWidget* widget,const QRect& row,Qt::CheckState state) {
        QStyleOptionViewItem option;option.initFrom(widget);option.rect = row;
        option.features = QStyleOptionViewItem::HasCheckIndicator|QStyleOptionViewItem::HasDisplay;
        option.checkState = state;
        return widget->style()->subElementRect(QStyle::SE_ItemViewItemCheckIndicator,&option,widget).center();
    }
private slots:
    void initTestCase() {
        QPalette dark;dark.setColor(QPalette::Window,Qt::black);dark.setColor(QPalette::Base,Qt::black);
        qApp->setPalette(dark);AlloyTheme::apply(*qApp);
        QCOMPARE(qApp->palette().color(QPalette::Window),QColor("#f7f8f9"));
    }
    void commandFirstCircle() {
        AlloyWindow window;window.resize(1180,760);window.show();window.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&window));auto* view=viewport(window);QVERIFY(view);
        view->setFocus();QTest::keyClick(view,Qt::Key_C);
        QTRY_COMPARE(session(window)["command"].toString(),QString("sketch.plane"));
        QCOMPARE(session(window)["pendingTool"].toString(),QString("circle"));
        QTest::keyClick(view,Qt::Key_Escape);QTRY_VERIFY(session(window).isEmpty());
        QCOMPARE(window.state()["document"].toObject()["features"].toInt(),0);
        auto* action=window.findChild<QAction*>("solid.action.sketch.circle");QVERIFY(action);action->trigger();
        QTRY_COMPARE(session(window)["pendingTool"].toString(),QString("circle"));
        auto* plane=window.findChild<QPushButton*>("sketch.plane.XY");QVERIFY(plane);QTest::mouseClick(plane,Qt::LeftButton);
        QTRY_COMPARE(session(window)["tool"].toString(),QString("circle"));
        QTRY_VERIFY(QApplication::focusWidget()==view);
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,sketchPixel(view,0,0));
        QTest::keyClick(view,Qt::Key_2);
        auto* value=window.findChild<QLineEdit*>("sketch.inlineValue");QVERIFY(value);QTRY_VERIFY(value->isVisible());
        QTRY_VERIFY(QApplication::focusWidget()==value);QTest::keyClicks(value,"6");QTest::keyClick(value,Qt::Key_Return);
        auto parameters=session(window)["draftSketch"].toObject()["parameters"].toObject();
        QCOMPARE(parameters["entities"].toArray().size(),1);QCOMPARE(parameters["dimensions"].toArray().size(),1);
        QCOMPARE(parameters["dimensions"].toArray()[0].toObject()["valueMm"].toDouble(),26.0);
        QVERIFY2(run(window,"sketch.cancel"),error_.constData());
        QCOMPARE(window.state()["document"].toObject()["features"].toInt(),0);
    }
    void mirrorBodyRimCancelCommit() {
        AlloyWindow window;window.resize(1180,760);window.show();window.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&window));auto* view=viewport(window);QVERIFY(view);
        // Only stock authoring uses commands; Shell input is dispatched to real widgets.
        QVERIFY2(run(window,"sketch.begin",{{"plane","XY"}}),error_.constData());
        QVERIFY2(run(window,"sketch.rectangle",{{"x1",0},{"y1",0},{"x2",25},{"y2",25}}),error_.constData());
        QVERIFY2(run(window,"sketch.finish"),error_.constData());
        auto inspected=window.execute("document.inspect")["result"].toObject();
        const auto sketch=inspected["evaluation"].toObject()["sketches"].toArray()[0].toObject();
        QVERIFY2(run(window,"solid.extrude",{{"sketchId",sketch["sketchId"]},
            {"profileId",sketch["profiles"].toArray()[0].toObject()["profileId"]},{"distance",12.5}}),error_.constData());
        QVERIFY2(run(window,"selection.set",{{"bodyId",""}}),error_.constData());
        QVERIFY(session(window).isEmpty());
        QVERIFY2(run(window,"view.home"),error_.constData());
        inspected=window.execute("document.inspect")["result"].toObject();

        const auto body=inspected["evaluation"].toObject()["bodies"].toArray()[0].toObject();
        QJsonObject cap;
        for(const auto& value:body["supportFaces"].toArray()) {
            const auto face=value.toObject();
            if(face["frame"].toObject()["normal"].toArray()==QJsonArray{0,0,1})cap=face;
        }
        QVERIFY(!cap.isEmpty());
        const QJsonObject support{{"producerFeatureId",cap["producerFeatureId"]},{"bodyId",cap["bodyId"]},{"faceId",cap["faceId"]}};
        QVERIFY2(run(window,"solid.shell",{{"targetBodyId",body["bodyId"]},{"removedFace",support},{"thicknessMm",2}}),error_.constData());
        QVERIFY2(run(window,"selection.set",{{"bodyId",""}}),error_.constData());
        QVERIFY(session(window).isEmpty());
        QVERIFY2(run(window,"view.orientation",{{"name","top"}}),error_.constData());
        const auto baseline=window.execute("document.inspect")["result"].toObject()["authored"].toObject();
        auto* mirror=window.findChild<QToolButton*>("command.solid.mirrorDialog");QVERIFY(mirror);
        for(const bool commit:{false,true}) {
            QTest::mouseClick(mirror,Qt::LeftButton);
            QTRY_COMPARE(session(window)["command"].toString(),QString("solid.mirror"));
            auto* dock=window.findChild<QDockWidget*>("mirrorDock");QVERIFY(dock);
            auto* buttons=dock->findChild<QDialogButtonBox*>();QVERIFY(buttons);
            QPushButton* objects=nullptr;QPushButton* plane=nullptr;
            for(auto* button:dock->findChildren<QPushButton*>()) {
                if(button->text()=="Select Object")objects=button;
                if(button->text()=="Select Mirror Plane")plane=button;
            }
            QVERIFY(objects && plane);QTest::mouseClick(objects,Qt::LeftButton);
            QCOMPARE(session(window)["selectionRole"].toString(),QString("objects"));
            const auto point=view->projectSketchPoint(1,12.5,"face",cap["frame"].toObject());
            const QPoint rim(qRound(point["x"].toDouble()),qRound(point["y"].toDouble()));
            QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,rim);
            QTRY_COMPARE(session(window)["inputs"].toObject()["sourceBodyId"],body["bodyId"]);
            QTest::mouseClick(plane,Qt::LeftButton);
            QCOMPARE(session(window)["selectionRole"].toString(),QString("mirrorPlane"));
            QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,rim);
            QTRY_VERIFY(session(window)["geometryEvaluated"].toBool());
            QCOMPARE(session(window)["inputs"].toObject()["mirrorPlane"].toObject()["faceId"],cap["faceId"]);
            QVERIFY(std::abs(session(window)["geometry"].toObject()["volumeMm3"].toDouble()-3182)<1e-4);
            QCOMPARE(window.execute("document.inspect")["result"].toObject()["authored"].toObject(),baseline);
            QVERIFY(buttons->button(QDialogButtonBox::Ok)->isEnabled());
            if(commit) {
                QVERIFY2(run(window,"view.orientation",{{"name","isometric"}}),error_.constData());
                dock->widget()->grab().save("build-tutorial-polish/evidence/mirror-independent/qt-dialog.png");
                QVERIFY2(run(window,"capture.screenshot",{{"path",QDir::currentPath()+"/build-tutorial-polish/evidence/mirror-independent/qt-preview.png"}}),error_.constData());
            }
            QTest::mouseClick(buttons->button(commit ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel),Qt::LeftButton);
            QTRY_VERIFY(session(window).isEmpty());
            inspected=window.execute("document.inspect")["result"].toObject();
            if(!commit)QCOMPARE(inspected["authored"].toObject(),baseline);
        }
        const auto results=inspected["evaluation"].toObject()["bodies"].toArray();QCOMPARE(results.size(),2);
        for(const auto& result:results)QVERIFY(std::abs(result.toObject()["volumeMm3"].toDouble()-3182)<1e-4);
    }
    void centerRectangleGestureAndInlineValues() {
        AlloyWindow window;window.resize(1180,760);window.show();window.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&window));auto* view=viewport(window);QVERIFY(view);
        QVERIFY2(run(window,"sketch.begin",{{"plane","XY"}}),error_.constData());
        auto* action=window.findChild<QAction*>("sketch.tool.centerRectangle");QVERIFY(action);
        action->trigger();QCOMPARE(session(window)["tool"].toString(),QString("centerRectangle"));
        view->setFocus();QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,sketchPixel(view,12.5,12.5));
        QTRY_VERIFY(session(window).contains("firstPoint"));
        QTest::keyClick(view,Qt::Key_1);
        auto* width=window.findChild<QLineEdit*>("sketch.inlineValue");
        auto* height=window.findChild<QLineEdit*>("sketch.centerRectangle.height");
        QVERIFY(width && height);QTRY_VERIFY(width->isVisible() && height->isVisible());
        QTRY_VERIFY(QApplication::focusWidget()==width);
        QTest::keyClick(width,Qt::Key_2);QCOMPARE(width->text(),QString("12"));
        QTest::keyClick(width,Qt::Key_Tab);QTRY_VERIFY(QApplication::focusWidget()==height);
        QTest::keyClicks(height,"12");QTest::keyClick(height,Qt::Key_Return);
        auto parameters=session(window)["draftSketch"].toObject()["parameters"].toObject();
        QCOMPARE(parameters["entities"].toArray().size(),6);
        QCOMPARE(parameters["dimensions"].toArray().size(),2);
        QCOMPARE(parameters["dimensions"].toArray()[0].toObject()["valueMm"].toDouble(),12.0);
        QCOMPARE(parameters["dimensions"].toArray()[1].toObject()["valueMm"].toDouble(),12.0);
        QVERIFY(!session(window).contains("pendingCenterRectangle"));
        QVERIFY2(run(window,"sketch.cancel"),error_.constData());
    }
    void circleOffsetFlipPreviewCancelAccept() {
        AlloyWindow window;window.resize(1180,760);window.show();window.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QVERIFY2(run(window,"sketch.begin",{{"plane","XY"}}),error_.constData());
        QVERIFY2(run(window,"sketch.addCircle",{{"centerX",0},{"centerY",0},{"diameter",6.3}}),error_.constData());
        const auto original=session(window)["draftSketch"].toObject();
        const auto source=original["parameters"].toObject()["entities"].toArray()[0].toObject()["id"];
        QVERIFY2(run(window,"sketch.selectionSet",{{"entityIds",QJsonArray{source}}}),error_.constData());
        auto* action=window.findChild<QAction*>("sketch.tool.offset");QVERIFY(action);action->trigger();
        QTRY_COMPARE(session(window)["tool"].toString(),QString("offset"));
        QTRY_VERIFY(session(window)["pendingOffset"].toObject()["valid"].toBool());
        QCOMPARE(session(window)["draftSketch"].toObject(),original);
        auto* flip=window.findChild<QPushButton*>("sketch.offset.flip");QVERIFY(flip);
        QTest::mouseClick(flip,Qt::LeftButton);
        QTRY_COMPARE(session(window)["pendingOffset"].toObject()["distanceMm"].toDouble(),-1.0);
        const auto preview=session(window)["pendingOffset"].toObject()["previewDraft"].toObject();
        QVERIFY(std::abs(preview["parameters"].toObject()["entities"].toArray()[1].toObject()["radiusMm"].toDouble()-2.15)<1e-6);
        auto* cancel=window.findChild<QPushButton*>("sketch.offset.cancel");QVERIFY(cancel);
        QTest::mouseClick(cancel,Qt::LeftButton);
        QCOMPARE(session(window)["draftSketch"].toObject(),original);
        QVERIFY(!session(window).contains("pendingOffset"));
        QVERIFY2(run(window,"sketch.tool",{{"tool","offset"}}),error_.constData());
        QVERIFY2(run(window,"sketch.offsetPreview",{{"sourceEntityId",source},{"distance",-1}}),error_.constData());
        auto* accept=window.findChild<QPushButton*>("sketch.offset.accept");QVERIFY(accept);
        QTest::mouseClick(accept,Qt::LeftButton);
        const auto authored=session(window)["draftSketch"].toObject()["parameters"].toObject();
        QCOMPARE(authored["entities"].toArray().size(),2);
        QCOMPARE(authored["constraints"].toArray().size(),2);
        QVERIFY(std::abs(authored["entities"].toArray()[1].toObject()["radiusMm"].toDouble()-2.15)<1e-6);
        const auto targetId=authored["entities"].toArray()[1].toObject()["id"];
        QVERIFY2(run(window,"sketch.selectionSet",{{"entityIds",QJsonArray{source}}}),error_.constData());
        QVERIFY2(run(window,"sketch.tool",{{"tool","offset"}}),error_.constData());
        QTRY_VERIFY(session(window)["pendingOffset"].toObject()["constraintId"].isString());
        auto* distance=window.findChild<QLineEdit*>("sketch.offset.distance");QVERIFY(distance);
        distance->setText("-1.5");
        QTRY_COMPARE(session(window)["pendingOffset"].toObject()["distanceMm"].toDouble(),-1.5);
        QTest::mouseClick(accept,Qt::LeftButton);
        const auto edited=session(window)["draftSketch"].toObject()["parameters"].toObject();
        QCOMPARE(edited["entities"].toArray().size(),2);
        QCOMPARE(edited["entities"].toArray()[1].toObject()["id"],targetId);
        QVERIFY(std::abs(edited["entities"].toArray()[1].toObject()["radiusMm"].toDouble()-1.65)<1e-6);
        QVERIFY2(run(window,"sketch.cancel"),error_.constData());
    }
    void shellFaceThicknessCancelCommit() {
        AlloyWindow window;window.resize(1180,760);window.show();window.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&window));auto* view=viewport(window);QVERIFY(view);
        // Only stock authoring uses commands; Shell input is dispatched to real widgets.
        QVERIFY2(run(window,"sketch.begin",{{"plane","XY"}}),error_.constData());
        QVERIFY2(run(window,"sketch.rectangle",{{"x1",0},{"y1",0},{"x2",25},{"y2",25}}),error_.constData());
        QVERIFY2(run(window,"sketch.finish"),error_.constData());
        auto inspected=window.execute("document.inspect")["result"].toObject();
        const auto sketch=inspected["evaluation"].toObject()["sketches"].toArray()[0].toObject();
        QVERIFY2(run(window,"solid.extrude",{{"sketchId",sketch["sketchId"]},
            {"profileId",sketch["profiles"].toArray()[0].toObject()["profileId"]},{"distance",12.5}}),error_.constData());
        QVERIFY2(run(window,"selection.set",{{"bodyId",""}}),error_.constData());
        QVERIFY2(run(window,"view.home"),error_.constData());
        inspected=window.execute("document.inspect")["result"].toObject();
        const auto baseline=inspected["authored"].toObject();
        const auto body=inspected["evaluation"].toObject()["bodies"].toArray()[0].toObject();
        QJsonObject cap;
        for(const auto& value:body["supportFaces"].toArray()) {
            const auto face=value.toObject();
            if(face["frame"].toObject()["normal"].toArray()==QJsonArray{0,0,1})cap=face;
        }
        QVERIFY(!cap.isEmpty());
        auto* shell=window.findChild<QToolButton*>("command.solid.shellDialog");QVERIFY(shell);
        for(const bool commit:{false,true}) {
            QTest::mouseClick(shell,Qt::LeftButton);
            QTRY_COMPARE(session(window)["command"].toString(),QString("solid.shell"));
            QVERIFY(!session(window)["geometryEvaluated"].toBool());
            auto* dock=window.findChild<QDockWidget*>("shellDock");QVERIFY(dock);
            auto* buttons=dock->findChild<QDialogButtonBox*>();QVERIFY(buttons);
            QVERIFY(!buttons->button(QDialogButtonBox::Ok)->isEnabled());
            const auto projected=view->projectSketchPoint(12.5,12.5,"face",cap["frame"].toObject());
            QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,
                QPoint(qRound(projected["x"].toDouble()),qRound(projected["y"].toDouble())));
            QTRY_COMPARE(session(window)["inputs"].toObject()["removedFace"].toObject()["faceId"],cap["faceId"]);
            auto* thickness=dock->findChild<QDoubleSpinBox*>("shellThickness");QVERIFY(thickness);
            thickness->setFocus();thickness->selectAll();QTest::keyClicks(thickness,"2");QTest::keyClick(thickness,Qt::Key_Return);
            QTRY_VERIFY(session(window)["geometryEvaluated"].toBool());
            QVERIFY(std::abs(session(window)["geometry"].toObject()["volumeMm3"].toDouble()-3182)<1e-4);
            QVERIFY(buttons->button(QDialogButtonBox::Ok)->isEnabled());
            QCOMPARE(window.execute("document.inspect")["result"].toObject()["authored"].toObject(),baseline);
            QTest::mouseClick(buttons->button(commit ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel),Qt::LeftButton);
            QTRY_VERIFY(session(window).isEmpty());
            inspected=window.execute("document.inspect")["result"].toObject();
            if(!commit)QCOMPARE(inspected["authored"].toObject(),baseline);
        }
        const auto features=inspected["authored"].toObject()["features"].toArray();
        QCOMPARE(features.size(),baseline["features"].toArray().size()+1);
        QCOMPARE(features.last().toObject()["type"].toString(),QString("shell"));
        const auto result=inspected["evaluation"].toObject()["bodies"].toArray()[0].toObject();
        QCOMPARE(result["bodyId"],body["bodyId"]);
        QVERIFY(result["valid"].toBool() && result["closed"].toBool());
        QVERIFY(std::abs(result["volumeMm3"].toDouble()-3182)<1e-4);
    }
    void offsetPlaneSelectedSketchAndPlaneInput() {
        AlloyWindow window;window.resize(1180,760);window.show();window.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QVERIFY2(run(window,"sketch.begin",{{"plane","XY"}}),error_.constData());
        QVERIFY2(run(window,"sketch.rectangle",{{"x1",0},{"y1",0},{"x2",20},{"y2",20}}),error_.constData());
        QVERIFY2(run(window,"sketch.finish"),error_.constData());
        auto inspected=window.execute("document.inspect")["result"].toObject();
        QString sourceId=inspected["evaluation"].toObject()["sketches"].toArray()[0].toObject()["sketchId"].toString();
        auto* view=viewport(window);QVERIFY(view);
        auto* tree=window.findChild<QTreeWidget*>("browser.tree");QVERIFY(tree);
        auto* offset=window.findChild<QToolButton*>("command.construction.plane");QVERIFY(offset);
        auto* create=window.findChild<QToolButton*>("command.sketch.create");QVERIFY(create);
        for(int pass=0;pass<3;++pass) {
            QTreeWidgetItem* sourceItem=nullptr;
            for(QTreeWidgetItemIterator it(tree);*it;++it)
                if((*it)->data(0,Qt::UserRole+2).toString()==sourceId)sourceItem=*it;
            QVERIFY(sourceItem);tree->scrollToItem(sourceItem);
            QTest::mouseClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,tree->visualItemRect(sourceItem).center());
            const auto before=window.execute("document.inspect")["result"].toObject()["authored"].toObject();
            bool handled=false;
            auto fillDialog=[&]{
                auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());
                if(!dialog)return;
                QTimer::singleShot(5000,dialog,&QDialog::reject);
                auto* choices=dialog->findChild<QComboBox*>("construction.source");
                auto* value=dialog->findChild<QDoubleSpinBox*>("construction.offset");
                auto* buttons=dialog->findChild<QDialogButtonBox*>();
                QVERIFY(choices && value && buttons);
                QCOMPARE(choices->currentData().toJsonObject().value(pass<2 ? "sketchId" : "planeId").toString(),sourceId);
                value->setFocus();value->selectAll();QTest::keyClicks(value,pass<2 ? "18" : "20");
                // Tab commits the numeric editor without accepting the modal dialog.
                QTest::keyClick(value,Qt::Key_Tab);
                QCOMPARE(value->value(),pass<2 ? 18.0 : 20.0);
                QCOMPARE(window.execute("document.inspect")["result"].toObject()["authored"].toObject(),before);
                if(pass==1) {
                    QVERIFY2(run(window,"capture.dialog",{{"path",QDir::currentPath()+"/build-tutorial-polish/evidence/construction-planes/qt-offset-dialog.png"}}),error_.constData());
                    QVERIFY2(run(window,"capture.screenshot",{{"path",QDir::currentPath()+"/build-tutorial-polish/evidence/construction-planes/qt-amber-preview.png"}}),error_.constData());
                    value->setFocus();value->selectAll();QTest::keyClicks(value,"18");
                    QTest::keyClick(value,Qt::Key_Return);
                    QVERIFY2(!dialog->isVisible(),"Enter in Offset must accept the plane dialog");
                    handled=true;return;
                }
                handled=true;
                QTest::mouseClick(buttons->button(pass==0 ? QDialogButtonBox::Cancel : QDialogButtonBox::Ok),Qt::LeftButton);
            };
            if(pass==1) {
                QVERIFY2(run(window,"view.orientation",{{"name","top"}}),error_.constData());
                QTimer::singleShot(100,&window,[&]{
                    auto* menu=qobject_cast<QMenu*>(QApplication::activePopupWidget());QVERIFY(menu);
                    QTimer::singleShot(5000,menu,&QMenu::close);
                    QCOMPARE(menu->objectName(),QString("context.viewportProfile"));
                    auto* action=menu->findChild<QAction*>("context.offsetPlane");QVERIFY(action);
                    menu->grab().save("build-tutorial-polish/evidence/construction-planes/qt-profile-context.png");
                    QTimer::singleShot(100,&window,fillDialog);
                    QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,menu->actionGeometry(action).center());
                });
                const auto point=sketchPixel(view,10,10);
                QContextMenuEvent event(QContextMenuEvent::Mouse,point,view->mapToGlobal(point));
                QApplication::sendEvent(view,&event);
            } else {
                QTimer::singleShot(100,&window,fillDialog);
                QTest::mouseClick(offset,Qt::LeftButton);
            }
            QVERIFY(handled);
            inspected=window.execute("document.inspect")["result"].toObject();
            if(pass==0){QCOMPARE(inspected["authored"].toObject(),before);continue;}
            const auto planes=inspected["evaluation"].toObject()["constructionPlanes"].toArray();
            QCOMPARE(planes.size(),pass);
            const auto plane=planes.last().toObject();
            QCOMPARE(plane["frame"].toObject()["origin"].toArray()[2].toDouble(),pass==1 ? 18.0 : 38.0);
            sourceId=plane["featureId"].toString();
            QTreeWidgetItem* planeItem=nullptr;
            for(QTreeWidgetItemIterator it(tree);*it;++it)
                if((*it)->data(0,Qt::UserRole+2).toString()==sourceId)planeItem=*it;
            QVERIFY(planeItem);tree->scrollToItem(planeItem);
            QTest::mouseClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,tree->visualItemRect(planeItem).center());
            QTest::mouseClick(create,Qt::LeftButton);
            QTRY_COMPARE(session(window)["command"].toString(),QString("sketch.edit"));
            QCOMPARE(session(window)["inputs"].toObject()["planeId"].toString(),sourceId);
            QCOMPARE(session(window)["resolvedFrame"].toObject()["origin"].toArray()[2].toDouble(),pass==1 ? 18.0 : 38.0);
            QVERIFY2(run(window,"sketch.cancel"),error_.constData());
        }
    }
    void meshExportFileScopeAndCheckedBodies() {
        AlloyWindow window;window.resize(1180,760);window.show();window.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QVERIFY2(run(window,"solid.box",{{"width",20},{"depth",10},{"height",15}}),error_.constData());
        QVERIFY2(run(window,"solid.box",{{"width",12},{"depth",12},{"height",12},{"x",40}}),error_.constData());
        QVERIFY2(run(window,"selection.set",{{"bodyId",""}}),error_.constData());
        QVERIFY(session(window).isEmpty());
        const auto baseline=window.execute("document.inspect")["result"].toObject()["authored"].toObject();
        QPushButton* file=nullptr;for(auto* button:window.findChildren<QPushButton*>())if(button->text()=="File")file=button;
        QVERIFY(file && file->menu());
        QTimer::singleShot(100,&window,[&]{
            auto* menu=file->menu();QAction* mesh=nullptr;
            QTimer::singleShot(5000,menu,&QMenu::close);
            for(auto* action:menu->actions())if(action->text()=="Save As Mesh…")mesh=action;
            QVERIFY(mesh);QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,menu->actionGeometry(mesh).center());
        });
        QTest::mouseClick(file,Qt::LeftButton);
        QTRY_VERIFY(window.findChild<QDialog*>("mesh.export.dialog"));
        auto* dialog=window.findChild<QDialog*>("mesh.export.dialog");QTRY_VERIFY(dialog->isVisible());
        auto* scope=dialog->findChild<QComboBox*>("mesh.export.body");QVERIFY(scope);
        QCOMPARE(scope->currentData().toString(),QString("visible"));
        dialog->grab().save("build-tutorial-polish/evidence/multibody-independent/qt-all-visible.png");
        scope->setFocus();QTest::keyClick(scope,Qt::Key_Home);
        QTest::keyClick(scope,Qt::Key_Down);QTest::keyClick(scope,Qt::Key_Down);
        QCOMPARE(scope->currentData().toString(),QString("selected"));
        auto* list=dialog->findChild<QListWidget*>("mesh.export.bodyIds");QVERIFY(list);QTRY_VERIFY(list->isVisible());QCOMPARE(list->count(),2);
        for(int i=0;i<list->count();++i) {
            QCOMPARE(list->item(i)->checkState(),Qt::Unchecked);
            QTest::mouseClick(list->viewport(),Qt::LeftButton,Qt::NoModifier,list->visualItemRect(list->item(i)).center());
            QTest::keyClick(list,Qt::Key_Space);
            QCOMPARE(list->item(i)->checkState(),Qt::Checked);
        }
        dialog->grab().save("build-tutorial-polish/evidence/multibody-independent/qt-selected-bodies.png");
        auto* buttons=dialog->findChild<QDialogButtonBox*>();QVERIFY(buttons);
        QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel),Qt::LeftButton);
        QCOMPARE(window.execute("document.inspect")["result"].toObject()["authored"].toObject(),baseline);
    }
    void loftOrderedViewportProfilesCancelCommit() {
        AlloyWindow window;window.resize(1180,760);window.show();window.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&window));auto* view=viewport(window);QVERIFY(view);
        QJsonArray refs;QString preceding;
        for(int i=0;i<4;++i) {
            QJsonObject args{{"plane","XY"}};
            if(i) {
                const auto response=window.execute("construction.plane",{{"source",QJsonObject{{"sketchId",preceding}}},{"offset",i==1 ? 18 : 20}});
                QVERIFY(response["ok"].toBool());args={{"planeId",response["result"].toObject()["planeId"]}};
            }
            QVERIFY2(run(window,"sketch.begin",args),error_.constData());
            QVERIFY2(run(window,"sketch.addCircle",{{"centerX",0},{"centerY",0},{"diameter",QJsonArray{38,16,30,44}[i]}}),error_.constData());
            QVERIFY2(run(window,"sketch.finish"),error_.constData());
            const auto sketch=window.execute("document.inspect")["result"].toObject()["evaluation"].toObject()["sketches"].toArray().last().toObject();
            preceding=sketch["sketchId"].toString();refs.append(QJsonObject{{"sketchId",preceding},{"profileId",sketch["profiles"].toArray()[0].toObject()["profileId"]}});
        }
        QVERIFY2(run(window,"selection.set",{{"bodyId",""}}),error_.constData());
        QVERIFY2(run(window,"view.home"),error_.constData());
        const auto baseline=window.execute("document.inspect")["result"].toObject()["authored"].toObject();
        auto* loft=window.findChild<QToolButton*>("command.solid.loftDialog");QVERIFY(loft);
        for(const bool commit:{false,true}) {
            QTest::mouseClick(loft,Qt::LeftButton);QTRY_COMPARE(session(window)["command"].toString(),QString("solid.loft"));
            auto* dock=window.findChild<QDockWidget*>("loftDock");QVERIFY(dock);
            auto* buttons=dock->findChild<QDialogButtonBox*>();QVERIFY(buttons);
            QVERIFY(!buttons->button(QDialogButtonBox::Ok)->isEnabled());
            for(int i=0;i<refs.size();++i) {
                const auto point=window.execute("view.projectSketchPoint",{{"sketchId",refs[i].toObject()["sketchId"]},{"uMm",0},{"vMm",0}})["result"].toObject();
                QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,QPoint(qRound(point["x"].toDouble()),qRound(point["y"].toDouble())));
                QTRY_COMPARE(session(window)["inputs"].toObject()["profiles"].toArray().size(),i+1);
            }
            QCOMPARE(session(window)["inputs"].toObject()["profiles"].toArray(),refs);
            QVERIFY(session(window)["geometryEvaluated"].toBool());
            QVERIFY(session(window)["geometry"].toObject()["valid"].toBool());
            QCOMPARE(window.execute("document.inspect")["result"].toObject()["authored"].toObject(),baseline);
            if(commit) {
                dock->widget()->grab().save("build-tutorial-polish/evidence/loft-independent/qt-panel.png");
                QVERIFY2(run(window,"capture.screenshot",{{"path",QDir::currentPath()+"/build-tutorial-polish/evidence/loft-independent/qt-preview.png"}}),error_.constData());
            }
            QTest::mouseClick(buttons->button(commit ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel),Qt::LeftButton);
            QTRY_VERIFY(session(window).isEmpty());
            if(!commit)QCOMPARE(window.execute("document.inspect")["result"].toObject()["authored"].toObject(),baseline);
        }
        const auto bodies=window.execute("document.inspect")["result"].toObject()["evaluation"].toObject()["bodies"].toArray();
        QCOMPARE(bodies.size(),1);QVERIFY(bodies[0].toObject()["valid"].toBool() && bodies[0].toObject()["closed"].toBool());
        QVERIFY(std::abs(bodies[0].toObject()["boundsMm"].toObject()["max"].toArray()[2].toDouble()-58)<1e-5);
    }
    void patternTimelineAxesAndRoofCenterSnaps() {
        AlloyWindow window;window.resize(1280,900);window.show();window.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&window));auto* view=viewport(window);QVERIFY(view);
        auto inspect=[&]{return window.execute("document.inspect")["result"].toObject();};
        auto body=[&]{return inspect()["evaluation"].toObject()["bodies"].toArray()[0].toObject();};
        auto support=[](const QJsonObject& f){return QJsonObject{{"bodyId",f["bodyId"]},{"producerFeatureId",f["producerFeatureId"]},{"faceId",f["faceId"]}};};
        QVERIFY2(run(window,"sketch.begin",{{"plane","XY"}}),error_.constData());
        QVERIFY2(run(window,"sketch.rectangle",{{"x1",0},{"y1",0},{"x2",31.8},{"y2",15.8}}),error_.constData());
        QVERIFY2(run(window,"sketch.finish"),error_.constData());
        auto sketch=inspect()["evaluation"].toObject()["sketches"].toArray().last().toObject();
        QVERIFY2(run(window,"solid.extrude",{{"sketchId",sketch["sketchId"]},{"profileId",sketch["profiles"].toArray()[0].toObject()["profileId"]},{"distance",9.6}}),error_.constData());
        const auto target=body().value("bodyId");QJsonObject cap;
        for(const auto& value:body()["supportFaces"].toArray())if(value.toObject()["role"]=="extrude.endCap")cap=value.toObject();
        QVERIFY(!cap.isEmpty());
        QVERIFY2(run(window,"sketch.begin",{{"support",support(cap)}}),error_.constData());
        QVERIFY2(run(window,"sketch.addCircle",{{"centerX",3.9},{"centerY",3.9},{"diameter",5}}),error_.constData());
        QVERIFY2(run(window,"sketch.finish"),error_.constData());
        sketch=inspect()["evaluation"].toObject()["sketches"].toArray().last().toObject();
        QVERIFY2(run(window,"solid.extrude",{{"sketchId",sketch["sketchId"]},{"profileId",sketch["profiles"].toArray()[0].toObject()["profileId"]},{"distance",1.7},{"operation","join"},{"targetBodyIds",QJsonArray{target}}}),error_.constData());
        const auto baseline=inspect()["authored"].toObject();const auto seed=baseline["features"].toArray().last().toObject().value("id");
        QJsonArray axes;
        for(int axis=0;axis<2;++axis) {
            for(const auto& entry:body()["solidEdges"].toArray()) {
                const auto edge=entry.toObject();if(!edge["available"].toBool())continue;
                const auto start=edge["startMm"].toArray(),end=edge["endMm"].toArray();
                double length2=0;for(int j=0;j<3;++j)length2+=std::pow(end[j].toDouble()-start[j].toDouble(),2);
                const double delta=end[axis].toDouble()-start[axis].toDouble();
                // Use the visible outer top rim, avoiding coincident bottom-edge projections.
                if(std::abs(start[2].toDouble()-9.6)>1e-5 || std::abs(end[2].toDouble()-9.6)>1e-5)continue;
                if(std::abs(delta)>1 && std::abs(delta*delta-length2)<1e-6) {
                    axes.append(QJsonObject{{"edgeId",edge["edgeId"]},{"extent",std::copysign(axis==0 ? 24.0 : 8.0,delta)}});break;
                }
            }
        }
        QCOMPARE(axes.size(),2);QVERIFY2(run(window,"view.home"),error_.constData());
        auto* pattern=window.findChild<QAction*>("solid.action.solid.rectangularPatternDialog");QVERIFY(pattern);
        for(const bool commit:{false,true}) {
            pattern->trigger();QTRY_COMPARE(session(window)["command"].toString(),QString("solid.rectangularPattern"));
            QToolButton* timeline=nullptr;for(auto* button:window.findChildren<QToolButton*>())if(button->property("featureId").toString()==seed.toString())timeline=button;
            QVERIFY(timeline);QTest::mouseClick(timeline,Qt::LeftButton,Qt::NoModifier,QPoint(8,timeline->height()/2));
            QCOMPARE(session(window)["inputs"].toObject()["sourceFeatureId"],seed);
            auto* dock=window.findChild<QDockWidget*>("patternDock");QVERIFY(dock);
            auto* scroll=dock->findChild<QScrollArea*>();QVERIFY(scroll);
            for(int axis=0;axis<2;++axis) {
                auto* choose=dock->findChild<QPushButton*>("patternAxis"+QString::number(axis));QVERIFY(choose);
                scroll->ensureWidgetVisible(choose);QTest::mouseClick(choose,Qt::LeftButton);
                const auto point=window.execute("view.projectEdge",{{"bodyId",target},{"edgeId",axes[axis].toObject()["edgeId"]}})["result"].toObject();
                QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,QPoint(qRound(point["x"].toDouble()),qRound(point["y"].toDouble())));
                QTRY_COMPARE(session(window)["inputs"].toObject()["axes"].toArray().size(),axis+1);
                auto* quantity=dock->findChild<QSpinBox*>("patternQuantity"+QString::number(axis));QVERIFY(quantity);
                scroll->ensureWidgetVisible(quantity);quantity->setFocus();quantity->selectAll();QTest::keyClicks(quantity,axis==0 ? "4" : "2");QTest::keyClick(quantity,Qt::Key_Tab);
                auto* extent=dock->findChild<QDoubleSpinBox*>("patternExtent"+QString::number(axis));QVERIFY(extent);
                scroll->ensureWidgetVisible(extent);extent->setFocus();extent->selectAll();QTest::keyClicks(extent,QString::number(axes[axis].toObject()["extent"].toDouble()));QTest::keyClick(extent,Qt::Key_Return);
            }
            QVERIFY(session(window)["geometryEvaluated"].toBool());QCOMPARE(session(window)["geometry"].toObject()["instanceCount"].toInt(),8);
            QCOMPARE(inspect()["authored"].toObject(),baseline);
            auto* buttons=dock->findChild<QDialogButtonBox*>();QVERIFY(buttons);scroll->ensureWidgetVisible(buttons);
            if(commit)QVERIFY2(run(window,"capture.screenshot",{{"path",QDir::currentPath()+"/build-tutorial-polish/evidence/toy-independent/qt-pattern.png"}}),error_.constData());
            QTest::mouseClick(buttons->button(commit ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel),Qt::LeftButton);
            QTRY_VERIFY(session(window).isEmpty());if(!commit)QCOMPARE(inspect()["authored"].toObject(),baseline);
        }
        const auto patternId=inspect()["authored"].toObject()["features"].toArray().last().toObject().value("id");
        QCOMPARE(body()["instanceCount"].toInt(),8);QJsonObject bottom;
        for(const auto& value:body()["supportFaces"].toArray())if(value.toObject()["role"]=="extrude.base")bottom=value.toObject();
        QVERIFY(!bottom.isEmpty());QVERIFY2(run(window,"solid.shell",{{"targetBodyId",target},{"removedFace",support(bottom)},{"thicknessMm",1.49}}),error_.constData());
        QJsonObject roof;for(const auto& value:body()["supportFaces"].toArray()) {
            const auto f=value.toObject();if(f["role"]=="shell-inner" && std::abs(f["frame"].toObject()["origin"].toArray()[2].toDouble()-8.11)<1e-5)roof=f;
        }
        QVERIFY(!roof.isEmpty());auto* tree=window.findChild<QTreeWidget*>("browser.tree");QVERIFY(tree);QTreeWidgetItem* bottomView=nullptr;
        for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole).toString()=="bottom")bottomView=*it;
        QVERIFY(bottomView);tree->scrollToItem(bottomView);QTest::mouseClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,tree->visualItemRect(bottomView).center());
        QCOMPARE(view->orientation,QString("bottom"));
        const auto frame=roof["frame"].toObject();
        auto roofPixel=[&](double x,double y) {
            const QJsonArray world{x,y,8.11};double uv[2]={};const auto origin=frame["origin"].toArray();
            for(int axis=0;axis<2;++axis){const auto direction=frame[axis==0 ? "uAxis" : "vAxis"].toArray();for(int j=0;j<3;++j)uv[axis]+=(world[j].toDouble()-origin[j].toDouble())*direction[j].toDouble();}
            const auto point=view->projectSketchPoint(uv[0],uv[1],"face",frame);return QPoint(qRound(point["x"].toDouble()),qRound(point["y"].toDouble()));
        };
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,roofPixel(15.9,7.9));
        auto* create=window.findChild<QToolButton*>("command.sketch.create");QVERIFY(create);QTest::mouseClick(create,Qt::LeftButton);
        QTRY_COMPARE(session(window)["command"].toString(),QString("sketch.edit"));
        QCOMPARE(session(window)["inputs"].toObject()["support"].toObject()["faceId"],roof["faceId"]);
        QCOMPARE(session(window)["projectionContext"].toObject()["points"].toArray().size(),8);
        auto* lineAction=window.findChild<QAction*>("sketch.tool.line");QVERIFY(lineAction);lineAction->trigger();QCOMPARE(session(window)["tool"].toString(),QString("line"));
        for(const auto p:{QPointF(3.9,3.9),QPointF(11.9,11.9)}) {
            QTest::mouseMove(view,roofPixel(p.x(),p.y()));QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,roofPixel(p.x(),p.y()));
        }
        auto* select=window.findChild<QPushButton*>("sketch.tool.select");QVERIFY(select);QTest::mouseClick(select,Qt::LeftButton);
        auto parameters=session(window)["draftSketch"].toObject()["parameters"].toObject();
        QCOMPARE(parameters["pointReferences"].toArray().size(),2);QCOMPARE(parameters["entities"].toArray().size(),1);
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,roofPixel(7.9,7.9));auto* construction=window.findChild<QAction*>("sketch.action.constructionToggle");QVERIFY(construction);construction->trigger();
        parameters=session(window)["draftSketch"].toObject()["parameters"].toObject();QVERIFY(parameters["entities"].toArray()[0].toObject()["construction"].toBool());
        const auto guide=parameters["entities"].toArray()[0].toObject().value("id");
        auto* circle=window.findChild<QAction*>("sketch.tool.circle");QVERIFY(circle);circle->trigger();QTest::mouseMove(view,roofPixel(7.9,7.9));QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,roofPixel(7.9,7.9));
        QTest::keyClick(view,Qt::Key_6);auto* value=window.findChild<QLineEdit*>("sketch.inlineValue");QVERIFY(value);QTRY_VERIFY(value->isVisible());
        QTest::keyClicks(value,".3");QTest::keyClick(value,Qt::Key_Return);
        parameters=session(window)["draftSketch"].toObject()["parameters"].toObject();QCOMPARE(parameters["entities"].toArray().size(),2);
        bool midpoint=false;for(const auto& c:parameters["constraints"].toArray())if(c.toObject()["type"]=="midpoint" && c.toObject()["entityIds"].toArray()==QJsonArray{guide})midpoint=true;
        QVERIFY(midpoint);QVERIFY(!parameters["entities"].toArray()[1].toObject()["construction"].toBool());
        QVERIFY2(run(window,"capture.screenshot",{{"path",QDir::currentPath()+"/build-tutorial-polish/evidence/toy-independent/qt-roof-midpoint.png"}}),error_.constData());
        QVERIFY2(run(window,"sketch.cancel"),error_.constData());
        // Pending numeric text must not call into destroyed window state during QWidget teardown.
        QVERIFY2(run(window,"solid.rectangularPatternDialog",{{"featureId",patternId}}),error_.constData());
        auto* pending=window.findChild<QDoubleSpinBox*>("patternExtent0");QVERIFY(pending);pending->setFocus();pending->selectAll();QTest::keyClicks(pending,"23");
        QCOMPARE(pending->value(),axes[0].toObject()["extent"].toDouble());
    }
    void moveActionsAtShortHeights() {
        AlloyWindow window;window.resize(1136,790);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QVERIFY2(run(window,"solid.cylinder",{{"radius",3},{"height",40},{"units","mm"}}),error_.constData());
        const auto authored=window.execute("document.inspect")["result"].toObject()["authored"].toObject();
        const auto body=authored["features"].toArray()[0].toObject()["bodyId"];
        for(const auto size:{QSize(1136,790),QSize(1040,640)}) {
            window.resize(size);
            QVERIFY2(run(window,"solid.moveDialog",{{"targetBodyId",body}}),error_.constData());
            QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
            QCoreApplication::processEvents();
            auto* scroll=window.findChild<QScrollArea*>("move.formScroll");QVERIFY(scroll);
            auto* pivot=window.findChild<QDoubleSpinBox*>("pivotZMm");QVERIFY(pivot);
            auto* ok=window.findChild<QPushButton*>("move.ok");QVERIFY(ok);
            auto* cancel=window.findChild<QPushButton*>("move.cancel");QVERIFY(cancel);
            QTRY_VERIFY(window.rect().contains(QRect(ok->mapToGlobal(QPoint())-window.mapToGlobal(QPoint()),ok->size())));
            QVERIFY(window.rect().contains(QRect(cancel->mapToGlobal(QPoint())-window.mapToGlobal(QPoint()),cancel->size())));
            const auto actions=QRect(cancel->mapToGlobal(QPoint())-window.mapToGlobal(QPoint()),cancel->size());
            scroll->ensureWidgetVisible(pivot);QCoreApplication::processEvents();
            QVERIFY(scroll->viewport()->rect().contains(QRect(pivot->mapTo(scroll->viewport(),QPoint()),pivot->size())));
            QCOMPARE(QRect(cancel->mapToGlobal(QPoint())-window.mapToGlobal(QPoint()),cancel->size()),actions);
            if(size.height()==640)QVERIFY(scroll->verticalScrollBar()->maximum()>0);
            QTest::mouseClick(cancel,Qt::LeftButton);QTRY_VERIFY(session(window).isEmpty());
            QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
            QCOMPARE(window.execute("document.inspect")["result"].toObject()["authored"].toObject(),authored);
        }
        QVERIFY2(run(window,"solid.moveDialog",{{"targetBodyId",body}}),error_.constData());
        QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
        QVERIFY2(run(window,"solid.moveHandle",{{"field","zMm"},{"value",2}}),error_.constData());
        auto* ok=window.findChild<QPushButton*>("move.ok");QVERIFY(ok);
        QTest::mouseClick(ok,Qt::LeftButton);QTRY_VERIFY(session(window).isEmpty());
        QCOMPARE(window.state()["document"].toObject()["features"].toInt(),2);
    }
    void moveEscapeRepeatedSessions() {
        AlloyWindow window;window.resize(1180,760);window.show();window.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&window));auto* view=viewport(window);QVERIFY(view);
        QVERIFY2(run(window,"solid.cylinder",{{"radius",3},{"height",40},{"z",11},{"units","mm"}}),error_.constData());
        const auto inspected=window.execute("document.inspect")["result"].toObject();
        const auto authored=inspected["authored"].toObject();const auto body=authored["features"].toArray()[0].toObject()["bodyId"];
        for(int i=0;i<3;++i){
            QVERIFY2(run(window,"solid.moveDialog",{{"targetBodyId",body}}),error_.constData());
            QVERIFY2(run(window,"solid.moveHandle",{{"field","zMm"},{"value",2}}),error_.constData());
            QVERIFY2(run(window,"solid.moveApplyGesture"),error_.constData());
            view->setFocus();QTest::keyClick(view,Qt::Key_Escape);QCoreApplication::processEvents();
            QVERIFY(session(window).isEmpty());
            QCOMPARE(window.execute("document.inspect")["result"].toObject()["authored"].toObject(),authored);
        }
    }
    void circlePrismBodyPicking() {
        AlloyWindow window;window.resize(1136,790);window.show();window.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&window));auto* view=viewport(window);QVERIFY(view);
        QVERIFY2(run(window,"solid.box",{{"width",10},{"depth",10},{"height",10},{"x",20},{"y",0},{"z",0},{"units","mm"}}),error_.constData());
        QVERIFY2(run(window,"sketch.begin",{{"plane","XY"}}),error_.constData());QVERIFY2(run(window,"sketch.addCircle",{{"centerX",7.5},{"centerY",-5},{"diameter",5.2},{"units","mm"}}),error_.constData());QVERIFY2(run(window,"sketch.finish"),error_.constData());
        auto inspected=window.execute("document.inspect")["result"].toObject();const auto sketch=inspected["evaluation"].toObject()["sketches"].toArray().last().toObject();
        QVERIFY2(run(window,"solid.extrude",{{"sketchId",sketch["sketchId"]},{"profileId",sketch["profiles"].toArray().first().toObject()["profileId"]},{"distance",47},{"units","mm"}}),error_.constData());QVERIFY2(run(window,"view.home"),error_.constData());
        const auto bodies=window.execute("document.inspect")["result"].toObject()["evaluation"].toObject()["bodies"].toArray();const auto stock=bodies[0].toObject()["bodyId"].toString(),cutter=bodies[1].toObject()["bodyId"].toString();
        const auto point=window.execute("view.projectBody",{{"bodyId",cutter}})["result"].toObject();QCOMPARE(view->pickBody(point["x"].toDouble(),point["y"].toDouble()),cutter);
        QVERIFY2(run(window,"selection.pick",{{"x",point["x"]},{"y",point["y"]}}),error_.constData());QCOMPARE(window.state()["selection"].toObject()["bodyId"].toString(),cutter);
        QVERIFY2(run(window,"solid.combineDialog",{{"targetBodyId",stock}}),error_.constData());QVERIFY2(run(window,"solid.combineSelectionMode",{{"role","tools"}}),error_.constData());const auto toolPoint=window.execute("view.projectBody",{{"bodyId",cutter}})["result"].toObject();QVERIFY2(run(window,"selection.pick",{{"x",toolPoint["x"]},{"y",toolPoint["y"]}}),error_.constData());QVERIFY(session(window)["inputs"].toObject()["toolBodyIds"].toArray().contains(cutter));QVERIFY2(run(window,"command.cancel",{{"session",session(window)["id"]}}),error_.constData());
    }
    void popupCaptureUsesGlobalCoordinates() {
        AlloyWindow window;window.setGeometry(160,120,1180,760);window.show();window.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&window));auto* view=viewport(window);QVERIFY(view);
        QMenu menu(view);menu.setObjectName("capture.fixturePopup");menu.setStyleSheet("QMenu {background:#b91726; color:white; border:0; padding:8px;} QMenu::item {background:#b91726; padding:8px;}");menu.addAction("Capture fixture");
        menu.popup(view->mapToGlobal(QPoint(220,180)));QCoreApplication::processEvents();QVERIFY(menu.isVisible());
        const auto global=menu.mapToGlobal(QPoint()),local=global-window.mapToGlobal(QPoint());
        QVERIFY(local!=menu.mapTo(&window,QPoint()));
        const auto popupState=window.state()["ui"].toObject()["ownedPopups"].toArray();QVERIFY(!popupState.isEmpty());
        QTemporaryDir temporary;const auto path=temporary.filePath("popup.png");QVERIFY2(run(window,"capture.screenshot",{{"path",path}}),error_.constData());
        const QImage pixels(path);QVERIFY(!pixels.isNull());const double scale=double(pixels.width())/window.width();
        const auto color=pixels.pixelColor(qRound((local.x()+4)*scale),qRound((local.y()+4)*scale));
        QVERIFY2(color.red()>120 && color.green()<70,QColor(color).name().toUtf8().constData());menu.close();
    }
    void planeCircleDimension() {
        AlloyWindow window;window.resize(1180,760);window.show();window.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto* view = viewport(window);QVERIFY(view);
        QVERIFY2(run(window,"sketch.create",{{"name","Widget circle"}}),error_.constData());
        auto* plane = window.findChild<QPushButton*>("sketch.plane.XY");QVERIFY(plane);
        QTest::mouseClick(plane,Qt::LeftButton);
        QTRY_VERIFY(QApplication::focusWidget() == view);
        // Send to the actual focus owner without manually focusing the canvas.
        QTest::keyClick(QApplication::focusWidget(),Qt::Key_C);
        QTRY_COMPARE(session(window)["tool"].toString(),QString("circle"));
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,sketchPixel(view,0,0));
        QTest::mouseMove(view,sketchPixel(view,5,0));
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,sketchPixel(view,5,0));
        QTRY_COMPARE(session(window)["draftSketch"].toObject()["parameters"].toObject()["entities"].toArray().size(),1);
        auto parameters = session(window)["draftSketch"].toObject()["parameters"].toObject();
        QCOMPARE(parameters["entities"].toArray()[0].toObject()["type"].toString(),QString("circle"));
        QTest::keyClick(QApplication::focusWidget(),Qt::Key_Escape);
        QTRY_COMPARE(session(window)["tool"].toString(),QString("select"));
        // Use the authored radius after pixel rounding, away from its center point.
        const auto entity = parameters["entities"].toArray()[0].toObject();
        const auto points = parameters["points"].toArray();
        QJsonObject center;
        for (const auto& p : points) if (p.toObject()["id"] == entity["centerPointId"]) center = p.toObject();
        QVERIFY(!center.isEmpty());
        const auto edge = sketchPixel(view,center["uMm"].toDouble(),center["vMm"].toDouble()+entity["radiusMm"].toDouble());
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,edge);
        QTRY_COMPARE(session(window)["selection"].toObject()["entityIds"].toArray().size(),1);
        QTest::keyClick(QApplication::focusWidget(),Qt::Key_D);
        const auto placement = sketchPixel(view,9,8);QTest::mouseMove(view,placement);
        QTRY_COMPARE(session(window)["pendingDimension"].toObject()["kind"].toString(),QString("diameter"));
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,placement);
        auto* value = window.findChild<QLineEdit*>("sketch.inlineValue");QVERIFY(value);
        QTRY_VERIFY(value->isVisible());QTRY_VERIFY(QApplication::focusWidget() == value);
        value->selectAll();QTest::keyClicks(value,"10");QTest::keyClick(value,Qt::Key_Return);
        QTRY_COMPARE(session(window)["draftSketch"].toObject()["parameters"].toObject()["dimensions"].toArray().size(),1);
        QVERIFY2(run(window,"sketch.selectionSet",{{"entityIds",QJsonArray{entity["id"]}}}),error_.constData());
        auto* dimensions=window.findChild<QComboBox*>("sketch.dimension.list");
        auto* edit=window.findChild<QDoubleSpinBox*>("sketch.dimension.edit");
        auto* apply=window.findChild<QPushButton*>("sketch.dimension.apply");
        QVERIFY(dimensions && edit && apply);QVERIFY(edit->isEnabled());QVERIFY(apply->isEnabled());
        QCOMPARE(edit->value(),10.0);edit->setValue(12);
        auto params=[&]{return session(window)["draftSketch"].toObject()["parameters"].toObject();};
        QCOMPARE(params()["entities"].toArray()[0].toObject()["radiusMm"].toDouble(),5.0);
        QTest::mouseClick(apply,Qt::LeftButton);
        QCOMPARE(params()["dimensions"].toArray().size(),1);
        QVERIFY(std::abs(params()["entities"].toArray()[0].toObject()["radiusMm"].toDouble()-6)<1e-6);
        QVERIFY2(run(window,"sketch.dimension",{{"kind","radius"},{"entityId",entity["id"]},{"driving",false}}),error_.constData());
        const auto drivenId=params()["dimensions"].toArray().last().toObject()["id"].toString();
        dimensions->setCurrentIndex(dimensions->findData(drivenId));
        QVERIFY(!edit->isEnabled());QVERIFY(!apply->isEnabled());
        QVERIFY2(run(window,"sketch.line",{{"x1",20},{"y1",0},{"x2",30},{"y2",0},{"units","mm"}}),error_.constData());
        const auto line=params()["entities"].toArray().last().toObject();
        QVERIFY2(run(window,"sketch.dimension",{{"kind","linear"},{"pointIds",QJsonArray{line["endPointId"],line["startPointId"]}},{"value",10},{"driving",true}}),error_.constData());
        const auto lineDimension=params()["dimensions"].toArray().last().toObject()["id"].toString();
        QVERIFY2(run(window,"sketch.selectionSet",{{"entityIds",QJsonArray{line["id"]}}}),error_.constData());
        QCOMPARE(dimensions->currentData().toString(),lineDimension);QVERIFY(apply->isEnabled());
        QVERIFY2(run(window,"sketch.cancel"),error_.constData());
        QCOMPARE(window.state()["document"].toObject()["features"].toInt(),0);
    }

    void sketchRowsSurviveDraftUndoRedo() {
        AlloyWindow window;window.resize(1180,760);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QVERIFY2(run(window,"sketch.begin",{{"plane","XY"},{"name","Stable rows"}}),error_.constData());
        auto* tree=window.findChild<QTreeWidget*>("sketch.selection");QVERIFY(tree);
        auto* points=tree->topLevelItem(0);auto* curves=tree->topLevelItem(1);
        QVERIFY(points && curves);
        auto* accessible=QAccessible::queryAccessibleInterface(tree);QVERIFY(accessible);
        auto accessibleNames=[&]{QStringList result;for(int i=0;i<accessible->childCount();++i)
            if(auto* child=accessible->child(i))result.append(child->text(QAccessible::Name));return result;};
        accessibleNames(); // Prime Qt's logical-index cache before row insertion.
        QVERIFY2(run(window,"sketch.line",{{"x1",0},{"y1",0},{"x2",20},{"y2",0},{"units","mm"}}),error_.constData());
        const auto line=session(window)["draftSketch"].toObject()["parameters"].toObject()["entities"].toArray().first().toObject();
        const auto lineId=line["id"].toString();
        auto* lineRow=curves->child(0);QVERIFY(lineRow);
        QCOMPARE(lineRow->data(0,Qt::UserRole+1).toString(),lineId);
        auto* firstPoint=points->child(0);QVERIFY(firstPoint);
        QVERIFY2(run(window,"sketch.selectionSet",{{"entityIds",QJsonArray{lineId}}}),error_.constData());
        QVERIFY(lineRow->isSelected());
        QVERIFY(accessibleNames().join(" ").contains("Line 1"));
        QVERIFY2(run(window,"history.undo"),error_.constData());
        QCOMPARE(tree->topLevelItem(0),points);QCOMPARE(tree->topLevelItem(1),curves);
        QCOMPARE(curves->child(0),lineRow);QCOMPARE(points->child(0),firstPoint);
        QVERIFY(lineRow->isHidden());QVERIFY(!lineRow->isSelected());
        QCoreApplication::processEvents();
        QVERIFY(!accessibleNames().join(" ").contains("Line 1"));
        QVERIFY2(run(window,"history.redo"),error_.constData());
        QCOMPARE(curves->child(0),lineRow);QCOMPARE(points->child(0),firstPoint);
        QVERIFY(!lineRow->isHidden());
        QCOMPARE(lineRow->data(0,Qt::UserRole+1).toString(),lineId);
        QCoreApplication::processEvents();
        QVERIFY(accessibleNames().join(" ").contains("Line 1"));
        QVERIFY2(run(window,"sketch.line",{{"x1",20},{"y1",0},{"x2",30},{"y2",10},{"units","mm"}}),error_.constData());
        QCoreApplication::processEvents();
        QVERIFY(accessibleNames().join(" ").contains("Line 2"));
        QVERIFY2(run(window,"history.undo"),error_.constData());
        QCoreApplication::processEvents();
        const auto remaining=accessibleNames().join(" ");
        QVERIFY(remaining.contains("Line 1"));QVERIFY(!remaining.contains("Line 2"));
        QVERIFY2(run(window,"sketch.cancel"),error_.constData());
    }

    void sketchGroupedMenuCommands() {
        AlloyWindow window;window.resize(1180,760);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QVERIFY2(run(window,"sketch.begin",{{"plane","XY"},{"name","Grouped commands"}}),error_.constData());
        auto* create=window.findChild<QMenu*>("sketch.commands.create");QVERIFY(create);
        auto* circle=window.findChild<QMenu*>("sketch.commands.circle");QVERIFY(circle);
        QVERIFY(create->actions().contains(circle->menuAction()));
        for(const auto& tool:QStringList{"line","rectangle","circle","centerArc","threePointArc","dimension","fillet"}) {
            auto* action=window.findChild<QAction*>("sketch.tool."+tool);QVERIFY(action);
            action->trigger();QCOMPARE(session(window)["tool"].toString(),tool);
        }
        // Fixed and Coincident use point targets, as in the palette; the
        // remaining named constraints use the same authoritative tool command.
        for(const auto& type:QStringList{"coincident","fixed","horizontalVertical","horizontal","vertical","parallel","perpendicular","equal"}) {
            auto* action=window.findChild<QAction*>("sketch.action.constraint."+type);QVERIFY(action);
            action->trigger();QCOMPARE(session(window)["tool"].toString(),QString("constraint"));
            QCOMPARE(session(window)["constraintType"].toString(),type);
        }
        QVERIFY2(run(window,"sketch.tool",{{"tool","select"}}),error_.constData());
        QVERIFY2(run(window,"sketch.line",{{"x1",0},{"y1",0},{"x2",20},{"y2",2},{"units","mm"}}),error_.constData());
        auto parameters=session(window)["draftSketch"].toObject()["parameters"].toObject();
        const auto edge=parameters["entities"].toArray().first().toObject();
        QVERIFY2(run(window,"sketch.selectionSet",{{"pointIds",QJsonArray{edge["startPointId"]}}}),error_.constData());
        window.findChild<QAction*>("sketch.action.constraint.fixed")->trigger();
        parameters=session(window)["draftSketch"].toObject()["parameters"].toObject();
        QCOMPARE(parameters["constraints"].toArray().last().toObject()["type"].toString(),QString("fixed"));
        QVERIFY2(run(window,"sketch.tool",{{"tool","select"}}),error_.constData());
        QVERIFY2(run(window,"sketch.selectionSet",{{"entityIds",QJsonArray{edge["id"]}}}),error_.constData());
        auto* toggle=window.findChild<QAction*>("sketch.action.constructionToggle");QVERIFY(toggle);
        const auto historyIndex=session(window)["draftHistory"].toObject()["index"].toInt();
        toggle->trigger();
        parameters=session(window)["draftSketch"].toObject()["parameters"].toObject();
        QVERIFY(parameters["entities"].toArray().first().toObject()["construction"].toBool());
        QCOMPARE(session(window)["draftHistory"].toObject()["index"].toInt(),historyIndex+1);
        toggle->trigger();
        QVERIFY(!session(window)["draftSketch"].toObject()["parameters"].toObject()["entities"].toArray().first().toObject()["construction"].toBool());
        QVERIFY2(run(window,"sketch.cancel"),error_.constData());
        auto* solidModify=window.findChild<QMenu*>("solid.commands.modify");QVERIFY(solidModify);
        auto* pressPull=window.findChild<QAction*>("solid.action.solid.pressPullDialog");QVERIFY(pressPull);
        QVERIFY(solidModify->actions().contains(pressPull));
        pressPull->trigger();QCOMPARE(session(window)["command"].toString(),QString("solid.extrude"));
        QVERIFY2(run(window,"command.cancel",{{"session",session(window).value("id").toString()}}),error_.constData());
    }
    void sketchConstraintBadgeInteraction() {
        AlloyWindow window;window.resize(1180,760);window.show();window.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto* view=viewport(window);QVERIFY(view);
        QVERIFY2(run(window,"sketch.begin",{{"plane","XY"}}),error_.constData());
        QVERIFY2(run(window,"sketch.line",{{"x1",-20},{"y1",0},{"x2",20},{"y2",2},{"units","mm"}}),error_.constData());
        auto params=[&]{return session(window)["draftSketch"].toObject()["parameters"].toObject();};
        const auto line=params()["entities"].toArray().first().toObject();
        QVERIFY2(run(window,"sketch.constraint",{{"type","horizontal"},{"entityIds",QJsonArray{line["id"]}}}),error_.constData());
        QVERIFY2(run(window,"sketch.dimension",{{"kind","linear"},{"pointIds",QJsonArray{line["startPointId"],line["endPointId"]}},{"value",40},{"driving",true}}),error_.constData());
        const auto original=params(),relation=original["constraints"].toArray().last().toObject();
        const auto dimension=original["dimensions"].toArray().last().toObject().value("id").toString();
        const auto dof=session(window)["solverDiagnostics"].toObject()["dof"].toInt();
        const auto history=session(window)["draftHistory"].toObject()["index"].toInt();
        auto glyphs=[&]{return window.state()["viewport"].toObject()["gpu"].toObject()["sketchConstraintGlyphs"].toArray();};
        QTRY_VERIFY(!glyphs().isEmpty());
        auto bounds=glyphs().first().toObject()["bounds"].toObject();
        const QPoint badge(qRound(bounds["x"].toDouble()+bounds["width"].toDouble()/2),qRound(bounds["y"].toDouble()+bounds["height"].toDouble()/2));
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,badge);
        QCOMPARE(session(window)["selection"].toObject()["constraintIds"].toArray(),QJsonArray{relation["id"]});
        // Text editing must consume Delete/Backspace without deleting CAD relations.
        auto* name=window.findChild<QLineEdit*>("sketch.name");QVERIFY(name);
        const auto originalName=name->text();name->setFocus();name->setText("abcd");name->setCursorPosition(2);
        QTest::keyClick(name,Qt::Key_Backspace);QCOMPARE(name->text(),QString("acd"));
        QTest::keyClick(name,Qt::Key_Delete);QCOMPARE(name->text(),QString("ad"));
        QCOMPARE(params(),original);
        name->setText(originalName);view->setFocus();QTest::keyClick(view,Qt::Key_Backspace);
        QTRY_COMPARE(params()["constraints"].toArray().size(),original["constraints"].toArray().size()-1);
        QCOMPARE(session(window)["solverDiagnostics"].toObject()["dof"].toInt(),dof+1);
        QCOMPARE(session(window)["draftHistory"].toObject()["index"].toInt(),history+1);
        QVERIFY2(run(window,"history.undo"),error_.constData());QCOMPARE(params(),original);
        QVERIFY2(run(window,"history.redo"),error_.constData());
        QVERIFY2(run(window,"history.undo"),error_.constData());QCOMPARE(params(),original);
        QVERIFY2(run(window,"sketch.constraintSelect",{{"constraintId",relation["id"]}}),error_.constData());
        QVERIFY2(run(window,"sketch.displayOptions",{{"showConstraints",false}}),error_.constData());
        QVERIFY(glyphs().isEmpty());QVERIFY(view->pickConstraint(badge.x(),badge.y()).isEmpty());
        QVERIFY(session(window)["selection"].toObject()["constraintIds"].toArray().isEmpty());
        QVERIFY(!run(window,"sketch.deleteSelection"));QCOMPARE(params(),original);
        QVERIFY2(run(window,"sketch.annotationSelect",{{"dimensionId",dimension}}),error_.constData());
        QVERIFY2(run(window,"sketch.displayOptions",{{"showDimensions",false}}),error_.constData());
        QVERIFY(session(window)["selection"].toObject().value("dimensionId").toString().isEmpty());
        QCOMPARE(session(window)["draftHistory"].toObject()["index"].toInt(),history);
        QVERIFY2(run(window,"sketch.displayOptions",{{"showConstraints",true},{"showDimensions",true}}),error_.constData());
        QVERIFY2(run(window,"sketch.constraintHover",{{"constraintId",relation["id"]}}),error_.constData());
        QEvent leave(QEvent::Leave);QApplication::sendEvent(view,&leave);
        QVERIFY(session(window).value("hoveredConstraintId").toString().isEmpty());
        QVERIFY(!run(window,"sketch.deleteSelection",{{"constraintIds",QJsonArray{relation["id"],"stale"}}}));QCOMPARE(params(),original);
        QVERIFY2(run(window,"sketch.deleteSelection",{{"dimensionId",dimension}}),error_.constData());
        QVERIFY(params()["dimensions"].toArray().isEmpty());
        QVERIFY2(run(window,"history.undo"),error_.constData());QCOMPARE(params(),original);
        QVERIFY2(run(window,"sketch.cancel"),error_.constData());
    }
    void pressPullProfileModifierToggle() {
        AlloyWindow window;window.resize(1180,760);window.show();window.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto* view=viewport(window);QVERIFY(view);
        QVERIFY2(run(window,"sketch.begin",{{"plane","XY"},{"name","Profile toggle"}}),error_.constData());
        QVERIFY2(run(window,"sketch.addCircle",{{"centerX",0},{"centerY",0},{"diameter",16},{"units","mm"}}),error_.constData());
        QVERIFY2(run(window,"sketch.finish"),error_.constData());
        const auto before=window.state()["document"].toObject();
        const auto sketch=before["evaluation"].toObject()["sketches"].toArray().first().toObject();
        const auto profiles=sketch["profiles"].toArray();QCOMPARE(profiles.size(),1);
        const auto profileId=profiles.first().toObject()["profileId"].toString();QVERIFY(!profileId.isEmpty());
        const QJsonArray selected{profileId};
        QVERIFY2(run(window,"solid.pressPullDialog",{{"sketchId",sketch["sketchId"]}}),error_.constData());
        // Pick inside the real face, away from the centroid's extrusion arrow.
        const auto pick=sketchPixel(view,4,0);QVERIFY(view->rect().contains(pick));
        QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pick);
        QTRY_COMPARE(window.state()["selection"].toObject()["profileIds"].toArray(),selected);
        const auto sessionId=session(window).value("id").toString();
        QVERIFY2(run(window,"command.update",{{"session",sessionId},{"inputs",QJsonObject{{"distance",5},{"units","mm"}}}}),error_.constData());
        QVERIFY2(run(window,"command.preview",{{"session",sessionId}}),error_.constData());
        // Qt maps ControlModifier to physical Command on macOS by default;
        // Alloy does not set AA_MacDontSwapCtrlAndMeta. Elsewhere it is Control.
        const auto modifier=Qt::ControlModifier;
        // Each synthesized modified click must toggle exactly once, even after
        // distance entry has enabled the disposable solid preview.
        QTest::mouseClick(view,Qt::LeftButton,modifier,sketchPixel(view,4,0));
        QTRY_COMPARE(session(window)["inputs"].toObject()["profileIds"].toArray(),QJsonArray{});
        QVERIFY(window.state()["selection"].toObject()["profileIds"].toArray().isEmpty());
        QCOMPARE(session(window)["inputs"].toObject()["distance"].toDouble(),5.0);
        QTest::mouseClick(view,Qt::LeftButton,modifier,sketchPixel(view,4,0));
        QTRY_COMPARE(session(window)["inputs"].toObject()["profileIds"].toArray(),selected);
        QCOMPARE(window.state()["selection"].toObject()["profileIds"].toArray(),selected);
        QCOMPARE(session(window)["inputs"].toObject()["distance"].toDouble(),5.0);
        QVERIFY2(run(window,"command.preview",{{"session",sessionId}}),error_.constData());
        QCOMPARE(window.state()["document"].toObject()["authored"],before["authored"]);
        QCOMPARE(window.state()["document"].toObject()["revision"],before["revision"]);
        QVERIFY2(run(window,"command.cancel",{{"session",sessionId}}),error_.constData());
    }
    void combineAccessibleSelectionRole() {
        AlloyWindow window;window.resize(1180,760);window.show();window.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QVERIFY2(run(window,"solid.box",{{"width",30},{"depth",20},{"height",5}}),error_.constData());
        const auto authored=window.state()["document"].toObject()["authored"].toObject();
        const auto body=authored["features"].toArray()[0].toObject()["bodyId"];
        QVERIFY2(run(window,"solid.combineDialog",{{"targetBodyId",body}}),error_.constData());
        auto* target=window.findChild<QPushButton*>("combine.pickTarget");
        auto* tools=window.findChild<QPushButton*>("combine.pickTools");QVERIFY(target);QVERIFY(tools);
        auto* accessible=QAccessible::queryAccessibleInterface(target);QVERIFY(accessible);QVERIFY(accessible->actionInterface());
        QCOMPARE(session(window)["selectionRole"].toString(),QString("tools"));
        accessible->actionInterface()->doAction(QAccessibleActionInterface::toggleAction());
        QCOMPARE(session(window)["selectionRole"].toString(),QString("target"));
        QVERIFY(target->isChecked());QVERIFY(!tools->isChecked());
        // An active role cannot be unchecked into a misleading no-role state.
        accessible->actionInterface()->doAction(QAccessibleActionInterface::toggleAction());
        QVERIFY(target->isChecked());
        QTest::mouseClick(tools,Qt::LeftButton);
        QCOMPARE(session(window)["selectionRole"].toString(),QString("tools"));
        QVERIFY(!target->isChecked());QVERIFY(tools->isChecked());
        QVERIFY2(run(window,"command.cancel",{{"session",session(window)["id"]}}),error_.constData());
        QCOMPARE(window.state()["document"].toObject()["authored"].toObject(),authored);
    }
    void combineCheckboxAndCommit() {
        AlloyWindow window;window.resize(1180,640);window.show();window.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QVERIFY2(run(window,"solid.box",{{"width",30},{"depth",20},{"height",5},{"units","mm"},{"name","Plate"}}),error_.constData());
        QVERIFY2(run(window,"solid.cylinder",{{"radius",3},{"height",10},{"x",15},{"y",10},{"z",-2},{"units","mm"},{"name","Tool"}}),error_.constData());
        const auto features = window.state()["document"].toObject()["authored"].toObject()["features"].toArray();
        const auto target = features[0].toObject()["bodyId"].toString();
        const auto tool = features[1].toObject()["bodyId"].toString();
        const QJsonObject inputs{{"targetBodyId",target},{"toolBodyIds",QJsonArray{}},{"operation","cut"},{"keepTools",false}};
        QVERIFY2(run(window,"command.begin",{{"command","solid.combine"},{"inputs",inputs}}),error_.constData());
        auto* tree = window.findChild<QTreeWidget*>("browser.tree");QVERIFY(tree);
        QTreeWidgetItem* toolItem = nullptr;
        for (QTreeWidgetItemIterator it(tree); *it; ++it)
            if ((*it)->data(0,Qt::UserRole+1).toString() == tool) {toolItem = *it;break;}
        QVERIFY(toolItem);tree->scrollToItem(toolItem);
        const auto row = tree->visualItemRect(toolItem);QVERIFY(!row.isEmpty());
        // Checkbox change followed by itemClicked used to access a deleted row.
        QTest::mouseClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,checkbox(tree,row,toolItem->checkState(0)));
        QTRY_VERIFY(window.state()["document"].toObject()["revision"].toInteger() > 2);
        QCOMPARE(session(window)["inputs"].toObject()["toolBodyIds"].toArray().size(),0);
        QCOMPARE(session(window)["baseDocumentRevision"].toInteger(),window.state()["document"].toObject()["revision"].toInteger());
        QVERIFY2(run(window,"feature.visibility",{{"featureId",features[1].toObject()["id"]},{"visible",true}}),error_.constData());
        auto* tools = window.findChild<QListWidget*>("combine.toolBodies");QVERIFY(tools);
        QListWidgetItem* item = nullptr;
        for (int i = 0;i < tools->count();++i) if (tools->item(i)->data(Qt::UserRole).toString() == tool) item = tools->item(i);
        QVERIFY(item);tools->scrollToItem(item);
        QTest::mouseClick(tools->viewport(),Qt::LeftButton,Qt::NoModifier,checkbox(tools,tools->visualItemRect(item),item->checkState()));
        QTRY_VERIFY(session(window)["previewed"].toBool());
        for(auto* scroll:window.findChildren<QScrollArea*>())
            QVERIFY(scroll->widget()->palette().color(QPalette::Window).lightness()>200);
        auto* keep = window.findChild<QCheckBox*>("combine.keepTools");QVERIFY(keep);
        // The form may scroll on this short window; selection remains through real events.
        keep->setFocus();QTest::keyClick(keep,Qt::Key_Space);
        QTRY_VERIFY(session(window)["inputs"].toObject()["keepTools"].toBool());
        auto* ok = window.findChild<QPushButton*>("combine.ok");QVERIFY(ok);
        auto* cancel = window.findChild<QPushButton*>("combine.cancel");QVERIFY(cancel);
        QTRY_VERIFY(ok->isEnabled());QVERIFY(ok->isVisible());QVERIFY(cancel->isVisible());
        QVERIFY(window.rect().contains(QRect(ok->mapTo(&window,QPoint()),ok->size())));
        QVERIFY(window.rect().contains(QRect(cancel->mapTo(&window,QPoint()),cancel->size())));
        QTest::mouseClick(ok,Qt::LeftButton);
        QTRY_VERIFY(session(window).isEmpty());
        const auto document = window.state()["document"].toObject();
        QCOMPARE(document["features"].toInt(),3);QCOMPARE(document["bodies"].toInt(),2);
        const auto bodies = document["evaluation"].toObject()["bodies"].toArray();
        bool found = false;
        for (const auto& body : bodies) if (body.toObject()["bodyId"] == target) {
            found = true;QVERIFY(std::abs(body.toObject()["volumeMm3"].toDouble()-(3000-45*std::acos(-1.0))) < 1e-6);
        }
        QVERIFY(found);
        QVERIFY2(run(window,"history.undo"),error_.constData());
        QCOMPARE(window.state()["document"].toObject()["features"].toInt(),2);
    }
};
QTEST_MAIN(WidgetInteractions)
#include "widget_interactions.moc"
