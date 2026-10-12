// SPDX-License-Identifier: MIT
#include "SketchGeometry.h"
#include "SketchProfiles.h"
#include <BRepCheck_Analyzer.hxx>
#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMap>
#include <QPointF>
#include <QSet>
#include <QUuid>
#include <cmath>
#include <iostream>
#include <numbers>
#include <stdexcept>
namespace {
void check(bool ok,const QString& message){if(!ok)throw std::runtime_error(message.toStdString());}
QJsonObject accepted(const SketchGeometry::SolveResult& result){check(result.ok,result.error);return result.feature;}
QJsonObject entity(const QJsonObject& f,int index){return f["parameters"].toObject()["entities"].toArray()[index].toObject();}
QSet<QString> ids(const SketchProfiles::RegionsResult& result){QSet<QString> value;for(const auto& region:result.profiles)value.insert(region.profileId);return value;}
void inspect(const SketchProfiles::RegionsResult& result,int count){check(result.ok,result.error);check(int(result.profiles.size())==count,"Wrong number of bounded regions");
    for(const auto& region:result.profiles){check(BRepCheck_Analyzer(region.face).IsValid(),"Region face is invalid");check(region.summary["areaMm2"].toDouble()>0,"Region area is not positive");check(!region.summary["sourceEntityIds"].toArray().isEmpty(),"Region lacks authored provenance");}}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    const auto owner=QUuid::createUuid().toString(QUuid::WithoutBraces);
    auto simple=SketchGeometry::addRectangle(SketchGeometry::newFeature(owner,"XY"),{{"u1Mm",0},{"v1Mm",0},{"u2Mm",20},{"v2Mm",10}});
    const auto legacy=SketchProfiles::derive(simple);
    const auto pluralLegacy=SketchProfiles::deriveRegions(simple);
    inspect(pluralLegacy,1);check(pluralLegacy.profiles[0].profileId==legacy.profile.profileId,"Legacy boundary ID changed");
    auto circle=SketchGeometry::addCircle(SketchGeometry::newFeature(owner,"XY"),{{"centerUMm",0},{"centerVMm",0},{"diameterMm",10}});
    check(SketchProfiles::deriveRegions(circle).profiles[0].profileId==SketchProfiles::derive(circle).profile.profileId,"Legacy circle ID changed");
    auto f=SketchGeometry::newFeature(owner,"XY","Three bracket regions");
    const double vertices[][2]{{0,0},{490,0},{800,240},{735,315},{430,155},{0,155},{0,0}};
    for(int i=0;i<6;++i)f=SketchGeometry::addLine(f,{{"u1Mm",vertices[i][0]},{"v1Mm",vertices[i][1]},{"u2Mm",vertices[i+1][0]},{"v2Mm",vertices[i+1][1]}});
    f=accepted(SketchGeometry::addConstraint(f,{{"type","fixed"},{"pointIds",QJsonArray{entity(f,0)["startPointId"]}}}));
    auto pair=[&](const QString& type,int a,int b){f=accepted(SketchGeometry::addConstraint(f,{{"type",type},{"entityIds",QJsonArray{entity(f,a)["id"],entity(f,b)["id"]}}}));};
    pair("perpendicular",5,0);pair("perpendicular",1,2);pair("perpendicular",2,3);
    for(int index:{0,4})f=accepted(SketchGeometry::addConstraint(f,{{"type","horizontal"},{"entityIds",QJsonArray{entity(f,index)["id"]}}}));
    pair("equal",0,1);pair("equal",2,5);
    f=accepted(SketchGeometry::addDimension(f,{{"kind","horizontal"},{"entityId",entity(f,0)["id"]},{"valueMm",500}}));
    f=accepted(SketchGeometry::addDimension(f,{{"kind","angle"},{"entityIds",QJsonArray{entity(f,0)["id"],entity(f,1)["id"]}},{"raySigns",QJsonArray{1,1}},{"valueDeg",45}}));
    f=accepted(SketchGeometry::addDimension(f,{{"kind","linear"},{"entityId",entity(f,2)["id"]},{"valueMm",100}}));
    const QJsonValue capDimension=f["parameters"].toObject()["dimensions"].toArray().last().toObject()["id"];
    QMap<QString,QPointF> points;for(const auto& value:f["parameters"].toObject()["points"].toArray()){const auto p=value.toObject();points[p["id"].toString()]={p["uMm"].toDouble(),p["vMm"].toDouble()};}
    double polygonArea=0;for(int i=0;i<6;++i){const auto e=entity(f,i);const auto a=points[e["startPointId"].toString()],b=points[e["endPointId"].toString()];polygonArea+=a.x()*b.y()-a.y()*b.x();}polygonArea=std::abs(polygonArea)/2;
    double cornerAreaChange=0;
    for(const auto& pair:std::initializer_list<std::pair<int,int>>{{0,1},{3,4}}) {
        const auto first=entity(f,pair.first),second=entity(f,pair.second);
        const auto a=points[first["endPointId"].toString()]-points[first["startPointId"].toString()];
        const auto b=points[second["endPointId"].toString()]-points[second["startPointId"].toString()];
        const double turn=std::atan2(a.x()*b.y()-a.y()*b.x(),QPointF::dotProduct(a,b));
        cornerAreaChange-=std::copysign(10000*(std::tan(std::abs(turn)/2)-std::abs(turn)/2),turn);
    }
    const QJsonValue cap=entity(f,2)["id"],left=entity(f,5)["id"];
    f=accepted(SketchGeometry::setConstruction(f,{{"entityIds",QJsonArray{cap,left}},{"construction",true}}));
    for(const auto& pair:std::initializer_list<std::pair<int,int>>{{5,0},{2,1}})f=accepted(SketchGeometry::addPointerCircle(f,{{"centerUMm",0},{"centerVMm",0},{"diameterMm",100},
        {"centerSnapTarget",QJsonObject{{"kind","midpoint"},{"entityId",entity(f,pair.first)["id"]}}},
        {"tangentSnapTarget",QJsonObject{{"kind","tangent"},{"entityId",entity(f,pair.second)["id"]}}}}));
    f=accepted(SketchGeometry::filletCorners(f,{{"corners",QJsonArray{QJsonObject{{"entityIds",QJsonArray{entity(f,0)["id"],entity(f,1)["id"]}}},QJsonObject{{"entityIds",QJsonArray{entity(f,3)["id"],entity(f,4)["id"]}}}}},{"radiusMm",100}}));
    const auto regions=SketchProfiles::deriveRegions(f);inspect(regions,3);
    double area=0;int disks=0;for(const auto& region:regions.profiles){area+=region.summary["areaMm2"].toDouble();
        if(std::abs(region.summary["areaMm2"].toDouble()-2500*std::numbers::pi)<1e-5)++disks;
        check(!region.summary["sourceEntityIds"].toArray().contains(cap)&&!region.summary["sourceEntityIds"].toArray().contains(left),"Construction entered region provenance");}
    check(disks==2,"Expected two exact circular end regions");
    const double expected=polygonArea+2500*std::numbers::pi+cornerAreaChange;
    check(std::abs(area-expected)<1e-4,"Three-region area differs from analytic rounded outline");
    const QJsonValue radiusDimension=f["parameters"].toObject()["dimensions"].toArray().last().toObject()["id"];
    auto edited=accepted(SketchGeometry::updateDimension(f,{{"dimensionId",radiusDimension},{"valueMm",90}}));
    const auto resized=SketchProfiles::deriveRegions(edited);inspect(resized,3);check(ids(resized)==ids(regions),"Radius edit changed region identities");
    edited=accepted(SketchGeometry::updateDimension(edited,{{"dimensionId",capDimension},{"valueMm",120}}));
    const auto wider=SketchProfiles::deriveRegions(edited);inspect(wider,3);check(ids(wider)==ids(regions),"Width edit changed region identities");
    const auto evaluated=SketchGeometry::evaluateFeature(edited);check(evaluated.profiles.size()==3 && evaluated.summary["profiles"].toArray().size()==3,"Plural regions lost in evaluated sketch");
    const auto reopened=QJsonDocument::fromJson(QJsonDocument(edited).toJson()).object();check(ids(SketchProfiles::deriveRegions(reopened))==ids(wider),"Reopen changed region identities");
    // Coincident independently authored circles cannot provide unique edge provenance.
    auto overlap=SketchGeometry::addCircle(circle,{{"centerUMm",0},{"centerVMm",0},{"diameterMm",10}});
    const auto ambiguous=SketchProfiles::deriveRegions(overlap);check(!ambiguous.ok && ambiguous.diagnostics["code"]=="ambiguous_region_provenance","Overlapping sources silently acquired region identity");
    std::cout<<"PASS: actual rounded bracket three OCCT regions, analytic area, retained IDs, construction exclusion and legacy identity\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
