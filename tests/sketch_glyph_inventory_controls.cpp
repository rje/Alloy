// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QProcess>
#include <QTest>
#include <QWindow>

class SketchGlyphInventoryControls final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& w,const QString& command,const QJsonObject& args={}) {
        auto response=w.executeApi(command,args);const auto initial=response.value("result").toObject();
        for(int i=0;response.value("completion")=="pending" && i<400;++i){QTest::qWait(25);response=w.executeApi("command.status",{{"session",initial.value("id")},{"generation",initial.value("previewGeneration")}});}
        if(!response.value("ok").toBool() || response.value("completion")=="pending")qFatal("%s: %s",qPrintable(command),QJsonDocument(response).toJson().constData());
        return response.value("result").toObject();
    }
private slots:
    void mixedInventory() {
        AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1500,1000);w.show();QVERIFY(QTest::qWaitForWindowExposed(&w));
        ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(child))view=v;QVERIFY(view);
        auto session=[&]{return w.state().value("activeSession").toObject();};
        auto draft=[&]{return session().value("draftSketch").toObject();};
        auto params=[&]{return draft().value("parameters").toObject();};
        auto glyphs=[&]{return w.state().value("viewport").toObject().value("gpu").toObject().value("sketchConstraintGlyphs").toArray();};
        auto last=[&]{return params().value("entities").toArray().last().toObject();};
        auto line=[&](double x1,double y1,double x2,double y2){call(w,"sketch.line",{{"x1",x1},{"y1",y1},{"x2",x2},{"y2",y2},{"units","mm"}});return last();};
        auto point=[&](double x,double y){call(w,"sketch.addPoint",{{"x",x},{"y",y},{"units","mm"}});return last();};
        auto circle=[&](double x,double y,double diameter){call(w,"sketch.addCircle",{{"centerX",x},{"centerY",y},{"diameter",diameter},{"units","mm"}});return last();};
        QJsonArray inventory;
        auto relation=[&](QString kind,QJsonArray entities,QJsonArray points=QJsonArray{}){
            QJsonObject args{{"type",kind}};if(!entities.isEmpty())args.insert("entityIds",entities);if(!points.isEmpty())args.insert("pointIds",points);
            call(w,"sketch.constraint",args);const auto r=params().value("constraints").toArray().last().toObject();inventory.append(r);return r;
        };
        call(w,"sketch.begin",{{"plane","XY"},{"units","mm"},{"name","Mixed relation glyph inventory"}});
        const auto h=line(-110,60,-80,60);relation("horizontal",{h.value("id")});
        const auto v=line(-35,48,-35,73);relation("vertical",{v.value("id")});
        auto a=line(15,53,40,63),b=line(15,67,40,77);relation("parallel",{a.value("id"),b.value("id")});
        a=line(75,54,100,54);b=line(105,58,105,78);relation("perpendicular",{a.value("id"),b.value("id")});
        a=line(-110,-5,-85,5);b=line(-110,10,-85,20);relation("equal",{a.value("id"),b.value("id")});
        a=line(-50,-10,-10,-10);b=circle(-30,0,20);relation("tangent",{b.value("id"),a.value("id")});
        a=circle(30,0,24);b=circle(32,1,12);relation("concentric",{a.value("id"),b.value("id")});
        a=line(75,-8,105,-8);b=point(90,-8);relation("midpoint",{a.value("id")},{b.value("pointId")});
        a=line(-110,-65,-97,-60);b=line(-84,-55,-71,-50);relation("collinear",{a.value("id"),b.value("id")});
        a=line(-47,-70,-47,-50);b=line(-13,-50,-13,-70);const auto axis=line(-30,-77,-30,-43);
        call(w,"sketch.construction",{{"entityIds",QJsonArray{axis.value("id")}},{"construction",true}});relation("symmetry",{a.value("id"),b.value("id"),axis.value("id")});
        a=line(12,-65,48,-65);b=point(30,-65);relation("pointOnCurve",{a.value("id")},{b.value("pointId")});
        const auto fixedPoint=point(65,-65);const auto fixed=relation("fixed",{},{fixedPoint.value("pointId")});inventory.removeLast();
        const auto ppA=point(90,-65),ppB=point(90,-65);const auto pp=relation("coincident",{},{ppA.value("pointId"),ppB.value("pointId")});inventory.removeLast();
        // Two-point H/V uses the same family but a distinct point-target path.
        const auto hvA=point(60,32),hvB=point(78,32);relation("horizontal",{},{hvA.value("pointId"),hvB.value("pointId")});
        call(w,"sketch.tool",{{"tool","select"}});call(w,"sketch.selectionSet");call(w,"sketch.lookAt");call(w,"view.zoom",{{"factor",1.4}});QTest::qWait(100);
        QVERIFY(session().value("solverDiagnostics").toObject().value("redundantConstraintIds").toArray().isEmpty());
        const auto baseline=draft();const auto output=qEnvironmentVariable("ALLOY_GLYPH_INVENTORY_EVIDENCE");if(!output.isEmpty())QDir().mkpath(output);
        QJsonArray steps;
        auto record=[&](QString name){steps.append(QJsonObject{{"step",name},{"state",w.state()}});if(!output.isEmpty())call(w,"capture.screenshot",{{"path",output+"/"+name+".png"}});};
        auto badge=[&](QJsonValue id){for(const auto& value:glyphs())if(value.toObject().value("constraintId")==id)return value.toObject();return QJsonObject{};};
        auto inspect=[&]{
            for(const auto& value:inventory){const auto r=value.toObject(),g=badge(r.value("id"));QVERIFY(!g.isEmpty());QCOMPARE(g.value("kind"),r.value("type"));
                QCOMPARE(g.value("entityIds").toArray(),r.value("entityIds").toArray());QCOMPARE(g.value("pointIds").toArray(),r.value("pointIds").toArray());
                QVERIFY(g.value("viewportLocal").toBool());QCOMPARE(g.value("units").toString(),QString("logicalPixels"));const auto bounds=g.value("bounds").toObject();QVERIFY(bounds.value("width").toDouble()>0 && bounds.value("height").toDouble()>0);
            }
            // A visible Vertical mark must not disappear into its target line.
            const auto verticalBadge=badge(inventory.at(1).toObject().value("id"));
            const auto verticalBounds=verticalBadge.value("bounds").toObject();
            const auto verticalLine=view->projectSketchPoint(-35,60,"XY");
            QVERIFY(verticalBounds.value("x").toDouble()>verticalLine.value("x").toDouble()+3);
            QVERIFY(badge(fixed.value("id")).isEmpty());QVERIFY(badge(pp.value("id")).isEmpty());
        };
        inspect();record("mixed-unselected");
#ifdef Q_OS_MACOS
        QProcess activation;activation.start("osascript",{"-e",QString("tell application \"System Events\" to set frontmost of first application process whose unix id is %1 to true").arg(QCoreApplication::applicationPid())});QVERIFY(activation.waitForFinished(5000));QCOMPARE(activation.exitCode(),0);
#endif
        w.raise();w.activateWindow();w.windowHandle()->requestActivate();QApplication::setActiveWindow(&w);QTRY_VERIFY(w.isActiveWindow());
        // Initial activation only. Each badge click must provide its own focus.
        int index=0;
        for(const auto& value:inventory){const auto r=value.toObject(),g=badge(r.value("id"));const auto bounds=g.value("bounds").toObject();
            const QPoint at(qRound(bounds.value("x").toDouble()+bounds.value("width").toDouble()/2),qRound(bounds.value("y").toDouble()+bounds.value("height").toDouble()/2));
            QVERIFY(view->rect().contains(at));QVERIFY(!view->childAt(at));QCOMPARE(view->pickConstraint(at.x(),at.y()).value("constraintId"),r.value("id"));
            QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,at);QVERIFY(w.isActiveWindow());QCOMPARE(session().value("selection").toObject().value("constraintIds").toArray(),QJsonArray{r.value("id")});
            QVERIFY(badge(r.value("id")).value("selected").toBool());QCOMPARE(draft(),baseline);QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));
            record(QString("selected-%1-%2").arg(index++).arg(r.value("type").toString()));
            if(QStringList{"pointOnCurve","horizontal","tangent","symmetry"}.contains(r.value("type").toString())){
                QTest::keyClick(view,Qt::Key_Backspace);QVERIFY(badge(r.value("id")).isEmpty());const auto removed=draft();QVERIFY(removed!=baseline);
                const auto remaining=removed.value("parameters").toObject().value("constraints").toArray();for(const auto& item:remaining)QVERIFY(item.toObject().value("id")!=r.value("id"));
                call(w,"history.undo");QCOMPARE(draft(),baseline);
            }
        }
        // Fixed point is authored and removable by stable ID; native Fix/UnFix is SKT-133.
        call(w,"sketch.deleteSelection",{{"constraintIds",QJsonArray{fixed.value("id")}}});QVERIFY(draft()!=baseline);call(w,"history.undo");QCOMPARE(draft(),baseline);
        call(w,"sketch.selectionSet");inspect();const auto sketchId=draft().value("id");call(w,"sketch.finish");
        if(!output.isEmpty()){
            const auto authored=call(w,"document.inspect").value("authored");call(w,"document.save",{{"path",output+"/mixed-glyph-inventory.alloy"}});call(w,"document.open",{{"path",output+"/mixed-glyph-inventory.alloy"}});QCOMPARE(call(w,"document.inspect").value("authored"),authored);
            call(w,"sketch.edit",{{"sketchId",sketchId}});call(w,"sketch.selectionSet");QCOMPARE(draft(),baseline);inspect();QTest::qWait(150);record("reopened");
            QFile file(output+"/receipt.json");QVERIFY(file.open(QIODevice::WriteOnly));file.write(QJsonDocument(QJsonObject{{"result","passed"},{"steps",steps},{"inventory",inventory},{"fixedRelation",fixed},{"pointPointRelation",pp},{"baselineDraft",baseline}}).toJson());call(w,"sketch.cancel");
        }
        w.close();
    }
};
QTEST_MAIN(SketchGlyphInventoryControls)
#include "sketch_glyph_inventory_controls.moc"
