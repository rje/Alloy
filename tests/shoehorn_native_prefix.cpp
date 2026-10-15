// SPDX-License-Identifier: MIT
// Bounded from-blank, Qt-delivered shoehorn creation through the 3 mm blank.
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QDockWidget>
#include <QFile>
#include <QJsonDocument>
#include <QLineEdit>
#include <QMenu>
#include <QMouseEvent>
#include <QPushButton>
#include <QScrollArea>
#include <QTest>
#include <QToolButton>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QWindow>
#include <algorithm>
#include <cmath>
#include <stdexcept>

class ShoehornNativePrefix final : public QObject {
    Q_OBJECT
    AlloyWindow* window_=nullptr;
    ReferenceView* view_=nullptr;
    QString output_;
    bool recording_=false;
    static void need(bool okay,const QString& what){if(!okay)throw std::runtime_error(what.toStdString());}
    QJsonObject call(const QString& name,const QJsonObject& args={}){
        const auto response=window_->executeApi(name,args);
        need(response.value("ok").toBool(),name+": "+QString::fromUtf8(QJsonDocument(response).toJson(QJsonDocument::Compact)));
        return response.value("result").toObject();
    }
    QJsonObject state() const{return window_->state();}
    QJsonObject session() const{return state().value("activeSession").toObject();}
    QJsonObject draft() const{return session().value("draftSketch").toObject();}
    QJsonObject parameters() const{return draft().value("parameters").toObject();}
    QJsonObject inspect(){return call("document.inspect");}
    QJsonObject entity(const QString& type) const {
        for(const auto& value:parameters().value("entities").toArray())if(value.toObject().value("type")==type)return value.toObject();
        return {};
    }
    QJsonObject point(const QJsonValue& id) const {
        for(const auto& value:parameters().value("points").toArray())if(value.toObject().value("id")==id)return value.toObject();
        return {};
    }
    void step(const QString& label){
        qInfo().noquote()<<label;
        if(recording_){call("recording.step",{{"text",label}});QTest::qWait(650);}
    }
    void capture(const QString& name){call("capture.screenshot",{{"path",output_+"/"+name+".png"}});}
    void move(QWidget* widget,const QPoint& local){
        const auto global=widget->mapToGlobal(local);
        QMouseEvent event(QEvent::MouseMove,QPointF(local),QPointF(global),Qt::NoButton,Qt::NoButton,Qt::NoModifier);
        QApplication::sendEvent(widget,&event);QTest::qWait(recording_?90:20);
    }
    void click(QWidget* widget,const QPoint& local){
        need(widget&&widget->isVisible(),"Native target is visible");
        const auto global=widget->mapToGlobal(local);
        auto* actual=QApplication::widgetAt(global);
        need(actual==widget || (actual && widget->isAncestorOf(actual)),
            QString("Actual hit recipient differs from %1: %2").arg(widget->objectName(),actual?actual->objectName():"none"));
        const auto recipientPoint=actual->mapFromGlobal(global);
        move(actual,recipientPoint);QTest::mouseClick(actual,Qt::LeftButton,Qt::NoModifier,recipientPoint);
        QTest::qWait(recording_?180:30);
    }
    void click(QWidget* widget){click(widget,widget->rect().center());}
    template<class Widget> Widget* widget(const QString& id) const {
        auto* found=window_->findChild<Widget*>(id);need(found,QString("Missing control %1").arg(id));return found;
    }
    void clickControl(const QString& id){
        auto* target=widget<QWidget>(id);
        for(auto* scroll=target->parentWidget();scroll;scroll=scroll->parentWidget())
            if(auto* area=qobject_cast<QScrollArea*>(scroll)){area->ensureWidgetVisible(target);break;}
        need(target->isVisible(),QString("Control %1 not visible").arg(id));click(target);
    }
    void menuAction(const QString& menuId,const QString& actionId){
        auto* menu=widget<QMenu>(menuId);auto* action=widget<QAction>(actionId);
        need(menu->actions().contains(action),"Action belongs to intended menu");
        // QMenu::popup avoids QToolButton::InstantPopup's nested event loop; the
        // visible action itself still receives the Qt pointer event.
        menu->popup(window_->mapToGlobal(QPoint(120,165)));
        need(QTest::qWaitFor([&]{return QApplication::activePopupWidget()==menu;},2000),"Menu became active");
        click(menu,menu->actionGeometry(action).center());
        need(QTest::qWaitFor([]{return QApplication::activePopupWidget()==nullptr;},2000),"Menu closed after action");
    }
    QPoint pixel(const QString& sketchId,double u,double v){
        const auto p=call("view.projectSketchPoint",{{"sketchId",sketchId},{"uMm",u},{"vMm",v}});
        const QPoint local(qRound(p.value("x").toDouble()),qRound(p.value("y").toDouble()));
        need(view_->rect().contains(local),"Projected point in viewport");return local;
    }
    void clickSheetInterior(const QJsonObject& sheet){
        const auto bounds=sheet.value("boundsMm").toObject();
        const auto low=bounds.value("min").toArray(),high=bounds.value("max").toArray();
        need(low.size()==3&&high.size()==3,"Source sheet has world bounds");
        QJsonArray middle;for(int axis=0;axis<3;++axis)middle.append((low[axis].toDouble()+high[axis].toDouble())*0.5);
        const auto projected=view_->projectWorldPoint(middle);
        const QPoint hit(qRound(projected.x()),qRound(projected.y()));
        need(view_->rect().contains(hit),"Projected sheet interior in viewport");
        need(QTest::qWaitFor([&]{return view_->pickBody(hit.x(),hit.y())==sheet.value("bodyId").toString();},2000),
            QString("Projected interior must hit exact source sheet at %1,%2").arg(hit.x()).arg(hit.y()));
        click(view_,hit);
    }
    void pointClick(const QString& sketchId,double u,double v){click(view_,pixel(sketchId,u,v));}
    void authoredPointClick(const QString& sketchId,const QJsonValue& id){
        const auto p=point(id);need(!p.isEmpty(),"Authored point exists");pointClick(sketchId,p.value("uMm").toDouble(),p.value("vMm").toDouble());
    }
    void beginSketch(const QString& planeButton){
        if(planeButton!="YZ"){
            auto* tree=widget<QTreeWidget>("browser.tree");QTreeWidgetItem* item=nullptr;
            for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole+2).toString()==planeButton){item=*it;break;}
            need(item,"Retained path plane appears in Browser");tree->scrollToItem(item);
            const auto rectangle=tree->visualItemRect(item);
            click(tree->viewport(),QPoint(std::clamp(80,rectangle.left()+30,rectangle.right()-4),rectangle.center().y()));
            need(state().value("selection").toObject().value("planeId")==planeButton,"Browser selected intended path plane");
        }
        clickControl("command.sketch.create");
        if(session().value("command")=="sketch.plane")clickControl("sketch.plane."+planeButton);
        need(QTest::qWaitFor([&]{return session().value("command")=="sketch.edit";},2000),"Sketch editor opened");
        if(planeButton!="YZ")need(session().value("inputs").toObject().value("planeId")==planeButton,"Sketch uses selected path plane");
        call("sketch.lookAt"); // camera-only setup for precise viewport positions
        need(view_->isVisible(),"Sketch canvas visible");
    }
    void nameSketch(const QString& title){
        auto* name=widget<QLineEdit>("sketch.name");click(name);name->selectAll();QTest::keyClicks(name,title);QTest::keyClick(name,Qt::Key_Tab);
        need(session().value("draftSketch").toObject().value("name")==title,"Native sketch name retained");
    }
    void finishSketch(){
        clickControl("sketch.finish");
        need(QTest::qWaitFor([&]{return session().isEmpty();},2000),"Finish Sketch completed");
    }
    void escapeTool(){
        need(view_->hasFocus(),"Viewport naturally has focus before Escape");
        QTest::keyClick(view_,Qt::Key_Escape);
        need(session().value("tool")=="select","Escape restored Select");
    }
    void constraint(const QString& kind,const QString& sketchId,const QList<QPair<double,double>>& picks){
        clickControl("sketch.constraint."+kind);
        need(session().value("constraintType")==kind,"Native constraint tool active");
        const int before=parameters().value("constraints").toArray().size();
        for(const auto& [u,v]:picks)pointClick(sketchId,u,v);
        const auto rows=parameters().value("constraints").toArray();
        need(rows.size()>=before+1,"Native relation retained");
        const QString authoredKind=kind=="horizontalVertical" ? "horizontal" : kind;
        bool retained=false;for(const auto& row:rows)if(row.toObject().value("type")==authoredKind)retained=true;
        need(retained,"Requested native relation retained");
        escapeTool();
    }
    void dimension(const QString& sketchId,const QJsonValue& first,const QJsonValue& second,
                   double placeU,double placeV,const QString& text,const QString& kind){
        need(view_->hasFocus(),"Viewport naturally focused before D");
        QTest::keyClick(view_,Qt::Key_D);need(session().value("tool")=="dimension","D activates Dimension");
        const auto before=parameters().value("dimensions").toArray().size();
        authoredPointClick(sketchId,first);authoredPointClick(sketchId,second);
        const auto position=pixel(sketchId,placeU,placeV);
        move(view_,position);need(session().value("pendingDimension").toObject().value("phase")=="placing","Dimension placement preview");
        click(view_,position);
        auto* editor=widget<QLineEdit>("sketch.inlineValue");need(editor->isVisible()&&QApplication::focusWidget()==editor,"Inline dimension editor naturally focused");
        QTest::keyClick(editor,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(editor,text,Qt::NoModifier,recording_?70:-1);
        QTest::keyClick(editor,Qt::Key_Return);
        need(!session().contains("pendingDimension"),"Native dimension accepted");
        const auto rows=parameters().value("dimensions").toArray();need(rows.size()==before+1,"One retained dimension added");
        const auto row=rows.last().toObject();need(row.value("kind")==kind && std::abs(row.value("valueMm").toDouble()-text.toDouble())<1e-5,
            "Dimension kind and value match source");
        need(view_->hasFocus(),"Dimension returns focus to canvas");
    }
    QString pathPlane(const QString& pathId,double hitU,double hitV,bool end){
        clickControl("command.construction.pathPlane");
        need(QTest::qWaitFor([&]{return session().value("command")=="construction.pathPlane";},2000),"Plane Along Path opened");
        call("view.orientation",{{"name","right"}}); // camera-only setup
        pointClick(pathId,hitU,hitV);
        need(!session().value("inputs").toObject().value("path").toObject().isEmpty(),"Actual canvas pick selects retained spline");
        auto* endpoint=widget<QComboBox>("pathPlane.endpoint");
        if(end){
            click(endpoint);need(QTest::qWaitFor([&]{return QApplication::activePopupWidget()!=nullptr;},2000),"Endpoint popup active");
            auto* focus=QApplication::focusWidget();need(focus&&endpoint->view()->isAncestorOf(focus),"Endpoint popup owns keyboard");
            QTest::keyClick(focus,Qt::Key_End);QTest::keyClick(focus,Qt::Key_Return);
            need(QTest::qWaitFor([]{return QApplication::activePopupWidget()==nullptr;},2000),"Endpoint popup closed");
        }
        need(session().value("inputs").toObject().value("path").toObject().value("endpoint").toInt()==(end?1:0),"Requested exact spline endpoint");
        auto* buttons=widget<QDialogButtonBox>("pathPlane.buttons");auto* okay=buttons->button(QDialogButtonBox::Ok);
        need(okay&&okay->isEnabled(),"Path plane preview enabled");click(okay);
        need(QTest::qWaitFor([&]{return session().isEmpty();},2000),"Path plane committed");
        const auto planes=inspect().value("evaluation").toObject().value("constructionPlanes").toArray();
        need(planes.size()==(end?2:1),"Expected retained path planes");return planes.last().toObject().value("featureId").toString();
    }
private slots:
    void fromBlankToThicken(){
        output_=qEnvironmentVariable("ALLOY_SHOEHORN_NATIVE_EVIDENCE");QVERIFY(!output_.isEmpty());QVERIFY(QDir().mkpath(output_));
        AlloyTheme::apply(*qApp);AlloyWindow window;window_=&window;window.resize(1500,960);window.show();QVERIFY(QTest::qWaitForWindowExposed(&window));
        window.raise();window.activateWindow();if(window.windowHandle())window.windowHandle()->requestActivate();QApplication::setActiveWindow(&window);
        QTRY_VERIFY(window.isActiveWindow());
        for(auto* child:window.findChildren<QWidget*>())if(auto* view=dynamic_cast<ReferenceView*>(child)){view_=view;break;}QVERIFY(view_);
        try{
            need(inspect().value("authored").toObject().value("features").toArray().isEmpty(),"Start from blank document");
            call("recording.start",{{"path",output_+"/native-shoehorn-prefix.mp4"},{"fps",15}});recording_=true;
            step("Create Sketch · YZ plane · three-point fit spline");
            beginSketch("YZ");nameSketch("Dimensioned spine");
            // The initially empty document fits only the origin grid. Frame the
            // full 130 mm source path before its third viewport pick.
            call("view.zoom",{{"factor",0.5}});
            menuAction("sketch.commands.spline","sketch.tool.fitSpline");
            need(session().value("tool")=="fitSpline" && view_->hasFocus(),"Fit Point Spline owns viewport");
            const QString pathId=draft().value("id").toString();
            for(const auto& uv:QList<QPair<double,double>>{{0,0},{50,16},{130,0}})pointClick(pathId,uv.first,uv.second);
            need(session().value("pendingSplinePoints").toArray().size()==3,"Three native fit-point picks retained pending");
            clickControl("sketch.spline.complete");
            auto spline=entity("fitSpline");need(spline.value("pointIds").toArray().size()==3,"Retained fit spline created");
            escapeTool();
            auto ids=spline.value("pointIds").toArray();
            step("Anchor source spine · 130, 50 and 16 mm");
            constraint("fixed",pathId,{{0,0}});
            constraint("horizontalVertical",pathId,{{0,0},{130,0}});
            dimension(pathId,ids[0],ids[2],65,-18,"130","horizontal");
            dimension(pathId,ids[0],ids[1],25,30,"50","horizontal");
            dimension(pathId,ids[0],ids[1],-12,8,"16","vertical");
            need(parameters().value("dimensions").toArray().size()==3,"Three driving spine dimensions");
            capture("01-dimensioned-spine");finishSketch();
            step("Construct planes normal to the spline at Start and End");
            const auto frontPlane=pathPlane(pathId,50,16,false);
            const auto rearPlane=pathPlane(pathId,50,16,true);
            capture("02-path-planes");
            step("Front section · three-point arc with midpoint and 50/10/10 mm dimensions");
            beginSketch(frontPlane);nameSketch("Front arc");
            const QString frontId=draft().value("id").toString();
            menuAction("sketch.commands.arc","sketch.tool.threePointArc");
            need(session().value("tool")=="threePointArc","3-Point Arc tool active");
            for(const auto& uv:QList<QPair<double,double>>{{-25,10},{25,10},{0,0}})pointClick(frontId,uv.first,uv.second);
            auto arc=entity("arc");need(!arc.isEmpty(),"Native front arc created");escapeTool();
            constraint("midpoint",frontId,{{-15,3.244},{0,0}});
            arc=entity("arc");const auto origin=parameters().value("originPointId");
            dimension(frontId,arc.value("startPointId"),arc.value("endPointId"),0,20,"50","horizontal");
            dimension(frontId,origin,arc.value("startPointId"),-33,5,"10","vertical");
            dimension(frontId,origin,arc.value("endPointId"),33,5,"10","vertical");
            capture("03-front-arc");finishSketch();
            step("Rear section · centered horizontal line, 30 mm");
            beginSketch(rearPlane);nameSketch("Rear line");
            const QString rearId=draft().value("id").toString();
            menuAction("sketch.commands.create","sketch.tool.line");
            need(session().value("tool")=="line","Line tool active");
            pointClick(rearId,-15,0);pointClick(rearId,15,0);escapeTool();
            auto line=entity("line");need(!line.isEmpty(),"Native rear line created");
            constraint("midpoint",rearId,{{0,0},{0,0}});
            line=entity("line");
            dimension(rearId,line.value("startPointId"),line.value("endPointId"),0,10,"30","horizontal");
            capture("04-rear-line");finishSketch();
            step("Surface Loft · front arc, rear line, retained Centerline · New Component");
            call("view.home");
            clickControl("design.tab.surface");
            menuAction("surface.commands.create","surface.action.surface.loftDialog");
            need(QTest::qWaitFor([&]{return session().value("command")=="solid.loft";},2000),"Surface Loft dialog opened");
            auto* loft=widget<QDockWidget>("loftDock");auto* operation=loft->findChild<QComboBox*>("loftOperation");
            need(operation&&operation->currentData()=="newComponent","New Component selected");
            auto* component=widget<QLineEdit>("loftComponentName");click(component);component->selectAll();QTest::keyClicks(component,"Shoehorn");QTest::keyClick(component,Qt::Key_Tab);
            clickControl("loft.section1");pointClick(frontId,15,3.244);
            pointClick(rearId,10,0);pointClick(pathId,50,16);
            need(QTest::qWaitFor([&]{return session().value("previewed").toBool();},5000),"Native three-source guided sheet preview");
            need(session().value("geometry").toObject().value("type")=="surface","Preview is open sheet");
            capture("05-loft-preview");
            auto* loftButtons=loft->findChild<QDialogButtonBox*>();need(loftButtons&&loftButtons->button(QDialogButtonBox::Ok)->isEnabled(),"Loft OK enabled");
            click(loftButtons->button(QDialogButtonBox::Ok));need(QTest::qWaitFor([&]{return session().isEmpty();},3000),"Loft committed");
            auto sheet=inspect().value("evaluation").toObject().value("bodies").toArray().first().toObject();
            need(sheet.value("type")=="surface"&&sheet.value("valid").toBool()&&sheet.value("topology").toObject().value("faces").toInt()==1,"One valid source sheet retained");
            const auto sheetId=sheet.value("bodyId").toString();
            const auto sheetFace=sheet.value("supportFaces").toArray().first().toObject();
            const auto componentId=sheet.value("ownerId").toString();
            for(const auto& id:QStringList{pathId,frontId,rearId,frontPlane,rearPlane})call("feature.visibility",{{"featureId",id},{"visible",false}});
            call("view.home");capture("06-guided-sheet");
            step("Thicken · select the sheet, preview 3 mm One Side New Body");
            clickControl("design.tab.solid");
            menuAction("solid.commands.create","solid.action.solid.thickenDialog");
            need(QTest::qWaitFor([&]{return session().value("command")=="solid.thicken";},2000),"Thicken opened");
            auto* thicken=widget<QDockWidget>("thickenDock");clickControl("thickenFace");
            clickSheetInterior(sheet);
            need(QTest::qWaitFor([&]{return session().value("previewed").toBool();},5000),"Actual sheet-face pick yields solid preview");
            need(session().value("geometry").toObject().value("type")=="solid","Thicken preview solid");
            auto* amount=widget<QDoubleSpinBox>("thickenThickness");
            click(amount);need(QApplication::focusWidget()==amount || amount->isAncestorOf(QApplication::focusWidget()),"Thickness field naturally focused");
            auto* amountFocus=QApplication::focusWidget();QTest::keyClick(amountFocus,Qt::Key_A,Qt::ControlModifier);
            QTest::keyClicks(amountFocus,"3",Qt::NoModifier,recording_?90:-1);QTest::keyClick(amountFocus,Qt::Key_Tab);
            need(std::abs(amount->value()-3)<1e-6 && std::abs(session().value("inputs").toObject().value("signedThicknessMm").toDouble()-3)<1e-6,"Entered +3 mm reaches shared preview");
            capture("07-thicken-preview");
            const auto beforeCancel=inspect().value("authored");
            auto* thickButtons=thicken->findChild<QDialogButtonBox*>();need(thickButtons,"Thicken actions exist");
            step("Cancel preview · authored source remains");click(thickButtons->button(QDialogButtonBox::Cancel));
            need(QTest::qWaitFor([&]{return session().isEmpty();},2000),"Cancel closed Thicken");
            need(inspect().value("authored")==beforeCancel,"Cancel leaves source unchanged");
            step("Thicken · reselect source sheet · commit +3 mm");
            menuAction("solid.commands.create","solid.action.solid.thickenDialog");thicken=widget<QDockWidget>("thickenDock");clickControl("thickenFace");clickSheetInterior(sheet);
            need(QTest::qWaitFor([&]{return session().value("previewed").toBool();},5000),"Reselected source sheet previews solid");
            thickButtons=thicken->findChild<QDialogButtonBox*>();need(thickButtons&&thickButtons->button(QDialogButtonBox::Ok)->isEnabled(),"Valid Thicken OK");
            click(thickButtons->button(QDialogButtonBox::Ok));need(QTest::qWaitFor([&]{return session().isEmpty();},3000),"Thicken committed");
            const auto final=inspect();const auto authored=final.value("authored").toObject();
            const auto bodies=final.value("evaluation").toObject().value("bodies").toArray();
            need(bodies.size()==2,"Retained sheet and solid coexist");
            QJsonObject solid,retainedSheet;for(const auto& value:bodies){const auto body=value.toObject();if(body.value("type")=="solid")solid=body;else if(body.value("bodyId")==sheetId)retainedSheet=body;}
            need(solid.value("valid").toBool()&&solid.value("closed").toBool()&&solid.value("volumeMm3").toDouble()>0,"Positive valid closed blank");
            need(!retainedSheet.isEmpty()&&!retainedSheet.value("visible").toBool()&&retainedSheet.value("ownerId")==componentId,"Same sheet hidden in retained component");
            need(componentId!=authored.value("documentId").toString()&&solid.value("ownerId")==authored.value("documentId"),"Sheet component and root solid ownership");
            const auto feature=authored.value("features").toArray().last().toObject();
            need(feature.value("type")=="thicken"&&std::abs(feature.value("parameters").toObject().value("signedThicknessMm").toDouble()-3)<1e-6,"Retained Thicken has signed +3 mm");
            const auto source=feature.value("parameters").toObject().value("sourceFace").toObject();
            need(source.value("bodyId")==sheetId&&source.value("faceId")==sheetFace.value("faceId"),"Thicken retains exact source sheet face");
            call("view.home");capture("08-thickened-blank");
            const QString model=output_+"/shoehorn-native-thickened.alloy";call("document.save",{{"path",model}});
            QTest::qWait(1200);const auto movie=call("recording.stop");recording_=false;
            QFile receipt(output_+"/result.json");need(receipt.open(QIODevice::WriteOnly),"Result file writable");
            receipt.write(QJsonDocument(QJsonObject{{"passed",true},{"model",model},{"recording",movie},
                {"input","Qt-delivered command controls, sketch/selector viewport picks, keyboard dimensions, Cancel and OK; API camera, presentation visibility, inspect, record and save"},
                {"authored",final.value("authored")},{"evaluation",final.value("evaluation")}}).toJson());receipt.close();
            call("document.new",{{"discard",true}});window.close();
        }catch(const std::exception& error){
            qWarning().noquote()<<"NATIVE_SHOEHORN_FAILURE"<<error.what();
            if(recording_){window.executeApi("recording.stop",{});recording_=false;}
            window.executeApi("capture.screenshot",{{"path",output_+"/failure.png"}});
            QFile failure(output_+"/failure-state.json");if(failure.open(QIODevice::WriteOnly))failure.write(QJsonDocument(window.state()).toJson());
            window.executeApi("document.new",{{"discard",true}});window.close();QFAIL(error.what());
        }
    }
};
QTEST_MAIN(ShoehornNativePrefix)
#include "shoehorn_native_prefix.moc"
