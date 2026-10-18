// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "RoundedLoopOffset.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QUuid>
#include <cmath>
#include <iostream>
#include <numbers>
#include <stdexcept>

static void check(bool condition,const QString& message){if(!condition)throw std::runtime_error(message.toStdString());}
static QJsonObject accepted(const SketchGeometry::SolveResult& result){check(result.ok,result.error);return result.feature;}
static QJsonObject sourceSketch(const QString& owner){
    auto sketch=SketchGeometry::newFeature(owner,"XY");
    sketch=accepted(SketchGeometry::addPointerEntity(sketch,"rectangle",{{"u1Mm",0},{"v1Mm",0},{"u2Mm",60},{"v2Mm",-30},{"startSnapTarget",QJsonObject{{"kind","origin"}}}}));
    QStringList sides;for(const auto& v:sketch["parameters"].toObject()["entities"].toArray())sides<<v.toObject()["id"].toString();
    sketch=accepted(SketchGeometry::addDimension(sketch,{{"kind","horizontal"},{"entityId",sides[0]},{"valueMm",60}}));
    sketch=accepted(SketchGeometry::addDimension(sketch,{{"kind","vertical"},{"entityId",sides[1]},{"valueMm",30}}));
    for(int i=0;i<4;++i)sketch=accepted(SketchGeometry::filletCorners(sketch,{{"corners",QJsonArray{QJsonObject{{"entityIds",QJsonArray{sides[i],sides[(i+1)%4]}}}}},{"radiusMm",6}}));
    return sketch;
}
static EvaluatedSupportFace topFace(const CadDocument& document){
    for(const auto& face:document.bodies().front().supportFaces)if(face.role=="extrude.endCap")return face;
    throw std::runtime_error("Top cap unavailable");
}
static QJsonObject offsetSketch(const CadDocument& document,double distance){
    const auto cap=topFace(document);
    const QJsonObject support{{"producerFeatureId",cap.producerFeatureId},{"bodyId",cap.bodyId},{"faceId",cap.faceId}};
    auto sketch=SketchGeometry::newSupportedFeature(document.activeOwnerId(),support,"Rounded loop offset",{});
    auto p=sketch["parameters"].toObject();p["loopOffsets"]=QJsonArray{QJsonObject{{"id",CadDocument::newId()},
        {"outerLoopId",RoundedLoopOffset::outerLoopId(cap)},{"distanceMm",distance},{"chain",true}}};sketch["parameters"]=p;
    return sketch;
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument doc;auto source=sourceSketch(doc.activeOwnerId());
    check(doc.commit(SketchGeometry::replaceFeature(doc.authored(),source),"Rounded source").isEmpty(),"Source sketch commit");
    const auto outerArea=1800-36*(4-std::numbers::pi);
    check(std::abs(doc.sketches().front().profiles.front().summary["areaMm2"].toDouble()-outerArea)<1e-5,"Source area");
    check(doc.commit(CadDocument::extrusionCandidate(doc.authored(),{{"sketchId",source["id"]},
        {"profileId",doc.sketches().front().profiles.front().profileId},{"distanceMm",2}}),"Base 2 mm").isEmpty(),"Base extrusion");
    const auto cap=topFace(doc);check(std::abs(cap.frame["origin"].toArray()[2].toDouble()-2)<1e-8,"Actual top face z2");
    // Both generated-only owner schemas resolve an evaluated profile, never an authored edge.
    for(const bool inset:{false,true}) {
        auto owned=offsetSketch(doc,-1.5);auto p=owned.value("parameters").toObject();
        if(inset){auto rows=p.value("loopOffsets").toArray();auto row=rows.first().toObject();row["kind"]="planarInsetV1";row["outerLoopId"]=RoundedLoopOffset::insetLoopId(cap);rows[0]=row;p["loopOffsets"]=rows;owned["parameters"]=p;}
        const auto evaluated=RoundedLoopOffset::evaluate(owned,cap,cap.frame);const QJsonArray target{evaluated.profiles.front().profileId};
        const auto changed=RoundedLoopOffset::deleteProfile(owned,cap,target);
        auto expected=owned;auto ep=p;ep.remove("loopOffsets");expected["parameters"]=ep;check(changed==expected,"Delete touched support/sketch state");
        check(SketchGeometry::solveDraft(changed).ok,"Empty supported sketch refused");
        for(const auto& bad:{QJsonArray{},QJsonArray{target.first(),target.first()},QJsonArray{CadDocument::newId()}}){
            bool refused=false;try{RoundedLoopOffset::deleteProfile(owned,cap,bad);}catch(const GeometryKernel::EvaluationError&){refused=true;}check(refused,"Invalid profile accepted");
        }
        auto stale=cap;stale.faceId=CadDocument::newId();bool refused=false;try{RoundedLoopOffset::deleteProfile(owned,stale,target);}catch(const GeometryKernel::EvaluationError&){refused=true;}check(refused,"Changed support accepted");
        stale=cap;stale.boundaryEdges.front().edgeId=CadDocument::newId();stale.boundaryEdges.front().provenance["sourceEntityId"]=CadDocument::newId();refused=false;
        try{RoundedLoopOffset::deleteProfile(owned,stale,target);}catch(const GeometryKernel::EvaluationError&){refused=true;}check(refused,"Changed lineage accepted");
        auto malformed=owned;auto mp=p;auto rows=mp.value("loopOffsets").toArray();auto row=rows.first().toObject();row["unexpected"]=true;rows[0]=row;mp["loopOffsets"]=rows;malformed["parameters"]=mp;refused=false;
        try{RoundedLoopOffset::deleteProfile(malformed,cap,target);}catch(const GeometryKernel::EvaluationError&){refused=true;}check(refused,"Malformed owner accepted");
    }
    auto offset=offsetSketch(doc,-1.5);const auto authoredBefore=doc.authored();
    auto evaluated=RoundedLoopOffset::evaluate(offset,cap);
    const auto innerArea=57*27-4.5*4.5*(4-std::numbers::pi),rimArea=outerArea-innerArea;
    check(evaluated.profiles.size()==1 && std::abs(evaluated.profiles[0].summary["areaMm2"].toDouble()-rimArea)<1e-5,"Exact R4.5 annulus");
    const auto inside=evaluated.profiles[0].summary["innerBoundsMm"].toObject()["size"].toArray();
    check(std::abs(inside[0].toDouble()-57)<1e-5 && std::abs(inside[1].toDouble()-27)<1e-5,"Inner loop must be 57 x 27");
    const auto radii=evaluated.profiles[0].summary["innerArcRadiiMm"].toArray();
    check(radii.size()==4,"Inner loop needs four exact arcs");
    for(const auto& radius:radii)check(std::abs(radius.toDouble()-4.5)<1e-6,"Inner arc radius is not 4.5");
    auto outward=offset;auto outwardParameters=outward["parameters"].toObject();auto outwardObjects=outwardParameters["loopOffsets"].toArray();
    auto outwardObject=outwardObjects[0].toObject();outwardObject["distanceMm"]=1.5;outwardObjects[0]=outwardObject;
    outwardParameters["loopOffsets"]=outwardObjects;outward["parameters"]=outwardParameters;
    check(RoundedLoopOffset::evaluate(outward,cap).profiles[0].summary["areaMm2"].toDouble()>0,"Reverse-wire signed side");
    check(doc.authored()==authoredBefore,"Preview changed authored data");
    check(doc.commit(SketchGeometry::replaceFeature(doc.authored(),offset),"Retained offset").isEmpty(),"Offset commit");
    const auto profileId=doc.sketches().back().profiles.front().profileId,bodyId=doc.bodies().front().bodyId;
    check(doc.commit(CadDocument::extrusionCandidate(doc.authored(),{{"sketchId",offset["id"]},{"profileId",profileId},
        {"distanceMm",1.5},{"operation","join"},{"targetBodyIds",QJsonArray{bodyId}}}),"Rim Join").isEmpty(),"Annular Join");
    check(doc.bodies().size()==1 && std::abs(doc.bodies().front().summary["volumeMm3"].toDouble()-(outerArea*2+rimArea*1.5))<1e-4,"Rim volume");
    const auto final=doc.authored();QTemporaryDir temp;check(doc.save(temp.filePath("rounded-loop.alloy")).isEmpty(),"Save");
    CadDocument reopened;check(reopened.open(temp.filePath("rounded-loop.alloy")).isEmpty() && reopened.authored()==final,"Exact reopen");
    check(reopened.sketches().back().profiles.front().profileId==profileId,"Stable annulus ID");
    auto edited=offset;auto p=edited["parameters"].toObject();auto values=p["loopOffsets"].toArray();auto item=values[0].toObject();item["distanceMm"]=-1.25;values[0]=item;p["loopOffsets"]=values;edited["parameters"]=p;
    check(doc.commit(SketchGeometry::replaceFeature(doc.authored(),edited),"Offset distance edit").isEmpty(),"Signed distance edit");
    check(doc.sketches().back().profiles.front().profileId==profileId,"Profile identity changed with distance");
    doc.history().undo();check(doc.authored()==final,"Undo not exact");
    QString radiusId;for(const auto& value:source["parameters"].toObject()["dimensions"].toArray())
        if(value.toObject()["kind"]=="radius")radiusId=value.toObject()["id"].toString();
    check(!radiusId.isEmpty(),"Source radius dimension");
    const auto changed=SketchGeometry::updateDimension(source,{{"dimensionId",radiusId},{"valueMm",5.5}});
    check(changed.ok,"Supported source radius edit");
    check(doc.commit(SketchGeometry::replaceFeature(doc.authored(),changed.feature),"Edit source radius").isEmpty(),"Source radius change must recompute rim");
    check(doc.sketches().back().profiles.front().profileId==profileId,"Profile role identity lost after source edit");
    check(std::abs(doc.sketches().back().profiles.front().summary["areaMm2"].toDouble()-rimArea)>1e-3,"Offset failed to respond to source radius edit");
    doc.history().undo();check(doc.authored()==final,"Source edit undo not exact");
    auto collapsed=edited;p=collapsed["parameters"].toObject();values=p["loopOffsets"].toArray();item=values[0].toObject();item["distanceMm"]=-100;values[0]=item;p["loopOffsets"]=values;collapsed["parameters"]=p;
    check(!doc.commit(SketchGeometry::replaceFeature(doc.authored(),collapsed),"Collapsed offset").isEmpty(),"Collapsed offset accepted");
    check(doc.authored()==final,"Rejected collapse mutated authored state");
    std::cout<<"PASS: exact R6/R4.5 annulus, z2 support, 1.5 Join, edit, undo and reopen\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
