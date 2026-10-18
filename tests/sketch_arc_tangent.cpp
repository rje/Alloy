// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QUuid>
#include <iostream>
#include <stdexcept>

static void require(bool value,const QString& message){if(!value)throw std::runtime_error(message.toStdString());}

int main(int argc,char** argv){
    QCoreApplication app(argc,argv);
    try{
        CadDocument document;
        auto sketch=SketchGeometry::newFeature(document.authored()["documentId"].toString(),"XY");
        auto line=[&](double u1,double v1,double u2,double v2){
            sketch=SketchGeometry::addLine(sketch,{{"u1Mm",u1},{"v1Mm",v1},{"u2Mm",u2},{"v2Mm",v2}});
            return sketch["parameters"].toObject()["entities"].toArray().last().toObject();
        };
        const auto right=line(60,0,60,35);
        sketch=SketchGeometry::addArc(sketch,{{"centerUMm",55},{"centerVMm",35},{"radiusMm",5},
            {"startAngleDeg",0},{"sweepAngleDeg",90}});
        const auto arc=sketch["parameters"].toObject()["entities"].toArray().last().toObject();
        const auto top=line(55,40,0,40);
        require(right["endPointId"]==arc["startPointId"] && arc["endPointId"]==top["startPointId"],
            "arc and lines share authored endpoint IDs");
        auto first=SketchGeometry::addConstraint(sketch,{{"type","tangent"},{"entityIds",QJsonArray{right["id"],arc["id"]}}});
        require(first.ok,"right tangency: "+first.error);sketch=first.feature;
        auto second=SketchGeometry::addConstraint(sketch,{{"type","tangent"},{"entityIds",QJsonArray{arc["id"],top["id"]}}});
        require(second.ok,"top tangency: "+second.error);sketch=second.feature;
        const auto relations=sketch["parameters"].toObject()["constraints"].toArray();
        require(relations.size()==2 && relations[0].toObject()["type"]=="arcTangent" && relations[1].toObject()["type"]=="arcTangent",
            "both retained arc-line relations use the existing evaluator");
        require(relations[0].toObject()["pointIds"].toArray()==QJsonArray{arc["startPointId"]}
            && relations[1].toObject()["pointIds"].toArray()==QJsonArray{arc["endPointId"]},
            "distinct contacts retained");
        const auto unrelated=line(15,15,25,15);
        const auto rejected=SketchGeometry::addConstraint(sketch,{{"type","tangent"},{"entityIds",QJsonArray{arc["id"],unrelated["id"]}}});
        require(!rejected.ok && rejected.feature==sketch,"missing shared endpoint refuses without changing draft");
        const auto injected=SketchGeometry::addConstraint(sketch,{{"type","tangent"},{"entityIds",QJsonArray{arc["id"],top["id"]}},
            {"pointIds",QJsonArray{arc["startPointId"]}}});
        require(!injected.ok && injected.feature==sketch,"caller cannot replace solved contact");
        require(document.commit(SketchGeometry::replaceFeature(document.authored(),sketch),"Arc-line Tangent").isEmpty(),"commit");
        const auto authored=document.authored();document.history().undo();require(document.sketches().empty(),"undo");
        document.history().redo();require(document.authored()==authored,"redo");
        QTemporaryDir dir;require(dir.isValid(),"temporary directory");const auto path=dir.path()+"/arc-tangent.alloy";
        require(document.save(path).isEmpty(),"save");CadDocument reopened;
        require(reopened.open(path).isEmpty() && reopened.authored()==authored,"reopen");
        for(double sweep:{180.0,-180.0})for(bool reverse:{false,true}){
            auto interior=SketchGeometry::newFeature(document.authored()["documentId"].toString(),"XY");
            interior=SketchGeometry::addArc(interior,{{"centerUMm",0},{"centerVMm",0},{"radiusMm",10},{"startAngleDeg",sweep>0 ? 0 : 180},{"sweepAngleDeg",sweep}});
            const auto curve=interior["parameters"].toObject()["entities"].toArray().last().toObject();
            interior=SketchGeometry::addLine(interior,{{"u1Mm",-20},{"v1Mm",11},{"u2Mm",20},{"v2Mm",11}});
            const auto edge=interior["parameters"].toObject()["entities"].toArray().last().toObject();
            auto result=SketchGeometry::addConstraint(interior,{{"type","tangent"},{"entityIds",reverse ? QJsonArray{edge["id"],curve["id"]} : QJsonArray{curve["id"],edge["id"]}}});
            require(result.ok,"interior tangent: "+result.error);interior=result.feature;
            const auto relation=interior["parameters"].toObject()["constraints"].toArray().last().toObject();
            require(relation["type"]=="arcTangent" && relation["contact"].toObject()["sweepSign"]==(sweep>0 ? 1 : -1),"interior branch correspondence");
            require(std::abs(relation["contact"].toObject()["arcParameter"].toDouble()-0.5)<1e-6,"interior arc parameter");
            require(std::abs(relation["contact"].toObject()["lineParameter"].toDouble()-0.5)<1e-6,"interior segment parameter");
            auto duplicate=SketchGeometry::addConstraint(interior,{{"type","tangent"},{"entityIds",QJsonArray{curve["id"],edge["id"]}}});
            require(!duplicate.ok && duplicate.feature==interior,"interior duplicate atomic");
        }
        for(const auto& bounds:{QPair<double,double>{10,20},{11,20},{-20,0}}){
            auto invalid=SketchGeometry::newFeature(document.authored()["documentId"].toString(),"XY");
            invalid=SketchGeometry::addArc(invalid,{{"centerUMm",0},{"centerVMm",0},{"radiusMm",10},{"startAngleDeg",0},{"sweepAngleDeg",180}});
            const auto id=invalid["parameters"].toObject()["entities"].toArray().last().toObject().value("id");
            invalid=SketchGeometry::addLine(invalid,{{"u1Mm",bounds.first},{"v1Mm",11},{"u2Mm",bounds.second},{"v2Mm",11}});
            const auto lineId=invalid["parameters"].toObject()["entities"].toArray().last().toObject().value("id");
            const auto result=SketchGeometry::addConstraint(invalid,{{"type","tangent"},{"entityIds",QJsonArray{id,lineId}}});
            require(!result.ok && result.feature==invalid,"interior endpoint/out-of-segment refusal atomic");
        }
        for(bool reverse:{false,true})for(bool explicitRoles:{false,true})for(const auto& role:{QString("start"),QString("end")})for(const auto& other:{QString("start"),QString("end")})for(double sweep:{90.,-90.}){
            auto candidate=SketchGeometry::newFeature(document.authored()["documentId"].toString(),"XY");
            candidate=SketchGeometry::addArc(candidate,{{"centerUMm",0},{"centerVMm",0},{"radiusMm",10},{"startAngleDeg",role=="start" ? 0 : -sweep},{"sweepAngleDeg",sweep}});
            const auto curve=candidate["parameters"].toObject()["entities"].toArray().last().toObject();
            candidate=SketchGeometry::addLine(candidate,{{"u1Mm",10},{"v1Mm",other=="start" ? 0 : -20},{"u2Mm",10},{"v2Mm",other=="start" ? 20 : 0}});
            auto p=candidate["parameters"].toObject();auto points=p["points"].toArray();auto entities=p["entities"].toArray();auto edge=entities.last().toObject();
            QJsonObject point;for(const auto& value:points)if(value.toObject()["id"]==edge[other+"PointId"])point=value.toObject();
            point["id"]=QUuid::createUuid().toString(QUuid::WithoutBraces);points.append(point);edge[other+"PointId"]=point["id"];entities[entities.size()-1]=edge;
            p["points"]=points;p["entities"]=entities;candidate["parameters"]=p;
            QJsonObject args{{"type","tangent"},{"entityIds",reverse ? QJsonArray{edge["id"],curve["id"]} : QJsonArray{curve["id"],edge["id"]}}};
            if(explicitRoles)args["endpoint"]=reverse ? other+":"+role : role+":"+other;
            auto result=SketchGeometry::addConstraint(candidate,args);require(result.ok,"distinct endpoint tangent "+role+":"+other+QString::number(sweep)+" explicit="+QString::number(explicitRoles)+": "+result.error);
            const auto relation=result.feature["parameters"].toObject()["constraints"].toArray().last().toObject();
            const auto contacts=relation["endpoints"].toArray();
            require(contacts.size()==2 && contacts[0].toObject()["pointId"]==curve[role+"PointId"] && contacts[1].toObject()["pointId"]==edge[other+"PointId"],"exact distinct endpoint identities");
            require(contacts[0].toObject()["endpoint"]==role && contacts[1].toObject()["endpoint"]==other,"exact endpoint roles");
            require(SketchGeometry::solveDraft(result.feature).ok,"distinct endpoint solve roundtrip");
            auto duplicate=SketchGeometry::addConstraint(result.feature,args);require(!duplicate.ok && duplicate.feature==result.feature,"distinct endpoint duplicate atomic");
            auto invalid=result.feature;p=invalid["parameters"].toObject();entities=p["entities"].toArray();auto corrupt=entities[0].toObject();corrupt["sweepAngleDeg"]=-sweep;entities[0]=corrupt;p["entities"]=entities;invalid["parameters"]=p;
            require(!SketchGeometry::solveDraft(invalid).ok,"distinct endpoint sweep reversal refuses");
        }
        std::cout<<"sketch arc-line Tangent: both contacts, pick orders, refusal, history, reopen passed\n";
        return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
