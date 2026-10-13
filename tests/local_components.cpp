// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include "MeshExport.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <StlAPI_Reader.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <cmath>
#include <iostream>
#include <stdexcept>
namespace {
void check(bool ok,const QString& message){if(!ok)throw std::runtime_error(message.toStdString());}
void commit(CadDocument& d,const QJsonObject& candidate){const auto error=d.commit(candidate,"Test edit");check(error.isEmpty(),error);}
void append(CadDocument& d,const QJsonObject& feature){auto candidate=d.authored();auto features=candidate["features"].toArray();features.append(feature);candidate["features"]=features;commit(d,candidate);}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument d;const auto root=d.activeOwnerId();
    append(d,{{"id",CadDocument::newId()},{"bodyId",CadDocument::newId()},{"ownerId",root},{"type","box"},{"name","Root stock"},{"visible",true},{"parameters",QJsonObject{{"widthMm",100},{"depthMm",100},{"heightMm",5},{"xMm",0},{"yMm",0},{"zMm",0}}}});
    auto candidate=CadDocument::componentCandidate(d.authored(),"die");const auto definition=candidate["components"].toArray()[0].toObject()["id"].toString();const auto occurrence=candidate["occurrences"].toArray()[0].toObject()["id"].toString();
    check(definition!=occurrence && definition!=root,"Definition/occurrence/root identities must differ");commit(d,candidate);
    const auto beforeActivation=d.authored();const auto revision=d.revision();const auto count=d.history().count();check(d.activateOwner(definition).isEmpty(),"Activation failed");
    check(d.authored()==beforeActivation && d.revision()==revision && d.history().count()==count,"Activation must not edit CAD or history");
    auto rectangle=SketchGeometry::addCenterRectangle(SketchGeometry::newFeature(d.activeOwnerId(),"XY","Die sketch"),{{"centerUMm",0},{"centerVMm",0},{"widthMm",20},{"heightMm",20}});check(rectangle.ok,rectangle.error);
    commit(d,SketchGeometry::replaceFeature(d.authored(),rectangle.feature));const auto sketchId=rectangle.feature["id"].toString();
    const auto profile=d.sketches()[0].profiles[0].profileId;
    commit(d,CadDocument::extrusionCandidate(d.authored(),{{"ownerId",d.activeOwnerId()},{"sketchId",sketchId},{"profileId",profile},{"distanceMm",20}}));
    check(d.bodies().size()==2 && d.bodies()[1].ownerId==definition,"Extrusion must belong to component");const auto dieBody=d.bodies()[1].bodyId;
    QJsonArray edges;for(const auto& edge:d.bodies()[1].solidEdges)edges.append(edge.edgeId);check(edges.size()==12,"Die cube has twelve edges");
    commit(d,CadDocument::filletCandidate(d.authored(),{{"ownerId",definition},{"targetBodyId",dieBody},{"edgeIds",edges},{"radiusMm",1},{"tangentChain",true}}));
    const auto authored=d.authored();check(d.activateOwner(root).isEmpty(),"Activate root failed");
    const auto fillet=authored["features"].toArray().last().toObject();commit(d,CadDocument::filletCandidate(d.authored(),{{"ownerId",root},{"featureId",fillet["id"]},{"targetBodyId",dieBody},{"edgeIds",edges},{"radiusMm",1.2},{"tangentChain",true}}));
    check(d.authored()["features"].toArray().last().toObject()["ownerId"]==definition,"Edit must preserve original owner after root activation");
    d.history().undo();check(d.authored()==authored,"Exact undo authored chain");d.history().redo();const auto edited=d.authored();
    check(d.bodies()[1].ownerId==definition && d.bodies()[0].ownerId==root,"Modifier must preserve distinct evaluated body ownership");
    // Unsupported cross-owner support and modifiers fail without changing the design.
    const auto oldRevision=d.revision();candidate=CadDocument::moveCandidate(d.authored(),{{"ownerId",root},{"targetBodyId",dieBody},{"xMm",1}});
    check(!d.commit(candidate,"Invalid cross owner").isEmpty() && d.lastEvaluationError()["code"]=="cross_component_dependency","Cross-owner modifier rejected explicitly");check(d.authored()==edited && d.revision()==oldRevision,"Cross-owner refusal must be atomic");
    candidate=d.authored();auto bad=candidate["features"].toArray();auto feature=bad[0].toObject();feature["ownerId"]=CadDocument::newId();bad[0]=feature;candidate["features"]=bad;check(!d.commit(candidate,"Dangling owner").isEmpty(),"Dangling feature owner rejected");
    candidate=d.authored();candidate["occurrences"]=QJsonArray{};check(!d.commit(candidate,"Missing occurrence").isEmpty(),"Definition without occurrence rejected");
    candidate=d.authored();auto occurrences=candidate["occurrences"].toArray();auto nested=occurrences[0].toObject();nested["parentId"]=definition;occurrences[0]=nested;candidate["occurrences"]=occurrences;check(!d.commit(candidate,"Nesting").isEmpty(),"Nesting rejected");
    QTemporaryDir directory;check(directory.isValid(),"Temporary directory");const auto path=directory.filePath("components.alloy");check(d.save(path).isEmpty(),"Save components");CadDocument reopened;check(reopened.open(path).isEmpty(),"Reopen components");check(reopened.authored()==edited && reopened.activeOwnerId()==root,"Reopen preserves authored structure and resets activation to root");
    std::vector<EvaluatedBody> scoped;for(const auto& body:reopened.bodies())if(body.ownerId==definition)scoped.push_back(body);check(scoped.size()==1,"Component scope resolves exactly actual die body");
    const auto stl=directory.filePath("die.stl");const auto exported=MeshExport::writeStl(scoped,stl,"mm",true,0.1,10);check(exported["bodyCount"]==1 && exported["bodyIds"].toArray()==QJsonArray{dieBody},"STL scope excludes root stock");
    TopoDS_Shape readback;check(StlAPI_Reader().Read(readback,stl.toUtf8().constData()),"Independent OCCT STL readback");Bnd_Box bounds;BRepBndLib::Add(readback,bounds);double x0,y0,z0,x1,y1,z1;bounds.Get(x0,y0,z0,x1,y1,z1);check(std::abs(x1-x0-20)<0.01 && std::abs(y1-y0-20)<0.01 && std::abs(z1-z0-20)<0.01,"Readback mm bounds exclude 100mm root stock");
    // Undo component creation in a minimal design removes the occurrence and
    // makes a removed active definition fall back to root; redo restores IDs.
    CadDocument history;const auto created=CadDocument::componentCandidate(history.authored(),"Empty");commit(history,created);const auto id=created["components"].toArray()[0].toObject()["id"].toString();history.activateOwner(id);history.history().undo();check(history.activeOwnerId()==history.authored()["documentId"].toString(),"Removed active owner falls back to root");history.history().redo();check(history.authored()==created,"Component redo retains definition and occurrence IDs");
    std::cout<<"local component ownership/history/reopen/STL readback passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
