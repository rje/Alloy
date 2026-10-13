#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <numbers>
#include <stdexcept>

namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void near(double actual,double expected,const char* message){check(std::abs(actual-expected)<1e-5,message);}
QJsonObject point(const QJsonObject& sketch,const QJsonValue& id){
    for(const auto& value:sketch["parameters"].toObject()["points"].toArray())if(value.toObject()["id"]==id)return value.toObject();
    return {};
}
}

int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    try {
        CadDocument document;
        const auto empty=SketchGeometry::newFeature(document.activeOwnerId(),"XY");
        auto result=SketchGeometry::addCenterPointSlot(empty,{{"centerUMm",0.03},{"centerVMm",-0.02},
            {"endUMm",68},{"endVMm",12},{"widthMm",42},{"centerSnapTarget",QJsonObject{{"kind","origin"}}}});
        check(result.ok,result.error.toUtf8().constData());
        auto sketch=result.feature;auto parameters=sketch["parameters"].toObject();
        const auto group=parameters["slotGroups"].toArray().first().toObject();
        const auto guideId=group["centerlineId"],arcId=group["arcIds"].toArray().first();
        check(result.diagnostics["dof"].toInt(-1)==3,"rough snapped slot has exactly three degrees of freedom");
        check(parameters["dimensions"].toArray().isEmpty(),"creation adds no hidden driving dimensions");
        check(parameters["entities"].toArray().size()==5,"slot retains two sides, two arcs and one guide");
        const auto entities=parameters["entities"].toArray();const auto guide=entities[2].toObject();
        check(guide["id"]==guideId && guide["construction"].toBool(),"stable construction centerline");
        const auto center=point(sketch,group["centerPointId"]);near(center["uMm"].toDouble(),0,"center snaps to origin x");
        near(center["vMm"].toDouble(),0,"center snaps to origin y");
        bool coincident=false,midpoint=false;for(const auto& value:parameters["constraints"].toArray()){
            const auto relation=value.toObject();
            if(relation["type"]=="coincident" && relation["pointIds"].toArray().contains(group["centerPointId"]))coincident=true;
            if(relation["type"]=="midpoint" && relation["entityIds"].toArray().contains(guideId))midpoint=true;
        }
        check(coincident && midpoint,"origin Coincident and guide Midpoint are retained relations");
        const auto endId=guide["endPointId"];
        result=SketchGeometry::movePoint(sketch,{{"pointId",endId},{"uMm",60},{"vMm",25}});
        check(result.ok && result.diagnostics["dof"].toInt(-1)==3,"rough end-center drag remains underdefined");sketch=result.feature;
        check(std::abs(point(sketch,endId)["vMm"].toDouble()-12)>1,"rough drag visibly changes orientation");
        result=SketchGeometry::addConstraint(sketch,{{"type","horizontal"},{"entityIds",QJsonArray{guideId}}});
        check(result.ok && result.diagnostics["dof"].toInt(-1)==2,"Horizontal straightens dragged guide and leaves length/radius free");sketch=result.feature;
        result=SketchGeometry::addDimension(sketch,{{"kind","linear"},{"entityId",guideId},{"valueMm",150}});
        check(result.ok && result.diagnostics["dof"].toInt(-1)==1,"D150 constrains full centerline, not half-length");sketch=result.feature;
        const auto lengthId=sketch["parameters"].toObject()["dimensions"].toArray().last().toObject()["id"];
        result=SketchGeometry::addDimension(sketch,{{"kind","radius"},{"entityId",arcId},{"valueMm",25}});
        check(result.ok && result.diagnostics["dof"].toInt(-1)==0,"R25 fully constrains the slot");sketch=result.feature;
        const auto radiusId=sketch["parameters"].toObject()["dimensions"].toArray().last().toObject()["id"];
        auto geometry=SketchGeometry::evaluateFeature(sketch);
        check(geometry.profiles.size()==1,"four analytic boundary edges yield one closed selectable profile");
        const auto bounds=geometry.summary["boundsMm"].toObject();
        near(bounds["min"].toArray()[0].toDouble(),-100,"outside left bound");
        near(bounds["max"].toArray()[0].toDouble(),100,"outside right bound");
        near(bounds["size"].toArray()[1].toDouble(),50,"outside width");
        near(geometry.profiles[0].summary["areaMm2"].toDouble(),7500+625*std::numbers::pi,"analytic slot area");
        const auto before=sketch;
        result=SketchGeometry::updateDimension(sketch,{{"dimensionId",radiusId},{"valueMm",-2}});
        check(!result.ok && result.feature==before,"invalid radius edit refuses atomically");
        result=SketchGeometry::updateDimension(sketch,{{"dimensionId",lengthId},{"valueMm",170}});
        check(result.ok && result.diagnostics["dof"].toInt(-1)==0,"length edit retains full constraint");sketch=result.feature;
        geometry=SketchGeometry::evaluateFeature(sketch);
        near(geometry.summary["boundsMm"].toObject()["size"].toArray()[0].toDouble(),220,"length edit regenerates both ends");
        check(sketch["parameters"].toObject()["slotGroups"].toArray().first().toObject()==group,"slot group IDs survive dimension edits");
        check(document.commit(SketchGeometry::replaceFeature(document.authored(),sketch),"Center Point Slot").isEmpty(),"commit slot");
        const auto authored=document.authored();document.history().undo();check(document.sketches().empty(),"undo removes slot");
        document.history().redo();check(document.authored()==authored,"redo restores exact authored slot");
        QTemporaryDir directory;check(directory.isValid(),"temporary save directory");
        const auto path=directory.path()+"/center-point-slot.alloy";check(document.save(path).isEmpty(),"save slot");
        CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==authored,"reopen retains exact authored slot");
        std::cout<<"Center Point Slot core contract passed\n";
        return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
