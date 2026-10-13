// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <numbers>

namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void near(double actual,double expected,double tolerance=1e-5){check(std::abs(actual-expected)<tolerance,"Hole analytic geometry differs");}
QJsonObject solved(const SketchGeometry::SolveResult& result){if(!result.ok)throw std::runtime_error(result.error.toStdString());return result.feature;}
void commit(CadDocument& doc,const QJsonObject& candidate){const auto error=doc.commit(candidate,"Hole fixture");if(!error.isEmpty())throw std::runtime_error(error.toStdString());}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument doc;auto base=solved(SketchGeometry::addCenterRectangle(
        SketchGeometry::newFeature(doc.authored()["documentId"].toString(),"XY"),
        {{"centerUMm",0},{"centerVMm",0},{"widthMm",75},{"heightMm",50}}));
    const auto widthId=base["parameters"].toObject()["dimensions"].toArray()[0].toObject()["id"].toString();
    commit(doc,SketchGeometry::replaceFeature(doc.authored(),base));
    const auto profileId=doc.sketches()[0].profiles[0].profileId;
    commit(doc,CadDocument::extrusionCandidate(doc.authored(),{{"sketchId",base["id"]},{"profileId",profileId},{"distanceMm",10}}));
    auto extrusion=doc.authored()["features"].toArray().last().toObject();
    const auto target=doc.bodies()[0].bodyId;QJsonArray corners;
    for(const auto& edge:doc.bodies()[0].solidEdges)if(edge.provenance["role"]=="sideCorner")corners.append(edge.edgeId);
    check(corners.size()==4,"Four vertical edges required");
    commit(doc,CadDocument::filletCandidate(doc.authored(),{{"targetBodyId",target},{"edgeIds",corners},{"radiusMm",5}}));
    EvaluatedSupportFace cap;bool found=false;
    for(const auto& face:doc.bodies()[0].supportFaces)if(face.role=="extrude.endCap"){cap=face;found=true;break;}
    check(found,"Rounded top face required");
    const QJsonObject support{{"producerFeatureId",cap.producerFeatureId},{"bodyId",target},{"faceId",cap.faceId}};
    auto pointSketch=SketchGeometry::newSupportedFeature(doc.authored()["documentId"].toString(),support,"Hole point",GeometryKernel::supportEdgeDescriptors(cap));
    const auto context=GeometryKernel::projectionContext(doc.evaluation(),pointSketch);
    pointSketch=solved(SketchGeometry::addStandalonePoint(pointSketch,{{"uMm",-27.5},{"vMm",15}},context));
    const auto point=pointSketch["parameters"].toObject()["entities"].toArray()[0].toObject();
    QString left,upper;
    for(const auto& value:context["lines"].toArray()) {
        const auto line=value.toObject();
        if(std::abs(line["u1Mm"].toDouble()+37.5)<1e-6 && std::abs(line["u2Mm"].toDouble()+37.5)<1e-6)left=line["id"].toString();
        if(std::abs(line["v1Mm"].toDouble()-25)<1e-6 && std::abs(line["v2Mm"].toDouble()-25)<1e-6)upper=line["id"].toString();
    }
    check(!left.isEmpty() && !upper.isEmpty(),"Two retained adjacent rim references");
    for(const auto& edge:{left,upper})pointSketch=solved(SketchGeometry::addDimension(pointSketch,
        {{"kind","pointLineDistance"},{"pointIds",QJsonArray{point["pointId"]}},{"entityId",edge},{"valueMm",10}},context));
    commit(doc,SketchGeometry::replaceFeature(doc.authored(),pointSketch));
    QJsonObject args{{"sourceSketchId",pointSketch["id"]},{"sourceEntityId",point["id"]},{"sourcePointId",point["pointId"]},
        {"targetBodyId",target},{"placement","fromSketch"},{"holeType","countersink"},{"extent","all"},
        {"boreDiameterMm",4},{"sinkDiameterMm",10},{"includedAngleDeg",90}};
    const auto beforeHole=doc.authored();
    commit(doc,CadDocument::holeCandidate(doc.authored(),args));const auto created=doc.authored();
    const auto holeId=created["features"].toArray().last().toObject()["id"].toString();
    check(doc.bodies().size()==1 && doc.bodies()[0].bodyId==target,"Hole modifies its retained target");
    auto summary=doc.bodies()[0].summary;
    near(summary["sinkDepthMm"].toDouble(),3);near(summary["removedVolumeMm3"].toDouble(),67*std::numbers::pi);
    near(summary["volumeMm3"].toDouble(),36500+183*std::numbers::pi);
    check(summary["valid"].toBool() && summary["closed"].toBool() && summary["sourcePointId"]==point["pointId"],"Valid retained through Hole");
    doc.history().undo();check(doc.authored()==beforeHole,"Exact Hole undo");doc.history().redo();check(doc.authored()==created,"Exact Hole redo");
    args["featureId"]=holeId;args["boreDiameterMm"]=5;commit(doc,CadDocument::holeCandidate(doc.authored(),args));
    near(doc.bodies()[0].summary["sinkDepthMm"].toDouble(),2.5);check(doc.authored()["features"].toArray().last().toObject()["id"]==holeId,"Edit retains Hole ID");
    args["boreDiameterMm"]=4;commit(doc,CadDocument::holeCandidate(doc.authored(),args));
    extrusion["parameters"]=QJsonObject{{"sketchId",base["id"]},{"profileId",profileId},{"distanceMm",12}};
    commit(doc,CadDocument::extrusionCandidate(doc.authored(),{{"featureId",extrusion["id"]},{"sketchId",base["id"]},{"profileId",profileId},{"distanceMm",12}}));
    near(doc.bodies()[0].summary["removedVolumeMm3"].toDouble(),75*std::numbers::pi);
    near(doc.bodies()[0].summary["entryMm"].toArray()[2].toDouble(),12);
    base=solved(SketchGeometry::updateDimension(base,{{"dimensionId",widthId},{"valueMm",85}}));
    commit(doc,SketchGeometry::replaceFeature(doc.authored(),base));
    near(doc.bodies()[0].summary["entryMm"].toArray()[0].toDouble(),-32.5);
    const auto stable=doc.authored();const auto revision=doc.revision();
    bool invalid=false;try{auto bad=args;bad["sinkDiameterMm"]=2;CadDocument::holeCandidate(stable,bad);}catch(const std::exception&){invalid=true;}
    check(invalid && doc.revision()==revision && doc.authored()==stable,"Invalid Hole edit atomic");
    auto missing=args;missing["sourcePointId"]=CadDocument::newId();
    const auto badPoint=CadDocument::holeCandidate(stable,missing);
    check(!doc.commit(badPoint,"Missing point").isEmpty() && doc.revision()==revision && doc.authored()==stable,
        "Missing retained point refuses atomically");
    const auto diagnostic=doc.lastEvaluationError();
    check(diagnostic["featureId"]==holeId && diagnostic["featureType"]=="hole"
        && diagnostic["sourceSketchId"]==pointSketch["id"] && diagnostic["sourceEntityId"]==point["id"]
        && diagnostic["sourcePointId"]==missing["sourcePointId"] && diagnostic["targetBodyId"]==target,
        "Hole refusal attributes retained source and target");
    QTemporaryDir temp;const auto path=temp.filePath("hole.alloy");check(doc.save(path).isEmpty(),"Save Hole document");
    CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==stable,"Exact Hole reopen");
    near(reopened.bodies()[0].summary["entryMm"].toArray()[0].toDouble(),-32.5);
    // A cap on XZ points outward along -Y. The cutter must follow that normal,
    // rather than assuming every supported Hole drills downward in world Z.
    CadDocument side;auto sideProfile=SketchGeometry::addRectangle(
        SketchGeometry::newFeature(side.authored()["documentId"].toString(),"XZ"),
        {{"u1Mm",-10},{"v1Mm",-10},{"u2Mm",10},{"v2Mm",10}});
    commit(side,SketchGeometry::replaceFeature(side.authored(),sideProfile));
    commit(side,CadDocument::extrusionCandidate(side.authored(),{{"sketchId",sideProfile["id"]},
        {"profileId",side.sketches()[0].profiles[0].profileId},{"distanceMm",10}}));
    const auto sideBody=side.bodies()[0].bodyId;EvaluatedSupportFace sideCap;found=false;
    for(const auto& face:side.bodies()[0].supportFaces)if(face.role=="extrude.endCap"){sideCap=face;found=true;break;}
    check(found,"XZ end cap required");
    const QJsonObject sideSupport{{"producerFeatureId",sideCap.producerFeatureId},{"bodyId",sideBody},{"faceId",sideCap.faceId}};
    auto sidePoint=SketchGeometry::newSupportedFeature(side.authored()["documentId"].toString(),sideSupport,"Side point",
        GeometryKernel::supportEdgeDescriptors(sideCap));
    sidePoint=solved(SketchGeometry::addStandalonePoint(sidePoint,{{"uMm",0},{"vMm",0}},
        GeometryKernel::projectionContext(side.evaluation(),sidePoint)));
    const auto sideEntity=sidePoint["parameters"].toObject()["entities"].toArray()[0].toObject();
    commit(side,SketchGeometry::replaceFeature(side.authored(),sidePoint));
    commit(side,CadDocument::holeCandidate(side.authored(),{{"sourceSketchId",sidePoint["id"]},
        {"sourceEntityId",sideEntity["id"]},{"sourcePointId",sideEntity["pointId"]},{"targetBodyId",sideBody},
        {"boreDiameterMm",4},{"sinkDiameterMm",10},{"includedAngleDeg",90}}));
    near(side.bodies()[0].summary["entryMm"].toArray()[1].toDouble(),-10);
    near(side.bodies()[0].summary["volumeMm3"].toDouble(),4000-67*std::numbers::pi);
    std::cout<<"Through countersink geometry, edits, undo and reopen passed\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
