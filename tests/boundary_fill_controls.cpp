// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QDir>
#include <QJsonDocument>
#include <QListWidget>
#include <QMenu>
#include <QPushButton>
#include <QStyle>
#include <QStyleOptionViewItem>
#include <QTest>
#include <QTimer>
#include <QTemporaryDir>
#include <QElapsedTimer>
#include <QStatusBar>
#include <cmath>
#include <QToolButton>

class BoundaryFillControlsTest final:public QObject{
 Q_OBJECT
 QJsonObject call(AlloyWindow& w,const QString& command,const QJsonObject& args={}){auto r=w.execute(command,args);
  if(r.value("completion")=="pending"){
   const auto initial=r.value("result").toObject();QElapsedTimer deadline;deadline.start();
   do{QTest::qWait(25);r=w.execute("command.status",{{"session",initial.value("id")},{"generation",initial.value("previewGeneration")}});
    if(deadline.elapsed()>180000)qFatal("Boundary Fill commit timed out");
   }while(r.value("completion")=="pending");
  }
  if(!r.value("ok").toBool())qFatal("%s",QJsonDocument(r).toJson().constData());return r.value("result").toObject();}
 QJsonObject session(AlloyWindow& w){return w.state().value("activeSession").toObject();}
 void choose(QToolButton* opener){
  QVERIFY(opener && opener->isVisible());auto* menu=opener->menu();QVERIFY(menu);bool active=false,clicked=false,watchdogClosed=false;
  QTimer::singleShot(50,opener,[&]{active=QApplication::activePopupWidget()==menu;
   for(auto* action:menu->actions())if(action->objectName()=="surface.action.surface.boundaryFillDialog" && active){const auto p=menu->actionGeometry(action).center();
    if(QApplication::widgetAt(menu->mapToGlobal(p))==menu){QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,p);clicked=true;}break;}});
  QTimer watchdog;watchdog.setSingleShot(true);connect(&watchdog,&QTimer::timeout,opener,[&]{watchdogClosed=true;menu->close();});watchdog.start(2000);
  QTest::mouseClick(opener,Qt::LeftButton);watchdog.stop();QVERIFY(active && clicked && !watchdogClosed);QTRY_VERIFY(QApplication::activePopupWidget()==nullptr);
 }
 void clickCheck(QCheckBox* box){
  QVERIFY(box && box->isVisible());QStyleOptionButton option;option.initFrom(box);option.text=box->text();
  const auto point=box->style()->subElementRect(QStyle::SE_CheckBoxIndicator,&option,box).center();
  QCOMPARE(QApplication::widgetAt(box->mapToGlobal(point)),static_cast<QWidget*>(box));QTest::mouseClick(box,Qt::LeftButton,Qt::NoModifier,point);
 }
private slots:
 void asynchronousCommitContracts(){
  QTemporaryDir temp;QVERIFY(temp.isValid());CadDocument d;const auto owner=d.authored().value("documentId").toString();
  auto sheet=[&](const QString& plane,double x1,double y1,double x2,double y2){
   auto sketch=SketchGeometry::newFeature(owner,plane,"Box boundary");sketch=SketchGeometry::addLine(sketch,{{"u1Mm",x1},{"v1Mm",y1},{"u2Mm",x2},{"v2Mm",y2}});
   auto draft=d.authored();auto rows=draft.value("features").toArray();rows.append(sketch);draft["features"]=rows;QVERIFY(d.commit(draft,"Sketch").isEmpty());
   const auto eid=sketch.value("parameters").toObject().value("entities").toArray()[0].toObject().value("id");
   QVERIFY(d.commit(CadDocument::surfaceExtrudeCandidate(d.authored(),{{"sketchId",sketch.value("id")},{"entityIds",QJsonArray{eid}},{"sideOneMm",10},{"sideTwoMm",10}}),"Sheet").isEmpty());
  };
  sheet("XZ",-10,0,10,0);sheet("XZ",-10,20,10,20);sheet("XZ",-10,0,-10,20);sheet("XZ",10,0,10,20);sheet("YZ",-10,0,-10,20);sheet("YZ",10,0,10,20);
  QJsonArray tools;for(const auto& body:d.bodies())tools.append(QJsonObject{{"bodyId",body.bodyId},{"producerFeatureId",body.featureId}});
  const auto path=temp.filePath("six-tools.alloy");QVERIFY(d.save(path).isEmpty());AlloyWindow w;call(w,"document.open",{{"path",path},{"discard",true}});const auto before=call(w,"document.inspect");
  auto chooseCell=[&](){auto active=call(w,"surface.boundaryFillDialog",{{"tools",tools},{"removeTools",true}});
   const auto cells=call(w,"surface.boundaryFillSelectCells").value("cells").toArray();if(cells.size()!=1)qFatal("Expected one box cell");
   call(w,"surface.boundaryFillCell",{{"cellId",cells[0].toObject().value("cellId")}});return session(w);};
  auto active=chooseCell();const auto sid=active.value("id"),cell=active.value("inputs").toObject().value("cell");
  auto raw=w.execute("command.commit",{{"session",sid}});QVERIFY(raw.value("ok").toBool());QCOMPARE(raw.value("completion").toString(),QString("pending"));
  const auto generation=raw.value("result").toObject().value("previewGeneration");
  raw=w.execute("command.commit",{{"session",sid}});QCOMPARE(raw.value("result").toObject().value("previewGeneration"),generation);
  QCOMPARE(w.execute("command.status",{{"session",sid},{"generation",generation.toInteger()+1}}).value("error").toObject().value("code").toString(),QString("stale_preview"));
  QCOMPARE(w.execute("command.update",{{"session",sid},{"inputs",QJsonObject{{"removeTools",false}}}}).value("error").toObject().value("code").toString(),QString("evaluation_busy"));
  QVERIFY(session(w).value("inputs").toObject().value("removeTools").toBool());
  raw=w.execute("command.cancel",{{"session",sid}});QVERIFY(raw.value("ok").toBool());QVERIFY(raw.value("result").toObject().value("resultDiscarded").toBool());
  QVERIFY(w.statusBar()->currentMessage().contains("Finishing background geometry"));
  QCOMPARE(w.execute("command.status",{{"session",sid},{"generation",generation}}).value("error").toObject().value("code").toString(),QString("command_cancelled"));
  QTRY_VERIFY_WITH_TIMEOUT(!w.state().value("geometryWork").toObject().value("active").toBool(),10000);QVERIFY(session(w).isEmpty());QCOMPARE(call(w,"document.inspect").value("authored"),before.value("authored"));
  active=chooseCell();const auto nextId=active.value("id");QCOMPARE(w.execute("command.status",{{"session",sid},{"generation",generation}}).value("error").toObject().value("code").toString(),QString("invalid_session"));
  const auto committed=call(w,"command.commit",{{"session",nextId}});QCOMPARE(committed.value("status").toString(),QString("committed"));QVERIFY(session(w).isEmpty());
  const auto terminal=w.execute("command.status",{{"session",nextId},{"generation",committed.value("previewGeneration")}});QVERIFY(terminal.value("ok").toBool());QCOMPARE(terminal.value("completion").toString(),QString("completed"));
  const auto after=call(w,"document.inspect");const auto bodies=after.value("evaluation").toObject().value("bodies").toArray();QCOMPARE(bodies.size(),1);QVERIFY(bodies[0].toObject().value("valid").toBool());QVERIFY(std::abs(bodies[0].toObject().value("volumeMm3").toDouble()-8000.0)<1e-5);
  const auto feature=after.value("authored").toObject().value("features").toArray().last().toObject();QCOMPARE(feature.value("parameters").toObject().value("cell"),cell);QVERIFY(feature.value("parameters").toObject().value("removeTools").toBool());
  const auto replacement=call(w,"surface.extrudeDialog");QCOMPARE(w.execute("command.status",{{"session",nextId}}).value("error").toObject().value("code").toString(),QString("invalid_session"));call(w,"command.cancel",{{"session",replacement.value("id")}});
  QCOMPARE(w.execute("command.status",{{"session",nextId}}).value("error").toObject().value("code").toString(),QString("invalid_session"));
  call(w,"history.undo");QCOMPARE(call(w,"document.inspect").value("authored"),before.value("authored"));call(w,"history.redo");QCOMPARE(call(w,"document.inspect").value("authored"),after.value("authored"));
  const auto saved=temp.filePath("solid.alloy");call(w,"document.save",{{"path",saved}});call(w,"document.open",{{"path",saved},{"discard",true}});QCOMPARE(call(w,"document.inspect").value("authored"),after.value("authored"));
  const auto candidate=CadDocument::boundaryFillCandidate(d.authored(),{{"ownerId",owner},{"tools",tools},{"cell",cell},{"removeTools",true}});const auto evaluated=CadDocument::evaluateCandidate(candidate);QVERIFY(evaluated.ok);
  QVERIFY(!d.commitBoundaryFillEvaluation(evaluated,candidate,d.authored(),d.revision()+1,"Stale").isEmpty());QCOMPARE(d.authored(),before.value("authored").toObject());
 }
 void correctedOpaquePreviewCapture(){
  const auto source=qEnvironmentVariable("ALLOY_BOUNDARY_FILL_SOURCE"),out=qEnvironmentVariable("ALLOY_BOUNDARY_FILL_EVIDENCE");if(source.isEmpty())QSKIP("No qualified five-sheet source");
  QDir().mkpath(out);AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1500,950);w.show();w.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&w));
  ReferenceView* view=nullptr;for(auto* widget:w.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(widget))view=v;QVERIFY(view);
  call(w,"document.open",{{"path",source},{"discard",true}});const auto before=call(w,"document.inspect");const auto bodies=before.value("evaluation").toObject().value("bodies").toArray();
  call(w,"component.activate",{{"definitionId",bodies[0].toObject().value("ownerId")}});call(w,"ui.designTab",{{"tab","surface"}});call(w,"view.home");QApplication::setActiveWindow(&w);w.setFocus();
  QJsonArray refs;for(const auto& row:bodies){const auto b=row.toObject();refs.append(QJsonObject{{"bodyId",b.value("bodyId")},{"producerFeatureId",b.value("featureId")}});}
  call(w,"surface.boundaryFillDialog",{{"tools",refs}});auto* dock=w.findChild<QDockWidget*>("boundaryFillDock");QVERIFY(dock);auto* list=dock->findChild<QListWidget*>("boundaryFill.tools");
  for(int i=0;i<list->count();++i)for(const auto& row:bodies)QVERIFY(!list->item(i)->text().contains(row.toObject().value("bodyId").toString().left(8)));
  QTest::mouseClick(dock->findChild<QPushButton*>("boundaryFill.selectCells"),Qt::LeftButton);QTRY_COMPARE(session(w).value("cells").toArray().size(),1);QVERIFY(view->hasFocus());
  QCheckBox* marker=nullptr;for(auto* box:view->findChildren<QCheckBox*>())if(box->objectName().startsWith("boundaryFill.cellMarker.") && box->isVisible())marker=box;QVERIFY(marker);
  QCOMPARE(QApplication::widgetAt(marker->mapToGlobal(marker->rect().center())),static_cast<QWidget*>(marker));QTest::mouseClick(marker,Qt::LeftButton);QTRY_VERIFY(session(w).value("previewed").toBool());QVERIFY(view->hasFocus());
  clickCheck(dock->findChild<QCheckBox*>("boundaryFill.removeTools"));QVERIFY(session(w).value("partitionReused").toBool());QCOMPARE(session(w).value("partitionBuildCount").toInteger(),qint64(1));
  const auto gpu=w.state().value("viewport").toObject().value("gpu").toObject();QVERIFY(gpu.value("bodyStageKey").toString().startsWith("boundary-fill-cells:"));QVERIFY(gpu.value("displayedBodyIds").toArray().isEmpty());
  QCOMPARE(gpu.value("previewAppearance").toObject().value("transparency").toDouble(),0.0);
  call(w,"capture.screenshot",{{"path",out+"/native-corrected-cell-marker.png"}});call(w,"capture.dialog",{{"path",out+"/native-corrected-controls.png"}});
  QTest::mouseClick(dock->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Cancel),Qt::LeftButton);QTRY_VERIFY(session(w).isEmpty());QCOMPARE(call(w,"document.inspect").value("authored"),before.value("authored"));
 }
 void actualToolsCellToggleCancel(){
  const auto source=qEnvironmentVariable("ALLOY_BOUNDARY_FILL_SOURCE"),out=qEnvironmentVariable("ALLOY_BOUNDARY_FILL_EVIDENCE");if(source.isEmpty())QSKIP("No qualified actual five-sheet fixture supplied");
  QDir().mkpath(out);AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1500,950);w.show();w.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&w));
  ReferenceView* view=nullptr;for(auto* widget:w.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(widget))view=v;QVERIFY(view);
  call(w,"document.open",{{"path",source},{"discard",true}});const auto before=call(w,"document.inspect");const auto bodies=before.value("evaluation").toObject().value("bodies").toArray();QCOMPARE(bodies.size(),5);
  call(w,"component.activate",{{"definitionId",bodies[0].toObject().value("ownerId")}});call(w,"view.home");QApplication::setActiveWindow(&w);w.setFocus(); // Initial activation only.
  auto* tab=w.findChild<QToolButton*>("design.tab.surface");QVERIFY(tab);QTest::mouseClick(tab,Qt::LeftButton);choose(w.findChild<QToolButton*>("surface.menu.create"));
  auto* dock=w.findChild<QDockWidget*>("boundaryFillDock");QVERIFY(dock && dock->isVisible());auto* tools=dock->findChild<QListWidget*>("boundaryFill.tools");QVERIFY(tools);QCOMPARE(tools->count(),5);QSet<QString> labels;for(int i=0;i<tools->count();++i)labels.insert(tools->item(i)->text());QCOMPARE(labels.size(),5);
  auto* remove=dock->findChild<QCheckBox*>("boundaryFill.removeTools");QVERIFY(remove && !remove->isChecked());auto* buttons=dock->findChild<QDialogButtonBox*>();QVERIFY(buttons && !buttons->button(QDialogButtonBox::Ok)->isEnabled());
  call(w,"capture.dialog",{{"path",out+"/native-initial-tools.png"}});QVERIFY(view->hasFocus());
  for(int i=0;i<tools->count();++i){const auto* item=tools->item(i);QStyleOptionViewItem option;option.initFrom(tools);option.rect=tools->visualItemRect(item);
   option.features=QStyleOptionViewItem::HasCheckIndicator|QStyleOptionViewItem::HasDisplay;option.checkState=item->checkState();option.text=item->text();option.widget=tools;
   const auto indicator=tools->style()->subElementRect(QStyle::SE_ItemViewItemCheckIndicator,&option,tools);QVERIFY(!indicator.isEmpty());const QPoint p=indicator.center();
   qInfo()<<"tool indicator"<<i<<indicator<<"active"<<(QApplication::activeWindow()==&w);
   QCOMPARE(QApplication::widgetAt(tools->viewport()->mapToGlobal(p)),static_cast<QWidget*>(tools->viewport()));QTest::mouseClick(tools->viewport(),Qt::LeftButton,Qt::NoModifier,p);
   qInfo()<<"selected tools"<<session(w).value("inputs").toObject().value("tools").toArray().size()<<"partition builds"<<session(w).value("partitionBuildCount").toInteger();
   QTRY_COMPARE(session(w).value("inputs").toObject().value("tools").toArray().size(),i+1);QCOMPARE(session(w).value("partitionBuildCount").toInteger(),qint64(0));QVERIFY(view->hasFocus());}
  auto* cells=dock->findChild<QPushButton*>("boundaryFill.selectCells");QVERIFY(cells && cells->isEnabled());QTest::mouseClick(cells,Qt::LeftButton);
  QTRY_COMPARE(session(w).value("cells").toArray().size(),1);QCOMPARE(session(w).value("partitionBuildCount").toInteger(),qint64(1));QVERIFY(!buttons->button(QDialogButtonBox::Ok)->isEnabled());QVERIFY(view->hasFocus());
  QCheckBox* marker=nullptr;for(auto* box:view->findChildren<QCheckBox*>())if(box->objectName().startsWith("boundaryFill.cellMarker.") && box->isVisible())marker=box;
  QVERIFY(marker);QCOMPARE(QApplication::widgetAt(marker->mapToGlobal(marker->rect().center())),static_cast<QWidget*>(marker));QTest::mouseClick(marker,Qt::LeftButton);
  QTRY_VERIFY(session(w).value("previewed").toBool());QVERIFY(session(w).value("partitionReused").toBool());QCOMPARE(session(w).value("partitionBuildCount").toInteger(),qint64(1));QVERIFY(view->hasFocus());
  call(w,"capture.screenshot",{{"path",out+"/native-cell-keep-tools.png"}});clickCheck(remove);QTRY_VERIFY(session(w).value("inputs").toObject().value("removeTools").toBool());QVERIFY(session(w).value("partitionReused").toBool());QVERIFY(view->hasFocus());
  QCOMPARE(dock->findChild<QComboBox*>("boundaryFill.operation")->currentData().toString(),QString("newBody"));QVERIFY(buttons->button(QDialogButtonBox::Ok)->isEnabled());
  call(w,"capture.dialog",{{"path",out+"/native-boundary-fill-controls.png"}});call(w,"capture.screenshot",{{"path",out+"/native-boundary-fill-cell.png"}});
  QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel),Qt::LeftButton);QTRY_VERIFY(session(w).isEmpty());QCOMPARE(call(w,"document.inspect").value("authored"),before.value("authored"));
  for(auto* box:view->findChildren<QCheckBox*>())QVERIFY(!box->objectName().startsWith("boundaryFill.cellMarker.") || !box->isVisible());
  // Replacement reuses authored tool references, then exercises the actual
  // native cell/remove-tools/OK path without repeating the five tool clicks.
  QJsonArray refs;for(const auto& row:bodies){const auto b=row.toObject();refs.append(QJsonObject{{"bodyId",b.value("bodyId")},{"producerFeatureId",b.value("featureId")}});}
  call(w,"surface.boundaryFillDialog",{{"tools",refs}});dock=w.findChild<QDockWidget*>("boundaryFillDock");QVERIFY(dock);
  QTest::mouseClick(dock->findChild<QPushButton*>("boundaryFill.selectCells"),Qt::LeftButton);QTRY_COMPARE(session(w).value("cells").toArray().size(),1);
  marker=nullptr;for(auto* box:view->findChildren<QCheckBox*>())if(box->objectName().startsWith("boundaryFill.cellMarker.") && box->isVisible())marker=box;QVERIFY(marker);
  QCOMPARE(QApplication::widgetAt(marker->mapToGlobal(marker->rect().center())),static_cast<QWidget*>(marker));QTest::mouseClick(marker,Qt::LeftButton);QTRY_VERIFY(session(w).value("previewed").toBool());QVERIFY(view->hasFocus());
  clickCheck(dock->findChild<QCheckBox*>("boundaryFill.removeTools"));QTRY_VERIFY(session(w).value("inputs").toObject().value("removeTools").toBool());QVERIFY(view->hasFocus());
  buttons=dock->findChild<QDialogButtonBox*>();QVERIFY(buttons->button(QDialogButtonBox::Ok)->isEnabled());QTest::mouseClick(buttons->button(QDialogButtonBox::Ok),Qt::LeftButton);QTRY_VERIFY_WITH_TIMEOUT(session(w).isEmpty(),180000);
  const auto final=call(w,"document.inspect");const auto results=final.value("evaluation").toObject().value("bodies").toArray();QCOMPARE(results.size(),1);QVERIFY(results[0].toObject().value("valid").toBool());
  QCOMPARE(results[0].toObject().value("topology").toObject().value("solids").toInt(),1);call(w,"document.save",{{"path",out+"/native-boundary-filled-iron.alloy"}});
  call(w,"capture.screenshot",{{"path",out+"/native-boundary-fill-solid.png"}});call(w,"history.undo");QCOMPARE(call(w,"document.inspect").value("authored"),before.value("authored"));
 }
};
QTEST_MAIN(BoundaryFillControlsTest)
#include "boundary_fill_controls.moc"
