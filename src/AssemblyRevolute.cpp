// SPDX-License-Identifier: MIT
#include "AssemblyRevolute.h"
#include "AssemblySolver.h"
#include "LocalReference.h"
#include <BRepAdaptor_Curve.hxx>
#include <GeomAbs_CurveType.hxx>
#include <gp_Trsf.hxx>
#include <QSet>
#include <QUuid>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {
void require(bool condition,const char* message){if(!condition)throw std::invalid_argument(message);}
QString definitionFor(const QJsonObject& authored,const QString& id){
    for(const auto& value:authored.value("occurrences").toArray())if(value.toObject()["id"]==id)
        return value.toObject()["definitionId"].toString();
    return {};
}
QString presentedOwner(const QJsonObject& authored,const EvaluatedBody& body){
    return authored.value("objectOwners").toObject().value("bodies").toObject()
        .value(body.bodyId).toString(body.ownerId);
}
bool rootLeaf(const QJsonObject& authored,const QString& occurrenceId){
    const auto root=authored["documentId"].toString();QString definition;
    for(const auto& value:authored.value("occurrences").toArray()){
        const auto row=value.toObject();if(row["id"]==occurrenceId && row["parentId"]==root
            && row["placement"]=="identity")definition=row["definitionId"].toString();
    }
    if(definition.isEmpty())return false;
    for(const auto& value:authored.value("occurrences").toArray())
        if(value.toObject()["parentId"]==definition)return false;
    return true;
}
QJsonArray matrixOf(const gp_Pnt& center){
    // X, -Z, +Y is a right-handed frame for this declared Y-axis fixture.
    return {1,0,0,center.X(),0,0,1,center.Y(),0,-1,0,center.Z()};
}
QJsonArray circularFrame(const QJsonObject& authored,const EvaluatedDocument& definitions,const QJsonObject& record){
    const auto fixed=definitionFor(authored,record["component2OccurrenceId"].toString());
    const auto ref=record["position"].toObject();
    for(const auto& body:definitions.bodies)if(body.bodyId==ref["bodyId"].toString()){
        require(presentedOwner(authored,body)==fixed,"Revolute Position edge must belong to the fixed component");
        for(const auto& edge:body.solidEdges)if(edge.edgeId==ref["edgeId"].toString()){
            require(!edge.edge.IsNull(),"Revolute Position edge is unavailable");
            const BRepAdaptor_Curve curve(edge.edge);
            require(curve.GetType()==GeomAbs_Circle,"Revolute Position requires a circular retained edge");
            require(std::abs(std::abs(curve.LastParameter()-curve.FirstParameter())-2*std::acos(-1.0))<1e-6,
                "Revolute Position requires one complete circle, not a partial arc");
            const auto circle=curve.Circle();
            require(circle.Radius()>1e-6 && std::abs(circle.Axis().Direction().Y())>1-1e-6,
                "This Revolute slice requires a Y-axis circular origin");
            return matrixOf(circle.Location());
        }
    }
    throw std::invalid_argument("Revolute Position edge is unavailable");
}
AlloyAssemblyPose pose(const QJsonArray& values){
    require(values.size()==12,"Revolute marker frame requires twelve coordinates");
    AlloyAssemblyPose result{};
    for(int row=0;row<3;++row){
        for(int col=0;col<3;++col)result.rotation[row*3+col]=values[row*4+col].toDouble();
        result.translationMm[row]=values[row*4+3].toDouble();
    }
    return result;
}
QJsonArray poseArray(const AlloyAssemblyPose& p){
    return {p.rotation[0],p.rotation[1],p.rotation[2],p.translationMm[0],
        p.rotation[3],p.rotation[4],p.rotation[5],p.translationMm[1],
        p.rotation[6],p.rotation[7],p.rotation[8],p.translationMm[2]};
}
gp_Trsf transform(const AlloyAssemblyPose& p){
    gp_Trsf result;result.SetValues(p.rotation[0],p.rotation[1],p.rotation[2],p.translationMm[0],
        p.rotation[3],p.rotation[4],p.rotation[5],p.translationMm[1],
        p.rotation[6],p.rotation[7],p.rotation[8],p.translationMm[2]);
    return result;
}
struct Solution {AlloyAssemblyPose moving{};QJsonArray fixedFrame;bool authoredPin=false;double angle=0;AlloyAssemblyReport report{};};
std::vector<EvaluatedBody> placedBodies(const QJsonObject& authored,const EvaluatedDocument& definitions,
    const AlloyAssemblyPose& moving){
    auto bodies=definitions.bodies;const auto joint=authored["joints"].toArray()[0].toObject();
    const auto owner=definitionFor(authored,joint["component1OccurrenceId"].toString());
    bool unchanged=true;
    for(int i=0;i<3;++i)unchanged &= std::abs(moving.translationMm[i])<1e-10;
    for(int i=0;i<9;++i)unchanged &= std::abs(moving.rotation[i]-(i%4==0 ? 1.0:0.0))<1e-10;
    if(unchanged)return bodies;
    const auto placement=transform(moving);
    for(auto& body:bodies)if(presentedOwner(authored,body)==owner)body=LocalReference::placed(std::move(body),placement);
    return bodies;
}
Solution solve(const QJsonObject& authored,const EvaluatedDocument& definitions,std::optional<double> transientAngle){
    const auto rows=authored.value("joints").toArray();require(rows.size()==1,"Revolute joint is unavailable");
    const auto joint=rows[0].toObject();require(joint["type"]=="asBuiltRevolute","Expected an as-built Revolute joint");
    const auto movingId=joint["component1OccurrenceId"].toString(),fixedId=joint["component2OccurrenceId"].toString();
    require(movingId!=fixedId && rootLeaf(authored,movingId) && rootLeaf(authored,fixedId),
        "Revolute participants must be distinct root-level leaf components");
    Solution resolved;resolved.fixedFrame=circularFrame(authored,definitions,joint);
    resolved.angle=transientAngle.value_or(joint["angleDeg"].toDouble());
    require(std::isfinite(resolved.angle) && std::abs(resolved.angle)<=180,
        "Revolute angle must be finite within ±180 degrees");
    for(const auto& value:authored.value("features").toArray())if(value.toObject()["type"]=="assemblyPin"
        && std::find(definitions.featureOrder.begin(),definitions.featureOrder.end(),value.toObject()["id"].toString())!=definitions.featureOrder.end()){
        const auto parameters=value.toObject()["parameters"].toObject();
        require(parameters["occurrenceId"]==fixedId,"This Revolute slice requires Pin on the fixed component");
        resolved.authoredPin=true;
    }
    const auto identity=pose(QJsonArray{1,0,0,0,0,1,0,0,0,0,1,0});
    const auto fixedName=fixedId.toUtf8(),movingName=movingId.toUtf8(),jointName=joint["id"].toString().toUtf8();
    AlloyAssemblyPart parts[2]{{fixedName.constData(),identity,1},{movingName.constData(),identity,0}};
    AlloyAssemblyJoint relationship{jointName.constData(),fixedName.constData(),movingName.constData(),
        ALLOY_ASSEMBLY_REVOLUTE,pose(resolved.fixedFrame),pose(joint["movingMarkerLocal"].toArray())};
    AlloyAssemblyDrive drive{jointName.constData(),resolved.angle*std::acos(-1.0)/180.0};
    AlloyAssemblyPose outputs[2]{};
    const auto status=alloy_assembly_solver_solve(parts,2,&relationship,1,&drive,outputs,&resolved.report);
    require(status==ALLOY_ASSEMBLY_SUCCESS,"Revolute solver could not satisfy the retained joint, Pin and angle");
    resolved.moving=outputs[1];return resolved;
}
}
QJsonObject AssemblyRevolute::candidate(const QJsonObject& authored,const EvaluatedDocument& definitions,
    const QJsonObject& arguments){
    const auto old=authored.value("joints").toArray();const auto edit=arguments["jointId"].toString();
    require(edit.isEmpty() ? old.isEmpty() : old.size()==1 && old[0].toObject()["id"]==edit,
        "This slice supports one available as-built Revolute");
    QJsonObject record=old.isEmpty() ? QJsonObject{{"id",QUuid::createUuid().toString(QUuid::WithoutBraces)},
        {"type","asBuiltRevolute"},{"name","Revolute"},{"angleDeg",0}} : old[0].toObject();
    const QSet<QString> allowed{"jointId","name","component1OccurrenceId","component2OccurrenceId","position","angleDeg"};
    for(auto it=arguments.begin();it!=arguments.end();++it){
        require(allowed.contains(it.key()),"Unknown Revolute argument");if(it.key()!="jointId")record[it.key()]=it.value();
    }
    if(!old.isEmpty())for(const auto& key:{"component1OccurrenceId","component2OccurrenceId","position"})
        require(record[key]==old[0].toObject()[key],"Changing Revolute participants or Position requires a new as-built joint");
    require(rootLeaf(authored,record["component1OccurrenceId"].toString())
        && rootLeaf(authored,record["component2OccurrenceId"].toString()),
        "Revolute requires root-level leaf participants");
    const auto current=circularFrame(authored,definitions,record);
    if(old.isEmpty())record["movingMarkerLocal"]=current;
    auto result=authored;result["joints"]=QJsonArray{record};return result;
}
QJsonObject AssemblyRevolute::inspect(const QJsonObject& authored,const EvaluatedDocument& definitions,
    std::optional<double> transientAngleDeg){
    const auto result=solve(authored,definitions,transientAngleDeg);
    const auto joint=authored["joints"].toArray()[0].toObject();
    const auto moved=placedBodies(authored,definitions,result.moving);
    QJsonArray occurrences;
    for(const auto& value:authored.value("occurrences").toArray()){
        const auto occurrence=value.toObject();QJsonArray bodies;
        for(const auto& body:moved)if(presentedOwner(authored,body)==occurrence["definitionId"].toString())
            bodies.append(QJsonObject{{"bodyId",body.bodyId},{"boundsMm",body.summary["boundsMm"]}});
        occurrences.append(QJsonObject{{"occurrenceId",occurrence["id"]},{"definitionId",occurrence["definitionId"]},
            {"bodies",bodies},{"worldPose",occurrence["id"]==joint["component1OccurrenceId"]
                ? poseArray(result.moving):QJsonArray{1,0,0,0,0,1,0,0,0,0,1,0}}});
    }
    auto resolved=joint;resolved["originFrameWorld"]=result.fixedFrame;
    resolved["coordinateDeg"]=result.angle;resolved["relativeDegreesOfFreedom"]=1;
    resolved["pinAuthored"]=result.authoredPin;resolved["gaugeOccurrenceId"]=joint["component2OccurrenceId"];
    resolved["transient"]=transientAngleDeg.has_value();
    resolved["maxOriginErrorMm"]=result.report.maxOriginErrorMm;
    resolved["maxAxisErrorRadians"]=result.report.maxAxisErrorRadians;
    resolved["driveErrorRadians"]=result.report.driveErrorRadians;
    return {{"joints",QJsonArray{resolved}},{"occurrences",occurrences}};
}
std::vector<EvaluatedBody> AssemblyRevolute::worldBodies(const QJsonObject& authored,const EvaluatedDocument& definitions,
    std::optional<double> transientAngleDeg){
    const auto result=solve(authored,definitions,transientAngleDeg);
    return placedBodies(authored,definitions,result.moving);
}
