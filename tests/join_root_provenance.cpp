// SPDX-License-Identifier: MIT
// Exact Join-root ancestry on a source-shaped retained Case fixture.
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
static QJsonObject root(const CadDocument& doc,const QString& bodyId){
    const auto body=std::find_if(doc.bodies().begin(),doc.bodies().end(),
        [&](const auto& value){return value.bodyId==bodyId;});
    check(body!=doc.bodies().end(),"Retained target body absent");
    QJsonObject found;int count=0;
    TopTools_IndexedMapOfShape current;TopExp::MapShapes(body->shape,TopAbs_EDGE,current);
    for(const auto& edge:body->solidEdges)if(edge.provenance["role"]=="join.root"){
        check(!edge.edge.IsNull() && current.Contains(edge.edge),"Join root is not a current result edge");
        const auto faceId=edge.provenance["targetFaceId"].toString();
        check(!faceId.isEmpty() && !edge.provenance["sourceToolEdgeId"].toString().isEmpty(),
            "Join root lacks source and target identities");
        const auto face=std::find_if(body->supportFaces.begin(),body->supportFaces.end(),
            [&](const auto& value){return value.faceId==faceId;});
        check(face!=body->supportFaces.end(),"Join root target face no longer resolves");
        BRepAdaptor_Curve curve(edge.edge);const auto a=curve.Value(curve.FirstParameter()),b=curve.Value(curve.LastParameter());
        check(std::abs(a.X()+28.5)<1e-6 && std::abs(b.X()+28.5)<1e-6
            && std::abs(a.Y()-17.5)<1e-6 && std::abs(b.Y()-17.5)<1e-6
            && ((std::abs(a.Z()-15)<1e-6 && std::abs(b.Z()-20.5)<1e-6)
                || (std::abs(b.Z()-15)<1e-6 && std::abs(a.Z()-20.5)<1e-6)),
            "Published Join root is not the actual Top-wall attachment segment");
        found={{"edgeId",edge.edgeId},{"targetFaceId",faceId},
            {"sourceToolEdgeId",edge.provenance["sourceToolEdgeId"]}};++count;
    }
    check(count==1,"The actual attachment root must have one stable identity");
    return found;
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    check(argc==2 || argc==3,"Pass source-shaped Join fixture and optional save path");
    CadDocument doc;check(doc.open(QString::fromLocal8Bit(argv[1])).isEmpty(),"Join fixture open failed");
    const auto join=doc.authored()["features"].toArray().last().toObject();
    check(join["type"]=="extrude" && join["parameters"].toObject()["operation"]=="join",
        "Expected authored Join");
    const auto p=join["parameters"].toObject();const auto bodyId=p["targetBodyIds"].toArray().first().toString();
    const auto initial=root(doc,bodyId),initialAuthored=doc.authored();
    const auto failure=CadDocument::chamferCandidate(initialAuthored,{{"ownerId",join["ownerId"]},
        {"targetBodyId",bodyId},{"edgeIds",QJsonArray{initial["edgeId"]}},{"distanceMm",100}});
    const auto revision=doc.revision();
    check(!doc.commit(failure,"Oversized root preview").isEmpty(),"Unexpected oversized Chamfer accepted");
    check(doc.authored()==initialAuthored && doc.revision()==revision,"Failed root finish mutated authoring");
    const auto edited=CadDocument::extrusionCandidate(doc.authored(),{{"featureId",join["id"]},
        {"sketchId",p["sketchId"]},{"profileIds",p["profileIds"]},{"distanceMm",5.25}});
    check(doc.commit(edited,"Edit Join depth").isEmpty(),"Source Join edit failed");
    check(root(doc,bodyId)==initial,"Join depth edit changed root semantic identity");
    const auto final=doc.authored();doc.history().undo();
    check(doc.authored()==initialAuthored && root(doc,bodyId)==initial,"Undo lost original root");
    doc.history().redo();check(doc.authored()==final && root(doc,bodyId)==initial,"Redo lost edited root");
    if(argc==3){const auto path=QString::fromLocal8Bit(argv[2]);check(doc.save(path).isEmpty(),"Join save failed");
        CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==final
            && root(reopened,bodyId)==initial,"Reopen changed exact Join root identity");}
    std::cout<<"JOIN_ROOT_PASS "<<initial["edgeId"].toString().toStdString()<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
