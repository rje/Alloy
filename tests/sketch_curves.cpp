// SPDX-License-Identifier: MIT
#include "SketchGeometry.h"
#include "SketchProfiles.h"
#include <BRepAdaptor_Curve.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QUuid>
#include <cmath>
#include <iostream>
#include <numbers>
#include <stdexcept>

namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void near(double value,double expected,const char* message){check(std::abs(value-expected)<1e-6,message);}
QJsonObject accepted(const SketchGeometry::SolveResult& result){check(result.ok,qPrintable(result.error));return result.feature;}
QJsonObject entity(const QJsonObject& f,int index=0){return f["parameters"].toObject()["entities"].toArray()[index].toObject();}
QJsonObject lastDimension(const QJsonObject& f){return f["parameters"].toObject()["dimensions"].toArray().last().toObject();}
double lineLength(const QJsonObject& f,int index){const auto e=entity(f,index);QJsonObject a,b;
    for(const auto& p:f["parameters"].toObject()["points"].toArray()){
        if(p.toObject()["id"]==e["startPointId"])a=p.toObject();if(p.toObject()["id"]==e["endPointId"])b=p.toObject();}
    return std::hypot(a["uMm"].toDouble()-b["uMm"].toDouble(),a["vMm"].toDouble()-b["vMm"].toDouble());}
void equalConstraints(const QString& owner){
    auto lines=SketchGeometry::newFeature(owner,"XY","Equal line lengths");
    lines=SketchGeometry::addLine(lines,{{"u1Mm",0},{"v1Mm",0},{"u2Mm",10},{"v2Mm",0}});
    lines=SketchGeometry::addLine(lines,{{"u1Mm",0},{"v1Mm",20},{"u2Mm",6},{"v2Mm",28}});
    lines=accepted(SketchGeometry::addConstraint(lines,{{"type","equal"},{"entityIds",QJsonArray{entity(lines)["id"],entity(lines,1)["id"]}}}));
    check(SketchGeometry::solveDraft(lines).diagnostics["dof"].toInt()==7,"Equal line relation must remove one DOF");
    const auto first=entity(lines);
    lines=accepted(SketchGeometry::addDimension(lines,{{"kind","linear"},{"pointIds",QJsonArray{first["startPointId"],first["endPointId"]}},{"valueMm",15}}));
    near(lineLength(lines,0),15,"Driving first equal line");near(lineLength(lines,1),15,"Equal second line did not follow");
    const auto second=entity(lines,1);
    const auto badLine=SketchGeometry::addDimension(lines,{{"kind","linear"},{"pointIds",QJsonArray{second["startPointId"],second["endPointId"]}},{"valueMm",12}});
    check(!badLine.ok && badLine.feature==lines,"Equal line contradiction lost previous draft");
    check(badLine.diagnostics.contains("solverStatus") && !badLine.error.isEmpty(),"Equal line contradiction omitted solver diagnostics");

    auto circles=SketchGeometry::newFeature(owner,"XY","Equal circle chain");
    for(int i=0;i<5;++i)circles=SketchGeometry::addCircle(circles,{{"centerUMm",i*20},{"centerVMm",0},{"diameterMm",8+i}});
    const auto originalEntities=circles["parameters"].toObject()["entities"].toArray();
    for(int i=1;i<5;++i)circles=accepted(SketchGeometry::addConstraint(circles,{{"type","equal"},{"entityIds",QJsonArray{entity(circles,i-1)["id"],entity(circles,i)["id"]}}}));
    check(SketchGeometry::solveDraft(circles).diagnostics["dof"].toInt()==11,"Equal circle chain must retain center freedom and one radius DOF");
    circles=accepted(SketchGeometry::addDimension(circles,{{"kind","diameter"},{"entityId",entity(circles)["id"]},{"valueMm",10}}));
    check(SketchGeometry::solveDraft(circles).diagnostics["dof"].toInt()==10,"One diameter must control all five equal radii");
    const auto relations=circles["parameters"].toObject()["constraints"];
    const auto changed=accepted(SketchGeometry::updateDimension(circles,{{"dimensionId",lastDimension(circles)["id"]},{"valueMm",12}}));
    for(int i=0;i<5;++i){near(entity(changed,i)["radiusMm"].toDouble(),6,"Equal circle chain did not follow diameter edit");
        check(entity(changed,i)["id"]==originalEntities[i].toObject()["id"],"Equal solve replaced circle identity");}
    check(changed["parameters"].toObject()["constraints"]==relations,"Equal solve changed retained relation IDs");
    const auto conflict=SketchGeometry::addDimension(changed,{{"kind","diameter"},{"entityId",entity(changed,4)["id"]},{"valueMm",10}});
    check(!conflict.ok && conflict.feature==changed && !conflict.diagnostics["conflictingConstraintIds"].toArray().isEmpty(),"Equal circle conflict failed preservation");
    auto malformed=SketchGeometry::addConstraint(changed,{{"type","equal"},{"entityIds",QJsonArray{entity(changed)["id"],entity(changed)["id"]}}});
    check(!malformed.ok && malformed.feature==changed,"Equal accepted repeated entity");
    malformed=SketchGeometry::addConstraint(changed,{{"type","equal"},{"entityIds",QJsonArray{entity(changed)["id"],QUuid::createUuid().toString(QUuid::WithoutBraces)}}});
    check(!malformed.ok && malformed.feature==changed,"Equal accepted stale entity");
    auto mixed=SketchGeometry::addCircle(lines,{{"centerUMm",30},{"centerVMm",30},{"diameterMm",10}});
    malformed=SketchGeometry::addConstraint(mixed,{{"type","equal"},{"entityIds",QJsonArray{entity(mixed)["id"],entity(mixed,2)["id"]}}});
    check(!malformed.ok && malformed.feature==mixed,"Equal accepted line/curve pair");
    auto arc=SketchGeometry::newFeature(owner,"XY","Equal analytic radii");
    arc=SketchGeometry::addCircle(arc,{{"centerUMm",0},{"centerVMm",0},{"diameterMm",10}});
    arc=SketchGeometry::addArc(arc,{{"centerUMm",20},{"centerVMm",0},{"radiusMm",3},{"startAngleDeg",0},{"sweepAngleDeg",90}});
    arc=accepted(SketchGeometry::addConstraint(arc,{{"type","equal"},{"entityIds",QJsonArray{entity(arc)["id"],entity(arc,1)["id"]}}}));
    arc=accepted(SketchGeometry::addDimension(arc,{{"kind","diameter"},{"entityId",entity(arc)["id"]},{"valueMm",12}}));
    near(entity(arc,1)["radiusMm"].toDouble(),6,"Arc Equal must compare radius rather than length or sweep");
}
void rejectedCreation(const QJsonObject& f,const QJsonObject& args,bool circle=false){bool refused=false;
    try{if(circle)SketchGeometry::addCircle(f,args);else SketchGeometry::addArc(f,args);}catch(const std::invalid_argument&){refused=true;}
    check(refused,"Degenerate curve input was accepted");}
void analytic(const QJsonObject& f,double length){const auto edge=SketchGeometry::analyticEdge(f["parameters"].toObject(),entity(f));
    check(BRepAdaptor_Curve(edge).GetType()==GeomAbs_Circle,"Curve edge lost analytic circle type");
    GProp_GProps properties;BRepGProp::LinearProperties(edge,properties);near(properties.Mass(),length,"OCCT analytic edge length");}
}

int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    const auto owner=QUuid::createUuid().toString(QUuid::WithoutBraces);
    equalConstraints(owner);
    const auto blank=SketchGeometry::newFeature(owner,"XY");
    auto circle=SketchGeometry::addCircle(blank,{{"centerUMm",0},{"centerVMm",0},{"diameterMm",10}});
    circle=accepted(SketchGeometry::solveDraft(circle));analytic(circle,10*std::numbers::pi);
    check(SketchGeometry::solveDraft(circle).diagnostics["dof"].toInt()==3,"Free circle must retain center/radius DOF");
    const auto circleId=entity(circle)["id"].toString();
    auto disk=SketchProfiles::derive(circle);check(disk.ok,qPrintable(disk.error));near(disk.profile.summary["areaMm2"].toDouble(),25*std::numbers::pi,"Circle disk exact area");
    circle=accepted(SketchGeometry::addDimension(circle,{{"kind","diameter"},{"entityId",circleId},{"valueMm",10},{"annotation",QJsonObject{{"uMm",8},{"vMm",8}}}}));
    const auto diameterId=lastDimension(circle)["id"].toString();
    circle=accepted(SketchGeometry::addDimension(circle,{{"kind","radius"},{"entityId",circleId},{"driving",false}}));
    check(SketchGeometry::solveDraft(circle).diagnostics["dof"].toInt()==2,"Radius dimension must not freeze free center");
    auto changed=accepted(SketchGeometry::updateDimension(circle,{{"dimensionId",diameterId},{"valueMm",12}}));
    near(entity(changed)["radiusMm"].toDouble(),6,"Diameter edit normalizes radius");analytic(changed,12*std::numbers::pi);
    auto resizedDisk=SketchProfiles::derive(changed);check(resizedDisk.ok,qPrintable(resizedDisk.error));
    near(resizedDisk.profile.summary["areaMm2"].toDouble(),36*std::numbers::pi,"Edited disk area");check(disk.profile.profileId==resizedDisk.profile.profileId,"Radius edits changed profile identity");
    const auto measured=SketchGeometry::solveDraft(changed).diagnostics["dimensionValues"].toArray();near(measured.last().toObject()["valueMm"].toDouble(),6,"Driven radius follows diameter edit");
    const auto annotation=accepted(SketchGeometry::updateAnnotation(changed,{{"dimensionId",diameterId},{"annotation",QJsonObject{{"uMm",10},{"vMm",9}}}}));
    check(annotation["parameters"].toObject()["points"]==changed["parameters"].toObject()["points"] && annotation["parameters"].toObject()["entities"]==changed["parameters"].toObject()["entities"],"Annotation changed analytic intent");
    auto conflict=SketchGeometry::addDimension(changed,{{"kind","radius"},{"entityId",circleId},{"valueMm",7}});
    check(!conflict.ok && conflict.feature==changed && !conflict.diagnostics["conflictingConstraintIds"].toArray().isEmpty(),"Conflicting circle dimensions did not preserve draft");
    check(!SketchGeometry::updateDimension(changed,{{"dimensionId",diameterId},{"valueDeg",12}}).ok,"Length accepted angular units");
    const auto reopened=QJsonDocument::fromJson(QJsonDocument(changed).toJson()).object();check(accepted(SketchGeometry::solveDraft(reopened))==changed,"JSON roundtrip changed analytic intent");

    auto arc=SketchGeometry::addArc(blank,{{"centerUMm",0},{"centerVMm",0},{"radiusMm",5},{"startAngleDeg",0},{"sweepAngleDeg",90}});
    arc=accepted(SketchGeometry::solveDraft(arc));analytic(arc,5*std::numbers::pi/2);
    check(SketchGeometry::solveDraft(arc).diagnostics["dof"].toInt()==5,"Free arc must retain five geometric DOF");
    check(!SketchProfiles::derive(arc).ok && SketchProfiles::derive(arc).diagnostics["code"]=="open_profile","Isolated arc was treated as a closed profile");
    const auto arcId=entity(arc)["id"].toString();
    arc=accepted(SketchGeometry::addDimension(arc,{{"kind","radius"},{"entityId",arcId},{"valueMm",5}}));
    const auto radiusId=lastDimension(arc)["id"].toString();
    arc=accepted(SketchGeometry::addDimension(arc,{{"kind","arcSweep"},{"entityId",arcId},{"valueDeg",90}}));
    const auto sweepId=lastDimension(arc)["id"].toString();
    arc=accepted(SketchGeometry::addDimension(arc,{{"kind","diameter"},{"entityId",arcId},{"driving",false}}));
    check(SketchGeometry::solveDraft(arc).diagnostics["dof"].toInt()==3,"Radius/sweep dimensions froze free center/rotation");
    auto larger=accepted(SketchGeometry::updateDimension(arc,{{"dimensionId",radiusId},{"valueMm",6}}));analytic(larger,3*std::numbers::pi);
    larger=accepted(SketchGeometry::updateDimension(larger,{{"dimensionId",sweepId},{"valueDeg",180}}));analytic(larger,6*std::numbers::pi);
    near(entity(larger)["sweepAngleDeg"].toDouble(),180,"Sweep edit");
    check(!SketchGeometry::updateDimension(larger,{{"dimensionId",sweepId},{"valueMm",45}}).ok,"Sweep accepted millimeters");
    conflict=SketchGeometry::addDimension(larger,{{"kind","arcSweep"},{"entityId",arcId},{"valueDeg",120}});
    check(!conflict.ok && conflict.feature==larger,"Conflicting arc sweep did not preserve draft");
    auto clockwise=SketchGeometry::addArc(blank,{{"centerUMm",0},{"centerVMm",0},{"radiusMm",5},{"startAngleDeg",0},{"sweepAngleDeg",-270}});
    clockwise=accepted(SketchGeometry::addDimension(clockwise,{{"kind","arcSweep"},{"entityId",entity(clockwise)["id"]},{"valueDeg",270}}));
    clockwise=accepted(SketchGeometry::updateDimension(clockwise,{{"dimensionId",lastDimension(clockwise)["id"]},{"valueDeg",180}}));
    near(entity(clockwise)["sweepAngleDeg"].toDouble(),-180,"Sweep edit lost clockwise direction");

    const double mid=5/std::sqrt(2.0);
    auto three=SketchGeometry::addArc(blank,{{"startUMm",5},{"startVMm",0},{"endUMm",0},{"endVMm",5},{"throughUMm",mid},{"throughVMm",mid}});
    near(entity(three)["radiusMm"].toDouble(),5,"Three-point radius");near(entity(three)["sweepAngleDeg"].toDouble(),90,"Three-point endpoint/end/on-arc branch");
    auto major=SketchGeometry::addArc(blank,{{"startUMm",5},{"startVMm",0},{"endUMm",0},{"endVMm",5},{"throughUMm",-5},{"throughVMm",0}});
    near(entity(major)["sweepAngleDeg"].toDouble(),-270,"Three-point opposite-side major branch");analytic(major,15*std::numbers::pi/2);
    for(const auto& plane : {"XZ","YZ"}){auto other=SketchGeometry::addArc(SketchGeometry::newFeature(owner,plane),{{"centerUMm",0},{"centerVMm",0},{"radiusMm",5},{"startAngleDeg",0},{"sweepAngleDeg",90}});analytic(other,5*std::numbers::pi/2);}

    auto sector=three;
    sector=SketchGeometry::addLine(sector,{{"u1Mm",0},{"v1Mm",5},{"u2Mm",0},{"v2Mm",0},{"startPointId",entity(three)["endPointId"]},{"endPointId",entity(three)["centerPointId"]}});
    sector=SketchGeometry::addLine(sector,{{"u1Mm",0},{"v1Mm",0},{"u2Mm",5},{"v2Mm",0},{"startPointId",entity(three)["centerPointId"]},{"endPointId",entity(three)["startPointId"]}});
    sector=accepted(SketchGeometry::solveDraft(sector));const auto sectorProfile=SketchProfiles::derive(sector);check(sectorProfile.ok,qPrintable(sectorProfile.error));
    near(sectorProfile.profile.summary["areaMm2"].toDouble(),25*std::numbers::pi/4,"Quarter sector analytic area");
    auto segment=SketchGeometry::addArc(blank,{{"centerUMm",0},{"centerVMm",0},{"radiusMm",5},{"startAngleDeg",0},{"sweepAngleDeg",180}});
    segment=SketchGeometry::addLine(segment,{{"u1Mm",-5},{"v1Mm",0},{"u2Mm",5},{"v2Mm",0},{"startPointId",entity(segment)["endPointId"]},{"endPointId",entity(segment)["startPointId"]}});
    const auto halfDisk=SketchProfiles::derive(accepted(SketchGeometry::solveDraft(segment)));check(halfDisk.ok,qPrintable(halfDisk.error));near(halfDisk.profile.summary["areaMm2"].toDouble(),25*std::numbers::pi/2,"Arc plus chord profile area");
    auto crossing=three;
    crossing=SketchGeometry::addLine(crossing,{{"u1Mm",0},{"v1Mm",5},{"u2Mm",10},{"v2Mm",0}});
    crossing=SketchGeometry::addLine(crossing,{{"u1Mm",10},{"v1Mm",0},{"u2Mm",5},{"v2Mm",0}});
    const auto crossingProfile=SketchProfiles::derive(accepted(SketchGeometry::solveDraft(crossing)));
    check(!crossingProfile.ok && crossingProfile.diagnostics["code"]=="self_intersecting_profile","Interior analytic arc/line crossing was accepted as a profile");
    auto twoCircles=SketchGeometry::addCircle(circle,{{"centerUMm",20},{"centerVMm",0},{"diameterMm",10}});
    check(!SketchProfiles::derive(twoCircles).ok && SketchProfiles::derive(twoCircles).diagnostics["code"]=="multiple_loops","Multiple circle regions silently accepted");
    rejectedCreation(blank,{{"centerUMm",0},{"centerVMm",0},{"diameterMm",0}},true);
    rejectedCreation(blank,{{"centerUMm",0},{"centerVMm",0},{"radiusMm",5},{"startAngleDeg",0},{"sweepAngleDeg",360}});
    rejectedCreation(blank,{{"centerUMm",0},{"centerVMm",0},{"radiusMm",5},{"startAngleDeg",0},{"sweepAngleDeg",0}});
    rejectedCreation(blank,{{"startUMm",0},{"startVMm",0},{"endUMm",1},{"endVMm",0},{"throughUMm",2},{"throughVMm",0}});
    std::cout<<"PASS: analytic circles/arcs, radius/diameter/sweep solve and edits, driven values, DOF, JSON intent, exact disk/sector/chord profiles and refusals\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
