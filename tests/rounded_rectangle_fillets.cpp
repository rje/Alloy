// SPDX-License-Identifier: MIT
#include "SketchGeometry.h"
#include "CadDocument.h"
#include <QCoreApplication>
#include <QJsonArray>
#include <QTemporaryDir>
#include <QUuid>
#include <cmath>
#include <iostream>
#include <limits>
#include <numbers>
#include <stdexcept>

static void check(bool ok,const QString& message){if(!ok)throw std::runtime_error(message.toStdString());}
static QJsonObject accepted(const SketchGeometry::SolveResult& result){check(result.ok,result.error);return result.feature;}
static QJsonObject dimension(const QJsonObject& sketch,const QString& id){
    for(const auto& value:sketch["parameters"].toObject()["dimensions"].toArray())if(value.toObject()["id"]==id)return value.toObject();return {};
}
static QJsonObject point(const QJsonObject& sketch,const QString& id){
    for(const auto& value:sketch["parameters"].toObject()["points"].toArray())if(value.toObject()["id"]==id)return value.toObject();return {};
}
static double span(const QJsonObject& sketch,const QJsonObject& d){
    const auto ids=d["pointIds"].toArray();const auto a=point(sketch,ids[0].toString()),b=point(sketch,ids[1].toString());
    return std::abs(d["kind"]=="horizontal" ? b["uMm"].toDouble()-a["uMm"].toDouble() : b["vMm"].toDouble()-a["vMm"].toDouble());
}
static QPair<double,double> boundarySize(const QJsonObject& sketch){
    double minU=std::numeric_limits<double>::infinity(),maxU=-minU,minV=minU,maxV=-minU;
    for(const auto& value:sketch["parameters"].toObject()["entities"].toArray()){
        const auto e=value.toObject();
        for(const auto& field:{"startPointId","endPointId"}){
            const auto p=point(sketch,e[field].toString());
            minU=std::min(minU,p["uMm"].toDouble());maxU=std::max(maxU,p["uMm"].toDouble());
            minV=std::min(minV,p["vMm"].toDouble());maxV=std::max(maxV,p["vMm"].toDouble());
        }
    }
    return {maxU-minU,maxV-minV};
}

int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument document;
    auto sketch=SketchGeometry::newFeature(document.activeOwnerId(),"XY");
    sketch=accepted(SketchGeometry::addPointerEntity(sketch,"rectangle",{{"u1Mm",0},{"v1Mm",0},{"u2Mm",60},{"v2Mm",-30},{"startSnapTarget",QJsonObject{{"kind","origin"}}}}));
    const auto source=sketch["parameters"].toObject()["entities"].toArray();
    QStringList lines;for(const auto& value:source)lines<<value.toObject()["id"].toString();
    sketch=accepted(SketchGeometry::addDimension(sketch,{{"kind","horizontal"},{"entityId",lines[0]},{"valueMm",60}}));
    const auto widthId=sketch["parameters"].toObject()["dimensions"].toArray().last().toObject()["id"].toString();
    sketch=accepted(SketchGeometry::addDimension(sketch,{{"kind","vertical"},{"entityId",lines[1]},{"valueMm",30}}));
    const auto heightId=sketch["parameters"].toObject()["dimensions"].toArray().last().toObject()["id"].toString();
    for(int i=0;i<4;++i){
        const auto before=sketch;
        const auto corners=QJsonArray{QJsonObject{{"entityIds",QJsonArray{lines[i],lines[(i+1)%4]}}}};
        const auto result=SketchGeometry::filletCorners(sketch,{{"corners",corners},{"radiusMm",6}});
        check(result.ok,QString("corner %1: %2").arg(i).arg(result.error));
        if(i==3)check(!result.diagnostics["releasedOriginCoincidence"].toBool(),"Origin anchor was released");
        sketch=result.feature;
        const auto width=dimension(sketch,widthId),height=dimension(sketch,heightId);
        check(!width.isEmpty() && !height.isEmpty(),"Overall dimension ID lost");
        for(const auto& d:{width,height})check(d["driving"].toBool() && d.contains("valueMm"),"Overall span lost driving intent");
        check(std::abs(span(sketch,width)-60)<1e-5 && std::abs(span(sketch,height)-30)<1e-5,"Overall spans changed to shortened side length");
        check(sketch!=before,"Fillet did not change source sketch");
    }
    int linesLeft=0,arcs=0;QString radiusId;
    for(const auto& value:sketch["parameters"].toObject()["entities"].toArray()){
        const auto entity=value.toObject();linesLeft+=entity["type"]=="line";arcs+=entity["type"]=="arc";
    }
    for(const auto& value:sketch["parameters"].toObject()["dimensions"].toArray())if(value.toObject()["kind"]=="radius")radiusId=value.toObject()["id"].toString();
    check(linesLeft==4 && arcs==4,"Rounded rectangle needs four surviving lines and four real arcs");
    check(!radiusId.isEmpty(),"Radius dimension missing");
    const auto evaluated=SketchGeometry::evaluateFeature(sketch);
    check(evaluated.profiles.size()==1,"Rounded rectangle must have one closed profile");
    check(std::abs(evaluated.profiles[0].summary["areaMm2"].toDouble()-(1800-36*(4-std::numbers::pi)))<1e-5,"Rounded rectangle area is not analytic R6 area");
    const auto edited=SketchGeometry::updateDimension(sketch,{{"dimensionId",radiusId},{"valueMm",5.5}});
    check(edited.ok,QString("Radius edit: %1").arg(edited.error));
    check(dimension(edited.feature,widthId)["driving"].toBool() && dimension(edited.feature,heightId)["driving"].toBool(),"Radius edit lost driving dimensions");
    const auto editedSize=boundarySize(edited.feature);
    check(std::abs(span(edited.feature,dimension(edited.feature,widthId))-editedSize.first)<1e-5
        && std::abs(span(edited.feature,dimension(edited.feature,heightId))-editedSize.second)<1e-5,
        "Edited dimensions no longer measure overall boundary bounds");
    check(std::abs(editedSize.first-60)<1e-5 && std::abs(editedSize.second-30)<1e-5,"Radius edit changed outer size");
    const auto cornerIds=edited.feature["parameters"].toObject()["roundedRectangleIntent"].toObject()["cornerArcIds"].toArray();
    check(cornerIds.size()==4,"Rounded boundary lost recorded corner arc identities");
    auto withCircle=SketchGeometry::addCircle(edited.feature,{{"centerUMm",90},{"centerVMm",-15},{"diameterMm",8}});
    auto withArc=SketchGeometry::addArc(withCircle,{{"centerUMm",100},{"centerVMm",-15},{"radiusMm",3},{"startAngleDeg",0},{"sweepAngleDeg",90}});
    const auto mixed=SketchGeometry::solveDraft(withArc);
    check(mixed.ok,"Unrelated circle/arc was refused in a rounded sketch: "+mixed.error);
    check(mixed.feature["parameters"].toObject()["roundedRectangleIntent"].toObject()["cornerArcIds"].toArray()==cornerIds,
        "Unrelated curves changed rounded-boundary identities");
    auto oldCandidate=withCircle;auto oldParameters=oldCandidate["parameters"].toObject();auto oldIntent=oldParameters["roundedRectangleIntent"].toObject();
    oldIntent.remove("cornerArcIds");oldParameters["roundedRectangleIntent"]=oldIntent;oldCandidate["parameters"]=oldParameters;
    check(SketchGeometry::solveDraft(oldCandidate).ok,"Earlier five-field rounded intent stopped loading with unrelated geometry");
    int intentLines=0;
    for(const auto& value:edited.feature["parameters"].toObject()["constraints"].toArray()){
        const auto c=value.toObject();if(c["type"]=="supportLine"){
            ++intentLines;check(c["pointIds"].toArray()==QJsonArray{sketch["parameters"].toObject()["originPointId"]},"Supporting line lost origin point");
        }
    }
    check(intentLines==2,"Removed origin corner needs two explicit supporting lines");
    auto resized=accepted(SketchGeometry::updateDimension(edited.feature,{{"dimensionId",widthId},{"valueMm",70}}));
    resized=accepted(SketchGeometry::updateDimension(resized,{{"dimensionId",heightId},{"valueMm",35}}));
    const auto resizedBounds=boundarySize(resized);
    check(std::abs(resizedBounds.first-70)<1e-5 && std::abs(resizedBounds.second-35)<1e-5,"Driving width/height edit changed the wrong supports");
    const double expectedArea=70*35-(1-std::numbers::pi/4)*(3*36+30.25);
    const auto newProfile=SketchGeometry::evaluateFeature(resized).profiles.front();
    check(std::abs(newProfile.summary["areaMm2"].toDouble()-expectedArea)<1e-4,"Mixed radius/size area changed");
    const auto impossible=SketchGeometry::updateDimension(resized,{{"dimensionId",radiusId},{"valueMm",30}});
    check(!impossible.ok && impossible.feature==resized,"Oversized fillet edit was not atomic");
    check(document.commit(SketchGeometry::replaceFeature(document.authored(),sketch),"Rounded source").isEmpty(),"Commit rounded source");
    const auto profileId=document.sketches().front().profiles.front().profileId;
    check(document.commit(CadDocument::extrusionCandidate(document.authored(),{{"sketchId",sketch["id"]},{"profileId",profileId},{"distanceMm",2}}),"Floor").isEmpty(),"Extrude rounded source");
    const auto bodyId=document.bodies().front().bodyId;
    check(document.commit(SketchGeometry::replaceFeature(document.authored(),resized),"Resize rounded source").isEmpty(),"Resize downstream extrusion");
    check(document.bodies().front().bodyId==bodyId && std::abs(document.bodies().front().summary["volumeMm3"].toDouble()-2*expectedArea)<1e-4,"Downstream volume or body identity changed");
    const auto resizedAuthored=document.authored();document.history().undo();document.history().redo();
    check(document.authored()==resizedAuthored,"Undo/redo changed rounded source");
    QTemporaryDir temp;check(document.save(temp.filePath("rounded.alloy")).isEmpty(),"Save rounded model");
    CadDocument reopened;check(reopened.open(temp.filePath("rounded.alloy")).isEmpty(),"Reopen rounded model");
    check(reopened.authored()==resizedAuthored && reopened.bodies().front().bodyId==bodyId,"Reopen changed history or body identity");
    CadDocument legacy;check(legacy.open(QStringLiteral(ALLOY_TEST_SOURCE_DIR "/tests/fixtures/rounded-rectangle-source.alloy")).isEmpty(),"Load legacy driven source");
    const auto old=legacy.authored()["features"].toArray().first().toObject()["parameters"].toObject();
    check(!old.contains("roundedRectangleIntent"),"Legacy source was migrated");
    for(const auto& value:old["dimensions"].toArray()){
        const auto d=value.toObject();if(d["kind"]=="horizontal" || d["kind"]=="vertical")
            check(!d["driving"].toBool(),"Legacy measured span became driving");
    }
    std::cout<<"PASS: four sequential rectangle fillets retain driving 60/30 size and origin\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
