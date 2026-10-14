// SPDX-License-Identifier: MIT
// Headless source-shaped post-Split Join and edge-selection proof for SOL-051.
#include "CadDocument.h"
#include "SketchGeometry.h"
#include "SolidFillet.h"
#include <QCoreApplication>
#include <QJsonDocument>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
static void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
static void commit(CadDocument& doc,const QJsonObject& next){const auto error=doc.commit(next,"Snap proof");
    if(!error.isEmpty())throw std::runtime_error((error+" "+QString::fromUtf8(QJsonDocument(doc.lastEvaluationError()).toJson())).toStdString());}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    check(argc==2 || argc==3,"Pass accepted rounded-shell Split fixture path and optional output path");CadDocument doc;
    check(doc.open(QString::fromLocal8Bit(argv[1])).isEmpty(),"Cannot open Split fixture");
    check(doc.bodies().size()==2,"Fixture must have two actual split solids");
    const auto split=doc.authored()["features"].toArray().last().toObject();
    const auto topId=split["bodyId"].toString();
    const auto target=std::find_if(doc.bodies().begin(),doc.bodies().end(),[&](const auto& b){return b.bodyId==topId;});
    check(target!=doc.bodies().end(),"Top half unavailable");
    const EvaluatedSupportFace* side=nullptr;
    for(const auto& face:target->supportFaces){const auto normal=face.frame["normal"].toArray();
        if(normal.size()==3 && normal[1].toDouble()>.99){side=&face;break;}}
    check(side,"Top side planar face unavailable");
    std::cout<<"SIDE "<<QJsonDocument(side->frame).toJson(QJsonDocument::Compact).constData()<<'\n';
    const auto before=target->summary["volumeMm3"].toDouble();
    QJsonObject ref{{"producerFeatureId",side->producerFeatureId},{"bodyId",side->bodyId},{"faceId",side->faceId}};
    auto sketch=SketchGeometry::newSupportedFeature(doc.authored()["documentId"].toString(),ref,"Stepped snap proof");
    // Public step lengths 8,2,10,6,2; the sketch-frame placement is a fixture adaptation.
    const double p[][2]={{46.5,17},{46.5,25},{48.5,25},{48.5,15},{42.5,15},{42.5,17},{46.5,17}};
    for(int i=0;i<6;++i)sketch=SketchGeometry::addLine(sketch,{{"u1Mm",p[i][0]},{"v1Mm",p[i][1]},
        {"u2Mm",p[i+1][0]},{"v2Mm",p[i+1][1]}});
    commit(doc,SketchGeometry::replaceFeature(doc.authored(),sketch));
    check(doc.sketches().back().profiles.size()==1,"Stepped outline did not form one profile");
    commit(doc,CadDocument::extrusionCandidate(doc.authored(),{{"sketchId",sketch["id"]},
        {"profileId",doc.sketches().back().profiles.front().profileId},{"distanceMm",5},{"operation","join"},
        {"targetBodyIds",QJsonArray{topId}}}));
    const auto joinId=doc.authored()["features"].toArray().last().toObject()["id"].toString();
    const auto body=std::find_if(doc.bodies().begin(),doc.bodies().end(),[&](const auto& b){return b.bodyId==topId;});
    check(body!=doc.bodies().end(),"Joined top body unavailable");
    int generated=0;
    for(const auto& descriptor:SolidFillet::descriptors(body->solidEdges)){
        const auto edge=descriptor.toObject();
        if(edge["provenance"].toObject()["generatorFeatureId"]!=doc.authored()["features"].toArray().last().toObject()["id"] || !edge["available"].toBool())continue;
        ++generated;}
    std::cout<<"RESULT prior="<<before<<" after="<<body->summary["volumeMm3"].toDouble()<<" availableJoinEdges="<<generated<<'\n';
    check(generated>0,"Join published no current snap edges");
    auto same=[](const QJsonArray& value,const std::array<double,3>& point){
        return value.size()==3 && std::abs(value[0].toDouble()-point[0])<1e-6
            && std::abs(value[1].toDouble()-point[1])<1e-6 && std::abs(value[2].toDouble()-point[2])<1e-6;};
    auto select=[&](const std::array<double,3>& a,const std::array<double,3>& b){
        QString found;int count=0;for(const auto& value:SolidFillet::descriptors(std::find_if(doc.bodies().begin(),doc.bodies().end(),
                [&](const auto& candidate){return candidate.bodyId==topId;})->solidEdges)){
            const auto edge=value.toObject();if(!edge["available"].toBool())continue;
            const auto start=edge["startMm"].toArray(),end=edge["endMm"].toArray();
            if((same(start,a)&&same(end,b))||(same(start,b)&&same(end,a))){found=edge["edgeId"].toString();++count;}}
        if(count!=1)throw std::runtime_error("Expected exactly one retained snap edge, got "+std::to_string(count));
        return found;};
    auto chamfer=[&](const std::array<double,3>& a,const std::array<double,3>& b,double distance){
        const auto edgeId=select(a,b);std::cout<<"CHAMFER edge="<<edgeId.toStdString()<<" distance="<<distance<<'\n';
        commit(doc,CadDocument::chamferCandidate(doc.authored(),{{"targetBodyId",topId},
            {"edgeIds",QJsonArray{edgeId}},{"distanceMm",distance}}));};
    const auto filletEdge=select({-2,37,15},{-2,37,25});
    chamfer({-2,37,15},{4,37,15},2);
    chamfer({-2,37,25},{0,37,25},1);
    chamfer({0,32,17},{0,32,25},2);
    bool filletDescendant=false;for(const auto& edge:std::find_if(doc.bodies().begin(),doc.bodies().end(),
            [&](const auto& candidate){return candidate.bodyId==topId;})->solidEdges)
        if(edge.edgeId==filletEdge)filletDescendant=!edge.edge.IsNull();
    check(filletDescendant,"Original inner snap edge lost its unique Chamfer descendant");
    std::cout<<"FILLET edge="<<filletEdge.toStdString()<<" radius=0.5\n";
    commit(doc,CadDocument::filletCandidate(doc.authored(),{{"targetBodyId",topId},
        {"edgeIds",QJsonArray{filletEdge}},{"radiusMm",0.5},{"tangentChain",true}}));
    check(doc.bodies().size()==2,"Finishes changed split body count");
    const auto finished=std::find_if(doc.bodies().begin(),doc.bodies().end(),
        [&](const auto& candidate){return candidate.bodyId==topId;});
    check(finished!=doc.bodies().end() && finished->summary["valid"].toBool()
        && finished->summary["closed"].toBool(),"Finished snap must be a valid closed top half");
    const auto finishedVolume=finished->summary["volumeMm3"].toDouble();
    const auto accepted=doc.authored();
    QJsonArray five;for(const auto& feature:accepted["features"].toArray()){
        const auto f=feature.toObject();if(f["id"]==joinId || f["type"]=="chamfer" ||
            (f["type"]=="fillet" && f["parameters"].toObject()["radiusMm"]==0.5))five.append(f["id"]);}
    check(five.size()==5,"Expected source Join, three Chamfers and first Fillet IDs");
    commit(doc,CadDocument::extrusionCandidate(doc.authored(),{{"featureId",joinId},
        {"sketchId",sketch["id"]},{"profileId",doc.sketches().back().profiles.front().profileId},
        {"distanceMm",5.5}}));
    const auto edited=doc.authored();const auto grown=std::find_if(doc.bodies().begin(),doc.bodies().end(),
        [&](const auto& candidate){return candidate.bodyId==topId;});
    check(grown!=doc.bodies().end() && grown->summary["valid"].toBool() && grown->summary["closed"].toBool()
        && grown->summary["volumeMm3"].toDouble()>finishedVolume,"Join distance edit did not rebuild finished snap");
    const auto unchanged=doc.authored();const auto revision=doc.revision();
    const auto firstChamfer=unchanged["features"].toArray()[unchanged["features"].toArray().size()-4].toObject();
    check(firstChamfer["type"]=="chamfer","First source finish unavailable");
    const auto impossible=CadDocument::chamferCandidate(unchanged,{{"featureId",firstChamfer["id"]},
        {"targetBodyId",topId},{"edgeIds",QJsonArray{CadDocument::newId()}},
        {"distanceMm",2}});
    check(!doc.commit(impossible,"Missing snap edge").isEmpty(),"Missing source edge accepted");
    check(doc.authored()==unchanged && doc.revision()==revision,"Failed finish changed authored snap");
    doc.history().undo();check(doc.authored()==accepted,"Undo lost five-feature snap");
    doc.history().redo();check(doc.authored()==edited,"Redo lost edited snap");
    if(argc==3){const auto path=QString::fromLocal8Bit(argv[2]);check(doc.save(path).isEmpty(),"Save failed");
        CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==edited,
            "Reopen changed snap feature intent");}
    std::cout<<"PASS stepped post-Split Join5, Chamfer2/1/2, Fillet0.5, Join edit/history/reopen\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
