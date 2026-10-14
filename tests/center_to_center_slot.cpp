#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
QJsonObject byId(const QJsonArray& rows,const QJsonValue& id){
    for(const auto& value:rows)if(value.toObject()["id"]==id)return value.toObject();return {};
}
QJsonObject accept(const SketchGeometry::SolveResult& result){
    if(!result.ok)throw std::runtime_error(result.error.toStdString());return result.feature;
}
}

int main(int argc,char** argv){
    QCoreApplication app(argc,argv);
    try {
        CadDocument document;
        auto sketch=SketchGeometry::newFeature(document.activeOwnerId(),"XY");
        sketch=accept(SketchGeometry::addStandalonePoint(sketch,{{"uMm",42},{"vMm",8}}));
        const auto target=sketch["parameters"].toObject()["entities"].toArray().first().toObject()
            .value("pointId").toString();
        const auto before=sketch;
        auto invalid=SketchGeometry::addCenterToCenterSlot(sketch,{{"firstUMm",0},{"firstVMm",0},
            {"secondUMm",0},{"secondVMm",0},{"widthMm",10},
            {"firstSnapTarget",QJsonObject{{"kind","origin"}}},
            {"secondSnapTarget",QJsonObject{{"kind","origin"}}}});
        check(!invalid.ok && invalid.feature==before,"same cap-center target refuses atomically");
        sketch=accept(SketchGeometry::addCenterToCenterSlot(sketch,{{"firstUMm",0.2},{"firstVMm",0.1},
            {"secondUMm",40},{"secondVMm",7},{"widthMm",10},
            {"firstSnapTarget",QJsonObject{{"kind","origin"}}},
            {"secondSnapTarget",QJsonObject{{"kind","point"},{"pointId",target}}}}));
        auto parameters=sketch["parameters"].toObject();
        check(parameters["slotGroups"].toArray().size()==1 && parameters["dimensions"].toArray().isEmpty(),
            "rough center-to-center slot retains one analytic group and no automatic dimensions");
        const auto group=parameters["slotGroups"].toArray().first().toObject();
        const auto guide=byId(parameters["entities"].toArray(),group["centerlineId"]);
        check(!guide.isEmpty() && guide["construction"].toBool(),"retained construction guide");
        const auto first=byId(parameters["points"].toArray(),guide["startPointId"]);
        const auto second=byId(parameters["points"].toArray(),guide["endPointId"]);
        check(!first.isEmpty() && !second.isEmpty(),"guide endpoint point records exist");
        check(std::hypot(first["uMm"].toDouble(),first["vMm"].toDouble())<1e-5,
            "first picked cap center coincides with origin, not midpoint");
        check(std::hypot(second["uMm"].toDouble()-42,second["vMm"].toDouble()-8)<1e-5,
            "second picked cap center coincides with retained point");
        int endpointCoincidences=0;
        for(const auto& value:parameters["constraints"].toArray()){
            const auto relation=value.toObject();if(relation["type"]!="coincident")continue;
            const auto ids=relation["pointIds"].toArray();
            if(ids.contains(guide["startPointId"]) || ids.contains(guide["endPointId"]))++endpointCoincidences;
        }
        check(endpointCoincidences==2,"both exact cap-center endpoint relations retained");
        const auto geometry=SketchGeometry::evaluateFeature(sketch);
        check(geometry.profiles.size()==1,"four analytic slot edges bound one profile");
        check(document.commit(SketchGeometry::replaceFeature(document.authored(),sketch),"Center to Center Slot").isEmpty(),
            "commit slot");
        const auto authored=document.authored();document.history().undo();document.history().redo();
        check(document.authored()==authored,"slot history exact");
        QTemporaryDir directory;check(directory.isValid(),"save directory");
        const auto path=directory.path()+"/center-to-center-slot.alloy";
        check(document.save(path).isEmpty(),"save slot");
        CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==authored,
            "reopen preserves endpoint and group identities");
        std::cout<<"Center to Center Slot core contract passed\n";
        return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
