// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include "SolidFillet.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
QJsonObject accepted(const SketchGeometry::SolveResult& result){
    if(!result.ok)throw std::runtime_error(result.error.toStdString());return result.feature;
}
QJsonObject support(const EvaluatedSupportFace& face){
    return {{"producerFeatureId",face.producerFeatureId},{"bodyId",face.bodyId},{"faceId",face.faceId}};
}
QString upperRim(const CadDocument& document,const QString& source,bool requireAvailable=true){
    QString result;
    for(const auto& edge:document.bodies().front().solidEdges){
        if((requireAvailable && edge.edge.IsNull()) || edge.provenance["sourceSketchId"]!=source || edge.provenance["role"]!="baseRim")continue;
        check(result.isEmpty(),"Circular Cut has ambiguous upper rims");result=edge.edgeId;
    }
    return result;
}
bool keepsFloor(const CadDocument& document,const QJsonObject& reference){
    for(const auto& face:document.bodies().front().supportFaces)
        if(support(face)==reference && face.frame["normal"]==QJsonArray{0,0,1}
            && std::abs(face.frame["origin"].toArray()[2].toDouble()-2)<1e-6)return true;
    return false;
}
}

int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument document;
    auto floor=SketchGeometry::addRectangle(SketchGeometry::newFeature(document.activeOwnerId(),"XY","Floor"),
        {{"u1Mm",0},{"v1Mm",0},{"u2Mm",60},{"v2Mm",-30}});
    check(document.commit(SketchGeometry::replaceFeature(document.authored(),floor),"Floor sketch").isEmpty(),"Floor sketch");
    const auto profile=document.sketches().front().profiles.front().profileId;
    check(document.commit(CadDocument::extrusionCandidate(document.authored(),{{"sketchId",floor["id"]},{"profileId",profile},{"distanceMm",2}}),"Floor 2").isEmpty(),"Floor extrusion");
    const auto bodyId=document.bodies().front().bodyId;
    QJsonObject originalFloor;
    for(const auto& face:document.bodies().front().supportFaces)if(face.role=="extrude.endCap")originalFloor=support(face);
    check(!originalFloor.isEmpty(),"Floor has no top cap");
    auto hole=SketchGeometry::newSupportedFeature(document.activeOwnerId(),originalFloor,"Diameter 4 bore");
    hole=SketchGeometry::addCircle(hole,{{"centerUMm",7},{"centerVMm",-15},{"diameterMm",4}});
    const auto circle=hole["parameters"].toObject()["entities"].toArray().first().toObject();
    hole=accepted(SketchGeometry::addDimension(hole,{{"kind","diameter"},{"entityId",circle["id"]},{"valueMm",4}}));
    const auto diameter=hole["parameters"].toObject()["dimensions"].toArray().last().toObject()["id"].toString();
    check(document.commit(SketchGeometry::replaceFeature(document.authored(),hole),"Bore sketch").isEmpty(),"Supported circle sketch");
    const auto holeProfile=document.sketches().back().profiles.front().profileId;
    check(document.commit(CadDocument::extrusionCandidate(document.authored(),
        {{"sketchId",hole["id"]},{"profileId",holeProfile},{"distanceMm",-10},{"operation","cut"},
            {"targetBodyIds",QJsonArray{bodyId}}}),"Cut bore").isEmpty(),"Diameter 4 Cut -10");
    const auto edgeId=upperRim(document,hole["id"].toString());
    check(!edgeId.isEmpty(),"Cut upper rim has no retained selectable edge");
    check(keepsFloor(document,originalFloor),"Cut lost broad floor support");
    auto selected=QJsonArray{edgeId};
    check(document.commit(CadDocument::filletCandidate(document.authored(),
        {{"targetBodyId",bodyId},{"edgeIds",selected},{"radiusMm",1}}),"Hole R1").isEmpty(),"Upper rim R1 Fillet");
    check(keepsFloor(document,originalFloor),"Fillet lost broad floor support");
    const auto filleted=document.authored();
    const auto revised=accepted(SketchGeometry::updateDimension(hole,
        {{"dimensionId",diameter},{"valueMm",4.5}}));
    check(document.commit(SketchGeometry::replaceFeature(filleted,revised),"Bore diameter edit").isEmpty(),"Diameter edit should rebuild Cut and Fillet");
    check(upperRim(document,hole["id"].toString(),false)==edgeId,"Cut rim ID changed after diameter edit");
    check(keepsFloor(document,originalFloor),"Diameter edit lost broad floor support");
    document.history().undo();check(document.authored()==filleted,"Undo restored wrong authored source");
    QTemporaryDir temp;check(document.save(temp.filePath("hole-rim.alloy")).isEmpty(),"Save");
    CadDocument reopened;check(reopened.open(temp.filePath("hole-rim.alloy")).isEmpty(),"Reopen");
    check(reopened.authored()==filleted && upperRim(reopened,hole["id"].toString(),false)==edgeId,"Reopen lost Cut rim/Fillet");
    check(keepsFloor(reopened,originalFloor),"Reopen lost broad floor support");
    std::cout<<"PASS: retained Cut upper circle, R1 Fillet, diameter edit, undo and reopen\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
