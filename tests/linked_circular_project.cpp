// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <BRepAdaptor_Curve.hxx>
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <numbers>
#include <stdexcept>

namespace {
void need(bool yes,const char* why){if(!yes)throw std::runtime_error(why);}
void commit(CadDocument& doc,const QJsonObject& candidate,const char* why){
    const auto error=doc.commit(candidate,why);if(!error.isEmpty())throw std::runtime_error((QString(why)+": "+error).toStdString());
}
void append(CadDocument& doc,const QJsonObject& feature){auto candidate=doc.authored();auto features=candidate["features"].toArray();features.append(feature);candidate["features"]=features;commit(doc,candidate,"append");}
QJsonObject ref(const EvaluatedSupportFace& face){return {{"producerFeatureId",face.producerFeatureId},{"bodyId",face.bodyId},{"faceId",face.faceId}};}
gp_Pnt world(const QJsonObject& frame,double u,double v){const auto o=frame["origin"].toArray(),a=frame["uAxis"].toArray(),b=frame["vAxis"].toArray();
    return {o[0].toDouble()+u*a[0].toDouble()+v*b[0].toDouble(),o[1].toDouble()+u*a[1].toDouble()+v*b[1].toDouble(),o[2].toDouble()+u*a[2].toDouble()+v*b[2].toDouble()};}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try {
    const auto fixture=qEnvironmentVariable("ALLOY_ADAPTED_BOARD_FIXTURE");if(fixture.isEmpty()){std::cout<<"SKIP: set ALLOY_ADAPTED_BOARD_FIXTURE\n";return 77;}
    CadDocument doc;auto inserted=CadDocument::localReferenceCandidate(doc.authored(),{{"path",fixture},{"name","Board Reference"}});
    commit(doc,inserted,"insert board");const auto import=doc.authored()["features"].toArray()[0].toObject();
    const auto placement=QJsonObject{{"axis","Z"},{"angleDeg",90},{"translationMm",QJsonArray{28,-42.5,6}}};
    commit(doc,CadDocument::localReferencePlacementCandidate(doc.authored(),import["id"].toString(),placement),"capture board");
    std::vector<std::pair<QString,QString>> sources;QString pcbId,occurrence;
    for(const auto& body:doc.bodies())if(body.name.contains("PCB-001")) {
        pcbId=body.bodyId;occurrence=body.occurrenceId;
        for(const auto& face:body.supportFaces)for(const auto& edge:face.boundaryEdges) {
            const BRepAdaptor_Curve curve(edge.edge);
            if(curve.GetType()==GeomAbs_Circle && std::abs(curve.Circle().Radius()-1.35)<1e-5
                && std::abs(curve.Circle().Location().Z()-7.6)<1e-5) {
                bool duplicate=false;for(const auto& source:sources)duplicate|=source.second==edge.edgeId;
                if(!duplicate)sources.emplace_back(body.bodyId,edge.edgeId);
            }
        }
    }
    need(!pcbId.isEmpty() && !occurrence.isEmpty() && sources.size()==4,"four occurrence-qualified true PCB hole edges");
    auto components=CadDocument::componentCandidate(doc.authored(),"Plastic Case");commit(doc,components,"case component");
    const auto owner=components["components"].toArray().last().toObject()["id"].toString();
    const auto box=QJsonObject{{"id",CadDocument::newId()},{"bodyId",CadDocument::newId()},{"ownerId",owner},
        {"type","box"},{"name","Case Floor"},{"visible",true},
        {"parameters",QJsonObject{{"widthMm",64},{"depthMm",93},{"heightMm",3.5},{"xMm",-32},{"yMm",-46.5},{"zMm",0}}}};
    append(doc,box);const auto& floor=doc.bodies().back();const EvaluatedSupportFace* top=nullptr;
    for(const auto& face:floor.supportFaces)if(face.role=="box.z1")top=&face;
    need(top,"case floor top support");
    auto sketch=SketchGeometry::newSupportedFeature(owner,ref(*top),"Mounting Circles",GeometryKernel::supportEdgeDescriptors(*top));
    for(const auto& [bodyId,edgeId]:sources)sketch=SketchGeometry::linkedProjectionCandidate(doc.evaluation(),sketch,{{"bodyId",bodyId},{"edgeId",edgeId}});
    need(sketch["parameters"].toObject()["circleProjections"].toArray().size()==4,"four retained circle refs");
    auto context=GeometryKernel::projectionContext(doc.evaluation(),sketch);
    need(context["circles"].toArray().size()==4,"four exact projected circles");
    std::vector<std::pair<double,double>> expected{{24.5,-39},{24.5,19},{-24.5,-39},{-24.5,19}};
    for(const auto& value:context["circles"].toArray()) {
        const auto circle=value.toObject();need(std::abs(circle["radiusMm"].toDouble()-1.35)<1e-5,"projected radius");
        const auto center=world(top->frame,circle["uMm"].toDouble(),circle["vMm"].toDouble());
        bool matched=false;for(const auto& [x,y]:expected)matched|=std::hypot(center.X()-x,center.Y()-y)<1e-5;
        need(matched,"captured placement drives true projected center");
    }
    auto solved=SketchGeometry::solveDraft(sketch,context);need(solved.ok,"projected circles solve");sketch=solved.feature;
    for(const auto& value:sketch["parameters"].toObject()["circleProjections"].toArray()) {
        solved=SketchGeometry::addCircleOffset(sketch,{{"sourceEntityId",value.toObject()["id"]},{"distanceMm",-0.3}},context);
        if(!solved.ok)std::cerr<<solved.error.toStdString()<<'\n';need(solved.ok,"retained inward circle offset");sketch=solved.feature;
    }
    append(doc,sketch);need(doc.sketches().back().summary["projectedCircles"].toArray().size()==4,"evaluated linked circles");
    need(sketch["parameters"].toObject()["constraints"].toArray().size()>=8,"four offset and center relations");
    QJsonArray pegProfiles;for(const auto& profile:doc.sketches().back().profiles)
        if(std::abs(profile.summary["areaMm2"].toDouble()-std::numbers::pi*1.05*1.05)<1e-4)pegProfiles.append(profile.profileId);
    need(pegProfiles.size()==4,"four distinct 1.05mm peg profiles");
    const auto joined=CadDocument::extrusionCandidate(doc.authored(),{{"ownerId",owner},{"sketchId",sketch["id"]},
        {"profileIds",pegProfiles},{"distanceMm",5},{"operation","join"},{"targetBodyIds",QJsonArray{box["bodyId"]}}});
    commit(doc,joined,"Join four pegs 5mm");
    need(doc.bodies().size()==13 && doc.bodies().back().summary["valid"].toBool(),"four pegs joined to case floor");
    const auto beforeMove=doc.authored();auto shiftedPlacement=placement;shiftedPlacement["translationMm"]=QJsonArray{29,-42.5,6};
    commit(doc,CadDocument::localReferencePlacementCandidate(beforeMove,import["id"].toString(),shiftedPlacement),"move captured board");
    const auto movedCircles=doc.sketches().back().summary["projectedCircles"].toArray();need(movedCircles.size()==4,"moved projection retains four circles");
    const auto firstBefore=context["circles"].toArray()[0].toObject(),firstAfter=movedCircles[0].toObject();
    const auto oldCenter=world(top->frame,firstBefore["uMm"].toDouble(),firstBefore["vMm"].toDouble());
    const auto newCenter=world(doc.sketches().back().resolvedFrame,firstAfter["uMm"].toDouble(),firstAfter["vMm"].toDouble());
    need(firstBefore["id"]==firstAfter["id"] && std::abs(newCenter.X()-oldCenter.X()-1)<1e-5
        && std::abs(newCenter.Y()-oldCenter.Y())<1e-5,"retained projection recomputed after captured placement");
    doc.history().undo();need(doc.authored()==beforeMove,"captured placement undo restores pegs and board");
    QTemporaryDir temp;need(temp.isValid(),"temp");const auto saved=temp.filePath("circles.alloy");need(doc.save(saved).isEmpty(),"save");
    CadDocument reopened;need(reopened.open(saved).isEmpty(),"reopen");need(reopened.authored()==doc.authored(),"authored circular refs preserved");
    const auto before=reopened.authored();auto broken=before;auto features=broken["features"].toArray();features.removeAt(0);broken["features"]=features;
    need(!reopened.commit(broken,"remove board").isEmpty() && reopened.authored()==before,"missing imported source refuses atomically");
    std::cout<<"four retained imported circles, inward offsets, Join5, placement update, undo, reopen, missing source refusal passed\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
