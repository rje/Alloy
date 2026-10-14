// SPDX-License-Identifier: MIT
#include "AssemblyGraph.h"
#include "AssemblySolver.h"
#include "LocalReference.h"
#include <BRepAdaptor_Curve.hxx>
#include <GeomAbs_CurveType.hxx>
#include <QUuid>
#include <QSet>
#include <gp_Trsf.hxx>
#include <algorithm>
#include <cmath>
#include <map>
#include <stdexcept>
namespace {
constexpr double pi=3.14159265358979323846;
void require(bool value,const char* text){if(!value)throw std::invalid_argument(text);}
bool uuid(const QString& s){return !QUuid(s).isNull();}
QJsonArray identity(){return {1,0,0,0,0,1,0,0,0,0,1,0};}
QString definition(const QJsonObject& a,const QString& id){for(const auto& v:a["occurrences"].toArray())if(v.toObject()["id"]==id)return v.toObject()["definitionId"].toString();return {};}
QString owner(const QJsonObject& a,const EvaluatedBody& b){return a["objectOwners"].toObject()["bodies"].toObject().value(b.bodyId).toString(b.ownerId);}
AlloyAssemblyPose pose(const QJsonArray& a){require(a.size()==12,"Joint marker needs twelve coordinates");AlloyAssemblyPose p{};for(int r=0;r<3;++r){for(int c=0;c<3;++c)p.rotation[3*r+c]=a[4*r+c].toDouble();p.translationMm[r]=a[4*r+3].toDouble();}return p;}
QJsonArray array(const AlloyAssemblyPose& p){return {p.rotation[0],p.rotation[1],p.rotation[2],p.translationMm[0],p.rotation[3],p.rotation[4],p.rotation[5],p.translationMm[1],p.rotation[6],p.rotation[7],p.rotation[8],p.translationMm[2]};}
bool atRest(const AlloyAssemblyPose& p){for(double value:p.translationMm)if(std::abs(value)>1e-10)return false;for(int i=0;i<9;++i)if(std::abs(p.rotation[i]-(i%4==0?1.:0.))>1e-10)return false;return true;}
gp_Trsf trsf(const AlloyAssemblyPose& p){gp_Trsf t;t.SetValues(p.rotation[0],p.rotation[1],p.rotation[2],p.translationMm[0],p.rotation[3],p.rotation[4],p.rotation[5],p.translationMm[1],p.rotation[6],p.rotation[7],p.rotation[8],p.translationMm[2]);return t;}
struct Circle {gp_Pnt center;QJsonObject ref;};
Circle circle(const QJsonObject& a,const EvaluatedDocument& d,const QString& occurrence,const QJsonObject& ref,bool cylinder){
 const auto own=definition(a,occurrence);
 for(const auto& b:d.bodies)if(b.bodyId==ref["bodyId"].toString()){
  require(owner(a,b)==own,"Joint origin must belong to the selected leaf occurrence");
  for(const auto& e:b.solidEdges)if(e.edgeId==ref["edgeId"].toString()){
   require(!e.edge.IsNull(),"Retained circular origin unavailable");BRepAdaptor_Curve c(e.edge);
   require(c.GetType()==GeomAbs_Circle&&std::abs(std::abs(c.LastParameter()-c.FirstParameter())-2*pi)<1e-6,"Joint origin needs one complete retained circle");
   const auto value=c.Circle();const auto direction=value.Axis().Direction();
   require(value.Radius()>1e-6 && std::abs(cylinder?direction.Z():direction.Y())>1-1e-6,"This fixture requires a Y pin circle or Z tube circle");return {value.Location(),ref};
  }
 }
 throw std::invalid_argument("Retained circular origin unavailable");
}
QJsonArray frame(gp_Pnt p,bool cylinder){return cylinder?QJsonArray{1,0,0,p.X(),0,1,0,p.Y(),0,0,1,p.Z()}:QJsonArray{1,0,0,p.X(),0,0,1,p.Y(),0,-1,0,p.Z()};}
gp_Pnt framePoint(const QJsonArray& f){return {f[3].toDouble(),f[7].toDouble(),f[11].toDouble()};}
bool sameAxis(gp_Pnt a,gp_Pnt b,bool cylinder){return cylinder?std::hypot(a.X()-b.X(),a.Y()-b.Y())<1e-5:std::hypot(a.X()-b.X(),a.Z()-b.Z())<1e-5;}
Circle counterpart(const QJsonObject& a,const EvaluatedDocument& d,const QString& occurrence,gp_Pnt fixed,bool cylinder){
 std::vector<Circle> matches;
 for(const auto& b:d.bodies)if(owner(a,b)==definition(a,occurrence))for(const auto& e:b.solidEdges){try{QJsonObject ref{{"bodyId",b.bodyId},{"edgeId",e.edgeId}};auto c=circle(a,d,occurrence,ref,cylinder);if(sameAxis(c.center,fixed,cylinder))matches.push_back(c);}catch(const std::exception&){} }
 require(!matches.empty(),"Moving leaf has no compatible current circular hole/tube axis");
 // Multiple circular rims on one infinite axis are equivalent contacts; retain
 // one explicit rim deterministically. Different axes are never rebound.
 std::sort(matches.begin(),matches.end(),[](const auto& x,const auto& y){return x.ref["edgeId"].toString()<y.ref["edgeId"].toString();});return matches.front();
}
QJsonArray movingFrame(const QJsonObject& a,const EvaluatedDocument& d,const QJsonObject& j,bool cylinder){
 if(j.contains("movingPosition")){auto c=circle(a,d,j["component1OccurrenceId"].toString(),j["movingPosition"].toObject(),cylinder);auto point=c.center;if(cylinder)point.SetZ(point.Z()+j["movingMarkerOffsetMm"].toDouble());else point.SetY(point.Y()+j["movingMarkerOffsetMm"].toDouble());return frame(point,cylinder);}
 const auto old=j["movingMarkerLocal"].toArray();counterpart(a,d,j["component1OccurrenceId"].toString(),framePoint(old),cylinder);return old;
}
QString driverFor(const QJsonArray& rows,const QString& jointId){QString start;for(const auto& v:rows)if(v.toObject()["id"]==jointId)start=v.toObject()["component1OccurrenceId"].toString();QSet<QString> members{start};bool changed=true;while(changed){changed=false;for(const auto& v:rows){const auto j=v.toObject();const auto x=j["component1OccurrenceId"].toString(),y=j["component2OccurrenceId"].toString();if(members.contains(x)||members.contains(y)){const auto old=members.size();members.insert(x);members.insert(y);changed|=members.size()!=old;}}}for(const auto& v:rows)if(members.contains(v.toObject()["component1OccurrenceId"].toString()))return v.toObject()["id"].toString();return {};}
struct Result {QMap<QString,AlloyAssemblyPose> poses;QJsonArray joints,islands;};
Result solve(const QJsonObject& a,const EvaluatedDocument& d,const QJsonObject& override){
 const auto error=AssemblyGraph::validateRecords(a);require(error.isEmpty(),qPrintable(error));const auto rows=a["joints"].toArray();Result result;
 for(const auto& v:a["occurrences"].toArray())result.poses[v.toObject()["id"].toString()]=pose(identity());
 if(rows.isEmpty())return result;
 if(!override.isEmpty()){require(override["jointId"].isString(),"Preview requires a joint ID");for(const auto& key:{"angleDeg","rotationDeg","slideMm"})if(override.contains(key))require(override[key].isDouble()&&std::isfinite(override[key].toDouble()),"Preview coordinates must be finite numbers");bool found=false;for(const auto& v:rows)found|=v.toObject()["id"]==override["jointId"];require(found,"Preview joint is unavailable");require(driverFor(rows,override["jointId"].toString())==override["jointId"].toString(),"Only the island driver supports coordinate preview; passive coordinates are measured only");}
 QMap<QString,QSet<QString>> neighbors;
 for(const auto& v:rows){const auto j=v.toObject();const auto x=j["component1OccurrenceId"].toString(),y=j["component2OccurrenceId"].toString();neighbors[x].insert(y);neighbors[y].insert(x);}
 QSet<QString> visited;QMap<QString,QJsonObject> inspected;
 for(const auto& value:rows){const auto first=value.toObject();const auto start=first["component2OccurrenceId"].toString();if(visited.contains(start))continue;
  QSet<QString> members;QStringList pending{start};while(!pending.isEmpty()){const auto id=pending.takeLast();if(members.contains(id))continue;members.insert(id);for(const auto& n:neighbors[id])pending.push_back(n);}visited.unite(members);
  QJsonArray islandRows;for(const auto& v:rows)if(members.contains(v.toObject()["component1OccurrenceId"].toString()))islandRows.append(v);
  QString gauge=islandRows[0].toObject()["component2OccurrenceId"].toString();bool pinAuthored=false;QJsonArray pinPose=identity();
  for(const auto& v:a["features"].toArray()){const auto f=v.toObject();if(f["type"]!="assemblyPin"||std::find(d.featureOrder.begin(),d.featureOrder.end(),f["id"].toString())==d.featureOrder.end())continue;const auto p=f["parameters"].toObject();if(members.contains(p["occurrenceId"].toString())){require(!pinAuthored,"One Pin per island is supported");gauge=p["occurrenceId"].toString();pinPose=p["worldPose"].toArray();pinAuthored=true;}}
  auto memberList=members.values();std::sort(memberList.begin(),memberList.end());std::vector<QByteArray> names;names.reserve(memberList.size()+islandRows.size());for(const auto& id:memberList)names.push_back(id.toUtf8());for(const auto& v:islandRows)names.push_back(v.toObject()["id"].toString().toUtf8());
  std::vector<AlloyAssemblyPart> parts;for(int i=0;i<memberList.size();++i)parts.push_back({names[i].constData(),pose(memberList[i]==gauge?pinPose:identity()),memberList[i]==gauge});
  std::vector<AlloyAssemblyJoint> relationships;QMap<QString,std::pair<QJsonArray,QJsonArray>> frames;
  for(int i=0;i<islandRows.size();++i){const auto j=islandRows[i].toObject();const bool cylinder=j["type"]=="asBuiltCylindrical";const auto fixed=frame(circle(a,d,j["component2OccurrenceId"].toString(),j["position"].toObject(),cylinder).center,cylinder);const auto moving=movingFrame(a,d,j,cylinder);frames[j["id"].toString()]={fixed,moving};const auto fixedIndex=memberList.indexOf(j["component2OccurrenceId"].toString()),movingIndex=memberList.indexOf(j["component1OccurrenceId"].toString());relationships.push_back({names[memberList.size()+i].constData(),names[fixedIndex].constData(),names[movingIndex].constData(),cylinder?ALLOY_ASSEMBLY_CYLINDRICAL:ALLOY_ASSEMBLY_REVOLUTE,pose(fixed),pose(moving)});}
  auto driver=islandRows[0].toObject();QJsonObject targets=driver;
  for(const auto& v:islandRows)if(v.toObject()["id"]==override["jointId"]){driver=v.toObject();targets=driver;for(auto it=override.begin();it!=override.end();++it)targets[it.key()]=it.value();targets["type"]=driver["type"];}
  for(const auto& v:islandRows){const auto passive=v.toObject();if(passive["id"]==driver["id"])continue;require(passive["angleDeg"].toDouble()==0&&passive["rotationDeg"].toDouble()==0&&passive["slideMm"].toDouble()==0,"Passive joints retain only zero reference coordinates; edit the island driver instead");}
  const auto driverId=driver["id"].toString();auto driverName=driverId.toUtf8();std::vector<AlloyAssemblyCoordinate> coordinates;
  if(driver["type"]=="asBuiltCylindrical")coordinates={{driverName.constData(),ALLOY_ASSEMBLY_ROTATION_RADIANS,targets["rotationDeg"].toDouble()*pi/180},{driverName.constData(),ALLOY_ASSEMBLY_SLIDE_MM,targets["slideMm"].toDouble()}};
  else coordinates={{driverName.constData(),ALLOY_ASSEMBLY_ROTATION_RADIANS,targets["angleDeg"].toDouble()*pi/180}};
  std::vector<AlloyAssemblyPose> output(parts.size());AlloyAssemblyCoordinateReport report{};const auto status=alloy_assembly_solver_solve_coordinates(parts.data(),parts.size(),relationships.data(),relationships.size(),coordinates.data(),coordinates.size(),output.data(),&report);
  require(status==ALLOY_ASSEMBLY_SUCCESS,report.base.message);for(int i=0;i<memberList.size();++i)result.poses[memberList[i]]=output[i];
  QJsonArray memberIds;for(const auto& id:memberList)memberIds.append(id);result.islands.append(QJsonObject{{"occurrenceIds",memberIds},{"gaugeOccurrenceId",gauge},{"pinAuthored",pinAuthored},{"driverJointId",driverId}});
  for(const auto& v:islandRows){auto j=v.toObject();const auto id=j["id"].toString();const auto worldI=trsf(result.poses[j["component2OccurrenceId"].toString()])*trsf(pose(frames[id].first)),worldJ=trsf(result.poses[j["component1OccurrenceId"].toString()])*trsf(pose(frames[id].second));gp_Vec xI(1,0,0),xJ(1,0,0),zI(0,0,1);xI.Transform(worldI);xJ.Transform(worldJ);zI.Transform(worldI);const gp_Pnt pI=gp_Pnt(0,0,0).Transformed(worldI),pJ=gp_Pnt(0,0,0).Transformed(worldJ);const auto angle=std::atan2(zI.Dot(xI.Crossed(xJ)),xI.Dot(xJ))*180/pi,slide=gp_Vec(pI,pJ).Dot(zI);j["coordinateEditable"]=id==driverId;j["driverJointId"]=driverId;j["coordinateMode"]=id==driverId?"driver":"passive";j["coordinateDeg"]=angle;j["coordinateMm"]=slide;j["originFrameWorld"]=QJsonArray{worldI.Value(1,1),worldI.Value(1,2),worldI.Value(1,3),worldI.Value(1,4),worldI.Value(2,1),worldI.Value(2,2),worldI.Value(2,3),worldI.Value(2,4),worldI.Value(3,1),worldI.Value(3,2),worldI.Value(3,3),worldI.Value(3,4)};j["relativeDegreesOfFreedom"]=j["type"]=="asBuiltCylindrical"?2:1;j["pinAuthored"]=pinAuthored;j["gaugeOccurrenceId"]=gauge;j["transient"]=!override.isEmpty();j["maxOriginErrorMm"]=report.base.maxOriginErrorMm;j["maxAxisErrorRadians"]=report.base.maxAxisErrorRadians;j["driveErrorRadians"]=report.base.driveErrorRadians;j["slideErrorMm"]=report.slideErrorMm;inspected[id]=j;}
 }
 for(const auto& v:rows)result.joints.append(inspected[v.toObject()["id"].toString()]);return result;
}
QJsonObject candidate(const QJsonObject& a,const EvaluatedDocument& d,const QJsonObject& args,bool cylinder){
 require(!a.contains("joints")||a["joints"].toArray().isEmpty()||AssemblyGraph::isJointGraph(a),"Legacy Slider cannot mix with this graph");auto rows=a["joints"].toArray();const auto edit=args["jointId"].toString();int index=-1;for(int i=0;i<rows.size();++i)if(rows[i].toObject()["id"]==edit)index=i;require(edit.isEmpty()||index>=0,"Edited joint is unavailable");
 QJsonObject j=index<0?QJsonObject{{"id",QUuid::createUuid().toString(QUuid::WithoutBraces)},{"type",cylinder?"asBuiltCylindrical":"asBuiltRevolute"},{"name",cylinder?"Cylindrical":"Revolute"}}:rows[index].toObject();require(j["type"]==(cylinder?"asBuiltCylindrical":"asBuiltRevolute"),"Joint type cannot change during edit");
 if(index<0){if(cylinder){j["rotationDeg"]=0;j["slideMm"]=0;}else j["angleDeg"]=0;}
 const QSet<QString> allowed=cylinder?QSet<QString>{"jointId","name","component1OccurrenceId","component2OccurrenceId","position","rotationDeg","slideMm"}:QSet<QString>{"jointId","name","component1OccurrenceId","component2OccurrenceId","position","angleDeg"};
 for(auto it=args.begin();it!=args.end();++it){require(allowed.contains(it.key()),"Unknown joint argument");if(it.key()!="jointId")j[it.key()]=it.value();}
 if(index>=0&&driverFor(rows,j["id"].toString())!=j["id"].toString())for(const auto& key:{"angleDeg","rotationDeg","slideMm"})if(args.contains(key))require(args[key]==rows[index].toObject()[key],"Only the island driver supports coordinate edits; passive coordinates are measured only");
 if(index>=0)for(const auto& k:{"component1OccurrenceId","component2OccurrenceId","position"})require(j[k]==rows[index].toObject()[k],"Changing participants or Position requires a new as-built joint");
 require(AssemblyGraph::identityLeaf(a,j["component1OccurrenceId"].toString())&&AssemblyGraph::identityLeaf(a,j["component2OccurrenceId"].toString()),"Joint participants must be current identity leaf occurrences");
 if(index<0){const auto fixed=circle(a,d,j["component2OccurrenceId"].toString(),j["position"].toObject(),cylinder);const auto moving=counterpart(a,d,j["component1OccurrenceId"].toString(),fixed.center,cylinder);j["movingPosition"]=moving.ref;j["movingMarkerOffsetMm"]=cylinder?fixed.center.Z()-moving.center.Z():fixed.center.Y()-moving.center.Y();j["movingMarkerLocal"]=frame(fixed.center,cylinder);require(cylinder?(j["rotationDeg"].toDouble()==0&&j["slideMm"].toDouble()==0):j["angleDeg"].toDouble()==0,"Create the as-built joint at its existing zero coordinate, then edit/preview");rows.append(j);}else rows[index]=j;
 auto result=a;result["joints"]=rows;const auto after=solve(result,d,{});if(index<0){const auto before=solve(a,d,{});for(auto it=before.poses.begin();it!=before.poses.end();++it){const auto x=array(it.value()),y=array(after.poses[it.key()]);for(int k=0;k<12;++k)require(std::abs(x[k].toDouble()-y[k].toDouble())<1e-6,"As-built creation cannot move an already positioned leaf; return to rest first");}}return result;
}
}
bool AssemblyGraph::isJointGraph(const QJsonObject& a){const auto rows=a["joints"].toArray();return !rows.isEmpty()&&(rows[0].toObject()["type"]=="asBuiltRevolute"||rows[0].toObject()["type"]=="asBuiltCylindrical");}
bool AssemblyGraph::identityLeaf(const QJsonObject& a,const QString& id){QString own;for(const auto& v:a["occurrences"].toArray()){const auto r=v.toObject();if(r["id"]==id&&r["placement"]=="identity")own=r["definitionId"].toString();}if(own.isEmpty())return false;for(const auto& v:a["occurrences"].toArray())if(v.toObject()["parentId"]==own)return false;return true;}
QString AssemblyGraph::validateRecords(const QJsonObject& a){try{
 const auto rows=a["joints"].toArray();require(rows.size()<=4,"This source-sized graph supports at most four joints; copied mechanisms are separate");QSet<QString> ids;
 for(const auto& v:rows){const auto j=v.toObject();const bool cylinder=j["type"]=="asBuiltCylindrical";const QSet<QString> allowed=cylinder?QSet<QString>{"id","type","name","component1OccurrenceId","component2OccurrenceId","position","movingMarkerLocal","rotationDeg","slideMm","movingPosition","movingMarkerOffsetMm"}:QSet<QString>{"id","type","name","component1OccurrenceId","component2OccurrenceId","position","movingMarkerLocal","angleDeg","movingPosition","movingMarkerOffsetMm"};for(auto it=j.begin();it!=j.end();++it)require(allowed.contains(it.key()),"Unknown authored joint field");require(j.contains("movingPosition")==j.contains("movingMarkerOffsetMm"),"Moving origin binding requires identity and offset together");require(cylinder||j["type"]=="asBuiltRevolute","Slider or unsupported type cannot mix with this graph");require(uuid(j["id"].toString())&&!ids.contains(j["id"].toString())&&!j["name"].toString().trimmed().isEmpty(),"Joint needs a distinct stable ID and name");ids.insert(j["id"].toString());const auto x=j["component1OccurrenceId"].toString(),y=j["component2OccurrenceId"].toString();require(x!=y&&identityLeaf(a,x)&&identityLeaf(a,y),"Joint needs distinct identity leaf occurrences");require(definition(a,x)!=definition(a,y),"Multiple placed copies of one definition are outside this graph slice");const auto ref=j["position"].toObject();require(ref.size()==2&&uuid(ref["bodyId"].toString())&&!ref["edgeId"].toString().isEmpty(),"Circular Position needs retained body and edge IDs");const auto marker=j["movingMarkerLocal"].toArray();require(marker.size()==12,"Joint needs its retained local marker");for(const auto& value:marker)require(value.isDouble()&&std::isfinite(value.toDouble()),"Joint marker must be finite");const auto typeFrame=frame(gp_Pnt(0,0,0),cylinder);for(int i=0;i<12;++i)if(i%4!=3)require(std::abs(marker[i].toDouble()-typeFrame[i].toDouble())<1e-9,"Joint local marker has an unsupported axis frame");
  if(j.contains("movingPosition")){const auto moving=j["movingPosition"].toObject();require(moving.size()==2&&uuid(moving["bodyId"].toString())&&!moving["edgeId"].toString().isEmpty()&&j["movingMarkerOffsetMm"].isDouble()&&std::isfinite(j["movingMarkerOffsetMm"].toDouble()),"Moving origin needs retained identity and finite axial offset");}
  if(cylinder)require(j["rotationDeg"].isDouble()&&j["slideMm"].isDouble()&&std::isfinite(j["rotationDeg"].toDouble())&&std::abs(j["rotationDeg"].toDouble())<=180&&std::isfinite(j["slideMm"].toDouble())&&std::abs(j["slideMm"].toDouble())<=100000,"Cylindrical coordinates need finite ±180degrees/±100000mm values");
  else require(j["angleDeg"].isDouble()&&std::isfinite(j["angleDeg"].toDouble())&&std::abs(j["angleDeg"].toDouble())<=180,"Revolute angle must be finite within ±180degrees");
 }
 return {};}catch(const std::exception& e){return QString::fromUtf8(e.what());}}
QJsonObject AssemblyGraph::revoluteCandidate(const QJsonObject& a,const EvaluatedDocument& d,const QJsonObject& args){return candidate(a,d,args,false);}
QJsonObject AssemblyGraph::cylindricalCandidate(const QJsonObject& a,const EvaluatedDocument& d,const QJsonObject& args){return candidate(a,d,args,true);}
std::vector<EvaluatedBody> AssemblyGraph::worldBodies(const QJsonObject& a,const EvaluatedDocument& d,const QJsonObject& o){const auto solved=solve(a,d,o);auto bodies=d.bodies;for(auto& b:bodies){QString occurrence;for(const auto& v:a["occurrences"].toArray())if(v.toObject()["definitionId"]==owner(a,b))occurrence=v.toObject()["id"].toString();if(!occurrence.isEmpty()){if(!atRest(solved.poses[occurrence]))b=LocalReference::placed(std::move(b),trsf(solved.poses[occurrence]));b.occurrenceId=occurrence;b.definitionId=definition(a,occurrence);}}return bodies;}
QJsonObject AssemblyGraph::inspect(const QJsonObject& a,const EvaluatedDocument& d,const QJsonObject& o){const auto solved=solve(a,d,o);QJsonArray occurrences;for(const auto& v:a["occurrences"].toArray()){const auto r=v.toObject();QJsonArray bodies;for(const auto& b:d.bodies)if(owner(a,b)==r["definitionId"].toString()){auto placed=LocalReference::placed(b,trsf(solved.poses[r["id"].toString()]));bodies.append(QJsonObject{{"bodyId",b.bodyId},{"boundsMm",placed.summary["boundsMm"]}});}occurrences.append(QJsonObject{{"occurrenceId",r["id"]},{"definitionId",r["definitionId"]},{"bodies",bodies},{"worldPose",array(solved.poses[r["id"].toString()])}});}return {{"joints",solved.joints},{"occurrences",occurrences},{"islands",solved.islands},{"graphMobility","unqualified"}};}
