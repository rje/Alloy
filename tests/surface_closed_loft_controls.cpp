// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QAbstractItemView>
#include <QAbstractItemModel>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QDockWidget>
#include <QJsonDocument>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QTest>
#include <QTemporaryDir>
#include <QElapsedTimer>
#include <QLineEdit>
#include "SketchSpline.h"
#include <cmath>
#include <vector>

class SurfaceClosedLoftControlsTest final:public QObject {
 Q_OBJECT
 QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& arguments={}) {
  auto response=window.execute(command,arguments);const auto initial=response.value("result").toObject();QElapsedTimer deadline;deadline.start();
  while(response.value("completion")=="pending"){
   QTest::qWait(25);response=window.execute("command.status",{{"session",initial.value("id")},{"generation",initial.value("previewGeneration")}});
   if(deadline.elapsed()>180000)qFatal("Loft evaluation timed out");
  }
  if(!response.value("ok").toBool())qFatal("%s",QJsonDocument(response).toJson().constData());
  return response["result"].toObject();
 }
 QJsonObject session(AlloyWindow& window){return window.state()["activeSession"].toObject();}
private slots:
 void asynchronousNewBodyContracts(){
  QTemporaryDir temp;QVERIFY(temp.isValid());CadDocument fixture;const auto owner=fixture.authored().value("documentId").toString();
  auto append=[&](const QJsonObject& feature){auto authored=fixture.authored();auto features=authored.value("features").toArray();features.append(feature);authored["features"]=features;if(!fixture.commit(authored,"Fixture").isEmpty())qFatal("Fixture failed");};
  auto lens=[&](double offset){
   QJsonObject plane{{"id",CadDocument::newId()},{"ownerId",owner},{"type","constructionPlane"},{"name","Section plane"},{"visible",true},{"parameters",QJsonObject{{"source",QJsonObject{{"plane","XZ"}}},{"offsetMm",offset}}}};append(plane);
   auto sketch=SketchGeometry::newConstructionPlaneFeature(owner,plane.value("id").toString(),"Lens");
   for(int sign:{1,-1}){QJsonArray points;for(int i=0;i<5;++i){const auto angle=i*std::acos(-1.)/4;points.append(QJsonObject{{"uMm",60-40*std::cos(angle)},{"vMm",30+sign*15*std::sin(angle)}});}const auto result=SketchSpline::add(sketch,{{"points",points}});if(!result.ok)qFatal("Spline fixture failed");sketch=result.feature;}append(sketch);return sketch;
  };
  const auto first=lens(25),last=lens(-25);auto middle=SketchGeometry::newFeature(owner,"XZ","Ellipse");const auto ellipse=SketchGeometry::addCenterPointEllipse(middle,{{"centerUMm",60},{"centerVMm",30},{"firstUMm",100},{"firstVMm",30},{"contourUMm",60},{"contourVMm",45}});QVERIFY(ellipse.ok);middle=ellipse.feature;append(middle);
  auto reference=[](const QJsonObject& sketch){QJsonArray ids;for(const auto& row:sketch.value("parameters").toObject().value("entities").toArray())if(row.toObject().value("type")=="fitSpline" || row.toObject().value("type")=="ellipse")ids.append(row.toObject().value("id"));return QJsonObject{{"sketchId",sketch.value("id")},{"entityIds",ids}};};
  const QJsonArray sections{reference(first),reference(middle),reference(last)};const QJsonObject inputs{{"operation","newBody"},{"guideType","none"},{"sections",sections},{"name","Inner"}};
  const auto path=temp.filePath("source.alloy");QVERIFY(fixture.save(path).isEmpty());AlloyWindow w;call(w,"document.open",{{"path",path},{"discard",true}});const auto before=call(w,"document.inspect");
  auto pending=w.execute("surface.loftDialog",inputs);QCOMPARE(pending.value("completion").toString(),QString("pending"));auto operation=pending.value("result").toObject();
  QVERIFY(w.state().value("geometryWork").toObject().value("resultDiscardAvailable").toBool());auto* name=w.findChild<QLineEdit*>("loftName");QVERIFY(name && !name->isEnabled());auto* buttons=w.findChild<QDockWidget*>("loftDock")->findChild<QDialogButtonBox*>();QVERIFY(buttons->button(QDialogButtonBox::Cancel)->isEnabled());
  QVERIFY(call(w,"command.cancel",{{"session",operation.value("id")}}).value("resultDiscarded").toBool());QTRY_VERIFY_WITH_TIMEOUT(!w.state().value("geometryWork").toObject().value("active").toBool(),10000);QCOMPARE(call(w,"document.inspect"),before);
  pending=w.execute("surface.loftDialog",inputs);operation=pending.value("result").toObject();name=w.findChild<QLineEdit*>("loftName");name->setText("Stale native input");QVERIFY(QMetaObject::invokeMethod(name,"editingFinished",Qt::DirectConnection));QJsonObject stale;
  QTRY_VERIFY_WITH_TIMEOUT((stale=w.execute("command.status",{{"session",operation.value("id")}})).value("completion")!="pending",10000);QCOMPARE(stale.value("error").toObject().value("code").toString(),QString("stale_session"));QCOMPARE(call(w,"document.inspect"),before);call(w,"command.cancel",{{"session",operation.value("id")}});
  auto invalid=inputs;invalid["sections"]=QJsonArray{reference(first),reference(middle),reference(first)};pending=w.execute("surface.loftDialog",invalid);operation=pending.value("result").toObject();QJsonObject failed;
  QTRY_VERIFY_WITH_TIMEOUT((failed=w.execute("command.status",{{"session",operation.value("id")}})).value("completion")!="pending",10000);QCOMPARE(failed.value("completion").toString(),QString("failed"));QCOMPARE(call(w,"document.inspect"),before);call(w,"command.cancel",{{"session",operation.value("id")}});
  const auto ready=call(w,"surface.loftDialog",inputs);QVERIFY(ready.value("previewed").toBool());QCOMPARE(ready.value("inputs").toObject().value("sections"),QJsonValue(sections));QCOMPARE(ready.value("candidateEvaluationCount").toInteger(),qint64(1));const auto repeated=call(w,"command.preview",{{"session",ready.value("id")}});QVERIFY(repeated.value("previewEvaluationReused").toBool());QCOMPARE(repeated.value("candidateEvaluationCount").toInteger(),qint64(1));
  const auto committed=w.execute("command.commit",{{"session",ready.value("id")}});QVERIFY(committed.value("ok").toBool());QCOMPARE(committed.value("completion").toString(),QString("completed"));const auto after=call(w,"document.inspect");QCOMPARE(after.value("undo").toObject().value("count").toInt(),1);
  const auto feature=after.value("authored").toObject().value("features").toArray().last().toObject();auto expectedArgs=inputs;expectedArgs["newFeatureId"]=feature.value("id");expectedArgs["newBodyId"]=feature.value("bodyId");QVERIFY(fixture.commit(CadDocument::loftCandidate(fixture.authored(),expectedArgs),"Baseline").isEmpty());QCOMPARE(after.value("evaluation").toObject().value("bodies"),fixture.state().value("evaluation").toObject().value("bodies"));
  call(w,"history.undo");QCOMPARE(call(w,"document.inspect").value("authored"),before.value("authored"));call(w,"history.redo");QCOMPARE(call(w,"document.inspect").value("authored"),after.value("authored"));
  call(w,"body.remove",{{"bodyId",feature.value("bodyId")}});const auto suffix=call(w,"document.inspect");const auto edit=call(w,"feature.editDialog",{{"featureId",feature.value("id")}});QVERIFY(edit.value("previewed").toBool());QVERIFY(edit.value("geometry").toObject().value("valid").toBool());QVERIFY(call(w,"document.inspect").value("evaluation").toObject().value("bodies").toArray().isEmpty());call(w,"command.cancel",{{"session",edit.value("id")}});QCOMPARE(call(w,"document.inspect"),suffix);
  const auto saved=temp.filePath("saved.alloy");call(w,"document.save",{{"path",saved}});call(w,"document.open",{{"path",saved},{"discard",true}});QCOMPARE(call(w,"document.inspect").value("authored"),suffix.value("authored"));
 }
 void nonTailPreviewAndSourceInvalidation(){
  const auto source=qEnvironmentVariable("ALLOY_CLOSED_LOFT_SOURCE");
  if(source.isEmpty())QSKIP("No prepared closed-section fixture supplied");
  AlloyTheme::apply(*qApp);AlloyWindow window;
  call(window,"document.open",{{"path",source},{"discard",true}});
  const auto initial=call(window,"document.inspect");QJsonObject first,middle,last;
  for(const auto& value:initial["authored"].toObject()["features"].toArray()){
   const auto feature=value.toObject();if(feature["name"]=="First closed spline section")first=feature;
   if(feature["name"]=="Middle full ellipse")middle=feature;
   if(feature["name"]=="Last closed spline section")last=feature;
  }QVERIFY(!first.isEmpty() && !middle.isEmpty() && !last.isEmpty());
  auto reference=[](const QJsonObject& sketch,const QString& type){QJsonArray ids;
   for(const auto& value:sketch["parameters"].toObject()["entities"].toArray())
    if(value.toObject()["type"]==type)ids.append(value.toObject()["id"]);
   return QJsonObject{{"sketchId",sketch["id"]},{"entityIds",ids}};
  };
  auto created=call(window,"surface.loftDialog",{{"operation","newComponent"},{"guideType","none"},
      {"sections",QJsonArray{reference(first,"fitSpline"),reference(middle,"ellipse"),reference(last,"fitSpline")}}});
  QVERIFY(created["previewed"].toBool());call(window,"command.commit",{{"session",created["id"]}});
  const auto feature=call(window,"document.inspect")["authored"].toObject()["features"].toArray().last().toObject();
  call(window,"body.remove",{{"bodyId",feature["bodyId"]}});
  const auto before=call(window,"document.inspect");QVERIFY(before["evaluation"].toObject()["bodies"].toArray().isEmpty());
  const auto opened=call(window,"feature.editDialog",{{"featureId",feature["id"]}});
  // The final accepted document has no body after Remove. The edited earlier
  // Loft still needs its actual through-feature preview and downstream checks.
  QVERIFY(opened["previewed"].toBool());QVERIFY(opened["geometry"].toObject()["valid"].toBool());
  QCOMPARE(opened["geometry"].toObject()["topology"].toObject()["faces"].toInt(),2);
  QCOMPARE(opened["sourcePrefixEvaluationCount"].toInteger(),qint64(1));
  QCOMPARE(opened["candidateEvaluationCount"].toInteger(),qint64(1));
  QCOMPARE(opened["featurePrefixEvaluationCount"].toInteger(),qint64(1));
  const auto repeated=call(window,"command.preview",{{"session",opened["id"]}});
  QVERIFY(repeated["previewEvaluationReused"].toBool());
  QCOMPARE(repeated["sourcePrefixEvaluationCount"].toInteger(),qint64(1));
  QCOMPARE(repeated["candidateEvaluationCount"].toInteger(),qint64(1));
  QCOMPARE(repeated["featurePrefixEvaluationCount"].toInteger(),qint64(1));
  const auto pending=call(window,"command.update",{{"session",opened["id"]},{"inputs",QJsonObject{{"name","Pending earlier Loft"}}}});
  QVERIFY(!pending["previewed"].toBool());QVERIFY(!pending["geometryEvaluated"].toBool());
  QVERIFY(!pending.contains("geometry"));QVERIFY(!pending.contains("documentDraft"));
  auto parameters=first["parameters"].toObject();auto points=parameters["points"].toArray();
  const auto sourcePoint=parameters["entities"].toArray()[0].toObject()["pointIds"].toArray()[1].toString();bool moved=false;
  for(int i=0;i<points.size();++i)if(points[i].toObject()["id"]==sourcePoint){
   auto point=points[i].toObject();point["vMm"]=point["vMm"].toDouble()+1;points[i]=point;moved=true;break;
  }QVERIFY(moved);parameters["points"]=points;
  call(window,"feature.edit",{{"featureId",first["id"]},{"parameters",parameters}});
  const auto changed=call(window,"document.inspect").value("authored");
  const auto refused=window.execute("command.preview",{{"session",opened["id"]}});
  QVERIFY(!refused["ok"].toBool());QCOMPARE(refused["error"].toObject()["code"].toString(),QString("stale_session"));
  QVERIFY(!session(window)["previewed"].toBool());QVERIFY(!session(window).contains("geometry"));
  const auto commit=window.execute("command.commit",{{"session",opened["id"]}});
  QVERIFY(!commit["ok"].toBool());QCOMPARE(commit["error"].toObject()["code"].toString(),QString("stale_session"));
  call(window,"command.cancel",{{"session",opened["id"]}});QCOMPARE(call(window,"document.inspect")["authored"],changed);
 }
 void nativeThreeSectionsCancelAndCommit(){
  const auto source=qEnvironmentVariable("ALLOY_CLOSED_LOFT_SOURCE");
  const auto out=qEnvironmentVariable("ALLOY_CLOSED_LOFT_EVIDENCE");
  QVERIFY(!source.isEmpty() && !out.isEmpty());QDir().mkpath(out);
  AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1040,640);window.show();window.activateWindow();
  QVERIFY(QTest::qWaitForWindowExposed(&window));
  ReferenceView* view=nullptr;for(auto* child:window.findChildren<QWidget*>())
   if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;
  QVERIFY(view);
  call(window,"document.open",{{"path",source},{"discard",true}});call(window,"view.home");
  const auto before=call(window,"document.inspect");
  QJsonObject first,middle,last;
  for(const auto& value:before["authored"].toObject()["features"].toArray()){
   const auto feature=value.toObject();
   if(feature["name"]=="First closed spline section")first=feature;
   if(feature["name"]=="Middle full ellipse")middle=feature;
   if(feature["name"]=="Last closed spline section")last=feature;
  }
  QVERIFY(!first.isEmpty() && !middle.isEmpty() && !last.isEmpty());
  auto pixel=[&](const QString& sketch,double u,double v){
   const auto projected=call(window,"view.projectSketchPoint",{{"sketchId",sketch},{"uMm",u},{"vMm",v}});
   return QPoint(qRound(projected["x"].toDouble()),qRound(projected["y"].toDouble()));};
  const auto dialog=call(window,"surface.loftDialog",{{"componentName","Iron upper"}});
  auto* dock=window.findChild<QDockWidget*>("loftDock");QVERIFY(dock && dock->isVisible());
  auto* guide=dock->findChild<QComboBox*>("loftGuideType");QVERIFY(guide);
  QCOMPARE(guide->currentData().toString(),QString("centerline"));
  QTest::mouseClick(guide,Qt::LeftButton);
  QTRY_COMPARE(QApplication::activePopupWidget(),guide->view()->window());
  auto* popupFocus=QApplication::focusWidget();
  QVERIFY(popupFocus && (popupFocus==guide || guide->view()->window()->isAncestorOf(popupFocus)));
  QTest::keyClick(popupFocus,Qt::Key_Home);
  QTest::keyClick(popupFocus,Qt::Key_Down);
  QTest::keyClick(popupFocus,Qt::Key_Down);
  QTest::keyClick(popupFocus,Qt::Key_Return);
  QTRY_VERIFY(QApplication::activePopupWidget()==nullptr);
  QTRY_COMPARE(session(window)["inputs"].toObject()["guideType"].toString(),QString("none"));
  QTRY_VERIFY(window.findChild<QPushButton*>("loft.section3") &&
   window.findChild<QPushButton*>("loft.section3")->isVisible());
  dock=window.findChild<QDockWidget*>("loftDock");
  QTRY_COMPARE(window.size(),QSize(1040,640));
  struct Pick {QJsonObject sketch;QString role;double u,v;int expected;};
  const std::vector<Pick> picks{{first,"section1",60,45,1},{first,"section1",60,15,2},
   {middle,"section2",88.2842712474619,40.60660171779821,1},
   {last,"section3",60,45,1},{last,"section3",60,15,2}};
  for(const auto& pick:picks) {
   dock=window.findChild<QDockWidget*>("loftDock");QVERIFY(dock && dock->isVisible());
   auto* button=dock->findChild<QPushButton*>("loft."+pick.role);QVERIFY(button);
   QCOMPARE(QApplication::widgetAt(button->mapToGlobal(button->rect().center())),static_cast<QWidget*>(button));
   QTest::mouseClick(button,Qt::LeftButton);
   const auto point=pixel(pick.sketch["id"].toString(),pick.u,pick.v);
   QCOMPARE(QApplication::widgetAt(view->mapToGlobal(point)),static_cast<QWidget*>(view));
   QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,point);
   const int index=pick.role=="section1"?0:pick.role=="section2"?1:2;
   QTRY_COMPARE(session(window)["inputs"].toObject()["sections"].toArray()[index].toObject()["entityIds"].toArray().size(),pick.expected);
  }
  QTRY_VERIFY(session(window)["previewed"].toBool());
  QCOMPARE(session(window)["geometry"].toObject()["topology"].toObject()["faces"].toInt(),2);
  dock=window.findChild<QDockWidget*>("loftDock");
  auto* buttons=dock->findChild<QDialogButtonBox*>();QVERIFY(buttons);
  auto* ok=buttons->button(QDialogButtonBox::Ok),*cancel=buttons->button(QDialogButtonBox::Cancel);
  QVERIFY(ok && cancel && ok->isEnabled());
  auto* scroll=dock->findChild<QScrollArea*>("loft.formScroll");QVERIFY(scroll);
  QTRY_VERIFY(dock->width()>=280 && dock->height()<=window.height());
  QVERIFY(scroll->widget()->width()<=scroll->viewport()->width());
  QVERIFY(scroll->verticalScrollBar()->maximum()>0);
  call(window,"capture.screenshot",{{"path",out+"/minimum-three-section-preview.png"}});
  scroll->verticalScrollBar()->setValue(scroll->verticalScrollBar()->maximum());
  QTRY_COMPARE(scroll->verticalScrollBar()->value(),scroll->verticalScrollBar()->maximum());
  QTest::qWait(50);
  call(window,"capture.screenshot",{{"path",out+"/minimum-guidance-footer.png"}});
  QCOMPARE(QApplication::widgetAt(cancel->mapToGlobal(cancel->rect().center())),static_cast<QWidget*>(cancel));
  QTest::mouseClick(cancel,Qt::LeftButton);
  QTRY_VERIFY(session(window).isEmpty());
  QCOMPARE(call(window,"document.inspect")["authored"],before["authored"]);
  const auto refs=[&](const QJsonObject& sketch,const QString& type){
   QJsonArray result;for(const auto& value:sketch["parameters"].toObject()["entities"].toArray())
    if(value.toObject()["type"]==type)result.append(value.toObject()["id"]);
   return result;};
  const QJsonArray sections{QJsonObject{{"sketchId",first["id"]},{"entityIds",refs(first,"fitSpline")}},
   QJsonObject{{"sketchId",middle["id"]},{"entityIds",refs(middle,"ellipse")}},
   QJsonObject{{"sketchId",last["id"]},{"entityIds",refs(last,"fitSpline")}}};
  call(window,"surface.loftDialog",{{"guideType","none"},{"sections",sections},{"componentName","Iron upper"}});
  dock=window.findChild<QDockWidget*>("loftDock");buttons=dock->findChild<QDialogButtonBox*>();
  ok=buttons->button(QDialogButtonBox::Ok);QTRY_VERIFY(ok->isEnabled());
  QCOMPARE(QApplication::widgetAt(ok->mapToGlobal(ok->rect().center())),static_cast<QWidget*>(ok));
  QTest::mouseClick(ok,Qt::LeftButton);QTRY_VERIFY(session(window).isEmpty());
  const auto committed=call(window,"document.inspect");
  QCOMPARE(committed["authored"].toObject()["features"].toArray().size(),before["authored"].toObject()["features"].toArray().size()+1);
  QCOMPARE(committed["evaluation"].toObject()["bodies"].toArray()[0].toObject()["topology"].toObject()["faces"].toInt(),2);
  call(window,"document.save",{{"path",out+"/native-three-section-sheet.alloy"}});
  window.close();
 }
};
QTEST_MAIN(SurfaceClosedLoftControlsTest)
#include "surface_closed_loft_controls.moc"
