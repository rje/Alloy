// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QTest>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QWindow>
#ifdef Q_OS_MACOS
#include <dlfcn.h>
#endif
class RetainedMeshPresentationControls final:public QObject {
 Q_OBJECT
 QJsonArray transcript_,steps_;
 QJsonObject native(AlloyWindow& w){
  QJsonObject result;
#ifdef Q_OS_MACOS
  const auto selector=reinterpret_cast<void*(*)(const char*)>(dlsym(RTLD_DEFAULT,"sel_registerName"));
  const auto send=reinterpret_cast<void*(*)(void*,void*)>(dlsym(RTLD_DEFAULT,"objc_msgSend"));
  const auto boolean=reinterpret_cast<bool(*)(void*,void*)>(dlsym(RTLD_DEFAULT,"objc_msgSend"));
  const auto cls=reinterpret_cast<void*(*)(const char*)>(dlsym(RTLD_DEFAULT,"objc_getClass"));
  const auto app=send(cls("NSApplication"),selector("sharedApplication"));
  const auto window=send(reinterpret_cast<void*>(w.winId()),selector("window"));
  result.insert("applicationActive",boolean(app,selector("isActive")));
  result.insert("keyWindowEditor",send(app,selector("keyWindow"))==window);
#endif
  return result;
 }
 QJsonObject call(AlloyWindow& w,const QString& id,const QJsonObject& args={}){
  const auto r=w.execute(id,args);transcript_.append(QJsonObject{{"command",id},{"args",args},{"response",r}});
  if(!r.value("ok").toBool())qFatal("%s",QJsonDocument(r).toJson().constData());return r.value("result").toObject();
 }
 void click(QWidget* surface,const QPoint& point){
  auto* hit=QApplication::widgetAt(surface->mapToGlobal(point));QVERIFY(hit);QVERIFY(hit==surface || surface->isAncestorOf(hit));
  QTest::mouseClick(hit,Qt::LeftButton,Qt::NoModifier,hit->mapFromGlobal(surface->mapToGlobal(point)));
 }
private slots:
 void nativeSequence(){
  const auto output=qEnvironmentVariable("ALLOY_MESH_EVIDENCE"),fixtures=qEnvironmentVariable("ALLOY_MESH_FIXTURES");QVERIFY(!output.isEmpty());QVERIFY(!fixtures.isEmpty());QDir().mkpath(output);
  const auto source=output+"/source.stl";QFile::remove(source);QVERIFY(QFile::copy(fixtures+"/ascii_box_10mm.stl",source));
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1280,900);w.show();w.raise();w.activateWindow();w.windowHandle()->requestActivate();
  QVERIFY(QTest::qWaitForWindowExposed(&w));QTRY_VERIFY(w.isActiveWindow());
  QTRY_VERIFY(native(w).value("applicationActive").toBool(true) && native(w).value("keyWindowEditor").toBool(true));
  ReferenceView* v=nullptr;for(auto* widget:w.findChildren<QWidget*>())if(auto* view=dynamic_cast<ReferenceView*>(widget))v=view;
  QVERIFY(v);auto* tree=w.findChild<QTreeWidget*>();QVERIFY(tree);
  call(w,"solid.box",{{"width",12},{"depth",10},{"height",8},{"x",30},{"y",0},{"z",0},{"units","mm"}});
  const auto body=call(w,"document.inspect").value("evaluation").toObject().value("bodies").toArray().first().toObject().value("bodyId").toString();
  const auto imported=call(w,"document.importStl",{{"path",source},{"units","mm"}});
  const auto mesh=imported.value("meshes").toArray().first().toObject().value("id").toString();QVERIFY(!mesh.isEmpty());
  call(w,"view.orientation",{{"name","top"}});call(w,"view.fit");
  for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->text(0)=="Named Views")(*it)->setExpanded(false);
  auto row=[&]()->QTreeWidgetItem*{for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole+14)==mesh)return *it;return nullptr;};
  QVERIFY(row());QCOMPARE(row()->parent()->text(0),QString("Mesh Bodies"));QVERIFY(!row()->icon(0).isNull());QVERIFY(!row()->isHidden());
  auto expected=[&](const QString& step,const QString& selected,bool visible){
   const auto state=w.state(),selection=state.value("selection").toObject();QCOMPARE(selection.value("bodyId").toString(),selected);
   if(selected==mesh)QCOMPARE(selection.value("kind").toString(),QString("mesh"));
   QCOMPARE(row()->isSelected(),selected==mesh);QCOMPARE(row()->checkState(0),visible ? Qt::Checked : Qt::Unchecked);
   const auto actors=state.value("viewport").toObject().value("gpu").toObject().value("meshes").toObject().value("bodies").toArray();QCOMPARE(actors.size(),1);QCOMPARE(actors[0].toObject().value("displayed").toBool(),visible);
   QVERIFY(w.isActiveWindow());QVERIFY(native(w).value("keyWindowEditor").toBool(true));
   steps_.append(QJsonObject{{"step",step},{"state",state},{"native",native(w)}});
  };
  expected("API import selects mesh",mesh,true);
  click(v,{v->width()-65,v->height()-65});QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(v));expected("canvas clear",{},true);
  auto rect=tree->visualItemRect(row());QVERIFY(!rect.isEmpty());click(tree->viewport(),rect.center());QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(tree));expected("Browser select",mesh,true);
  call(w,"capture.screenshot",{{"path",output+"/selected.png"}});QVERIFY(v->framebuffer().save(output+"/selected-gpu.png"));
  auto checkbox=[&]{const auto r=tree->visualItemRect(row());click(tree->viewport(),{r.left()+8,r.center().y()});QTest::qWait(30);};
  const auto beforeRefusal=call(w,"document.inspect");
  const auto session=call(w,"command.begin",{{"command","solid.box"},{"inputs",QJsonObject{}}});
  checkbox();expected("active-command Browser refusal restores check",mesh,true);
  QCOMPARE(call(w,"document.inspect"),beforeRefusal);
  call(w,"command.cancel",{{"session",session.value("id")}});
  checkbox();expected("Browser hide clears selection",{},false);call(w,"capture.screenshot",{{"path",output+"/hidden.png"}});
  call(w,"history.undo");expected("Undo visibility",{},true);call(w,"history.redo");expected("Redo visibility",{},false);
  checkbox();expected("Browser show",{},true);
  auto canvas=[&](const QString& id){const auto p=call(w,"view.projectBody",{{"bodyId",id}});click(v,{qRound(p.value("x").toDouble()),qRound(p.value("y").toDouble())});QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(v));};
  canvas(mesh);expected("canvas pick mesh",mesh,true);
  // Whole-solid selection uses documented Body Priority, mesh remains whole-object in Auto.
  call(w,"selection.priority",{{"mode","body"}});canvas(body);expected("canvas pick solid",body,true);
  // Selecting the already-active Body Priority toggles it back to Auto.
  call(w,"selection.priority",{{"mode","body"}});canvas(mesh);expected("mesh pick after Body Priority toggles to Auto",mesh,true);
  call(w,"window.resize",{{"width",1360},{"height",840}});QTest::qWait(60);canvas(mesh);expected("resize pick",mesh,true);
  call(w,"capture.screenshot",{{"path",output+"/resized.png"}});QVERIFY(v->framebuffer().save(output+"/resized-gpu.png"));
  const auto gpu=w.state().value("viewport").toObject().value("gpu").toObject();QVERIFY(gpu.value("hardwareAccelerated").toBool());
  const auto geometry=call(w,"mesh.inspect",{{"meshId",mesh}});const auto before=call(w,"document.inspect");
  call(w,"document.save",{{"path",output+"/mixed.alloy"}});QVERIFY(QFile::remove(source));
  call(w,"document.new",{{"discard",true}});QVERIFY(row()->isHidden());
  call(w,"document.open",{{"path",output+"/mixed.alloy"},{"discard",true}});QTRY_VERIFY(!row()->isHidden());
  QCOMPARE(call(w,"mesh.inspect",{{"meshId",mesh}}),geometry);QCOMPARE(call(w,"document.inspect").value("authored"),before.value("authored"));
  call(w,"selection.set",{{"bodyId",mesh}});call(w,"view.fit");expected("source-independent reopen",mesh,true);
  call(w,"capture.screenshot",{{"path",output+"/reopened.png"}});
  QFile report(output+"/report.json");QVERIFY(report.open(QIODevice::WriteOnly));report.write(QJsonDocument(QJsonObject{{"passed",true},{"gpu",gpu},{"steps",steps_},{"transcript",transcript_}}).toJson());w.hide();
 }
};
QTEST_MAIN(RetainedMeshPresentationControls)
#include "retained_mesh_presentation_controls.moc"
