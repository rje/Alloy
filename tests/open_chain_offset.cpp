// SPDX-License-Identifier: MIT
#include "SketchGeometry.h"
#include "SketchOpenChainOffset.h"
#include "CadDocument.h"
#include <QCoreApplication>
#include <QJsonArray>
#include <cmath>
#include <iostream>

int main(int argc,char** argv){
    QCoreApplication app(argc,argv);
    auto check=[](bool ok,const char* message){if(!ok){std::cerr<<message<<'\n';std::exit(1);}};
    auto sketch=SketchGeometry::newFeature("13fa543c-e237-4e2e-947c-320071321821","XY","Lid clearance");
    const double xy[][4]={{-12,10,-8,10},{-8,10,-6,0},{-6,0,6,0},{6,0,8,10},{8,10,12,10},{-8,10,8,10}};
    for(const auto& e:xy)sketch=SketchGeometry::addLine(sketch,{{"u1Mm",e[0]},{"v1Mm",e[1]},
        {"u2Mm",e[2]},{"v2Mm",e[3]}});
    QJsonArray source;const auto lines=sketch["parameters"].toObject()["entities"].toArray();
    for(int i=0;i<5;++i)source.append(lines[i].toObject()["id"]);
    try{sketch=SketchOpenChainOffset::add(sketch,sketch["parameters"].toObject(),
        {{"sourceEntityIds",source},{"topEntityId",lines[5].toObject()["id"]},{"signedDistanceMm",-0.25}});}
    catch(const std::exception& e){std::cerr<<"Offset add: "<<e.what()<<'\n';return 1;}
    auto solved=SketchGeometry::solveDraft(sketch);if(!solved.ok)std::cerr<<solved.error.toStdString()<<'\n';check(solved.ok,"Offset solve failed");sketch=solved.feature;
    const auto p=sketch["parameters"].toObject(),group=p["openChainOffsets"].toArray()[0].toObject();
    const auto hits=group["topIntersectionPointIds"].toArray();QJsonObject left,right;
    for(const auto& v:p["points"].toArray()){
        const auto point=v.toObject();if(point["id"]==hits[0])left=point;if(point["id"]==hits[1])right=point;}
    check(!left.isEmpty()&&!right.isEmpty(),"Derived top intersections missing");
    check(std::abs(left["vMm"].toDouble()-10)<1e-7&&std::abs(right["vMm"].toDouble()-10)<1e-7,
        "Closure intersections lost original top height");
    sketch=SketchGeometry::addLine(sketch,{{"u1Mm",left["uMm"]},{"v1Mm",left["vMm"]},
        {"u2Mm",right["uMm"]},{"v2Mm",right["vMm"]},
        {"startPointId",hits[0]},{"endPointId",hits[1]}});
    solved=SketchGeometry::solveDraft(sketch);if(!solved.ok)std::cerr<<solved.error.toStdString()<<'\n';
    check(solved.ok,"Explicit Line closure solve failed");
    const auto evaluated=SketchGeometry::evaluateFeature(solved.feature);
    check(!evaluated.profiles.empty(),"Offset plus explicit closure produced no selectable profile");
    // A face sketch stores only stable Project sources. The generated Offset
    // and its ordinary Line connector must re-evaluate from current lines.
    const QJsonObject support{{"producerFeatureId","d24b5bb9-9edf-41bd-93e2-2cb6746b4583"},
        {"bodyId","037f795a-7862-44c7-a386-40d592a2d8b5"},{"faceId","ba20718f-9abd-4bd7-a54d-60bd70e9bd33"}};
    auto projected=SketchGeometry::newSupportedFeature("13fa543c-e237-4e2e-947c-320071321821",support,"Projected lid");
    auto pp=projected["parameters"].toObject();QJsonArray projections,resolved;QJsonArray projectedSource;
    for(int i=0;i<5;++i){
        const auto id=QString("d091ea1d-8d57-46c4-84bc-%1").arg(i+1,12,10,QLatin1Char('0'));
        const auto edge=QString("a45f10c7-a486-4f85-8c4a-%1").arg(i+1,12,10,QLatin1Char('0'));
        const QJsonObject source{{"bodyId",support["bodyId"]},{"edgeId",edge}};
        projections.append(QJsonObject{{"id",id},{"source",source},{"profile",true}});
        resolved.append(QJsonObject{{"id",id},{"source",source},{"u1Mm",xy[i][0]},{"v1Mm",xy[i][1]},
            {"u2Mm",xy[i][2]},{"v2Mm",xy[i][3]}});
        projectedSource.append(id);
    }
    pp["projections"]=projections;projected["parameters"]=pp;
    QJsonObject context{{"lines",resolved},{"points",QJsonArray{}}};
    auto expanded=SketchGeometry::expandedProjectionFeature(projected,context)["parameters"].toObject();
    projected=SketchOpenChainOffset::add(projected,expanded,{{"sourceEntityIds",projectedSource},
        {"signedDistanceMm",-0.25}});
    auto projectionSolve=SketchGeometry::solveDraft(projected,context);
    if(!projectionSolve.ok)std::cerr<<projectionSolve.error.toStdString()<<'\n';
    check(projectionSolve.ok,"Projected offset solve failed");projected=projectionSolve.feature;
    const auto projectedGroup=projected["parameters"].toObject()["openChainOffsets"].toArray()[0].toObject();
    const auto projectedHits=projectedGroup["topIntersectionPointIds"].toArray();
    QJsonObject firstHit,secondHit;for(const auto& v:projected["parameters"].toObject()["points"].toArray()){
        const auto point=v.toObject();if(point["id"]==projectedHits[0])firstHit=point;
        if(point["id"]==projectedHits[1])secondHit=point;}
    projected=SketchGeometry::addLine(projected,{{"u1Mm",firstHit["uMm"]},{"v1Mm",firstHit["vMm"]},
        {"u2Mm",secondHit["uMm"]},{"v2Mm",secondHit["vMm"]},
        {"startPointId",projectedHits[0]},{"endPointId",projectedHits[1]}});
    projectionSolve=SketchGeometry::solveDraft(projected,context);
    if(!projectionSolve.ok)std::cerr<<projectionSolve.error.toStdString()<<'\n';
    check(projectionSolve.ok,"Projected closure solve failed");projected=projectionSolve.feature;
    const auto frame=SketchGeometry::frame("XY");
    check(!SketchGeometry::evaluateFeature(projected,frame,context).profiles.empty(),
        "Projected offset closure produced no profile");
    auto taller=resolved;
    for(int i=0;i<taller.size();++i){auto line=taller[i].toObject();
        if(line["v1Mm"].toDouble()==10)line["v1Mm"]=12;
        if(line["v2Mm"].toDouble()==10)line["v2Mm"]=12;
        taller[i]=line;}
    context["lines"]=taller;
    projectionSolve=SketchGeometry::solveDraft(projected,context);
    if(!projectionSolve.ok)std::cerr<<projectionSolve.error.toStdString()<<'\n';
    check(projectionSolve.ok,"Projected source height edit broke offset");
    for(const auto& v:projectionSolve.feature["parameters"].toObject()["points"].toArray())
        if(projectedHits.contains(v.toObject()["id"]))check(std::abs(v.toObject()["vMm"].toDouble()-12)<1e-7,
            "Closure did not follow original projected top height");
    const QString clearanceId="7223d46f-ac8c-43c1-90e4-79930d6575ef";
    auto boundSketch=projectionSolve.feature,boundParameters=boundSketch["parameters"].toObject();
    auto boundGroups=boundParameters["openChainOffsets"].toArray();auto boundGroup=boundGroups[0].toObject();
    boundGroup["distanceParameterId"]=clearanceId;boundGroup["sideSign"]=-1;boundGroups[0]=boundGroup;
    boundParameters["openChainOffsets"]=boundGroups;boundSketch["parameters"]=boundParameters;
    QJsonObject authored{{"features",QJsonArray{boundSketch}},
        {"userParameters",QJsonArray{QJsonObject{{"id",clearanceId},{"name","Clearance"},
            {"units","mm"},{"valueMm",0.4}}}}};
    auto resolvedBinding=CadDocument::resolveLengthBindings(authored);
    check(std::abs(resolvedBinding["features"].toArray()[0].toObject()["parameters"].toObject()
        ["openChainOffsets"].toArray()[0].toObject()["signedDistanceMm"].toDouble()+0.4)<1e-9,
        "Stable-ID Clearance binding did not resolve with retained inward side");
    auto rows=authored["userParameters"].toArray();auto renamed=rows[0].toObject();renamed["name"]="ClearanceRenamed";rows[0]=renamed;
    authored["userParameters"]=rows;resolvedBinding=CadDocument::resolveLengthBindings(authored);
    check(std::abs(resolvedBinding["features"].toArray()[0].toObject()["parameters"].toObject()
        ["openChainOffsets"].toArray()[0].toObject()["signedDistanceMm"].toDouble()+0.4)<1e-9,
        "Parameter rename changed retained Offset binding");
    std::cout<<"open chain offset core pass\n";
}
