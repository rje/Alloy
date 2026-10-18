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
        auto arcs=SketchGeometry::newFeature(document.authored()["documentId"].toString(),"XY");
        arcs=SketchGeometry::addArc(arcs,{{"centerUMm",-20},{"centerVMm",0},{"radiusMm",10},{"startAngleDeg",90},{"sweepAngleDeg",180}});
        const auto arcA=arcs["parameters"].toObject()["entities"].toArray().last().toObject();
        arcs=SketchGeometry::addArc(arcs,{{"centerUMm",21},{"centerVMm",1},{"radiusMm",10},{"startAngleDeg",270},{"sweepAngleDeg",180}});
        const auto arcB=arcs["parameters"].toObject()["entities"].toArray().last().toObject();
        arcs=SketchGeometry::addLine(arcs,{{"u1Mm",0},{"v1Mm",-20},{"u2Mm",0},{"v2Mm",20}});
        const auto arcAxis=arcs["parameters"].toObject()["entities"].toArray().last().toObject();
        auto built=SketchGeometry::setConstruction(arcs,{{"entityIds",QJsonArray{arcAxis["id"]}},{"construction",true}});
        check(built.ok,"construction axis");arcs=built.feature;
        auto arcRelation=SketchGeometry::addConstraint(arcs,{{"type","symmetry"},{"entityIds",QJsonArray{arcA["id"],arcB["id"],arcAxis["id"]}}});
        check(arcRelation.ok,"arc symmetry: "+arcRelation.error);arcs=arcRelation.feature;
        const auto arcRetained=arcs["parameters"].toObject()["constraints"].toArray().last().toObject();
        check(arcRetained["pointIds"].toArray()==QJsonArray{arcA["startPointId"],arcB["endPointId"],arcA["endPointId"],arcB["startPointId"],arcA["centerPointId"],arcB["centerPointId"]},"arc endpoint and center correspondence");
        const auto arcMove=SketchGeometry::movePoint(arcs,{{"pointId",arcA["centerPointId"]},{"uMm",-24},{"vMm",3}});
        check(arcMove.ok,"arc source edit: "+arcMove.error);arcs=arcMove.feature;
        const auto aa=point(arcs,arcAxis["startPointId"].toString()),ab=point(arcs,arcAxis["endPointId"].toString());
        const double ax=aa["uMm"].toDouble(),ay=aa["vMm"].toDouble(),vx=ab["uMm"].toDouble()-ax,vy=ab["vMm"].toDouble()-ay;
        const auto pairs=arcRetained["pointIds"].toArray();
        for(int i=0;i<6;i+=2){const auto a=point(arcs,pairs[i].toString()),b=point(arcs,pairs[i+1].toString());
            const double x=a["uMm"].toDouble(),y=a["vMm"].toDouble(),t=((x-ax)*vx+(y-ay)*vy)/(vx*vx+vy*vy);
            check(std::hypot(b["uMm"].toDouble()-(2*(ax+t*vx)-x),b["vMm"].toDouble()-(2*(ay+t*vy)-y))<1e-5,"all arc point pairs reflect after move");}
        const auto mixed=SketchGeometry::addConstraint(arcs,{{"type","symmetry"},{"entityIds",QJsonArray{arcA["id"],arcAxis["id"],arcB["id"]}}});
        check(!mixed.ok && mixed.feature==arcs,"mixed pairing refuses atomically");
        auto branchCase=[&](double start,double sweep){
            auto candidate=SketchGeometry::newFeature(document.authored()["documentId"].toString(),"XY");
            candidate=SketchGeometry::addArc(candidate,{{"centerUMm",-20},{"centerVMm",0},{"radiusMm",10},{"startAngleDeg",90},{"sweepAngleDeg",180}});
            const auto first=candidate["parameters"].toObject()["entities"].toArray().last().toObject();
            candidate=SketchGeometry::addArc(candidate,{{"centerUMm",20},{"centerVMm",0},{"radiusMm",10},{"startAngleDeg",start},{"sweepAngleDeg",sweep}});
            const auto second=candidate["parameters"].toObject()["entities"].toArray().last().toObject();
            candidate=SketchGeometry::addLine(candidate,{{"u1Mm",0},{"v1Mm",-20},{"u2Mm",0},{"v2Mm",20}});
            const auto axisId=candidate["parameters"].toObject()["entities"].toArray().last().toObject()["id"].toString();
            const auto construction=SketchGeometry::setConstruction(candidate,{{"entityIds",QJsonArray{axisId}},{"construction",true}});
            check(construction.ok,"branch construction");candidate=construction.feature;
            const auto result=SketchGeometry::addConstraint(candidate,{{"type","symmetry"},{"entityIds",QJsonArray{first["id"],second["id"],axisId}}});
            if(!result.ok)check(result.feature==candidate,"branch refusal atomic");return result;
        };
        check(branchCase(90,-180).ok,"opposite sweep uses forward endpoint mapping");
        const auto incompatible=branchCase(90,180);check(!incompatible.ok && incompatible.error.contains("incompatible"),"incompatible sweep branch refused");
        const auto ambiguous=branchCase(0,180);check(!ambiguous.ok && ambiguous.error.contains("ambiguous"),"equidistant endpoint branches refused");
        std::cout<<"sketch symmetry: line and arc pairing, source edit, refusal, history, reopen passed\n";
        return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
