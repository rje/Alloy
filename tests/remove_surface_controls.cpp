// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include <QApplication>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QDir>
#include <QJsonDocument>
#include <QMenu>
#include <QPushButton>
#include <QTest>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>

class RemoveSurfaceControlsTest final:public QObject{
 Q_OBJECT
 QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& args={}){
  const auto response=window.execute(command,args);
  if(!response["ok"].toBool())qFatal("%s: %s",qPrintable(command),QJsonDocument(response).toJson().constData());
  return response["result"].toObject();
 }
 QTreeWidgetItem* row(QTreeWidget* tree,const QString& id){
  for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole+1).toString()==id && !(*it)->isHidden())return *it;
  return nullptr;
 }
private slots:
 void dialogCancelAndBrowserDirectRemove(){
  const auto source=qEnvironmentVariable("ALLOY_REMOVE_SOURCE"),evidence=qEnvironmentVariable("ALLOY_REMOVE_EVIDENCE");
  QVERIFY(!source.isEmpty() && !evidence.isEmpty());QDir().mkpath(evidence);
  AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1360,860);window.show();
  QVERIFY(QTest::qWaitForWindowExposed(&window));window.activateWindow();
  call(window,"document.open",{{"path",source},{"discard",true}});
  QApplication::setActiveWindow(&window);window.setFocus();
  const auto before=call(window,"document.inspect"),body=before["evaluation"].toObject()["bodies"].toArray()[0].toObject();
  const auto id=body["bodyId"].toString();auto* tree=window.findChild<QTreeWidget*>("browser.tree");QVERIFY(tree);tree->expandAll();
  const auto started=call(window,"body.removeDialog");QCOMPARE(started["command"].toString(),QString("body.remove"));
  auto* dock=window.findChild<QDockWidget*>("removeBodyDock");QVERIFY(dock && dock->isVisible());
  auto* chooser=dock->findChild<QComboBox*>("removeBody.selector");QVERIFY(chooser);
  QCOMPARE(chooser->currentData().toString(),QString{});
  QTest::mouseClick(chooser,Qt::LeftButton);QTest::keyClick(chooser,Qt::Key_Down);QTest::keyClick(chooser,Qt::Key_Return);
  QTRY_COMPARE(window.state()["activeSession"].toObject()["inputs"].toObject()["bodyId"].toString(),id);
  QTRY_VERIFY(window.state()["activeSession"].toObject()["previewed"].toBool());
  call(window,"capture.screenshot",{{"path",evidence+"/native-remove-preview.png"}});
  auto* buttons=dock->findChild<QDialogButtonBox*>();QVERIFY(buttons);
  QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel),Qt::LeftButton);
  QTRY_VERIFY(window.state()["activeSession"].toObject().isEmpty());
  QCOMPARE(call(window,"document.inspect")["authored"],before["authored"]);
  auto* item=row(tree,id);QVERIFY(item);
  tree->scrollToItem(item);const auto point=tree->visualItemRect(item).center();
  QTimer::singleShot(40,&window,[]{
   auto* menu=qobject_cast<QMenu*>(QApplication::activePopupWidget());if(!menu)qFatal("Browser context menu unavailable");
   for(auto* action:menu->actions())if(action->objectName()=="context.removeBody"){
    QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,menu->actionGeometry(action).center());return;
   }
   qFatal("Browser Remove action unavailable");
  });
  QContextMenuEvent event(QContextMenuEvent::Mouse,point,tree->viewport()->mapToGlobal(point));
  QApplication::sendEvent(tree->viewport(),&event);
  QTRY_VERIFY(call(window,"document.inspect")["evaluation"].toObject()["bodies"].toArray().isEmpty());
  QVERIFY(!row(tree,id));
  call(window,"capture.screenshot",{{"path",evidence+"/native-removed-browser.png"}});
 }
};
QTEST_MAIN(RemoveSurfaceControlsTest)
#include "remove_surface_controls.moc"
