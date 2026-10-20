// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include "rib_fixture.h"
#include <QTest>
#include <QApplication>
#include <QToolButton>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QAbstractItemView>
#include <QCheckBox>
#include <QMenu>
#include <QTimer>
#include <QTemporaryDir>
#include <QJsonDocument>
#include <QDir>
#include <QFile>
class RibControlsTest final:public QObject {
 Q_OBJECT
 QJsonObject call(AlloyWindow& w,const QString& c,const QJsonObject& a={}){const auto r=w.execute(c,a);if(!r["ok"].toBool())qFatal("%s",QJsonDocument(r).toJson().constData());return r["result"].toObject();}
 QJsonObject session(AlloyWindow& w){return w.state()["activeSession"].toObject();}
private slots:
 void nativeSelectorsPreviewCancelCommitEdit(){
  AlloyTheme::apply(*qApp);QTemporaryDir tmp;CadDocument fixture;RibFixture::make(fixture);QVERIFY(fixture.save(tmp.filePath("rib-source.alloy")).isEmpty());AlloyWindow w;w.resize(1440,1000);w.show();w.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&w));call(w,"document.open",{{"path",tmp.filePath("rib-source.alloy")}});QTRY_COMPARE(call(w,"document.inspect").value("authored").toObject().value("documentId"),fixture.authored().value("documentId"));call(w,"view.home");const auto before=call(w,"document.inspect");
  auto* create=w.findChild<QToolButton*>("solid.menu.create");QVERIFY(create && create->menu());auto* menu=create->menu();QAction* rib=nullptr;for(auto* a:menu->actions())if(a->objectName()=="solid.action.solid.ribDialog")rib=a;QVERIFY(rib);QTimer::singleShot(50,&w,[&]{if(!menu->isVisible() || QApplication::activePopupWidget()!=menu)qFatal("Create popup not active");const auto hit=menu->actionGeometry(rib).center();QTest::mouseMove(menu,hit);if(menu->actionAt(hit)!=rib)qFatal("Rib actual popup hit mismatch");QTest::mouseClick(menu,Qt::LeftButton,Qt::NoModifier,hit);});QTest::mouseClick(create,Qt::LeftButton);QTRY_COMPARE(session(w)["command"].toString(),QString("solid.rib"));QTRY_VERIFY(!menu->isVisible());QVERIFY(QApplication::activePopupWidget()==nullptr);
  auto* dock=w.findChild<QDockWidget*>("ribDock");QVERIFY(dock && dock->isVisible());auto* buttons=dock->findChild<QDialogButtonBox*>();QVERIFY(!buttons->button(QDialogButtonBox::Ok)->isEnabled());auto* profile=dock->findChild<QComboBox*>("rib.profile");auto* target=dock->findChild<QComboBox*>("rib.target");QVERIFY(profile && target && profile->count()==2 && target->count()==2);
  const auto choose=[&](QComboBox* box){QTest::mouseClick(box,Qt::LeftButton);QTRY_VERIFY(QApplication::activePopupWidget()==box->view()->window());const auto local=box->view()->visualRect(box->model()->index(1,0)).center();QTest::mouseMove(box->view()->viewport(),local);QTest::qWait(QApplication::doubleClickInterval());auto* hit=QApplication::widgetAt(box->view()->viewport()->mapToGlobal(local));QVERIFY(hit==box->view()->viewport());QTest::mouseClick(hit,Qt::LeftButton,Qt::NoModifier,hit->mapFromGlobal(box->view()->viewport()->mapToGlobal(local)));QTRY_VERIFY(QApplication::activePopupWidget()==nullptr);};
  choose(profile);QTRY_VERIFY(session(w)["inputs"].toObject().contains("sourceEntityId"));choose(target);QTRY_VERIFY(session(w)["previewed"].toBool());QVERIFY(std::abs(session(w)["geometry"].toObject()["volumeMm3"].toDouble()-7472)<1e-6);
  auto* thickness=dock->findChild<QLineEdit*>("rib.thicknessMm");QTest::mouseClick(thickness,Qt::LeftButton);QVERIFY(thickness->hasFocus());QTest::keySequence(thickness,QKeySequence::SelectAll);QTest::keyClicks(thickness,"3");QTest::keyClick(thickness,Qt::Key_Return);QTRY_VERIFY(session(w)["previewed"].toBool());QVERIFY(std::abs(session(w)["geometry"].toObject()["volumeMm3"].toDouble()-8408)<1e-6);
  const auto evidence=qEnvironmentVariable("ALLOY_RIB_CONTROLS_EVIDENCE");if(!evidence.isEmpty()){QDir().mkpath(evidence);call(w,"capture.screenshot",{{"path",evidence+"/rib-native-preview.png"}});QFile state(evidence+"/rib-native-state.json");QVERIFY(state.open(QIODevice::WriteOnly));state.write(QJsonDocument(w.state()).toJson(QJsonDocument::Indented));}
  QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel),Qt::LeftButton);QTRY_VERIFY(session(w).isEmpty());const auto cancelled=call(w,"document.inspect");QCOMPARE(cancelled["authored"],before["authored"]);QCOMPARE(cancelled["undo"],before["undo"]);
  const auto source=fixture.authored()["features"].toArray().last().toObject();const auto eid=source["parameters"].toObject()["entities"].toArray().first().toObject().value("id");const auto body=fixture.bodies().front().bodyId;
  call(w,"solid.ribDialog",{{"sketchId",source["id"]},{"sourceEntityId",eid},{"targetBodyId",body},{"thicknessMm",2.}});dock=w.findChild<QDockWidget*>("ribDock");buttons=dock->findChild<QDialogButtonBox*>();QVERIFY(buttons->button(QDialogButtonBox::Ok)->isEnabled());const auto feature=session(w).value("featureId");QTest::mouseClick(buttons->button(QDialogButtonBox::Ok),Qt::LeftButton);QTRY_VERIFY(session(w).isEmpty());qInfo()<<"Rib committed expected feature"<<feature<<"authored last"<<call(w,"document.inspect")["authored"].toObject()["features"].toArray().last();QToolButton* history=nullptr;QTRY_VERIFY((history=w.findChild<QToolButton*>("timeline.feature."+feature.toString())) && history->isVisible());QTest::mouseClick(history,Qt::LeftButton,Qt::NoModifier,QPoint(10,history->height()/2));QTRY_COMPARE(session(w)["command"].toString(),QString("solid.rib"));QVERIFY(session(w)["previewed"].toBool());dock=w.findChild<QDockWidget*>("ribDock");auto* flip=dock->findChild<QCheckBox*>("rib.flip");QTest::mouseClick(flip,Qt::LeftButton,Qt::NoModifier,QPoint(8,flip->height()/2));QTRY_VERIFY(!session(w)["previewed"].toBool());QVERIFY(!dock->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->isEnabled());QVERIFY(!session(w)["previewError"].toObject().isEmpty());QTest::keyClick(flip,Qt::Key_Escape);QTRY_VERIFY(session(w).isEmpty());call(w,"document.new",{{"discard",true}});w.close();
 }
};
QTEST_MAIN(RibControlsTest)
#include "rib_controls.moc"
