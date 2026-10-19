// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include <QAbstractItemView>
#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QPushButton>
#include <QTest>
#include <QToolButton>
#include <QTreeWidget>

class DisplayUnitsControlsTest final : public QObject {
    Q_OBJECT
private slots:
    void browserUnitChange() {
        AlloyTheme::apply(*qApp);
        AlloyWindow window;
        window.resize(1250, 800);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto* tree = window.findChild<QTreeWidget*>("browser.tree");
        auto* button = window.findChild<QToolButton*>("browser.documentUnits.change");
        QVERIFY(tree && button);
        tree->topLevelItem(0)->setExpanded(true);
        tree->topLevelItem(0)->child(0)->setExpanded(true);
        tree->scrollToItem(tree->topLevelItem(0)->child(0)->child(0));
        QTRY_VERIFY(button->isVisible());
        QCOMPARE(QApplication::widgetAt(button->mapToGlobal(button->rect().center())), static_cast<QWidget*>(button));
        const auto before = window.execute("document.inspect").value("result").toObject();
        QTest::mouseClick(button, Qt::LeftButton);
        auto* units = window.findChild<QComboBox*>("document.units.input");
        QTRY_VERIFY(units && units->isVisible());
        QTRY_COMPARE(QApplication::activeModalWidget(), units->window());
        QTRY_COMPARE(QApplication::activeWindow(), units->window());
        const auto evidence = qEnvironmentVariable("ALLOY_DISPLAY_UNITS_EVIDENCE");
        if (!evidence.isEmpty()) {
            QVERIFY(QDir().mkpath(evidence));
            const auto capture = window.execute("capture.screenshot", {{"path", evidence + "/browser-change-units.png"}});
            QVERIFY(capture.value("ok").toBool());
        }
        QCOMPARE(units->currentText(), QString("mm"));
        QCOMPARE(window.execute("document.inspect").value("result").toObject().value("authored"), before.value("authored"));
        QTest::mouseClick(units, Qt::LeftButton);
        QTRY_VERIFY(units->view()->isVisible());
        QTRY_COMPARE(QApplication::activePopupWidget(), units->view()->window());
        QTest::keyClick(units->view(), Qt::Key_Down);
        QTest::keyClick(units->view(), Qt::Key_Return);
        QTRY_VERIFY(!units->view()->isVisible());
        QTRY_VERIFY(!QApplication::activePopupWidget());
        QTRY_COMPARE(QApplication::activeWindow(), units->window());
        QTRY_COMPARE(units->currentText(), QString("in"));
        auto* box = window.findChild<QDialogButtonBox*>();
        QVERIFY(box && box->button(QDialogButtonBox::Ok));
        auto* ok = box->button(QDialogButtonBox::Ok);
        QCOMPARE(QApplication::widgetAt(ok->mapToGlobal(ok->rect().center())), static_cast<QWidget*>(ok));
        QTest::mouseClick(ok, Qt::LeftButton);
        QTRY_VERIFY(!units->window()->isVisible());
        QTRY_VERIFY(!QApplication::activeModalWidget());
        QTRY_COMPARE(window.execute("document.inspect").value("result").toObject().value("units"), QJsonValue("in"));
        QCOMPARE(button->text(), QString("Units: in  ✎"));
        const auto gpu = window.state().value("viewport").toObject().value("gpu").toObject();
        QVERIFY(gpu.value("hardwareAccelerated").toBool());
    }

    void browserCancel() {
        AlloyTheme::apply(*qApp);
        AlloyWindow window;
        window.resize(1250, 800);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto* tree = window.findChild<QTreeWidget*>("browser.tree");
        auto* button = window.findChild<QToolButton*>("browser.documentUnits.change");
        QVERIFY(tree && button);
        tree->topLevelItem(0)->setExpanded(true);
        tree->topLevelItem(0)->child(0)->setExpanded(true);
        tree->scrollToItem(tree->topLevelItem(0)->child(0)->child(0));
        QTRY_VERIFY(button->isVisible());
        QCOMPARE(QApplication::widgetAt(button->mapToGlobal(button->rect().center())), static_cast<QWidget*>(button));
        const auto before = window.execute("document.inspect").value("result").toObject();
        QTest::mouseClick(button, Qt::LeftButton);
        auto* units = window.findChild<QComboBox*>("document.units.input");
        QTRY_VERIFY(units->isVisible());
        QTRY_COMPARE(QApplication::activeWindow(), units->window());
        auto* box = window.findChild<QDialogButtonBox*>();
        QVERIFY(box && box->button(QDialogButtonBox::Cancel));
        QTest::mouseClick(box->button(QDialogButtonBox::Cancel), Qt::LeftButton);
        QTRY_VERIFY(!units->window()->isVisible());
        QTRY_VERIFY(!QApplication::activeModalWidget());
        QCOMPARE(window.execute("document.inspect").value("result").toObject().value("authored"), before.value("authored"));
    }
};

QTEST_MAIN(DisplayUnitsControlsTest)
#include "display_units_controls.moc"
