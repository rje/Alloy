// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QAbstractItemView>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QPushButton>
#include <QTest>
#include <QToolButton>
#include <QJsonDocument>
#include <QDir>
class TangentPointPlaneControlsTest final:public QObject {
 Q_OBJECT
 QJsonObject call(AlloyWindow& w,const QString& name,const QJsonObject& args={}){const auto r=w.execute(name,args);if(!r["ok"].toBool())qFatal("%s",QJsonDocument(r).toJson().constData());return r["result"].toObject();}
 QJsonObject session(AlloyWindow& w){return w.state()["activeSession"].toObject();}
private slots:
 void viewportEscapeAndRetainedEndpoint(){
  const auto source=qEnvironmentVariable("ALLOY_TANGENT_SOURCE"),out=qEnvironmentVariable("ALLOY_TANGENT_EVIDENCE");QVERIFY(!source.isEmpty() && !out.isEmpty());QDir().mkpath(out);
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1440,900);w.show();w.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&w));
  ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(child))view=v;QVERIFY(view);
  call(w,"document.open",{{"path",source},{"discard",true}});call(w,"view.home");
  const auto initial=call(w,"document.inspect");
  auto* launch=w.findChild<QToolButton*>("command.construction.tangentPlane");QVERIFY(launch);
  QTest::mouseClick(launch,Qt::LeftButton);QTRY_COMPARE(session(w)["command"].toString(),QString("construction.tangentPlane"));
  QTRY_VERIFY(view->hasFocus());QVERIFY(w.state()["ui"].toObject()["activeDialog"].toObject()["visible"].toBool());
  // The command itself produced this focus. No activation/focus repair before Escape.
  QTest::keyClick(QApplication::focusWidget(),Qt::Key_Escape);QTRY_VERIFY(session(w).isEmpty());
  QCOMPARE(call(w,"document.inspect")["authored"],initial["authored"]);
  QTest::mouseClick(launch,Qt::LeftButton);QTRY_COMPARE(session(w)["command"].toString(),QString("construction.tangentPlane"));
  auto* dock=w.findChild<QDockWidget*>("tangentPlaneDock");QVERIFY(dock && dock->isVisible());
  auto* faces=dock->findChild<QComboBox*>("tangentPlane.face");QVERIFY(faces && faces->count()>1);
  QTest::mouseClick(faces,Qt::LeftButton);QTRY_VERIFY(faces->view()->isVisible());
  auto* popup=QApplication::focusWidget();QVERIFY(popup && (popup==faces->view() || faces->view()->isAncestorOf(popup)));
  QTest::keyClick(popup,Qt::Key_End);QTest::keyClick(popup,Qt::Key_Return);QTRY_VERIFY(QApplication::activePopupWidget()==nullptr);
  const auto chosen=session(w)["inputs"].toObject()["tangent"].toObject()["face"].toObject();QVERIFY(!chosen.isEmpty());
  const auto bodies=initial["evaluation"].toObject()["bodies"].toArray();QJsonObject body;
  for(const auto& b:bodies)if(b.toObject()["bodyId"]==chosen["bodyId"])body=b.toObject();QVERIFY(!body.isEmpty());
  QJsonObject edge;for(const auto& e:body["solidEdges"].toArray())if(e.toObject()["available"].toBool()){edge=e.toObject();break;}QVERIFY(!edge.isEmpty());
  const auto xyz=edge["startMm"].toArray();QVERIFY(xyz.size()==3);
  const auto pixel=view->projectSketchPoint(0,0,"XY",QJsonObject{{"origin",xyz},{"uAxis",QJsonArray{1,0,0}},{"vAxis",QJsonArray{0,1,0}},{"normal",QJsonArray{0,0,1}}});
  QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,QPoint(qRound(pixel["x"].toDouble()),qRound(pixel["y"].toDouble())));
  QTRY_VERIFY(session(w)["previewed"].toBool());
  dock=w.findChild<QDockWidget*>("tangentPlaneDock");auto* buttons=dock->findChild<QDialogButtonBox*>("tangentPlane.buttons");QVERIFY(buttons && buttons->button(QDialogButtonBox::Ok)->isEnabled());
  call(w,"capture.screenshot",{{"path",out+"/native-tangent-preview.png"}});call(w,"capture.dialog",{{"path",out+"/native-tangent-controls.png"}});
  QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel),Qt::LeftButton);QTRY_VERIFY(session(w).isEmpty());QCOMPARE(call(w,"document.inspect")["authored"],initial["authored"]);
 }
};
QTEST_MAIN(TangentPointPlaneControlsTest)
#include "tangent_point_plane_controls.moc"
