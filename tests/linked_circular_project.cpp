// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <BRepAdaptor_Curve.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include <QCoreApplication>
#include <QTemporaryDir>
#include <algorithm>
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
    {
        QTemporaryDir fixtureCopy;need(fixtureCopy.isValid(),"port fixture temp");
        const auto fixturePath=fixtureCopy.filePath("board-and-case.alloy");need(doc.save(fixturePath).isEmpty(),"save port baseline");
        CadDocument ports;need(ports.open(fixturePath).isEmpty(),"open imported board and sibling Case");
        const auto wall=std::find_if(floor.supportFaces.begin(),floor.supportFaces.end(),[](const auto& face){return face.role=="box.y1";});
        need(wall!=floor.supportFaces.end(),"Case front support");
        const auto planeId=CadDocument::newId();
        append(ports,{{"id",planeId},{"ownerId",owner},{"type","constructionPlane"},{"name","Port plane"},{"visible",true},
            {"parameters",QJsonObject{{"source",QJsonObject{{"support",ref(*wall)}}},{"offsetMm",0}}}});
        auto projected=SketchGeometry::newConstructionPlaneFeature(owner,planeId,"Three imported faces");
        QJsonArray groups;
        for(const auto& name:{"SIDE-ETH-001","SIDE-USB-A-001","SIDE-USB-B-001"}) {
            const auto connector=std::find_if(ports.bodies().begin(),ports.bodies().end(),[&](const auto& body){return body.name==name;});
            need(connector!=ports.bodies().end(),"imported connector exists");
            const auto face=std::find_if(connector->supportFaces.begin(),connector->supportFaces.end(),[](const auto& face){
                const auto frame=face.frame;return std::abs(frame["origin"].toArray()[1].toDouble()-45.5)<1e-6
                    && frame["normal"].toArray()[1].toDouble()>.99 && face.boundaryEdges.size()==4;
            });
            need(face!=connector->supportFaces.end(),"current rectangular connector face");
            QJsonArray ids;
            for(const auto& edge:face->boundaryEdges) {
                projected=SketchGeometry::linkedProjectionCandidate(ports.evaluation(),projected,{{"bodyId",connector->bodyId},{"edgeId",edge.edgeId}});
                ids.append(projected["parameters"].toObject()["projections"].toArray().last().toObject()["id"]);
            }
            groups.append(QJsonObject{{"id",CadDocument::newId()},{"source",ref(*face)},{"projectionIds",ids}});
        }
        auto parameters=projected["parameters"].toObject();parameters["faceProjectionGroups"]=groups;projected["parameters"]=parameters;
        auto candidate=ports.authored();auto features=candidate["features"].toArray();features.append(projected);candidate["features"]=features;
        // A group cannot use the foreign source body with a producer from the target owner.
        auto invalid=candidate;auto badFeatures=features;auto bad=projected;auto badParameters=parameters;auto badGroups=groups;
        auto badGroup=badGroups[0].toObject(),badSource=badGroup["source"].toObject();badSource["producerFeatureId"]=box["id"];
        badGroup["source"]=badSource;badGroups[0]=badGroup;badParameters["faceProjectionGroups"]=badGroups;bad["parameters"]=badParameters;
        badFeatures[badFeatures.size()-1]=bad;invalid["features"]=badFeatures;
        const auto before=ports.authored();
        const auto refused=ports.commit(invalid,"invalid group producer");
        need(!refused.isEmpty() && ports.lastEvaluationError()["code"]=="cross_component_dependency"
            && ports.authored()==before,"foreign face group with mismatched producer refuses atomically");
        commit(ports,candidate,"three imported face groups");
        need(ports.sketches().back().profiles.size()==3,"twelve imported face edges form three port profiles");
        const auto committed=ports.authored();
        need(committed["features"].toArray().last().toObject()["parameters"].toObject()["faceProjectionGroups"].toArray()==groups,
            "authored imported face groups retained exactly");
        ports.history().undo();need(ports.authored()==before,"grouped Project undo");ports.history().redo();
        need(ports.authored()==committed,"grouped Project redo");
        QTemporaryDir saved;need(saved.isValid(),"port temp");const auto path=saved.filePath("ports.alloy");
        need(ports.save(path).isEmpty(),"save imported face groups");CadDocument reopened;
        need(reopened.open(path).isEmpty() && reopened.authored()==committed && reopened.sketches().back().profiles.size()==3,
            "reopen exact imported face groups and profiles");
    }
    auto sketch=SketchGeometry::newSupportedFeature(owner,ref(*top),"Mounting Circles",GeometryKernel::supportEdgeDescriptors(*top));
    {
        auto simulation=doc.evaluation();EvaluatedBody arcBody;
        arcBody.bodyId=CadDocument::newId();arcBody.occurrenceId=CadDocument::newId();
        const auto arcId=CadDocument::newId();
        const auto arc=BRepBuilderAPI_MakeEdge(gp_Circ(gp_Ax2(gp_Pnt(0,0,7.6),gp_Dir(0,0,1)),1.35),0,std::numbers::pi).Edge();
        BRep_Builder builder;TopoDS_Compound compound;builder.MakeCompound(compound);builder.Add(compound,arc);arcBody.shape=compound;
        EvaluatedSupportFace source;source.boundaryEdges.push_back({arcId,"curve",arc,{{"occurrenceId",arcBody.occurrenceId}}});
        arcBody.supportFaces.push_back(source);simulation.bodies.push_back(arcBody);
        bool refused=false;try{SketchGeometry::linkedProjectionCandidate(simulation,sketch,{{"bodyId",arcBody.bodyId},{"edgeId",arcId}});}
        catch(const GeometryKernel::EvaluationError& error){refused=error.diagnostics()["code"]=="unsupported_projection";}
        need(refused,"partial circular arc refuses full-circle projection");
    }
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
