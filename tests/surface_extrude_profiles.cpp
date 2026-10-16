// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include "SketchSpline.h"
#include "SurfaceExtrude.h"
#include "SurfaceTrim.h"
#include <QCoreApplication>
#include <QDir>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool value,const QString& message){if(!value)throw std::runtime_error(message.toStdString());}
void commit(CadDocument& d,const QJsonObject& c,const QString& label){const auto e=d.commit(c,label);check(e.isEmpty(),label+": "+e+" "+QString::fromUtf8(QJsonDocument(d.lastEvaluationError()).toJson(QJsonDocument::Compact)));}
void append(CadDocument& d,const QJsonObject& f){auto c=d.authored();auto a=c["features"].toArray();a.append(f);c["features"]=a;commit(d,c,"Add source");}
QJsonObject entity(const QJsonObject& sketch,int i){return sketch["parameters"].toObject()["entities"].toArray()[i].toObject();}
QString eid(const QJsonObject& sketch,int i){return entity(sketch,i)["id"].toString();}
QJsonObject profile(const QJsonArray& ids){return {{"entityIds",ids}};}
const EvaluatedBody& body(const CadDocument& d,const QString& id){for(const auto& b:d.bodies())if(b.bodyId==id)return b;throw std::runtime_error("Missing result body");}
QSet<QString> edges(const EvaluatedBody& b){QSet<QString> ids;for(const auto& e:b.solidEdges)ids.insert(e.edgeId);return ids;}
QSet<QString> faces(const EvaluatedBody& b){QSet<QString> ids;for(const auto& f:b.supportFaces)ids.insert(f.faceId);return ids;}
QJsonObject addSpline(const QJsonObject& sketch,const QJsonArray& points){const auto r=SketchSpline::add(sketch,{{"points",points}});check(r.ok,r.error);return r.feature;}
QJsonObject point(double u,double v){return {{"uMm",u},{"vMm",v}};}
void exactTopology(const EvaluatedBody& b,int count);
QJsonObject sourceEdit(const QJsonObject& sketch,const QString& pointId,double v){auto f=sketch;auto p=f["parameters"].toObject();auto points=p["points"].toArray();for(int i=0;i<points.size();++i)if(points[i].toObject()["id"]==pointId){auto q=points[i].toObject();q["vMm"]=v;points[i]=q;}p["points"]=points;f["parameters"]=p;return f;}
QJsonObject args(const QJsonObject& sketch,const QJsonArray& profiles,double one=60,double two=65){return {{"resultMode","perProfile"},{"sketchId",sketch["id"]},{"profiles",profiles},{"sideOneMm",one},{"sideTwoMm",two},{"ownerId",sketch["ownerId"]}};}
void snappedCoincidentChain(){
 CadDocument d;auto sketch=SketchGeometry::newFeature(d.authored()["documentId"].toString(),"XZ","Pointer-snapped open chain");
 sketch=addSpline(sketch,QJsonArray{point(0,0),point(10,10),point(20,10)});const auto spline=entity(sketch,0);
 auto snapped=SketchGeometry::addPointerEntity(sketch,"line",{{"u1Mm",25},{"v1Mm",15},{"u2Mm",20},{"v2Mm",10},
  {"endSnapTarget",QJsonObject{{"kind","point"},{"pointId",spline["endPointId"]}}}});
 check(snapped.ok,snapped.error);sketch=snapped.feature;const auto line=entity(sketch,1);
 check(line["endPointId"]!=spline["endPointId"],"Pointer snap preserves distinct endpoint IDs");
 bool retained=false;for(const auto& value:sketch["parameters"].toObject()["constraints"].toArray()){
  const auto relation=value.toObject();const auto ids=relation["pointIds"].toArray();
  retained|=relation["type"]=="coincident" && ids.size()==2 && ids.contains(line["endPointId"]) && ids.contains(spline["endPointId"]);
 }check(retained,"Pointer snap retains the endpoint Coincident relation");
 sketch=addSpline(sketch,QJsonArray{point(20.00000001,10),point(30,2),point(40,-5)});append(d,sketch);
 const QJsonArray chain{eid(sketch,0),eid(sketch,1)};const auto discovered=SurfaceExtrude::openChain(d.sketches().back(),eid(sketch,0));
 check(discovered.size()==2 && discovered.contains(eid(sketch,1)) && !discovered.contains(eid(sketch,2)),"Discovery follows declared coincidence without proximity welding");
 auto worldPoint=[](const EvaluatedSketch& evaluated,const QString& id){
  for(const auto& value:evaluated.summary["worldPoints"].toArray())if(value.toObject()["id"]==id)return value.toObject()["positionMm"].toArray();
  return QJsonArray{};
 };
 const auto originalSplineEnd=worldPoint(d.sketches().back(),spline["endPointId"].toString());
 const auto originalLineEnd=worldPoint(d.sketches().back(),line["endPointId"].toString());
 const auto originalLineStart=worldPoint(d.sketches().back(),line["startPointId"].toString());
 const auto disconnected=CadDocument::evaluateCandidate(CadDocument::surfaceExtrudeCandidate(d.authored(),
  args(sketch,QJsonArray{profile(QJsonArray{eid(sketch,0),eid(sketch,1),eid(sketch,2)})})));
 check(!disconnected.ok,"Disconnected profile containing the snapped chain refuses");
 commit(d,CadDocument::surfaceExtrudeCandidate(d.authored(),args(sketch,QJsonArray{profile(chain)})),"Extrude pointer-snapped chain");
 const auto feature=d.authored()["features"].toArray().last().toObject();const auto bodyId=SurfaceExtrude::resultBodyIds(feature).first();
 const auto before=body(d,bodyId);exactTopology(before,2);
 const auto changed=sourceEdit(sketch,line["startPointId"].toString(),18);
 commit(d,SketchGeometry::replaceFeature(d.authored(),changed),"Edit snapped chain endpoint");
 const auto& after=body(d,bodyId);const auto movedSplineEnd=worldPoint(d.sketches().back(),spline["endPointId"].toString());
 const auto movedLineEnd=worldPoint(d.sketches().back(),line["endPointId"].toString());
 const auto movedLineStart=worldPoint(d.sketches().back(),line["startPointId"].toString());
 check(edges(after)==edges(before) && faces(after)==faces(before) && originalLineStart!=movedLineStart
   && originalSplineEnd==movedSplineEnd && originalLineEnd==movedLineEnd && movedSplineEnd==movedLineEnd,
  "Open endpoint edit preserves stable shared boundary identity and snapped contact");
 auto branch=SketchGeometry::addPointerEntity(changed,"line",{{"u1Mm",25},{"v1Mm",15},{"u2Mm",20},{"v2Mm",13},
  {"endSnapTarget",QJsonObject{{"kind","point"},{"pointId",spline["endPointId"]}}}});
 check(branch.ok,branch.error);const auto branchEval=CadDocument::evaluateCandidate(SketchGeometry::replaceFeature(d.authored(),branch.feature));
 check(branchEval.ok,"Retained coincident branch sketch evaluates");bool refused=false;
 try{SurfaceExtrude::openChain(branchEval.evaluation->sketches.back(),eid(sketch,0));}catch(const std::exception&){refused=true;}
 check(refused,"Explicitly coincident branch still refuses as ambiguous");
}
void exactTopology(const EvaluatedBody& b,int count){TopTools_IndexedMapOfShape fs,es,vs;TopExp::MapShapes(b.shape,TopAbs_FACE,fs);TopExp::MapShapes(b.shape,TopAbs_EDGE,es);TopExp::MapShapes(b.shape,TopAbs_VERTEX,vs);
 check(b.summary["valid"].toBool() && b.summary["type"]=="surface" && fs.Extent()==count && es.Extent()==3*count+1 && vs.Extent()==2*(count+1),"Actual connected wire-prism topology");check(int(b.supportFaces.size())==count && int(b.solidEdges.size())==es.Extent(),"All current faces/unique edges named");for(const auto& f:b.supportFaces)check(f.bodyId==b.bodyId && f.provenance["lineage"]=="surfacePrismBuilder","Actual result support ancestry");}
void standalone(){
 CadDocument d;const auto owner=d.authored()["documentId"].toString();auto s=SketchGeometry::newFeature(owner,"XZ","Declared open chains");
 s=addSpline(s,QJsonArray{point(0,0),point(10,10),point(20,10)});
 s=SketchGeometry::addLine(s,{{"u1Mm",20},{"v1Mm",10},{"u2Mm",25},{"v2Mm",15},{"startPointId",entity(s,0)["endPointId"]}});
 s=addSpline(s,QJsonArray{point(-5,-5),point(10,-8),point(30,-3)});append(d,s);
 const QJsonArray upper{eid(s,0),eid(s,1)},lower{eid(s,2)};QJsonArray groups{profile(upper),profile(lower)};
 const auto chain=SurfaceExtrude::openChain(d.sketches().back(),eid(s,0));check(chain.size()==2 && chain.contains(eid(s,1)),"Chaining follows endpoint ancestry");
 auto c=CadDocument::surfaceExtrudeCandidate(d.authored(),args(s,groups));commit(d,c,"Two separate profile results");const auto feature=d.authored()["features"].toArray().last().toObject();
 check(!feature.contains("bodyId") && d.bodies().size()==2,"One feature, separate outputs, no primary body");const auto ids=SurfaceExtrude::resultBodyIds(feature);const auto first=body(d,ids[0]),second=body(d,ids[1]);exactTopology(first,2);exactTopology(second,1);
 auto changed=sourceEdit(s,entity(s,0)["pointIds"].toArray()[1].toString(),13);commit(d,SketchGeometry::replaceFeature(d.authored(),changed),"Edit retained spline");
 check(faces(body(d,ids[0]))==faces(first) && edges(body(d,ids[0]))==edges(first) && body(d,ids[0]).summary["areaMm2"]!=first.summary["areaMm2"],"Source edit preserves current result ancestry");
 auto edit=args(s,QJsonArray{profile(QJsonArray{eid(s,2)}),profile(QJsonArray{eid(s,1),eid(s,0)})},70,55);edit.remove("ownerId");edit["featureId"]=feature["id"];commit(d,CadDocument::surfaceExtrudeCandidate(d.authored(),edit),"Reorder profiles and reaches");
 check(faces(body(d,ids[0]))==faces(first) && edges(body(d,ids[0]))==edges(first) && body(d,ids[1]).bodyId==second.bodyId,"Order and reaches do not identify outputs");
 changed=SketchGeometry::addLine(changed,{{"u1Mm",-20},{"v1Mm",-15},{"u2Mm",45},{"v2Mm",-15}});commit(d,SketchGeometry::replaceFeature(d.authored(),changed),"Add independent bottom overrun");
 const auto beforeAdd=d.authored();groups.append(profile(QJsonArray{eid(changed,3)}));edit["profiles"]=groups;commit(d,CadDocument::surfaceExtrudeCandidate(d.authored(),edit),"Include third profile in same feature");const auto final=d.authored();
 check(d.bodies().size()==3 && edges(body(d,ids[0]))==edges(first) && faces(body(d,ids[1]))==faces(second),"Third output preserves earlier result IDs");d.history().undo();check(d.authored()==beforeAdd && d.bodies().size()==2,"Undo third selection");d.history().redo();check(d.authored()==final && d.bodies().size()==3,"Redo third selection");
 auto remove=CadDocument::removeBodyCandidate(d.authored(),{{"bodyId",ids[0]},{"ownerId",owner}});commit(d,remove,"Remove one profile body");check(d.bodies().size()==2 && body(d,ids[1]).bodyId==ids[1],"Remove leaves sibling available");d.history().undo();check(d.authored()==final,"Undo Remove restores exact authored history");
 QTemporaryDir temp;check(d.save(temp.filePath("profiles.alloy")).isEmpty(),"Save per-profile sheets");CadDocument reopen;check(reopen.open(temp.filePath("profiles.alloy")).isEmpty() && reopen.authored()==final && edges(body(reopen,ids[0]))==edges(first),"Exact reopen per-profile outputs");
 auto bad=edit;bad["profiles"]=QJsonArray{profile(upper),profile(QJsonArray{eid(s,0)})};bool refused=false;try{auto candidate=CadDocument::surfaceExtrudeCandidate(d.authored(),bad);refused=!d.commit(candidate,"Overlap").isEmpty();}catch(const std::exception&){refused=true;}check(refused && d.authored()==final,"Overlapping profiles refuse atomically");
 auto disconnected=edit;disconnected["profiles"]=QJsonArray{profile(QJsonArray{eid(s,0),eid(s,2)})};auto invalid=CadDocument::surfaceExtrudeCandidate(d.authored(),disconnected);check(!d.commit(invalid,"Disconnected profile").isEmpty() && d.authored()==final,"Disconnected profile refuses atomically");
 auto branch=SketchGeometry::addLine(changed,{{"u1Mm",20},{"v1Mm",10},{"u2Mm",22},{"v2Mm",2},{"startPointId",entity(s,0)["endPointId"]}});auto branchAuthored=SketchGeometry::replaceFeature(final,branch);const auto branchEval=CadDocument::evaluateCandidate(branchAuthored);check(branchEval.ok,"Unselected branch does not regroup authored profile");refused=false;try{SurfaceExtrude::openChain(branchEval.evaluation->sketches.back(),eid(s,0));}catch(const std::exception&){refused=true;}check(refused,"New chaining refuses a branch");
 // A referenced output may not disappear when upstream profile membership is changed.
 commit(d,CadDocument::removeBodyCandidate(d.authored(),{{"bodyId",ids[0]},{"ownerId",owner}}),"Reference first output with Remove");
 const auto withReference=d.authored();auto omit=edit;omit["profiles"]=QJsonArray{profile(lower)};
 check(!d.commit(CadDocument::surfaceExtrudeCandidate(d.authored(),omit),"Drop referenced profile").isEmpty() && d.authored()==withReference,"Lost referenced output refuses atomically");d.history().undo();
 auto lost=sourceEdit(changed,entity(s,0)["endPointId"].toString(),12); // Moving the shared point preserves the actual contact, not a coordinate binding.
 commit(d,SketchGeometry::replaceFeature(final,lost),"Move retained shared contact");check(edges(body(d,ids[0]))==edges(first),"Shared contact source edit");
 auto mode=d.authored();auto rows=mode["features"].toArray();auto f=rows.last().toObject();auto p=f["parameters"].toObject();p.remove("resultMode");p.remove("profiles");p["entityIds"]=upper;f["bodyId"]=CadDocument::newId();f["parameters"]=p;rows[rows.size()-1]=f;mode["features"]=rows;const auto stable=d.authored();check(!d.commit(mode,"Migrate result mode").isEmpty() && d.authored()==stable,"No silent legacy migration");
 // Separate planar results independently qualify as tools for the existing bounded Trim, without iron Trim expansion.
 CadDocument trim;auto t=SketchGeometry::newFeature(trim.authored()["documentId"].toString(),"XY","Target");t=SketchGeometry::addLine(t,{{"u1Mm",-20},{"v1Mm",0},{"u2Mm",20},{"v2Mm",0}});append(trim,t);auto ta=args(t,QJsonArray{profile(QJsonArray{eid(t,0)})},20,20);commit(trim,CadDocument::surfaceExtrudeCandidate(trim.authored(),ta),"Target surface");const auto target=trim.bodies().back();
 auto tools=SketchGeometry::newFeature(trim.authored()["documentId"].toString(),"YZ","Two crossing tools");tools=SketchGeometry::addLine(tools,{{"u1Mm",-30},{"v1Mm",-4},{"u2Mm",30},{"v2Mm",-4}});tools=SketchGeometry::addLine(tools,{{"u1Mm",-30},{"v1Mm",5},{"u2Mm",30},{"v2Mm",5}});append(trim,tools);commit(trim,CadDocument::surfaceExtrudeCandidate(trim.authored(),args(tools,QJsonArray{profile(QJsonArray{eid(tools,0)}),profile(QJsonArray{eid(tools,1)})},30,30)),"Independent tools");
 const auto toolFeature=trim.authored()["features"].toArray().last().toObject();for(const auto& tool:SurfaceExtrude::resultBodyIds(toolFeature)){
  const auto id=CadDocument::newId();const QJsonObject probe{{"id",id},{"type","surfaceTrim"},{"parameters",QJsonObject{{"targetBodyId",target.bodyId},{"toolBodyId",tool}}}};const auto split=SurfaceTrim::split(probe,target,trim.evaluation());check(split.regions.size()==2,"Each separate output is an independently addressable Trim tool");
  const auto candidate=CadDocument::surfaceTrimCandidate(trim.authored(),{{"newFeatureId",id},{"targetBodyId",target.bodyId},{"toolBodyId",tool},{"removedRegionIds",QJsonArray{split.regions[0].regionId}}});check(CadDocument::evaluateCandidate(candidate).ok,"Secondary tool dependency registration");
 }
 std::cout<<"SURFACE_EXTRUDE_PROFILES_CORE_OK chainPrism sourceReachOrderEdit thirdResult lineage remove undo reopen atomicRefusal independentTrimTools\n";
}
void iron(const QString& path,const QString& out){
 CadDocument d;check(d.open(path).isEmpty(),"Open accepted actual native bridge");check(d.bodies().size()==2,"Actual iron starts with exterior and bridge");const auto owner=d.bodies().front().ownerId;
 auto sketch=SketchGeometry::newFeature(owner,"XZ","Back and notch sheets");
 // Independent engineering fixture. Public footage does not establish exact notch membership or coordinates.
 sketch=addSpline(sketch,QJsonArray{point(82,20),point(102,38),point(135,60)});
 sketch=SketchGeometry::addLine(sketch,{{"u1Mm",82},{"v1Mm",20},{"u2Mm",78},{"v2Mm",16},{"startPointId",entity(sketch,0)["startPointId"]}});
 sketch=addSpline(sketch,QJsonArray{point(80,14),point(110,8),point(135,-8)});append(d,sketch);QDir().mkpath(out);check(d.save(out+"/pre-extrude.alloy").isEmpty(),"Save actual later sketch");
 QJsonArray profiles{profile(QJsonArray{eid(sketch,0),eid(sketch,1)}),profile(QJsonArray{eid(sketch,2)})};commit(d,CadDocument::surfaceExtrudeCandidate(d.authored(),args(sketch,profiles)),"Two actual iron crossing profiles");check(d.bodies().size()==4,"Actual iron plus two independent tools");const auto feature=d.authored()["features"].toArray().last().toObject();const auto ids=SurfaceExtrude::resultBodyIds(feature);const auto first=body(d,ids[0]),second=body(d,ids[1]);check(first.ownerId==owner && second.ownerId==owner,"Actual active-component outputs");exactTopology(first,2);exactTopology(second,1);check(d.save(out+"/two-profile-sheets.alloy").isEmpty(),"Save two actual tools");
 sketch=SketchGeometry::addLine(sketch,{{"u1Mm",-15},{"v1Mm",-4},{"u2Mm",145},{"v2Mm",-4}});commit(d,SketchGeometry::replaceFeature(d.authored(),sketch),"Add overrun bottom line to same sketch");profiles.append(profile(QJsonArray{eid(sketch,3)}));auto edit=args(sketch,profiles);edit.remove("ownerId");edit["featureId"]=feature["id"];commit(d,CadDocument::surfaceExtrudeCandidate(d.authored(),edit),"Include bottom in retained Extrude");
 check(d.bodies().size()==5 && edges(body(d,ids[0]))==edges(first) && faces(body(d,ids[1]))==faces(second),"Actual five-sheet precursor with stable old outputs");check(d.save(out+"/five-closing-sheet-precursor.alloy").isEmpty(),"Save same-feature bottom edit");CadDocument reopened;check(reopened.open(out+"/five-closing-sheet-precursor.alloy").isEmpty() && reopened.authored()==d.authored(),"Actual iron reopen");
 std::cout<<"SURFACE_EXTRUDE_PROFILES_IRON_OK actualNativeBridge twoProfiles twoBodies sameSketchBottomEdit fiveBodies stableOldOutputs owner reopen\n";
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{snappedCoincidentChain();standalone();if(argc>2)iron(QString::fromLocal8Bit(argv[1]),QString::fromLocal8Bit(argv[2]));return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
