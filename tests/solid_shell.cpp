#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <BRepAdaptor_Curve.hxx>

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
    check(document.commit(CadDocument::lengthParameterCandidate(document.authored(),{{"name","Wall"},{"value",2},{"units","mm"}}),"Add Wall").isEmpty(),"Wall parameter created");
    const auto wallId=document.authored()["userParameters"].toArray()[0].toObject()["id"];
    const auto before=document.authored();
    QJsonObject arguments{{"targetBodyId",bodyId},{"removedFace",support},{"thicknessMm",2},{"thicknessParameterId",wallId}};
    const auto candidate=CadDocument::shellCandidate(before,arguments);
    check(document.commit(candidate,"Shell").isEmpty(),"shell commit");
    check(document.bodies().size()==1 && document.bodies()[0].bodyId==bodyId,"body identity");
    check(std::abs(document.bodies()[0].summary["volumeMm3"].toDouble()-3182)<1e-3,"cube shell volume");
    check(document.bodies()[0].summary["valid"].toBool() && document.bodies()[0].summary["closed"].toBool(),"valid closed shell");
    const auto rim=GeometryKernel::resolveSketchSupport(document.evaluation(),support);
    check(rim.role=="shell-rim" && rim.frame["origin"].toArray()[2].toDouble()==12.5,"retained top rim");
    bool innerBase=false;
    for(const auto& face:document.bodies()[0].supportFaces)
        if(face.role=="shell-inner" && std::abs(face.frame["origin"].toArray()[2].toDouble()-2)<1e-5)innerBase=true;
    check(innerBase,"kernel-mapped inner base support");
    check(document.authored()["features"].toArray().last().toObject()["parameters"].toObject()["thicknessParameterId"]==wallId,"Shell retains Wall ID");
    check(document.commit(CadDocument::lengthParameterCandidate(document.authored(),{{"parameterId",wallId},{"name","WallSize"},{"value",2.5}}),"Edit Wall").isEmpty(),"bound Shell rebuilds after rename/value edit");
    check(document.authored()["features"].toArray().last().toObject()["parameters"].toObject()["thicknessParameterId"]==wallId,"Shell binding survives rename");
    const auto boundStable=document.authored();const auto boundRevision=document.revision();
    check(!document.commit(CadDocument::lengthParameterCandidate(document.authored(),{{"parameterId",wallId},{"value",20}}),"Impossible bound Shell").isEmpty(),"bound Shell refuses impossible thickness");
    check(document.authored()==boundStable && document.revision()==boundRevision,"failed bound recompute atomic");
    document.history().undo();check(document.authored()==candidate,"undo Wall edit restores Shell cache");
    const auto shellFeature=document.authored()["features"].toArray().last().toObject();
    arguments["featureId"]=shellFeature["id"];arguments["thicknessMm"]=1.5;arguments.remove("thicknessParameterId");
    check(document.commit(CadDocument::shellCandidate(document.authored(),arguments),"Edit Shell").isEmpty(),"shell edit");
    check(!document.authored()["features"].toArray().last().toObject()["parameters"].toObject().contains("thicknessParameterId"),"numeric Shell edit replaces binding");
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
    CadDocument stud;
    const auto owner=stud.authored()["documentId"].toString();
    auto base=SketchGeometry::newFeature(owner,"XY","Block");
    base=SketchGeometry::addRectangle(base,{{"u1Mm",0},{"v1Mm",0},{"u2Mm",31.8},{"v2Mm",15.8}});
    check(stud.commit(SketchGeometry::replaceFeature(stud.authored(),base),"Block").isEmpty(),"block sketch");
    check(stud.commit(CadDocument::extrusionCandidate(stud.authored(),{{"sketchId",base["id"]},
        {"profileId",stud.sketches()[0].profiles[0].profileId},{"distanceMm",9.6}}),"Block").isEmpty(),"block extrude");
    const auto block=stud.bodies()[0];
    const auto roofSource=*std::find_if(block.supportFaces.begin(),block.supportFaces.end(),[](const auto& face){return face.role=="extrude.endCap";});
    auto topSketch=SketchGeometry::newSupportedFeature(owner,{{"producerFeatureId",roofSource.producerFeatureId},
        {"bodyId",roofSource.bodyId},{"faceId",roofSource.faceId}},"Stud",{});
    topSketch=SketchGeometry::addCircle(topSketch,{{"centerUMm",3.9},{"centerVMm",3.9},{"diameterMm",5}});
    check(stud.commit(SketchGeometry::replaceFeature(stud.authored(),topSketch),"Stud").isEmpty(),"stud sketch");
    check(stud.commit(CadDocument::extrusionCandidate(stud.authored(),{{"sketchId",topSketch["id"]},
        {"profileId",stud.sketches().back().profiles[0].profileId},{"distanceMm",1.7},{"operation","join"},
        {"targetBodyIds",QJsonArray{block.bodyId}}}),"Stud").isEmpty(),"stud join");
    const auto beforeStudShell=stud.bodies()[0];
    const auto bottom=*std::find_if(beforeStudShell.supportFaces.begin(),beforeStudShell.supportFaces.end(),[](const auto& face){return face.role=="extrude.base";});
    check(stud.commit(CadDocument::shellCandidate(stud.authored(),{{"targetBodyId",block.bodyId},
        {"removedFace",QJsonObject{{"producerFeatureId",bottom.producerFeatureId},{"bodyId",bottom.bodyId},{"faceId",bottom.faceId}}},
        {"thicknessMm",1.49}}),"Shell" ).isEmpty(),"stud bottom shell");
    const auto roof=std::find_if(stud.bodies()[0].supportFaces.begin(),stud.bodies()[0].supportFaces.end(),[](const auto& face){
        return face.role=="shell-inner" && std::abs(face.frame["origin"].toArray()[2].toDouble()-8.11)<1e-5;
    });
    check(roof!=stud.bodies()[0].supportFaces.end(),"source-mapped inner roof");
    check(roof->boundaryEdges.size()==1,"one actual inner circular mouth");
    check(roof->frame["normal"].toArray()[2].toDouble()==-1,"inner roof outward normal");
    const BRepAdaptor_Curve mouth(roof->boundaryEdges[0].edge);
    check(mouth.GetType()==GeomAbs_Circle && std::abs(mouth.Circle().Radius()-1.01)<1e-5,"inner circular mouth radius");
    const QJsonObject roofRef{{"producerFeatureId",roof->producerFeatureId},{"bodyId",roof->bodyId},{"faceId",roof->faceId}};
    auto roofSketch=SketchGeometry::newSupportedFeature(owner,roofRef,"Inner roof",{});
    const auto centerContext=GeometryKernel::projectionContext(stud.evaluation(),roofSketch);
    check(centerContext["points"].toArray().size()==1,"one retained inner mouth center");
    const auto center=centerContext["points"].toArray()[0].toObject();
    check(std::abs(center["uMm"].toDouble()-3.9)<1e-5 && std::abs(center["vMm"].toDouble()+3.9)<1e-5,"mouth center in roof frame");
    const auto snapped=SketchGeometry::addPointerCircle(roofSketch,{{"centerUMm",0},{"centerVMm",0},
        {"diameterMm",6.3},{"centerSnapTarget",QJsonObject{{"kind","point"},{"pointId",center["id"]}}}},centerContext);
    check(snapped.ok,"snap circle to inner mouth center");roofSketch=snapped.feature;
    check(roofSketch["parameters"].toObject()["pointReferences"].toArray().size()==1,"retained mouth center reference");
    check(stud.commit(SketchGeometry::replaceFeature(stud.authored(),roofSketch),"Roof sketch").isEmpty(),"commit roof sketch");
    const auto pointReferences=roofSketch["parameters"].toObject()["pointReferences"].toArray();
    const auto shellId=stud.authored()["features"].toArray()[4].toObject()["id"].toString();
    check(!shellId.isEmpty(),"shell feature ID");
    check(stud.commit(CadDocument::shellCandidate(stud.authored(),{{"featureId",shellId},
        {"targetBodyId",block.bodyId},{"removedFace",QJsonObject{{"producerFeatureId",bottom.producerFeatureId},{"bodyId",bottom.bodyId},{"faceId",bottom.faceId}}},
        {"thicknessMm",1.3}}),"Edit stud shell").isEmpty(),"edit shell with dependent center sketch");
    const auto moved=GeometryKernel::projectionContext(stud.evaluation(),roofSketch)["points"].toArray()[0].toObject();
    check(moved["id"]==center["id"] && moved["source"]==center["source"] && moved["retained"].toBool(),"stable retained center identity");
    const auto changedRoof=GeometryKernel::resolveSketchSupport(stud.evaluation(),roofRef);
    check(std::abs(changedRoof.frame["origin"].toArray()[2].toDouble()-8.3)<1e-5,"roof follows thickness edit");
    check(stud.sketches().back().summary["valid"].toBool(),"dependent sketch evaluates after shell edit");
    const auto edited=stud.authored();stud.history().undo();stud.history().redo();
    check(stud.authored()==edited && stud.sketches().back().summary["valid"].toBool(),"shell edit undo redo");
    const auto studPath=directory.filePath("stud-roof.alloy");
    check(stud.save(studPath).isEmpty(),"save roof sketch");CadDocument restored;
    check(restored.open(studPath).isEmpty() && restored.authored()==edited,"reopen roof sketch");
    check(SketchGeometry::findFeature(restored.authored(),roofSketch["id"].toString())["parameters"].toObject()["pointReferences"].toArray()==pointReferences,"reopen retained center");
    check(GeometryKernel::projectionContext(restored.evaluation(),roofSketch)["points"].toArray()[0].toObject()["id"]==center["id"],"reopen center identity");
    auto missing=roofSketch;auto missingParameters=missing["parameters"].toObject();
    auto missingReferences=missingParameters["pointReferences"].toArray();
    auto missingReference=missingReferences[0].toObject();auto missingSource=missingReference["source"].toObject();
    missingSource["occurrenceId"]=CadDocument::newId();
    missingReference["source"]=missingSource;
    missingReferences[0]=missingReference;missingParameters["pointReferences"]=missingReferences;missing["parameters"]=missingParameters;
    const auto safe=restored.authored();const auto safeRevision=restored.revision();const auto safeHistory=restored.history().count();
    check(!restored.commit(SketchGeometry::replaceFeature(safe,missing),"Missing occurrence").isEmpty()
        && restored.lastEvaluationError()["code"]=="projection_unavailable","missing occurrence did not fail explicitly");
    check(restored.authored()==safe && restored.revision()==safeRevision && restored.history().count()==safeHistory,"missing occurrence changed accepted work");
    std::cout<<"One-face sharp inward Shell document and geometry passed\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
