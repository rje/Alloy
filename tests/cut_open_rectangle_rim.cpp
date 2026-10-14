// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool valid,const char* message){if(!valid)throw std::runtime_error(message);}
QJsonArray rims(const CadDocument& document,const QString& sketch) {
    QJsonArray ids;QSet<QString> sourceEntities;
    for(const auto& edge:document.bodies().front().solidEdges)
        if(!edge.edge.IsNull() && edge.provenance["role"]=="cutRim" && edge.provenance["sourceSketchId"]==sketch) {
            check(!edge.provenance["sourceEntityId"].toString().isEmpty(),"Cut rim lost source entity");
            check(!sourceEntities.contains(edge.provenance["sourceEntityId"].toString()),"Cut rim source is ambiguous");
            sourceEntities.insert(edge.provenance["sourceEntityId"].toString());ids.append(edge.edgeId);
        }
    return ids;
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument document;auto box=QJsonObject{{"id",CadDocument::newId()},{"bodyId",CadDocument::newId()},
        {"ownerId",document.authored()["documentId"]},{"type","box"},{"name","Wall stock"},{"visible",true},
        {"parameters",QJsonObject{{"widthMm",40},{"depthMm",20},{"heightMm",20},
            {"xMm",-20},{"yMm",-10},{"zMm",0}}}};
    auto candidate=document.authored();candidate["features"]=QJsonArray{box};
    check(document.commit(candidate,"Wall stock").isEmpty(),"Box commit");
    const auto bodyId=document.bodies().front().bodyId;
    QJsonObject support;for(const auto& face:document.bodies().front().supportFaces)if(face.role=="box.y0")
        support={{"bodyId",face.bodyId},{"producerFeatureId",face.producerFeatureId},{"faceId",face.faceId}};
    check(!support.isEmpty(),"Front support unavailable");
    auto sketch=SketchGeometry::newSupportedFeature(document.activeOwnerId(),support,"Bottom-open card rectangle");
    auto solved=SketchGeometry::addCenterRectangle(sketch,{{"centerUMm",20},{"centerVMm",0},{"widthMm",16},{"heightMm",12}});
    check(solved.ok,"Rectangle solve");sketch=solved.feature;
    check(document.commit(SketchGeometry::replaceFeature(document.authored(),sketch),"Card rectangle").isEmpty(),"Sketch commit");
    check(document.sketches().back().profiles.size()==1,"One card profile");
    const auto profile=document.sketches().back().profiles.front().profileId;
    check(document.commit(CadDocument::extrusionCandidate(document.authored(),{{"sketchId",sketch["id"]},
        {"profileId",profile},{"distanceMm",-5},{"operation","cut"},{"targetBodyIds",QJsonArray{bodyId}}}),
        "Card Cut").isEmpty(),"Bottom-open Cut");
    const auto ids=rims(document,sketch["id"].toString());check(ids.size()==3,"Three exact current U-lip edges");
    check(document.commit(CadDocument::filletCandidate(document.authored(),{{"targetBodyId",bodyId},
        {"edgeIds",ids},{"radiusMm",2},{"tangentChain",true}}),"Card lip R2").isEmpty(),"R2 Fillet");
    const auto filleted=document.authored();const auto initialVolume=document.bodies().front().summary["volumeMm3"].toDouble();
    QString widthId;for(const auto& value:sketch["parameters"].toObject()["dimensions"].toArray())
        if(value.toObject()["kind"]=="horizontal")widthId=value.toObject()["id"].toString();
    check(!widthId.isEmpty(),"Retained width dimension");
    const auto edit=SketchGeometry::updateDimension(sketch,{{"dimensionId",widthId},{"valueMm",14}});
    check(edit.ok,"Rectangle width edit");
    check(document.commit(SketchGeometry::replaceFeature(document.authored(),edit.feature),"Width 14").isEmpty(),
        "Edited U-lip Cut and R2 replay");
    check(document.bodies().front().summary["valid"].toBool() &&
        std::abs(document.bodies().front().summary["volumeMm3"].toDouble()-initialVolume)>1e-4,"Edited solid changed");
    auto cutOnly=document.authored();auto features=cutOnly["features"].toArray();features.removeLast();cutOnly["features"]=features;
    check(document.commit(cutOnly,"Inspect edited Cut").isEmpty(),"Edited Cut evaluation");
    check(rims(document,sketch["id"].toString())==ids,"Cut U-lip edge identities changed after width edit");
    document.history().undo();document.history().undo();check(document.authored()==filleted,"Undo restored authored cut/fillet");
    QTemporaryDir temp;check(document.save(temp.filePath("open-cut-r2.alloy")).isEmpty(),"Save");
    CadDocument reopened;check(reopened.open(temp.filePath("open-cut-r2.alloy")).isEmpty(),"Reopen");
    check(reopened.authored()==filleted && reopened.bodies().front().summary["valid"].toBool(),
        "Reopen lost edited Fillet");
    std::cout<<"Bottom-open Cut U-lip R2, width edit, undo and reopen passed\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
