// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QLineEdit>
#include <QListWidget>
#include <QProcess>
#include <QScopeGuard>
#include <QTest>
#include <QToolButton>
#include <QWindow>
#ifdef Q_OS_MACOS
#include <dlfcn.h>
#endif
class CommandSearchControls final:public QObject {
 Q_OBJECT
 QJsonArray steps;
 QJsonObject native(QWidget& widget){
  QJsonObject r;
#ifdef Q_OS_MACOS
  const auto sel=reinterpret_cast<void*(*)(const char*)>(dlsym(RTLD_DEFAULT,"sel_registerName"));
  const auto send=reinterpret_cast<void*(*)(void*,void*)>(dlsym(RTLD_DEFAULT,"objc_msgSend"));
  const auto boolean=reinterpret_cast<bool(*)(void*,void*)>(dlsym(RTLD_DEFAULT,"objc_msgSend"));
  const auto cls=reinterpret_cast<void*(*)(const char*)>(dlsym(RTLD_DEFAULT,"objc_getClass"));
  auto* app=send(cls("NSApplication"),sel("sharedApplication"));
  r={{"applicationActive",boolean(app,sel("isActive"))},{"expectedKeyWindow",send(app,sel("keyWindow"))==send(reinterpret_cast<void*>(widget.winId()),sel("window"))}};
#endif
  return r;
 }
 QJsonObject call(AlloyWindow& w,const QString& id,const QJsonObject& args={}) {
  auto response=w.execute(id,args);steps.append(QJsonObject{{"command",id},{"args",args},{"response",response}});
  if(!response.value("ok").toBool())qFatal("%s",QJsonDocument(response).toJson().constData());return response.value("result").toObject();
 }
 void observe(AlloyWindow& w,QWidget& expected,const QString& name){steps.append(QJsonObject{{"step",name},{"native",native(expected)},{"state",w.state()}});}
 void click(QWidget* surface,const QPoint& point){
  auto* hit=QApplication::widgetAt(surface->mapToGlobal(point));QVERIFY(hit);QVERIFY(hit==surface || surface->isAncestorOf(hit));
  QTest::mouseClick(hit,Qt::LeftButton,Qt::NoModifier,hit->mapFromGlobal(surface->mapToGlobal(point)));
 }
private slots:
 void sequence(){
  const auto output=qEnvironmentVariable("ALLOY_SEARCH_EVIDENCE");QVERIFY(!output.isEmpty());QDir().mkpath(output);
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1280,900);w.show();w.raise();w.activateWindow();w.windowHandle()->requestActivate();QVERIFY(QTest::qWaitForWindowExposed(&w));
  const auto report=qScopeGuard([&]{QFile file(output+"/report.json");if(file.open(QIODevice::WriteOnly))file.write(QJsonDocument(QJsonObject{{"passed",!QTest::currentTestFailed()},{"steps",steps},{"finalState",w.state()}}).toJson());});
  QProcess activation;activation.start("osascript",{"-e",QString("tell application \"System Events\" to set frontmost of first application process whose unix id is %1 to true").arg(QCoreApplication::applicationPid())});QVERIFY(activation.waitForFinished(5000));QCOMPARE(activation.exitCode(),0);
  QTRY_VERIFY(native(w).value("applicationActive").toBool(true));QTRY_VERIFY(native(w).value("expectedKeyWindow").toBool(true));
  auto* button=w.findChild<QToolButton*>("command.commands.search");QVERIFY(button);
  ReferenceView* view=nullptr;for(auto* widget:w.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(widget))view=v;QVERIFY(view);
  const auto before=call(w,"document.inspect");const auto selection=w.state().value("selection");
  // Harness setup ends here; subsequent focus is observed, never repaired.
  view->setFocus(Qt::OtherFocusReason);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
  QPointer<QWidget> lastEditorFocus=view;
  const auto focusObservation=connect(qApp,&QApplication::focusChanged,&w,[&](QWidget*,QWidget* now){if(now && now->window()==&w)lastEditorFocus=now;});
  click(button,button->rect().center());if(QTest::currentTestFailed())return;
  QPointer<QWidget> buttonOrigin=lastEditorFocus;
  disconnect(focusObservation);
  auto* dialog=w.findChild<QDialog*>("commands.search.dialog");auto* input=w.findChild<QLineEdit*>("commands.search.query");auto* list=w.findChild<QListWidget*>("commands.search.results");QVERIFY(dialog);QVERIFY(input);QVERIFY(list);
  auto session=[&]{return w.state().value("activeSession").toObject();};
  auto query=[&]{return session().value("inputs").toObject().value("query").toString();};
  auto ready=[&](const QString& name){observe(w,*dialog,name);};
  QTRY_VERIFY(native(*dialog).value("expectedKeyWindow").toBool(true));QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(input));QCOMPARE(input->text(),QString());QCOMPARE(query(),input->text());ready("button-open");
  QTest::keyClicks(QApplication::focusWidget(),"zz-no-results-s");QCOMPARE(query(),QString("zz-no-results-s"));QCOMPARE(list->count(),0);ready("no-results");
  QTest::keyClick(QApplication::focusWidget(),Qt::Key_Escape);QTRY_VERIFY(!dialog->isVisible());observe(w,w,"first-escape");QTRY_VERIFY(native(w).value("expectedKeyWindow").toBool(true));QVERIFY(QApplication::focusWidget());QVERIFY(QApplication::focusWidget()->window()==&w);QCOMPARE(QApplication::focusWidget(),buttonOrigin.data());QVERIFY(buttonOrigin->isVisible() && buttonOrigin->isEnabled());QVERIFY(session().isEmpty());QCOMPARE(call(w,"document.inspect"),before);
  QTest::keyClick(QApplication::focusWidget(),Qt::Key_S);QTRY_VERIFY(dialog->isVisible());QTRY_VERIFY(native(*dialog).value("expectedKeyWindow").toBool(true));QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(input));QCOMPARE(input->text(),QString());ready("shortcut-reopen");
  const auto id=session().value("id");QTest::keyClicks(QApplication::focusWidget(),"s");QCOMPARE(query(),QString("s"));QCOMPARE(session().value("id"),id);QVERIFY(list->count()>0);
  call(w,"commands.search");QCOMPARE(session().value("id"),id);QCOMPARE(input->text(),QString("s"));QCOMPARE(query(),input->text());
  QTRY_VERIFY(native(*dialog).value("expectedKeyWindow").toBool(true));QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(input));
  QTest::keySequence(QApplication::focusWidget(),QKeySequence::SelectAll);QTest::keyClicks(QApplication::focusWidget(),"view.home");QCOMPARE(query(),QString("view.home"));
  int row=-1;for(int i=0;i<list->count();++i)if(list->item(i)->data(Qt::UserRole)=="view.home")row=i;QVERIFY(row>=0);
  call(w,"capture.dialog",{{"path",output+"/search.png"}});ready("result-ready");
  const auto activationRevision=w.state().value("revision").toInteger();
  const auto point=list->visualItemRect(list->item(row)).center();click(list->viewport(),point);if(QTest::currentTestFailed())return;
  QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(list));QCOMPARE(list->currentItem()->data(Qt::UserRole).toString(),QString("view.home"));QVERIFY(native(*dialog).value("expectedKeyWindow").toBool(true));QTest::keyClick(QApplication::focusWidget(),Qt::Key_Return);
  QTRY_VERIFY(!dialog->isVisible());observe(w,w,"result-activated");QTRY_VERIFY(native(w).value("expectedKeyWindow").toBool(true));QCOMPARE(w.state().value("revision").toInteger(),activationRevision+2);QVERIFY(session().isEmpty());QVERIFY(QApplication::focusWidget());QVERIFY(QApplication::focusWidget()->window()==&w);
  QPointer<QWidget> shortcutOrigin=QApplication::focusWidget();
  QTest::keyClick(QApplication::focusWidget(),Qt::Key_S);QTRY_VERIFY(dialog->isVisible());QTRY_VERIFY(native(*dialog).value("expectedKeyWindow").toBool(true));QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(input));QCOMPARE(input->text(),QString());
  QTest::keyClick(QApplication::focusWidget(),Qt::Key_Escape);QTRY_VERIFY(!dialog->isVisible());QTRY_VERIFY(native(w).value("expectedKeyWindow").toBool(true));observe(w,w,"final-escape");QCOMPARE(QApplication::focusWidget(),shortcutOrigin.data());QVERIFY(shortcutOrigin->isVisible() && shortcutOrigin->isEnabled());
  auto opened=call(w,"commands.search");const auto apiId=opened.value("id");call(w,"command.update",{{"session",apiId},{"inputs",QJsonObject{{"query","view.home"}}}});QCOMPARE(query(),input->text());call(w,"commands.search");QCOMPARE(session().value("id"),apiId);QCOMPARE(input->text(),QString("view.home"));call(w,"commands.close");QTRY_VERIFY(!dialog->isVisible());QTRY_VERIFY(native(w).value("expectedKeyWindow").toBool(true));
  QCOMPARE(call(w,"document.inspect"),before);QCOMPARE(w.state().value("selection"),selection);call(w,"capture.screenshot",{{"path",output+"/restored.png"}});observe(w,w,"api-closed");
 }
};
QTEST_MAIN(CommandSearchControls)
#include "command_search_controls.moc"
