// SPDX-License-Identifier: MIT
#include "AssemblySlider.h"
#include "CadDocument.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
void near(double actual,double expected,const char* message){check(std::abs(actual-expected)<1e-6,message);}
void commit(CadDocument& document,const QJsonObject& candidate){const auto error=document.commit(candidate,"Slider test");check(error.isEmpty(),qPrintable(error));}
QString component(CadDocument& document,const QString& name){
    const auto candidate=CadDocument::componentCandidate(document.authored(),name);
    const auto id=candidate["components"].toArray().last().toObject()["id"].toString();commit(document,candidate);return id;
}
void box(CadDocument& document,const QString& owner,const QString& name,double width,double depth,double height,double x,double y,const QString& depthParameterId={}){
    auto candidate=document.authored();auto features=candidate["features"].toArray();
    QJsonObject dimensions{{"widthMm",width},{"depthMm",depth},{"heightMm",height},{"xMm",x},{"yMm",y},{"zMm",0}};
    if(!depthParameterId.isEmpty())dimensions["depthParameterId"]=depthParameterId;
    features.append(QJsonObject{{"id",CadDocument::newId()},{"bodyId",CadDocument::newId()},{"ownerId",owner},{"type","box"},
        {"name",name},{"visible",true},{"parameters",dimensions}});candidate["features"]=features;commit(document,candidate);
}
QString occurrence(const CadDocument& document,const QString& definition){
    for(const auto& value:document.authored()["occurrences"].toArray())if(value.toObject()["definitionId"]==definition)return value.toObject()["id"].toString();return {};
}
QJsonObject bounds(const EvaluatedBody& body){return body.summary["boundsMm"].toObject();}
double yMin(const EvaluatedBody& body){return bounds(body)["min"].toArray()[1].toDouble();}
}

int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    QTemporaryDir temporary;check(temporary.isValid(),"temporary directory");
    CadDocument document;
    commit(document,CadDocument::lengthParameterCandidate(document.authored(),{{"name","BoxDepth"},{"value",80},{"units","mm"}}));
    const auto depthId=document.authored()["userParameters"].toArray()[0].toObject()["id"].toString();
    const auto boxOwner=component(document,"Box"),lidOwner=component(document,"Lid");
    box(document,boxOwner,"Box stock",50,80,40,0,0,depthId);
    box(document,lidOwner,"Lid stock",50,80,8,0,0);
    const auto restFeatures=document.authored()["features"].toArray();const auto restBodies=document.bodies();check(restBodies.size()==2,"two definitions evaluate");
    const auto fixedBody=restBodies[0],movingBody=restBodies[1];
    check(!fixedBody.solidEdges.empty(),"box has retained edges");
    QJsonObject position;for(const auto& edge:fixedBody.solidEdges){
        const auto start=edge.provenance["startMm"].toArray(),end=edge.provenance["endMm"].toArray();
        if(start.size()==3 && end.size()==3 && std::abs(start[2].toDouble()-40)<1e-6 && std::abs(end[2].toDouble()-40)<1e-6
            && std::abs(start[1].toDouble()-80)<1e-6 && std::abs(end[1].toDouble()-80)<1e-6){
            position={{"bodyId",fixedBody.bodyId},{"edgeId",edge.edgeId}};break;
        }
    }
    check(!position.isEmpty(),"top ledge retained");
    const auto first=occurrence(document,lidOwner),second=occurrence(document,boxOwner);
    commit(document,AssemblySlider::candidate(document.authored(),{{"component1OccurrenceId",first},{"component2OccurrenceId",second},
        {"position",position},{"axis","Y"},{"minimumEnabled",true},{"minimumMm",0},
        {"maximumEnabled",true},{"maximumMm",50},{"positionMm",0}}));
    const auto jointId=document.authored()["joints"].toArray()[0].toObject()["id"].toString();
    auto resolved=document.assemblyState();const auto joint=resolved["joints"].toArray()[0].toObject();
    check(joint["relativeDegreesOfFreedom"]==1 && joint["gaugeOccurrenceId"]==second,"one DOF and Box gauge");
    check(joint["origin"].toObject()["edgeId"]==position["edgeId"],"Position retains selected edge");
    const auto edgePoint=joint["origin"].toObject()["pointMm"].toArray();near(edgePoint[2].toDouble(),40,"joint origin on top ledge");
    commit(document,AssemblySlider::candidate(document.authored(),{{"jointId",jointId},{"positionMm",30}}));
    check(document.bodies()[1].shape.IsSame(movingBody.shape),"joint drive reuses definition BRep");
    check(document.authored()["features"].toArray()==restFeatures,"joint drive does not edit definition features");
    near(yMin(document.bodies()[1]),0,"definition bounds remain at rest");
    const auto world=document.worldBodies();near(yMin(world[1]),30,"moving occurrence world bounds translated");
    near(yMin(world[0]),0,"reference occurrence stays fixed");
    check(!world[1].shape.IsSame(document.bodies()[1].shape),"world BRep is a located copy");
    near(AssemblySlider::inspect(document.authored(),document.evaluation(),50)["joints"].toArray()[0].toObject()["coordinateMm"].toDouble(),50,"transient upper endpoint");
    near(yMin(document.worldBodies(50)[1]),50,"transient world pose at upper endpoint");
    check(document.authored()["joints"].toArray()[0].toObject()["positionMm"].toDouble()==30,"transient does not edit authored coordinate");
    const auto stable=document.authored();const auto revision=document.revision();
    check(!document.commit(AssemblySlider::candidate(stable,{{"jointId",jointId},{"positionMm",51}}),"Outside limit").isEmpty(),"limit refusal");
    check(document.authored()==stable && document.revision()==revision,"limit refusal is atomic");
    auto invalid=AssemblySlider::candidate(stable,{{"jointId",jointId},{"position",QJsonObject{{"bodyId",movingBody.bodyId},{"edgeId",position["edgeId"]}}}});
    check(!document.commit(invalid,"Wrong edge owner").isEmpty() && document.authored()==stable,"wrong-owner edge refusal is atomic");
    document.history().undo();near(document.assemblyState()["joints"].toArray()[0].toObject()["coordinateMm"].toDouble(),0,"drive undo");
    document.history().redo();check(document.authored()==stable,"drive redo exact");
    const auto path=temporary.filePath("slider.alloy");check(document.save(path).isEmpty(),"save");
    CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==stable,"reopen retains joint IDs/limits/pose");
    near(yMin(reopened.worldBodies()[1]),30,"reopen world pose");
    commit(reopened,CadDocument::lengthParameterCandidate(reopened.authored(),{{"parameterId",depthId},{"value",90}}));
    near(reopened.assemblyState()["joints"].toArray()[0].toObject()["origin"].toObject()["pointMm"].toArray()[1].toDouble(),90,"source depth moves retained ledge midpoint in rest context");
    near(yMin(reopened.worldBodies()[1]),30,"source recompute reapplies one occurrence translation");
    commit(reopened,AssemblySlider::candidate(reopened.authored(),{{"jointId",jointId},{"reverseDirection",true}}));
    near(reopened.assemblyState()["joints"].toArray()[0].toObject()["coordinateMm"].toDouble(),30,"reverse retains positive coordinate");
    near(yMin(reopened.worldBodies()[1]),-30,"reverse moves moving occurrence toward negative Y");
    near(yMin(reopened.worldBodies(50)[1]),-50,"reverse preview at positive maximum clears front");
    near(yMin(reopened.worldBodies()[0]),0,"reverse keeps fixed occurrence still");
    near(yMin(reopened.bodies()[1]),0,"reverse keeps definition at rest");
    auto reverseStable=reopened.authored();
    auto badReverse=AssemblySlider::candidate(reverseStable,{{"jointId",jointId},{"reverseDirection",1}});
    check(!reopened.commit(badReverse,"Invalid reverse flag").isEmpty() && reopened.authored()==reverseStable,"nonboolean reverse refused atomically");
    reopened.history().undo();near(yMin(reopened.worldBodies()[1]),30,"reverse undo restores +Y");
    reopened.history().redo();check(reopened.authored()==reverseStable,"reverse redo exact");
    check(reopened.save(path).isEmpty(),"save reversed Slider");
    CadDocument reverseOpen;check(reverseOpen.open(path).isEmpty() && reverseOpen.authored()==reverseStable,"reopen reverse retains authored identity");
    near(yMin(reverseOpen.worldBodies()[1]),-30,"reopen reversed world pose");
    auto legacy=reverseStable;auto joints=legacy["joints"].toArray();auto oldJoint=joints[0].toObject();oldJoint.remove("reverseDirection");
    joints[0]=oldJoint;legacy["joints"]=joints;commit(reverseOpen,legacy);
    near(yMin(reverseOpen.worldBodies()[1]),30,"legacy joint without direction retains +Y");
    check(reverseOpen.save(path).isEmpty(),"save legacy Slider");
    CadDocument legacyOpen;check(legacyOpen.open(path).isEmpty() && legacyOpen.authored()==legacy,"legacy saved joint reopens exactly");
    near(yMin(legacyOpen.worldBodies()[1]),30,"legacy reopened world pose");
    std::cout<<"PASS: as-built Slider retains ledge, one DOF, direction, definition/rest separation, limits, undo and reopen\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
