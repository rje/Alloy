#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <stdexcept>

static void check(bool okay,const char* message){if(!okay)throw std::runtime_error(message);}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument document;
    auto sketch=SketchGeometry::newFeature(document.authored()["documentId"].toString(),"XY","Cube base");
    sketch=SketchGeometry::addRectangle(sketch,{{"u1Mm",0},{"v1Mm",0},{"u2Mm",25},{"v2Mm",25}});
    check(document.commit(SketchGeometry::replaceFeature(document.authored(),sketch),"Rectangle").isEmpty(),"sketch commit");
    const auto profile=document.sketches()[0].profiles[0].profileId;
    check(document.commit(CadDocument::extrusionCandidate(document.authored(),{{"sketchId",sketch["id"]},
        {"profileId",profile},{"distanceMm",12.5}}),"Extrude").isEmpty(),"extrude commit");
    const auto bodyId=document.bodies()[0].bodyId;
    const auto cap=document.bodies()[0].supportFaces[0];
    QJsonObject support{{"producerFeatureId",cap.producerFeatureId},{"bodyId",cap.bodyId},{"faceId",cap.faceId}};
    const auto before=document.authored();
    QJsonObject arguments{{"targetBodyId",bodyId},{"removedFace",support},{"thicknessMm",2}};
    const auto candidate=CadDocument::shellCandidate(before,arguments);
    check(document.commit(candidate,"Shell").isEmpty(),"shell commit");
    check(document.bodies().size()==1 && document.bodies()[0].bodyId==bodyId,"body identity");
    check(std::abs(document.bodies()[0].summary["volumeMm3"].toDouble()-3182)<1e-3,"cube shell volume");
    check(document.bodies()[0].summary["valid"].toBool() && document.bodies()[0].summary["closed"].toBool(),"valid closed shell");
    const auto rim=GeometryKernel::resolveSketchSupport(document.evaluation(),support);
    check(rim.role=="shell-rim" && rim.frame["origin"].toArray()[2].toDouble()==12.5,"retained top rim");
    const auto shellFeature=document.authored()["features"].toArray().last().toObject();
    arguments["featureId"]=shellFeature["id"];arguments["thicknessMm"]=1.5;
    check(document.commit(CadDocument::shellCandidate(document.authored(),arguments),"Edit Shell").isEmpty(),"shell edit");
    check(std::abs(document.bodies()[0].summary["volumeMm3"].toDouble()-(7812.5-22*22*11))<1e-3,"edited thickness volume");
    const auto extrude=document.authored()["features"].toArray()[1].toObject();
    check(document.commit(CadDocument::extrusionCandidate(document.authored(),{{"featureId",extrude["id"]},
        {"sketchId",sketch["id"]},{"profileId",profile},{"distanceMm",13.5}}),"Resize source").isEmpty(),"upstream edit");
    check(std::abs(document.bodies()[0].summary["volumeMm3"].toDouble()-(25*25*13.5-22*22*12))<1e-3,"upstream shell rebuild");
    check(GeometryKernel::resolveSketchSupport(document.evaluation(),support).frame["origin"].toArray()[2].toDouble()==13.5,"rim follows upstream edit");
    document.history().undo();
    document.history().undo();check(document.authored()==candidate,"shell edit undo");
    document.history().redo();check(document.authored()!=candidate,"shell edit redo");
    const auto stable=document.authored();const auto revision=document.revision();const auto history=document.history().count();
    arguments["removedFace"]=QJsonObject{{"producerFeatureId",cap.producerFeatureId},{"bodyId",bodyId},{"faceId",CadDocument::newId()}};
    check(!document.commit(CadDocument::shellCandidate(stable,arguments),"Missing face").isEmpty(),"missing face rejected");
    arguments["removedFace"]=support;arguments["thicknessMm"]=20;
    check(!document.commit(CadDocument::shellCandidate(stable,arguments),"Impossible thickness").isEmpty(),"impossible thickness rejected");
    check(document.authored()==stable && document.revision()==revision && document.history().count()==history,"failure atomic");
    QTemporaryDir directory;check(directory.isValid(),"temporary directory");const auto path=directory.filePath("shell.alloy");
    check(document.save(path).isEmpty(),"save shell");CadDocument reopened;check(reopened.open(path).isEmpty(),"reopen shell");
    check(reopened.authored()==stable && std::abs(reopened.bodies()[0].summary["volumeMm3"].toDouble()-
        document.bodies()[0].summary["volumeMm3"].toDouble())<1e-6,"reopened exact shell");
    std::cout<<"One-face sharp inward Shell document and geometry passed\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
