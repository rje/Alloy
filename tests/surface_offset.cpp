// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include "SurfaceOffset.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <BRepAdaptor_Surface.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <gp_Cylinder.hxx>
#include <iostream>
#include <cmath>
#include <limits>
namespace {void check(bool ok,const QString& s){if(!ok)throw std::runtime_error(s.toStdString());}void near(double a,double b){check(std::abs(a-b)<1e-6,QString("%1 != %2").arg(a).arg(b));}}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument doc;const auto owner=doc.authored().value("documentId").toString();
    auto sketch=SketchGeometry::addRectangle(SketchGeometry::newFeature(owner,"XY","Boundary"),{{"u1Mm",0},{"v1Mm",0},{"u2Mm",40},{"v2Mm",30}});
    const auto entities=sketch.value("parameters").toObject().value("entities").toArray();
    sketch=SketchGeometry::addDimension(sketch,{{"kind","horizontal"},{"entityId",entities[0].toObject().value("id")},{"valueMm",40}}).feature;
    const auto dimension=sketch.value("parameters").toObject().value("dimensions").toArray().last().toObject().value("id").toString();
    check(doc.commit(SketchGeometry::replaceFeature(doc.authored(),sketch),"Boundary").isEmpty(),"sketch");
    const auto profile=doc.evaluation().sketches.front().profiles.front().profileId;
    check(doc.commit(CadDocument::surfacePatchCandidate(doc.authored(),{{"sketchId",sketch.value("id")},{"profileId",profile}}),"Patch").isEmpty(),"patch");
    const auto source=doc.bodies().front();const auto face=source.supportFaces.front();
    const QJsonObject ref{{"bodyId",source.bodyId},{"producerFeatureId",face.producerFeatureId},{"faceId",face.faceId}};
    const auto baseline=doc.authored();
    auto candidate=CadDocument::surfaceOffsetCandidate(baseline,{{"sourceFace",ref},{"distanceMm",5.}});
    const auto preview=CadDocument::evaluateCandidate(candidate);check(preview.ok,preview.error);check(doc.authored()==baseline && doc.bodies().size()==1,"preview isolation");
    check(doc.commit(candidate,"Offset").isEmpty(),"offset");const auto result=doc.bodies().back();
    auto verify=[&](double z,double area){const auto& b=doc.bodies().back();const auto t=b.summary.value("topology").toObject();check(t.value("faces")==1 && t.value("solids")==0 && b.summary.value("volumeMm3").isNull(),"open sheet");near(b.summary.value("areaMm2").toDouble(),area);near(BRepAdaptor_Surface(b.supportFaces.front().face).Plane().Location().Z(),z);check(b.featureId==result.featureId && b.bodyId==result.bodyId && b.supportFaces.front().faceId==result.supportFaces.front().faceId,"stable output identity");check(doc.bodies().front().bodyId==source.bodyId,"preserved source");};verify(5,1200);
    for(double d:{-5.,2.5,-2.5,0.}){check(doc.commit(CadDocument::surfaceOffsetCandidate(doc.authored(),{{"featureId",result.featureId},{"distanceMm",d}}),"Edit").isEmpty(),"distance edit");verify(d,1200);}
    sketch=SketchGeometry::updateDimension(sketch,{{"dimensionId",dimension},{"valueMm",42}}).feature;check(doc.commit(SketchGeometry::replaceFeature(doc.authored(),sketch),"Width").isEmpty(),"source edit");verify(0,1260);doc.history().undo();verify(0,1200);doc.history().redo();verify(0,1260);
    const auto accepted=doc.authored();const auto rev=doc.revision();auto stale=ref;stale["faceId"]=CadDocument::newId();check(!doc.commit(CadDocument::surfaceOffsetCandidate(accepted,{{"featureId",result.featureId},{"sourceFace",stale}}),"Stale").isEmpty() && doc.authored()==accepted && doc.revision()==rev,"atomic stale refusal");
    auto corrupt=doc.evaluation();corrupt.bodies.front().supportFaces.front().face=BRepBuilderAPI_MakeFace(gp_Cylinder(gp_Ax3(),10),0,1,0,30).Face();bool refused=false;try{SurfaceOffset::evaluate(accepted.value("features").toArray().last().toObject(),corrupt);}catch(const GeometryKernel::EvaluationError&){refused=true;}check(refused,"curved unsupported refusal");
    for(const auto value:{QJsonValue(100001.),QJsonValue(std::numeric_limits<double>::infinity()),QJsonValue("5")}){bool rejected=false;try{CadDocument::surfaceOffsetCandidate(accepted,{{"featureId",result.featureId},{"distanceMm",value}});}catch(const std::invalid_argument&){rejected=true;}check(rejected && doc.authored()==accepted && doc.revision()==rev,"invalid numeric atomic refusal");}
    auto reversed=doc.evaluation();reversed.bodies.front().supportFaces.front().face.Reverse();auto reverseFeature=accepted.value("features").toArray().last().toObject();auto reverseParameters=reverseFeature.value("parameters").toObject();reverseParameters["distanceMm"]=5.;reverseFeature["parameters"]=reverseParameters;
    const auto reversedResult=SurfaceOffset::evaluate(reverseFeature,reversed);near(BRepAdaptor_Surface(reversedResult.supportFaces.front().face).Plane().Location().Z(),-5.);
    QTemporaryDir tmp;const auto file=tmp.filePath("offset.alloy");check(doc.save(file).isEmpty(),"save");CadDocument reopened;check(reopened.open(file).isEmpty() && reopened.authored()==accepted,"reopen");near(reopened.bodies().back().summary.value("areaMm2").toDouble(),1260);
    std::cout<<"PASS planar signed Surface Offset exact sheet, stable lineage, edit, source recompute, atomic refusal, undo/reopen\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
