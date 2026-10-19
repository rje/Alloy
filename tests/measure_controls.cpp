// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QDialogButtonBox>
#include <QPushButton>
#include <QDir>
#include <QApplication>
#include <QDialog>
#include <QJsonDocument>
#include <QLabel>
#include <QTest>
#include <QToolButton>
class MeasureControls final:public QObject{
 Q_OBJECT
 QJsonObject call(AlloyWindow& w,const QString& id,const QJsonObject& args={}){const auto r=w.execute(id,args);if(!r.value("ok").toBool())qFatal("%s",QJsonDocument(r).toJson().constData());return r.value("result").toObject();}
private slots:
 void actualToolbar(){AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1400,900);w.show();QVERIFY(QTest::qWaitForWindowExposed(&w));w.raise();w.activateWindow();QTRY_VERIFY(w.isActiveWindow());
  call(w,"solid.box",{{"width",20},{"depth",10},{"height",5},{"units","mm"}});call(w,"view.orientation",{{"name","top"}});call(w,"view.fit");const auto before=call(w,"document.inspect");
  const auto body=before.value("evaluation").toObject().value("bodies").toArray().first().toObject();QJsonObject edge;
  for(const auto& value:body.value("solidEdges").toArray()){const auto candidate=value.toObject();const auto provenance=candidate.value("provenance").toObject();const auto a=provenance.value("startMm").toArray(),b=provenance.value("endMm").toArray();if(a.size()==3 && b.size()==3 && a[2].toDouble()==5 && b[2].toDouble()==5 && a[1].toDouble()==0 && b[1].toDouble()==0){edge=candidate;break;}}
  QVERIFY(!edge.isEmpty());
  QToolButton* button=nullptr;for(auto* candidate:w.findChildren<QToolButton*>("command.inspect.measure"))if(candidate->isVisible()){button=candidate;break;}
  QVERIFY(button);QCOMPARE(QApplication::widgetAt(button->mapToGlobal(button->rect().center())),static_cast<QWidget*>(button));
  QTest::mouseClick(button,Qt::LeftButton);QPointer<QDialog> dialog=w.findChild<QDialog*>("measure.dialog");QVERIFY(dialog);QVERIFY(dialog->isVisible());QTRY_VERIFY(dialog->isActiveWindow() || w.isActiveWindow());
  QVERIFY(dialog->findChild<QLabel*>("measure.Results")->text().contains("Select"));QCOMPARE(call(w,"document.inspect"),before);
  ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(child)){view=candidate;break;}QVERIFY(view);
  const auto pixel=call(w,"view.projectEdge",{{"bodyId",body.value("bodyId")},{"edgeId",edge.value("edgeId")}});
  const QPoint point(qRound(pixel.value("x").toDouble()),qRound(pixel.value("y").toDouble()));
  QCOMPARE(QApplication::widgetAt(view->mapToGlobal(point)),static_cast<QWidget*>(view));QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,point);
  QTRY_VERIFY(dialog->findChild<QLabel*>("measure.Results")->text().contains("20.0000 mm"));
  QCOMPARE(call(w,"document.inspect"),before);
  const auto evidence=qEnvironmentVariable("ALLOY_MEASURE_EVIDENCE");if(!evidence.isEmpty()){QVERIFY(QDir().mkpath(evidence));QVERIFY(dialog->grab().save(evidence+"/measure-native.png"));}
  auto* close=dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Close);QVERIFY(close);
  QCOMPARE(QApplication::widgetAt(close->mapToGlobal(close->rect().center())),static_cast<QWidget*>(close));QTest::mouseClick(close,Qt::LeftButton);QTRY_VERIFY(!dialog || !dialog->isVisible());
 }
};
QTEST_MAIN(MeasureControls)
#include "measure_controls.moc"
