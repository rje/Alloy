#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
static bool close(double a,double b){return std::abs(a-b)<1e-5;}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument document;auto sketch=SketchGeometry::newFeature(document.authored()["documentId"].toString(),"XY","Square");
    sketch=SketchGeometry::addRectangle(sketch,{{"u1Mm",0},{"v1Mm",0},{"u2Mm",25},{"v2Mm",25}});
    check(document.commit(SketchGeometry::replaceFeature(document.authored(),sketch),"Square").isEmpty(),"sketch commit");
    const auto profile=document.sketches()[0].profiles[0].profileId;
    check(document.commit(CadDocument::extrusionCandidate(document.authored(),{{"sketchId",sketch["id"]},{"profileId",profile},{"distanceMm",12.5}}),"Extrude").isEmpty(),"extrude commit");
    const auto sourceId=document.bodies()[0].bodyId;const auto cap=document.bodies()[0].supportFaces[0];
    QJsonObject plane{{"producerFeatureId",cap.producerFeatureId},{"bodyId",cap.bodyId},{"faceId",cap.faceId}};
    check(document.commit(CadDocument::shellCandidate(document.authored(),{{"targetBodyId",sourceId},{"removedFace",plane},{"thicknessMm",2}}),"Shell").isEmpty(),"shell commit");
    check(close(document.bodies()[0].summary["volumeMm3"].toDouble(),3182),"source shell volume");
    const auto original=document.bodies()[0];const auto beforeMirror=document.authored();
    QJsonObject arguments{{"sourceBodyId",sourceId},{"mirrorPlane",plane}};
    const auto candidate=CadDocument::mirrorCandidate(beforeMirror,arguments);
    check(document.commit(candidate,"Mirror").isEmpty(),"mirror commit");
    check(document.bodies().size()==2 && document.bodies()[0].bodyId==sourceId
        && close(document.bodies()[0].summary["volumeMm3"].toDouble(),original.summary["volumeMm3"].toDouble()),"source body changed");
    const auto mirror=document.bodies()[1];check(mirror.bodyId!=sourceId && mirror.summary["valid"].toBool() && mirror.summary["closed"].toBool(),"new closed body");
    check(close(mirror.summary["volumeMm3"].toDouble(),3182),"mirror volume");
    const auto bounds=mirror.summary["boundsMm"].toObject();
    check(close(bounds["min"].toArray()[2].toDouble(),12.5) && close(bounds["max"].toArray()[2].toDouble(),25),"mirror bounds");
    check(!mirror.supportFaces.empty() && mirror.supportFaces[0].faceId!=cap.faceId && mirror.supportFaces[0].bodyId==mirror.bodyId,"distinct face identity");
    check(close(mirror.supportFaces[0].frame["origin"].toArray()[2].toDouble(),12.5),"mirrored rim frame");
    const auto frame=mirror.supportFaces[0].frame;
    const auto u=frame["uAxis"].toArray(),v=frame["vAxis"].toArray(),n=frame["normal"].toArray();
    const double handed=(u[1].toDouble()*v[2].toDouble()-u[2].toDouble()*v[1].toDouble())*n[0].toDouble()
        +(u[2].toDouble()*v[0].toDouble()-u[0].toDouble()*v[2].toDouble())*n[1].toDouble()
        +(u[0].toDouble()*v[1].toDouble()-u[1].toDouble()*v[0].toDouble())*n[2].toDouble();
    check(close(handed,1),"mirrored face frame must be right handed");
    for(const auto& edge:mirror.solidEdges)for(const auto& sourceEdge:original.solidEdges)check(edge.edgeId!=sourceEdge.edgeId,"distinct edge identity");
    const auto feature=candidate["features"].toArray().last().toObject();
    arguments["featureId"]=feature["id"];check(document.commit(CadDocument::mirrorCandidate(document.authored(),arguments),"Edit Mirror").isEmpty(),"mirror edit");
    check(document.bodies()[1].bodyId==mirror.bodyId && document.bodies()[1].supportFaces[0].faceId==mirror.supportFaces[0].faceId,"edit identity");
    document.history().undo();document.history().redo();check(document.bodies()[1].bodyId==mirror.bodyId,"undo redo mirror");
    const auto extrude=document.authored()["features"].toArray()[1].toObject();
    check(document.commit(CadDocument::extrusionCandidate(document.authored(),{{"featureId",extrude["id"]},
        {"sketchId",sketch["id"]},{"profileId",profile},{"distanceMm",13.5}}),"Edit source depth").isEmpty(),"upstream edit");
    const auto changed=document.bodies()[1].summary["boundsMm"].toObject();
    check(close(changed["min"].toArray()[2].toDouble(),13.5) && close(changed["max"].toArray()[2].toDouble(),27),"mirror follows source plane");
    check(document.bodies()[1].bodyId==mirror.bodyId && document.bodies()[1].supportFaces[0].faceId==mirror.supportFaces[0].faceId,"upstream identity");
    document.history().undo();
    const auto stable=document.authored();const auto revision=document.revision();const auto history=document.history().count();
    arguments["mirrorPlane"]=QJsonObject{{"producerFeatureId",cap.producerFeatureId},{"bodyId",sourceId},{"faceId",CadDocument::newId()}};
    check(!document.commit(CadDocument::mirrorCandidate(stable,arguments),"Missing plane").isEmpty(),"missing plane rejected");
    check(document.authored()==stable && document.revision()==revision && document.history().count()==history,"failure atomic");
    QTemporaryDir dir;check(dir.isValid(),"temporary dir");auto path=dir.filePath("mirror.alloy");
    check(document.save(path).isEmpty(),"save mirror");CadDocument reopen;check(reopen.open(path).isEmpty(),"reopen mirror");
    check(reopen.authored()==stable && reopen.bodies().size()==2 && close(reopen.bodies()[1].summary["volumeMm3"].toDouble(),3182),"reopen exact");
    check(reopen.commit(CadDocument::booleanCandidate(reopen.authored(),{{"operation","join"},
        {"targetBodyId",mirror.bodyId},{"toolBodyIds",QJsonArray{sourceId}},{"keepTools",false}}),"Join mirrored cups").isEmpty(),"combine mirrored cups");
    check(reopen.bodies().size()==1 && close(reopen.bodies()[0].summary["volumeMm3"].toDouble(),6364)
        && reopen.bodies()[0].summary["valid"].toBool(),"joined hollow cube");
    CadDocument plain;auto plainSketch=SketchGeometry::newFeature(plain.authored()["documentId"].toString(),"XY","Plain square");
    plainSketch=SketchGeometry::addRectangle(plainSketch,{{"u1Mm",0},{"v1Mm",0},{"u2Mm",25},{"v2Mm",25}});
    check(plain.commit(SketchGeometry::replaceFeature(plain.authored(),plainSketch),"Square").isEmpty(),"plain sketch");
    const auto plainProfile=plain.sketches()[0].profiles[0].profileId;
    check(plain.commit(CadDocument::extrusionCandidate(plain.authored(),{{"sketchId",plainSketch["id"]},{"profileId",plainProfile},{"distanceMm",12.5}}),"Extrude").isEmpty(),"plain extrude");
    const auto plainCap=plain.bodies()[0].supportFaces[0];
    const QJsonObject plainPlane{{"producerFeatureId",plainCap.producerFeatureId},{"bodyId",plainCap.bodyId},{"faceId",plainCap.faceId}};
    check(plain.commit(CadDocument::mirrorCandidate(plain.authored(),{{"sourceBodyId",plainCap.bodyId},{"mirrorPlane",plainPlane}}),"Mirror plain cap").isEmpty(),"mirror works without Shell");
    check(plain.bodies().size()==2 && close(plain.bodies()[1].summary["volumeMm3"].toDouble(),7812.5),"plain mirror volume");
    std::cout<<"Reflected shell pair passed\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
