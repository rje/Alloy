// SPDX-License-Identifier: MIT
#include "ReferenceView.h"
#include "SketchConic.h"
#include <AIS_Shape.hxx>
#include <AIS_Point.hxx>
#include <Geom_CartesianPoint.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <Prs3d_PointAspect.hxx>
#include <Graphic3d_ZLayerId.hxx>
#include <QJsonArray>
#include <QMap>
#include <QLineF>
#include <cmath>

void ReferenceView::decorateConicHandle(const QJsonObject& feature,const QJsonObject& selection) {
    conicHandleState_={};if(context_.IsNull())return;
    const auto parameters=feature["parameters"].toObject();const auto plane=parameters["plane"].toString();
    QMap<QString,QPointF> points;
    for(const auto& value:parameters["points"].toArray()) {
        const auto point=value.toObject();points[point["id"].toString()]={point["uMm"].toDouble(),point["vMm"].toDouble()};
    }
    for(const auto& value:parameters["entities"].toArray()) {
        const auto entity=value.toObject();
        if(entity["type"]!="conic" || !selection["entityIds"].toArray().contains(entity["id"]))continue;
        const auto a=points[entity["startPointId"].toString()],b=points[entity["endPointId"].toString()];
        const auto vertex=points[entity["vertexPointId"].toString()],middle=(a+b)/2;
        const auto shoulder=middle+(vertex-middle)*entity["rho"].toDouble();
        const auto color=Quantity_Color(0.03,0.40,0.18,Quantity_TOC_RGB);
        auto line=[&](QPointF from,QPointF to,double width) {
            if(QLineF(from,to).length()<1e-8)return;
            Handle(AIS_Shape) shape=new AIS_Shape(BRepBuilderAPI_MakeEdge(sketchWorld(plane,from.x(),from.y()),sketchWorld(plane,to.x(),to.y())).Edge());
            shape->SetColor(color);shape->SetWidth(width*devicePixelRatioF());shape->SetZLayer(Graphic3d_ZLayerId_Topmost);
            context_->Display(shape,AIS_WireFrame,-1,false);context_->Deactivate(shape);sketchDecorations_.push_back(shape);
        };
        line(a,vertex,1);line(b,vertex,1);line(middle,shoulder,2);
        const auto direction=(vertex-middle)/QLineF(middle,vertex).length();
        const QPointF normal(-direction.y(),direction.x());
        const double size=QLineF(middle,vertex).length()*.045;
        line(shoulder,shoulder-direction*size+normal*size*.45,2);
        line(shoulder,shoulder-direction*size-normal*size*.45,2);
        Handle(AIS_Point) marker=new AIS_Point(new Geom_CartesianPoint(sketchWorld(plane,shoulder.x(),shoulder.y())));
        marker->Attributes()->SetPointAspect(new Prs3d_PointAspect(Aspect_TOM_O,color,4*devicePixelRatioF()));
        marker->SetZLayer(Graphic3d_ZLayerId_Topmost);context_->Display(marker,0,-1,false);context_->Deactivate(marker);sketchDecorations_.push_back(marker);
        const auto screen=projectSketchPoint(shoulder.x(),shoulder.y(),plane,sketchFrame_);
        conicHandleState_={{"entityId",entity["id"]},{"x",screen["x"]},{"y",screen["y"]},{"rho",entity["rho"]},
                          {"uMm",shoulder.x()},{"vMm",shoulder.y()}};
        break;
    }
}
