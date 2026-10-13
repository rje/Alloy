// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QJsonArray>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool ok,const QString& message) {if(!ok)throw std::runtime_error(message.toStdString());}
QJsonObject plane(const QString& owner,const QJsonObject& source,double offset) {
    return {{"id",CadDocument::newId()},{"ownerId",owner},{"type","constructionPlane"},{"name","Offset Plane"},{"visible",true},
        {"parameters",QJsonObject{{"source",source},{"offsetMm",offset}}}};
}
void append(CadDocument& document,const QJsonObject& feature) {
    auto candidate=document.authored();auto features=candidate["features"].toArray();features.append(feature);candidate["features"]=features;
    check(document.commit(candidate,"Create feature").isEmpty(),"Commit failed: "+document.lastEvaluationError()["message"].toString());
}
void atZ(const QJsonObject& frame,double z) {check(std::abs(frame["origin"].toArray()[2].toDouble()-z)<1e-7,"Plane origin Z mismatch");}
}

int main(int argc,char** argv) {QCoreApplication app(argc,argv);try {
    CadDocument document;const auto owner=document.authored()["documentId"].toString();
    auto base=SketchGeometry::newFeature(owner,"XY","Base circle");
    base=SketchGeometry::addCircle(base,{{"centerUMm",0},{"centerVMm",0},{"diameterMm",38}});
    append(document,base);
    const auto first=plane(owner,{{"sketchId",base["id"]}},18);append(document,first);
    auto raised=SketchGeometry::newConstructionPlaneFeature(owner,first["id"].toString(),"Raised circle");
    raised=SketchGeometry::addCircle(raised,{{"centerUMm",0},{"centerVMm",0},{"diameterMm",16}});
    append(document,raised);
    const auto second=plane(owner,{{"sketchId",raised["id"]}},20);append(document,second);
    const auto third=plane(owner,{{"planeId",second["id"]}},20);append(document,third);
    check(document.evaluation().constructionPlanes.size()==3,"Expected three retained planes");
    atZ(document.evaluation().constructionPlanes[0].frame,18);
    atZ(document.evaluation().constructionPlanes[1].frame,38);
    atZ(document.evaluation().constructionPlanes[2].frame,58);
    check(std::abs(document.sketches()[1].summary["worldPoints"].toArray()[0].toObject()["positionMm"].toArray()[2].toDouble()-18)<1e-7,"Sketch did not follow its plane");

    auto candidate=document.authored();auto features=candidate["features"].toArray();auto edited=features[1].toObject();auto parameters=edited["parameters"].toObject();parameters["offsetMm"]=19;edited["parameters"]=parameters;features[1]=edited;candidate["features"]=features;
    check(document.commit(candidate,"Edit offset").isEmpty(),"Edit offset failed");
    atZ(document.evaluation().constructionPlanes[0].frame,19);atZ(document.evaluation().constructionPlanes[2].frame,59);
    document.history().undo();atZ(document.evaluation().constructionPlanes[2].frame,58);
    document.history().redo();atZ(document.evaluation().constructionPlanes[2].frame,59);

    QTemporaryDir directory;check(directory.isValid(),"Temporary directory unavailable");
    const auto path=directory.filePath("planes.alloy");check(document.save(path).isEmpty(),"Save failed");
    CadDocument reopened;check(reopened.open(path).isEmpty(),"Reopen failed");atZ(reopened.evaluation().constructionPlanes[2].frame,59);
    candidate=document.authored();features=candidate["features"].toArray();features.removeAt(1);candidate["features"]=features;
    check(!document.commit(candidate,"Delete source").isEmpty(),"Removing a referenced plane must fail atomically");
    atZ(document.evaluation().constructionPlanes[2].frame,59);

    // Signed offsets preserve the complete source frame, including negative Y normal.
    for(const auto& origin:QStringList{"XY","XZ","YZ"}) {
        CadDocument axes;append(axes,plane(axes.authored()["documentId"].toString(),{{"plane",origin}},-7));
        const auto expected=SketchGeometry::frame(origin),actual=axes.evaluation().constructionPlanes[0].frame;
        check(actual["uAxis"]==expected["uAxis"] && actual["vAxis"]==expected["vAxis"] && actual["normal"]==expected["normal"],"Offset changed source axes");
        for(int axis=0;axis<3;++axis)check(std::abs(actual["origin"].toArray()[axis].toDouble()+7*expected["normal"].toArray()[axis].toDouble())<1e-7,"Offset did not follow signed normal");
    }
    CadDocument faceDocument;const auto faceOwner=faceDocument.authored()["documentId"].toString();
    auto footprint=SketchGeometry::newFeature(faceOwner,"XY","Footprint");
    footprint=SketchGeometry::addRectangle(footprint,{{"u1Mm",-12.5},{"v1Mm",-12.5},{"u2Mm",12.5},{"v2Mm",12.5}});
    append(faceDocument,footprint);
    const auto profile=faceDocument.sketches()[0].profiles.at(0).profileId;
    check(faceDocument.commit(CadDocument::extrusionCandidate(faceDocument.authored(),{{"sketchId",footprint["id"]},{"profileId",profile},{"distanceMm",25}}),"Extrude").isEmpty(),"Face source extrusion failed");
    const auto cap=faceDocument.bodies()[0].supportFaces.at(0);
    const QJsonObject support{{"producerFeatureId",cap.producerFeatureId},{"bodyId",cap.bodyId},{"faceId",cap.faceId}};
    const auto inset=plane(faceOwner,{{"support",support}},-18.5);append(faceDocument,inset);
    atZ(faceDocument.evaluation().constructionPlanes[0].frame,6.5);
    candidate=faceDocument.authored();features=candidate["features"].toArray();auto extrusion=features[1].toObject();parameters=extrusion["parameters"].toObject();parameters["distanceMm"]=30;extrusion["parameters"]=parameters;features[1]=extrusion;candidate["features"]=features;
    check(faceDocument.commit(candidate,"Edit extrusion").isEmpty(),"Supported face edit failed");
    atZ(faceDocument.evaluation().constructionPlanes[0].frame,11.5);
    std::cout << "Construction plane history, dependent sketch, edits, undo and storage passed\n";
    return 0;
} catch(const std::exception& error) {std::cerr << error.what() << '\n';return 1;}}
