// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include <QCoreApplication>
#include <QJsonDocument>
#include <cmath>
#include <iostream>
#include <stdexcept>
static void check(bool valid,const char* why){if(!valid)throw std::runtime_error(why);}
static const EvaluatedBody& body(const std::vector<EvaluatedBody>& bodies,const QString& id){
    for(const auto& value:bodies)if(value.bodyId==id)return value;
    throw std::runtime_error("Target body unavailable");
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    check(argc==2 || argc==3,"Pass completed post-Split snap fixture and optional output path");
    CadDocument doc;check(doc.open(QString::fromLocal8Bit(argv[1])).isEmpty(),"Snap fixture open failed");
    const auto features=doc.authored()["features"].toArray();check(features.size()>=5,"Five source features absent");
    const auto join=features[features.size()-5].toObject();check(join["type"]=="extrude"
        && join["parameters"].toObject()["operation"]=="join","Join source absent");
    const auto target=join["parameters"].toObject()["targetBodyIds"].toArray().first().toString();
    QJsonArray ids;for(int index=features.size()-5;index<features.size();++index)ids.append(features[index].toObject()["id"]);
    const auto sourceVolume=body(doc.bodies(),target).summary["volumeMm3"].toDouble();
    const auto preJoin=doc.evaluationBefore(join["id"].toString());
    const auto stockVolume=body(preJoin.bodies,target).summary["volumeMm3"].toDouble();
    const auto sourceDelta=sourceVolume-stockVolume;check(sourceDelta>0,"Finished snap adds no material");
    const auto candidate=CadDocument::featureMirrorCandidate(doc.authored(),{{"targetBodyId",target},
        {"sourceFeatureIds",ids},{"mirrorPlane",QJsonObject{{"kind","origin"},{"plane","XZ"}}}});
    const auto preview=doc.prepareCandidate(candidate);
    if(!preview.ok)throw std::runtime_error((preview.error+" "+QString::fromUtf8(QJsonDocument(preview.diagnostics).toJson())).toStdString());
    const auto mirrored=body(preview.evaluation->bodies,target);
    check(mirrored.summary["valid"].toBool() && mirrored.summary["closed"].toBool(),"Mirrored snap invalid");
    std::cout<<"RESULT stock="<<stockVolume<<" source="<<sourceVolume<<" mirrored="
        <<mirrored.summary["volumeMm3"].toDouble()<<" sourceDelta="<<sourceDelta<<'\n';
    check(std::abs(mirrored.summary["volumeMm3"].toDouble()-(sourceVolume+sourceDelta))<1e-4,
        "Mirrored snap is not a symmetric second local result");
    int mirroredEdges=0,mirroredCaps=0;
    const auto mirrorId=candidate["features"].toArray().last().toObject()["id"];
    for(const auto& edge:mirrored.solidEdges)if(!edge.edge.IsNull() && edge.provenance["mirrorFeatureId"]==mirrorId)++mirroredEdges;
    for(const auto& face:mirrored.supportFaces)if(!face.face.IsNull() && face.producerFeatureId==mirrorId)++mirroredCaps;
    std::cout<<"RESULT mirroredEdges="<<mirroredEdges<<" mirroredCaps="<<mirroredCaps<<'\n';
    check(mirroredEdges>0 && mirroredCaps==1,"Mirrored snap edge/cap references unavailable");
    const auto atJoin=doc.evaluationBefore(features[features.size()-4].toObject()["id"].toString());
    QJsonObject sourceFrame,mirroredFrame;
    for(const auto& face:body(atJoin.bodies,target).supportFaces)
        if(face.producerFeatureId==join["id"].toString() && !face.face.IsNull())sourceFrame=face.frame;
    for(const auto& face:mirrored.supportFaces)
        if(face.producerFeatureId==mirrorId.toString() && !face.face.IsNull())mirroredFrame=face.frame;
    check(!sourceFrame.isEmpty() && !mirroredFrame.isEmpty(),"Source/reflected cap frame unavailable");
    const auto sourceOrigin=sourceFrame["origin"].toArray(),mirroredOrigin=mirroredFrame["origin"].toArray();
    check(sourceOrigin.size()==3 && mirroredOrigin.size()==3
        && std::abs(sourceOrigin[0].toDouble()-mirroredOrigin[0].toDouble())<1e-6
        && std::abs(sourceOrigin[1].toDouble()+mirroredOrigin[1].toDouble())<1e-6
        && std::abs(sourceOrigin[2].toDouble()-mirroredOrigin[2].toDouble())<1e-6,
        "Retained reflected cap frame is not on the opposite side of XZ");
    check(doc.commit(candidate,"Mirror snap group").isEmpty(),"Mirror commit failed");
    const auto accepted=doc.authored();
    const auto p=join["parameters"].toObject();
    check(doc.commit(CadDocument::extrusionCandidate(doc.authored(),{{"featureId",join["id"]},
        {"sketchId",p["sketchId"]},{"profileId",p["profileId"]},{"distanceMm",6}}),
        "Edit source Join").isEmpty(),"Upstream Join edit broke reflected finishes");
    const auto edited=doc.authored();const auto editedBody=body(doc.bodies(),target);
    check(editedBody.summary["valid"].toBool() && editedBody.summary["closed"].toBool()
        && editedBody.summary["volumeMm3"].toDouble()>mirrored.summary["volumeMm3"].toDouble(),
        "Upstream Join edit did not rebuild both snaps");
    doc.history().undo();check(doc.authored()==accepted,"Undo changed accepted Mirror intent");
    doc.history().redo();check(doc.authored()==edited,"Redo changed edited Mirror intent");
    const auto unchanged=doc.authored();const auto revision=doc.revision();
    auto invalid=unchanged;auto invalidFeatures=invalid["features"].toArray();
    const auto extra=QJsonObject{{"id",CadDocument::newId()},{"ownerId",join["ownerId"]},
        {"type","chamfer"},{"name","Unselected Top finish"},{"visible",true},
        {"parameters",QJsonObject{{"targetBodyId",target},{"edgeIds",QJsonArray{CadDocument::newId()}},{"distanceMm",1}}}};
    invalidFeatures.insert(invalidFeatures.size()-1,extra);invalid["features"]=invalidFeatures;
    const auto refused=CadDocument::evaluateCandidate(invalid);
    check(!refused.ok && refused.diagnostics["code"]=="mirror_source_gap",
        "Unselected same-target modifier was not refused before geometry evaluation");
    check(doc.authored()==unchanged && doc.revision()==revision,"Failed group edit changed document");
    if(argc==3){const auto path=QString::fromLocal8Bit(argv[2]);check(doc.save(path).isEmpty(),"Mirror save failed");
        CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==doc.authored(),"Mirror reopen failed");}
    CadDocument withBottom;check(withBottom.open(QString::fromLocal8Bit(argv[1])).isEmpty(),"Interlude fixture open failed");
    QString bottomId;for(const auto& candidateBody:withBottom.bodies())if(candidateBody.bodyId!=target)bottomId=candidateBody.bodyId;
    check(!bottomId.isEmpty(),"Separate Bottom body unavailable");
    bool bottomFinish=false;
    for(const auto& edge:body(withBottom.bodies(),bottomId).solidEdges){
        if(edge.edge.IsNull())continue;
        const auto trial=CadDocument::filletCandidate(withBottom.authored(),{{"targetBodyId",bottomId},
            {"edgeIds",QJsonArray{edge.edgeId}},{"radiusMm",0.2}});
        if(!withBottom.prepareCandidate(trial).ok)continue;
        check(withBottom.commit(trial,"Unrelated Bottom finish").isEmpty(),"Bottom interlude commit failed");
        bottomFinish=true;break;
    }
    check(bottomFinish,"No valid unrelated Bottom finish for interlude proof");
    const auto withInterlude=CadDocument::featureMirrorCandidate(withBottom.authored(),{{"targetBodyId",target},
        {"sourceFeatureIds",ids},{"mirrorPlane",QJsonObject{{"kind","origin"},{"plane","XZ"}}}});
    check(withBottom.prepareCandidate(withInterlude).ok,"Unrelated Bottom finish blocked snap Mirror");
    std::cout<<"PASS five ordered snap feature Mirror\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
