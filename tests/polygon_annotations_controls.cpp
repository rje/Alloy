#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QDir>
#include <QJsonDocument>
#include <QLineEdit>
#include <QTest>
#include <cmath>
class PolygonAnnotationsTest final:public QObject {
 Q_OBJECT
 QJsonObject call(AlloyWindow& w,const QString& command,const QJsonObject& args={}){
  const auto result=w.execute(command,args);if(!result["ok"].toBool())qFatal("%s",QJsonDocument(result).toJson().constData());return result["result"].toObject();
 }
 QJsonObject active(AlloyWindow& w){return w.state()["activeSession"].toObject();}
 QJsonObject parameters(AlloyWindow& w){return active(w)["draftSketch"].toObject()["parameters"].toObject();}
 QJsonObject layout(ReferenceView* view,const QJsonValue& polygonId){for(const auto& row:view->annotations())if(row.toObject()["polygonId"]==polygonId)return row.toObject();return {};}
private slots:
 void truthfulLabelsAndNativeEdit(){
  AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1500,950);w.show();w.activateWindow();QVERIFY(QTest::qWaitForWindowExposed(&w));QApplication::setActiveWindow(&w);
  ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;QVERIFY(view);view->setFocus();
  call(w,"sketch.begin",{{"plane","XY"}});call(w,"view.zoom",{{"factor",0.45}});
  const QStringList modes{"inscribed","circumscribed","edge"};
  for(int i=0;i<3;++i)call(w,"sketch.polygon",{{"mode",modes[i]},{"x",(i-1)*40},{"y",0},{"size",10},{"count",6},{"angleDeg",i==0 ? 135 : i==1 ? 90 : 0},{"units","mm"}});
  call(w,"sketch.tool",{{"tool","select"}});QTRY_COMPARE(view->annotations().size(),3);
  auto groups=parameters(w)["polygonGroups"].toArray();QVERIFY(parameters(w)["dimensions"].toArray().isEmpty());
  const auto evidence=qEnvironmentVariable("ALLOY_POLYGON_ANNOTATION_EVIDENCE");if(!evidence.isEmpty()){QVERIFY(QDir().mkpath(evidence));QApplication::processEvents();QTest::qWait(80);call(w,"capture.screenshot",{{"path",evidence+"/three-annotations.png"}});}
  for(int i=0;i<3;++i){
   const auto g=groups[i].toObject(),a=layout(view,g["id"]);QVERIFY(!a.isEmpty());QCOMPARE(a["dimensionId"],g["id"]);QCOMPARE(a["units"].toString(),QString("mm"));QVERIFY(a["label"].toObject()["text"].toString().endsWith(" mm"));
   QCOMPARE(a["sizeBasis"].toString(),i==0 ? QString("circumradius") : i==1 ? QString("inradius") : QString("edgeLength"));
   QCOMPARE(a["anchorKind"].toString(),i==2 ? QString("baseEndpoint") : QString("center"));
   const auto anchor=a["anchor"].toObject(),end=a["sizeEndpoint"].toObject();
   QVERIFY(std::abs(anchor["uMm"].toDouble()-(i-1)*40)<1e-7);QVERIFY(std::abs(anchor["vMm"].toDouble())<1e-7);
   QVERIFY(std::abs(std::hypot(end["uMm"].toDouble()-anchor["uMm"].toDouble(),end["vMm"].toDouble()-anchor["vMm"].toDouble())-10)<1e-7);
   if(i==1){QVERIFY(std::abs(end["uMm"].toDouble())<1e-7);QVERIFY(std::abs(end["vMm"].toDouble()-10)<1e-7);}
   const auto label=a["label"].toObject();const QPoint hit(qRound(label["x"].toDouble()),qRound(label["y"].toDouble()));
   QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,hit);QCOMPARE(active(w)["selection"].toObject()["dimensionId"],g["id"]);
   QTest::mouseDClick(view,Qt::LeftButton,Qt::NoModifier,hit);
   auto* editor=w.findChild<QLineEdit*>("sketch.inlineValue");QVERIFY(editor && editor->isVisible());QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(editor));QCOMPARE(active(w)["pendingDimension"].toObject()["polygonId"],g["id"]);
   if(i==0){QTest::keyClicks(editor,"0 mm");QTest::keyClick(editor,Qt::Key_Return);QVERIFY(active(w).contains("pendingDimension"));QCOMPARE(parameters(w)["polygonGroups"].toArray()[i].toObject()["sizeMm"].toDouble(),10.0);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(editor));QTest::keyClick(editor,Qt::Key_A,Qt::ControlModifier);}
   QTest::keyClicks(editor,QString::number(12+i)+" mm");QTest::keyClick(editor,Qt::Key_Return);QVERIFY(!active(w).contains("pendingDimension"));QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
   QCOMPARE(parameters(w)["polygonGroups"].toArray()[i].toObject()["sizeMm"].toDouble(),double(12+i));QVERIFY(parameters(w)["dimensions"].toArray().isEmpty());QCOMPARE(active(w)["profileSummaries"].toArray().size(),3);
  }
  const auto changed=active(w)["draftSketch"].toObject();call(w,"history.undo");QCOMPARE(parameters(w)["polygonGroups"].toArray()[2].toObject()["sizeMm"].toDouble(),10.0);call(w,"history.redo");QCOMPARE(active(w)["draftSketch"].toObject(),changed);
  call(w,"sketch.displayOptions",{{"showDimensions",false}});QVERIFY(view->annotations().isEmpty());call(w,"sketch.displayOptions",{{"showDimensions",true}});QCOMPARE(view->annotations().size(),3);
  call(w,"command.update",{{"session",active(w)["id"]},{"inputs",QJsonObject{{"units","in"}}}});for(const auto& row:view->annotations())QVERIFY(row.toObject()["label"].toObject()["text"].toString().endsWith(" in"));
  const auto inchLabel=layout(view,groups[0].toObject()["id"])["label"].toObject();QVERIFY(inchLabel["text"].toString().startsWith("R0.472441"));
  QTest::mouseDClick(view,Qt::LeftButton,Qt::NoModifier,QPoint(qRound(inchLabel["x"].toDouble()),qRound(inchLabel["y"].toDouble())));
  auto* inchEditor=w.findChild<QLineEdit*>("sketch.inlineValue");QVERIFY(inchEditor && inchEditor->isVisible());QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(inchEditor));QVERIFY(inchEditor->text().endsWith(" in"));QTest::keyClicks(inchEditor,"0.5 in");QTest::keyClick(inchEditor,Qt::Key_Return);QVERIFY(std::abs(parameters(w)["polygonGroups"].toArray()[0].toObject()["sizeMm"].toDouble()-12.7)<1e-8);
  call(w,"sketch.cancel");w.close();
 }
};
QTEST_MAIN(PolygonAnnotationsTest)
#include "polygon_annotations_controls.moc"
