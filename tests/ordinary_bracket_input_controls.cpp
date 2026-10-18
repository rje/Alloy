// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QDir>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QContextMenuEvent>
#include <QMenu>
#include <QDialog>
#include <QPointer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QFile>
#include <QJsonDocument>
#include <QLineEdit>
#include <QProcess>
#include <QPushButton>
#include <QTest>
#include <QToolButton>
#include <QWindow>
#include <stdexcept>

// Cumulative ordinary-input gate. API calls below only load, frame and observe;
// user steps must use the actual pointer hit widget or naturally focused widget.
class OrdinaryBracketInputControls final : public QObject {
 Q_OBJECT
 AlloyWindow* window_=nullptr; ReferenceView* view_=nullptr; QString output_,step_;
 QJsonArray steps_;
 void require(bool ok,const QString& message){if(!ok)throw std::runtime_error((step_+": "+message).toStdString());}
 QJsonObject call(const QString& command,const QJsonObject& args={}){
  auto response=window_->executeApi(command,args);
  require(response.value("ok").toBool(),command+": "+QString::fromUtf8(QJsonDocument(response).toJson()));
  if(response.value("completion")=="pending"){
   const auto initial=response.value("result").toObject();
   require(QTest::qWaitFor([&]{response=window_->executeApi("command.status",{{"session",initial.value("id")},{"generation",initial.value("previewGeneration")}});return response.value("completion")!="pending";},30000),"asynchronous command completes");
   require(response.value("ok").toBool(),"asynchronous completion succeeds: "+QString::fromUtf8(QJsonDocument(response).toJson()));
  }
  return response.value("result").toObject();
 }
 QJsonObject session(){return window_->state().value("activeSession").toObject();}
 void mark(const QString& name){step_=name;qInfo().noquote()<<name;steps_.append(QJsonObject{{"step",name},{"state",window_->state()}});}
 void click(QWidget* target){require(target&&target->isVisible(),"visible pointer target");
  auto* hit=QApplication::widgetAt(target->mapToGlobal(target->rect().center()));
  require(hit&&(hit==target||target->isAncestorOf(hit)),"actual hit belongs to pointer target");
  QTest::mouseClick(hit,Qt::LeftButton,Qt::NoModifier,hit->mapFromGlobal(target->mapToGlobal(target->rect().center())));QTest::qWait(100);
 }
 void key(Qt::Key key){auto* focus=QApplication::focusWidget();require(focus,"natural focus exists");
  QTest::keyClick(focus,key);QTest::qWait(80);}
 void saveReceipt(const QString& status,const QString& error={}){
  QFile file(output_+"/receipt.json");if(file.open(QIODevice::WriteOnly))file.write(QJsonDocument(QJsonObject{
   {"applicationActive",QGuiApplication::applicationState()==Qt::ApplicationActive},{"nativeFocusWindow",QGuiApplication::focusWindow()?QGuiApplication::focusWindow()->objectName():"none"},{"qtActiveWindow",QApplication::activeWindow()?QApplication::activeWindow()->objectName():"none"},
   {"focusClass",QApplication::focusWidget()?QApplication::focusWidget()->metaObject()->className():"none"},{"focusObject",QApplication::focusWidget()?QApplication::focusWidget()->objectName():"none"},{"status",status},{"step",step_},{"error",error},{"steps",steps_},{"state",window_->state()},
   {"inputRoute","Qt delivered pointer/keyboard; initial activation only; no interstep focus or selection repair"},
   {"limits",QJsonArray{"Not physical OS input or packaged qualification","Full-route native readiness is required before modal qualification; OS Save chooser remains unexercised; default final STL uses shared API export"}}}).toJson());
 }
private slots:
 void bracketGate(){
  output_=qEnvironmentVariable("ALLOY_ORDINARY_INPUT_EVIDENCE");QVERIFY(!output_.isEmpty());QVERIFY(QDir().mkpath(output_));
  const auto input=qEnvironmentVariable("ALLOY_ORDINARY_INPUT_MODEL");QVERIFY(!input.isEmpty());
  AlloyTheme::apply(*qApp);AlloyWindow window;window_=&window;window.resize(1500,950);window.show();QVERIFY(QTest::qWaitForWindowExposed(&window));
  for(auto* child:window.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(child))view_=v;QVERIFY(view_);
#ifdef Q_OS_MACOS
  if(!qEnvironmentVariableIntValue("ALLOY_ORDINARY_INPUT_REQUIRE_MODAL")){
  QProcess activate;activate.start("osascript",{"-e",QString("tell application \"System Events\" to set frontmost of first application process whose unix id is %1 to true").arg(QCoreApplication::applicationPid())});
  QVERIFY(activate.waitForFinished(5000));QCOMPARE(activate.exitCode(),0);}
#endif
  window.raise();window.activateWindow();window.windowHandle()->requestActivate();
  const bool requireModal=qEnvironmentVariableIntValue("ALLOY_ORDINARY_INPUT_REQUIRE_MODAL");
  // The legacy accepted Qt prefix permits initial harness activation. Full modal
  // qualification must instead observe real platform activation: setting Qt's
  // active window directly can conceal an inactive native application.
  if(!requireModal){QApplication::setActiveWindow(&window);QTRY_VERIFY(window.isActiveWindow());}
  try{
   if(requireModal){
    mark("Initial native application/window readiness before user steps");
    require(QTest::qWaitFor([&]{return QGuiApplication::applicationState()==Qt::ApplicationActive
      && QGuiApplication::focusWindow()==window.windowHandle() && window.isActiveWindow();},3000),
      "native application and top-level window activate without Qt bookkeeping repair");
   }
   call("document.open",{{"path",input},{"discard",true}});QTest::qWait(150);
   const auto before=call("document.inspect");const auto features=before.value("authored").toObject().value("features").toArray();
   const auto sketch=features.first().toObject().value("id").toString();
   mark("Native timeline sketch activation");click(window.findChild<QToolButton*>("timeline.feature."+sketch));
   require(QTest::qWaitFor([&]{return session().value("command")=="sketch.edit";},15000),"timeline opens sketch");
   call("sketch.lookAt");call("view.fit");call("view.zoom",{{"factor",0.8}}); // Initial camera setup only.
   mark("Sketch activation produces viewport keyboard focus");require(QApplication::focusWidget()==view_,"viewport owns natural focus before C");
   key(Qt::Key_C);require(session().value("tool")=="circle","C activates Circle");
   auto point=call("view.projectSketchPoint",{{"sketchId",sketch},{"uMm",300},{"vMm",200}});
   const QPoint local(qRound(point.value("x").toDouble()),qRound(point.value("y").toDouble()));
   require(view_->rect().contains(local),"pending-circle center is visible");
   require(QApplication::widgetAt(view_->mapToGlobal(local))==view_,"viewport receives center hit");
   QTest::mouseClick(view_,Qt::LeftButton,Qt::NoModifier,local);QTest::qWait(80);
   mark("Pending circle text focus and Tab lock");require(QApplication::focusWidget()==view_,"center pick retains viewport focus");
   key(Qt::Key_2);auto* editor=window.findChild<QLineEdit*>("sketch.inlineValue");
   require(editor&&editor->isVisible()&&QApplication::focusWidget()==editor,"typing starts focused numeric entry");
   key(Qt::Key_Tab);require(session().value("pendingCircleDiameter").toObject().value("phase")=="locked","Tab locks numeric entry");
   mark("Escape cancels pending numeric tool and returns focus");key(Qt::Key_Escape);
   require(!session().contains("pendingCircleDiameter"),"Escape cancels pending circle");require(QApplication::focusWidget()==view_,"Escape returns natural viewport focus");
   key(Qt::Key_Escape);
   mark("Finish sketch without authored change");click(window.findChild<QPushButton*>("sketch.finish"));
   require(QTest::qWaitFor([&]{return session().isEmpty();},15000),"Finish closes sketch");
   require(call("document.inspect").value("authored")==before.value("authored"),"canceled pending shape preserves authored bracket");
   call("capture.screenshot",{{"path",output_+"/sketch-prefix.png"}});QTest::qWait(150);
   const auto extrudeId=features.at(1).toObject().value("id").toString();
   mark("Native timeline extrusion edit and typed Return/Tab");click(window.findChild<QToolButton*>("timeline.feature."+extrudeId));
   require(QTest::qWaitFor([&]{return session().value("command")=="solid.extrude";},10000),"timeline opens Extrude");
   auto* distance=window.findChild<QDoubleSpinBox*>("extrude.distance");require(distance,"Distance exists");
   auto* line=distance->findChild<QLineEdit*>();click(line);require(QApplication::focusWidget()==line||QApplication::focusWidget()==distance,"distance gets natural text focus");
   QTest::keyClick(QApplication::focusWidget(),Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(QApplication::focusWidget(),"51");key(Qt::Key_Tab);
   require(QTest::qWaitFor([&]{return session().value("previewed").toBool();},10000),"typed distance produces preview");
   require(session().value("inputs").toObject().value("distance").toDouble()==51,"Tab publishes 51 mm");
   call("capture.screenshot",{{"path",output_+"/extrude-preview.png"}});
   auto* box=window.findChild<QDialogButtonBox*>("extrude.buttons");require(box,"Extrude buttons exist");click(box->button(QDialogButtonBox::Cancel));
   require(session().isEmpty(),"Cancel returns from Extrude");require(QApplication::focusWidget()==view_,"Cancel returns natural viewport focus");require(call("document.inspect").value("authored")==before.value("authored"),"Cancel restores exact authored bracket");
   mark("Reenter timeline edit and Return commits depth");QTest::qWait(100);click(window.findChild<QToolButton*>("timeline.feature."+extrudeId));
   require(QTest::qWaitFor([&]{return session().value("previewed").toBool();},10000),"reopened depth preview ready");
   distance=window.findChild<QDoubleSpinBox*>("extrude.distance");line=distance->findChild<QLineEdit*>();click(line);
   require(QApplication::focusWidget()==line||QApplication::focusWidget()==distance,"reopened distance naturally focused");
   QTest::keyClick(QApplication::focusWidget(),Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(QApplication::focusWidget(),"51");key(Qt::Key_Return);
   require(QTest::qWaitFor([&]{return session().isEmpty();},15000),"OK returns from Extrude");
   require(call("document.inspect").value("authored").toObject().value("features").toArray().at(1).toObject().value("parameters").toObject().value("distanceMm").toDouble()==51,"Return commits edited depth");
   require(QApplication::focusWidget()==view_,"commit restores natural viewport focus");
   QTest::keyClick(QApplication::focusWidget(),Qt::Key_Z,Qt::ControlModifier);QTest::qWait(200);
   require(call("document.inspect").value("authored")==before.value("authored"),"native Undo restores original bracket");
   mark("Browser body selection and native popup Escape");
   auto* tree=window.findChild<QTreeWidget*>("browser.tree");require(tree,"Browser exists");
   const auto bodyId=before.value("evaluation").toObject().value("bodies").toArray().first().toObject().value("bodyId").toString();
   QTreeWidgetItem* body=nullptr;for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole+1).toString()==bodyId){body=*it;break;}
   require(body,"body row exists");const auto position=tree->visualItemRect(body).center();
   auto* hit=QApplication::widgetAt(tree->viewport()->mapToGlobal(position));require(hit==tree->viewport(),"Browser body receives real hit");
   QTest::mouseClick(hit,Qt::LeftButton,Qt::NoModifier,position);QTest::qWait(80);
   const auto popup=[&]{QContextMenuEvent event(QContextMenuEvent::Mouse,position,tree->viewport()->mapToGlobal(position));QApplication::sendEvent(tree->viewport(),&event);};
   popup();require(QTest::qWaitFor([]{return QApplication::activePopupWidget()!=nullptr;},2000),"body context popup becomes active");
   auto* menu=qobject_cast<QMenu*>(QApplication::activePopupWidget());require(menu&&menu->objectName()=="context.browserBody","body context owns popup");
   QTest::keyClick(menu,Qt::Key_Escape);require(QTest::qWaitFor([]{return !QApplication::activePopupWidget();},2000),"Escape closes popup");
   // Opt-in full-route mode first requires genuine initial native activation.
   // Default acceptance stops at popup dismissal; it never repairs modal focus.
   if(requireModal){
   mark("Browser popup opens export modal and Escape returns");popup();
   require(QTest::qWaitFor([]{return QApplication::activePopupWidget()!=nullptr;},2000),"second context popup active");menu=qobject_cast<QMenu*>(QApplication::activePopupWidget());
   auto* exportAction=menu->findChild<QAction*>("context.exportMesh");require(exportAction,"Save As Mesh action exists");
   const auto actionPoint=menu->actionGeometry(exportAction).center();require(QApplication::widgetAt(menu->mapToGlobal(actionPoint))==menu,"menu receives export action hit");
   QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,actionPoint);
   QPointer<QDialog> dialog;
   require(QTest::qWaitFor([&]{dialog=window.findChild<QDialog*>("mesh.export.dialog");return dialog&&dialog->isVisible()&&QApplication::activeModalWidget()==dialog;},3000),"export modal natively active");
   require(!QApplication::activePopupWidget(),"menu closed before modal input");
   require(QTest::qWaitFor([&]{auto* focus=QApplication::focusWidget();return focus&&(focus==dialog||dialog->isAncestorOf(focus));},2000),"export modal owns natural focus");
   auto* formats=dialog->findChild<QComboBox*>("mesh.export.format");key(Qt::Key_Tab);
   require(QApplication::focusWidget()==formats,"Tab advances naturally to Format");key(Qt::Key_Down);
   require(formats->currentData()=="ascii","native key changes export Format");key(Qt::Key_Up);
   require(formats->currentData()=="binary","native key restores binary Format");
   call("capture.screenshot",{{"path",output_+"/mesh-modal.png"}});key(Qt::Key_Escape);
   require(QTest::qWaitFor([&]{return !dialog||!dialog->isVisible();},2000),"Escape closes export modal");
   require(!QApplication::activeModalWidget()&&!QApplication::activePopupWidget(),"modal/popup fully dismissed");
   require(call("document.inspect").value("authored")==before.value("authored"),"export cancel preserves authored state");
   call("capture.screenshot",{{"path",output_+"/returned-after-modal.png"}});
   }
   mark("Shared API export after native popup dismissal (modal/OS chooser separate)");
   const auto beforeExport=call("document.inspect");
   const auto exported=call("document.exportStl",{{"path",output_+"/bracket-mm.stl"},{"bodyId",bodyId},{"units","mm"},{"format","binary"},{"surfaceDeviationMm",0.05},{"normalDeviationDeg",5}});
   require(call("document.inspect")==beforeExport,"shared API export preserves document and history");
   require(exported.value("triangles").toInt()>0,"shared API export writes triangles");
   call("document.save",{{"path",output_+"/bracket.alloy"}});
   saveReceipt("qt-prefix-and-api-export-passed-modal-native-chooser-open");
   call("document.new",{{"discard",true}});window.close();
  }catch(const std::exception& error){saveReceipt("failed",error.what());window.executeApi("capture.screenshot",{{"path",output_+"/failure.png"}});
   window.executeApi("document.new",{{"discard",true}});window.close();QFAIL(error.what());}
 }
};
QTEST_MAIN(OrdinaryBracketInputControls)
#include "ordinary_bracket_input_controls.moc"
