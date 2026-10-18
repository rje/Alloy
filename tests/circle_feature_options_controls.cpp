// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QAbstractItemView>
#include <QComboBox>
#include <QDir>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QTemporaryDir>
#include <QTest>
class CircleFeatureOptionsControlsTest final:public QObject {
 Q_OBJECT
 QJsonObject call(AlloyWindow& w,const QString& command,const QJsonObject& arguments={}){const auto r=w.execute(command,arguments);if(!r["ok"].toBool())qFatal("%s",QJsonDocument(r).toJson().constData());return r["result"].toObject();}
 QJsonObject session(AlloyWindow& w){return w.state()["activeSession"].toObject();}
 QJsonObject draft(AlloyWindow& w){return session(w)["draftSketch"].toObject();}
 int circles(AlloyWindow& w){int n=0;for(const auto& row:draft(w)["parameters"].toObject()["entities"].toArray())n+=row.toObject()["type"]=="circle";return n;}
private slots:
 void nativeModeSwitchResetsPendingInputsAndPreservesGeometry(){
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1500,950);w.show();w.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&w));QApplication::setActiveWindow(&w);
  ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;QVERIFY(view);view->setFocus();
  call(w,"sketch.begin",{{"plane","XY"}});call(w,"sketch.rectangle",{{"x1",0},{"y1",0},{"x2",30},{"y2",30},{"units","mm"}});QJsonArray lines;for(const auto& row:draft(w)["parameters"].toObject()["entities"].toArray())lines.append(row.toObject()["id"]);call(w,"sketch.construction",{{"entityIds",lines},{"construction",true}});
  auto* mode=w.findChild<QComboBox*>("sketch.circle.mode");QVERIFY(mode);QCOMPARE(mode->count(),5);auto* options=w.findChild<QWidget*>("sketch.circle.featureOptions");QVERIFY(options);QVERIFY(!options->isVisible());
  call(w,"sketch.tool",{{"tool","circle"}});QVERIFY(options->isVisible());QCOMPARE(mode->currentData().toString(),QString("circle"));
  auto pixel=[&](double u,double v){const auto p=call(w,"view.projectSketchPoint",{{"sketchId",draft(w)["id"]},{"uMm",u},{"vMm",v}});return QPoint(qRound(p["x"].toDouble()),qRound(p["y"].toDouble()));};
  auto click=[&](double u,double v){QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,pixel(u,v));};
  auto move=[&](double u,double v){const auto p=pixel(u,v);QMouseEvent e(QEvent::MouseMove,QPointF(p),QPointF(view->mapToGlobal(p)),Qt::NoButton,Qt::NoButton,Qt::NoModifier);QApplication::sendEvent(view,&e);};
  const auto output=qEnvironmentVariable("ALLOY_CIRCLE_FEATURE_OPTIONS_EVIDENCE");if(!output.isEmpty())QDir().mkpath(output);auto capture=[&](const QString& name){if(!output.isEmpty()){QTest::qWait(100);call(w,"capture.screenshot",{{"path",output+"/"+name+".png"}});}};
  auto switchMode=[&](const QString& tool,bool keyboard){
   const auto before=draft(w);const int index=mode->findData(tool);QVERIFY(index>=0);QTest::mouseClick(mode,Qt::LeftButton);QTRY_VERIFY(mode->view()->isVisible());QVERIFY(QApplication::focusWidget()==mode->view() || mode->view()->isAncestorOf(QApplication::focusWidget()));
   if(keyboard){auto* focus=QApplication::focusWidget();QTest::keyClick(focus,Qt::Key_Home);for(int i=0;i<index;++i)QTest::keyClick(focus,Qt::Key_Down);QTest::keyClick(focus,Qt::Key_Return);}
   else {const auto row=mode->model()->index(index,0);auto* viewport=mode->view()->viewport();const auto point=mode->view()->visualRect(row).center();QTest::mouseMove(viewport,point);QTest::qWait(QApplication::doubleClickInterval());QVERIFY(QApplication::widgetAt(viewport->mapToGlobal(point))==viewport);QTest::mouseClick(viewport,Qt::LeftButton,Qt::NoModifier,point);}
   QTRY_VERIFY(QApplication::activePopupWidget()==nullptr);QTRY_COMPARE(session(w)["tool"].toString(),tool);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QCOMPARE(mode->currentData().toString(),tool);QCOMPARE(draft(w),before);
   const QMap<QString,QString> prompts{{"circle","center"},{"twoPointCircle","diameter endpoint"},{"threePointCircle","first circumference point"},{"twoTangentCircle","Select two straight"},{"threeTangentCircle","Select three straight"}};auto* info=w.findChild<QLabel*>("sketch.info");QVERIFY(info);QVERIFY2(info->text().contains(prompts.value(tool),Qt::CaseInsensitive),qPrintable(info->text()));
   for(const auto& key:{"firstPoint","secondPoint","pointerPoint","pendingCircleDiameter","tangentCircleLineIds","tangentCirclePointer","tangentCirclePreview","threeTangentCircleLineIds","threeTangentCirclePreview","threeTangentCircleHoverLineId"})QVERIFY2(!session(w).contains(key),key);
   QVERIFY(session(w)["selection"].toObject()["entityIds"].toArray().isEmpty());QVERIFY(!w.state()["inlineEditor"].toObject()["visible"].toBool());
  };
  const auto original=draft(w);click(-35,15);call(w,"sketch.circleDiameterPreview",{{"text","10"}});QVERIFY(session(w).contains("pendingCircleDiameter"));capture("01-center-diameter-options");switchMode("twoPointCircle",false);
  click(-40,10);QVERIFY(session(w).contains("firstPoint"));move(-30,10);call(w,"sketch.circleDiameterPreview",{{"text","8"}});switchMode("threePointCircle",true);
  click(-40,10);click(-30,20);move(-20,10);QVERIFY(session(w).contains("secondPoint"));capture("02-three-point-options-preview");switchMode("twoTangentCircle",false);
  click(10,0);click(30,20);move(20,10);QVERIFY(session(w)["tangentCirclePreview"].toBool());QTest::keyClick(view,Qt::Key_8);auto* radius=w.findChild<QLineEdit*>("sketch.twoTangentCircle.radius");QVERIFY(radius && radius->isVisible());QCOMPARE(QApplication::focusWidget(),static_cast<QWidget*>(radius));switchMode("threeTangentCircle",true);QVERIFY(!radius->isVisible());
  click(10,0);click(30,20);move(10,30);QVERIFY(session(w)["threeTangentCirclePreview"].toBool());capture("03-three-tangent-options-preview");switchMode("circle",false);QCOMPARE(draft(w),original);
  click(-35,15);QVERIFY(session(w).contains("firstPoint"));move(-25,15);click(-25,15);QCOMPARE(circles(w),1);const auto committed=draft(w);capture("04-committed-circle-after-switch");
  switchMode("threePointCircle",true);QCOMPARE(circles(w),1);click(-40,-20);QTest::keyClick(view,Qt::Key_Escape);QCOMPARE(session(w)["tool"].toString(),QString("select"));QCOMPARE(draft(w),committed);QVERIFY(!options->isVisible());
  call(w,"sketch.tool",{{"tool","twoPointCircle"}});QVERIFY(options->isVisible());QTest::keyClick(view,Qt::Key_Return);QCOMPARE(session(w)["tool"].toString(),QString("select"));QVERIFY(!options->isVisible());
  call(w,"history.undo");QCOMPARE(circles(w),0);call(w,"history.redo");QCOMPARE(draft(w),committed);
  call(w,"sketch.finish");const auto authored=call(w,"document.inspect")["authored"].toObject();QTemporaryDir folder;const auto path=folder.path()+"/circle-options.alloy";call(w,"document.save",{{"path",path}});call(w,"document.open",{{"path",path}});QCOMPARE(call(w,"document.inspect")["authored"].toObject(),authored);w.close();
 }
};
QTEST_MAIN(CircleFeatureOptionsControlsTest)
#include "circle_feature_options_controls.moc"
