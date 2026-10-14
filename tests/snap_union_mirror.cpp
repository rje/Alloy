// SPDX-License-Identifier: MIT
// Technical replay gate for a connected selected-profile Join source.
#include "CadDocument.h"
#include <QCoreApplication>
#include <QJsonDocument>
#include <iostream>
#include <stdexcept>
static void check(bool yes,const char* why){if(!yes)throw std::runtime_error(why);}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    check(argc==2 || argc==3,"Pass actual five-feature source and optional Mirror output");
    CadDocument doc;check(doc.open(QString::fromLocal8Bit(argv[1])).isEmpty(),"Open source failed");
    const auto features=doc.authored()["features"].toArray();check(features.size()>=5,"Source feature count");
    const auto join=features[features.size()-5].toObject();
    check(join["type"]=="extrude" && join["parameters"].toObject()["profileIds"].toArray().size()==3,
        "Expected retained three-region Join");
    const auto joinId=join["id"].toString();
    check(doc.evaluation().joinFeatureTools.value(joinId).size()==1 &&
        doc.evaluation().joinFeatureCaps.value(joinId).size()==1,"Connected source union is unavailable");
    QJsonArray ids;for(int i=features.size()-5;i<features.size();++i)ids.append(features[i].toObject()["id"]);
    const auto target=join["parameters"].toObject()["targetBodyIds"].toArray().first();
    const auto candidate=CadDocument::featureMirrorCandidate(doc.authored(),{{"ownerId",join["ownerId"]},
        {"targetBodyId",target},{"sourceFeatureIds",ids},
        {"mirrorPlane",QJsonObject{{"kind","origin"},{"plane","YZ"}}}});
    const auto preview=doc.prepareCandidate(candidate);
    std::cout<<"MIRROR_PREVIEW "<<preview.ok<<' '<<preview.error.toStdString()<<' '
        <<QString::fromUtf8(QJsonDocument(preview.diagnostics).toJson(QJsonDocument::Compact)).toStdString()<<'\n';
    if(!preview.ok)return 2;
    check(doc.commit(candidate,"Union snap Mirror").isEmpty(),"Mirror commit failed");
    const auto accepted=doc.authored();
    const auto p=join["parameters"].toObject();
    const auto edited=CadDocument::extrusionCandidate(doc.authored(),{{"featureId",joinId},
        {"sketchId",p["sketchId"]},{"profileIds",p["profileIds"]},{"distanceMm",5.25}});
    const auto editPreview=doc.prepareCandidate(edited);
    std::cout<<"JOIN_EDIT_PREVIEW "<<editPreview.ok<<' '<<editPreview.error.toStdString()<<' '
        <<QString::fromUtf8(QJsonDocument(editPreview.diagnostics).toJson(QJsonDocument::Compact)).toStdString()<<'\n';
    check(editPreview.ok,"Connected Join edit broke retained Mirror");
    check(doc.commit(edited,"Edit selected Join").isEmpty(),"Join edit commit failed");
    const auto final=doc.authored();
    doc.history().undo();check(doc.authored()==accepted,"Undo lost accepted Mirror");
    doc.history().redo();check(doc.authored()==final,"Redo lost edited Mirror");
    if(argc==3)check(doc.save(QString::fromLocal8Bit(argv[2])).isEmpty(),"Mirror save failed");
    if(argc==3){CadDocument reopened;check(reopened.open(QString::fromLocal8Bit(argv[2])).isEmpty()
        && reopened.authored()==final,"Mirror reopen changed retained intent");}
    std::cout<<"MIRROR_COMMITTED "<<doc.authored()["features"].toArray().size()<<'\n';
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
