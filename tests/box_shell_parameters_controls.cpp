// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include <QApplication>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QJsonDocument>
#include <QLineEdit>
#include <QPushButton>
#include <QTest>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QLabel>

class BoxShellParametersControlsTest final:public QObject{
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& args={}){
        const auto result=window.execute(command,args);
        if(!result["ok"].toBool())qFatal("%s: %s",qPrintable(command),QJsonDocument(result).toJson().constData());
        return result["result"].toObject();
    }
private slots:
    void closedBodySelection(){
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1300,850);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));window.activateWindow();
        call(window,"solid.box",{{"width",50},{"depth",80},{"height",40},{"units","mm"}});
        const auto source=call(window,"document.inspect");
        const auto body=source["evaluation"].toObject()["bodies"].toArray()[0].toObject();
        const auto face=body["supportFaces"].toArray()[0].toObject();
        QJsonObject support;for(const auto& key:{"producerFeatureId","bodyId","faceId"})support[key]=face[key];
        call(window,"selection.faceSet",{{"support",support}});
        auto session=call(window,"solid.shellDialog",{{"thicknessMm",3.5}});
        auto* tree=window.findChild<QTreeWidget*>("browser.tree");QVERIFY(tree);QTreeWidgetItem* item=nullptr;
        for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole+1).toString()==body["bodyId"].toString()){item=*it;break;}
        QVERIFY(item);for(auto* parent=item->parent();parent;parent=parent->parent())parent->setExpanded(true);
        tree->scrollToItem(item);QApplication::processEvents();const auto rect=tree->visualItemRect(item);
        QVERIFY(rect.isValid() && tree->itemAt(rect.center())==item);
        QTest::mouseClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,rect.center());
        auto active=window.state()["activeSession"].toObject();
        QVERIFY(active["inputs"].toObject()["removedFace"].toObject().isEmpty());
        QCOMPARE(active["geometry"].toObject()["topology"].toObject()["shells"].toInt(),2);
        auto* dock=window.findChild<QDockWidget*>("shellDock");QVERIFY(dock);
        QVERIFY(dock->findChild<QLabel*>("shellSelection")->text().contains("closed cavity"));
        auto* buttons=dock->findChild<QDialogButtonBox*>();QVERIFY(buttons->button(QDialogButtonBox::Ok)->isEnabled());
        const auto output=qEnvironmentVariable("ALLOY_CLOSED_SHELL_EVIDENCE");
        if(!output.isEmpty())call(window,"capture.screenshot",{{"path",output+"/closed-shell-native.png"}});
        QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel),Qt::LeftButton);
        QCOMPARE(call(window,"document.inspect")["authored"],source["authored"]);
        session=call(window,"solid.shellDialog",{{"thicknessMm",3.5}});
        QVERIFY(window.state()["activeSession"].toObject()["inputs"].toObject()["removedFace"].toObject().isEmpty());
        dock=window.findChild<QDockWidget*>("shellDock");buttons=dock->findChild<QDialogButtonBox*>();
        QTest::mouseClick(buttons->button(QDialogButtonBox::Ok),Qt::LeftButton);
        QCOMPARE(call(window,"document.inspect")["evaluation"].toObject()["bodies"].toArray()[0].toObject()["topology"].toObject()["shells"].toInt(),2);
        call(window,"document.new",{{"discard",true}});window.close();
    }
    void directNamesInExistingFields(){
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1300,850);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));QApplication::setActiveWindow(&window);
        for(const auto& pair:QVector<QPair<QString,double>>{{"Width",50},{"Length",80},{"Height",40},{"Thickness",4}})
            call(window,"document.parameterAdd",{{"name",pair.first},{"value",pair.second},{"units","mm"}});
        auto session=call(window,"command.begin",{{"command","solid.box"},{"inputs",QJsonObject{}}});
        auto* dock=window.findChild<QDockWidget*>("primitiveDock");QVERIFY(dock);
        auto* unknown=dock->findChild<QDoubleSpinBox*>("primitive.width");QVERIFY(unknown);
        QTest::mouseClick(unknown,Qt::LeftButton);QTest::keyClick(unknown,Qt::Key_A,Qt::ControlModifier);
        QTest::keyClicks(unknown,"MissingWidth");QTest::keyClick(unknown,Qt::Key_Tab);
        QVERIFY(window.state()["activeSession"].toObject().contains("invalidNamedInput"));
        const auto refused=window.execute("command.commit",{{"session",session["id"]}});
        QVERIFY(!refused["ok"].toBool());
        QCOMPARE(call(window,"document.inspect")["authored"].toObject()["features"].toArray().size(),0);
        for(const auto& pair:QVector<QPair<QString,QString>>{{"width","Width"},{"depth","Length"},{"height","Height"}}){
            auto* field=dock->findChild<QDoubleSpinBox*>("primitive."+pair.first);QVERIFY(field);
            auto* editor=field->findChild<QLineEdit*>();QVERIFY(editor);
            QTest::mouseClick(field,Qt::LeftButton);
            QTest::keyClick(field,Qt::Key_A,Qt::ControlModifier);
            QTest::keyClicks(field,pair.second);QTest::keyClick(field,Qt::Key_Tab);
            QCOMPARE(window.state()["activeSession"].toObject()["inputs"].toObject()[pair.first+"Parameter"].toString(),pair.second);
            QVERIFY(editor->text().contains(pair.second));
        }
        const auto evidence=qEnvironmentVariable("ALLOY_BOX_SHELL_EVIDENCE");
        if(!evidence.isEmpty())call(window,"capture.screenshot",{{"path",evidence+"/box-name-fields.png"}});
        call(window,"command.preview",{{"session",session["id"]}});
        call(window,"command.commit",{{"session",session["id"]}});
        auto inspect=call(window,"document.inspect");const auto box=inspect["authored"].toObject()["features"].toArray().last().toObject();
        QVERIFY(box["parameters"].toObject().contains("widthParameterId"));
        QCOMPARE(inspect["evaluation"].toObject()["bodies"].toArray()[0].toObject()["volumeMm3"].toDouble(),160000.);
        if(!evidence.isEmpty())call(window,"capture.screenshot",{{"path",evidence+"/bound-box-native.png"}});
        session=call(window,"feature.editDialog",{{"featureId",box["id"]}});
        dock=window.findChild<QDockWidget*>("primitiveDock");auto* width=dock->findChild<QDoubleSpinBox*>("primitive.width");QVERIFY(width);
        QVERIFY(width->findChild<QLineEdit*>()->text().contains("Width"));
        QTest::mouseClick(width,Qt::LeftButton);QTest::keyClick(width,Qt::Key_A,Qt::ControlModifier);
        QTest::keyClicks(width,"50");QTest::keyClick(width,Qt::Key_Tab);
        QVERIFY(!window.state()["activeSession"].toObject()["inputs"].toObject().contains("widthParameter"));
        call(window,"command.commit",{{"session",session["id"]}});
        QVERIFY(!call(window,"document.inspect")["authored"].toObject()["features"].toArray().last().toObject()["parameters"].toObject().contains("widthParameterId"));
        call(window,"document.new",{{"discard",true}});
        call(window,"document.parameterAdd",{{"name","Thickness"},{"value",4},{"units","mm"}});
        call(window,"sketch.begin",{{"plane","XY"}});
        call(window,"sketch.rectangle",{{"x1",0},{"y1",0},{"x2",25},{"y2",25},{"units","mm"}});
        const auto sketch=window.state()["activeSession"].toObject()["draftSketch"].toObject();call(window,"sketch.finish");
        const auto profile=call(window,"document.inspect")["evaluation"].toObject()["sketches"].toArray()[0].toObject()["profiles"].toArray()[0].toObject()["profileId"];
        call(window,"solid.extrude",{{"sketchId",sketch["id"]},{"profileId",profile},{"distance",12.5},{"units","mm"}});
        const auto body=call(window,"document.inspect")["evaluation"].toObject()["bodies"].toArray()[0].toObject();
        QJsonObject top;for(const auto& entry:body["supportFaces"].toArray())if(entry.toObject()["role"]=="extrude.endCap"){const auto f=entry.toObject();top={{"producerFeatureId",f["producerFeatureId"]},{"bodyId",f["bodyId"]},{"faceId",f["faceId"]}};break;}
        QVERIFY(!top.isEmpty());
        session=call(window,"solid.shellDialog",{{"targetBodyId",body["bodyId"]},{"removedFace",top},{"thicknessMm",2}});
        auto* shellDock=window.findChild<QDockWidget*>("shellDock");QVERIFY(shellDock);
        auto* thickness=shellDock->findChild<QDoubleSpinBox*>("shellThickness");QVERIFY(thickness);
        QTest::mouseClick(thickness,Qt::LeftButton);QTest::keyClick(thickness,Qt::Key_A,Qt::ControlModifier);
        QTest::keyClicks(thickness,"MissingWall");QTest::keyClick(thickness,Qt::Key_Tab);
        QVERIFY(window.state()["activeSession"].toObject().contains("invalidNamedInput"));
        QVERIFY(!shellDock->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->isEnabled());
        QVERIFY(!window.execute("command.commit",{{"session",session["id"]}})["ok"].toBool());
        QCOMPARE(call(window,"document.inspect")["authored"].toObject()["features"].toArray().size(),2);
        QTest::mouseClick(thickness,Qt::LeftButton);QTest::keyClick(thickness,Qt::Key_A,Qt::ControlModifier);
        QTest::keyClicks(thickness,"Thickness");QTest::keyClick(thickness,Qt::Key_Tab);
        QCOMPARE(window.state()["activeSession"].toObject()["inputs"].toObject()["thicknessParameter"].toString(),QString("Thickness"));
        QVERIFY(thickness->findChild<QLineEdit*>()->text().contains("Thickness"));
        if(!evidence.isEmpty())call(window,"capture.screenshot",{{"path",evidence+"/shell-name-field.png"}});
        call(window,"command.commit",{{"session",session["id"]}});
        const auto shell=call(window,"document.inspect")["authored"].toObject()["features"].toArray().last().toObject();
        QVERIFY(shell["parameters"].toObject().contains("thicknessParameterId"));
        if(!evidence.isEmpty())call(window,"capture.screenshot",{{"path",evidence+"/bound-shell-native.png"}});
        session=call(window,"feature.editDialog",{{"featureId",shell["id"]}});
        shellDock=window.findChild<QDockWidget*>("shellDock");thickness=shellDock->findChild<QDoubleSpinBox*>("shellThickness");QVERIFY(thickness);
        QVERIFY(thickness->findChild<QLineEdit*>()->text().contains("Thickness"));
        QTest::mouseClick(thickness,Qt::LeftButton);QTest::keyClick(thickness,Qt::Key_A,Qt::ControlModifier);
        QTest::keyClicks(thickness,"4");QTest::keyClick(thickness,Qt::Key_Tab);
        QVERIFY(!window.state()["activeSession"].toObject()["inputs"].toObject().contains("thicknessParameter"));
        call(window,"command.commit",{{"session",session["id"]}});
        QVERIFY(!call(window,"document.inspect")["authored"].toObject()["features"].toArray().last().toObject()["parameters"].toObject().contains("thicknessParameterId"));
        call(window,"document.new",{{"discard",true}});window.close();
    }
};
QTEST_MAIN(BoxShellParametersControlsTest)
#include "box_shell_parameters_controls.moc"
