// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
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
        std::cout<<"sketch arc-line Tangent: both contacts, pick orders, refusal, history, reopen passed\n";
        return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
