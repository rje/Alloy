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
#include <QScrollArea>
#include <QScrollBar>
#include <QTest>
#include <QToolButton>

class FeatureMirrorSnapControlsTest final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& name,const QJsonObject& args={}) {
        const auto response=window.execute(name,args);
        if(!response["ok"].toBool())qFatal("%s",QJsonDocument(response).toJson().constData());
        return response["result"].toObject();
    }
private slots:
    void selectFiveSourceFeatures() {
        const auto path=qEnvironmentVariable("ALLOY_SNAP_MIRROR_FIXTURE");
        const auto evidence=qEnvironmentVariable("ALLOY_SNAP_MIRROR_EVIDENCE");
        QVERIFY(!path.isEmpty() && !evidence.isEmpty());QDir().mkpath(evidence);
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1440,900);window.show();window.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        call(window,"document.open",{{"path",path}});
        const auto authored=call(window,"document.inspect")["authored"].toObject();
        const auto features=authored["features"].toArray();QVERIFY(features.size()>=5);
        QJsonArray ids;for(int i=features.size()-5;i<features.size();++i)ids.append(features[i].toObject()["id"]);
        auto* timelineScroll=window.findChild<QScrollArea*>("timeline.scroll");QVERIFY(timelineScroll);
        QTRY_VERIFY(timelineScroll->isVisible());
        // Arrange the five source tiles within the test viewport before starting the gesture.
        timelineScroll->horizontalScrollBar()->setValue(timelineScroll->horizontalScrollBar()->maximum());
        auto* launch=window.findChild<QToolButton*>("command.solid.featureMirrorDialog");QVERIFY(launch);
        QTest::mouseClick(launch,Qt::LeftButton);
        QTRY_COMPARE(window.state()["activeSession"].toObject()["command"].toString(),QString("solid.featureMirror"));
        ReferenceView* view=nullptr;for(auto* child:window.findChildren<QWidget*>())
            if(auto* found=dynamic_cast<ReferenceView*>(child))view=found;
        QVERIFY(view && view->hasFocus());
        for(const auto& id:ids){
            auto* timeline=window.findChild<QToolButton*>("timeline.feature."+id.toString());QVERIFY(timeline);
            QTRY_VERIFY(timeline->isVisible());
            const auto center=timeline->mapTo(timelineScroll->viewport(),timeline->rect().center());
            QVERIFY(timelineScroll->viewport()->rect().contains(center));
            QTest::mouseClick(timeline,Qt::LeftButton,Qt::NoModifier,QPoint(12,12));
        }
        auto* plane=window.findChild<QComboBox*>("featureMirror.plane");QVERIFY(plane);
        QTest::mouseClick(plane,Qt::LeftButton);QVERIFY(plane->view()->isVisible());
        auto* popupFocus=QApplication::focusWidget();QVERIFY(popupFocus);
        QTest::keyClick(popupFocus,Qt::Key_Home);QTest::keyClick(popupFocus,Qt::Key_Down);
        QTest::keyClick(popupFocus,Qt::Key_Return);
        QTRY_VERIFY(window.state()["activeSession"].toObject()["previewed"].toBool());
        QCOMPARE(window.state()["activeSession"].toObject()["inputs"].toObject()["sourceFeatureIds"].toArray(),ids);
        auto* dock=window.findChild<QDockWidget*>("featureMirrorDock");QVERIFY(dock);
        QVERIFY(dock->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->isEnabled());
        call(window,"capture.dialog",{{"path",evidence+"/native-snap-mirror-panel.png"}});
        call(window,"capture.screenshot",{{"path",evidence+"/native-snap-mirror-preview.png"}});
        QTest::mouseClick(dock->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Cancel),Qt::LeftButton);
        QVERIFY(window.state()["activeSession"].toObject().isEmpty());
        QCOMPARE(call(window,"document.inspect")["authored"].toObject(),authored);
        window.hide();
    }
};
QTEST_MAIN(FeatureMirrorSnapControlsTest)
#include "feature_mirror_snap_controls.moc"
