// SPDX-License-Identifier: MIT
#include "AssemblyRevolute.h"
#include "AssemblyGraph.h"
namespace {
QJsonObject legacyOverride(const QJsonObject& authored,std::optional<double> angle){
 if(!angle)return {};for(const auto& value:authored["joints"].toArray())if(value.toObject()["type"]=="asBuiltRevolute")return {{"jointId",value.toObject()["id"]},{"angleDeg",*angle}};return {};
}
}
QJsonObject AssemblyRevolute::candidate(const QJsonObject& a,const EvaluatedDocument& d,const QJsonObject& args){return AssemblyGraph::revoluteCandidate(a,d,args);}
QJsonObject AssemblyRevolute::inspect(const QJsonObject& a,const EvaluatedDocument& d,std::optional<double> angle){return AssemblyGraph::inspect(a,d,legacyOverride(a,angle));}
std::vector<EvaluatedBody> AssemblyRevolute::worldBodies(const QJsonObject& a,const EvaluatedDocument& d,std::optional<double> angle){return AssemblyGraph::worldBodies(a,d,legacyOverride(a,angle));}
