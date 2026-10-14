// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool value,const QString& message){if(!value)throw std::runtime_error(message.toStdString());}
static QJsonObject entity(const QJsonObject& sketch){return sketch["parameters"].toObject()["entities"].toArray().last().toObject();}
static QJsonObject addLine(const QJsonObject& sketch,double x1,double y1,double x2,double y2){
    return SketchGeometry::addLine(sketch,{{"u1Mm",x1},{"v1Mm",y1},{"u2Mm",x2},{"v2Mm",y2}});
}
static void checkCollinear(const QJsonObject& sketch,const QJsonObject& first,const QJsonObject& second){
    QMap<QString,QJsonObject> points;
    for(const auto& value:sketch["parameters"].toObject()["points"].toArray()){
        const auto point=value.toObject();points[point["id"].toString()]=point;
    }
    const auto a=points[first["startPointId"].toString()],b=points[first["endPointId"].toString()];
    const auto c=points[second["startPointId"].toString()],d=points[second["endPointId"].toString()];
    const double ax=b["uMm"].toDouble()-a["uMm"].toDouble(),ay=b["vMm"].toDouble()-a["vMm"].toDouble();
    const double bx=d["uMm"].toDouble()-c["uMm"].toDouble(),by=d["vMm"].toDouble()-c["vMm"].toDouble();
    const double al=std::hypot(ax,ay),bl=std::hypot(bx,by);
    require(std::abs(ax*by-ay*bx)/(al*bl)<1e-8,"parallel residual");
    require(std::abs(ax*(c["vMm"].toDouble()-a["vMm"].toDouble())-ay*(c["uMm"].toDouble()-a["uMm"].toDouble()))/al<1e-6,
        "support-line distance residual");
}
int main(int argc,char** argv){
    QCoreApplication app(argc,argv);
    try{
        CadDocument document;
        auto separated=SketchGeometry::newFeature(document.authored()["documentId"].toString(),"XY");
        separated=addLine(separated,0,0,20,0);const auto a=entity(separated);
        separated=addLine(separated,30,5,50,6);const auto b=entity(separated);
        require(a["endPointId"]!=b["startPointId"],"separated points retain distinct identities");
        auto solved=SketchGeometry::addConstraint(separated,{{"type","collinear"},{"entityIds",QJsonArray{b["id"],a["id"]}}});
        require(solved.ok,"separated Collinear: "+solved.error);separated=solved.feature;
        checkCollinear(separated,a,b);
        const auto relation=separated["parameters"].toObject()["constraints"].toArray().last().toObject();
        require(relation["entityIds"]==QJsonArray{b["id"],a["id"]},"input order retained");
        auto duplicate=SketchGeometry::addConstraint(separated,{{"type","collinear"},{"entityIds",QJsonArray{a["id"],b["id"]}}});
        require(!duplicate.ok && duplicate.feature==separated,"reversed duplicate refuses atomically");
        auto shared=SketchGeometry::newFeature(document.authored()["documentId"].toString(),"XY");
        shared=addLine(shared,0,0,20,0);const auto first=entity(shared);
        shared=addLine(shared,20,0,40,2);const auto second=entity(shared);
        require(first["endPointId"]==second["startPointId"],"shared contact is one authored point ID");
        solved=SketchGeometry::addConstraint(shared,{{"type","collinear"},{"entityIds",QJsonArray{first["id"],second["id"]}}});
        require(solved.ok,"shared Collinear: "+solved.error);checkCollinear(solved.feature,first,second);
        std::cout<<"sketch Collinear: separated and shared contacts, order, duplicate refusal passed\n";
        return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
