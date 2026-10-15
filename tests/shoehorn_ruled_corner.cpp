// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <stdexcept>
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    check(argc==2,"Pass the accepted thickened shoehorn fixture path");
    CadDocument doc;const auto openError=doc.open(QString::fromLocal8Bit(argv[1]));
    if(!openError.isEmpty())throw std::runtime_error(("Open thickened fixture: "+openError).toStdString());
    check(doc.bodies().size()==2,"Expected source sheet and solid blank");
    const auto sheet=doc.bodies().front(),solid=doc.bodies().back();
    QMap<QString,QString> sourceEdges;
    for(const auto& edge:sheet.solidEdges)sourceEdges[edge.provenance.value("role").toString()]=edge.edgeId;
    QJsonArray groups;
    for(const auto& role:QStringList{"start","end"}){
        QJsonArray seeds;for(const auto& edge:solid.solidEdges)
            if(edge.provenance.value("role").toString().startsWith("corner/")
                && edge.provenance.value("role").toString().split('/').contains(sourceEdges[role]))seeds.append(edge.edgeId);
        check(seeds.size()==2,"Corner seeds unavailable");
        groups.append(QJsonObject{{"id",CadDocument::newId()},{"edgeIds",seeds},{"radiusMm",role=="start"?12.0:16.0},{"tangentChain",true}});
    }
    const auto candidate=CadDocument::filletCandidate(doc.authored(),{{"targetBodyId",solid.bodyId},{"groups",groups}});
    const auto error=doc.commit(candidate,"Round four corners");
    if(!error.isEmpty())throw std::runtime_error(error.toStdString());
    const auto result=doc.bodies().back();
    check(result.summary.value("construction")=="ruledCornerCut","Bounded construction was not used");
    check(result.summary.value("valid").toBool() && result.summary.value("closed").toBool(),"Result invalid");
    check(result.supportFaces.size()==10,"Four round supports not retained");
    int rounds=0;QSet<QString> roundFaceIds,roundEdgeIds,roundChainIds;
    for(const auto& face:result.supportFaces)if(face.role=="fillet.round"){
        ++rounds;roundFaceIds.insert(face.faceId);
        check(face.boundaryEdges.size()>=2,"Round source/offset boundaries unavailable");
        QSet<QString> chains;
        for(const auto& edge:face.boundaryEdges){
            check(!edge.edge.IsNull(),"Generated edge unavailable");
            roundEdgeIds.insert(edge.edgeId);chains.insert(edge.provenance.value("boundaryChainId").toString());
            roundChainIds.insert(edge.provenance.value("boundaryChainId").toString());
        }
        check(chains.size()==2,"Two logical source/offset chains unavailable");
    }
    check(rounds==4,"Four named round faces unavailable");
    check(roundFaceIds.size()==4 && roundChainIds.size()==8 && roundEdgeIds.size()>=8,"Generated identities repeat");
    const auto rearSeeds=groups[1].toObject().value("edgeIds").toArray();
    QString retainedFaceId,retainedEdgeId,retainedProducer;QJsonArray retainedStart;
    for(const auto& face:result.supportFaces)if(face.role=="fillet.round"){
        bool rear=false;for(const auto& value:rearSeeds)rear|=value.toString()==face.provenance.value("sourceSeedEdgeId").toString();
        if(rear)for(const auto& edge:face.boundaryEdges)
            if(!edge.provenance.value("terminalWallFaceId").toString().isEmpty()){
                retainedFaceId=face.faceId;retainedEdgeId=edge.edgeId;
                retainedProducer=face.producerFeatureId;retainedStart=edge.provenance.value("startMm").toArray();break;
            }
        if(!retainedEdgeId.isEmpty())break;
    }
    check(!retainedEdgeId.isEmpty(),"Rear already-split terminal edge unavailable");
    const QJsonObject retainedTangent{{"face",QJsonObject{{"bodyId",result.bodyId},{"faceId",retainedFaceId},{"producerFeatureId",retainedProducer}}},
        {"point",QJsonObject{{"bodyId",result.bodyId},{"edgeId",retainedEdgeId},{"endpoint",0}}}};
    auto withRetainedPlane=doc.authored();auto retainedFeatures=withRetainedPlane.value("features").toArray();
    retainedFeatures.append(QJsonObject{{"id",CadDocument::newId()},{"ownerId",result.ownerId},{"type","constructionPlane"},
        {"name","Retained round tangent"},{"visible",true},
        {"parameters",QJsonObject{{"source",QJsonObject{{"tangent",retainedTangent}}}}}});
    withRetainedPlane["features"]=retainedFeatures;
    check(doc.commit(withRetainedPlane,"Retained round tangent").isEmpty(),"Baseline generated tangent failed");
    auto retainedOrigin=doc.evaluation().constructionPlanes.back().frame.value("origin").toArray();
    for(int coordinate=0;coordinate<3;++coordinate)check(std::abs(retainedOrigin[coordinate].toDouble()-retainedStart[coordinate].toDouble())<1e-6,
        "Baseline generated tangent missed endpoint");
    const double volume=result.summary.value("volumeMm3").toDouble();
    std::cerr<<"corner volume "<<std::setprecision(17)<<volume<<'\n';
    check(std::abs(volume-17033.6912694748)<1e-2,"Unexpected result volume");
    const auto stable=doc.authored();doc.history().undo();doc.history().redo();check(doc.authored()==stable,"Undo/redo changed authored result");
    QTemporaryDir directory;const auto path=directory.filePath("corners.alloy");
    check(doc.save(path).isEmpty(),"Save result");CadDocument opened;
    check(opened.open(path).isEmpty() && opened.authored()==stable,"Reopen result");
    check(opened.bodies().back().summary.value("construction")=="ruledCornerCut","Reopened corner result changed");
    auto candidateEdit=stable;auto features=candidateEdit.value("features").toArray();
    auto spine=features.first().toObject();QString lengthDimension;
    for(const auto& value:spine.value("parameters").toObject().value("dimensions").toArray()){
        const auto row=value.toObject();if(row.value("kind")=="horizontal" && row.value("valueMm").toDouble()==130)lengthDimension=row.value("id").toString();
    }
    check(!lengthDimension.isEmpty(),"Source length dimension unavailable");
    const auto changed=SketchGeometry::updateDimension(spine,{{"dimensionId",lengthDimension},{"valueMm",140.0}});
    check(changed.ok,"Length edit did not solve");features[0]=changed.feature;candidateEdit["features"]=features;
    const auto editError=opened.commit(candidateEdit,"Length 140");
    if(!editError.isEmpty())throw std::runtime_error(("Length edit did not replay corner Fillet: "+editError).toStdString());
    const auto edited=opened.bodies().back();QSet<QString> editedFaces,editedEdges,editedChains;
    for(const auto& face:edited.supportFaces)if(face.role=="fillet.round"){
        editedFaces.insert(face.faceId);for(const auto& edge:face.boundaryEdges){
            editedEdges.insert(edge.edgeId);editedChains.insert(edge.provenance.value("boundaryChainId").toString());
        }
    }
    check(editedFaces==roundFaceIds && editedChains==roundChainIds,
        "Generated round face or logical chain IDs changed under upstream length edit");
    check(editedEdges.size()>=roundEdgeIds.size(),"Edited round boundary lost actual segments");
    const auto retainedAfter=std::find_if(edited.solidEdges.begin(),edited.solidEdges.end(),[&](const auto& edge){return edge.edgeId==retainedEdgeId;});
    check(retainedAfter!=edited.solidEdges.end(),"Already-split terminal edge ID changed on source edit");
    retainedOrigin=opened.evaluation().constructionPlanes.back().frame.value("origin").toArray();
    const auto movedStart=retainedAfter->provenance.value("startMm").toArray();
    for(int coordinate=0;coordinate<3;++coordinate)check(std::abs(retainedOrigin[coordinate].toDouble()-movedStart[coordinate].toDouble())<1e-6,
        "Retained generated tangent missed edited endpoint");
    check(std::abs(edited.summary.value("volumeMm3").toDouble()-volume)>1,"Length edit did not affect physical solid");
    const auto authoredEdited=opened.authored();opened.history().undo();check(opened.authored()==stable,"Length undo failed");
    opened.history().redo();check(opened.authored()==authoredEdited,"Length redo failed");
    const EvaluatedSupportFace* rearFace=nullptr;const EvaluatedSupportEdge* endpointEdge=nullptr;
    for(const auto& face:edited.supportFaces)if(face.role=="fillet.round"){
        bool rear=false;for(const auto& value:rearSeeds)rear|=value.toString()==face.provenance.value("sourceSeedEdgeId").toString();
        if(rear)for(const auto& edge:face.boundaryEdges)
            if(!edge.provenance.value("terminalWallFaceId").toString().isEmpty()){
                rearFace=&face;endpointEdge=&edge;break;
            }
        if(endpointEdge)break;
    }
    check(rearFace && endpointEdge,"Rear generated terminal endpoint unavailable");
    const QJsonObject tangent{{"face",QJsonObject{{"bodyId",edited.bodyId},{"faceId",rearFace->faceId},{"producerFeatureId",rearFace->producerFeatureId}}},
        {"point",QJsonObject{{"bodyId",edited.bodyId},{"edgeId",endpointEdge->edgeId},{"endpoint",0}}}};
    auto withPlane=opened.authored();auto planeFeatures=withPlane.value("features").toArray();
    planeFeatures.append(QJsonObject{{"id",CadDocument::newId()},{"ownerId",edited.ownerId},{"type","constructionPlane"},
        {"name","Round endpoint tangent"},{"visible",true},
        {"parameters",QJsonObject{{"source",QJsonObject{{"tangent",tangent}}}}}});
    withPlane["features"]=planeFeatures;
    check(opened.commit(withPlane,"Retained tangent").isEmpty(),"Generated endpoint tangent plane failed");
    const auto origin=opened.evaluation().constructionPlanes.back().frame.value("origin").toArray();
    const auto expected=endpointEdge->provenance.value("startMm").toArray();
    for(int coordinate=0;coordinate<3;++coordinate)check(std::abs(origin[coordinate].toDouble()-expected[coordinate].toDouble())<1e-6,
        "Tangent origin missed generated endpoint");
    const auto beforeInterior=opened.authored();auto interiorTangent=tangent;
    auto interiorPoint=interiorTangent.value("point").toObject();interiorPoint["endpoint"]=1;
    interiorTangent["point"]=interiorPoint;
    auto interiorCandidate=beforeInterior;auto interiorFeatures=interiorCandidate.value("features").toArray();
    interiorFeatures.append(QJsonObject{{"id",CadDocument::newId()},{"ownerId",edited.ownerId},{"type","constructionPlane"},
        {"name","Interior split vertex"},{"visible",true},
        {"parameters",QJsonObject{{"source",QJsonObject{{"tangent",interiorTangent}}}}}});
    interiorCandidate["features"]=interiorFeatures;
    check(!opened.commit(interiorCandidate,"Unsupported interior tangent").isEmpty() && opened.authored()==beforeInterior,
        "Interior split endpoint was accepted or changed committed model");
    CadDocument refused;check(refused.open(path).isEmpty(),"Open source for refusal test");
    auto refusedFeatures=stable.value("features").toArray();auto refusedSpine=refusedFeatures.first().toObject();
    QString bendDimension;
    for(const auto& value:refusedSpine.value("parameters").toObject().value("dimensions").toArray()){
        const auto row=value.toObject();if(row.value("kind")=="vertical" && row.value("valueMm").toDouble()==16)bendDimension=row.value("id").toString();
    }
    check(!bendDimension.isEmpty(),"Source bend dimension unavailable");
    const auto bent=SketchGeometry::updateDimension(refusedSpine,{{"dimensionId",bendDimension},{"valueMm",18.0}});
    check(bent.ok,"Bend 18 did not solve");refusedFeatures[0]=bent.feature;
    auto bentCandidate=stable;bentCandidate["features"]=refusedFeatures;
    check(!refused.commit(bentCandidate,"Bend 18").isEmpty(),"Unsupported bend transition committed");
    check(refused.authored()==stable && refused.bodies().back().summary.value("construction")=="ruledCornerCut",
        "Refused bend changed committed model");
    std::cout<<"SHOEHORN_RULED_CORNER_OK volume "<<volume<<" faces "<<result.supportFaces.size()<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
