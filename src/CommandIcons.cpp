// SPDX-License-Identifier: MIT
#include "CommandIcons.h"
#include <QIconEngine>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <cmath>

namespace {
class CommandIconEngine final : public QIconEngine {
public:
    explicit CommandIconEngine(QString command) : command_(std::move(command)) {}
    QIconEngine* clone() const override { return new CommandIconEngine(command_); }
    void paint(QPainter* painter, const QRect& rect, QIcon::Mode mode, QIcon::State) override {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        const qreal scale = qMin(rect.width(), rect.height()) / 32.0;
        painter->translate(rect.center().x() - 16 * scale, rect.center().y() - 16 * scale);
        painter->scale(scale, scale);
        auto color = [mode](const char* name) {
            QColor c(name);
            if (mode == QIcon::Disabled) {
                const int gray = qGray(c.rgb());
                c = QColor((gray + 180) / 2, (gray + 188) / 2, (gray + 194) / 2);
            }
            return c;
        };
        const auto edge = color("#536b78"), blue = color("#55b6e5");
        const auto pale = color("#dce8ee"), side = color("#9baeb9");
        const auto accent = color("#d98230"), white = color("#f7f8f9");
        auto pen = [&](QColor c, qreal width = 1.25) {
            painter->setPen(QPen(c, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        };
        auto polygon = [&](std::initializer_list<QPointF> points, QColor fill) {
            QPolygonF p; for (const auto& point : points) p << point;
            painter->setBrush(fill); pen(edge); painter->drawPolygon(p);
        };
        auto arrow = [&](QPointF from, QPointF to, QColor c) {
            pen(c, 1.8); painter->drawLine(from, to);
            QPointF direction = to - from;
            const double length = std::hypot(direction.x(), direction.y());
            if (length < .01) return;
            direction /= length; const QPointF normal(-direction.y(), direction.x());
            painter->setBrush(c);
            painter->drawPolygon(QPolygonF{to, to - direction * 4 + normal * 2,
                                            to - direction * 4 - normal * 2});
        };
        auto cube = [&](bool hollow = false) {
            polygon({{5,10},{16,4},{27,10},{16,16}}, blue);
            polygon({{5,10},{16,16},{16,28},{5,22}}, pale);
            polygon({{16,16},{27,10},{27,22},{16,28}}, side);
            if (hollow) polygon({{9,10},{16,6.5},{23,10},{16,13.5}}, white);
        };
        if (command_ == "primitive.box") {
            cube();
        } else if (command_ == "primitive.cylinder") {
            pen(edge); painter->setBrush(side); painter->drawEllipse(QRectF(6,20,20,8));
            painter->setPen(Qt::NoPen); painter->drawRect(QRectF(6,8,20,16));
            pen(edge); painter->drawLine(QPointF(6,8),QPointF(6,24));
            painter->drawLine(QPointF(26,8),QPointF(26,24));
            painter->setBrush(blue); painter->drawEllipse(QRectF(6,4,20,8));
        } else if (command_ == "primitive.cone") {
            pen(edge); painter->setBrush(side); painter->drawEllipse(QRectF(4,21,24,8));
            QPainterPath flank; flank.moveTo(4,25); flank.lineTo(14,5); flank.lineTo(18,5);
            flank.lineTo(28,25); flank.quadTo(16,31,4,25);
            painter->setBrush(pale); pen(edge); painter->drawPath(flank);
            painter->setBrush(blue); painter->drawEllipse(QRectF(14,4,4,3));
        } else if (command_ == "primitive.torus") {
            QPainterPath ring; ring.setFillRule(Qt::OddEvenFill);
            ring.addEllipse(QRectF(3,7,26,19)); ring.addEllipse(QRectF(10,12,12,8));
            pen(edge); painter->setBrush(blue); painter->drawPath(ring);
            painter->setBrush(Qt::NoBrush); pen(side,1.1);
            painter->drawArc(QRectF(4,10,24,17),195*16,150*16);
        } else if (command_ == "solid.sphereDialog") {
            pen(edge); painter->setBrush(blue); painter->drawEllipse(QRectF(4,4,24,24));
            painter->setBrush(Qt::NoBrush); painter->drawEllipse(QRectF(11,4,10,24));
            pen(white); painter->drawArc(QRectF(4,9,24,13),180*16,180*16);
        } else if (command_ == "sketch.create") {
            polygon({{3,12},{19,5},{28,19},{12,27}}, pale);
            painter->setPen(QPen(edge,1.1,Qt::DashLine));
            painter->drawLine(QPointF(8,14),QPointF(19,9));
            painter->drawLine(QPointF(8,14),QPointF(14,23));
            polygon({{15,20},{25,6},{29,9},{19,23}}, accent);
            polygon({{15,20},{19,23},{14,25}}, white);
        } else if (command_ == "solid.holeDialog") {
            polygon({{3,12},{20,5},{29,16},{12,25}},pale);
            pen(edge);painter->setBrush(side);painter->drawEllipse(QRectF(9,10,13,8));
            painter->setBrush(white);painter->drawEllipse(QRectF(12,12,7,4));
            arrow({16,2},{16,9},accent);
        } else if (command_ == "solid.extrudeDialog") {
            painter->save(); painter->translate(-2,3); painter->scale(.88,.88); cube(); painter->restore();
            arrow({27,21},{27,2},edge);
        } else if (command_ == "solid.thickenDialog") {
            QPainterPath sheet;sheet.moveTo(3,19);sheet.cubicTo(10,8,21,9,29,17);
            pen(edge,1.2);painter->setBrush(Qt::NoBrush);painter->drawPath(sheet);
            painter->save();painter->translate(0,5);pen(side,2.2);painter->drawPath(sheet);painter->restore();
            pen(blue,2.5);painter->drawPath(sheet);arrow({16,25},{16,15},accent);
        } else if (command_ == "solid.revolveDialog") {
            painter->setPen(QPen(edge,1.1,Qt::DashDotLine)); painter->drawLine(QPointF(8,3),QPointF(8,28));
            polygon({{11,7},{17,7},{17,12},{21,12},{21,24},{11,24}},blue);
            painter->setBrush(Qt::NoBrush); pen(accent,2); painter->drawArc(QRectF(2,13,27,14),-20*16,235*16);
            arrow({5,22},{3,17},accent);
        } else if (command_ == "solid.sweepDialog") {
            painter->setBrush(Qt::NoBrush);painter->setPen(QPen(edge,6,Qt::SolidLine,Qt::RoundCap));QPainterPath path;path.moveTo(5,26);path.lineTo(5,13);path.cubicTo(5,2,26,2,26,13);path.lineTo(26,25);painter->drawPath(path);painter->setPen(QPen(blue,4,Qt::SolidLine,Qt::RoundCap));painter->drawPath(path);painter->setBrush(white);pen(edge);painter->drawEllipse(QRectF(2,24,6,4));arrow({12,27},{23,27},accent);
        } else if ((command_ == "solid.loftDialog" || command_ == "surface.loftDialog")) {
            polygon({{4,24},{12,7},{22,7},{28,24}},pale);
            pen(edge); painter->setBrush(blue); painter->drawEllipse(QRectF(4,20,24,8));
            painter->drawEllipse(QRectF(12,4,10,6));
            painter->setBrush(Qt::NoBrush); painter->drawLine(QPointF(12,7),QPointF(4,24));
            painter->drawLine(QPointF(22,7),QPointF(28,24));
        } else if (command_ == "solid.circularPatternDialog") {
            for(int i=0;i<6;++i){painter->save();painter->translate(16,16);painter->rotate(i*60);painter->setBrush(i==0?blue:pale);pen(edge);painter->drawRect(QRectF(7,-3,7,6));painter->restore();}
            painter->setBrush(accent);pen(edge);painter->drawEllipse(QRectF(14,14,4,4));
        } else if (command_ == "solid.rectangularPatternDialog") {
            for(int row=0;row<2;++row)for(int col=0;col<3;++col){
                painter->setBrush(row==0&&col==0?blue:pale);pen(edge);
                painter->drawRect(QRectF(4+col*9,6+row*11,6,7));
            }
        } else if (command_ == "file.stepImport") {
            polygon({{5,4},{20,4},{27,11},{27,28},{5,28}},pale);
            pen(edge);painter->drawLine(QPointF(20,4),QPointF(20,11));painter->drawLine(QPointF(20,11),QPointF(27,11));
            arrow({1,18},{18,18},blue);
        } else if (command_ == "assembly.pin") {
            polygon({{11,5},{21,5},{24,15},{8,15}},blue);
            pen(edge,1.5);painter->drawLine(QPointF(16,15),QPointF(16,28));
            painter->setBrush(accent);painter->drawEllipse(QRectF(13,25,6,5));
        } else if (command_ == "solid.mirrorDialog") {
            polygon({{2,10},{10,7},{10,23},{2,20}},pale);
            polygon({{22,7},{30,10},{30,20},{22,23}},blue);
            painter->setPen(QPen(edge,1.3,Qt::DashDotLine)); painter->drawLine(QPointF(16,3),QPointF(16,29));
            arrow({10,27},{22,27},accent);
        } else if (command_ == "solid.pressPullDialog") {
            polygon({{3,22},{15,16},{29,22},{17,29}},pale);
            polygon({{9,17},{16,13},{23,17},{16,21}},blue);
            arrow({16,17},{16,2},edge);
        } else if (command_ == "solid.combineDialog") {
            painter->save(); painter->translate(-2,-2); painter->scale(.72,.72); cube(); painter->restore();
            painter->save(); painter->translate(10,10); painter->scale(.72,.72); cube(); painter->restore();
            pen(accent,2.3); painter->drawLine(QPointF(5,21),QPointF(5,29));
            painter->drawLine(QPointF(1,25),QPointF(9,25));
        } else if (command_ == "solid.filletDialog" || command_ == "solid.chamferDialog") {
            const bool fillet = command_ == "solid.filletDialog";
            QPainterPath shape; shape.moveTo(5,27); shape.lineTo(5,5); shape.lineTo(16,5);
            if (fillet) shape.quadTo(27,5,27,16); else shape.lineTo(27,16);
            shape.lineTo(27,27); shape.closeSubpath();
            pen(edge); painter->setBrush(pale); painter->drawPath(shape);
            QPainterPath corner; corner.moveTo(16,5);
            if (fillet) corner.quadTo(27,5,27,16); else corner.lineTo(27,16);
            painter->setBrush(Qt::NoBrush); pen(blue,4); painter->drawPath(corner);
            pen(edge,1); painter->drawLine(QPointF(9,23),QPointF(23,23));
        } else if (command_ == "solid.shellDialog") {
            cube(true);
        } else if (command_ == "solid.moveDialog") {
            arrow({16,16},{16,2},blue); arrow({16,16},{16,30},blue);
            arrow({16,16},{2,16},edge); arrow({16,16},{30,16},edge);
            polygon({{12,16},{16,12},{20,16},{16,20}},pale);
        } else if (command_ == "construction.plane") {
            polygon({{5,12},{19,5},{28,17},{14,25}},pale);
            painter->setPen(QPen(edge,1,Qt::DashLine)); painter->drawLine(QPointF(3,26),QPointF(29,12));
            polygon({{5,7},{19,1},{28,12},{14,19}},QColor(blue.red(),blue.green(),blue.blue(),185));
            arrow({7,24},{7,14},accent);
        } else if (command_ == "inspect.measure") {
            polygon({{3,12},{27,7},{29,18},{5,23}},pale);
            pen(edge); for(int i=0;i<6;++i) {
                const qreal x=7+i*3.5,y=11.2-i*.73;
                painter->drawLine(QPointF(x,y),QPointF(x+1,y+(i%2?4:6)));
            }
        } else {
            polygon({{4,9},{23,9},{28,15},{23,26},{4,26}},pale);
            pen(blue,2); painter->drawLine(QPointF(9,16),QPointF(23,16));
        }
        painter->restore();
    }
    QPixmap pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state) override {
        QPixmap result(size); result.fill(Qt::transparent); QPainter painter(&result);
        paint(&painter,QRect(QPoint(),size),mode,state); return result;
    }
private:
    QString command_;
};
}
QIcon commandIcon(const QString& command) { return QIcon(new CommandIconEngine(command)); }
