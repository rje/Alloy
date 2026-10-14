// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include "JointOrigin.h"
#include "LocalOccurrences.h"
#include "CadDocument.h"
#include <BRepAdaptor_Surface.hxx>
#include <QApplication>
#include <QContextMenuEvent>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QJsonDocument>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QPushButton>
#include <QTest>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <cmath>

class JointOriginControlsTest final:public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& name,const QJsonObject& args={}) {
        const auto response=window.execute(name,args);
        if(!response["ok"].toBool())qFatal("%s: %s",qPrintable(name),QJsonDocument(response).toJson().constData());
        return response["result"].toObject();
    }
    static QString definition(const QJsonObject& authored,const QString& name){
        for(const auto& value:authored["components"].toArray())if(value.toObject()["name"]==name)return value.toObject()["id"].toString();
        return {};
    }
    static QString occurrence(const QJsonObject& authored,const QString& owner){
        for(const auto& value:authored["occurrences"].toArray())if(value.toObject()["definitionId"]==owner)return value.toObject()["id"].toString();
        return {};
    }
    static void movePointer(ReferenceView* view,const QPoint& pixel,Qt::KeyboardModifiers modifiers=Qt::NoModifier){
        const QPointF local(pixel);
        QMouseEvent event(QEvent::MouseMove,local,view->mapToGlobal(pixel),Qt::NoButton,Qt::NoButton,modifiers);
        QApplication::sendEvent(view,&event);
    }
    QPoint hoverFace(AlloyWindow& window,ReferenceView* view,const EvaluatedBody& body,const QJsonObject& candidate){
        auto canonical=candidate["position"].toObject();canonical.remove("occurrenceId");
        const auto face=JointOrigin::resolve(body,canonical).face;BRepAdaptor_Surface surface(face);
        int visible=0,hoverEvents=0;
        for(int v=1;v<=3;++v)for(int u=0;u<24;++u){
            const auto point=surface.Value(surface.FirstUParameter()+(surface.LastUParameter()-surface.FirstUParameter())*(u+.5)/24,
                surface.FirstVParameter()+(surface.LastVParameter()-surface.FirstVParameter())*v/4.0);
            const auto projected=view->projectWorldPoint({point.X(),point.Y(),point.Z()});
            const QPoint pixel(qRound(projected.x()),qRound(projected.y()));
            if(!view->rect().contains(pixel))continue;
            ++visible;
            movePointer(view,pixel);
            const auto pointer=call(window,"joint.originInspect")["lastPointer"].toObject();
            if(qRound(pointer["x"].toDouble())!=pixel.x() || qRound(pointer["y"].toDouble())!=pixel.y())
                qFatal("Qt mouse move did not reach joint-origin hover handler");
            const auto hovered=call(window,"joint.originInspect")["candidate"].toObject();
            hoverEvents+=!hovered.isEmpty();
            if(hovered["position"].toObject()["rimEdgeIds"]==candidate["position"].toObject()["rimEdgeIds"]
                && hovered["position"].toObject()["occurrenceId"]==body.occurrenceId)return pixel;
        }
        qInfo()<<"face scan"<<candidate["centerMm"]<<"onscreen"<<visible<<"candidate events"<<hoverEvents
            <<"last state"<<QJsonDocument(call(window,"joint.originInspect")).toJson(QJsonDocument::Compact);
        return {};
    }
private slots:
    void heldFaceCenterAndJointsVisibility(){
        const auto source=qEnvironmentVariable("ALLOY_JOINT_ORIGIN_SOURCE"),complete=qEnvironmentVariable("ALLOY_JOINT_ORIGIN_COMPLETE");
        QVERIFY(!source.isEmpty() && !complete.isEmpty());
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1510,920);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));QApplication::setActiveWindow(&window);
        ReferenceView* view=nullptr;for(auto* child:window.findChildren<QWidget*>())
            if(auto* found=dynamic_cast<ReferenceView*>(child)){view=found;break;}QVERIFY(view);
        call(window,"document.open",{{"path",source},{"discard",true}});
        const auto initial=call(window,"document.inspect")["authored"].toObject();
        const auto lower=definition(initial,"Lower arm"),upper=definition(initial,"Upper arm");
        const auto lowerOccurrence=occurrence(initial,lower),upperOccurrence=occurrence(initial,upper);
        QVERIFY(!lowerOccurrence.isEmpty() && !upperOccurrence.isEmpty());
        CadDocument probe;QVERIFY(probe.open(source).isEmpty());
        EvaluatedBody lowerBody;for(const auto& body:LocalOccurrences::worldBodies(probe.authored(),probe.evaluation()))
            if(body.ownerId==lower && body.occurrenceId==lowerOccurrence)lowerBody=body;
        QVERIFY(!lowerBody.shape.IsNull());
        QJsonObject hinge,other;
        for(const auto& candidate:JointOrigin::cylindricalCandidates(lowerBody)){
            const auto center=candidate["centerMm"].toArray();
            if(std::abs(center[0].toDouble())<1e-5 && std::abs(center[2].toDouble()-80)<1e-5)hinge=candidate;
            if(std::abs(center[0].toDouble()-90)<1e-5 && std::abs(center[2].toDouble())<1e-5)other=candidate;
        }
        QVERIFY(!hinge.isEmpty() && !other.isEmpty());
        call(window,"joint.revoluteDialog");
        auto* dock=window.findChild<QDockWidget*>("revoluteDock");QVERIFY(dock && dock->isVisible());
        auto* first=dock->findChild<QComboBox*>("revolute.component1"),*second=dock->findChild<QComboBox*>("revolute.component2");
        QVERIFY(first && second);first->setCurrentIndex(first->findData(upperOccurrence));second->setCurrentIndex(second->findData(lowerOccurrence));
        auto* error=dock->findChild<QLabel*>("revolute.error");QVERIFY(error && error->text().isEmpty());
        qInfo().noquote()<<"origin state after choices"<<QJsonDocument(call(window,"joint.originInspect")).toJson(QJsonDocument::Compact);
        call(window,"view.home");call(window,"view.orientation",{{"name","isometric"}});
        const QPoint firstPoint=hoverFace(window,view,lowerBody,hinge);
        const QPoint secondPoint=hoverFace(window,view,lowerBody,other);
        QVERIFY2(!firstPoint.isNull() && !secondPoint.isNull(),"Actual two finite bores must be hoverable by exact face");
        movePointer(view,firstPoint);QVERIFY2(view->hasFocus(),"Joint hover must focus viewport before modifier key");
        const auto before=call(window,"joint.originInspect")["candidate"].toObject();
        QCOMPARE(before["position"].toObject()["rimEdgeIds"],hinge["position"].toObject()["rimEdgeIds"]);
        QTest::keyPress(view,Qt::Key_Control,Qt::ControlModifier);
        QVERIFY(call(window,"joint.originInspect")["locked"].toBool());
        movePointer(view,secondPoint,Qt::ControlModifier);
        QCOMPARE(call(window,"joint.originInspect")["candidate"].toObject()["position"].toObject()["rimEdgeIds"],
            hinge["position"].toObject()["rimEdgeIds"]);
        QTest::keyRelease(view,Qt::Key_Control,Qt::NoModifier);
        const auto released=call(window,"joint.originInspect");
        QVERIFY(!released["locked"].toBool());
        QCOMPARE(released["candidate"].toObject()["position"].toObject()["rimEdgeIds"],
            other["position"].toObject()["rimEdgeIds"]);
        movePointer(view,firstPoint);
        QTest::keyPress(view,Qt::Key_Control,Qt::ControlModifier);
        QVERIFY(call(window,"joint.originInspect")["locked"].toBool());
        movePointer(view,secondPoint,Qt::ControlModifier);
        const auto glyph=call(window,"joint.originInspect")["glyph"].toObject();
        const QPoint center(qRound(glyph["x"].toDouble()),qRound(glyph["y"].toDouble()));
        QVERIFY(view->rect().contains(center));
        const auto evidence=qEnvironmentVariable("ALLOY_JOINT_ORIGIN_EVIDENCE");
        if(!evidence.isEmpty())call(window,"capture.screenshot",{{"path",evidence+"/native-locked-origin.png"}});
        QTest::mouseClick(view,Qt::LeftButton,Qt::ControlModifier,center);
        QTest::keyRelease(view,Qt::Key_Control,Qt::NoModifier);
        const auto selectedSession=window.state()["activeSession"].toObject();
        const auto selected=selectedSession["inputs"].toObject()["position"].toObject();
        QCOMPARE(selected["rimEdgeIds"],hinge["position"].toObject()["rimEdgeIds"]);
        QCOMPARE(selected["occurrenceId"],lowerOccurrence);
        QVERIFY2(selectedSession["previewed"].toBool(),"Chosen finite bore must produce a valid joint preview");
        QVERIFY(error->text().isEmpty());
        if(!evidence.isEmpty())call(window,"capture.screenshot",{{"path",evidence+"/native-selected-preview.png"}});
        auto* buttons=dock->findChild<QDialogButtonBox*>("revolute.buttons");QVERIFY(buttons);
        QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel),Qt::LeftButton);
        QCOMPARE(call(window,"document.inspect")["authored"].toObject(),initial);

        call(window,"document.open",{{"path",complete},{"discard",true}});
        auto* tree=window.findChild<QTreeWidget*>("browser.tree");QVERIFY(tree);tree->expandAll();
        QTreeWidgetItem* folder=nullptr;for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->text(0)=="Joints"){folder=*it;break;}
        QVERIFY(folder);tree->scrollToItem(folder);QApplication::processEvents();
        const auto point=tree->visualItemRect(folder).center();QVERIFY(tree->viewport()->rect().contains(point));
        QTimer::singleShot(40,tree,[]{auto* menu=qobject_cast<QMenu*>(QApplication::activePopupWidget());if(!menu)qFatal("Joints menu missing");
            for(auto* action:menu->actions())if(action->objectName()=="context.jointOriginsVisibility"){
                QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,menu->actionGeometry(action).center());return;}
            qFatal("Joints Show/Hide action missing");});
        QContextMenuEvent event(QContextMenuEvent::Mouse,point,tree->viewport()->mapToGlobal(point));
        QApplication::sendEvent(tree->viewport(),&event);
        QVERIFY(call(window,"joint.originInspect")["originsVisible"].toBool());
        if(!evidence.isEmpty())call(window,"capture.screenshot",{{"path",evidence+"/native-joint-origins.png"}});
        call(window,"joint.originsVisibility",{{"visible",false}});
        QVERIFY(!call(window,"joint.originInspect")["originsVisible"].toBool());
        window.close();
    }
};
QTEST_MAIN(JointOriginControlsTest)
#include "joint_origin_controls.moc"
