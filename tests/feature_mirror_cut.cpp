#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void commit(CadDocument& document,const QJsonObject& candidate){
    const auto error=document.commit(candidate,"Feature Mirror check");
    if(!error.isEmpty())throw std::runtime_error(error.toStdString());
}
double volume(const CadDocument& document){return document.bodies().front().summary["volumeMm3"].toDouble();}
void near(double actual,double expected){check(std::abs(actual-expected)<1e-5,"Unexpected mirror volume");}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument document;
    const auto root=document.authored()["documentId"].toString();
    auto box=QJsonObject{{"id",CadDocument::newId()},{"bodyId",CadDocument::newId()},{"ownerId",root},
        {"type","box"},{"name","Symmetric case"},{"visible",true},
        {"parameters",QJsonObject{{"widthMm",50},{"depthMm",80},{"heightMm",20},
            {"xMm",-25},{"yMm",-40},{"zMm",0}}}};
    auto next=document.authored();next["features"]=QJsonArray{box};commit(document,next);
    auto sketch=SketchGeometry::newFeature(root,"XY");
    sketch=SketchGeometry::addRectangle(sketch,{{"u1Mm",10},{"v1Mm",20},{"u2Mm",16},{"v2Mm",26}});
    commit(document,SketchGeometry::replaceFeature(document.authored(),sketch));
    const auto profile=document.sketches().back().profiles.front().profileId;
    commit(document,CadDocument::extrusionCandidate(document.authored(),{{"sketchId",sketch["id"]},
        {"profileId",profile},{"distanceMm",5},{"operation","cut"},
        {"targetBodyIds",QJsonArray{box["bodyId"]}}}));
    const auto cut=document.authored()["features"].toArray().last().toObject();
    near(volume(document),80000-180);
    const QJsonObject mirrorArgs{{"targetBodyId",box["bodyId"]},
        {"sourceFeatureIds",QJsonArray{cut["id"]}},
        {"mirrorPlane",QJsonObject{{"kind","origin"},{"plane","XZ"}}}};
    auto yzArgs=mirrorArgs;yzArgs["mirrorPlane"]=QJsonObject{{"kind","origin"},{"plane","YZ"}};
    const auto yzPreview=document.prepareCandidate(CadDocument::featureMirrorCandidate(document.authored(),yzArgs));
    check(yzPreview.ok,"Origin YZ Cut Mirror preview failed");
    near(yzPreview.evaluation->bodies.front().summary["volumeMm3"].toDouble(),80000-360);
    commit(document,CadDocument::featureMirrorCandidate(document.authored(),mirrorArgs));
    const auto mirror=document.authored()["features"].toArray().last().toObject();
    near(volume(document),80000-360);
    check(document.bodies().size()==1,"Feature Mirror copied the complete body");
    check(document.bodies().front().bodyId==box["bodyId"],"Target body identity changed");
    int mirroredEdges=0;
    for(const auto& edge:document.bodies().front().solidEdges)
        if(!edge.edge.IsNull() && edge.provenance["generatorFeatureId"]==mirror["id"])++mirroredEdges;
    check(mirroredEdges>0,"Mirrored cut boundaries unavailable to downstream edge selection");
    const auto accepted=document.authored();const auto revision=document.revision();
    auto invalid=mirrorArgs;invalid["sourceFeatureIds"]=QJsonArray{CadDocument::newId()};
    invalid["featureId"]=mirror["id"];
    const auto bad=CadDocument::featureMirrorCandidate(document.authored(),invalid);
    check(!document.commit(bad,"Missing mirror seed").isEmpty(),"Missing source unexpectedly accepted");
    check(document.authored()==accepted && document.revision()==revision,"Failed mirror edit changed document");
    next=document.authored();auto features=next["features"].toArray();
    auto edited=features[0].toObject();auto dimensions=edited["parameters"].toObject();
    dimensions["depthMm"]=100;dimensions["yMm"]=-50;edited["parameters"]=dimensions;features[0]=edited;
    next["features"]=features;commit(document,next);
    near(volume(document),100000-360);
    const auto resized=document.authored();
    document.history().undo();check(document.authored()==accepted,"Undo did not restore mirror source");
    document.history().redo();check(document.authored()==resized,"Redo did not restore resized source");
    QTemporaryDir directory;check(document.save(directory.path()+"/mirror.alloy").isEmpty(),"Save failed");
    CadDocument reopened;check(reopened.open(directory.path()+"/mirror.alloy").isEmpty(),"Reopen failed");
    check(reopened.authored()==resized,"Reopen changed authored mirror intent");near(volume(reopened),100000-360);
    std::cout<<"PASS retained Cut feature Mirror, source resize, atomic edit and reopen\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
