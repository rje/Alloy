// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QJsonDocument>
#include <iostream>
#include <stdexcept>
namespace {
void check(bool b,const QString& why){if(!b)throw std::runtime_error(why.toStdString());}
QJsonObject p(const QJsonObject& f){return f.value("parameters").toObject();}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try {
 CadDocument doc;auto f=SketchGeometry::newConstructionPlaneFeature(doc.activeOwnerId(),CadDocument::newId(),"Projection Delete");
 const auto line=CadDocument::newId(),circle=CadDocument::newId(),point=CadDocument::newId();
 const auto source=[] {return QJsonObject{{"bodyId",CadDocument::newId()},{"edgeId",CadDocument::newId()},{"occurrenceId",CadDocument::newId()}};};
 const auto ls=source(),cs=source(),ps=source();auto params=p(f);
 params["projections"]=QJsonArray{QJsonObject{{"id",line},{"source",ls},{"profile",true}}};
 params["circleProjections"]=QJsonArray{QJsonObject{{"id",circle},{"source",cs},{"profile",true}}};
 params["pointProjections"]=QJsonArray{QJsonObject{{"id",point},{"source",ps}}};f["parameters"]=params;
 const QJsonObject context{{"lines",QJsonArray{QJsonObject{{"id",line},{"source",ls},{"u1Mm",0},{"v1Mm",0},{"u2Mm",20},{"v2Mm",0}}}},
  {"circles",QJsonArray{QJsonObject{{"id",circle},{"source",cs},{"uMm",30},{"vMm",20},{"radiusMm",5}}}},
  {"points",QJsonArray{QJsonObject{{"id",point},{"source",ps},{"uMm",40},{"vMm",30}}}}};
 auto solved=SketchGeometry::solveDraft(f,context);check(solved.ok,solved.error);f=solved.feature;
 for(const auto& target:QJsonArray{line,circle,point}) {
  const auto id=target.toString();const bool isPoint=id==point;const auto result=SketchGeometry::deleteEntities(f,isPoint ? QJsonArray{} : QJsonArray{id},isPoint ? QJsonArray{id} : QJsonArray{},context);
  check(result.ok,result.error);auto expected=p(f);expected.remove(isPoint ? "pointProjections" : id==line ? "projections" : "circleProjections");
  if(p(result.feature)!=expected)std::cerr<<QJsonDocument(expected).toJson().constData()<<"ACTUAL\n"<<QJsonDocument(p(result.feature)).toJson().constData();
  check(p(result.feature)==expected,"projection Delete changed unrelated authored parameters");
  check(result.diagnostics.value("removedProjectionIds")==QJsonArray{id},"missing projection owner receipt");
 }
 auto atomic=[&](const QJsonObject& input,const QJsonArray& e,const QJsonArray& pts={}) {const auto result=SketchGeometry::deleteEntities(input,e,pts,context);check(!result.ok && result.feature==input,"unsafe request was not atomic");};
 const auto unavailable=SketchGeometry::deleteEntities(f,{line});check(!unavailable.ok && unavailable.feature==f,"missing live source context must refuse atomically");
 atomic(f,{line,"stale"});atomic(f,{line,line});atomic(f,{},QJsonArray{point,point});atomic(f,{point});atomic(f,{},QJsonArray{line});
 // Direct geometric ownership cannot be cascaded like an annotation.
 const auto dependentOn=[&](const QString& id) {
  auto candidate=SketchGeometry::addLine(f,{{"u1Mm",0},{"v1Mm",0},{"u2Mm",3},{"v2Mm",8}});
  auto values=p(candidate);auto entities=values.value("entities").toArray();auto entity=entities.last().toObject();const auto old=entity.value("startPointId");QJsonArray points;for(const auto& row:values.value("points").toArray())if(row.toObject().value("id")!=old)points.append(row);values["points"]=points;entity["startPointId"]=id;entities[entities.size()-1]=entity;values["entities"]=entities;candidate["parameters"]=values;
  const auto solved=SketchGeometry::solveDraft(candidate,context);check(solved.ok,solved.error);return solved.feature;
 };
 atomic(dependentOn(SketchGeometry::projectionPointId(line,false)),{line});
 atomic(dependentOn(point),{},QJsonArray{point});
 // Direct read-only measurements are annotations and retire with their target.
 auto measured=SketchGeometry::addDimension(f,{{"kind","diameter"},{"entityId",circle},{"driving",false}},context);check(measured.ok,measured.error);
 auto removed=SketchGeometry::deleteEntities(measured.feature,{circle},{},context);check(removed.ok,removed.error);check(p(removed.feature).value("dimensions").toArray().isEmpty(),"attached measurement survived");
 // A retained owner referring to a projection must refuse before removal.
 auto offset=SketchGeometry::addCircleOffset(f,{{"sourceEntityId",circle},{"distanceMm",2}},context);
 check(offset.ok,offset.error);atomic(offset.feature,{circle});
 // Explicit face actions retain selected-edge policy, even for a shared result.
 auto grouped=f;auto gp=p(grouped);auto sharedSource=cs;sharedSource["bodyId"]=ls.value("bodyId");
 auto circleRows=gp.value("circleProjections").toArray();auto circleRow=circleRows[0].toObject();circleRow["source"]=sharedSource;circleRows[0]=circleRow;gp["circleProjections"]=circleRows;
 const auto faceSource=[&]{return QJsonObject{{"producerFeatureId",CadDocument::newId()},{"bodyId",ls.value("bodyId")},{"faceId",CadDocument::newId()}};};
 const QJsonObject groupA{{"id",CadDocument::newId()},{"source",faceSource()},{"projectionIds",QJsonArray{line,circle}}};
 const QJsonObject groupB{{"id",CadDocument::newId()},{"source",faceSource()},{"projectionIds",QJsonArray{line}}};
 gp["faceProjectionGroups"]=QJsonArray{groupA,groupB};grouped["parameters"]=gp;
 auto sharedContext=context;auto resolved=sharedContext.value("circles").toArray();auto resolvedCircle=resolved[0].toObject();resolvedCircle["source"]=sharedSource;resolved[0]=resolvedCircle;sharedContext["circles"]=resolved;
 auto groupSolve=SketchGeometry::solveDraft(grouped,sharedContext);check(groupSolve.ok,groupSolve.error);grouped=groupSolve.feature;
 auto groupedDependency=dependentOn(SketchGeometry::projectionPointId(line,false));auto dependentParams=p(groupedDependency);dependentParams["circleProjections"]=gp.value("circleProjections");dependentParams["faceProjectionGroups"]=gp.value("faceProjectionGroups");groupedDependency["parameters"]=dependentParams;
 auto dependencySolve=SketchGeometry::solveDraft(groupedDependency,sharedContext);check(dependencySolve.ok,dependencySolve.error);groupedDependency=dependencySolve.feature;
 const auto dependentRefusal=SketchGeometry::deleteEntities(groupedDependency,{line},{},sharedContext);check(!dependentRefusal.ok && dependentRefusal.feature==groupedDependency,"face-member virtual-point dependency must refuse atomically");
 auto edgeRemoved=SketchGeometry::deleteEntities(grouped,{line},{},sharedContext);check(edgeRemoved.ok,edgeRemoved.error);
 auto expectedGroup=groupA;expectedGroup["projectionIds"]=QJsonArray{circle};auto expectedParams=p(grouped);expectedParams.remove("projections");expectedParams["faceProjectionGroups"]=QJsonArray{expectedGroup};
 check(p(edgeRemoved.feature)==expectedParams,"selected-edge Delete did not preserve sibling/source or retire shared empty membership");
 auto malformed=grouped;auto bad=p(malformed);auto invalidGroup=groupA;invalidGroup["projectionIds"]=QJsonArray{line,line};bad["faceProjectionGroups"]=QJsonArray{invalidGroup};malformed["parameters"]=bad;
 auto invalid=SketchGeometry::deleteEntities(malformed,{line},{},sharedContext);check(!invalid.ok && invalid.feature==malformed,"duplicate face membership must refuse atomically");
 invalidGroup["projectionIds"]=QJsonArray{CadDocument::newId()};bad["faceProjectionGroups"]=QJsonArray{invalidGroup};malformed["parameters"]=bad;
 invalid=SketchGeometry::deleteEntities(malformed,{line},{},sharedContext);check(!invalid.ok && invalid.feature==malformed,"stale face membership must refuse atomically");
 std::cout<<"PASS: linked line/circle/point Delete, exact survivors, annotation cleanup and atomic refusals\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
