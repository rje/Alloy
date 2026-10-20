// SPDX-License-Identifier: MIT
#pragma once
#include <QApplication>
#include <QJsonObject>
#include <QJsonArray>
#include <QMenu>
#include <QMouseEvent>
#include <QToolButton>
#include <QWidgetAction>
#include <QTimer>
#include <array>
#include <cmath>
#include <functional>

// Canvas-owned popup. The context list remains normal QMenu actions so native
// keyboard traversal, accessibility, outside-click and Escape dismissal apply.
// Radial buttons are real widgets with stable IDs, never painted lookalikes.
class CanvasMarkingMenu final : public QMenu {
public:
    using Run = std::function<void(const QString&, const QJsonObject&)>;
    CanvasMarkingMenu(QWidget* owner, Run run, bool selected) : QMenu(owner), run_(std::move(run)) {
        setAttribute(Qt::WA_DeleteOnClose);
        setAccessibleName("Canvas marking and context menu");
        setStyleSheet("QMenu { background:#fafafa; color:#26333e; border:1px solid #b8bdc2; padding:3px; }"
                      "QMenu::item { padding:3px 18px 3px 9px; } QMenu::item:selected { background:#168bc1; color:white; }"
                      "QToolButton { background:#f5f5f5; border:1px solid #bfc3c7; border-radius:2px; padding:2px; color:#26333e; font-size:11px; }"
                      "QToolButton:hover { background:#ccecff; border-color:#168bc1; }");
        panel_=new QWidget(this);panel_->setFixedSize(300,180);
        auto* radial=new QWidgetAction(this);radial->setObjectName("context.radial");radial->setDefaultWidget(panel_);addAction(radial);
        const std::array<QPoint,8> places{QPoint(102,4),QPoint(193,34),QPoint(202,73),QPoint(193,112),QPoint(102,148),QPoint(7,112),QPoint(2,73),QPoint(7,34)};
        for(int i=0;i<8;++i){auto* b=new QToolButton(panel_);b->setGeometry(QRect(places[i],QSize(96,25)));buttons_[i]=b;}
        center_=new QToolButton(panel_);center_->setObjectName("context.radial.dismiss");center_->setAccessibleName("Dismiss canvas menu");
        center_->setText("•");center_->setGeometry(133,78,34,24);connect(center_,&QToolButton::clicked,this,&QMenu::close);
        back_=new QToolButton(panel_);back_->setObjectName("context.radial.back");back_->setAccessibleName("Return to first radial menu");
        back_->setText("↑");back_->setGeometry(145,50,20,20);connect(back_,&QToolButton::clicked,this,[this]{firstLevel();});
        selected_=selected;firstLevel();
    }
    void openAt(QPoint global) { origin_=global;popup(global-QPoint(150,90));qApp->installEventFilter(this); }
    QJsonObject inspection() const {
        QJsonArray entries;
        for(auto* button:buttons_)if(!button->isHidden()) {
            const auto bounds=button->geometry();
            entries.append(QJsonObject{{"id",button->objectName()},{"text",button->text()},
                {"command",button->property("commandId").toString()},
                {"arguments",QJsonValue::fromVariant(button->property("commandArguments"))},
                {"x",bounds.x()},{"y",bounds.y()},{"width",bounds.width()},{"height",bounds.height()}});
        }
        return {{"level",second_ ? "sketch" : "design"},{"entries",entries},
                {"width",width()},{"height",height()},{"gestureThresholdLogicalPixels",45}};
    }
    ~CanvasMarkingMenu() override { qApp->removeEventFilter(this); }
private:
    QWidget* panel_=nullptr;
    QToolButton* center_=nullptr;
    QToolButton* back_=nullptr;
    std::array<QToolButton*,8> buttons_{};
    Run run_;
    bool second_=false, selected_=false, gesturing_=false;
    QPoint origin_, secondOrigin_;
    void entry(int index,const QString& title,const QString& id,const QString& command,const QJsonObject& args={}) {
        auto* b=buttons_[index];b->show();b->setText(title);b->setObjectName(id);b->setProperty("commandId",command);b->setProperty("commandArguments",args);b->setAccessibleName(title);b->setToolTip(title);
        QObject::disconnect(b,nullptr,this,nullptr);
        connect(b,&QToolButton::clicked,this,[this,command,args,id]{invoke(id,command,args);});
    }
    void invoke(const QString& id,const QString& command,const QJsonObject& args={}) {
        setProperty("radialResult",id);auto run=run_;close();QTimer::singleShot(0,parentWidget(),[run,command,args]{run(command,args);});
    }
    void clearButtons(){for(auto* b:buttons_){QObject::disconnect(b,nullptr,this,nullptr);b->hide();}}
    void firstLevel(){
        buttons_[1]->setGeometry(193,34,96,25);buttons_[6]->setGeometry(2,73,96,25);buttons_[7]->setGeometry(7,34,96,25);
        back_->hide();second_=false;setProperty("radialLevel","design");clearButtons();
        entry(2,"Redo","context.radial.redo","history.redo");
        entry(6,"Undo","context.radial.undo","history.undo");
        if(selected_)entry(7,"Delete","context.radial.delete","sketch.deleteSelection");
        entry(4,"Sketch ▾","context.radial.sketch",{});
        QObject::disconnect(buttons_[4],nullptr,this,nullptr);
        connect(buttons_[4],&QToolButton::clicked,this,[this]{sketchLevel();});
    }
    void sketchLevel(){
        back_->show();second_=true;setProperty("radialLevel","sketch");clearButtons();
        entry(0,"Finish Sketch","context.radial.finish","sketch.finish");
        entry(1,"2-Point Rectangle","context.radial.rectangle","sketch.twoPointRectangle");
        entry(2,"Fit Point Spline","context.radial.spline","sketch.tool",{{"tool","fitSpline"}});
        entry(3,"Project","context.radial.project","sketch.projectBegin");
        entry(4,"Line","context.radial.line","sketch.tool",{{"tool","line"}});
        entry(5,"Offset","context.radial.offset","sketch.tool",{{"tool","offset"}});
        entry(6,"Sketch Dimension","context.radial.dimension","sketch.tool",{{"tool","dimension"}});
        entry(7,"Center Diameter Circle","context.radial.circle","sketch.tool",{{"tool","circle"}});
        // Keep the documented names readable rather than eliding them.
        buttons_[1]->setGeometry(177,34,121,25);
        buttons_[6]->setGeometry(0,73,122,25);
        buttons_[7]->setGeometry(0,34,139,25);
    }
    bool eventFilter(QObject* watched,QEvent* event) override {
        if(!isVisible())return QMenu::eventFilter(watched,event);
        if(event->type()==QEvent::MouseMove || event->type()==QEvent::MouseButtonRelease){
            auto* mouse=static_cast<QMouseEvent*>(event);
            if(mouse->buttons().testFlag(Qt::RightButton) || mouse->button()==Qt::RightButton){
                const QPoint global=mouse->globalPosition().toPoint();
                if(!second_ && global.y()-origin_.y()>=45 && std::abs(global.x()-origin_.x())<55){
                    gesturing_=true;secondOrigin_=global;sketchLevel();move(global-QPoint(150,90));
                }
                if(event->type()==QEvent::MouseButtonRelease && gesturing_){
                    const auto d=global-secondOrigin_;
                    if(std::hypot(d.x(),d.y())>=28){
                        // Clockwise octants from up. Thresholds are Alloy policy;
                        // the public source documents directions, not distances.
                        const int sector=(int(std::lround(std::atan2(double(d.x()),double(-d.y()))/(3.141592653589793/4)))+8)%8;
                        buttons_[sector]->click();
                    }
                    return true;
                }
                if(event->type()==QEvent::MouseButtonRelease)return true;
            }
        }
        return QMenu::eventFilter(watched,event);
    }
};
