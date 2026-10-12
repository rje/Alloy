// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMap>
#include <QPointF>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <numbers>
#include <stdexcept>
namespace {
void check(bool ok,const QString& message){if(!ok)throw std::runtime_error(message.toStdString());}
QJsonObject accepted(const SketchGeometry::SolveResult& result){check(result.ok,result.error);return result.feature;}
QJsonObject entity(const QJsonObject& f,int index){return f["parameters"].toObject()["entities"].toArray()[index].toObject();}
void worldZ(const EvaluatedSketch& sketch,double z){for(const auto& value:sketch.summary["worldPoints"].toArray())check(std::abs(value.toObject()["positionMm"].toArray()[2].toDouble()-z)<1e-6,"Face sketch world plane is wrong");}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try {
    CadDocument document;const auto owner=document.authored()["documentId"].toString();
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
    const auto regions=SketchProfiles::deriveRegions(f);check(regions.ok && regions.profiles.size()==3,regions.error);
    check(document.commit(SketchGeometry::replaceFeature(document.authored(),f),"Rounded source").isEmpty(),"Commit bracket source");
    QJsonArray profileIds;for(const auto& profile:regions.profiles)profileIds.append(profile.profileId);
    check(document.commit(CadDocument::extrusionCandidate(document.authored(),{{"sketchId",f["id"]},{"profileIds",profileIds},{"distanceMm",50}}),"Extrude bracket").isEmpty(),"Commit bracket union");
    const auto producer=document.authored()["features"].toArray().last().toObject();
    const auto supportCap=document.bodies()[0].supportFaces.at(0);
    QJsonObject support{{"producerFeatureId",producer["id"]},{"bodyId",producer["bodyId"]},{"faceId",supportCap.faceId}};
    const auto resolved=GeometryKernel::resolveSketchSupport(document.evaluation(),support);
    check(resolved.faceId==supportCap.faceId && resolved.frame["origin"].toArray()==QJsonArray{0,0,50},"Semantic cap frame drifted to its bounds");
    auto attached=SketchGeometry::newSupportedFeature(owner,support,"End-cap rectangle");
    attached=SketchGeometry::addRectangle(attached,{{"u1Mm",10},{"v1Mm",20},{"u2Mm",20},{"v2Mm",28}});
    attached=accepted(SketchGeometry::solveDraft(attached));
    bool needsFrame=false;try{SketchGeometry::evaluateFeature(attached);}catch(const std::exception&){needsFrame=true;}
    check(needsFrame,"Supported sketch silently fell back to an origin plane");
    check(document.commit(SketchGeometry::replaceFeature(document.authored(),attached),"Face sketch").isEmpty(),"Commit face-supported sketch");
    worldZ(document.sketches()[1],50);const auto originalFrame=document.sketches()[1].resolvedFrame;
    check(attached["parameters"].toObject()["plane"]=="face" && !attached["parameters"].toObject().contains("frame"),"Authored support persisted a stale frame");
    const auto attachedId=attached["id"].toString(),profileId=document.sketches()[1].profiles[0].profileId;
    const auto radiusId=f["parameters"].toObject()["dimensions"].toArray().last().toObject()["id"];
    f=accepted(SketchGeometry::updateDimension(f,{{"dimensionId",radiusId},{"valueMm",90}}));
    f=accepted(SketchGeometry::updateDimension(f,{{"dimensionId",capDimension},{"valueMm",120}}));
    check(document.commit(SketchGeometry::replaceFeature(document.authored(),f),"Resize support source").isEmpty(),"Parent radius/width edit failed");
    check(document.bodies()[0].supportFaces[0].faceId==supportCap.faceId && document.sketches()[1].resolvedFrame==originalFrame,"Parent edit moved cap origin or changed role identity");
    check(SketchGeometry::findFeature(document.authored(),attachedId)==attached,"Parent edit mutated local face-sketch intent");worldZ(document.sketches()[1],50);
    check(document.commit(CadDocument::extrusionCandidate(document.authored(),{{"featureId",producer["id"]},{"sketchId",f["id"]},{"profileIds",profileIds},{"distanceMm",60}}),"Change support depth").isEmpty(),"Depth change did not transport supported frame");
    worldZ(document.sketches()[1],60);check(document.sketches()[1].resolvedFrame["origin"].toArray()==QJsonArray{0,0,60},"Depth change used a centroid frame");
    check(document.sketches()[1].profiles[0].profileId==profileId,"Frame transport changed local profile identity");
    const auto beforeBoss=document.authored();
    check(document.commit(CadDocument::extrusionCandidate(beforeBoss,{{"sketchId",attachedId},{"profileId",profileId},{"distanceMm",5}}),"Face sketch extrusion").isEmpty(),"Nested extrusion ignored resolved frame");
    const auto bounds=document.bodies()[1].summary["boundsMm"].toObject();
    check(std::abs(bounds["min"].toArray()[2].toDouble()-60)<1e-6 && std::abs(bounds["max"].toArray()[2].toDouble()-65)<1e-6,"Nested extrusion started at origin plane");
    const auto stable=document.authored();const auto revision=document.revision();const auto history=document.history().count();
    auto missing=attached;auto parameters=missing["parameters"].toObject();auto missingSupport=support;missingSupport["faceId"]=CadDocument::newId();parameters["support"]=missingSupport;missing["parameters"]=parameters;
    check(!document.commit(SketchGeometry::replaceFeature(stable,missing),"Missing cap").isEmpty() && document.lastEvaluationError()["code"]=="support_unavailable","Stale support fell back to another face");
    auto forward=stable;auto ordered=forward["features"].toArray();const auto dependent=ordered.takeAt(2);ordered.insert(1,dependent);forward["features"]=ordered;
    check(!document.commit(forward,"Forward support").isEmpty() && document.lastEvaluationError()["code"]=="forward_dependency","Support dependency order was not retained");
    const auto reversed=CadDocument::extrusionCandidate(stable,{{"featureId",producer["id"]},{"sketchId",f["id"]},{"profileIds",profileIds},{"distanceMm",-60}});
    check(!document.commit(reversed,"Unsupported negative support").isEmpty(),"Negative cap support silently flipped authored axes");
    check(document.authored()==stable && document.revision()==revision && document.history().count()==history,"Support failure changed accepted work");
    document.history().undo();check(document.authored()==beforeBoss,"Undo lost prior support references");document.history().redo();check(document.authored()==stable,"Redo changed local support IDs");
    QTemporaryDir directory;check(directory.isValid(),"Support storage directory");const auto path=directory.path()+"/supported-sketch.alloy";
    check(document.save(path).isEmpty(),"Save supported sketch");CadDocument reopened;
    check(reopened.open(path).isEmpty() && reopened.authored()==stable,"Reopen lost authored support relationships");
    worldZ(reopened.sketches()[1],60);check(reopened.bodies()[0].supportFaces[0].faceId==supportCap.faceId,"Reopen changed semantic cap role");
    // Associative straight references use the real rounded cap, not coordinates copied into saved intent.
    const auto linkedSeedPath=directory.path()+"/linked-source.alloy";
    QFile linkedSeed(linkedSeedPath);check(linkedSeed.open(QIODevice::WriteOnly),"Write linked source fixture");
    check(linkedSeed.write(QJsonDocument(beforeBoss).toJson())>0,"Store linked source fixture");linkedSeed.close();
    CadDocument linked;const auto linkedSeedError=linked.open(linkedSeedPath);check(linkedSeedError.isEmpty(),"Prepare linked source: "+linkedSeedError);
    auto linkedCap=GeometryKernel::resolveSketchSupport(linked.evaluation(),support);
    auto projected=SketchGeometry::newSupportedFeature(owner,support,"Linked construction",GeometryKernel::supportEdgeDescriptors(linkedCap));
    const auto retainedProjections=projected["parameters"].toObject()["projections"].toArray();
    check(retainedProjections.size()==4,"Actual rounded cap must auto-retain four straight boundary references");
    auto context=GeometryKernel::projectionContext(linked.evaluation(),projected);
    QString firstProjection,secondProjection;
    for(const auto& reference:retainedProjections){const auto record=reference.toObject();
        for(const auto& edge:linkedCap.boundaryEdges)if(edge.edgeId==record["source"].toObject()["edgeId"]){
            if(edge.provenance["sourceEntityId"]==entity(f,0)["id"])firstProjection=record["id"].toString();
            if(edge.provenance["sourceEntityId"]==entity(f,1)["id"])secondProjection=record["id"].toString();
        }}
    check(!firstProjection.isEmpty() && !secondProjection.isEmpty(),"Tutorial outer edge identities missing");
    projected=SketchGeometry::addLine(projected,{{"u1Mm",60},{"v1Mm",80},{"u2Mm",160},{"v2Mm",90}});
    projected=SketchGeometry::addLine(projected,{{"u1Mm",500},{"v1Mm",120},{"u2Mm",620},{"v2Mm",180}});
    const auto firstLocal=entity(projected,0),secondLocal=entity(projected,1);
    projected=accepted(SketchGeometry::setConstruction(projected,{{"entityIds",QJsonArray{firstLocal["id"],secondLocal["id"]}},{"construction",true}},context));
    projected=accepted(SketchGeometry::addConstraint(projected,{{"type","parallel"},{"entityIds",QJsonArray{firstLocal["id"],firstProjection}}},context));
    projected=accepted(SketchGeometry::addConstraint(projected,{{"type","parallel"},{"entityIds",QJsonArray{secondLocal["id"],secondProjection}}},context));
    projected=accepted(SketchGeometry::addDimension(projected,{{"kind","horizontal"},{"pointIds",QJsonArray{firstLocal["startPointId"],SketchGeometry::projectionPointId(firstProjection,false)}},{"valueMm",40}},context));
    check(projected["parameters"].toObject()["points"].toArray().size()==4 && projected["parameters"].toObject()["entities"].toArray().size()==2,"Derived projection coordinates leaked into authored geometry");
    check(!SketchGeometry::solveDraft(projected).ok,"Linked references solved without current source context");
    check(linked.commit(SketchGeometry::replaceFeature(linked.authored(),projected),"Two external Parallel references").isEmpty(),"Commit linked Parallel/distance");
    const auto linkedBefore=linked.authored();const auto beforeContext=GeometryKernel::projectionContext(linked.evaluation(),projected);
    auto resized=accepted(SketchGeometry::updateDimension(f,{{"dimensionId",capDimension},{"valueMm",140}}));
    resized=accepted(SketchGeometry::updateDimension(resized,{{"dimensionId",radiusId},{"valueMm",80}}));
    check(linked.commit(SketchGeometry::replaceFeature(linked.authored(),resized),"Resize projected source").isEmpty(),"Source resize lost cap line lineage");
    const auto linkedAfter=linked.authored();projected=SketchGeometry::findFeature(linkedAfter,projected["id"].toString());
    check(projected["parameters"].toObject()["projections"].toArray()==retainedProjections,"Upstream resize renamed or appended auto projections");
    context=GeometryKernel::projectionContext(linked.evaluation(),projected);check(context["lines"]!=beforeContext["lines"],"Projected coordinates did not refresh after source changes");
    const auto evaluatedLinked=SketchGeometry::evaluateFeature(projected,linkedCap.frame,context);
    check(evaluatedLinked.profiles.empty() && evaluatedLinked.summary["projectedLines"].toArray().size()==4,"Reference-only lines created profiles or disappeared");
    check(linked.commit(CadDocument::extrusionCandidate(linked.authored(),{{"featureId",producer["id"]},{"sketchId",f["id"]},{"profileIds",profileIds},{"distanceMm",70}}),"Transport projected cap").isEmpty(),"Projected frame depth transport failed");
    worldZ(linked.sketches()[2],70);
    auto badProjection=SketchGeometry::findFeature(linked.authored(),projected["id"].toString());auto badParameters=badProjection["parameters"].toObject();
    auto badRefs=badParameters["projections"].toArray();auto badRef=badRefs[0].toObject();auto badSource=badRef["source"].toObject();badSource["edgeId"]=CadDocument::newId();badRef["source"]=badSource;badRefs[0]=badRef;badParameters["projections"]=badRefs;badProjection["parameters"]=badParameters;
    const auto beforeRejected=linked.authored();const auto rejectedCount=linked.history().count();
    check(!linked.commit(SketchGeometry::replaceFeature(beforeRejected,badProjection),"Unavailable projection").isEmpty() && linked.lastEvaluationError()["code"]=="projection_unavailable","Missing projection was frozen or substituted");
    check(linked.authored()==beforeRejected && linked.history().count()==rejectedCount,"Projection rejection changed authored work/history");
    linked.history().undo();check(linked.authored()==linkedAfter,"Projection depth undo changed local intent");linked.history().undo();check(linked.authored()==linkedBefore,"Projection source undo lost relationships");linked.history().redo();linked.history().redo();check(linked.authored()==beforeRejected,"Projection redo changed references");
    const auto linkedPath=directory.path()+"/linked-support.alloy";check(linked.save(linkedPath).isEmpty(),"Save linked support");CadDocument linkedReopen;check(linkedReopen.open(linkedPath).isEmpty() && linkedReopen.authored()==beforeRejected,"Reopen lost projection intent");
    worldZ(linkedReopen.sketches()[2],70);
    auto snapped=SketchGeometry::findFeature(linkedReopen.authored(),projected["id"].toString());
    auto snapContext=GeometryKernel::projectionContext(linkedReopen.evaluation(),snapped);
    const auto snapTarget=SketchGeometry::projectionPointId(secondProjection,false);
    QJsonObject externalPoint;
    for(const auto& value:SketchGeometry::expandedProjectionFeature(snapped,snapContext)["parameters"].toObject()["points"].toArray())if(value.toObject()["id"]==snapTarget)externalPoint=value.toObject();
    check(!externalPoint.isEmpty(),"Projected snap endpoint unavailable");
    const auto priorPoints=snapped["parameters"].toObject()["points"].toArray().size();
    snapped=accepted(SketchGeometry::addPointerEntity(snapped,"line",{{"u1Mm",-999},{"v1Mm",-999},{"u2Mm",externalPoint["uMm"].toDouble()+30},{"v2Mm",externalPoint["vMm"].toDouble()+50},{"startSnapTarget",QJsonObject{{"kind","point"},{"pointId",snapTarget}}}},snapContext));
    const auto snapLine=snapped["parameters"].toObject()["entities"].toArray().last().toObject();
    check(snapLine["startPointId"]!=snapTarget,"Snapped line took ownership of a projected endpoint");
    snapped=accepted(SketchGeometry::addPointerCircle(snapped,{{"centerUMm",-999},{"centerVMm",-999},{"diameterMm",50},{"centerSnapTarget",QJsonObject{{"kind","point"},{"pointId",snapTarget}}}},snapContext));
    const auto snapCircle=snapped["parameters"].toObject()["entities"].toArray().last().toObject();
    check(snapCircle["centerPointId"]!=snapTarget && snapCircle["centerPointId"]!=snapLine["startPointId"],"Circle snap reused external or line point ownership");
    check(snapped["parameters"].toObject()["points"].toArray().size()==priorPoints+3,"Snapping persisted virtual points");
    int snapRelations=0;for(const auto& value:snapped["parameters"].toObject()["constraints"].toArray()){const auto relation=value.toObject();if(relation["type"]=="coincident" && relation["pointIds"].toArray().contains(snapTarget))++snapRelations;}
    check(snapRelations==2,"Line/circle snaps did not retain explicit coincidence");
    const auto staleSnap=SketchGeometry::addPointerCircle(snapped,{{"centerUMm",0},{"centerVMm",0},{"diameterMm",50},{"centerSnapTarget",QJsonObject{{"kind","point"},{"pointId",CadDocument::newId()}}}},snapContext);
    check(!staleSnap.ok && staleSnap.feature==snapped,"Stale point snap changed the draft");
    check(linkedReopen.commit(SketchGeometry::replaceFeature(linkedReopen.authored(),snapped),"Snapped line and circle").isEmpty(),"Commit projected endpoint snaps");
    const auto beforeSnapSourceEdit=linkedReopen.authored();
    resized=accepted(SketchGeometry::updateDimension(resized,{{"dimensionId",capDimension},{"valueMm",160}}));
    resized=accepted(SketchGeometry::updateDimension(resized,{{"dimensionId",radiusId},{"valueMm",70}}));
    check(linkedReopen.commit(SketchGeometry::replaceFeature(linkedReopen.authored(),resized),"Move snapped source").isEmpty(),"Source update broke projected coincidences");
    const auto afterSnapSourceEdit=linkedReopen.authored();snapped=SketchGeometry::findFeature(afterSnapSourceEdit,snapped["id"].toString());
    const auto updatedSnapContext=GeometryKernel::projectionContext(linkedReopen.evaluation(),snapped);
    QMap<QString,QPointF> snapPositions;
    for(const auto& value:SketchGeometry::expandedProjectionFeature(snapped,updatedSnapContext)["parameters"].toObject()["points"].toArray()){const auto point=value.toObject();snapPositions[point["id"].toString()]={point["uMm"].toDouble(),point["vMm"].toDouble()};}
    check((snapPositions[snapTarget]-QPointF(externalPoint["uMm"].toDouble(),externalPoint["vMm"].toDouble())).manhattanLength()>1e-4,"Test source edit did not move its snap target");
    for(const auto& id:{snapLine["startPointId"].toString(),snapCircle["centerPointId"].toString()})check((snapPositions[id]-snapPositions[snapTarget]).manhattanLength()<1e-6,"Authored snap stopped following projected endpoint");
    linkedReopen.history().undo();check(linkedReopen.authored()==beforeSnapSourceEdit,"Snap source undo lost authored relationships");linkedReopen.history().redo();check(linkedReopen.authored()==afterSnapSourceEdit,"Snap source redo changed point IDs");
    check(linkedReopen.save(linkedPath).isEmpty(),"Save projected snaps");CadDocument snapReopened;check(snapReopened.open(linkedPath).isEmpty() && snapReopened.authored()==afterSnapSourceEdit,"Reopen lost projected snap coincidences");

    // Two real rounded-end centers anchor the tutorial guide; Parallel determines its shared bend.
    const auto centerCap=GeometryKernel::resolveSketchSupport(snapReopened.evaluation(),support);
    auto guide=SketchGeometry::newSupportedFeature(owner,support,"Center anchored guide",GeometryKernel::supportEdgeDescriptors(centerCap));
    auto centerContext=GeometryKernel::projectionContext(snapReopened.evaluation(),guide);
    QSet<QString> endCircleIds;for(const auto& value:resized["parameters"].toObject()["entities"].toArray())if(value.toObject()["type"]=="circle")endCircleIds.insert(value.toObject()["id"].toString());
    QJsonObject leftCenter,upperCenter;
    for(const auto& value:centerContext["points"].toArray()){const auto candidate=value.toObject();if(!endCircleIds.contains(candidate["source"].toObject()["sourceEntityId"].toString()))continue;
        if(leftCenter.isEmpty() || candidate["uMm"].toDouble()<leftCenter["uMm"].toDouble()) {upperCenter=leftCenter;leftCenter=candidate;}else upperCenter=candidate;}
    check(!leftCenter.isEmpty() && !upperCenter.isEmpty(),"Actual circular cap descendants lost rounded-end centers");
    check(!guide["parameters"].toObject().contains("pointReferences"),"Available center candidates authored references before acceptance");
    const auto initialExpanded=SketchGeometry::expandedProjectionFeature(guide,centerContext);
    check(initialExpanded["parameters"].toObject()["points"].toArray().size()==8,"Unretained circular centers entered solver geometry");
    const auto badCenter=SketchGeometry::addPointerEntity(guide,"line",{{"u1Mm",0},{"v1Mm",0},{"u2Mm",500},{"v2Mm",80},{"startSnapTarget",QJsonObject{{"kind","point"},{"pointId",leftCenter["id"]}}},{"endSnapTarget",QJsonObject{{"kind","point"},{"pointId",CadDocument::newId()}}}},centerContext);
    check(!badCenter.ok && badCenter.feature==guide,"Rejected center snap leaked point reference/history intent");
    guide=accepted(SketchGeometry::addPointerEntity(guide,"line",{{"u1Mm",0},{"v1Mm",0},{"u2Mm",500},{"v2Mm",80},{"startSnapTarget",QJsonObject{{"kind","point"},{"pointId",leftCenter["id"]}}}},centerContext));
    const auto guideFirst=entity(guide,0);
    guide=accepted(SketchGeometry::addPointerEntity(guide,"line",{{"u1Mm",500},{"v1Mm",80},{"u2Mm",0},{"v2Mm",0},{"startSnapTarget",QJsonObject{{"kind","point"},{"pointId",guideFirst["endPointId"]}}},{"endSnapTarget",QJsonObject{{"kind","point"},{"pointId",upperCenter["id"]}}}},centerContext));
    const auto guideSecond=entity(guide,1);
    guide=accepted(SketchGeometry::setConstruction(guide,{{"entityIds",QJsonArray{guideFirst["id"],guideSecond["id"]}},{"construction",true}},centerContext));
    QMap<QString,QString> guideReferences;
    for(const auto& value:guide["parameters"].toObject()["projections"].toArray()){const auto record=value.toObject();for(const auto& edge:centerCap.boundaryEdges)if(edge.edgeId==record["source"].toObject()["edgeId"])guideReferences[edge.provenance["sourceEntityId"].toString()]=record["id"].toString();}
    guide=accepted(SketchGeometry::addConstraint(guide,{{"type","parallel"},{"entityIds",QJsonArray{guideFirst["id"],guideReferences[entity(resized,0)["id"].toString()]}}},centerContext));
    const auto guideSolve=SketchGeometry::addConstraint(guide,{{"type","parallel"},{"entityIds",QJsonArray{guideSecond["id"],guideReferences[entity(resized,1)["id"].toString()]}}},centerContext);
    guide=accepted(guideSolve);check(guideSolve.diagnostics["dof"].toInt()==0,"Center anchors plus two Parallel constraints did not determine the guide bend");
    const auto centerRefs=guide["parameters"].toObject()["pointReferences"].toArray();check(centerRefs.size()==2,"Guide should retain exactly the two used cap centers");
    guide=accepted(SketchGeometry::addPointerCircle(guide,{{"centerUMm",0},{"centerVMm",0},{"diameterMm",50},{"centerSnapTarget",QJsonObject{{"kind","point"},{"pointId",upperCenter["id"]}}}},centerContext));
    const auto centerCircle=guide["parameters"].toObject()["entities"].toArray().last().toObject();
    guide=accepted(SketchGeometry::addDimension(guide,{{"kind","diameter"},{"entityId",centerCircle["id"]},{"valueMm",50}},centerContext));
    check(guide["parameters"].toObject()["pointReferences"].toArray()==centerRefs && guide["parameters"].toObject()["points"].toArray().size()==5,"Center snap duplicated refs or leaked fixed reference coordinates");
    check(snapReopened.commit(SketchGeometry::replaceFeature(snapReopened.authored(),guide),"Center anchored guide and circle").isEmpty(),"Commit center guide");
    const auto beforeCenterEdit=snapReopened.authored();
    resized=accepted(SketchGeometry::updateDimension(resized,{{"dimensionId",capDimension},{"valueMm",180}}));
    resized=accepted(SketchGeometry::updateDimension(resized,{{"dimensionId",radiusId},{"valueMm",60}}));
    check(snapReopened.commit(SketchGeometry::replaceFeature(snapReopened.authored(),resized),"Resize center referenced source").isEmpty(),"Parent edit lost retained circular center lineage");
    const auto afterCenterEdit=snapReopened.authored();guide=SketchGeometry::findFeature(afterCenterEdit,guide["id"].toString());
    const auto updatedCenters=GeometryKernel::projectionContext(snapReopened.evaluation(),guide);check(updatedCenters["points"]!=centerContext["points"],"Circular centers did not follow parent edits");
    const auto stableCenterSolve=SketchGeometry::solveDraft(guide,updatedCenters);check(guide["parameters"].toObject()["pointReferences"].toArray()==centerRefs && stableCenterSolve.ok && stableCenterSolve.diagnostics["dof"].toInt()==0,"Center IDs or fully constrained guide changed after source edit");
    auto missingCenter=guide;auto missingCenterParameters=missingCenter["parameters"].toObject();auto missingRefs=missingCenterParameters["pointReferences"].toArray();auto missingRef=missingRefs[0].toObject();auto missingCenterSource=missingRef["source"].toObject();missingCenterSource["sourceEntityId"]=CadDocument::newId();missingRef["source"]=missingCenterSource;missingRefs[0]=missingRef;missingCenterParameters["pointReferences"]=missingRefs;missingCenter["parameters"]=missingCenterParameters;
    const auto centerHistory=snapReopened.history().count();check(!snapReopened.commit(SketchGeometry::replaceFeature(afterCenterEdit,missingCenter),"Lost center").isEmpty() && snapReopened.lastEvaluationError()["code"]=="projection_unavailable","Lost center was frozen or substituted");
    check(snapReopened.authored()==afterCenterEdit && snapReopened.history().count()==centerHistory,"Lost center changed accepted intent");
    snapReopened.history().undo();check(snapReopened.authored()==beforeCenterEdit,"Center source undo lost references");snapReopened.history().redo();check(snapReopened.authored()==afterCenterEdit,"Center source redo changed IDs");
    check(snapReopened.save(linkedPath).isEmpty(),"Save center referenced guide");CadDocument centersReopened;check(centersReopened.open(linkedPath).isEmpty() && centersReopened.authored()==afterCenterEdit,"Center references lost on reopen");
    std::cout<<"PASS: actual bracket planar end-cap reference, immutable local sketch, width/radius/depth transport, nested extrusion and atomic storage\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
