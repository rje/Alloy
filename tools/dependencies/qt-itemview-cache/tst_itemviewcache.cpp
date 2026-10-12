// SPDX-License-Identifier: MIT
// Real Qt item-view accessibility cache regression; no platform factory override.
#include <QAccessible>
#include <QAbstractItemView>
#include <QListView>
#include <QStandardItemModel>
#include <QTableView>
#include <QTreeView>
#include <QtTest>
#include <memory>

class ItemViewCacheTest : public QObject {
    Q_OBJECT
private slots:
    void eviction_data() {
        QTest::addColumn<QString>("kind");
        QTest::newRow("table") << QString("table");
        QTest::newRow("tree") << QString("tree");
        QTest::newRow("list") << QString("list");
    }
    void eviction() {
        QFETCH(QString, kind);
        QStandardItemModel model(3, 1);
        for (int row = 0; row < 3; ++row)
            model.setData(model.index(row, 0), QString("row-%1").arg(row));
        std::unique_ptr<QAbstractItemView> view;
        if (kind == "table") view = std::make_unique<QTableView>();
        else if (kind == "tree") view = std::make_unique<QTreeView>();
        else view = std::make_unique<QListView>();
        view->setModel(&model);
        view->resize(320, 240);
        view->show();
        QCoreApplication::processEvents();
        auto *root = QAccessible::queryAccessibleInterface(view.get());
        QVERIFY(root);
        auto *table = root->tableInterface();
        QVERIFY(table);

        // Expiry without a model event must recreate each Table/Tree/List child.
        auto *first = table->cellAt(0, 0);
        QVERIFY(first);
        const int logicalIndex = root->indexOfChild(first);
        QVERIFY(logicalIndex >= 0);
        const auto firstId = QAccessible::uniqueId(first);
        QAccessible::deleteAccessibleInterface(firstId);
        QVERIFY(!QAccessible::accessibleInterface(firstId));
        auto *recreated = root->child(logicalIndex);
        QVERIFY(recreated);
        QVERIFY(recreated->isValid());
        QCOMPARE(recreated->text(QAccessible::Name), QString("row-0"));

        // Dead IDs must be pruned by both insertion and removal cache rebuilds.
        auto *stale = table->cellAt(1, 0);
        QVERIFY(stale);
        auto staleId = QAccessible::uniqueId(stale);
        QAccessible::deleteAccessibleInterface(staleId);
        model.insertRow(0, new QStandardItem("inserted"));
        QCoreApplication::processEvents();
        // Dispatch explicitly as well: this remains deterministic when no OS
        // accessibility client is attached. The model and view are real widgets.
        QAccessibleTableModelChangeEvent inserted(view.get(), QAccessibleTableModelChangeEvent::RowsInserted);
        inserted.setFirstRow(0);
        inserted.setLastRow(0);
        table->modelChange(&inserted);
        QCOMPARE(table->rowCount(), 4);
        auto *shifted = table->cellAt(1, 0);
        QVERIFY(shifted);
        QVERIFY(shifted->isValid());
        QCOMPARE(shifted->text(QAccessible::Name), QString("row-0"));

        stale = table->cellAt(0, 0);
        QVERIFY(stale);
        staleId = QAccessible::uniqueId(stale);
        QAccessible::deleteAccessibleInterface(staleId);
        QVERIFY(model.removeRow(2));
        QCoreApplication::processEvents();
        QAccessibleTableModelChangeEvent removed(view.get(), QAccessibleTableModelChangeEvent::RowsRemoved);
        removed.setFirstRow(2);
        removed.setLastRow(2);
        table->modelChange(&removed);
        QCOMPARE(table->rowCount(), 3);
        auto *surviving = table->cellAt(0, 0);
        QVERIFY(surviving);
        QVERIFY(surviving->isValid());
        QCOMPARE(surviving->text(QAccessible::Name), QString("inserted"));
    }
};
QTEST_MAIN(ItemViewCacheTest)
#include "tst_itemviewcache.moc"
