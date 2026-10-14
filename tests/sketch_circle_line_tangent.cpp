// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void require(bool value,const QString& message){if(!value)throw std::runtime_error(message.toStdString());}
static QJsonObject lastEntity(const QJsonObject& sketch){return sketch["parameters"].toObject()["entities"].toArray().last().toObject();}
static void checkContact(const QJsonObject& sketch,const QJsonObject& circle,const QJsonObject& line,int side){
    QMap<QString,QJsonObject> points;
    for(const auto& value:sketch["parameters"].toObject()["points"].toArray()){
        const auto point=value.toObject();points[point["id"].toString()]=point;
    }
    const auto a=points[line["startPointId"].toString()],b=points[line["endPointId"].toString()];
    const auto center=points[circle["centerPointId"].toString()];
    const double dx=b["uMm"].toDouble()-a["uMm"].toDouble(),dy=b["vMm"].toDouble()-a["vMm"].toDouble();
    const double px=center["uMm"].toDouble()-a["uMm"].toDouble(),py=center["vMm"].toDouble()-a["vMm"].toDouble();
    const double length=std::hypot(dx,dy),radius=[&]{for(const auto& value:sketch["parameters"].toObject()["entities"].toArray())
        if(value.toObject()["id"]==circle["id"])return value.toObject()["radiusMm"].toDouble();return 0.0;}();
    require(std::abs((dx*py-dy*px)/length-side*radius)<1e-6,"signed tangent distance");
    const double foot=(px*dx+py*dy)/(length*length);
    require(foot>=-1e-8 && foot<=1+1e-8,"contact lies on finite line");
}
int main(int argc,char** argv){
    QCoreApplication app(argc,argv);
    try{
        CadDocument document;
        auto sketch=SketchGeometry::newFeature(document.authored()["documentId"].toString(),"XY");
        sketch=SketchGeometry::addLine(sketch,{{"u1Mm",-20},{"v1Mm",0},{"u2Mm",20},{"v2Mm",0}});
        const auto line=lastEntity(sketch);
        sketch=SketchGeometry::addCircle(sketch,{{"centerUMm",0},{"centerVMm",8},{"diameterMm",10}});
        const auto circle=lastEntity(sketch);
        auto result=SketchGeometry::addConstraint(sketch,{{"type","tangent"},{"entityIds",QJsonArray{line["id"],circle["id"]}}});
        require(result.ok,"circle-line Tangent: "+result.error);sketch=result.feature;
        const auto relation=sketch["parameters"].toObject()["constraints"].toArray().last().toObject();
        require(relation["type"]=="tangent" && relation["entityIds"]==QJsonArray{circle["id"],line["id"]}
            && relation["side"]==1,"existing circle-line record retained in canonical order and side");
        checkContact(sketch,circle,line,1);
        auto duplicate=SketchGeometry::addConstraint(sketch,{{"type","tangent"},{"entityIds",QJsonArray{circle["id"],line["id"]}}});
        require(!duplicate.ok && duplicate.feature==sketch,"duplicate refuses atomically");
        auto onLine=SketchGeometry::newFeature(document.authored()["documentId"].toString(),"XY");
        onLine=SketchGeometry::addLine(onLine,{{"u1Mm",-20},{"v1Mm",0},{"u2Mm",20},{"v2Mm",0}});
        const auto axis=lastEntity(onLine);
        onLine=SketchGeometry::addCircle(onLine,{{"centerUMm",0},{"centerVMm",0},{"diameterMm",10}});
        auto ambiguous=SketchGeometry::addConstraint(onLine,{{"type","tangent"},{"entityIds",QJsonArray{axis["id"],lastEntity(onLine)["id"]}}});
        require(!ambiguous.ok && ambiguous.feature==onLine,"center-on-line side ambiguity refuses atomically");
        require(document.commit(SketchGeometry::replaceFeature(document.authored(),sketch),"Circle-line Tangent").isEmpty(),"commit");
        const auto authored=document.authored();document.history().undo();require(document.sketches().empty(),"undo");
        document.history().redo();require(document.authored()==authored,"redo");
        QTemporaryDir dir;require(dir.isValid(),"temporary directory");const auto path=dir.path()+"/circle-tangent.alloy";
        require(document.save(path).isEmpty(),"save");CadDocument reopened;
        require(reopened.open(path).isEmpty() && reopened.authored()==authored,"reopen");
        std::cout<<"sketch circle-line Tangent: contact, side, duplicate, ambiguity, history, reopen passed\n";
        return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
