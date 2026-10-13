// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <BRepTools.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <sstream>
#include <fstream>
#include <iostream>
namespace {
void check(bool yes,const QString& message){if(!yes)throw std::runtime_error(message.toStdString());}
std::string serialized(const TopoDS_Shape& shape){std::ostringstream stream;BRepTools::Write(shape,stream);return stream.str();}
QStringList lineage(const EvaluatedBody& body){QStringList ids;for(const auto& face:body.supportFaces)ids.append(face.faceId);for(const auto& edge:body.solidEdges)ids.append(edge.edgeId);return ids;}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
 CadDocument d;
 for(int x:{0,12,24}){auto sketch=SketchGeometry::newFeature(d.activeOwnerId(),"XY","Round participant");sketch=SketchGeometry::addCircle(sketch,{{"centerUMm",x},{"centerVMm",0},{"diameterMm",8}});auto error=d.commit(SketchGeometry::replaceFeature(d.authored(),sketch),"Circle");check(error.isEmpty(),error);error=d.commit(CadDocument::extrusionCandidate(d.authored(),{{"sketchId",sketch["id"]},{"profileId",d.sketches().back().profiles[0].profileId},{"distanceMm",10}}),"Participant");check(error.isEmpty(),error);}
 auto sketch=SketchGeometry::newFeature(d.activeOwnerId(),"XY","Connecting material");sketch=SketchGeometry::addRectangle(sketch,{{"u1Mm",-2},{"v1Mm",-2},{"u2Mm",26},{"v2Mm",2}});check(d.commit(SketchGeometry::replaceFeature(d.authored(),sketch),"Bridge").isEmpty(),"bridge sketch");
 auto participants=d.bodies();for(auto& body:participants)body.shape.Free(false); // Retained inputs are already owned; OCCT compound insertion clears this container flag.
 const auto profiles=d.sketches().back().profiles;check(participants.size()==3&&profiles.size()==1,"three curved participants and one bridge profile");const auto authored=d.authored();std::vector<std::string> originals;QJsonArray targets;for(const auto& body:participants){originals.push_back(serialized(body.shape));targets.append(body.bodyId);}const auto profileBefore=serialized(profiles[0].face);
 QJsonObject feature{{"id",CadDocument::newId()},{"name","Multi-party Join"},{"visible",true},{"parameters",QJsonObject{{"sketchId",sketch["id"]},{"distanceMm",5},{"targetBodyIds",targets}}}};
 QStringList priorLineage;QJsonArray ownership;double priorVolume=0;
 for(int repeat=0;repeat<2;++repeat){const auto result=GeometryKernel::joinResults(feature,participants,profiles);check(result.bodies.size()==1,"one connected multi-party result");const auto& body=result.bodies[0];check(BRepCheck_Analyzer(body.shape).IsValid()&&body.summary["closed"].toBool()&&body.summary["volumeMm3"].toDouble()>0,"actual Join output valid closed positive solid");check(body.bodyId==participants[0].bodyId&&body.featureId==participants[0].featureId,"first participant keeps body and producer ownership");check(result.ownership[0].toObject()["participantBodyIds"].toArray()==targets,"all three actual participants map to retained output");check(!body.supportFaces.empty()&&!body.solidEdges.empty(),"actual retained face and edge lineage exists");if(repeat){check(result.ownership==ownership&&lineage(body)==priorLineage,"repeat ownership and named lineage exact");check(body.summary["volumeMm3"]==priorVolume,"repeat actual volume exact");}ownership=result.ownership;priorLineage=lineage(body);priorVolume=body.summary["volumeMm3"].toDouble();feature["bodyResults"]=ownership;
  for(size_t i=0;i<participants.size();++i){const auto now=serialized(participants[i].shape);if(now!=originals[i]){const auto folder=qEnvironmentVariable("ALLOY_JOIN_EVIDENCE");if(!folder.isEmpty()){std::ofstream(folder.toStdString()+"/participant-before.brep")<<originals[i];std::ofstream(folder.toStdString()+"/participant-after.brep")<<now;}}check(now==originals[i]&&BRepCheck_Analyzer(participants[i].shape).IsValid(),"lineage Common leaves serialized participant BRep unchanged");}check(serialized(profiles[0].face)==profileBefore&&d.authored()==authored,"profile input and authoritative authored state unchanged");}
 std::cout<<"PASS: multi-party Join input BReps immutable, repeated ownership/lineage/validity retained\n";return 0;
 }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
