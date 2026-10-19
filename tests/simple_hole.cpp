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
        {"targetBodyId",target},{"holeType","simple"},{"extent","distance"},{"distanceMm",5},{"boreDiameterMm",4}};
    const auto stock=doc.authored();
    commit(doc,CadDocument::holeCandidate(doc.authored(),args));const auto created=doc.authored();
    const auto holeId=created["features"].toArray().last().toObject()["id"].toString();
    auto summary=doc.bodies()[0].summary;
    near(summary["removedVolumeMm3"].toDouble(),20*std::numbers::pi);
    check(summary["valid"].toBool() && summary["closed"].toBool(),"Simple blind hole valid/closed");
    check(created["features"].toArray().last().toObject()["parameters"].toObject().size()==9,"Only authored simple dimensions retained");
    doc.history().undo();check(doc.authored()==stock,"Exact Hole undo");doc.history().redo();check(doc.authored()==created,"Exact Hole redo");
    args["featureId"]=holeId;args["boreDiameterMm"]=6;args["distanceMm"]=7;
    commit(doc,CadDocument::holeCandidate(doc.authored(),args));near(doc.bodies()[0].summary["removedVolumeMm3"].toDouble(),63*std::numbers::pi);
    args["extent"]="all";commit(doc,CadDocument::holeCandidate(doc.authored(),args));
    near(doc.bodies()[0].summary["removedVolumeMm3"].toDouble(),90*std::numbers::pi);
    check(!doc.authored()["features"].toArray().last().toObject()["parameters"].toObject().contains("distanceMm"),"All has no stale depth");
    commit(doc,CadDocument::extrusionCandidate(doc.authored(),{{"featureId",extrusion["id"]},{"sketchId",base["id"]},{"profileId",profileId},{"distanceMm",12}}));
    near(doc.bodies()[0].summary["removedVolumeMm3"].toDouble(),108*std::numbers::pi);
    near(doc.bodies()[0].summary["entryMm"].toArray()[2].toDouble(),12);
    base=solved(SketchGeometry::updateDimension(base,{{"dimensionId",widthId},{"valueMm",85}}));
    commit(doc,SketchGeometry::replaceFeature(doc.authored(),base));
    near(doc.bodies()[0].summary["entryMm"].toArray()[0].toDouble(),-32.5);
    const auto stable=doc.authored();const auto revision=doc.revision();
    for(const auto& bad:QJsonArray{QJsonObject{{"distanceMm",0},{"extent","distance"}},QJsonObject{{"boreDiameterMm",-1}},QJsonObject{{"holeType","counterbore"}}}) {
        auto input=args;
        const auto updates=bad.toObject();for(auto i=updates.begin();i!=updates.end();++i)input[i.key()]=i.value();
        bool refused=false;try{CadDocument::holeCandidate(stable,input);}catch(const std::exception&){refused=true;}
        check(refused && doc.revision()==revision && doc.authored()==stable,"Invalid dimensions atomically refused");
    }
    auto missing=args;missing["sourcePointId"]=CadDocument::newId();
    check(!doc.commit(CadDocument::holeCandidate(stable,missing),"Missing point").isEmpty() && doc.authored()==stable,"Lost point atomically refused");
    QTemporaryDir temp;const auto path=temp.filePath("simple-hole.alloy");check(doc.save(path).isEmpty(),"Save Hole");
    CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==stable,"Exact Hole reopen");
    near(reopened.bodies()[0].summary["removedVolumeMm3"].toDouble(),108*std::numbers::pi);
    std::cout<<"Simple blind/All geometry, diameter/depth/source edits, refusal, undo and reopen passed\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
