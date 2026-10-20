// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include "SurfacePatch.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <gp_Pln.hxx>
#include <iostream>
#include <cmath>
namespace {
void check(bool ok,const QString& text){if(!ok)throw std::runtime_error(text.toStdString());}
void near(double a,double b,const QString& text){check(std::abs(a-b)<1e-5,text+QString(" %1 != %2").arg(a).arg(b));}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument doc;const auto owner=doc.authored().value("documentId").toString();
    auto sketch=SketchGeometry::addRectangle(SketchGeometry::newFeature(owner,"XY","Boundary"),{{"u1Mm",0},{"v1Mm",0},{"u2Mm",40},{"v2Mm",30}});
    const auto entities=sketch.value("parameters").toObject().value("entities").toArray();
    sketch=SketchGeometry::addDimension(sketch,{{"kind","horizontal"},{"entityId",entities[0].toObject().value("id")},{"valueMm",40}}).feature;
    const auto dimension=sketch.value("parameters").toObject().value("dimensions").toArray().last().toObject().value("id").toString();
    check(doc.commit(SketchGeometry::replaceFeature(doc.authored(),sketch),"Boundary").isEmpty(),"sketch");
    const auto profile=doc.evaluation().sketches.front().profiles.front().profileId;
    const auto candidate=CadDocument::surfacePatchCandidate(doc.authored(),{{"sketchId",sketch.value("id")},{"profileId",profile}});
    const auto preview=CadDocument::evaluateCandidate(candidate);check(preview.ok,preview.error);check(doc.bodies().empty(),"preview isolation");
    check(doc.commit(candidate,"Patch").isEmpty(),"patch commit");const auto first=doc.bodies().front();
    auto verify=[&](double area,double perimeter){const auto& b=doc.bodies().front();near(b.summary.value("areaMm2").toDouble(),area,"area");near(b.summary.value("perimeterMm").toDouble(),perimeter,"perimeter");const auto t=b.summary.value("topology").toObject();check(t.value("faces")==1 && t.value("solids")==0 && t.value("edges")==4 && b.summary.value("type")=="surface" && b.summary.value("volumeMm3").isNull(),"one face zero solid");check(b.bodyId==first.bodyId && b.supportFaces.front().faceId==first.supportFaces.front().faceId,"retained face and body");for(int i=0;i<4;++i)check(b.solidEdges[i].edgeId==first.solidEdges[i].edgeId && b.solidEdges[i].provenance.value("sourceEntityId")==first.solidEdges[i].provenance.value("sourceEntityId"),"source-based boundary identity");};verify(1200,140);
    const auto original=doc.authored();sketch=SketchGeometry::updateDimension(sketch,{{"dimensionId",dimension},{"valueMm",50}}).feature;
    check(doc.commit(SketchGeometry::replaceFeature(original,sketch),"Source edit").isEmpty(),"source width edit");verify(1500,160);doc.history().undo();verify(1200,140);check(doc.authored()==original,"undo authoring");doc.history().redo();verify(1500,160);
    check(doc.commit(CadDocument::surfacePatchCandidate(doc.authored(),{{"featureId",first.featureId},{"name","Edited Patch"}}),"Patch edit").isEmpty(),"feature edit");verify(1500,160);
    const auto accepted=doc.authored();const auto revision=doc.revision();
    check(!doc.commit(CadDocument::surfacePatchCandidate(accepted,{{"featureId",first.featureId},{"profileId",CadDocument::newId()}}),"Missing profile").isEmpty() && doc.authored()==accepted && doc.revision()==revision,"atomic invalid profile");
    bool refused=false;try{CadDocument::surfacePatchCandidate(accepted,{{"featureId",first.featureId},{"profileIds",QJsonArray{profile,profile}}});}catch(const std::invalid_argument&){refused=true;}check(refused,"plural refusal");
    for(int fixture:{0,1,2,3}){
        CadDocument invalidDoc;
        auto invalid=SketchGeometry::newFeature(invalidDoc.authored().value("documentId").toString(),"XY","Invalid");
        if(fixture==0)invalid=SketchGeometry::addLine(invalid,{{"u1Mm",0},{"v1Mm",0},{"u2Mm",10},{"v2Mm",0}});
        if(fixture==1){invalid=SketchGeometry::addRectangle(invalid,{{"u1Mm",0},{"v1Mm",0},{"u2Mm",10},{"v2Mm",10}});invalid=SketchGeometry::addRectangle(invalid,{{"u1Mm",20},{"v1Mm",0},{"u2Mm",30},{"v2Mm",10}});}
        if(fixture==2)for(const auto& a:std::vector<QJsonObject>{{{"u1Mm",0},{"v1Mm",0},{"u2Mm",10},{"v2Mm",10}},{{"u1Mm",10},{"v1Mm",10},{"u2Mm",0},{"v2Mm",10}},{{"u1Mm",0},{"v1Mm",10},{"u2Mm",10},{"v2Mm",0}},{{"u1Mm",10},{"v1Mm",0},{"u2Mm",0},{"v2Mm",0}}})invalid=SketchGeometry::addLine(invalid,a);
        if(fixture==3)invalid=SketchGeometry::addCircle(invalid,{{"centerUMm",0},{"centerVMm",0},{"diameterMm",20}});
        auto source=SketchGeometry::replaceFeature(invalidDoc.authored(),invalid);const auto ev=CadDocument::evaluateCandidate(source);check(ev.ok,ev.error);const auto& profiles=ev.evaluation->sketches.front().profiles;const auto selection=profiles.empty()?CadDocument::newId():profiles.front().profileId;
        const auto failure=CadDocument::evaluateCandidate(CadDocument::surfacePatchCandidate(source,{{"sketchId",invalid.value("id")},{"profileId",selection}}));check(!failure.ok,"invalid boundary refusal");
    }
    // Direct kernel invariant: a corrupted evaluated nonplanar profile is not accepted.
    auto prior=doc.evaluation();auto& corrupt=prior.sketches.front().profiles.front();corrupt.face=BRepBuilderAPI_MakeFace(gp_Pln(gp_Pnt(0,0,1),gp_Dir(0,0,1)),0,50,0,30).Face();
    refused=false;try{SurfacePatch::evaluate(doc.authored().value("features").toArray().last().toObject(),prior);}catch(const GeometryKernel::EvaluationError&){refused=true;}check(refused,"noncoplanar boundary refusal");
    QTemporaryDir tmp;const auto file=tmp.filePath("patch.alloy");check(doc.save(file).isEmpty(),"save");CadDocument reopened;check(reopened.open(file).isEmpty() && reopened.authored()==accepted,"reopen authoring");near(reopened.bodies().front().summary.value("areaMm2").toDouble(),1500,"reopen area");
    std::cout<<"PASS planar Patch topology, area/perimeter, source lineage, edit, atomic refusal, undo/redo/reopen\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
