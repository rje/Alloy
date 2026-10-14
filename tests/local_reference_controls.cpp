// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QJsonDocument>
#include <QPushButton>
#include <QTest>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>

class LocalReferenceControlsTest final:public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& command,const QJsonObject& arguments={}){
        const auto response=window.execute(command,arguments);
        if(!response["ok"].toBool())qFatal("%s: %s",qPrintable(command),QJsonDocument(response).toJson().constData());
        return response["result"].toObject();
    }
private slots:
    void nativeMovePreviewCancelAndCapture(){
        const auto fixture=qEnvironmentVariable("ALLOY_ASSEMBLY_FIXTURE");if(fixture.isEmpty())QSKIP("External board fixture unavailable");
        AlloyTheme::apply(*qApp);AlloyWindow window;window.resize(1500,950);window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));window.activateWindow();
        const auto inserted=call(window,"reference.insert",{{"path",fixture},{"name","Board Reference"}});
        const auto id=inserted["featureId"].toString();const auto before=call(window,"document.inspect")["authored"].toObject();
        auto* tree=window.findChild<QTreeWidget*>("browser.tree");QVERIFY(tree);
        QTreeWidgetItem* row=nullptr;for(QTreeWidgetItemIterator it(tree);*it;++it)
            if((*it)->data(0,Qt::UserRole+6)==id){row=*it;break;}
        QVERIFY(row);for(auto* parent=row->parent();parent;parent=parent->parent())parent->setExpanded(true);
        tree->scrollToItem(row);QApplication::processEvents();const auto rect=tree->visualItemRect(row);
        QVERIFY(rect.isValid() && tree->itemAt(rect.center())==row);
        call(window,"reference.moveDialog",{{"featureId",id}});
        QTRY_VERIFY(window.findChild<QDialog*>("reference.move.dialog"));
        auto* dialog=window.findChild<QDialog*>("reference.move.dialog");QVERIFY(dialog && dialog->isVisible());
        auto* x=dialog->findChild<QDoubleSpinBox*>("reference.move.x");QVERIFY(x);
        QTest::mouseClick(x,Qt::LeftButton);QTest::keyClick(x,Qt::Key_A,Qt::ControlModifier);QTest::keyClicks(x,"12");QTest::keyClick(x,Qt::Key_Tab);
        QTRY_VERIFY(!window.state()["pendingReferencePosition"].toObject().isEmpty());
        QCOMPARE(call(window,"document.inspect")["authored"].toObject(),before);
        const auto evidence=qEnvironmentVariable("ALLOY_REFERENCE_EVIDENCE");
        if(!evidence.isEmpty()){call(window,"capture.screenshot",{{"path",evidence+"/reference-native-preview.png"}});
            QVERIFY(dialog->grab().save(evidence+"/reference-native-dialog.png"));}
        auto* buttons=dialog->findChild<QDialogButtonBox*>();QVERIFY(buttons);
        QTest::mouseClick(buttons->button(QDialogButtonBox::Cancel),Qt::LeftButton);
        QTRY_VERIFY(window.state()["pendingReferencePosition"].toObject().isEmpty());
        QCOMPARE(call(window,"document.inspect")["authored"].toObject(),before);
        call(window,"document.new",{{"discard",true}});
        window.close();
    }
};
QTEST_MAIN(LocalReferenceControlsTest)
#include "local_reference_controls.moc"
