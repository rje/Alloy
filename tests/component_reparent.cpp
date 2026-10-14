// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include "MeshExport.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool good,const QString& message){if(!good)throw std::runtime_error(message.toStdString());}
void commit(CadDocument& d,const QJsonObject& candidate,const QString& label){
    const auto error=d.commit(candidate,label);check(error.isEmpty(),error);
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument document;const auto root=document.activeOwnerId();
    const auto sketch=SketchGeometry::addCenterRectangle(SketchGeometry::newFeature(root,"XY","Source sketch"),
        {{"centerUMm",0},{"centerVMm",0},{"widthMm",20},{"heightMm",10}});
    check(sketch.ok,sketch.error);auto candidate=SketchGeometry::replaceFeature(document.authored(),sketch.feature);
    commit(document,candidate,"Sketch");const auto sketchId=sketch.feature["id"].toString();
    const auto profile=document.sketches()[0].profiles[0].profileId;
    candidate=CadDocument::extrusionCandidate(document.authored(),{{"ownerId",root},{"sketchId",sketchId},
        {"profileId",profile},{"distanceMm",5}});commit(document,candidate,"Solid");
    const auto bodyId=document.bodies()[0].bodyId,bodyFeatureId=document.bodies()[0].featureId;
    const auto baseSummary=document.bodies()[0].summary,baseFrame=document.sketches()[0].resolvedFrame;
    candidate=CadDocument::componentCandidate(document.authored(),"Finished part");
    const auto owner=candidate["components"].toArray().last().toObject()["id"].toString();
    commit(document,candidate,"Component");
    const auto beforeMove=document.authored();const auto initialCount=document.history().count();
    candidate=CadDocument::reparentCandidate(document.authored(),document.evaluation(),"body",QJsonArray{bodyId},owner);
    commit(document,candidate,"Move body");
    check(document.bodies()[0].ownerId==owner && document.sketches()[0].ownerId==root,
        "Body move must leave source Sketches folder at root");
    check(document.bodies()[0].bodyId==bodyId && document.bodies()[0].featureId==bodyFeatureId
        && document.bodies()[0].summary["boundsMm"]==baseSummary["boundsMm"]
        && document.bodies()[0].summary["volumeMm3"]==baseSummary["volumeMm3"],
        "Body move must preserve its identity and world shape");
    const auto afterBody=document.authored();
    candidate=CadDocument::reparentCandidate(document.authored(),document.evaluation(),"sketches",QJsonArray{sketchId},owner);
    commit(document,candidate,"Move Sketches folder");
    check(document.sketches()[0].ownerId==owner && document.sketches()[0].resolvedFrame==baseFrame,
        "Separate Sketches move must preserve frame");
    check(document.authored()["features"]==beforeMove["features"],
        "Organization must preserve authored modeling features and references exactly");
    const auto prefix=document.evaluationBefore(bodyFeatureId);
    check(prefix.bodies.empty() && prefix.sketches.size()==1 && prefix.sketches[0].featureId==sketchId,
        "Editing an earlier Extrude must resolve its original sketch before final organization");
    check(document.history().count()==initialCount+2,"Each observed move is one transaction");
    const auto afterBoth=document.authored();
    document.history().undo();check(document.authored()==afterBody && document.sketches()[0].ownerId==root,
        "Sketches move undo");
    document.history().undo();check(document.authored()==beforeMove && document.bodies()[0].ownerId==root,
        "Body move undo");
    document.history().redo();document.history().redo();
    check(document.authored()==afterBoth && document.bodies()[0].ownerId==owner && document.sketches()[0].ownerId==owner,
        "Two-move redo must restore exact authored owners");
    candidate=document.authored();auto features=candidate["features"].toArray();auto solid=features[1].toObject();
    auto parameters=solid["parameters"].toObject();parameters["distanceMm"]=8;solid["parameters"]=parameters;
    features[1]=solid;candidate["features"]=features;commit(document,candidate,"Edit source extrusion");
    check(document.bodies()[0].ownerId==owner && document.sketches()[0].ownerId==owner
        && document.bodies()[0].bodyId==bodyId && std::abs(document.bodies()[0].summary["volumeMm3"].toDouble()-1600)<1e-5,
        "Source edit recomputes same reparented body");
    const auto valid=document.authored();const auto revision=document.revision();
    candidate=valid;features=candidate["features"].toArray();features.removeAt(1);candidate["features"]=features;
    check(!document.commit(candidate,"Remove owned body").isEmpty()
        && document.lastEvaluationError()["code"]=="reparent_body_unavailable",
        "Dangling reparented body must refuse with structured cause");
    check(document.authored()==valid && document.revision()==revision,"Dangling body refusal is atomic");
    candidate=CadDocument::moveCandidate(document.authored(),{{"ownerId",root},{"targetBodyId",bodyId},{"xMm",1}});
    check(!document.commit(candidate,"Hidden root edit").isEmpty()
        && document.lastEvaluationError()["code"]=="reparent_cross_owner_new_feature",
        "New source-owner operation on moved body must refuse explicitly");
    check(document.authored()==valid && document.revision()==revision,"New cross-owner refusal is atomic");
    QTemporaryDir directory;check(directory.isValid(),"Temporary directory");
    const auto path=directory.filePath("organized.alloy");check(document.save(path).isEmpty(),"Save organized document");
    CadDocument reopened;check(reopened.open(path).isEmpty(),"Reopen organized document");
    check(reopened.authored()==valid && reopened.bodies()[0].ownerId==owner && reopened.sketches()[0].ownerId==owner,
        "Reopen preserves organization and stable identities");
    std::vector<EvaluatedBody> scope;for(const auto& body:reopened.bodies())if(body.ownerId==owner)scope.push_back(body);
    const auto stl=MeshExport::writeStl(scope,directory.filePath("finished.stl"),"mm",true,0.1,10);
    check(stl["bodyCount"]==1 && stl["bodyIds"].toArray()==QJsonArray{bodyId},"Scoped export follows moved body");
    std::cout<<"component body then sketches reparent/history/edit/reopen/export passed\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
