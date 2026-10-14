// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
void commit(CadDocument& document,const QJsonObject& candidate,const char* label){
    const auto error=document.commit(candidate,label);if(!error.isEmpty())throw std::runtime_error(error.toStdString());
}
QJsonObject replace(const QJsonObject& authored,const QJsonObject& feature){return SketchGeometry::replaceFeature(authored,feature);}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument document;const auto root=document.authored()["documentId"].toString();
    auto sketch=SketchGeometry::newFeature(root,"XZ","Open sheet source");
    sketch=SketchGeometry::addLine(sketch,{{"u1Mm",0},{"v1Mm",0},{"u2Mm",40},{"v2Mm",0}});
    auto candidate=document.authored();auto features=candidate["features"].toArray();features.append(sketch);candidate["features"]=features;
    commit(document,candidate,"Sketch");
    const auto entityId=sketch["parameters"].toObject()["entities"].toArray()[0].toObject()["id"].toString();
    candidate=CadDocument::surfaceExtrudeCandidate(document.authored(),{{"sketchId",sketch["id"]},
        {"entityIds",QJsonArray{entityId}},{"sideOneMm",20.0},{"sideTwoMm",25.0}});
    commit(document,candidate,"Surface Extrude");
    const auto body=document.bodies().back();const auto bodyId=body.bodyId;
    check(body.summary["type"]=="surface" && body.summary["topology"].toObject()["solids"]==0,
        "Current source is an open surface body");
    commit(document,CadDocument::componentCandidate(document.authored(),"Sheet holder"),"Component");
    const auto owner=document.authored()["components"].toArray().last().toObject()["id"].toString();
    commit(document,CadDocument::renameBodyCandidate(document.authored(),document.evaluation(),bodyId,"Trial tool sheet"),"Rename body");
    commit(document,CadDocument::reparentCandidate(document.authored(),document.evaluation(),"body",QJsonArray{bodyId},owner),"Move body");
    check(document.bodies().back().name=="Trial tool sheet" && document.bodies().back().ownerId==owner,
        "Current authored name and owner");
    const auto beforeRemove=document.authored();const auto sourceArea=document.bodies().back().summary["areaMm2"].toDouble();
    candidate=CadDocument::removeBodyCandidate(document.authored(),{{"bodyId",bodyId},{"ownerId",root}});
    commit(document,candidate,"Remove surface body");
    const auto removed=document.authored();const auto removeId=removed["features"].toArray().last().toObject()["id"].toString();
    check(document.bodies().empty() && removed.value("bodyNames").toObject().contains(bodyId)
        && removed.value("objectOwners").toObject()["bodies"].toObject().contains(bodyId),
        "Remove hides active body but preserves authored name and owner");
    document.history().undo();check(document.authored()==beforeRemove && document.bodies().size()==1
        && document.bodies().back().name=="Trial tool sheet" && document.bodies().back().ownerId==owner,
        "Undo restores body, name and owner");
    document.history().redo();check(document.authored()==removed && document.bodies().empty(),"Redo restores Remove");
    QTemporaryDir temp;const auto path=temp.filePath("removed-sheet.alloy");check(document.save(path).isEmpty(),"Save removed sheet");
    CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==removed && reopened.bodies().empty(),
        "Reopen retains Remove and authored mappings");
    auto edited=sketch;auto p=edited["parameters"].toObject();auto points=p["points"].toArray();
    for(int i=0;i<points.size();++i){auto point=points[i].toObject();if(std::abs(point["uMm"].toDouble()-40)<1e-6){point["uMm"]=45.0;points[i]=point;}}
    p["points"]=points;edited["parameters"]=p;
    commit(reopened,replace(reopened.authored(),edited),"Edit hidden source");
    check(reopened.bodies().empty(),"Remove stays active through upstream edit");
    auto restore=reopened.authored();features=restore["features"].toArray();
    for(int i=0;i<features.size();++i)if(features[i].toObject()["id"]==removeId){features.removeAt(i);break;}
    restore["features"]=features;commit(reopened,restore,"Delete Remove feature");
    check(reopened.bodies().size()==1 && reopened.bodies().back().name=="Trial tool sheet"
        && reopened.bodies().back().ownerId==owner
        && reopened.bodies().back().summary["areaMm2"].toDouble()>sourceArea+100,
        "Deleting Remove restores edited surface and metadata");
    auto invalid=removed;features=invalid["features"].toArray();
    const auto support=QJsonObject{{"producerFeatureId",body.featureId},{"bodyId",bodyId},
        {"faceId",body.supportFaces[0].faceId}};
    features.append(SketchGeometry::newSupportedFeature(root,support,"Invalid downstream support"));
    invalid["features"]=features;
    check(!document.commit(invalid,"Invalid downstream support").isEmpty() && document.authored()==removed,
        "Downstream reference to removed body refuses atomically");
    invalid=removed;features=invalid["features"].toArray();
    for(int i=0;i<features.size();++i)if(features[i].toObject()["id"]==body.featureId){features.removeAt(i);break;}
    invalid["features"]=features;
    check(!document.commit(invalid,"Missing removed source").isEmpty() && document.authored()==removed,
        "Missing upstream source does not use metadata exemption");
    std::cout<<"REMOVE_SURFACE_OK body="<<bodyId.toStdString()<<"\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
