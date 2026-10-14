// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void commit(CadDocument& doc,const QJsonObject& candidate){
    const auto error=doc.commit(candidate,"Cut side support check");
    if(!error.isEmpty())throw std::runtime_error(error.toStdString());
}
QJsonObject side(const CadDocument& doc,const QString& cut,double y){
    QJsonObject found;
    for(const auto& body:doc.bodies())for(const auto& face:body.supportFaces){
        const auto frame=face.frame;
        const auto origin=frame["origin"].toArray(),normal=frame["normal"].toArray();
        if(face.producerFeatureId!=cut || face.role!="extrude.side" || origin.size()!=3 || normal.size()!=3
           || std::abs(origin[1].toDouble()-y)>1e-6 || std::abs(normal[1].toDouble())<.99)continue;
        check(found.isEmpty(),"Cut side identity is ambiguous");
        check(face.boundaryEdges.size()==4,"Cut side has no four exact boundary edges");
        QJsonArray ids;for(const auto& edge:face.boundaryEdges){
            check(!edge.edge.IsNull() && edge.type=="line" && edge.provenance["lineage"]=="exactLinearCutHistory",
                "Cut side boundary is not exact current lineage");
            ids.append(edge.edgeId);
        }
        found={{"faceId",face.faceId},{"bodyId",face.bodyId},
            {"producerFeatureId",face.producerFeatureId},{"edgeIds",ids},{"normal",normal}};
    }
    return found;
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument doc;const auto owner=doc.authored()["documentId"].toString();
    const QJsonObject box{{"id",CadDocument::newId()},{"bodyId",CadDocument::newId()},
        {"ownerId",owner},{"type","box"},{"name","Wall"},{"visible",true},
        {"parameters",QJsonObject{{"widthMm",40},{"depthMm",20},{"heightMm",20},
            {"xMm",-20},{"yMm",-10},{"zMm",0}}}};
    auto authored=doc.authored();authored["features"]=QJsonArray{box};commit(doc,authored);
    QJsonObject support;
    for(const auto& face:doc.bodies().front().supportFaces)if(face.role=="box.x0")
        support={{"bodyId",face.bodyId},{"producerFeatureId",face.producerFeatureId},{"faceId",face.faceId}};
    check(!support.isEmpty(),"Outer wall support is unavailable");
    auto sketch=SketchGeometry::newSupportedFeature(owner,support,"Closed catch");
    auto rectangle=SketchGeometry::addCenterRectangle(sketch,
        {{"centerUMm",10},{"centerVMm",10},{"widthMm",3},{"heightMm",6}});
    check(rectangle.ok,"Catch rectangle solve failed");sketch=rectangle.feature;
    commit(doc,SketchGeometry::replaceFeature(doc.authored(),sketch));
    const auto profile=doc.sketches().back().profiles.front().profileId;
    commit(doc,CadDocument::extrusionCandidate(doc.authored(),{{"sketchId",sketch["id"]},
        {"profileId",profile},{"distanceMm",-5},{"operation","cut"},
        {"targetBodyIds",QJsonArray{box["bodyId"]}}}));
    const auto cut=doc.authored()["features"].toArray().last().toObject()["id"].toString();
    const auto negative=side(doc,cut,-3),positive=side(doc,cut,3);
    check(!negative.isEmpty() && !positive.isEmpty(),"Closed Cut lacks exact side support");
    check(negative["normal"].toArray()[1].toDouble()>.99 &&
        positive["normal"].toArray()[1].toDouble()<-.99,"Cut cavity normals are reversed");
    const QJsonObject sideReference{{"bodyId",negative["bodyId"]},
        {"producerFeatureId",negative["producerFeatureId"]},{"faceId",negative["faceId"]}};
    auto supported=SketchGeometry::newSupportedFeature(owner,sideReference,"Side sketch");
    commit(doc,SketchGeometry::replaceFeature(doc.authored(),supported));
    const auto accepted=doc.authored();const auto revision=doc.revision();
    QJsonObject invalid=sideReference;invalid["faceId"]=CadDocument::newId();
    auto missing=SketchGeometry::newSupportedFeature(owner,invalid,"Missing side");
    check(!doc.commit(SketchGeometry::replaceFeature(doc.authored(),missing),"Missing support").isEmpty(),
        "Invented Cut side support was accepted");
    check(doc.authored()==accepted && doc.revision()==revision,"Invalid side changed document");
    QString heightId;for(const auto& value:sketch["parameters"].toObject()["dimensions"].toArray())
        if(value.toObject()["kind"]=="vertical")heightId=value.toObject()["id"].toString();
    check(!heightId.isEmpty(),"Catch height dimension missing");
    auto resized=SketchGeometry::updateDimension(sketch,{{"dimensionId",heightId},{"valueMm",5.8}});
    check(resized.ok,"Catch resize failed");commit(doc,SketchGeometry::replaceFeature(doc.authored(),resized.feature));
    const auto edited=side(doc,cut,-2.9);
    check(!edited.isEmpty() && edited["faceId"]==negative["faceId"] &&
        edited["edgeIds"]==negative["edgeIds"],"Cut side identity changed on edit");
    doc.history().undo();check(doc.authored()==accepted,"Undo lost original Cut side");
    doc.history().redo();check(doc.authored()!=accepted,"Redo lost Cut side edit");
    QTemporaryDir directory;check(doc.save(directory.filePath("cut-side.alloy")).isEmpty(),"Save failed");
    CadDocument reopened;check(reopened.open(directory.filePath("cut-side.alloy")).isEmpty(),"Reopen failed");
    check(side(reopened,cut,-2.9)["faceId"]==negative["faceId"],"Reopen lost Cut side identity");
    if(argc==2){
        CadDocument actual;check(actual.open(QString::fromLocal8Bit(argv[1])).isEmpty(),"Actual Cut fixture open failed");
        const auto features=actual.authored()["features"].toArray();
        const auto last=features.last().toObject();check(last["type"]=="extrude" &&
            last["parameters"].toObject()["operation"]=="cut","Actual fixture has no final Cut");
        const auto face=side(actual,last["id"].toString(),17);
        check(!face.isEmpty(),"Actual Y17 Cut side support unavailable");
        const QJsonObject reference{{"bodyId",face["bodyId"]},
            {"producerFeatureId",face["producerFeatureId"]},{"faceId",face["faceId"]}};
        const auto planeId=CadDocument::newId();
        QJsonObject plane{{"id",planeId},{"ownerId",last["ownerId"]},
            {"type","constructionPlane"},{"name","Catch side +0.5"},{"visible",true},
            {"parameters",QJsonObject{{"source",QJsonObject{{"support",reference}}},{"offsetMm",.5}}}};
        auto withPlane=actual.authored();auto rows=withPlane["features"].toArray();rows.append(plane);
        withPlane["features"]=rows;commit(actual,withPlane);
        const auto frame=actual.evaluation().constructionPlanes.back().frame;
        check(std::abs(frame["origin"].toArray()[1].toDouble()-17.5)<1e-6,
            "Actual Cut side plane did not offset toward the catch interior");
        const auto sideSketch=SketchGeometry::newConstructionPlaneFeature(last["ownerId"].toString(),
            planeId,"Actual cutout side sketch");
        commit(actual,SketchGeometry::replaceFeature(actual.authored(),sideSketch));
    }
    std::cout<<"PASS exact planar Cut side, boundaries, support, edit and reopen\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
