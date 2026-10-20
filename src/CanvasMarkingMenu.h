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
#include <QPainter>
#include <QPainterPath>
#include <QScreen>
#include <QKeyEvent>
#include <QStyle>
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
        setAttribute(Qt::WA_TranslucentBackground);
        setStyleSheet("QMenu { background:transparent; color:#26333e; border:none; padding:3px; font-size:11px; }"
                      "QMenu::item { background:#fafafa; padding:2px 18px 2px 8px; }"
                      "QMenu::separator { height:1px; background:#c9cdd0; margin:3px 5px; }"
                      "QMenu::item:selected { background:#168bc1; color:white; }"
                      "QToolButton { background:#f5f5f5; border:1px solid #bfc3c7; border-radius:2px; padding:2px; color:#26333e; font-size:11px; }"
                      "QToolButton:hover, QToolButton:focus, QToolButton[gestureActive=\"true\"] { background:#ccecff; border-color:#168bc1; }"
                      "QToolButton:pressed { background:#168bc1; color:white; }");
        panel_=new QWidget(this);panel_->setFixedSize(350,166);
        panel_->setObjectName("context.radial");
        panel_->move(3,3);
        for(int i=0;i<8;++i){auto* b=new QToolButton(panel_);b->setFocusPolicy(Qt::StrongFocus);b->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);b->setIconSize({14,14});buttons_[i]=b;}
        center_=new QToolButton(panel_);center_->setObjectName("context.radial.dismiss");center_->setAccessibleName("Dismiss canvas menu");
        center_->setText("•");center_->setGeometry(163,69,24,24);center_->setFocusPolicy(Qt::StrongFocus);connect(center_,&QToolButton::clicked,this,&QMenu::close);
        back_=new QToolButton(panel_);back_->setObjectName("context.radial.back");back_->setAccessibleName("Return to first radial menu");
        back_->setText("↑");back_->setGeometry(165,39,20,22);back_->setFocusPolicy(Qt::StrongFocus);connect(back_,&QToolButton::clicked,this,[this]{firstLevel();});
        selected_=selected;firstLevel();
    }
    void openAt(QPoint global) {
        for(auto* action:actions())if(!qobject_cast<QWidgetAction*>(action)&&!action->isSeparator())action->setIcon(icon(action->objectName()));
        // QMenu computes the real row width from its live font, icons and tab
        // column. Reserve symmetric margins around that list, rather than
        // letting the radial header widen every action's hit rectangle.
        const int listWidth=sizeHint().width();
        const int inset=qMax(0,(356-listWidth)/2);
        setContentsMargins(inset,169,inset,0);
        origin_=global;popup(global-QPoint(178,84));
        panel_->move((width()-panel_->width())/2,3);
        placeAt(global);
        qApp->installEventFilter(this);
    }
    QJsonObject inspection() const {
        QJsonArray entries;
        for(auto* button:buttons_)if(!button->isHidden()) {
            const auto bounds=button->geometry();
            entries.append(QJsonObject{{"id",button->objectName()},{"text",button->text()},
                {"command",button->property("commandId").toString()},
                {"arguments",QJsonValue::fromVariant(button->property("commandArguments"))},
                {"x",bounds.x()},{"y",bounds.y()},{"width",bounds.width()},{"height",bounds.height()}});
        }
        QRect list;QJsonArray rows;
        for(auto* action:actions())if(action->isVisible()) {
            const auto bounds=actionGeometry(action);if(bounds.isEmpty())continue;
            list=list.united(bounds);
            rows.append(QJsonObject{{"id",action->objectName()},{"x",bounds.x()},{"y",bounds.y()},{"width",bounds.width()},{"height",bounds.height()}});
        }
        return {{"level",second_ ? "sketch" : "design"},{"entries",entries},
                {"contextList",QJsonObject{{"x",list.x()},{"y",list.y()},{"width",list.width()},{"height",list.height()},{"rows",rows}}},
                {"width",width()},{"height",height()},{"globalX",x()},{"globalY",y()},
                {"devicePixelRatio",devicePixelRatioF()},{"gestureThresholdLogicalPixels",45}};
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
    // Original geometric glyphs, drawn at the screen DPR; no reference assets.
    QIcon icon(const QString& id) const {
        const qreal dpr=devicePixelRatioF();QPixmap pix(QSize(16,16)*dpr);pix.setDevicePixelRatio(dpr);pix.fill(Qt::transparent);
        QPainter p(&pix);p.setRenderHint(QPainter::Antialiasing);p.setPen(QPen(QColor("#536571"),1.4,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
        const auto suffix=id.section('.',-1);const auto name=suffix=="deleteGeometry" ? QString("delete") : suffix;
        if(name=="circle"){p.setPen(QPen(QColor("#168bc1"),1.4));p.drawEllipse(QRectF(2,2,12,12));p.drawPoint(QPointF(8,8));}
        else if(name=="rectangle"){p.drawRect(QRectF(2,3,12,10));}
        else if(name=="line"){p.setPen(QPen(QColor("#168bc1"),1.5));p.drawLine(2,13,14,2);p.drawRect(1,12,2,2);p.drawRect(13,1,2,2);}
        else if(name=="spline"){QPainterPath path;path.moveTo(1,12);path.cubicTo(3,-4,12,20,15,3);p.drawPath(path);}
        else if(name=="finish"){p.setPen(QPen(QColor("#3d8c59"),2));p.drawLine(2,8,6,12);p.drawLine(6,12,14,3);}
        else if(name=="delete"){p.setPen(QPen(QColor("#ce514a"),2));p.drawLine(4,4,12,12);p.drawLine(4,12,12,4);}
        else if(name=="undo"||name=="redo"){if(name=="redo"){p.translate(16,0);p.scale(-1,1);}QPainterPath path;path.moveTo(13,12);path.cubicTo(15,5,8,3,3,6);p.drawPath(path);p.drawLine(3,6,3,2);p.drawLine(3,6,7,7);}
        else if(name=="offset"){p.drawLine(2,5,14,5);p.drawLine(2,11,14,11);p.drawLine(8,2,8,14);}
        else if(name=="dimension"){p.setPen(QPen(QColor("#168bc1"),1));p.drawLine(2,2,2,14);p.drawLine(14,2,14,14);p.drawLine(2,8,14,8);p.drawLine(2,8,5,5);p.drawLine(14,8,11,11);}
        else if(name=="project"){p.setPen(QPen(QColor("#815da4"),1.4));p.drawRect(1,4,9,10);p.drawLine(6,1,14,1);p.drawLine(14,1,14,9);p.drawLine(13,3,7,9);p.drawLine(7,9,7,6);p.drawLine(7,9,10,9);}
        else if(name=="constructionToggle"){p.setPen(QPen(QColor("#168bc1"),1.5,Qt::DashLine));p.drawLine(1,13,14,2);}
        else if(name=="parallel"||name=="collinear"){p.setPen(QPen(QColor("#ce514a"),1.2));p.drawLine(2,11,10,3);p.drawLine(name=="parallel"?6:10,name=="parallel"?14:3,14,name=="parallel"?6:0);}
        else if(name=="midpoint"){p.setPen(QPen(QColor("#ce514a"),1.2));p.drawPolygon(QPolygonF{{2,13},{8,2},{14,13}});}
        else if(name=="perpendicular"||name=="horizontalVertical"){p.setPen(QPen(QColor("#ce514a"),1.2));p.drawLine(3,2,3,13);p.drawLine(3,13,14,13);if(name=="perpendicular")p.drawRect(3,9,4,4);}
        else if(name=="equal"){p.setPen(QPen(QColor("#ce514a"),1.5));p.drawLine(3,6,13,6);p.drawLine(3,10,13,10);}
        else if(name=="fixed"){p.setPen(QPen(QColor("#ce514a"),1.2));p.drawRect(3,7,10,7);p.drawArc(QRectF(5,1,6,10),0,180*16);}
        else if(name=="pan"){p.drawLine(8,1,8,15);p.drawLine(1,8,15,8);p.drawLine(8,1,5,4);p.drawLine(8,1,11,4);p.drawLine(1,8,4,5);p.drawLine(1,8,4,11);}
        else if(name=="zoom"){p.drawEllipse(QRectF(1,1,9,9));p.drawLine(9,9,15,15);}
        else {p.drawRect(2,3,11,10);p.setPen(QPen(QColor("#168bc1"),1.6));p.drawLine(5,11,14,2);}
        return QIcon(pix);
    }
    void placeAt(QPoint anchor) {
        auto* screen=QGuiApplication::screenAt(anchor);if(!screen)screen=this->screen();
        const QRect available=screen->availableGeometry();
        QPoint pos=anchor-QPoint(width()/2,84);
        pos.setX(qBound(available.left(),pos.x(),qMax(available.left(),available.right()-width()+1)));
        pos.setY(qBound(available.top(),pos.y(),qMax(available.top(),available.bottom()-height()+1)));
        move(pos);
    }
    void layoutButtons() {
        for(int i=0;i<8;++i){auto* b=buttons_[i];if(b->isHidden())continue;
            const int w=qMax(64,b->fontMetrics().horizontalAdvance(b->text())+30);
            const int x=(i==0||i==4) ? (350-w)/2 : (i<4 ? 350-w-2 : 2);
            const int y=i==0 ? 2 : i==4 ? 140 : (i==1||i==7 ? 30 : i==2||i==6 ? 70 : 110);
            b->setGeometry(x,y,w,24);
        }
    }
    void entry(int index,const QString& title,const QString& id,const QString& command,const QJsonObject& args={}) {
        auto* b=buttons_[index];b->show();b->setText(title);b->setObjectName(id);b->setProperty("commandId",command);b->setProperty("commandArguments",args);b->setAccessibleName(title);b->setToolTip(title);b->setIcon(icon(id));
        QObject::disconnect(b,nullptr,this,nullptr);
        connect(b,&QToolButton::clicked,this,[this,command,args,id]{invoke(id,command,args);});
    }
    void invoke(const QString& id,const QString& command,const QJsonObject& args={}) {
        setProperty("radialResult",id);auto run=run_;close();QTimer::singleShot(0,parentWidget(),[run,command,args]{run(command,args);});
    }
    void clearButtons(){for(auto* b:buttons_){QObject::disconnect(b,nullptr,this,nullptr);b->hide();}}
    void firstLevel(){
        back_->hide();second_=false;setProperty("radialLevel","design");clearButtons();
        entry(2,"Redo","context.radial.redo","history.redo");
        entry(6,"Undo","context.radial.undo","history.undo");
        if(selected_)entry(7,"Delete","context.radial.delete","sketch.deleteSelection");
        entry(4,"Sketch","context.radial.sketch",{});
        QObject::disconnect(buttons_[4],nullptr,this,nullptr);
        connect(buttons_[4],&QToolButton::clicked,this,[this]{sketchLevel();});
        layoutButtons();
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
        layoutButtons();
    }
    void paintEvent(QPaintEvent* event) override {
        // Keep the radial field transparent while retaining a solid context list.
        QRect list;
        for(auto* action:actions())if(action->isVisible())list=list.united(actionGeometry(action));
        list.adjust(-1,-2,1,2);
        QPainter painter(this);painter.fillRect(list,QColor("#fafafa"));
        painter.setPen(QColor("#b8bdc2"));painter.drawRect(list.adjusted(0,0,-1,-1));painter.end();
        QMenu::paintEvent(event);
    }
    bool traverse(int delta) {
        QList<QWidget*> controls;
        for(auto* b:buttons_)if(!b->isHidden())controls.append(b);
        if(!back_->isHidden())controls.append(back_);controls.append(center_);
        QList<QAction*> rows;
        for(auto* a:actions())if(a->isVisible()&&a->isEnabled()&&!a->isSeparator()&&!qobject_cast<QWidgetAction*>(a))rows.append(a);
        int current=controls.indexOf(QApplication::focusWidget());
        if(current<0&&rows.contains(activeAction()))current=controls.size()+rows.indexOf(activeAction());
        const int count=controls.size()+rows.size();
        const int next=current<0 ? (delta>0 ? 0 : count-1) : (current+delta+count)%count;
        if(next<controls.size()){setActiveAction(nullptr);controls[next]->setFocus(Qt::TabFocusReason);}
        else {setFocus(Qt::TabFocusReason);setActiveAction(rows[next-controls.size()]);}
        return true;
    }
    bool eventFilter(QObject* watched,QEvent* event) override {
        if(!isVisible())return QMenu::eventFilter(watched,event);
        if(event->type()==QEvent::KeyPress) {
            auto* key=static_cast<QKeyEvent*>(event);
            if(key->key()==Qt::Key_Escape){close();return true;}
            if(key->key()==Qt::Key_Tab||key->key()==Qt::Key_Backtab){return traverse(key->key()==Qt::Key_Backtab||key->modifiers().testFlag(Qt::ShiftModifier) ? -1 : 1);}
            if(panel_->isAncestorOf(QApplication::focusWidget())) {
                if(key->key()==Qt::Key_Left||key->key()==Qt::Key_Up)return traverse(-1);
                if(key->key()==Qt::Key_Right||key->key()==Qt::Key_Down)return traverse(1);
                if(key->key()==Qt::Key_Return||key->key()==Qt::Key_Enter||key->key()==Qt::Key_Space){
                    if(auto* b=qobject_cast<QToolButton*>(QApplication::focusWidget())){const bool submenu=b==buttons_[4]&&!second_;const bool back=b==back_;b->click();if(submenu||back){setFocus();traverse(1);}return true;}
                }
            }
        }
        if(event->type()==QEvent::MouseMove || event->type()==QEvent::MouseButtonRelease){
            auto* mouse=static_cast<QMouseEvent*>(event);
            if(mouse->buttons().testFlag(Qt::RightButton) || mouse->button()==Qt::RightButton){
                const QPoint global=mouse->globalPosition().toPoint();
                if(!second_ && global.y()-origin_.y()>=45 && std::abs(global.x()-origin_.x())<55){
                    gesturing_=true;secondOrigin_=global;sketchLevel();placeAt(global);
                }
                if(gesturing_) {
                    const auto d=global-secondOrigin_;
                    const int sector=(int(std::lround(std::atan2(double(d.x()),double(-d.y()))/(3.141592653589793/4)))+8)%8;
                    for(int i=0;i<8;++i){buttons_[i]->setProperty("gestureActive",i==sector&&std::hypot(d.x(),d.y())>=28);buttons_[i]->style()->unpolish(buttons_[i]);buttons_[i]->style()->polish(buttons_[i]);}
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
