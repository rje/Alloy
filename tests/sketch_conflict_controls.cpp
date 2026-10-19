// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QLabel>
#include <QPushButton>
#include <QTest>
#include <QWindow>
class SketchConflictControls final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& w,const QString& command,const QJsonObject& args={}) {
        const auto result=w.executeApi(command,args);
        if(!result.value("ok").toBool())qFatal("%s: %s",qPrintable(command),qPrintable(result.value("error").toObject().value("message").toString()));
        return result.value("result").toObject();
    }
private slots:
    void conflictSelectionAndBadgeDelete() {
        AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1500,1000);w.show();QVERIFY(QTest::qWaitForWindowExposed(&w));
        ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* v=dynamic_cast<ReferenceView*>(child))view=v;QVERIFY(view);
        auto session=[&]{return w.state().value("activeSession").toObject();};
        auto draft=[&]{return session().value("draftSketch").toObject();};
        call(w,"sketch.begin",{{"plane","XY"},{"units","mm"},{"name","Conflict repair"}});
        call(w,"sketch.line",{{"x1",0},{"y1",0},{"x2",40},{"y2",0},{"units","mm"}});
        const auto line=draft().value("parameters").toObject().value("entities").toArray().last().toObject();
        call(w,"sketch.constraint",{{"type","fixed"},{"pointIds",QJsonArray{line.value("startPointId")}}});
        call(w,"sketch.dimension",{{"kind","linear"},{"entityId",line.value("id")},{"value",40},{"units","mm"}});
        call(w,"sketch.constraint",{{"type","horizontal"},{"entityIds",QJsonArray{line.value("id")}}});
        const auto existing=draft().value("parameters").toObject().value("constraints").toArray().last().toObject();
        call(w,"sketch.tool",{{"tool","select"}});call(w,"sketch.selectionSet",{{"entityIds",QJsonArray{line.value("id")}}});call(w,"sketch.lookAt");
        // Initial harness activation only. A locked desktop must fail here;
        // no subsequent step repairs focus, selection or native activation.
        w.raise();w.activateWindow();w.windowHandle()->requestActivate();QTRY_VERIFY(w.isActiveWindow());
        const auto baseline=draft();const auto history=session().value("draftHistory");
        const auto failed=w.executeApi("sketch.constraint",{{"type","vertical"},{"entityIds",QJsonArray{line.value("id")}}});
        QVERIFY(!failed.value("ok").toBool());QCOMPARE(failed.value("error").toObject().value("code").toString(),QString("constraint_conflict"));
        QCOMPARE(draft(),baseline);QCOMPARE(session().value("draftHistory"),history);
        auto* error=w.findChild<QLabel*>("sketch.error");auto* select=w.findChild<QPushButton*>("sketch.conflict.select");QVERIFY(error&&select);
        QTRY_VERIFY(select->isVisible());QVERIFY(error->text().contains("Vertical conflicts with existing Horizontal"));
        QVERIFY(error->height()>=error->heightForWidth(error->contentsRect().width()));
        const auto p=select->rect().center();QVERIFY(!select->childAt(p));QTest::mouseClick(select,Qt::LeftButton,Qt::NoModifier,p);
        QCOMPARE(session().value("selection").toObject().value("constraintIds").toArray(),QJsonArray{existing.value("id")});
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QVERIFY(w.isActiveWindow());
        QJsonObject glyph;for(const auto& row:w.state().value("viewport").toObject().value("gpu").toObject().value("sketchConstraintGlyphs").toArray())
            if(row.toObject().value("constraintId")==existing.value("id"))glyph=row.toObject();
        QVERIFY(!glyph.isEmpty());const auto b=glyph.value("bounds").toObject();const QPoint at(qRound(b.value("x").toDouble()+b.value("width").toDouble()/2),qRound(b.value("y").toDouble()+b.value("height").toDouble()/2));
        QVERIFY(view->rect().contains(at));QVERIFY(!view->childAt(at));QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,at);
        QCOMPARE(session().value("selection").toObject().value("constraintIds").toArray(),QJsonArray{existing.value("id")});
        QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(view));QTest::keyClick(view,Qt::Key_Delete);
        for(const auto& row:draft().value("parameters").toObject().value("constraints").toArray())QVERIFY(row.toObject().value("id")!=existing.value("id"));
        QVERIFY(!select->isVisible());call(w,"history.undo");QCOMPARE(draft(),baseline);call(w,"history.redo");
        call(w,"sketch.constraint",{{"type","vertical"},{"entityIds",QJsonArray{line.value("id")}}});
        call(w,"sketch.cancel");w.close();
    }
};
QTEST_MAIN(SketchConflictControls)
#include "sketch_conflict_controls.moc"
