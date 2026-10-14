// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QDockWidget>
#include "ReferenceView.h"
#include <QMouseEvent>
#include <QToolButton>
#include <cmath>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QJsonDocument>
#include <QMenu>
#include <QMimeData>
#include <QPointer>
#include <QTest>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>

class ComponentClipboardControlsTest final : public QObject {
    Q_OBJECT
    class ClipboardRestore final {
        QClipboard* clipboard_;
        QMimeData* saved_=new QMimeData;
    public:
        explicit ClipboardRestore(QClipboard* clipboard):clipboard_(clipboard) {
            const auto* original=clipboard_->mimeData();
            for(const auto& format:original->formats())saved_->setData(format,original->data(format));
        }
        ~ClipboardRestore() {clipboard_->setMimeData(saved_);}
    };
    QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& args={}) {
        const auto response=window.execute(command,args);
        if(!response["ok"].toBool())qFatal("%s: %s",qPrintable(command),QJsonDocument(response).toJson().constData());
        return response["result"].toObject();
    }
    static QTreeWidgetItem* row(QTreeWidget* tree,const QString& id) {
        for(QTreeWidgetItemIterator it(tree);*it;++it)
            if((*it)->data(0,Qt::UserRole+8).toString()==id
                && !(*it)->data(0,Qt::UserRole+3).toString().isEmpty() && !(*it)->isHidden())return *it;
        return nullptr;
    }
    static QDockWidget* visibleCopyDialog(AlloyWindow& window) {
        for(auto* dialog:window.findChildren<QDockWidget*>("component.copy.dialog"))if(dialog->isVisible())return dialog;
        return nullptr;
    }
    static void browserCopy(QTreeWidget* tree,QTreeWidgetItem* item) {
        tree->scrollToItem(item);QApplication::processEvents();
        const auto point=tree->visualItemRect(item).center();
        if(!tree->viewport()->rect().contains(point))qFatal("Browser Copy row offscreen");
        QTimer::singleShot(40,tree,[]{
            auto* menu=qobject_cast<QMenu*>(QApplication::activePopupWidget());
            if(!menu)qFatal("Browser Copy context menu missing");
            for(auto* action:menu->actions())if(action->objectName()=="context.copyOccurrence"){
                QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,menu->actionGeometry(action).center());return;
            }
            qFatal("Browser Copy action missing");
        });
        QContextMenuEvent event(QContextMenuEvent::Mouse,point,tree->viewport()->mapToGlobal(point));
        QApplication::sendEvent(tree->viewport(),&event);
    }
private slots:
    void copyPastePlacementAndDocumentBoundary() {
        const auto source=qEnvironmentVariable("ALLOY_ASM019_SOURCE");
        QVERIFY(!source.isEmpty());
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1510,920);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));window.activateWindow();QApplication::setActiveWindow(&window);
        call(window,"document.open",{{"path",source},{"discard",true}});
        const auto starting=call(window,"document.inspect"),authored=starting["authored"].toObject();
        QString parentDefinition,parentOccurrence;
        for(const auto& value:authored["components"].toArray())
            if(value.toObject()["name"]=="Piston subassembly")parentDefinition=value.toObject()["id"].toString();
        for(const auto& value:authored["occurrences"].toArray())
            if(value.toObject()["definitionId"]==parentDefinition)parentOccurrence=value.toObject()["id"].toString();
        QVERIFY(!parentOccurrence.isEmpty());
        const auto driver=authored["joints"].toArray()[0].toObject()["id"].toString();
        call(window,"joint.angleSet",{{"jointId",driver},{"angleDeg",0}});

        ReferenceView* view=nullptr;for(auto* child:window.findChildren<QWidget*>())if(auto* found=dynamic_cast<ReferenceView*>(child)){view=found;break;}QVERIFY(view);call(window,"view.orientation",{{"name","front"}});call(window,"view.home");
        const auto assembly=window.state()["assembly"].toObject();const auto poses=assembly["occurrences"];const auto frame=assembly["joints"].toArray()[0].toObject()["originFrameWorld"].toArray();const auto pivot=view->projectWorldPoint({frame[3],frame[7],frame[11]});QString upperDefinition,upperBody;for(const auto& v:authored["components"].toArray())if(v.toObject()["name"]=="Upper arm")upperDefinition=v.toObject()["id"].toString();for(const auto& v:starting["evaluation"].toObject()["bodies"].toArray())if(v.toObject()["ownerId"]==upperDefinition)upperBody=v.toObject()["bodyId"].toString();const auto projection=call(window,"view.projectBody",{{"bodyId",upperBody}});const QPoint projected(qRound(projection["x"].toDouble()),qRound(projection["y"].toDouble()));QPoint armPress;double furthest=0;for(int x=-75;x<=75;x+=15)for(int y=-60;y<=60;y+=15){const QPoint p=projected+QPoint(x,y);if(!view->rect().contains(p)||view->pickBody(p.x(),p.y())!=upperBody)continue;const double distance=QLineF(QPointF(p),pivot).length();if(distance>furthest){furthest=distance;armPress=p;}}QVERIFY(furthest>35);const auto armRadius=QPointF(armPress)-pivot;const double radians=12*std::acos(-1.)/180;const QPoint armTarget(qRound(pivot.x()+std::cos(radians)*armRadius.x()-std::sin(radians)*armRadius.y()),qRound(pivot.y()+std::sin(radians)*armRadius.x()+std::cos(radians)*armRadius.y()));QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,armPress);QVERIFY(view->hasFocus());QMouseEvent armDrag(QEvent::MouseMove,armTarget,view->mapToGlobal(armTarget),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(view,&armDrag);QTest::mouseRelease(view,Qt::LeftButton,Qt::NoModifier,armTarget);QVERIFY(std::abs(window.state()["assembly"].toObject()["joints"].toArray()[0].toObject()["coordinateDeg"].toDouble())>1);
auto* positionTab=window.findChild<QToolButton*>("design.tab.position");QVERIFY(positionTab&&positionTab->isVisible());QTest::mouseClick(positionTab,Qt::LeftButton);auto* revert=window.findChild<QToolButton*>("position.command.revert");QVERIFY(revert&&revert->isVisible());QTest::mouseClick(revert,Qt::LeftButton);QCOMPARE(window.state()["assembly"].toObject()["occurrences"],poses);QVERIFY(!positionTab->isVisible());

        auto* tree=window.findChild<QTreeWidget*>("browser.tree");QVERIFY(tree);tree->expandAll();
        auto* item=row(tree,parentOccurrence);QVERIFY(item);
        browserCopy(tree,item);
        QCOMPARE(call(window,"component.clipboardInspect")["occurrenceId"].toString(),parentOccurrence);
        QTRY_VERIFY2(tree->hasFocus(),"Browser Copy must leave Browser ready for Paste shortcut");
        QTest::keyClick(tree,Qt::Key_C,Qt::ControlModifier);
        QCOMPARE(call(window,"component.clipboardInspect")["occurrenceId"].toString(),parentOccurrence);
        const auto rest=call(window,"document.inspect")["authored"].toObject();

        QTest::keyClick(tree,Qt::Key_V,Qt::ControlModifier);
        QPointer<QDockWidget> dialog=visibleCopyDialog(window);
        QVERIFY(dialog && dialog->isVisible());
        QVERIFY(dialog->windowTitle().contains("Paste"));
        QVERIFY(!QApplication::activeModalWidget());
        QVERIFY2(QApplication::focusWidget()==view,"Paste must leave the viewport focused for Escape and drag");
        QTest::keyClick(QApplication::focusWidget(),Qt::Key_Escape);
        QTRY_VERIFY(!dialog || !dialog->isVisible());
        QCOMPARE(call(window,"document.inspect")["authored"].toObject(),rest);

        QVERIFY(view && view->hasFocus());
        QTest::keyClick(view,Qt::Key_V,Qt::ControlModifier);
        dialog=visibleCopyDialog(window);QVERIFY(dialog && dialog->isVisible());
        auto* x=dialog->findChild<QDoubleSpinBox*>("component.copy.x");QVERIFY(x);
        QCOMPARE(x->value(),0.0);
        const auto preview=window.state()["activeSession"].toObject();const auto ephemeral=preview["copyResult"].toObject().value("rootOccurrenceId");
        const auto handles=call(window,"viewport.moveHandles")["handles"].toArray();QCOMPARE(handles.size(),3);const auto handle=handles[0].toObject();QCOMPARE(handle["ring"].toArray().size(),1);
        auto point=[](const QJsonObject& p){return QPointF(p["x"].toDouble(),p["y"].toDouble());};const auto center=point(handle["start"].toObject()),end=point(handle["end"].toObject());const QPoint press=(center+(end-center)*.65).toPoint();const QPoint target=(QPointF(press)-(end-center)*.5).toPoint();QVERIFY(view->rect().contains(press)&&view->rect().contains(target));
        QTest::mousePress(view,Qt::LeftButton,Qt::NoModifier,press);QVERIFY(view->hasFocus());const auto pressedHandle=call(window,"viewport.moveHandles");qInfo().noquote()<<"Paste press"<<press<<"target"<<target<<"viewport"<<view->size()<<QJsonDocument(pressedHandle).toJson(QJsonDocument::Compact);QVERIFY2(!pressedHandle["activeDrag"].toObject().isEmpty(),"Actual arrow press must begin a manipulator gesture");QMouseEvent drag(QEvent::MouseMove,target,view->mapToGlobal(target),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(view,&drag);qInfo().noquote()<<"Paste move"<<QJsonDocument(call(window,"viewport.moveHandles")).toJson(QJsonDocument::Compact)<<"error"<<QJsonDocument(window.state()["activeSession"].toObject()["handleError"].toObject()).toJson(QJsonDocument::Compact);QTest::mouseRelease(view,Qt::LeftButton,Qt::NoModifier,target);QVERIFY2(x->value()<-.1,qPrintable(QJsonDocument(window.state()["activeSession"].toObject()["inputs"].toObject()).toJson(QJsonDocument::Compact)));QCOMPARE(window.state()["activeSession"].toObject()["copyResult"].toObject()["rootOccurrenceId"],ephemeral);QCOMPARE(window.state()["activeSession"].toObject()["copyResult"].toObject()["occurrenceMap"],preview["copyResult"].toObject()["occurrenceMap"]);QCOMPARE(call(window,"document.inspect")["authored"].toObject(),rest);

        {
            ClipboardRestore restore(QApplication::clipboard());
            QApplication::clipboard()->setText("12");
            QTest::mouseClick(x,Qt::LeftButton);QTest::keyClick(x,Qt::Key_A,Qt::ControlModifier);
            QTest::keyClick(x,Qt::Key_V,Qt::ControlModifier);QTest::keyClick(x,Qt::Key_Tab);
            QCOMPARE(x->value(),12.0);
        }
        QTest::mouseClick(x,Qt::LeftButton);QTest::keyClick(x,Qt::Key_A,Qt::ControlModifier);
        QTest::keyClicks(x,"-180");QCOMPARE(x->value(),-180.0);
        const auto evidence=qEnvironmentVariable("ALLOY_ASM019_EVIDENCE");
        if(!evidence.isEmpty())call(window,"capture.screenshot",{{"path",evidence+"/native-paste-placement.png"}});
        QTest::keyClick(x,Qt::Key_Return);
        QTRY_VERIFY2(!dialog || !dialog->isVisible(),"Enter must commit the typed placement");
        const auto result=call(window,"document.inspect")["authored"].toObject();
        QCOMPARE(result["joints"].toArray().size(),rest["joints"].toArray().size()+1);
        for(int i=0;i<rest["joints"].toArray().size();++i)
            QCOMPARE(result["joints"].toArray()[i],rest["joints"].toArray()[i]);
        QCOMPARE(result["components"],rest["components"]);
        QVERIFY(window.state()["activeSession"].toObject().isEmpty());
        const auto committedRevision=call(window,"document.inspect").value("revision");
        QTest::keyClick(view,Qt::Key_Return);QCOMPARE(call(window,"document.inspect")["authored"].toObject(),result);QCOMPARE(call(window,"document.inspect")["revision"],committedRevision);
        QCOMPARE(call(window,"component.clipboardInspect")["occurrenceId"].toString(),parentOccurrence);
        if(!evidence.isEmpty())call(window,"capture.screenshot",{{"path",evidence+"/native-pasted-piston.png"}});

        call(window,"document.new",{{"discard",true}});
        const auto stale=window.execute("component.pasteOccurrence",{{"placement",
            QJsonObject{{"axis","Z"},{"angleDeg",0},{"translationMm",QJsonArray{0,0,0}}}}});
        QVERIFY(!stale["ok"].toBool());
        QCOMPARE(stale["error"].toObject()["code"].toString(),"clipboard_document_mismatch");
        call(window,"document.new",{{"discard",true}});window.close();
    }
};
QTEST_MAIN(ComponentClipboardControlsTest)
#include "component_clipboard_controls.moc"
