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

class BodySelectionSetControls final:public QObject {
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
 QJsonArray members(AlloyWindow& w){return w.state().value("selection").toObject().value("members").toArray();}
 void click(QWidget* surface,const QPoint& point,Qt::KeyboardModifiers modifiers){
  auto* hit=QApplication::widgetAt(surface->mapToGlobal(point));
  QVERIFY(hit);QVERIFY(hit==surface || surface->isAncestorOf(hit));
  QTest::mouseClick(hit,Qt::LeftButton,modifiers,hit->mapFromGlobal(surface->mapToGlobal(point)));
 }
private slots:
 void nativeSequence(){
  const auto output=qEnvironmentVariable("ALLOY_BODY_SET_EVIDENCE");QVERIFY(!output.isEmpty());QDir().mkpath(output);
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1440,900);w.show();w.raise();w.activateWindow();w.windowHandle()->requestActivate();
  QVERIFY(QTest::qWaitForWindowExposed(&w));QTRY_VERIFY(w.isActiveWindow());
  QTRY_VERIFY(native(w).value("applicationActive").toBool(true) && native(w).value("keyWindowEditor").toBool(true));
  ReferenceView* v=nullptr;for(auto* widget:w.findChildren<QWidget*>())if(auto* view=dynamic_cast<ReferenceView*>(widget))v=view;
  QVERIFY(v);auto* tree=w.findChild<QTreeWidget*>();QVERIFY(tree);
  for(const int x:{-60,0,60})call(w,"solid.box",{{"width",30},{"depth",25},{"height",20},{"x",x},{"y",0},{"z",0},{"units","mm"}});
  const auto bodies=call(w,"document.inspect").value("evaluation").toObject().value("bodies").toArray();
  const auto a=bodies[0].toObject().value("bodyId").toString(),b=bodies[1].toObject().value("bodyId").toString(),c=bodies[2].toObject().value("bodyId").toString();
  call(w,"component.createFromBodies",{{"bodyIds",QJsonArray{c}}});
  const auto original=call(w,"document.inspect").value("authored").toObject().value("occurrences").toArray().first().toObject().value("id").toString();
  const auto copied=call(w,"component.copyOccurrence",{{"occurrenceId",original},{"placement",QJsonObject{{"axis","Z"},{"angleDeg",0},{"translationMm",QJsonArray{0,50,0}}}}}).value("rootOccurrenceId").toString();
  call(w,"view.home");call(w,"view.orientation",{{"name","top"}});call(w,"selection.priority",{{"mode","body"}});
  call(w,"document.save",{{"path",output+"/fixture.alloy"}});const auto before=call(w,"document.inspect");
  // Initial fixture framing exposes the body rows before demonstrated input.
  for(QTreeWidgetItemIterator it(tree);*it;++it){
   if((*it)->text(0)=="Named Views")(*it)->setExpanded(false);
   const auto folder=(*it)->data(0,Qt::UserRole+4).toString();if(folder=="bodies" || folder=="instanceBodies")(*it)->setExpanded(true);
  }
  steps_.append(QJsonObject{{"step","initial"},{"state",w.state()},{"native",native(w)}});
  auto expected=[&](const QString& step,const QJsonArray& ids){
   QJsonArray rows;for(const auto& id:ids)rows.append(QJsonObject{{"bodyId",id},{"occurrenceId",""}});
   QCOMPARE(members(w),rows);QVERIFY(w.isActiveWindow());
   QJsonArray browserMembers;for(auto* row:tree->selectedItems())if(!row->data(0,Qt::UserRole+1).toString().isEmpty())
    browserMembers.append(QJsonObject{{"bodyId",row->data(0,Qt::UserRole+1).toString()},{"occurrenceId",row->data(0,Qt::UserRole+8).toString()}});
   QCOMPARE(browserMembers.size(),rows.size());for(const auto& row:rows)QVERIFY(browserMembers.contains(row));
   const auto readiness=native(w);QVERIFY(readiness.value("applicationActive").toBool(true) && readiness.value("keyWindowEditor").toBool(true));
   steps_.append(QJsonObject{{"step",step},{"state",w.state()},{"native",readiness}});
  };
  auto canvas=[&](const QString& id,Qt::KeyboardModifiers modifiers){
   const auto p=call(w,"view.projectBody",{{"bodyId",id}});click(v,{qRound(p.value("x").toDouble()),qRound(p.value("y").toDouble())},modifiers);
   QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(v));
  };
  canvas(a,Qt::NoModifier);expected("replace A",{a});
  call(w,"capture.screenshot",{{"path",output+"/single.png"}});
  canvas(b,Qt::ShiftModifier);expected("add B",{a,b});
  call(w,"capture.screenshot",{{"path",output+"/multi.png"}});
  canvas(b,Qt::ShiftModifier);expected("add B again",{a,b});
  canvas(a,Qt::ControlModifier);expected("toggle A",{b});
  canvas(b,Qt::ControlModifier);expected("toggle B",{});
  canvas(a,Qt::NoModifier);expected("replace A again",{a});
  auto browser=[&](const QString& id,Qt::KeyboardModifiers modifiers){
   QTreeWidgetItem* row=nullptr;for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole+1)==id){row=*it;break;}
   QVERIFY(row);const auto rect=tree->visualItemRect(row);QVERIFY(!rect.isEmpty());
   click(tree->viewport(),{rect.center().x(),rect.center().y()},modifiers);
   QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(tree));
  };
  browser(b,Qt::NoModifier);expected("Browser replace B",{b});
  browser(a,Qt::ShiftModifier);expected("Browser add A",{b,a});
  browser(a,Qt::ShiftModifier);expected("Browser idempotent A",{b,a});
  browser(b,Qt::ControlModifier);expected("Browser toggle B",{a});
  browser(a,Qt::ControlModifier);expected("Browser toggle A",{});
  // Actual occurrence hits retain separate membership for the same body definition.
  auto occurrenceCanvas=[&](const QString& occurrence,Qt::KeyboardModifiers modifiers){
   const auto p=call(w,"view.projectBody",{{"bodyId",c},{"occurrenceId",occurrence}});
   click(v,{qRound(p.value("x").toDouble()),qRound(p.value("y").toDouble())},modifiers);
   QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(v));
   const auto readiness=native(w);QVERIFY(readiness.value("applicationActive").toBool(true) && readiness.value("keyWindowEditor").toBool(true));
   steps_.append(QJsonObject{{"step","canvas occurrence "+occurrence},{"state",w.state()},{"native",readiness}});
  };
  occurrenceCanvas(original,Qt::NoModifier);
  QCOMPARE(members(w),QJsonArray({QJsonObject{{"bodyId",c},{"occurrenceId",original}}}));
  occurrenceCanvas(copied,Qt::ShiftModifier);
  QCOMPARE(members(w),QJsonArray({QJsonObject{{"bodyId",c},{"occurrenceId",original}},QJsonObject{{"bodyId",c},{"occurrenceId",copied}}}));
  int highlighted=0;for(auto* row:tree->selectedItems())if(row->data(0,Qt::UserRole+1)==c)++highlighted;QCOMPARE(highlighted,2);
  call(w,"capture.screenshot",{{"path",output+"/linked.png"}});
  const auto state=w.state();QVERIFY(state.value("viewport").toObject().value("gpu").toObject().value("hardwareAccelerated").toBool());
  call(w,"selection.set",{{"operation","clear"}});expected("API clear",{});
  call(w,"capture.screenshot",{{"path",output+"/clear.png"}});
  QCOMPARE(call(w,"document.inspect"),before);
  call(w,"selection.set",{{"bodyId",a}});call(w,"selection.set",{{"bodyId",b},{"operation","add"}});
  call(w,"window.resize",{{"width",1320},{"height",850}});QTest::qWait(60);QCOMPARE(members(w).size(),2);
  call(w,"capture.screenshot",{{"path",output+"/resized.png"}});
  QFile report(output+"/report.json");QVERIFY(report.open(QIODevice::WriteOnly));report.write(QJsonDocument(QJsonObject{{"passed",true},{"state",state},{"resizedState",w.state()},{"steps",steps_},{"transcript",transcript_}}).toJson());w.hide();
 }
};
QTEST_MAIN(BodySelectionSetControls)
#include "body_selection_set_controls.moc"
