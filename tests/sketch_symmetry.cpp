// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void check(bool ok,const QString& message){if(!ok)throw std::runtime_error(message.toStdString());}
static QJsonObject point(const QJsonObject& sketch,const QString& id){
    for(const auto& value:sketch["parameters"].toObject()["points"].toArray())if(value.toObject()["id"]==id)return value.toObject();return {};
}
int main(int argc,char** argv){
    QCoreApplication app(argc,argv);
    try{
        CadDocument document;auto sketch=SketchGeometry::newFeature(document.authored()["documentId"].toString(),"XY");
        auto parameters=sketch["parameters"].toObject();QJsonArray points,entities;
        auto addPoint=[&](double u,double v){const auto id=CadDocument::newId();points.append(QJsonObject{{"id",id},{"uMm",u},{"vMm",v}});return id;};
        auto addLine=[&](const QString& a,const QString& b,bool construction){const auto id=CadDocument::newId();entities.append(QJsonObject{{"id",id},{"type","line"},{"startPointId",a},{"endPointId",b},{"construction",construction}});return id;};
        const auto leftBottom=addPoint(-30,0),leftTop=addPoint(-30,30),rightTop=addPoint(30,30),rightBottom=addPoint(30,0);
        const auto axisBottom=addPoint(0,-5),axisTop=addPoint(0,35);
        const auto left=addLine(leftBottom,leftTop,false),right=addLine(rightTop,rightBottom,false),axis=addLine(axisBottom,axisTop,true);
        parameters["points"]=points;parameters["entities"]=entities;sketch["parameters"]=parameters;
        auto added=SketchGeometry::addConstraint(sketch,{{"type","symmetry"},{"entityIds",QJsonArray{left,right,axis}}});
        check(added.ok,"symmetry solve: "+added.error);sketch=added.feature;
        const auto retained=sketch["parameters"].toObject()["constraints"].toArray().last().toObject();
        check(retained["pointIds"].toArray()==QJsonArray{leftBottom,rightBottom,leftTop,rightTop},"reversed line endpoint pairing retained");
        auto moved=SketchGeometry::movePoint(sketch,{{"pointId",leftBottom},{"uMm",-35},{"vMm",0}});
        check(moved.ok,"symmetry source edit: "+moved.error);sketch=moved.feature;
        const auto source=point(sketch,leftBottom),opposite=point(sketch,rightBottom),axisA=point(sketch,axisBottom),axisB=point(sketch,axisTop);
        const double dx=axisB["uMm"].toDouble()-axisA["uMm"].toDouble(),dy=axisB["vMm"].toDouble()-axisA["vMm"].toDouble();
        const double su=source["uMm"].toDouble(),sv=source["vMm"].toDouble(),au=axisA["uMm"].toDouble(),av=axisA["vMm"].toDouble();
        const double along=((su-au)*dx+(sv-av)*dy)/(dx*dx+dy*dy);
        check(std::hypot(opposite["uMm"].toDouble()-(2*(au+along*dx)-su),opposite["vMm"].toDouble()-(2*(av+along*dy)-sv))<1e-5,
            "opposite endpoint follows retained reflection");
        auto tampered=sketch;auto p=tampered["parameters"].toObject();auto c=p["constraints"].toArray();auto relation=c.last().toObject();relation["pointIds"]=QJsonArray{leftBottom,rightTop,leftTop,rightBottom};c[c.size()-1]=relation;p["constraints"]=c;tampered["parameters"]=p;
        check(!SketchGeometry::solveDraft(tampered).ok,"wrong retained endpoint permutation refused");
        const auto injected=SketchGeometry::addConstraint(sketch,{{"type","symmetry"},{"entityIds",QJsonArray{left,right,axis}},{"pointIds",QJsonArray{leftBottom,rightTop,leftTop,rightBottom}}});
        check(!injected.ok && injected.feature==sketch,"caller cannot override solved pairing");
        check(document.commit(SketchGeometry::replaceFeature(document.authored(),sketch),"Sketch Symmetry").isEmpty(),"commit");
        const auto authored=document.authored();document.history().undo();check(document.sketches().empty(),"undo");document.history().redo();check(document.authored()==authored,"redo");
        QTemporaryDir dir;check(dir.isValid(),"temporary directory");const auto path=dir.path()+"/symmetry.alloy";
        check(document.save(path).isEmpty(),"save");CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==authored,"reopen");
        std::cout<<"sketch symmetry: paired endpoints, edit, refusal, history, reopen passed\n";
        return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
