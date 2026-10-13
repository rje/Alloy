// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include "PlanarFaces.h"
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <QCoreApplication>
#include <QJsonArray>
#include <QTemporaryDir>
#include <iostream>
#include <cmath>
namespace {
void check(bool v,const char* message){if(!v)throw std::runtime_error(message);}
QJsonObject ref(const EvaluatedSupportFace& f){return {{"producerFeatureId",f.producerFeatureId},{"bodyId",f.bodyId},{"faceId",f.faceId}};}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try {
    CadDocument document;const auto owner=document.authored()["documentId"].toString();
    auto rectangle=SketchGeometry::newFeature(owner,"XY","Face source");
    rectangle=SketchGeometry::addRectangle(rectangle,{{"u1Mm",0},{"v1Mm",0},{"u2Mm",25},{"v2Mm",25}});
    check(document.commit(SketchGeometry::replaceFeature(document.authored(),rectangle),"Rectangle").isEmpty(),"rectangle");
    auto extrusion=CadDocument::extrusionCandidate(document.authored(),{{"sketchId",rectangle["id"]},{"profileId",document.sketches()[0].profiles[0].profileId},{"distanceMm",25}});
    check(document.commit(extrusion,"Cube").isEmpty(),"cube");
    const auto body=document.bodies()[0];check(body.supportFaces.size()==6,"cube needs six named planar faces");
    int sides=0;
    for(const auto& f:body.supportFaces) {
        GeometryKernel::resolveSketchSupport(document.evaluation(),ref(f));
        if(f.role!="extrude.side")continue;
        ++sides;
        auto sketch=SketchGeometry::newSupportedFeature(owner,ref(f),"Side sketch",{});
        sketch=SketchGeometry::addCircle(sketch,{{"centerUMm",0},{"centerVMm",5},{"diameterMm",2}});
        const auto candidate=SketchGeometry::replaceFeature(document.authored(),sketch);
        const auto evaluated=GeometryKernel::evaluateDocument(candidate);
        check(evaluated.sketches.back().resolvedFrame==f.frame,"side sketch changed retained frame");
        const auto& profile=evaluated.sketches.back().profiles[0];
        const auto normal=f.frame["normal"].toArray();
        const auto origin=f.frame["origin"].toArray();
        const double outward=normal[0].toDouble()*(origin[0].toDouble()-12.5)+normal[1].toDouble()*(origin[1].toDouble()-12.5);
        check(outward>12.49,"side normal must point outward");
        auto solid=GeometryKernel::extrusion(profile,3);const auto summary=GeometryKernel::summarize(solid);
        check(std::abs(summary["volumeMm3"].toDouble()-3*std::acos(-1.0))<1e-6,"side sketch extrusion volume");
    }
    check(sides==4,"four side names");
    // Split an entire top face into two islands: neither may inherit its name.
    BRepAlgoAPI_Cut split(body.shape,BRepPrimAPI_MakeBox(gp_Pnt(10,-1,-1),5,27,27).Shape());
    split.Build();check(split.IsDone(),"split fixture");
    const auto mapped=PlanarFaces::transfer(split,body.supportFaces,split.Shape());
    for(const auto& f:mapped)check(f.faceId!=body.supportFaces[0].faceId,"ambiguous cap silently rebound");
    // The original producer reference still resolves after a height edit.
    const auto producer=document.authored()["features"].toArray().last().toObject();
    check(document.commit(CadDocument::extrusionCandidate(document.authored(),{{"featureId",producer["id"]},{"sketchId",rectangle["id"]},{"profileId",document.sketches()[0].profiles[0].profileId},{"distanceMm",30}}),"Height").isEmpty(),"height edit");
    for(const auto& f:body.supportFaces) {
        const auto current=GeometryKernel::resolveSketchSupport(document.evaluation(),ref(f));
        if(f.role=="extrude.side")check(current.frame==f.frame,"side origin drifted with height");
    }
    QTemporaryDir tmp;check(document.save(tmp.path()+"/faces.alloy").isEmpty(),"save");
    CadDocument reopened;check(reopened.open(tmp.path()+"/faces.alloy").isEmpty(),"reopen");
    for(const auto& f:body.supportFaces)GeometryKernel::resolveSketchSupport(reopened.evaluation(),ref(f));
    std::cout<<"PASS: six named faces, four oriented side sketches, split refusal, height edit and reopen\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
