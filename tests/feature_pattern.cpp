// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <BRepAdaptor_Curve.hxx>
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QDir>
#include <iostream>
#include <cmath>
#include <numbers>
#include <algorithm>
#include <stdexcept>
namespace {
void check(bool okay,const QString& message){if(!okay)throw std::runtime_error(message.toStdString());}
void append(CadDocument& d,const QJsonObject& f){auto a=d.authored();auto fs=a["features"].toArray();fs.append(f);a["features"]=fs;check(d.commit(a,"Append").isEmpty(),d.lastEvaluationError()["message"].toString());}
QJsonObject axis(const EvaluatedBody& body,int coordinate,int quantity,double extent){for(const auto& edge:body.solidEdges)if(!edge.edge.IsNull()){BRepAdaptor_Curve c(edge.edge);if(c.GetType()!=GeomAbs_Line)continue;gp_Vec vector(c.Value(c.FirstParameter()),c.Value(c.LastParameter()));const double component=coordinate==0 ? vector.X() : vector.Y();if(std::abs(component)>1 && std::abs(std::abs(component)-vector.Magnitude())<1e-6)return {{"edgeId",edge.edgeId},{"quantity",quantity},{"extentMm",std::copysign(extent,component)}};}throw std::runtime_error("Missing straight axis edge");}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument d;const auto owner=d.authored()["documentId"].toString();
    auto base=SketchGeometry::newFeature(owner,"XY","Base");base=SketchGeometry::addRectangle(base,{{"u1Mm",0},{"v1Mm",0},{"u2Mm",31.8},{"v2Mm",15.8}});append(d,base);
    check(d.commit(CadDocument::extrusionCandidate(d.authored(),{{"sketchId",base["id"]},{"profileId",d.sketches()[0].profiles[0].profileId},{"distanceMm",9.6}}),"Base extrude").isEmpty(),"Base extrude");
    auto body=d.bodies()[0];const auto target=body.bodyId;const auto cap=body.supportFaces[0];const QJsonObject support{{"producerFeatureId",cap.producerFeatureId},{"bodyId",cap.bodyId},{"faceId",cap.faceId}};
    auto stud=SketchGeometry::newSupportedFeature(owner,support,"Stud");stud=SketchGeometry::addCircle(stud,{{"centerUMm",3.9},{"centerVMm",3.9},{"diameterMm",5}});append(d,stud);
    check(d.commit(CadDocument::extrusionCandidate(d.authored(),{{"sketchId",stud["id"]},{"profileId",d.sketches().back().profiles[0].profileId},{"distanceMm",1.7},{"operation","join"},{"targetBodyIds",QJsonArray{target}}}),"Stud join").isEmpty(),"Stud Join");
    const auto seed=d.authored()["features"].toArray().last().toObject();body=d.bodies()[0];QJsonArray axes{axis(body,0,4,24),axis(body,1,2,8)};
    check(d.commit(CadDocument::patternCandidate(d.authored(),{{"sourceFeatureId",seed["id"]},{"targetBodyId",target},{"axes",axes}}),"Stud pattern").isEmpty(),"Stud pattern failed: "+d.lastEvaluationError()["message"].toString());
    body=d.bodies()[0];check(d.bodies().size()==1 && body.bodyId==target && body.summary["instanceCount"]==8,"Pattern body/instance count");
    check(std::abs(body.summary["volumeMm3"].toDouble()-(31.8*15.8*9.6+8*std::numbers::pi*2.5*2.5*1.7))<1e-5,"Whole-body duplication or wrong stud count");
    QSet<QString> topRims;for(const auto& e:body.solidEdges)if(!e.edge.IsNull() && e.provenance["sourceSketchId"]==stud["id"] && e.provenance["role"]=="endRim")topRims.insert(e.edgeId);check(topRims.size()==8,"Eight retained stud rim IDs");
    int baseRims=0,occurrences=0;for(const auto& e:body.solidEdges)if(!e.edge.IsNull() && e.provenance["sourceSketchId"]==stud["id"] && e.provenance["role"]=="baseRim"){++baseRims;if(!e.provenance["occurrenceId"].toString().isEmpty())++occurrences;}
    check(baseRims==8 && occurrences==7,"Eight base rims with explicit copy occurrences required for Shell roof lineage");
    if(argc>1){QDir output(QString::fromLocal8Bit(argv[1]));check(output.mkpath("."),"Fixture output directory");check(d.save(output.filePath("stud-pattern.alloy")).isEmpty(),"Stud fixture save");}
    const auto patterned=d.authored();const auto pattern=patterned["features"].toArray().last().toObject();auto bad=axes;auto a=bad[0].toObject();a["edgeId"]=CadDocument::newId();bad[0]=a;const auto revision=d.revision();check(!d.commit(CadDocument::patternCandidate(d.authored(),{{"featureId",pattern["id"]},{"sourceFeatureId",seed["id"]},{"targetBodyId",target},{"axes",bad}}),"Missing axis").isEmpty() && d.revision()==revision && d.authored()==patterned,"Missing axis atomic refusal");
    check(d.commit(CadDocument::extrusionCandidate(d.authored(),{{"featureId",seed["id"]},{"sketchId",stud["id"]},{"profileId",d.sketches().back().profiles[0].profileId},{"distanceMm",2},{"operation","join"},{"targetBodyIds",QJsonArray{target}}}),"Edit seed").isEmpty(),"Seed edit rebuild");
    check(std::abs(d.bodies()[0].summary["volumeMm3"].toDouble()-(31.8*15.8*9.6+8*std::numbers::pi*2.5*2.5*2))<1e-5,"Seed edit did not rebuild every stud");
    QSet<QString> editedRims;for(const auto& e:d.bodies()[0].solidEdges)if(!e.edge.IsNull() && e.provenance["sourceSketchId"]==stud["id"] && e.provenance["role"]=="endRim")editedRims.insert(e.edgeId);check(editedRims==topRims,"Instance rim IDs changed");d.history().undo();check(d.authored()==patterned,"Exact seed undo");d.history().redo();
    d.history().undo();body=d.bodies()[0];
    const auto bottom=std::find_if(body.supportFaces.begin(),body.supportFaces.end(),[](const auto& face){return face.role=="extrude.base";});check(bottom!=body.supportFaces.end(),"Retained bottom face missing");
    const QJsonObject bottomRef{{"producerFeatureId",bottom->producerFeatureId},{"bodyId",target},{"faceId",bottom->faceId}};
    check(d.commit(CadDocument::shellCandidate(d.authored(),{{"targetBodyId",target},{"removedFace",bottomRef},{"thicknessMm",1.49}}),"Shell").isEmpty(),"Stud pattern Shell failed: "+d.lastEvaluationError()["message"].toString());
    const auto& shelled=d.bodies()[0];
    const auto roof=std::find_if(shelled.supportFaces.begin(),shelled.supportFaces.end(),[](const auto& face){
        return face.role=="shell-inner" && std::abs(face.frame["origin"].toArray()[2].toDouble()-8.11)<1e-5;
    });
    check(roof!=shelled.supportFaces.end() && roof->frame["normal"].toArray()[2].toDouble()==-1,"named inner roof at z8.11 normal -Z");
    check(roof->boundaryEdges.size()==8,"eight source-mapped roof mouths");
    const QJsonObject roofRef{{"producerFeatureId",roof->producerFeatureId},{"bodyId",roof->bodyId},{"faceId",roof->faceId}};
    auto recess=SketchGeometry::newSupportedFeature(owner,roofRef,"Recess centers");
    const auto centers=GeometryKernel::projectionContext(d.evaluation(),recess)["points"].toArray();
    check(centers.size()==8,"eight distinct occurrence-aware recess centers");
    QSet<QString> centerIds,centerOccurrences;QJsonObject first,diagonal;
    for(const auto& value:centers){const auto p=value.toObject();centerIds.insert(p["id"].toString());
        const auto s=p["source"].toObject();if(s.contains("occurrenceId"))centerOccurrences.insert(s["occurrenceId"].toString());
        const double u=p["uMm"].toDouble(),v=p["vMm"].toDouble();
        if(std::abs(u-3.9)<1e-5 && std::abs(v+3.9)<1e-5)first=p;
        if(std::abs(u-11.9)<1e-5 && std::abs(v+11.9)<1e-5)diagonal=p;
    }
    check(centerIds.size()==8 && centerOccurrences.size()==7,"seed plus seven distinct occurrence center identities");
    check(!first.isEmpty() && !diagonal.isEmpty() && first["id"]!=diagonal["id"],"tutorial diagonal recess centers");
    auto line=SketchGeometry::addPointerEntity(recess,"line",{{"u1Mm",3.9},{"v1Mm",-3.9},{"u2Mm",11.9},{"v2Mm",-11.9},
        {"startSnapTarget",QJsonObject{{"kind","point"},{"pointId",first["id"]}}},
        {"endSnapTarget",QJsonObject{{"kind","point"},{"pointId",diagonal["id"]}}}},GeometryKernel::projectionContext(d.evaluation(),recess));
    check(line.ok,"retained diagonal from roof centers");recess=line.feature;
    const auto lineId=recess["parameters"].toObject()["entities"].toArray().last().toObject()["id"].toString();
    auto construction=SketchGeometry::setConstruction(recess,{{"entityIds",QJsonArray{lineId}},{"construction",true}},GeometryKernel::projectionContext(d.evaluation(),recess));
    check(construction.ok,"construction diagonal");recess=construction.feature;
    auto circle=SketchGeometry::addPointerCircle(recess,{{"centerUMm",7.9},{"centerVMm",-7.9},{"diameterMm",6.3},
        {"centerSnapTarget",QJsonObject{{"kind","midpoint"},{"entityId",lineId}}}},GeometryKernel::projectionContext(d.evaluation(),recess));
    check(circle.ok,"midpoint recess circle");recess=circle.feature;
    check(recess["parameters"].toObject()["pointReferences"].toArray().size()==2,"two retained occurrence center references");
    const auto midpointCircle=recess["parameters"].toObject()["entities"].toArray().last().toObject();
    check(midpointCircle["type"]=="circle" && !midpointCircle["construction"].toBool(),"nonconstruction recess circle");
    bool midpointRelation=false;for(const auto& value:recess["parameters"].toObject()["constraints"].toArray()){
        const auto c=value.toObject();if(c["type"]=="midpoint" && c["entityIds"].toArray().contains(lineId))midpointRelation=true;
    }
    check(midpointRelation,"retained diagonal midpoint relation");
    check(d.commit(SketchGeometry::replaceFeature(d.authored(),recess),"Recess sketch").isEmpty(),"commit diagonal midpoint circle");
    const auto circleCenterId=recess["parameters"].toObject()["entities"].toArray().last().toObject()["centerPointId"].toString();
    const auto expectedReferences=recess["parameters"].toObject()["pointReferences"].toArray();
    auto checkMidpoint=[&](double expectedU){
        const auto context=GeometryKernel::projectionContext(d.evaluation(),recess);
        const auto solved=SketchGeometry::solveDraft(recess,context);check(solved.ok,"recess sketch re-solve");
        bool found=false;for(const auto& value:solved.feature["parameters"].toObject()["points"].toArray()){
            const auto p=value.toObject();if(p["id"].toString()!=circleCenterId)continue;
            check(std::abs(p["uMm"].toDouble()-expectedU)<1e-5 && std::abs(p["vMm"].toDouble()+7.9)<1e-5,"midpoint circle did not follow referenced diagonal");found=true;
        }
        check(found,"midpoint circle center unavailable");
        check(SketchGeometry::findFeature(d.authored(),recess["id"].toString())["parameters"].toObject()["pointReferences"].toArray()==expectedReferences,"authored occurrence references changed");
    };
    checkMidpoint(7.9);
    const auto shellFeature=d.authored()["features"].toArray()[5].toObject();
    check(shellFeature["type"]=="shell","shell feature history position");
    check(d.commit(CadDocument::shellCandidate(d.authored(),{{"featureId",shellFeature["id"]},{"targetBodyId",target},
        {"removedFace",bottomRef},{"thicknessMm",1.3}}),"Roof thickness edit").isEmpty(),"Shell thickness edit with retained roof sketch");
    check(std::abs(GeometryKernel::resolveSketchSupport(d.evaluation(),roofRef).frame["origin"].toArray()[2].toDouble()-8.3)<1e-5,"roof height after thickness edit");
    checkMidpoint(7.9);d.history().undo();checkMidpoint(7.9);
    auto widerAxes=axes;auto wideAxis=widerAxes[0].toObject();wideAxis["extentMm"]=std::copysign(23.5,wideAxis["extentMm"].toDouble());widerAxes[0]=wideAxis;
    const auto pitchError=d.commit(CadDocument::patternCandidate(d.authored(),{{"featureId",pattern["id"]},{"sourceFeatureId",seed["id"]},
        {"targetBodyId",target},{"axes",widerAxes}}),"Stud pitch edit");
    check(pitchError.isEmpty(),"Pattern pitch edit with retained roof sketch: "+pitchError);
    checkMidpoint(7.9-0.5/6.0);d.history().undo();checkMidpoint(7.9);
    auto removalProbe=SketchGeometry::newSupportedFeature(owner,roofRef,"Occurrence removal probe");
    QJsonObject lastCenter;const auto removalContext=GeometryKernel::projectionContext(d.evaluation(),removalProbe);
    for(const auto& value:removalContext["points"].toArray()){
        const auto p=value.toObject();if(std::abs(p["uMm"].toDouble()-27.9)<1e-5 && std::abs(p["vMm"].toDouble()+11.9)<1e-5)lastCenter=p;
    }
    check(!lastCenter.isEmpty() && lastCenter["source"].toObject().contains("occurrenceId"),"last occurrence center");
    auto probeCircle=SketchGeometry::addPointerCircle(removalProbe,{{"centerUMm",0},{"centerVMm",0},{"diameterMm",1},
        {"centerSnapTarget",QJsonObject{{"kind","point"},{"pointId",lastCenter["id"]}}}},removalContext);
    check(probeCircle.ok,"retain final occurrence reference");removalProbe=probeCircle.feature;
    check(d.commit(SketchGeometry::replaceFeature(d.authored(),removalProbe),"Occurrence probe").isEmpty(),"commit occurrence probe");
    auto fewerAxes=axes;auto fewerX=fewerAxes[0].toObject();fewerX["quantity"]=3;fewerAxes[0]=fewerX;
    const auto protectedDocument=d.authored();const auto protectedRevision=d.revision();const auto protectedHistory=d.history().count();
    const auto fewerError=d.commit(CadDocument::patternCandidate(d.authored(),{{"featureId",pattern["id"]},{"sourceFeatureId",seed["id"]},
        {"targetBodyId",target},{"axes",fewerAxes}}),"Remove referenced occurrence");
    check(!fewerError.isEmpty() && d.lastEvaluationError()["code"]=="projection_unavailable","missing pattern occurrence did not refuse dependent sketch");
    check(d.authored()==protectedDocument && d.revision()==protectedRevision && d.history().count()==protectedHistory,"missing occurrence refusal changed accepted work");
    d.history().undo();bool probePresent=false;for(const auto& value:d.authored()["features"].toArray())if(value.toObject()["id"]==removalProbe["id"])probePresent=true;
    check(!probePresent,"remove probe from tutorial continuation");
    // Place the tube sketch at the source roof height with its downward normal;
    // actual roof selection/center snaps are qualified by the separate roof feature.
    const auto planeId=CadDocument::newId();append(d,{{"id",planeId},{"ownerId",owner},{"type","constructionPlane"},{"name","Tube plane"},{"visible",true},{"parameters",QJsonObject{{"source",QJsonObject{{"support",bottomRef}}},{"offsetMm",-8.11}}}});
    auto tube=SketchGeometry::newConstructionPlaneFeature(owner,planeId,"Tube");tube=SketchGeometry::addCircle(tube,{{"centerUMm",7.9},{"centerVMm",-7.9},{"diameterMm",6.3}});tube=SketchGeometry::addCircle(tube,{{"centerUMm",7.9},{"centerVMm",-7.9},{"diameterMm",4.3}});append(d,tube);
    const auto outerEntity=tube["parameters"].toObject()["entities"].toArray()[0].toObject()["id"].toString();QString annulus;for(const auto& region:d.sketches().back().profiles)if(std::abs(region.summary["areaMm2"].toDouble()-std::numbers::pi*(3.15*3.15-2.15*2.15))<1e-6)annulus=region.profileId;check(!annulus.isEmpty(),"Tube annular region");
    check(d.commit(CadDocument::extrusionCandidate(d.authored(),{{"sketchId",tube["id"]},{"profileId",annulus},{"distanceMm",8.1},{"operation","join"},{"targetBodyIds",QJsonArray{target}}}),"Tube Join").isEmpty(),"Tube Join failed: "+d.lastEvaluationError()["message"].toString());
    const auto tubeSeed=d.authored()["features"].toArray().last().toObject();const auto beforeTubePattern=d.bodies()[0].summary["volumeMm3"].toDouble();
    check(d.commit(CadDocument::patternCandidate(d.authored(),{{"sourceFeatureId",tubeSeed["id"]},{"targetBodyId",target},{"axes",QJsonArray{axis(d.bodies()[0],0,3,16)}}}),"Tube pattern").isEmpty(),"Tube pattern failed: "+d.lastEvaluationError()["message"].toString());
    body=d.bodies()[0];check(body.summary["instanceCount"]==3 && std::abs(body.summary["volumeMm3"].toDouble()-beforeTubePattern-2*std::numbers::pi*(3.15*3.15-2.15*2.15)*8.1)<1e-5,"Three hollow tubes material");
    QJsonArray filletEdges;int studCount=0,baseCount=0,tubeCount=0;
    for(const auto& edge:body.solidEdges)if(!edge.edge.IsNull()) {
        const auto provenance=edge.provenance;bool selected=false;
        if(provenance["sourceSketchId"]==stud["id"] && provenance["role"]=="endRim"){selected=true;++studCount;}
        if(provenance["sourceSketchId"]==base["id"] && (provenance["role"]=="endRim" || provenance["role"]=="sideCorner")){selected=true;++baseCount;}
        if(provenance["sourceSketchId"]==tube["id"] && provenance["sourceEntityId"]==outerEntity && provenance["role"]=="endRim"){selected=true;++tubeCount;}
        if(selected)filletEdges.append(edge.edgeId);
    }
    check(studCount==8 && baseCount==8 && tubeCount==3 && filletEdges.size()==19,"Nineteen source-defined final edge IDs");
    check(d.commit(CadDocument::filletCandidate(d.authored(),{{"targetBodyId",target},{"edgeIds",filletEdges},{"radiusMm",0.2},{"tangentChain",true}}),"Fillet 19").isEmpty(),"Final nineteen-edge0.2 Fillet failed: "+d.lastEvaluationError()["message"].toString());
    check(d.bodies().size()==1 && d.bodies()[0].summary["valid"].toBool() && d.bodies()[0].summary["closed"].toBool(),"Final valid closed one body");
    QTemporaryDir temp;const auto path=temp.filePath("pattern.alloy");check(d.save(path).isEmpty(),"Pattern save");CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==d.authored(),"Exact pattern reopen");
    check(SketchGeometry::findFeature(reopened.authored(),recess["id"].toString())["parameters"].toObject()["pointReferences"].toArray()==expectedReferences,"Reopen lost occurrence-aware center references");
    std::cout<<"Eight stud instances and inner roof centers, retained diagonal/midpoint, edit/history/reopen and missing occurrence refusal passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
