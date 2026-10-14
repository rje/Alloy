// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include <QApplication>
#include <QDialog>
#include <QHeaderView>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QTest>
#include <QTreeWidget>

class ModelParameterTableControlsTest final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& args={}) {
        const auto response=window.execute(command,args);
        if(!response["ok"].toBool())qFatal("%s: %s",qPrintable(command),QJsonDocument(response).toJson().constData());
        return response["result"].toObject();
    }
    QTreeWidgetItem* depthItem(QTreeWidget* tree) {
        QTreeWidgetItemIterator it(tree);
        while(*it) {auto* item=*it;
            if(item->data(1,Qt::UserRole).toJsonObject()["field"]=="depthMm")return item;
            ++it;
        }
        return nullptr;
    }
    void enterExpression(QTreeWidget* tree,QTreeWidgetItem* item,const QString& expression) {
        QVERIFY(item);tree->scrollToItem(item);
        const auto rect=tree->visualItemRect(item);
        const QPoint position(tree->header()->sectionViewportPosition(1)+tree->header()->sectionSize(1)/2,rect.center().y());
        QTest::mouseDClick(tree->viewport(),Qt::LeftButton,Qt::NoModifier,position);
        QTRY_VERIFY(qobject_cast<QLineEdit*>(QApplication::focusWidget()));
        QTest::keyClick(QApplication::focusWidget(),Qt::Key_A,Qt::ControlModifier);
        QTest::keyClicks(QApplication::focusWidget(),expression);
        QTest::keyClick(QApplication::focusWidget(),Qt::Key_Return);
    }
private slots:
    void rebindExistingBoxThroughExpressionCell() {
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1350,900);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        call(window,"solid.box",{{"width",50},{"depth",80},{"height",40},{"units","mm"},{"name","Box"}});
        const auto box=call(window,"document.inspect")["authored"].toObject()["features"].toArray().first().toObject();
        call(window,"document.parameterAdd",{{"name","Length"},{"value",80},{"units","mm"}});
        const auto user=call(window,"document.inspect")["userParameters"].toArray().first().toObject();
        call(window,"document.parametersDialog");
        auto* dialog=window.findChild<QDialog*>("parameters.dialog");QVERIFY(dialog && dialog->isVisible());
        auto* tree=dialog->findChild<QTreeWidget*>("parameters.modelTable");QVERIFY(tree);
        auto* status=dialog->findChild<QLabel*>("parameters.status");QVERIFY(status);
        QApplication::setActiveWindow(dialog);
        QTRY_VERIFY(depthItem(tree));
        QCOMPARE(depthItem(tree)->text(1),QString("80 mm"));
        const auto evidence=qEnvironmentVariable("ALLOY_MODEL_TABLE_EVIDENCE");
        if(!evidence.isEmpty())call(window,"capture.screenshot",{{"path",evidence+"/literal-table.png"}});
        enterExpression(tree,depthItem(tree),"Length");
        QTRY_COMPARE(call(window,"document.inspect")["authored"].toObject()["features"].toArray().first().toObject()["parameters"].toObject()["depthParameterId"],user["id"]);
        QTRY_COMPARE(depthItem(tree)->text(1),QString("Length"));
        QCOMPARE(depthItem(tree)->text(3),QString("Length · 80 mm"));
        if(!evidence.isEmpty())call(window,"capture.screenshot",{{"path",evidence+"/bound-table.png"}});
        const auto bound=call(window,"document.inspect")["authored"].toObject();
        enterExpression(tree,depthItem(tree),"MissingLength");
        QTRY_VERIFY(status->text().contains("unavailable"));
        QCOMPARE(call(window,"document.inspect")["authored"].toObject(),bound);
        QCOMPARE(depthItem(tree)->text(1),QString("Length"));
        enterExpression(tree,depthItem(tree),"80 mm");
        QTRY_VERIFY(!call(window,"document.inspect")["authored"].toObject()["features"].toArray().first().toObject()["parameters"].toObject().contains("depthParameterId"));
        QCOMPARE(depthItem(tree)->text(1),QString("80 mm"));
        QCOMPARE(call(window,"document.inspect")["authored"].toObject()["features"].toArray().first().toObject()["id"],box["id"]);
        dialog->close();window.close();
    }
};
QTEST_MAIN(ModelParameterTableControlsTest)
#include "model_parameter_table_controls.moc"
