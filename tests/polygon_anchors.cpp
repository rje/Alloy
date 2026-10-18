#include "CadDocument.h"
#include "SketchGeometry.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QJsonDocument>
#include <QUuid>
#include <cmath>
#include <iostream>
#include <stdexcept>
namespace {
void check(bool good,const char* text){if(!good)throw std::runtime_error(text);}
void near(double a,double b,const char* text){if(std::abs(a-b)>1e-5)throw std::runtime_error(QString("%1: %2 != %3").arg(text).arg(a).arg(b).toStdString());}
QJsonObject good(SketchGeometry::SolveResult r){if(!r.ok)throw std::runtime_error(r.error.toStdString());return r.feature;}
QJsonObject group(const QJsonObject& f){return f["parameters"].toObject()["polygonGroups"].toArray().last().toObject();}
QJsonObject point(const QJsonObject& f,const QJsonValue& id){for(const auto& row:f["parameters"].toObject()["points"].toArray())if(row.toObject()["id"]==id)return row.toObject();throw std::runtime_error("missing point");}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
 CadDocument d;const auto empty=SketchGeometry::newFeature(d.activeOwnerId(),"XY");
 auto source=good(SketchGeometry::addStandalonePoint(empty,{{"uMm",10},{"vMm",0}}));const auto sourcePoint=source["parameters"].toObject()["entities"].toArray().first().toObject().value("pointId");
 auto anchored=good(SketchGeometry::polygon(source,{{"mode","circumscribed"},{"uMm",99},{"vMm",99},{"sizeMm",10},{"count",6},{"anchorSnapTarget",QJsonObject{{"kind","point"},{"pointId",sourcePoint}}}}));
 const auto originalGroup=group(anchored);check(originalGroup["anchorSource"]==QJsonObject{{"kind","point"},{"pointId",sourcePoint}},"retained authored-point source");near(originalGroup["uMm"].toDouble(),10,"point snap overrides raw coordinate");
 auto moved=good(SketchGeometry::movePoint(anchored,{{"pointId",sourcePoint},{"uMm",25},{"vMm",7}}));near(group(moved)["uMm"].toDouble(),25,"free source translates anchor exactly");near(group(moved)["vMm"].toDouble(),7,"free source translates anchor y");
 check(group(moved)["pointIds"]==originalGroup["pointIds"] && group(moved)["entityIds"]==originalGroup["entityIds"],"translation preserves boundary identities");check(SketchGeometry::evaluateFeature(moved).profiles.size()==1,"translated profile remains closed");
 auto edited=good(SketchGeometry::polygon(moved,{{"polygonId",originalGroup["id"]},{"sizeMm",12},{"count",8}},true));near(group(edited)["uMm"].toDouble(),25,"size and count preserve source location");check(group(edited)["anchorSource"]==originalGroup["anchorSource"] && group(edited)["anchorConstraintId"]==originalGroup["anchorConstraintId"],"size and count preserve anchor relation");
 auto refused=SketchGeometry::polygon(edited,{{"polygonId",originalGroup["id"]},{"uMm",40}},true);check(!refused.ok && refused.feature==edited,"snapped anchor cannot silently relocate");
 refused=SketchGeometry::polygon(source,{{"mode","inscribed"},{"uMm",0},{"vMm",0},{"sizeMm",4},{"anchorSnapTarget",QJsonObject{{"kind","point"},{"pointId",CadDocument::newId()}}}});check(!refused.ok && refused.feature==source,"missing source refuses atomically");
 refused=SketchGeometry::polygon(source,{{"mode","inscribed"},{"uMm",0},{"vMm",0},{"sizeMm",4},{"anchorSnapTarget",QJsonObject{{"kind","ambiguous"}}}});check(!refused.ok && refused.feature==source,"ambiguous target cannot create geometry");
 refused=SketchGeometry::deleteEntities(edited,QJsonArray{source["parameters"].toObject()["entities"].toArray().first().toObject()["id"]});check(!refused.ok && refused.feature==edited,"removed source point refuses atomically");
 auto invalid=edited;auto ip=invalid["parameters"].toObject();auto gs=ip["polygonGroups"].toArray();auto self=gs[0].toObject();self["anchorSource"]=QJsonObject{{"kind","point"},{"pointId",self["pointIds"].toArray()[0]}};gs[0]=self;ip["polygonGroups"]=gs;invalid["parameters"]=ip;check(!SketchGeometry::solveDraft(invalid).ok,"self-reference refuses");
 check(d.commit(SketchGeometry::replaceFeature(d.authored(),anchored),"Anchor").isEmpty(),"commit anchor");check(d.commit(SketchGeometry::replaceFeature(d.authored(),edited),"Move and edit").isEmpty(),"commit edited anchor");const auto saved=d.authored();d.history().undo();d.history().redo();check(d.authored()==saved,"exact anchor history");QTemporaryDir directory;const auto path=directory.path()+"/anchor.alloy";check(d.save(path).isEmpty(),"save anchor");CadDocument reopened;check(reopened.open(path).isEmpty() && reopened.authored()==saved,"exact anchor reopen");
 // A constrained source line supplies one movable endpoint and a true midpoint.
 auto line=SketchGeometry::addLine(empty,{{"u1Mm",-20},{"v1Mm",-30},{"u2Mm",20},{"v2Mm",-30}});const auto e=line["parameters"].toObject()["entities"].toArray().first().toObject();
 line=good(SketchGeometry::setConstruction(line,{{"entityIds",QJsonArray{e["id"]}},{"construction",true}}));line=good(SketchGeometry::addConstraint(line,{{"type","fixed"},{"pointIds",QJsonArray{e["startPointId"]}}}));line=good(SketchGeometry::addConstraint(line,{{"type","horizontal"},{"entityIds",QJsonArray{e["id"]}}}));line=good(SketchGeometry::addDimension(line,{{"kind","linear"},{"entityId",e["id"]},{"valueMm",40}}));const auto driver=line["parameters"].toObject()["dimensions"].toArray().last().toObject().value("id");
 for(const auto& mode:{"inscribed","circumscribed","edge"}){
  const QJsonObject target=QString(mode)=="inscribed" ? QJsonObject{{"kind","point"},{"pointId",e["endPointId"]}} : QString(mode)=="circumscribed" ? QJsonObject{{"kind","midpoint"},{"entityId",e["id"]}} : QJsonObject{{"kind","origin"}};
  auto f=good(SketchGeometry::polygon(line,{{"mode",mode},{"uMm",0},{"vMm",0},{"sizeMm",5},{"count",6},{"anchorSnapTarget",target}}));const auto g=group(f);const auto witness=g["pointIds"].toArray().at(1);const auto w=point(f,witness);
  f=good(SketchGeometry::addStandalonePoint(f,{{"uMm",w["uMm"]},{"vMm",w["vMm"]}}));const auto external=f["parameters"].toObject()["entities"].toArray().last().toObject().value("pointId");f=good(SketchGeometry::addConstraint(f,{{"type","coincident"},{"pointIds",QJsonArray{external,witness}}}));
  f=good(SketchGeometry::updateDimension(f,{{"dimensionId",driver},{"valueMm",60}}));const auto updated=group(f);near(updated["uMm"].toDouble(),QString(mode)=="inscribed" ? 40 : QString(mode)=="circumscribed" ? 10 : 0,"source driver propagates true anchor");
  f=good(SketchGeometry::polygon(f,{{"polygonId",g["id"]},{"sizeMm",7},{"count",8}},true));near(point(f,external)["uMm"].toDouble(),point(f,witness)["uMm"].toDouble(),"surviving vertex external Coincident x");near(point(f,external)["vMm"].toDouble(),point(f,witness)["vMm"].toDouble(),"surviving vertex external Coincident y");check(SketchGeometry::evaluateFeature(f).profiles.size()==1,"all anchored modes retain closed profile");
  if(QString(mode)=="circumscribed"){const auto removed=SketchGeometry::deleteEntities(f,QJsonArray{e["id"]});check(!removed.ok && removed.feature==f,"midpoint line removal refuses");}
 }
 // Legacy absolute groups are still readable; edit migration reuses every boundary ID.
 auto legacy=good(SketchGeometry::polygon(empty,{{"mode","inscribed"},{"uMm",0},{"vMm",0},{"sizeMm",8},{"count",6}}));auto lp=legacy["parameters"].toObject();auto lg=group(legacy);QJsonArray lpoints,lconstraints;
 for(const auto& row:lp["points"].toArray())if(row.toObject()["id"]!=lg["anchorPointId"])lpoints.append(row);
 for(const auto& row:lp["constraints"].toArray()){auto c=row.toObject();if(c["id"]==lg["anchorConstraintId"])continue;if(c["type"]=="polygonVertex"){const auto id=c["pointIds"].toArray().at(1);const auto p=point(legacy,id);c={{"id",c["id"]},{"type","fixed"},{"pointIds",QJsonArray{id}},{"uMm",p["uMm"]},{"vMm",p["vMm"]}};}lconstraints.append(c);}
 lg.remove("anchorPointId");lg.remove("anchorConstraintId");lg.remove("anchorSource");lp["points"]=lpoints;lp["constraints"]=lconstraints;lp["polygonGroups"]=QJsonArray{lg};legacy["parameters"]=lp;check(SketchGeometry::solveDraft(legacy).ok,"legacy fixed group remains readable");
 const auto migrated=good(SketchGeometry::polygon(legacy,{{"polygonId",lg["id"]},{"sizeMm",9}},true));const auto again=good(SketchGeometry::polygon(legacy,{{"polygonId",lg["id"]},{"sizeMm",9}},true));check(migrated==again && group(migrated)["pointIds"]==lg["pointIds"] && group(migrated)["entityIds"]==lg["entityIds"],"legacy migration is deterministic and preserves boundary IDs");
 // Existing retained projected edges supply genuine locked point/midpoint anchors.
 CadDocument supportDoc;const auto boxId=CadDocument::newId();QJsonObject box{{"id",boxId},{"bodyId",CadDocument::newId()},{"ownerId",supportDoc.activeOwnerId()},
  {"type","box"},{"name","Support"},{"visible",true},{"parameters",QJsonObject{{"widthMm",40},{"depthMm",30},{"heightMm",10},{"xMm",0},{"yMm",0},{"zMm",0}}}};
 auto bodyDocument=supportDoc.authored();bodyDocument["features"]=QJsonArray{box};check(supportDoc.commit(bodyDocument,"Support").isEmpty(),"support box");
 const auto body=supportDoc.bodies()[0];EvaluatedSupportFace top;bool found=false;for(const auto& face:body.supportFaces)if(face.role=="box.z1"){top=face;found=true;break;}check(found,"support top");
 const QJsonObject support{{"producerFeatureId",top.producerFeatureId},{"bodyId",top.bodyId},{"faceId",top.faceId}};
 const auto supported=SketchGeometry::newSupportedFeature(supportDoc.activeOwnerId(),support,"Projected",GeometryKernel::supportEdgeDescriptors(top));const auto context=GeometryKernel::projectionContext(supportDoc.evaluation(),supported);const auto projectedLine=context["lines"].toArray().first().toObject();
 for(const auto& kind:{"point","midpoint"}){
  const QJsonObject target=QString(kind)=="point" ? QJsonObject{{"kind","point"},{"pointId",SketchGeometry::projectionPointId(projectedLine["id"].toString(),true)}} : QJsonObject{{"kind","midpoint"},{"entityId",projectedLine["id"]}};
  auto f=good(SketchGeometry::polygon(supported,{{"mode","inscribed"},{"uMm",0},{"vMm",0},{"sizeMm",2},{"anchorSnapTarget",target}},false,context));check(group(f)["anchorSource"].toObject()["kind"]==kind,"projected anchor source retained");
  check(SketchGeometry::evaluateFeature(f,top.frame,context).profiles.size()==1,"projected anchor closes one profile");
  const auto gid=group(f).value("id");const auto resized=good(SketchGeometry::polygon(f,{{"polygonId",gid},{"sizeMm",3}},true,context));check(group(resized)["anchorConstraintId"]==group(f)["anchorConstraintId"],"projected source survives size edit");
 }
 std::cout<<"Polygon anchor core contract passed\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
