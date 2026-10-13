#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <numbers>
#include <stdexcept>

namespace {
void check(bool good,const char* message){if(!good)throw std::runtime_error(message);}
void near(double actual,double expected,const char* message){check(std::abs(actual-expected)<1e-5,message);}
QJsonObject entity(const QJsonObject& sketch,const QJsonValue& id){
    for(const auto& value:sketch["parameters"].toObject()["entities"].toArray())if(value.toObject()["id"]==id)return value.toObject();
    return {};
}
}
int main(int argc,char** argv){
    QCoreApplication app(argc,argv);
    try {
        CadDocument document;
        const auto empty=SketchGeometry::newFeature(document.activeOwnerId(),"XY");
        auto result=SketchGeometry::addCenterPointSlot(empty,{{"centerUMm",0},{"centerVMm",0},
            {"endUMm",67},{"endVMm",13},{"widthMm",48},
            {"centerSnapTarget",QJsonObject{{"kind","origin"}}}});
        if(!result.ok)throw std::runtime_error(result.error.toStdString());
        auto straight=result.feature;
        const auto straightGroup=straight["parameters"].toObject()["slotGroups"].toArray().first().toObject();
        const auto straightGuide=straightGroup["centerlineId"];
        result=SketchGeometry::addConstraint(straight,{{"type","horizontal"},{"entityIds",QJsonArray{straightGuide}}});
        if(!result.ok)throw std::runtime_error(result.error.toStdString());straight=result.feature;
        result=SketchGeometry::addDimension(straight,{{"kind","linear"},{"entityId",straightGuide},{"valueMm",150}});
        if(!result.ok)throw std::runtime_error(result.error.toStdString());straight=result.feature;
        result=SketchGeometry::addDimension(straight,{{"kind","radius"},{"entityId",straightGroup["arcIds"].toArray()[0]},{"valueMm",25}});
        if(!result.ok)throw std::runtime_error(result.error.toStdString());straight=result.feature;
        result=SketchGeometry::addCenterPointArcSlot(straight,{{"centerUMm",0},{"centerVMm",0},
            {"firstUMm",-100},{"firstVMm",110},{"lastUMm",105},{"lastVMm",98},
            {"widthMm",28},{"centerSnapTarget",QJsonObject{{"kind","origin"}}}});
        if(!result.ok)throw std::runtime_error(result.error.toStdString());
        auto sketch=result.feature;auto parameters=sketch["parameters"].toObject();
        const auto group=parameters["arcSlotGroups"].toArray().first().toObject();
        check(result.diagnostics["dof"].toInt(-1)==4,"rough arc slot has four degrees of freedom");
        check(parameters["dimensions"].toArray().size()==2,"rough arc slot adds no hidden dimension");
        auto geometry=SketchGeometry::evaluateFeature(sketch);
        check(geometry.profiles.size()==2,"straight and arc slots have two independent profiles");
        const auto path=entity(sketch,group["pathArcId"]);const auto caps=group["capIds"].toArray();
        const QJsonObject horizontal{{"type","horizontal"},{"pointIds",QJsonArray{path["startPointId"],path["endPointId"]}}};
        result=SketchGeometry::addConstraint(sketch,horizontal);
        if(!result.ok)throw std::runtime_error(result.error.toStdString());sketch=result.feature;
        check(result.diagnostics["dof"].toInt(-1)==3,"path end centers horizontal");
        result=SketchGeometry::addDimension(sketch,{{"kind","radius"},{"entityId",group["pathArcId"]},{"valueMm",150}});
        if(!result.ok)throw std::runtime_error(result.error.toStdString());sketch=result.feature;
        check(result.diagnostics["dof"].toInt(-1)==2,"path R150");
        const auto pathRadiusId=sketch["parameters"].toObject()["dimensions"].toArray().last().toObject()["id"];
        result=SketchGeometry::addDimension(sketch,{{"kind","radius"},{"entityId",caps[0]},{"valueMm",15}});
        if(!result.ok)throw std::runtime_error(result.error.toStdString());sketch=result.feature;
        check(result.diagnostics["dof"].toInt(-1)==1,"cap R15");
        const auto firstPoint=[&](){for(const auto& value:sketch["parameters"].toObject()["points"].toArray())
            if(value.toObject()["id"]==path["startPointId"])return value.toObject();return QJsonObject{};}();
        check(!firstPoint.isEmpty(),"first path endpoint survives dimensions");
        result=SketchGeometry::addPointerEntity(sketch,"line",{{"u1Mm",firstPoint["uMm"]},{"v1Mm",firstPoint["vMm"]},
            {"u2Mm",0},{"v2Mm",0},{"startSnapTarget",QJsonObject{{"kind","point"},{"pointId",path["startPointId"]}}},
            {"endSnapTarget",QJsonObject{{"kind","origin"}}}});
        if(!result.ok)throw std::runtime_error(result.error.toStdString());sketch=result.feature;
        const auto lines=sketch["parameters"].toObject()["entities"].toArray();const auto radialId=lines.last().toObject()["id"];
        result=SketchGeometry::setConstruction(sketch,{{"entityIds",QJsonArray{radialId}},{"construction",true}});
        if(!result.ok)throw std::runtime_error(result.error.toStdString());sketch=result.feature;
        result=SketchGeometry::addDimension(sketch,{{"kind","angle"},{"entityIds",QJsonArray{straightGuide,radialId}},
            {"raySigns",QJsonArray{1,-1}},{"valueDeg",135}});
        if(!result.ok)throw std::runtime_error(result.error.toStdString());sketch=result.feature;
        check(result.diagnostics["dof"].toInt(-1)==0,"radial D135 fully constrains arc slot");
        geometry=SketchGeometry::evaluateFeature(sketch);
        check(geometry.profiles.size()==2,"two closed source regions remain");
        bool foundArcRegion=false;
        for(const auto& profile:geometry.profiles)if(std::abs(profile.summary["areaMm2"].toDouble()-2475*std::numbers::pi)<1e-4)
            foundArcRegion=true;
        check(foundArcRegion,"upper ninety-degree rounded arc band has analytic area 2475 pi");
        const auto finishedPath=entity(sketch,group["pathArcId"]);
        near(finishedPath["radiusMm"].toDouble(),150,"source path R150");
        near(finishedPath["sweepAngleDeg"].toDouble(),-90,"source upper clockwise path sweep");
        const auto finishedCaps=group["capIds"].toArray();
        near(entity(sketch,finishedCaps[0])["radiusMm"].toDouble(),15,"source cap R15");
        near(entity(sketch,group["outerArcId"])["radiusMm"].toDouble(),165,"source outer side R165");
        near(entity(sketch,group["innerArcId"])["radiusMm"].toDouble(),135,"source inner side R135");
        const auto retainedGroup=sketch["parameters"].toObject()["arcSlotGroups"].toArray().first().toObject();
        const auto before=sketch;
        result=SketchGeometry::updateDimension(sketch,{{"dimensionId",pathRadiusId},{"valueMm",10}});
        check(!result.ok && result.feature==before,"path smaller than cap radius refuses atomically");
        result=SketchGeometry::updateDimension(sketch,{{"dimensionId",pathRadiusId},{"valueMm",170}});
        if(!result.ok)throw std::runtime_error(result.error.toStdString());sketch=result.feature;
        check(result.diagnostics["dof"].toInt(-1)==0,"path radius edit retains zero DOF");
        geometry=SketchGeometry::evaluateFeature(sketch);
        bool foundEdited=false;for(const auto& profile:geometry.profiles)
            if(std::abs(profile.summary["areaMm2"].toDouble()-2775*std::numbers::pi)<1e-4)foundEdited=true;
        check(foundEdited,"path R170 regenerates analytic band area");
        check(sketch["parameters"].toObject()["arcSlotGroups"].toArray().first().toObject()==retainedGroup,
            "arc slot identities survive radius edit");
        check(document.commit(SketchGeometry::replaceFeature(document.authored(),sketch),"Arc Slot").isEmpty(),"commit source sketch");
        const auto authored=document.authored();document.history().undo();check(document.sketches().empty(),"undo source sketch");
        document.history().redo();check(document.authored()==authored,"redo exact source sketch");
        QTemporaryDir directory;check(directory.isValid(),"temporary save location");
        const auto savePath=directory.path()+"/arc-slot.alloy";check(document.save(savePath).isEmpty(),"save source sketch");
        CadDocument reopened;check(reopened.open(savePath).isEmpty() && reopened.authored()==authored,"reopen exact source sketch");
        // Each further arc slot owns its own cap radius unknown; editing one
        // group's cap cannot silently resize an earlier group.
        result=SketchGeometry::addCenterPointArcSlot(sketch,{{"centerUMm",0},{"centerVMm",-300},
            {"firstUMm",-80},{"firstVMm",-220},{"lastUMm",80},{"lastVMm",-220},{"widthMm",20}});
        if(!result.ok)throw std::runtime_error(result.error.toStdString());
        auto multiple=result.feature;
        const auto groups=multiple["parameters"].toObject()["arcSlotGroups"].toArray();
        check(groups.size()==2,"second arc slot remains a distinct authored group");
        result=SketchGeometry::addDimension(multiple,{{"kind","radius"},
            {"entityId",groups[1].toObject()["capIds"].toArray()[0]},{"valueMm",12}});
        if(!result.ok)throw std::runtime_error(result.error.toStdString());multiple=result.feature;
        near(entity(multiple,groups[0].toObject()["capIds"].toArray()[0])["radiusMm"].toDouble(),15,
            "first arc slot cap radius remains independent");
        near(entity(multiple,groups[1].toObject()["capIds"].toArray()[0])["radiusMm"].toDouble(),12,
            "second arc slot cap radius drives only its own group");
        check(SketchGeometry::evaluateFeature(multiple).profiles.size()==3,
            "two arc slots and straight slot remain distinct closed regions");
        std::cout<<"Arc slot full source core passed\n";
        return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
