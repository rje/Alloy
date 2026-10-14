// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QAbstractItemView>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QDockWidget>
#include <QJsonDocument>
#include <QPushButton>
#include <QTest>
#include <QToolButton>

class FeatureMirrorCutControlsTest final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& name,const QJsonObject& args={}) {
        const auto response=window.execute(name,args);
        if(!response["ok"].toBool())qFatal("%s",QJsonDocument(response).toJson().constData());
        return response["result"].toObject();
    }
private slots:
    void timelineSelectionAndPlane() {
        const auto evidence=qEnvironmentVariable("ALLOY_FEATURE_MIRROR_EVIDENCE");
        QVERIFY(!evidence.isEmpty());QDir().mkpath(evidence);
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1440,900);window.show();window.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        call(window,"document.open",{{"path",evidence+"/mirror-seed.alloy"}});
        const auto initial=call(window,"document.inspect")["authored"].toObject();
        const auto features=initial["features"].toArray();const auto cut=features.last().toObject();
        QCOMPARE(cut["type"].toString(),QString("extrude"));
        QCOMPARE(cut["parameters"].toObject()["operation"].toString(),QString("cut"));
        call(window,"view.home");call(window,"view.orientation",{{"name","bottom"}});
        auto* launch=window.findChild<QToolButton*>("command.solid.featureMirrorDialog");QVERIFY(launch);
        auto* timeline=window.findChild<QToolButton*>("timeline.feature."+cut["id"].toString());QVERIFY(timeline);
        auto choose=[&]{
            QTest::mouseClick(launch,Qt::LeftButton);
            QTRY_COMPARE(window.state()["activeSession"].toObject()["command"].toString(),QString("solid.featureMirror"));
            ReferenceView* view=nullptr;for(auto* child:window.findChildren<QWidget*>())if(auto* found=dynamic_cast<ReferenceView*>(child))view=found;
            QVERIFY(view && view->hasFocus());
            QTest::mouseClick(timeline,Qt::LeftButton,Qt::NoModifier,QPoint(12,12));
            auto session=window.state()["activeSession"].toObject();
            QCOMPARE(session["inputs"].toObject()["sourceFeatureIds"].toArray(),QJsonArray{cut["id"]});
            const auto premature=window.execute("command.commit",{{"session",session["id"]}});
            QVERIFY(!premature["ok"].toBool());
            QCOMPARE(call(window,"document.inspect")["authored"].toObject(),initial);
            auto* plane=window.findChild<QComboBox*>("featureMirror.plane");QVERIFY(plane);
            QTest::mouseClick(plane,Qt::LeftButton);QVERIFY(plane->view()->isVisible());
            auto* popupFocus=QApplication::focusWidget();QVERIFY(popupFocus);
            QTest::keyClick(popupFocus,Qt::Key_End);QTest::keyClick(popupFocus,Qt::Key_Return);
            QTRY_VERIFY(window.state()["activeSession"].toObject()["previewed"].toBool());
            auto* dock=window.findChild<QDockWidget*>("featureMirrorDock");QVERIFY(dock);
            QVERIFY(dock->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->isEnabled());
        };
        choose();auto* dock=window.findChild<QDockWidget*>("featureMirrorDock");QVERIFY(dock);
        call(window,"capture.dialog",{{"path",evidence+"/native-mirror-panel.png"}});
        call(window,"capture.screenshot",{{"path",evidence+"/native-mirror-preview.png"}});
        QCOMPARE(call(window,"document.inspect")["authored"].toObject(),initial);
        QTest::mouseClick(dock->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Cancel),Qt::LeftButton);
        QVERIFY(window.state()["activeSession"].toObject().isEmpty());
        QCOMPARE(call(window,"document.inspect")["authored"].toObject(),initial);
        choose();dock=window.findChild<QDockWidget*>("featureMirrorDock");QVERIFY(dock);
        QTest::mouseClick(dock->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok),Qt::LeftButton);
        QVERIFY(window.state()["activeSession"].toObject().isEmpty());
        const auto accepted=call(window,"document.inspect")["authored"].toObject();
        QCOMPARE(accepted["features"].toArray().last().toObject()["type"].toString(),QString("featureMirror"));
        call(window,"capture.screenshot",{{"path",evidence+"/native-mirror-accepted.png"}});
        window.hide();
    }
};
QTEST_MAIN(FeatureMirrorCutControlsTest)
#include "feature_mirror_cut_controls.moc"
