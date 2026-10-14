// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QJsonDocument>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <algorithm>
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
QJsonObject ref(const EvaluatedSupportFace& face){return {{"producerFeatureId",face.producerFeatureId},{"bodyId",face.bodyId},{"faceId",face.faceId}};}
void commit(CadDocument& d,const QJsonObject& next,const char* message){const auto error=d.commit(next,message);if(!error.isEmpty())throw std::runtime_error(QString("%1: %2").arg(message,error).toStdString());}
void append(CadDocument& d,const QJsonObject& feature){auto next=d.authored();auto features=next["features"].toArray();features.append(feature);next["features"]=features;commit(d,next,"append");}
QJsonObject point(const QJsonObject& context,const QString& id){for(const auto& value:context["points"].toArray())if(value.toObject()["id"]==id)return value.toObject();return {};}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    CadDocument d;auto components=CadDocument::componentCandidate(d.authored(),"Box");commit(d,components,"Box component");
    const auto boxOwner=components["components"].toArray()[0].toObject()["id"].toString();
    check(d.activateOwner(boxOwner).isEmpty(),"activate Box");
    auto box=QJsonObject{{"id",CadDocument::newId()},{"bodyId",CadDocument::newId()},{"ownerId",boxOwner},
        {"type","box"},{"name","Box"},{"visible",true},
        {"parameters",QJsonObject{{"widthMm",50},{"depthMm",80},{"heightMm",40},{"xMm",0},{"yMm",0},{"zMm",0}}}};
    append(d,box);auto body=d.bodies().at(0);auto top=std::find_if(body.supportFaces.begin(),body.supportFaces.end(),[](const auto& f){return f.role=="box.z1";});
    check(top!=body.supportFaces.end(),"top face");commit(d,CadDocument::shellCandidate(d.authored(),{{"ownerId",boxOwner},{"targetBodyId",body.bodyId},{"removedFace",ref(*top)},{"thicknessMm",4}}),"Shell4");
    body=d.bodies().at(0);auto front=std::find_if(body.supportFaces.begin(),body.supportFaces.end(),[](const auto& f){return f.role=="box.y0";});
    check(front!=body.supportFaces.end(),"front face");const auto support=ref(*front);
    auto sketch=SketchGeometry::newSupportedFeature(boxOwner,support,"Front",GeometryKernel::supportEdgeDescriptors(*front));
    auto baseline=sketch;for(const auto& edge:front->boundaryEdges)sketch=SketchGeometry::linkedProjectionCandidate(d.evaluation(),sketch,{{"bodyId",body.bodyId},{"edgeId",edge.edgeId}});
    check(sketch["parameters"].toObject()["projections"].toArray().size()==4,"face Project dedupes auto support edges");
    auto context=GeometryKernel::projectionContext(d.evaluation(),sketch);auto solved=SketchGeometry::solveDraft(sketch,context);check(solved.ok,"face Project solves");
    sketch=solved.feature;const auto projected=SketchGeometry::evaluateFeature(sketch,front->frame,context);
    if(projected.profiles.size()!=1)std::cerr<<QJsonDocument(projected.summary).toJson(QJsonDocument::Compact).toStdString()<<'\n';
    check(projected.profiles.size()==1,"projected face edges form profile");
    QString innerEdge;for(const auto& face:body.supportFaces)if(face.role=="shell-inner")for(const auto& edge:face.boundaryEdges){
        const auto a=edge.provenance["startMm"].toArray(),b=edge.provenance["endMm"].toArray();
        if(a.size()==3 && b.size()==3 && std::abs(a[0].toDouble()-b[0].toDouble())<1e-6
            && std::abs(a[2].toDouble()-b[2].toDouble())<1e-6 && std::abs(a[1].toDouble()-b[1].toDouble())>1){innerEdge=edge.edgeId;break;}
    }
    check(!innerEdge.isEmpty(),"perpendicular inner edge");
    sketch=SketchGeometry::linkedProjectionCandidate(d.evaluation(),sketch,{{"bodyId",body.bodyId},{"edgeId",innerEdge}});
    const auto points=sketch["parameters"].toObject()["pointProjections"].toArray();check(points.size()==1,"longitudinal edge projects as point");
    const auto projectedId=points[0].toObject()["id"].toString();context=GeometryKernel::projectionContext(d.evaluation(),sketch);
    check(!point(context,projectedId).isEmpty(),"point resolved");check(SketchGeometry::solveDraft(sketch,context).ok,"point Project solves");
    const auto anchor=point(context,projectedId);
    auto constrained=SketchGeometry::addPointerEntity(sketch,"line",{{"u1Mm",anchor["uMm"]},{"v1Mm",anchor["vMm"]},
        {"u2Mm",anchor["uMm"].toDouble()+10},{"v2Mm",anchor["vMm"]},
        {"startSnapTarget",QJsonObject{{"kind","point"},{"pointId",projectedId}}}},context);
    if(!constrained.ok)std::cerr<<constrained.error.toStdString()<<'\n';
    check(constrained.ok,"authored line endpoint snaps to retained projected point");sketch=constrained.feature;
    const auto authoredLine=sketch["parameters"].toObject()["entities"].toArray().last().toObject();
    const auto authoredStart=authoredLine["startPointId"].toString();
    append(d,sketch);const auto stableId=projectedId;
    check(!d.sketches().empty() && !d.sketches()[0].profiles.empty(),"linked profile remains selectable after point anchor");
    const auto extrusion=CadDocument::evaluateCandidate(CadDocument::extrusionCandidate(d.authored(),
        {{"ownerId",boxOwner},{"sketchId",sketch["id"]},{"profileId",d.sketches()[0].profiles[0].profileId},{"distanceMm",5}}));
    check(extrusion.ok && extrusion.evaluation->bodies.size()==2 && extrusion.evaluation->bodies[1].summary["valid"].toBool(),
        "projected-only profile evaluates as an ordinary extruded solid");
    const auto planeId=CadDocument::newId();
    append(d,{{"id",planeId},{"ownerId",boxOwner},{"type","constructionPlane"},{"name","Front offset plane"},{"visible",true},
        {"parameters",QJsonObject{{"source",QJsonObject{{"support",support}}},{"offsetMm",0}}}});
    auto planeSketch=SketchGeometry::newConstructionPlaneFeature(boxOwner,planeId,"Planar linked Project");
    for(const auto& edge:front->boundaryEdges)planeSketch=SketchGeometry::linkedProjectionCandidate(d.evaluation(),planeSketch,
        {{"bodyId",body.bodyId},{"edgeId",edge.edgeId}});
    auto planeContext=GeometryKernel::projectionContext(d.evaluation(),planeSketch);
    check(planeContext["lines"].toArray().size()==4 && SketchGeometry::solveDraft(planeSketch,planeContext).ok,
        "retained construction plane projects four current source edges");
    QJsonArray rectSources;for(const auto& value:planeSketch["parameters"].toObject()["projections"].toArray())
        rectSources.append(value.toObject()["id"]);
    const auto offset=SketchGeometry::addClosedRectOffset(planeSketch,rectSources,1,planeContext);
    if(!offset.ok)std::cerr<<offset.error.toStdString()<<'\n';
    check(offset.ok,"linked closed rectangle offsets outward");planeSketch=offset.feature;
    check(planeSketch["parameters"].toObject()["closedRectOffsets"].toArray().size()==1,
        "closed Offset retains one source-derived group");
    check(SketchGeometry::evaluateFeature(planeSketch,d.evaluation().constructionPlanes.back().frame,planeContext).profiles.size()==1,
        "offset silhouette has one selectable profile");
    const auto planeLineId=planeSketch["parameters"].toObject()["projections"].toArray()[0].toObject()["id"].toString();
    append(d,planeSketch);
    auto shifted=d.authored();auto shiftFeatures=shifted["features"].toArray();
    for(int i=0;i<shiftFeatures.size();++i)if(shiftFeatures[i].toObject()["id"]==planeId) {
        auto edited=shiftFeatures[i].toObject(),values=edited["parameters"].toObject();values["offsetMm"]=.25;edited["parameters"]=values;shiftFeatures[i]=edited;break;
    }
    shifted["features"]=shiftFeatures;commit(d,shifted,"edit retained plane");
    auto currentPlaneSketch=SketchGeometry::findFeature(d.authored(),planeSketch["id"].toString());
    planeContext=GeometryKernel::projectionContext(d.evaluationBefore(planeSketch["id"].toString()),currentPlaneSketch);
    check(planeContext["lines"].toArray().size()==4
        && currentPlaneSketch["parameters"].toObject()["projections"].toArray()[0].toObject()["id"]==planeLineId,
        "plane edit preserves live projected source identities");
    auto linked=CadDocument::componentCandidate(d.authored(),"Lid");commit(d,linked,"Lid component");
    const auto lidOwner=linked["components"].toArray().last().toObject()["id"].toString();
    auto lid=SketchGeometry::newSupportedFeature(lidOwner,support,"Lid front",GeometryKernel::supportEdgeDescriptors(*front));
    lid=SketchGeometry::linkedProjectionCandidate(d.evaluation(),lid,{{"bodyId",body.bodyId},{"edgeId",innerEdge}});
    check(lid["ownerId"]==lidOwner && lid["parameters"].toObject()["pointProjections"].toArray().size()==1,"sibling linked source and destination owner");
    append(d,lid);const auto before=d.authored();
    auto resized=before;auto features=resized["features"].toArray();auto first=features[0].toObject();auto p=first["parameters"].toObject();
    p["widthMm"]=74;p["depthMm"]=100;p["heightMm"]=40;first["parameters"]=p;features[0]=first;resized["features"]=features;
    commit(d,resized,"resize Box 74x40x100");
    const auto current=SketchGeometry::findFeature(d.authored(),sketch["id"].toString());
    check(current["parameters"].toObject()["pointProjections"].toArray()[0].toObject()["id"]==stableId,"projected point ID survives upstream edit");
    const auto movedFront=GeometryKernel::projectionContext(d.evaluationBefore(sketch["id"].toString()),current);
    const auto resizedAnchor=point(movedFront,stableId);
    const auto resolved=SketchGeometry::solveDraft(current,movedFront);check(resolved.ok,"constrained line resolves after upstream edit");
    QJsonObject resolvedStart;for(const auto& item:resolved.feature["parameters"].toObject()["points"].toArray())
        if(item.toObject()["id"]==authoredStart)resolvedStart=item.toObject();
    check(!resolvedStart.isEmpty() && std::abs(resolvedStart["uMm"].toDouble()-resizedAnchor["uMm"].toDouble())<1e-6
        && std::abs(resolvedStart["vMm"].toDouble()-resizedAnchor["vMm"].toDouble())<1e-6,
        "authored constrained endpoint follows projected anchor");
    const auto moved=GeometryKernel::projectionContext(d.evaluationBefore(lid["id"].toString()),lid);
    check(!point(moved,lid["parameters"].toObject()["pointProjections"].toArray()[0].toObject()["id"].toString()).isEmpty(),"sibling point recomputed");
    QTemporaryDir dir;check(dir.isValid(),"temporary directory");const auto path=dir.filePath("linked.alloy");check(d.save(path).isEmpty(),"save");CadDocument reopened;
    check(reopened.open(path).isEmpty() && reopened.authored()==d.authored(),"reopen stable authored links");
    auto missing=reopened.authored();auto broken=missing["features"].toArray();broken.removeAt(1);missing["features"]=broken;
    check(!reopened.commit(missing,"remove Shell").isEmpty() && reopened.authored()==d.authored(),"missing source refuses atomically");
    std::cout<<"linked face, collapsed point, sibling support, resize, reopen and refusal passed\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
