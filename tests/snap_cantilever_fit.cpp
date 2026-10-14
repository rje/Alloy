// SPDX-License-Identifier: MIT
// Focused source-shaped probe; pass the accepted Case with its retained Cut-side plane and Project sketch.
#include "CadDocument.h"
#include "SketchGeometry.h"
#include "SolidFillet.h"
#include <QCoreApplication>
#include <QJsonDocument>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void commit(CadDocument& doc,const QJsonObject& candidate){
    const auto error=doc.commit(candidate,"Stepped cantilever proof");
    if(!error.isEmpty())throw std::runtime_error((error+" "+QString::fromUtf8(QJsonDocument(doc.lastEvaluationError()).toJson())).toStdString());
}
std::pair<double,double> local(const QJsonObject& frame,double x,double y,double z){
    const double world[]={x,y,z};const auto origin=frame["origin"].toArray();
    auto component=[&](const char* key){double value=0;const auto axis=frame[key].toArray();
        for(int i=0;i<3;++i)value+=(world[i]-origin[i].toDouble())*axis[i].toDouble();return value;};
    return {component("uAxis"),component("vAxis")};
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    check(argc==2 || argc==3,"Pass accepted projected-Cut Case and optional output path");
    CadDocument doc;check(doc.open(QString::fromLocal8Bit(argv[1])).isEmpty(),"Accepted Case open failed");
    const auto sketch=doc.authored()["features"].toArray().last().toObject();
    check(sketch["type"]=="sketch" && sketch["parameters"].toObject()["plane"]=="construction",
        "Expected retained Cut-side construction sketch");
    const auto context=GeometryKernel::projectionContext(doc.evaluation(),sketch);
    const auto frame=std::find_if(doc.sketches().begin(),doc.sketches().end(),
        [&](const auto& s){return s.featureId==sketch["id"].toString();})->resolvedFrame;
    const auto projectedCorner=local(frame,-28.5,17.5,12.5);
    QString pointId;
    for(const auto& value:context["lines"].toArray()){
        const auto line=value.toObject();
        for(const bool end:{false,true}){
            const auto u=line[end ? "u2Mm" : "u1Mm"].toDouble(),v=line[end ? "v2Mm" : "v1Mm"].toDouble();
            if(std::hypot(u-projectedCorner.first,v-projectedCorner.second)<1e-6)
                pointId=SketchGeometry::projectionPointId(line["id"].toString(),end);
        }
    }
    check(!pointId.isEmpty(),"Upper inner corner has no exact projected point");
    const double world[][3]={{-28.5,17.5,12.5},{-28.5,17.5,20.5},
        {-26.5,17.5,20.5},{-26.5,17.5,10.5},{-32.5,17.5,10.5},
        {-32.5,17.5,12.5},{-28.5,17.5,12.5}};
    auto edited=sketch;
    for(int i=0;i<6;++i){
        const auto a=local(frame,world[i][0],world[i][1],world[i][2]);
        const auto b=local(frame,world[i+1][0],world[i+1][1],world[i+1][2]);
        if(i==0){
            const auto result=SketchGeometry::addPointerEntity(edited,"line",
                {{"u1Mm",a.first},{"v1Mm",a.second},{"u2Mm",b.first},{"v2Mm",b.second},
                 {"startSnapTarget",QJsonObject{{"kind","point"},{"pointId",pointId}}}},context);
            if(!result.ok)throw std::runtime_error(result.error.toStdString());
            edited=result.feature;
        }else edited=SketchGeometry::addLine(edited,{{"u1Mm",a.first},{"v1Mm",a.second},
            {"u2Mm",b.first},{"v2Mm",b.second}});
    }
    commit(doc,SketchGeometry::replaceFeature(doc.authored(),edited));
    const auto found=std::find_if(doc.sketches().begin(),doc.sketches().end(),
        [&](const auto& s){return s.featureId==sketch["id"].toString();});
    check(found!=doc.sketches().end(),"Edited sketch unavailable");
    std::cout<<"PROFILE_COUNT "<<found->profiles.size()<<'\n';
    for(const auto& profile:found->profiles)
        std::cout<<"PROFILE "<<profile.profileId.toStdString()<<" area "
            <<profile.summary["areaMm2"].toDouble()<<" edges "
            <<profile.summary["edgeProvenance"].toArray().size()<<'\n';
    QJsonArray joinedProfiles;
    for(const auto& profile:found->profiles){
        const double area=profile.summary["areaMm2"].toDouble();
        if(std::abs(area-20)<1e-6 || std::abs(area-7)<1e-6 || std::abs(area-1)<1e-6)
            joinedProfiles.append(profile.profileId);
    }
    check(joinedProfiles.size()==3,"Three intended source cells absent");
    std::vector<EvaluatedProfile> regions;
    for(const auto& profile:found->profiles)if(joinedProfiles.contains(profile.profileId))regions.push_back(profile);
    const auto combined=GeometryKernel::unionProfiles(regions);
    check(std::abs(combined.summary["areaMm2"].toDouble()-28)<1e-6,
        "Selected source cells do not certify the intended connected area");
    std::cout<<"UNION "<<combined.summary["areaMm2"].toDouble()<<' '<<combined.boundaryEdges.size()<<'\n';
    QString topId,ownerId;
    for(const auto& feature:doc.authored()["features"].toArray())if(feature.toObject()["type"]=="splitBody"){
        topId=feature.toObject()["bodyId"].toString();ownerId=feature.toObject()["ownerId"].toString();}
    check(!topId.isEmpty(),"Top body absent");
    auto joined=CadDocument::extrusionCandidate(doc.authored(),{{"ownerId",ownerId},{"sketchId",sketch["id"]},
        {"profileIds",joinedProfiles},{"distanceMm",5},{"operation","join"},
        {"targetBodyIds",QJsonArray{topId}}});
    const auto preview=CadDocument::evaluateCandidate(joined);
    std::cout<<"JOIN_PREVIEW "<<preview.ok<<' '<<preview.error.toStdString()<<' '
        <<QString::fromUtf8(QJsonDocument(preview.diagnostics).toJson(QJsonDocument::Compact)).toStdString()<<'\n';
    check(preview.ok,"Actual selected-cell Join preview failed");
    if(preview.ok){
        const auto baselineTop=std::find_if(doc.bodies().begin(),doc.bodies().end(),
            [&](const auto& body){return body.bodyId==topId;});
        check(baselineTop!=doc.bodies().end(),"Baseline Top absent");
        const auto oldShape=baselineTop->shape;
        const auto joinId=joined["features"].toArray().last().toObject()["id"].toString();
        commit(doc,joined);
        std::cout<<"JOIN_COMMITTED "<<doc.bodies().size()<<" cachedTools "
            <<doc.evaluation().joinFeatureTools.value(joinId).size()<<'\n';
        const auto top=std::find_if(doc.bodies().begin(),doc.bodies().end(),
            [&](const auto& body){return body.bodyId==topId;});
        check(top!=doc.bodies().end(),"Joined Top absent");
        for(const auto& value:SolidFillet::descriptors(top->solidEdges)){
            const auto edge=value.toObject();
            if(edge["provenance"].toObject()["generatorFeatureId"]!=joinId || !edge["available"].toBool())continue;
            std::cout<<"JOIN_EDGE "<<edge["edgeId"].toString().toStdString()<<' '
                <<QString::fromUtf8(QJsonDocument(edge["startMm"].toArray()).toJson(QJsonDocument::Compact)).toStdString()<<' '
                <<QString::fromUtf8(QJsonDocument(edge["endMm"].toArray()).toJson(QJsonDocument::Compact)).toStdString()<<'\n';
        }
        double maxAddedInterference=0;
        for(const auto& other:doc.bodies())if(other.bodyId!=topId){
            BRepAlgoAPI_Common shared(top->shape,other.shape);shared.Build();
            check(shared.IsDone(),"Interference Boolean failed");
            GProp_GProps properties;BRepGProp::VolumeProperties(shared.Shape(),properties);
            BRepAlgoAPI_Common oldShared(oldShape,other.shape);oldShared.Build();
            check(oldShared.IsDone(),"Baseline interference Boolean failed");
            GProp_GProps oldProperties;BRepGProp::VolumeProperties(oldShared.Shape(),oldProperties);
            const double added=properties.Mass()-oldProperties.Mass();
            maxAddedInterference=std::max(maxAddedInterference,added);
            if(std::abs(properties.Mass())>1e-6 || std::abs(added)>1e-6)
                std::cout<<"INTERFERENCE "<<other.bodyId.toStdString()<<' '<<other.name.toStdString()
                    <<" before "<<oldProperties.Mass()<<" after "<<properties.Mass()<<" added "<<added<<'\n';
        }
        std::cout<<"MAX_ADDED_INTERFERENCE "<<maxAddedInterference<<'\n';
        check(maxAddedInterference<1e-6,"Snap Join added interference with another actual body");
    }
    if(argc==3)check(doc.save(QString::fromLocal8Bit(argv[2])).isEmpty(),"Join save failed");
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
