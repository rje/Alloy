// SPDX-License-Identifier: MIT
#include "AlloyWindow.h"
#include "AlloyTheme.h"
#include "ReferenceView.h"
#include <QApplication>
#include <QDir>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QTest>

class RoundedRectangleControls final : public QObject {
    Q_OBJECT
    QJsonObject call(AlloyWindow& window,const QString& name,const QJsonObject& args={}){
        const auto response=window.execute(name,args);
        if(!response["ok"].toBool())qFatal("%s",QJsonDocument(response).toJson().constData());
        return response["result"].toObject();
    }
    QJsonObject session(AlloyWindow& w){return w.state()["activeSession"].toObject();}
    QJsonObject draft(AlloyWindow& w){return session(w)["draftSketch"].toObject();}
private slots:
    void sidePairsAndPreviewCancel(){
        AlloyTheme::apply(*qApp);AlloyWindow w;w.resize(1500,950);w.show();w.activateWindow();
        QVERIFY(QTest::qWaitForWindowExposed(&w));QApplication::setActiveWindow(&w);
        ReferenceView* view=nullptr;for(auto* child:w.findChildren<QWidget*>())if(auto* candidate=dynamic_cast<ReferenceView*>(child))view=candidate;
        QVERIFY(view);
        call(w,"sketch.begin",{{"plane","XY"},{"units","mm"}});call(w,"sketch.lookAt");
        call(w,"sketch.tool",{{"tool","rectangle"}});
        const auto sketchId=draft(w)["id"].toString();
        auto pixel=[&](double u,double v){const auto p=call(w,"view.projectSketchPoint",{{"sketchId",sketchId},{"uMm",u},{"vMm",v}});
            return QPoint(qRound(p["x"].toDouble()),qRound(p["y"].toDouble()));};
        auto place=[&](double u,double v){const auto p=pixel(u,v);return call(w,"sketch.placePoint",{{"x",p.x()},{"y",p.y()}});};
        place(0,0);const auto p=pixel(20,-10);call(w,"sketch.pointerMove",{{"x",p.x()},{"y",p.y()}});
        call(w,"sketch.twoPointRectanglePreview",{{"widthText","60"},{"heightText","30"}});
        call(w,"sketch.twoPointRectangleAccept");call(w,"sketch.tool",{{"tool","select"}});
        const auto base=draft(w);const auto lines=base["parameters"].toObject()["entities"].toArray();QCOMPARE(lines.size(),4);
        call(w,"view.zoom",{{"factor",0.7}});const auto center=pixel(30,-15);
        call(w,"view.pan",{{"x",view->width()/2-center.x()},{"y",view->height()/2-center.y()}});
        const double sideU[][2]={{30,60},{60,30},{30,0},{0,30}};
        const double sideV[][2]={{0,-15},{-15,-30},{-30,-15},{-15,0}};
        auto* radius=w.findChild<QLineEdit*>("sketch.fillet.radius");QVERIFY(radius);
        for(int i=0;i<4;++i){
            call(w,"sketch.tool",{{"tool","fillet"}});
            const auto first=pixel(sideU[i][0],sideV[i][0]),second=pixel(sideU[i][1],sideV[i][1]);
            QVERIFY(view->rect().contains(first));QVERIFY(view->rect().contains(second));
            QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,first);
            QTRY_VERIFY(session(w)["pendingFillet"].toObject().contains("firstSideId"));
            QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,second);
            QTRY_VERIFY(session(w)["pendingFillet"].toObject()["sidePair"].toBool());
            QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(radius));
            QTest::keyClick(QApplication::focusWidget(),Qt::Key_6);
            QTRY_VERIFY(session(w)["pendingFillet"].toObject().contains("previewDraft"));
            if(i==3){
                auto* notice=w.findChild<QLabel*>("sketch.fillet.referenceChanges");QVERIFY(notice);
                QVERIFY(notice->text().contains("origin anchor"));
                QTRY_VERIFY(notice->height()>=notice->heightForWidth(notice->width()));
                QTRY_VERIFY(radius->mapToGlobal(QPoint{}).y()>=notice->mapToGlobal(QPoint{}).y()+notice->height());
                qInfo("fillet notice width=%d height=%d hint=%d heightForWidth=%d radiusY=%d noticeY=%d",
                    notice->width(),notice->height(),notice->sizeHint().height(),notice->heightForWidth(notice->width()),
                    radius->mapToGlobal(QPoint{}).y(),notice->mapToGlobal(QPoint{}).y());
                const auto output=qEnvironmentVariable("ALLOY_ROUNDED_RECTANGLE_EVIDENCE");
                if(!output.isEmpty()){QDir().mkpath(output);call(w,"capture.screenshot",{{"path",output+"/rounded-preview.png"}});}
                const auto before=draft(w);call(w,"sketch.filletCancel");QCOMPARE(draft(w),before);
                call(w,"sketch.tool",{{"tool","select"}});call(w,"sketch.tool",{{"tool","fillet"}});
                QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,first);
                QTest::mouseClick(view,Qt::LeftButton,Qt::NoModifier,second);
                QTRY_COMPARE(QApplication::focusWidget(),static_cast<QWidget*>(radius));
                QTest::keyClick(QApplication::focusWidget(),Qt::Key_6);
            }
            QTest::keyClick(QApplication::focusWidget(),Qt::Key_Return);
            QTRY_VERIFY(!session(w).contains("pendingFillet"));
            QCOMPARE(draft(w)["parameters"].toObject()["entities"].toArray().size(),5+i);
        }
        call(w,"sketch.cancel");w.close();
    }
};
QTEST_MAIN(RoundedRectangleControls)
#include "rounded_rectangle_controls.moc"
