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
#include <QFile>
#include <QLineEdit>
#include <QLabel>
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
 void asynchronousDirectSketchEditContracts(){
  QTemporaryDir temp;QVERIFY(temp.isValid());CadDocument fixture;
  auto sketch=SketchGeometry::newFeature(fixture.authored().value("documentId").toString(),"XZ","Source");
  sketch=SketchGeometry::addLine(sketch,{{"u1Mm",0},{"v1Mm",0},{"u2Mm",20},{"v2Mm",5}});
  auto base=fixture.authored();base["features"]=QJsonArray{sketch};QVERIFY(fixture.commit(base,"Sketch").isEmpty());
  const auto entity=sketch.value("parameters").toObject().value("entities").toArray()[0].toObject().value("id");
  QVERIFY(fixture.commit(CadDocument::surfaceExtrudeCandidate(fixture.authored(),{{"sketchId",sketch.value("id")},{"entityIds",QJsonArray{entity}},{"sideOneMm",10},{"sideTwoMm",12}}),"Sheet").isEmpty());
  // Non-sketch direct edits keep their existing synchronous route.
  const auto sheet=fixture.authored().value("features").toArray().last().toObject();
  const auto path=temp.filePath("source.alloy");QVERIFY(fixture.save(path).isEmpty());AlloyWindow w;
  const auto capture=qEnvironmentVariable("ALLOY_REL041_CAPTURE");
  if(!capture.isEmpty()){w.resize(1440,900);w.show();QVERIFY(QTest::qWaitForWindowExposed(&w));}
  call(w,"document.open",{{"path",path},{"discard",true}});
  const auto same=w.execute("feature.edit",{{"featureId",sheet.value("id")},{"parameters",QJsonObject{{"sideOneMm",11}}}});
  QVERIFY(same.value("ok").toBool());QCOMPARE(same.value("completion").toString(),QString("completed"));call(w,"history.undo");
  // Reset history after verifying the unrelated route.
  call(w,"document.open",{{"path",path},{"discard",true}});const auto before=call(w,"document.inspect");
  auto parameters=SketchGeometry::findFeature(fixture.authored(),sketch.value("id").toString()).value("parameters").toObject();
  auto points=parameters.value("points").toArray();auto point=points[1].toObject();point["uMm"]=25;points[1]=point;parameters["points"]=points;
  const QJsonObject args{{"featureId",sketch.value("id")},{"parameters",parameters}};
  auto pending=w.execute("feature.edit",args);QCOMPARE(pending.value("completion").toString(),QString("pending"));const auto operation=pending.value("result").toObject();
  QVERIFY(w.statusBar()->currentMessage().startsWith("Updating sketch and downstream geometry"));
  if(!capture.isEmpty()){w.repaint();QVERIFY(w.grab().save(capture));}
  QCOMPARE(call(w,"document.inspect"),before);QVERIFY(session(w).isEmpty());
  QCOMPARE(w.execute("history.undo").value("error").toObject().value("code").toString(),QString("evaluation_busy"));
  QVERIFY(w.execute("command.cancel",{{"session",operation.value("id")}}).value("ok").toBool());
  QTRY_VERIFY_WITH_TIMEOUT(!w.state().value("geometryWork").toObject().value("active").toBool(),10000);
  QCOMPARE(call(w,"document.inspect"),before);QCOMPARE(w.execute("command.status",{{"session",operation.value("id")}}).value("error").toObject().value("code").toString(),QString("command_cancelled"));
  const auto after=call(w,"feature.edit",args);QVERIFY(after.contains("authored"));QVERIFY(after.value("authored")!=before.value("authored"));
  QCOMPARE(after.value("undo").toObject().value("count").toInt(),1);QVERIFY(after.value("evaluation").toObject().value("bodies")!=before.value("evaluation").toObject().value("bodies"));
  QVERIFY(!w.state().value("sketchEvaluation").toObject().contains("authored"));
  call(w,"history.undo");QCOMPARE(call(w,"document.inspect").value("authored"),before.value("authored"));call(w,"history.redo");QCOMPARE(call(w,"document.inspect").value("authored"),after.value("authored"));
  auto invalid=parameters;auto entities=invalid.value("entities").toArray();auto line=entities[0].toObject();line["construction"]=true;entities[0]=line;invalid["entities"]=entities;
  pending=w.execute("feature.edit",{{"featureId",sketch.value("id")},{"parameters",invalid}});const auto failedId=pending.value("result").toObject().value("id");QJsonObject failed;
  QTRY_VERIFY_WITH_TIMEOUT((failed=w.execute("command.status",{{"session",failedId}})).value("completion")!="pending",10000);
  QCOMPARE(failed.value("completion").toString(),QString("failed"));QCOMPARE(call(w,"document.inspect").value("authored"),after.value("authored"));
  const auto saved=temp.filePath("edited.alloy");call(w,"document.save",{{"path",saved}});call(w,"document.open",{{"path",saved},{"discard",true}});QCOMPARE(call(w,"document.inspect").value("authored"),after.value("authored"));
 }
 void asynchronousHistoricalSketchContracts(){
  QTemporaryDir temp;QVERIFY(temp.isValid());CadDocument fixture;
  auto sketch=SketchGeometry::newFeature(fixture.authored().value("documentId").toString(),"XZ","Historical source");
  sketch=SketchGeometry::addLine(sketch,{{"u1Mm",0},{"v1Mm",0},{"u2Mm",20},{"v2Mm",5}});
  auto base=fixture.authored();base["features"]=QJsonArray{sketch};QVERIFY(fixture.commit(base,"Sketch").isEmpty());
  const auto entity=sketch.value("parameters").toObject().value("entities").toArray()[0].toObject().value("id");
  QVERIFY(fixture.commit(CadDocument::surfaceExtrudeCandidate(fixture.authored(),{{"sketchId",sketch.value("id")},{"entityIds",QJsonArray{entity}},{"sideOneMm",10},{"sideTwoMm",12}}),"Sheet").isEmpty());
  const auto path=temp.filePath("historical.alloy");QVERIFY(fixture.save(path).isEmpty());
  AlloyWindow w;w.resize(1400,900);w.show();w.activateWindow();QApplication::setActiveWindow(&w);w.setFocus();QVERIFY(QTest::qWaitForWindowExposed(&w));
  call(w,"document.open",{{"path",path},{"discard",true}});const auto before=call(w,"document.inspect");
  auto pending=w.execute("sketch.edit",{{"sketchId",sketch.value("id")}});QCOMPARE(pending.value("completion").toString(),QString("pending"));QVERIFY(session(w).isEmpty());
  QVERIFY(w.state().value("documentOpen").toObject().isEmpty());
  const auto openId=pending.value("result").toObject().value("id");
  QCOMPARE(w.execute("sketch.line",{{"x1",0},{"y1",10},{"x2",10},{"y2",10}}).value("error").toObject().value("code").toString(),QString("evaluation_busy"));
  QVERIFY(w.execute("command.cancel",{{"session",openId}}).value("result").toObject().value("resultDiscarded").toBool());
  QTRY_VERIFY_WITH_TIMEOUT(!w.state().value("geometryWork").toObject().value("active").toBool(),10000);QVERIFY(session(w).isEmpty());QCOMPARE(call(w,"document.inspect"),before);
  const auto opened=call(w,"feature.editDialog",{{"featureId",sketch.value("id")}});const auto sid=opened.value("id");
  QCOMPARE(session(w).value("id"),sid);QCOMPARE(session(w).value("draftSketch"),QJsonValue(SketchGeometry::findFeature(fixture.authored(),sketch.value("id").toString())));
  call(w,"sketch.line",{{"x1",0},{"y1",15},{"x2",10},{"y2",15},{"units","mm"}});
  const auto extra=session(w).value("draftSketch").toObject().value("parameters").toObject().value("entities").toArray().last().toObject().value("id");
  call(w,"sketch.construction",{{"entityIds",QJsonArray{extra}},{"construction",true}});const auto retained=session(w);
  ReferenceView* view=nullptr;for(auto* widget:w.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(widget)){view=candidate;break;}QVERIFY(view);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
  pending=w.execute("sketch.finish");QCOMPARE(pending.value("completion").toString(),QString("pending"));const auto finish=pending.value("result").toObject();
  QCOMPARE(session(w).value("draftSketch"),retained.value("draftSketch"));auto* name=w.findChild<QLineEdit*>("sketch.name");QVERIFY(name && !name->isEnabled());
  QCOMPARE(w.execute("command.status",{{"session",finish.value("id")},{"generation",finish.value("previewGeneration").toInteger()+1}}).value("error").toObject().value("code").toString(),QString("stale_preview"));
  const auto center=view->rect().center();QCOMPARE(QApplication::widgetAt(view->mapToGlobal(center)),static_cast<QWidget*>(view));
  QTest::keyClick(view,Qt::Key_L);QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,center);QCOMPARE(session(w).value("draftSketch"),retained.value("draftSketch"));
  QVERIFY(w.execute("sketch.cancel").value("result").toObject().value("resultDiscarded").toBool());
  QTRY_VERIFY_WITH_TIMEOUT(!w.state().value("geometryWork").toObject().value("active").toBool(),10000);QCOMPARE(session(w),retained);QVERIFY(name->isEnabled());QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QCOMPARE(call(w,"document.inspect"),before);
  call(w,"sketch.construction",{{"entityIds",QJsonArray{entity}},{"construction",true}});const auto invalid=session(w);
  pending=w.execute("sketch.finish");QCOMPARE(pending.value("completion").toString(),QString("pending"));const auto failedId=pending.value("result").toObject().value("id");QJsonObject failed;
  QTRY_VERIFY_WITH_TIMEOUT((failed=w.execute("command.status",{{"session",failedId}})).value("completion")!="pending",10000);
  QCOMPARE(failed.value("completion").toString(),QString("failed"));QCOMPARE(session(w),invalid);QCOMPARE(call(w,"document.inspect"),before);QVERIFY(name->isEnabled());
  auto* error=w.findChild<QLabel*>("sketch.error");QVERIFY(error && error->isVisible() && !error->text().isEmpty());
  call(w,"sketch.construction",{{"entityIds",QJsonArray{entity}},{"construction",false}});pending=w.execute("sketch.finish");QCOMPARE(pending.value("completion").toString(),QString("pending"));QVERIFY(error->text().isEmpty());
  const auto retryId=pending.value("result").toObject().value("id");QJsonObject completed;
  QTRY_COMPARE_WITH_TIMEOUT((completed=w.execute("command.status",{{"session",retryId}})).value("completion").toString(),QString("completed"),10000);
  QVERIFY(completed.value("ok").toBool());QCOMPARE(completed.value("result").toObject().value("status").toString(),QString("committed"));QVERIFY(session(w).isEmpty());
  const auto progress=w.state().value("sketchEvaluation").toObject();QCOMPARE(progress.value("evaluationStatus").toString(),QString("completed"));QVERIFY(!progress.contains("document"));QVERIFY(!progress.contains("draftSketch"));
  QVERIFY(completed.value("result").toObject().value("document").toObject().contains("authored"));
  const auto after=call(w,"document.inspect");QVERIFY(after.value("authored")!=before.value("authored"));QCOMPARE(after.value("undo").toObject().value("count").toInt(),1);
  QCOMPARE(after.value("evaluation").toObject().value("bodies"),before.value("evaluation").toObject().value("bodies"));
  call(w,"history.undo");QCOMPARE(call(w,"document.inspect").value("authored"),before.value("authored"));call(w,"history.redo");QCOMPARE(call(w,"document.inspect").value("authored"),after.value("authored"));
  const auto saved=temp.filePath("finished.alloy");call(w,"document.save",{{"path",saved}});call(w,"document.open",{{"path",saved},{"discard",true}});QCOMPARE(call(w,"document.inspect").value("authored"),after.value("authored"));
  call(w,"sketch.begin",{{"plane","XY"},{"name","New sketch regression"}});call(w,"sketch.line",{{"x1",0},{"y1",0},{"x2",10},{"y2",0},{"units","mm"}});
  const auto newFinish=w.execute("sketch.finish");QVERIFY(newFinish.value("ok").toBool());QCOMPARE(newFinish.value("completion").toString(),QString("completed"));QVERIFY(session(w).isEmpty());
  const auto expected=fixture.authored();const auto revision=fixture.revision();fixture.reset();QVERIFY(!fixture.commitSketchEvaluation(CadDocument::evaluateCandidate(expected),expected,expected,revision).isEmpty());
 }
 void asynchronousDocumentOpenContracts(){
  QTemporaryDir temp;QVERIFY(temp.isValid());CadDocument target;
  auto authored=target.authored();const auto owner=authored.value("documentId");
  authored["features"]=QJsonArray{QJsonObject{{"id",CadDocument::newId()},{"bodyId",CadDocument::newId()},
   {"ownerId",owner},{"type","box"},{"name","Loaded box"},{"visible",true},
   {"parameters",QJsonObject{{"widthMm",25},{"depthMm",20},{"heightMm",10},{"xMm",0},{"yMm",0},{"zMm",0}}}}};
  QVERIFY(target.commit(authored,"Box").isEmpty());const auto path=temp.filePath("target.alloy");QVERIFY(target.save(path).isEmpty());
  const auto prepared=CadDocument::prepareOpen(path);QVERIFY(prepared.ok);CadDocument guard;const auto base=guard.authored();const auto revision=guard.revision();
  guard.reset();QVERIFY(!guard.adoptOpen(prepared,path,base,revision).isEmpty());QVERIFY(guard.authored()!=authored);
  AlloyWindow w;
  const auto capture=qEnvironmentVariable("ALLOY_REL036_CAPTURE");
  if(!capture.isEmpty()){w.resize(1440,900);w.show();QVERIFY(QTest::qWaitForWindowExposed(&w));}
  call(w,"document.units",{{"units","in"}});const auto before=call(w,"document.inspect");const auto tabs=call(w,"document.tabs");
  const auto draft=call(w,"command.begin",{{"command","solid.box"},{"inputs",QJsonObject{}}});const auto activeBefore=session(w);
  QCOMPARE(w.execute("document.open",{{"path",path}}).value("error").toObject().value("code").toString(),QString("unsaved_changes"));
  auto pending=w.execute("document.open",{{"path",path},{"discard",true}});QCOMPARE(pending.value("completion").toString(),QString("pending"));
  const auto request=pending.value("result").toObject();const auto id=request.value("id"),generation=request.value("previewGeneration");
  QCOMPARE(call(w,"document.inspect"),before);QCOMPARE(session(w),activeBefore);
  QCOMPARE(w.execute("document.tabNew").value("error").toObject().value("code").toString(),QString("evaluation_busy"));
  QCOMPARE(w.execute("command.update",{{"session",draft.value("id")},{"inputs",QJsonObject{{"width",5}}}}).value("error").toObject().value("code").toString(),QString("evaluation_busy"));
  QCOMPARE(w.execute("command.status",{{"session",id},{"generation",generation.toInteger()+1}}).value("error").toObject().value("code").toString(),QString("stale_preview"));
  auto* primitive=w.findChild<QLineEdit*>("primitive.name");QVERIFY(primitive && !primitive->isEnabled());
  auto* status=w.findChild<QLabel*>("application.status");QVERIFY(status && status->isHidden());
  if(!capture.isEmpty()){w.repaint();QVERIFY(w.grab().save(capture));}
  QCOMPARE(w.execute("command.cancel",{{"session",id},{"unexpected",true}}).value("error").toObject().value("code").toString(),QString("invalid_arguments"));
  QVERIFY(w.findChild<QPushButton*>("document.cancelOpen"));QVERIFY(w.execute("command.cancel",{{"session",id}}).value("result").toObject().value("resultDiscarded").toBool());
  QCOMPARE(w.execute("command.status",{{"session",id},{"generation",generation}}).value("error").toObject().value("code").toString(),QString("command_cancelled"));
  QTRY_VERIFY_WITH_TIMEOUT(!w.state().value("geometryWork").toObject().value("active").toBool(),10000);
  QCOMPARE(call(w,"document.inspect"),before);QCOMPARE(session(w),activeBefore);QCOMPARE(call(w,"document.tabs"),tabs);QVERIFY(primitive->isEnabled());QVERIFY(!status->isHidden());
  const auto badPath=temp.filePath("bad.alloy");QFile bad(badPath);QVERIFY(bad.open(QIODevice::WriteOnly));bad.write("{broken");bad.close();
  pending=w.execute("document.open",{{"path",badPath},{"discard",true}});QCOMPARE(pending.value("completion").toString(),QString("pending"));
  const auto badId=pending.value("result").toObject().value("id");QJsonObject failed;
  QTRY_VERIFY_WITH_TIMEOUT((failed=w.execute("command.status",{{"session",badId}})).value("completion")!="pending",10000);
  QCOMPARE(failed.value("error").toObject().value("code").toString(),QString("open_failed"));QCOMPARE(call(w,"document.inspect"),before);QCOMPARE(session(w),activeBefore);
  QCOMPARE(w.execute("command.status",{{"session",id}}).value("error").toObject().value("code").toString(),QString("invalid_session"));
  call(w,"command.cancel",{{"session",draft.value("id")}});
  const auto loft=call(w,"solid.loftDialog");auto* name=w.findChild<QLineEdit*>("loftName");QVERIFY(name);
  pending=w.execute("document.open",{{"path",path},{"discard",true}});const auto staleId=pending.value("result").toObject().value("id");
  QVERIFY(!name->isEnabled());name->setText("Changed after Open request");
  // Exercise the direct native callback even though real input is frozen.
  QVERIFY(QMetaObject::invokeMethod(name,"editingFinished",Qt::DirectConnection));
  const auto changedDraft=session(w);QVERIFY(changedDraft.value("inputs").toObject().value("name")=="Changed after Open request");
  QTRY_VERIFY_WITH_TIMEOUT((failed=w.execute("command.status",{{"session",staleId}})).value("completion")!="pending",10000);
  QCOMPARE(failed.value("error").toObject().value("code").toString(),QString("stale_open"));QCOMPARE(call(w,"document.inspect"),before);
  QCOMPARE(session(w),changedDraft);QVERIFY(name->isEnabled());call(w,"command.cancel",{{"session",loft.value("id")}});
  const auto loaded=call(w,"document.open",{{"path",path},{"discard",true}});QCOMPARE(loaded.value("authored"),QJsonValue(authored));QVERIFY(session(w).isEmpty());
  auto current=call(w,"document.inspect");QCOMPARE(current.value("authored"),QJsonValue(authored));QVERIFY(loaded.value("recovery").toObject().value("durable").toBool());QVERIFY(!current.value("dirty").toBool());QCOMPARE(current.value("undo").toObject().value("count").toInt(),0);
  QVERIFY(!w.state().value("documentOpen").toObject().contains("authored"));
  const auto loadedId=loaded.value("id"); // Operation ID is distinct from the loaded document ID.
  QCOMPARE(w.execute("command.status",{{"session",loadedId}}).value("completion").toString(),QString("completed"));
  call(w,"document.units",{{"units","in"}});call(w,"history.undo");QCOMPARE(call(w,"document.inspect").value("authored"),QJsonValue(authored));
  call(w,"history.redo");QCOMPARE(call(w,"document.inspect").value("units").toString(),QString("in"));
  QCOMPARE(w.execute("command.status",{{"session",loadedId}}).value("error").toObject().value("code").toString(),QString("invalid_session"));
 }
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
  const auto enumeration=call(w,"surface.boundaryFillDialog",{{"tools",tools},{"removeTools",true}});const auto enumerationId=enumeration.value("id");
  auto pending=w.execute("surface.boundaryFillSelectCells");QVERIFY(pending.value("ok").toBool());QCOMPARE(pending.value("completion").toString(),QString("pending"));
  const auto enumerationGeneration=pending.value("result").toObject().value("previewGeneration");
  QCOMPARE(pending.value("result").toObject().value("evaluationPhase").toString(),QString("partition"));QVERIFY(!session(w).value("geometryEvaluated").toBool());
  QCOMPARE(w.execute("surface.boundaryFillSelectCells").value("result").toObject().value("previewGeneration"),enumerationGeneration);
  QCOMPARE(w.execute("command.commit",{{"session",enumerationId}}).value("error").toObject().value("code").toString(),QString("evaluation_pending"));
  QCOMPARE(w.execute("command.update",{{"session",enumerationId},{"inputs",QJsonObject{{"tools",QJsonArray{}}}}}).value("error").toObject().value("code").toString(),QString("evaluation_busy"));
  QCOMPARE(w.execute("command.status",{{"session",enumerationId},{"generation",enumerationGeneration.toInteger()+1}}).value("error").toObject().value("code").toString(),QString("stale_preview"));
  const auto discarded=w.execute("command.cancel",{{"session",enumerationId}});QVERIFY(discarded.value("result").toObject().value("resultDiscarded").toBool());
  QTRY_VERIFY_WITH_TIMEOUT(!w.state().value("geometryWork").toObject().value("active").toBool(),10000);QVERIFY(session(w).isEmpty());QCOMPARE(call(w,"document.inspect").value("authored"),before.value("authored"));
  const auto openTools=call(w,"surface.boundaryFillDialog",{{"tools",QJsonArray{tools[0],tools[1]}}});
  const auto noCells=w.execute("surface.boundaryFillSelectCells");QCOMPARE(noCells.value("completion").toString(),QString("pending"));
  QTRY_COMPARE_WITH_TIMEOUT(session(w).value("evaluationStatus").toString(),QString("failed"),10000);
  const auto noCellStatus=w.execute("command.status",{{"session",openTools.value("id")},{"generation",noCells.value("result").toObject().value("previewGeneration")}});
  QCOMPARE(noCellStatus.value("completion").toString(),QString("failed"));QCOMPARE(noCellStatus.value("error").toObject().value("code").toString(),QString("boundary_fill_no_cells"));
  QVERIFY(!session(w).value("previewed").toBool());QVERIFY(session(w).value("cells").toArray().isEmpty());
  const auto cachedNoCells=w.execute("surface.boundaryFillSelectCells");QCOMPARE(cachedNoCells.value("completion").toString(),QString("failed"));
  QCOMPARE(cachedNoCells.value("error").toObject().value("code").toString(),QString("boundary_fill_no_cells"));QCOMPARE(session(w).value("evaluationStatus").toString(),QString("failed"));
  QCOMPARE(w.execute("command.commit",{{"session",openTools.value("id")}}).value("error").toObject().value("code").toString(),QString("boundary_fill_cell_required"));
  call(w,"command.cancel",{{"session",openTools.value("id")}});QCOMPARE(call(w,"document.inspect").value("authored"),before.value("authored"));
  auto active=chooseCell();QCOMPARE(active.value("partitionBuildCount").toInteger(),qint64(1));QVERIFY(active.value("partitionReused").toBool());
  QCOMPARE(w.execute("command.status",{{"session",enumerationId}}).value("error").toObject().value("code").toString(),QString("invalid_session"));
  const auto sid=active.value("id"),cell=active.value("inputs").toObject().value("cell");
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
  QTest::mouseClick(dock->findChild<QPushButton*>("boundaryFill.selectCells"),Qt::LeftButton);QTRY_COMPARE_WITH_TIMEOUT(session(w).value("cells").toArray().size(),1,180000);QVERIFY(view->hasFocus());
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
  QTRY_COMPARE_WITH_TIMEOUT(session(w).value("cells").toArray().size(),1,180000);QCOMPARE(session(w).value("partitionBuildCount").toInteger(),qint64(1));QVERIFY(!buttons->button(QDialogButtonBox::Ok)->isEnabled());QVERIFY(view->hasFocus());
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
  QTest::mouseClick(dock->findChild<QPushButton*>("boundaryFill.selectCells"),Qt::LeftButton);QTRY_COMPARE_WITH_TIMEOUT(session(w).value("cells").toArray().size(),1,180000);
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
