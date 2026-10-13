// SPDX-License-Identifier: MIT
#include "SketchEllipse.h"
#include <QJsonArray>
#include <QMap>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {
void require(bool condition,const char* message){if(!condition)throw std::invalid_argument(message);}
double number(const QJsonObject& object,const char* key){
    const auto value=object.value(QLatin1String(key));
    require(value.isDouble() && std::isfinite(value.toDouble()),"Ellipse coordinate must be finite");
    return value.toDouble();
}
}

namespace SketchEllipse {
double secondRadiusFromContour(double cu,double cv,double au,double av,double pu,double pv){
    const double dx=au-cu,dy=av-cv,a=std::hypot(dx,dy);
    require(std::isfinite(a) && a>=0.001 && a<=100000,"Ellipse first axis must have positive usable length");
    const double ux=dx/a,uy=dy/a,x=(pu-cu)*ux+(pv-cv)*uy,y=-(pu-cu)*uy+(pv-cv)*ux;
    const double radicand=1-(x/a)*(x/a);
    require(std::isfinite(x) && std::isfinite(y) && radicand>1e-12 && std::abs(y)>=0.001,
        "Ellipse contour pick does not determine a positive second axis");
    const double b=std::abs(y)/std::sqrt(radicand);
    require(std::isfinite(b) && b>=0.001 && b<=100000,"Ellipse second axis is outside supported length");
    return std::copysign(b,y);
}
Axes axes(const QJsonObject& parameters,const QJsonObject& ellipse){
    QMap<QString,QJsonObject> points,lines;
    for(const auto& value:parameters.value("points").toArray()){
        const auto point=value.toObject();points[point.value("id").toString()]=point;
    }
    for(const auto& value:parameters.value("entities").toArray()){
        const auto entity=value.toObject();if(entity.value("type")=="line")lines[entity.value("id").toString()]=entity;
    }
    const auto centerId=ellipse.value("centerPointId").toString();
    const auto firstId=ellipse.value("firstAxisId").toString(),secondId=ellipse.value("secondAxisId").toString();
    require(points.contains(centerId) && lines.contains(firstId) && lines.contains(secondId) && firstId!=secondId,
        "Ellipse lost retained center or axis");
    const auto center=points[centerId],first=lines[firstId],second=lines[secondId];
    require(first.value("construction").toBool() && second.value("construction").toBool(),
        "Ellipse axes must remain construction geometry");
    auto endpoint=[&](const QJsonObject& line,const char* key){
        const auto id=line.value(QLatin1String(key)).toString();
        require(points.contains(id),"Ellipse lost a retained axis endpoint");return points[id];
    };
    const auto a0=endpoint(first,"startPointId"),a1=endpoint(first,"endPointId");
    const auto b0=endpoint(second,"startPointId"),b1=endpoint(second,"endPointId");
    const double cu=number(center,"uMm"),cv=number(center,"vMm");
    const double ax=number(a1,"uMm")-cu,ay=number(a1,"vMm")-cv;
    const double bx=number(b1,"uMm")-cu,by=number(b1,"vMm")-cv;
    const double a=std::hypot(ax,ay),b=std::hypot(bx,by);
    require(a>=0.001 && b>=0.001 && a<=100000 && b<=100000,"Ellipse axis collapsed or exceeded supported length");
    const double tolerance=std::max(1e-6,1e-8*std::max(a,b));
    require(std::hypot(number(a0,"uMm")+number(a1,"uMm")-2*cu,
        number(a0,"vMm")+number(a1,"vMm")-2*cv)<=tolerance &&
        std::hypot(number(b0,"uMm")+number(b1,"uMm")-2*cu,
        number(b0,"vMm")+number(b1,"vMm")-2*cv)<=tolerance,
        "Ellipse center left its retained full axes");
    require(std::abs(ax*bx+ay*by)/(a*b)<=1e-8,"Ellipse axes must remain perpendicular");
    return {cu,cv,ax/a,ay/a,bx/b,by/b,a,b};
}
}
