// SPDX-License-Identifier: MIT
// Technical replay gate for a connected selected-profile Join source.
#include "CadDocument.h"
#include <QCoreApplication>
#include <QJsonDocument>
#include <BRepAdaptor_Curve.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <cmath>
#include <iostream>
#include <stdexcept>
static void check(bool yes,const char* why){if(!yes)throw std::runtime_error(why);}
static QString reflectedRoot(const CadDocument& doc,const QString& bodyId,const QString& mirrorId){
    const auto& bodies=doc.bodies();const auto body=std::find_if(bodies.begin(),bodies.end(),
        [&](const auto& value){return value.bodyId==bodyId;});
    check(body!=bodies.end(),"Mirrored snap body unavailable");
    TopTools_IndexedMapOfShape current;TopExp::MapShapes(body->shape,TopAbs_EDGE,current);
    QString found;int count=0;
    for(const auto& edge:body->solidEdges){
        if(edge.provenance["role"]!="join.root" || edge.provenance["mirrorFeatureId"]!=mirrorId)continue;
        check(current.Contains(edge.edge) && !edge.provenance["sourceToolEdgeId"].toString().isEmpty()
            && !edge.provenance["targetFaceId"].toString().isEmpty(),"Reflected root lacks exact current ancestry");
        BRepAdaptor_Curve line(edge.edge);const auto a=line.Value(line.FirstParameter()),b=line.Value(line.LastParameter());
        if(std::abs(a.X()-28.5)<1e-6 && std::abs(b.X()-28.5)<1e-6
            && std::abs(a.Y()-17.5)<1e-6 && std::abs(b.Y()-17.5)<1e-6
            && ((std::abs(a.Z()-15)<1e-6 && std::abs(b.Z()-20.5)<1e-6)
                || (std::abs(b.Z()-15)<1e-6 && std::abs(a.Z()-20.5)<1e-6))){found=edge.edgeId;++count;}
    }
    check(count==1,"Reflected wall/stem attachment root is not unique");return found;
}
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
    const auto mirrorId=accepted["features"].toArray().last().toObject()["id"].toString();
    const auto rootId=reflectedRoot(doc,target.toString(),mirrorId);
    const auto p=join["parameters"].toObject();
    const auto edited=CadDocument::extrusionCandidate(doc.authored(),{{"featureId",joinId},
        {"sketchId",p["sketchId"]},{"profileIds",p["profileIds"]},{"distanceMm",5.25}});
    const auto editPreview=doc.prepareCandidate(edited);
    std::cout<<"JOIN_EDIT_PREVIEW "<<editPreview.ok<<' '<<editPreview.error.toStdString()<<' '
        <<QString::fromUtf8(QJsonDocument(editPreview.diagnostics).toJson(QJsonDocument::Compact)).toStdString()<<'\n';
    check(editPreview.ok,"Connected Join edit broke retained Mirror");
    check(doc.commit(edited,"Edit selected Join").isEmpty(),"Join edit commit failed");
    check(reflectedRoot(doc,target.toString(),mirrorId)==rootId,"Join edit changed reflected root identity");
    const auto final=doc.authored();
    doc.history().undo();check(doc.authored()==accepted && reflectedRoot(doc,target.toString(),mirrorId)==rootId,"Undo lost accepted Mirror root");
    doc.history().redo();check(doc.authored()==final && reflectedRoot(doc,target.toString(),mirrorId)==rootId,"Redo lost edited Mirror root");
    if(argc==3)check(doc.save(QString::fromLocal8Bit(argv[2])).isEmpty(),"Mirror save failed");
    if(argc==3){CadDocument reopened;check(reopened.open(QString::fromLocal8Bit(argv[2])).isEmpty()
        && reopened.authored()==final && reflectedRoot(reopened,target.toString(),mirrorId)==rootId,
        "Mirror reopen changed retained root identity");}
    std::cout<<"MIRROR_COMMITTED "<<doc.authored()["features"].toArray().size()<<'\n';
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
