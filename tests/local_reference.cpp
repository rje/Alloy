// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "LocalReference.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <TopoDS.hxx>
#include <cmath>
#include <iostream>

namespace {
void need(bool value,const char* message){if(!value)throw std::runtime_error(message);}
gp_Pnt center(const TopoDS_Shape& shape){Bnd_Box bounds;BRepBndLib::Add(shape,bounds);Standard_Real x0,y0,z0,x1,y1,z1;
    bounds.Get(x0,y0,z0,x1,y1,z1);return gp_Pnt((x0+x1)/2,(y0+y1)/2,(z0+z1)/2);}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    const auto fixture=qEnvironmentVariable("ALLOY_ASSEMBLY_FIXTURE");if(fixture.isEmpty()){std::cout<<"SKIP: set ALLOY_ASSEMBLY_FIXTURE to the external qualified board STEP\n";return 77;}
    CadDocument document;auto created=CadDocument::localReferenceCandidate(document.authored(),{{"path",fixture},{"name","Board Reference"}});
    need(created["components"].toArray().size()==1,"one component");need(created["features"].toArray().size()==1,"one feature");
    const auto feature=created["features"].toArray()[0].toObject();const auto id=feature["id"].toString();
    need(feature["parameters"].toObject()["assembly"].toObject()["occurrences"].toArray().size()>12,"retained hierarchy");
    need(document.commit(created,"Insert Local Reference").isEmpty(),"insert transaction");
    need(document.bodies().size()==12,"twelve board leaves");
    const auto original=document.bodies()[0];need(!original.supportFaces.empty() && !original.solidEdges.empty(),"imported topology references");
    const QJsonObject placement{{"axis","Z"},{"angleDeg",90},{"translationMm",QJsonArray{10,20,30}}};
    auto moved=CadDocument::localReferencePlacementCandidate(document.authored(),id,placement);
    need(document.commit(moved,"Capture Reference Position").isEmpty(),"captured move");
    const auto& leaf=document.bodies()[0];need(leaf.bodyId==original.bodyId && leaf.occurrenceId==original.occurrenceId,"stable imported occurrence/body IDs");
    const auto expected=center(original.shape).Transformed(LocalReference::transform(placement)),actual=center(leaf.shape);
    need(expected.Distance(actual)<1e-5,"outer placement once");
    const auto faceExpected=center(original.supportFaces[0].face).Transformed(LocalReference::transform(placement));
    need(faceExpected.Distance(center(leaf.supportFaces[0].face))<1e-5,"support face moved");
    const auto edgeExpected=center(original.solidEdges[0].edge).Transformed(LocalReference::transform(placement));
    need(edgeExpected.Distance(center(leaf.solidEdges[0].edge))<1e-5,"solid edge moved");
    const auto owner=feature["ownerId"].toString();auto transparent=CadDocument::componentOpacityCandidate(document.authored(),owner,0.6);
    need(document.commit(transparent,"Opacity").isEmpty(),"opacity transaction");
    need(std::abs(document.bodies()[0].summary["opacity"].toDouble()-0.6)<1e-9,"evaluated opacity");
    document.history().undo();need(std::abs(document.bodies()[0].summary["opacity"].toDouble()-1.0)<1e-9,"opacity undo");
    document.history().undo();need(center(document.bodies()[0].shape).Distance(center(original.shape))<1e-5,"placement undo");
    document.history().redo();document.history().redo();
    const auto savedId=document.bodies()[0].bodyId;QTemporaryDir temp;need(temp.isValid(),"temp directory");
    const auto path=temp.filePath("reference.alloy");need(document.save(path).isEmpty(),"save");
    CadDocument reopened;need(reopened.open(path).isEmpty(),"reopen embedded snapshot");
    need(reopened.bodies().size()==12 && reopened.bodies()[0].bodyId==savedId,"reopen retained identity");
    need(expected.Distance(center(reopened.bodies()[0].shape))<1e-5,"reopen placement");
    need(std::abs(reopened.bodies()[0].summary["opacity"].toDouble()-0.6)<1e-9,"reopen opacity");
    bool bad=false;try{CadDocument::localReferencePlacementCandidate(reopened.authored(),id,{{"axis","Z"},{"angleDeg",90},{"translationMm",QJsonArray{0,0}}});}catch(const std::exception&){bad=true;}need(bad,"invalid placement rejected");
    std::cout<<"local reference core passed\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<"\n";return 1;}}
