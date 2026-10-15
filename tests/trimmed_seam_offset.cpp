// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "RoundedLoopOffset.h"
#include "SketchGeometry.h"
#include <BRepAdaptor_Curve.hxx>
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QUuid>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool condition,const char* message) {if(!condition)throw std::runtime_error(message);}
EvaluatedSupportFace cap(const CadDocument& document) {
    for(const auto& body:document.bodies())for(const auto& face:body.supportFaces)
        if(face.role=="split.negative.cap")return face;
    throw std::runtime_error("saved split cap unavailable");
}
QJsonObject insetSketch(const CadDocument& document) {
    const auto support=cap(document);
    QString ownerId;
    for(const auto& body:document.bodies())if(body.bodyId==support.bodyId)ownerId=body.ownerId;
    check(!ownerId.isEmpty(),"split cap owning component unavailable");
    const QJsonObject reference{{"producerFeatureId",support.producerFeatureId},
        {"bodyId",support.bodyId},{"faceId",support.faceId}};
    auto sketch=SketchGeometry::newSupportedFeature(ownerId,reference,"Trimmed seam inset",{});
    auto parameters=sketch["parameters"].toObject();
    parameters["loopOffsets"]=QJsonArray{QJsonObject{{"id",CadDocument::newId()},
        {"kind","planarInsetV1"},{"outerLoopId",RoundedLoopOffset::insetLoopId(support)},
        {"distanceMm",-1.25},{"chain",true}}};
    sketch["parameters"]=parameters;return sketch;
}
QSet<QString> edgeIds(const EvaluatedProfile& profile) {
    QSet<QString> ids;for(const auto& edge:profile.boundaryEdges)ids.insert(edge.sourceEntityId);return ids;
}
}

int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    try {
        check(argc==2,"usage: trimmed_seam_offset_test /absolute/path/split-prefix.alloy");
        CadDocument document;check(document.open(argv[1]).isEmpty(),"saved source could not reopen");
        auto sketch=insetSketch(document);
        const auto initial=document.authored();
        const auto commitError=document.commit(SketchGeometry::replaceFeature(document.authored(),sketch),"Retained seam inset");
        if(!commitError.isEmpty())throw std::runtime_error(("retained inset sketch commit refused: "+commitError).toStdString());
        const auto profile=document.sketches().back().profiles.front();
        check(std::abs(profile.summary["areaMm2"].toDouble()-233.8576962999306)<1e-5,
            "retained inset interior area changed");
        check(profile.boundaryEdges.size()==17 && edgeIds(profile).size()==17,
            "retained inset profile lost generated edge identities");
        QSet<QString> contacts;
        for(const auto& edge:profile.boundaryEdges) {
            check(!edge.startContact.isEmpty() && !edge.endContact.isEmpty(),
                "inset edge lacks stable endpoint contacts");
            contacts.insert(edge.startContact);contacts.insert(edge.endContact);
        }
        check(contacts.size()==17,"inset wire contacts are not closed and unique");
        QString bodyOwner;
        for(const auto& body:document.bodies())if(body.bodyId==cap(document).bodyId)bodyOwner=body.ownerId;
        const auto bodyId=cap(document).bodyId;
        const auto profileId=profile.profileId;
        const auto join=CadDocument::extrusionCandidate(document.authored(),
            {{"ownerId",bodyOwner},{"sketchId",sketch["id"]},{"profileId",profileId},
             {"distanceMm",2.0},{"operation","join"},{"targetBodyIds",QJsonArray{bodyId}}});
        const auto joinError=document.commit(join,"Inset interior Join");
        if(!joinError.isEmpty())throw std::runtime_error(("inset Join refused: "+joinError).toStdString());
        QString selectedEdge;double longest=0;
        for(const auto& body:document.bodies())if(body.bodyId==bodyId)
            for(const auto& edge:body.solidEdges) {
                if(edge.edge.IsNull() || edge.provenance.value("sourceSketchId")!=sketch["id"]
                    || edge.provenance.value("role")!="endRim")continue;
                const BRepAdaptor_Curve curve(edge.edge);
                if(curve.GetType()!=GeomAbs_Line)continue;
                const auto length=curve.Value(curve.FirstParameter()).Distance(curve.Value(curve.LastParameter()));
                if(length>longest){selectedEdge=edge.edgeId;longest=length;}
            }
        check(!selectedEdge.isEmpty() && longest>1,"joined inset line has no selected retained rim edge");
        const auto fillet=CadDocument::filletCandidate(document.authored(),
            {{"ownerId",bodyOwner},{"targetBodyId",bodyId},
             {"edgeIds",QJsonArray{selectedEdge}},{"radiusMm",0.5}});
        const auto filletError=document.commit(fillet,"Selected inset rim Fillet");
        if(!filletError.isEmpty())throw std::runtime_error(("selected inset Fillet refused: "+filletError).toStdString());
        const auto retained=document.authored();
        for(int i=0;i<3;++i)document.history().undo();
        check(document.authored()==initial,"undo did not remove retained inset, Join and Fillet");
        for(int i=0;i<3;++i)document.history().redo();
        check(document.authored()==retained,"redo did not restore retained inset, Join and Fillet");
        QTemporaryDir temporary;check(temporary.isValid(),"temporary save directory failed");
        const auto path=temporary.filePath("retained-inset.alloy");
        check(document.save(path).isEmpty(),"retained inset save failed");
        CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==retained,
            "retained inset reopen failed");
        const auto replay=reopened.sketches().back().profiles.front();
        check(replay.profileId==profile.profileId && edgeIds(replay)==edgeIds(profile),
            "retained inset profile identities changed on reopen");
        auto widened=retained;auto features=widened["features"].toArray();bool changed=false;
        for(int i=0;i<features.size();++i) {
            auto feature=features[i].toObject();if(feature["name"]!="Centered case stock")continue;
            auto parameters=feature["parameters"].toObject();auto dimensions=parameters["dimensions"].toArray();
            for(int j=0;j<dimensions.size();++j) {
                auto dimension=dimensions[j].toObject();
                if(dimension["kind"]!="horizontal" || dimension["valueMm"].toDouble()!=64)continue;
                dimension["valueMm"]=65;dimensions[j]=dimension;changed=true;break;
            }
            parameters["dimensions"]=dimensions;feature["parameters"]=parameters;features[i]=feature;break;
        }
        check(changed,"upstream stock width dimension unavailable");widened["features"]=features;
        const auto widthError=reopened.commit(widened,"Widen source stock 64 to 65");
        if(!widthError.isEmpty())throw std::runtime_error(("upstream width edit refused: "+widthError).toStdString());
        const auto widenedProfile=reopened.sketches().back().profiles.front();
        check(widenedProfile.profileId==profile.profileId && edgeIds(widenedProfile)==edgeIds(profile)
            && std::abs(widenedProfile.summary["areaMm2"].toDouble()-profile.summary["areaMm2"].toDouble())>0.01,
            "upstream width edit lost downstream inset lineage or geometry");
        const auto acceptedWidth=reopened.authored();
        reopened.history().undo();check(reopened.authored()==retained,"source width undo lost retained feature graph");
        reopened.history().redo();check(reopened.authored()==acceptedWidth,"source width redo lost retained feature graph");
        const auto widenedPath=temporary.filePath("widened-retained-inset.alloy");
        check(reopened.save(widenedPath).isEmpty(),"widened retained inset save failed");
        CadDocument widenedReopen;check(widenedReopen.open(widenedPath).isEmpty()
            && widenedReopen.authored()==acceptedWidth,"widened retained inset reopen failed");
        check(edgeIds(widenedReopen.sketches().back().profiles.front())==edgeIds(profile),
            "widened reopened inset edge IDs changed");
        auto collapsed=acceptedWidth;auto collapsedFeatures=collapsed["features"].toArray();
        for(int i=0;i<collapsedFeatures.size();++i) {
            auto feature=collapsedFeatures[i].toObject();if(feature["id"]!=sketch["id"])continue;
            auto parameters=feature["parameters"].toObject();auto offsets=parameters["loopOffsets"].toArray();
            auto offset=offsets.first().toObject();offset["distanceMm"]=-100;offsets[0]=offset;
            parameters["loopOffsets"]=offsets;feature["parameters"]=parameters;collapsedFeatures[i]=feature;break;
        }
        collapsed["features"]=collapsedFeatures;
        check(!widenedReopen.commit(collapsed,"Collapsed inset").isEmpty()
            && widenedReopen.authored()==acceptedWidth,"collapsed inset changed the valid document");
        auto missing=acceptedWidth;auto missingFeatures=missing["features"].toArray();
        for(int i=0;i<missingFeatures.size();++i) {
            auto feature=missingFeatures[i].toObject();if(feature["id"]!=sketch["id"])continue;
            auto parameters=feature["parameters"].toObject();auto support=parameters["support"].toObject();
            support["faceId"]=CadDocument::newId();parameters["support"]=support;
            feature["parameters"]=parameters;missingFeatures[i]=feature;break;
        }
        missing["features"]=missingFeatures;
        check(!widenedReopen.commit(missing,"Missing inset support").isEmpty()
            && widenedReopen.authored()==acceptedWidth,"missing support changed the valid document");
        std::cout<<"PASS retained inset area="<<replay.summary["areaMm2"].toDouble()
            <<" widened="<<widenedProfile.summary["areaMm2"].toDouble()
            <<" edges="<<replay.boundaryEdges.size()<<" contacts="<<contacts.size()
            <<" Join/selected-Fillet undo/redo/reopen\n";
        return 0;
    } catch(const std::exception& error) {
        std::cerr<<"FAIL "<<error.what()<<'\n';return 1;
    }
}
