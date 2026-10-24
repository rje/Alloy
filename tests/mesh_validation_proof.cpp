// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "MeshImport.h"
#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <iostream>
#include <stdexcept>
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
QJsonObject box(QJsonObject candidate,const QString& featureId={}){
    const auto root=candidate.value("documentId");auto features=candidate.value("features").toArray();
    features.append(QJsonObject{{"id",featureId.isEmpty()?CadDocument::newId():featureId},{"bodyId",CadDocument::newId()},
        {"ownerId",root},{"type","box"},{"name","Box"},{"visible",true},
        {"parameters",QJsonObject{{"widthMm",4},{"depthMm",5},{"heightMm",6},{"xMm",0},{"yMm",0},{"zMm",0}}}});
    candidate["features"]=features;return candidate;
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    QTemporaryDir dir;check(dir.isValid(),"temporary directory");const auto source=dir.filePath("owned.stl");
    QFile input(source);check(input.open(QIODevice::WriteOnly),"source open");
    input.write("solid owned\nfacet normal 0 0 1\nouter loop\nvertex 0 0 0\nvertex 1 0 0\nvertex 0 1 0\nendloop\nendfacet\nendsolid owned\n");input.close();
    const auto mesh=MeshImport::readStl(source,"mm");CadDocument document;
    const auto prepared=CadDocument::prepareMeshImport(document.authored(),mesh);
    check(document.commitMeshImport(*prepared,document.state().value("revision").toInteger()).isEmpty(),"trusted import commit");
    const auto imported=document.authored();auto candidate=box(imported);
    check(document.validateOwnedCandidate(candidate).isEmpty(),"unchanged owned mesh candidate");
    check(document.commit(candidate,"Box after owned mesh").isEmpty(),"Box commit");const auto committed=document.authored();
    document.history().undo();check(document.authored()==imported,"Undo restores owned mesh snapshot");
    document.history().redo();check(document.authored()==committed,"Redo restores owned mesh snapshot");
    // Same user UUID/SHA cannot certify changed coordinates or geometry-derived quality.
    auto corrupt=committed;auto records=corrupt.value("meshes").toArray();auto record=records.first().toObject();
    auto vertices=record.value("verticesMm").toArray();vertices[0]=QJsonArray{1e10,0,0};record["verticesMm"]=vertices;records[0]=record;corrupt["meshes"]=records;
    const auto error=document.validateOwnedCandidate(corrupt);check(error=="Invalid mesh coordinate","changed record must revalidate");
    const auto count=document.history().count();check(document.commit(corrupt,"Refused replacement")==error,"replacement commit exact refusal");
    check(document.authored()==committed && document.history().count()==count,"replacement refusal is atomic");
    check(!document.validateOwnedCandidate(box(committed,mesh.value("id").toString())).isEmpty(),"mesh proof must not skip document UUID collision checks");
    const auto invalid=dir.filePath("same-id-sha.alloy");QFile file(invalid);check(file.open(QIODevice::WriteOnly),"tampered native open");file.write(QJsonDocument(corrupt).toJson());file.close();
    check(!document.open(invalid).isEmpty() && document.authored()==committed,"untrusted reopen revalidates and preserves current document");
    const auto native=dir.filePath("owned.alloy");check(document.save(native).isEmpty(),"native save");check(QFile::remove(source),"remove original STL");
    CadDocument reopened;check(reopened.open(native).isEmpty() && reopened.authored()==committed,"source-independent reopen");
    auto next=box(reopened.authored());check(reopened.validateOwnedCandidate(next).isEmpty() && reopened.commit(next,"Box after reopen").isEmpty(),"fresh reopen proof supports later commit");
    reopened.reset();check(reopened.authored().value("meshes").toArray().isEmpty(),"new document drops owned mesh proof");
    std::cout<<"PASS owned mesh proof: Box/history, changed same-ID/SHA refusal, namespace collision, untrusted reopen and source-independent reload\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
