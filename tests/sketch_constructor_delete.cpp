// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
#include "SketchOpenChainOffset.h"
#include "SketchSpline.h"
#include <QCoreApplication>
#include <iostream>
#include <stdexcept>
namespace {
void check(bool b,const QString& m){if(!b)throw std::runtime_error(m.toStdString());}
QJsonObject good(const SketchGeometry::SolveResult& r){check(r.ok,r.error);return r.feature;}
QJsonObject p(const QJsonObject& f){return f.value("parameters").toObject();}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
 CadDocument doc;auto empty=SketchGeometry::newFeature(doc.activeOwnerId(),"XY");
 auto source=good(SketchGeometry::addStandalonePoint(empty,{{"uMm",0},{"vMm",0}}));
 const auto sourceEntity=p(source).value("entities").toArray().last().toObject();
 const QJsonObject snap{{"kind","point"},{"pointId",sourceEntity.value("pointId")}};
 auto slot=good(SketchGeometry::addCenterPointSlot(source,{{"centerUMm",0},{"centerVMm",0},{"endUMm",20},{"endVMm",0},{"widthMm",10},{"centerSnapTarget",snap}}));
 const auto sg=p(slot).value("slotGroups").toArray()[0].toObject();
 slot=good(SketchGeometry::addDimension(slot,{{"kind","linear"},{"entityId",sg.value("centerlineId")},{"valueMm",40}}));
 auto arc=good(SketchGeometry::addCenterPointArcSlot(source,{{"centerUMm",0},{"centerVMm",0},{"firstUMm",20},{"firstVMm",0},{"lastUMm",0},{"lastVMm",20},{"widthMm",6},{"centerSnapTarget",snap}}));
 auto polygon=good(SketchGeometry::polygon(source,{{"mode","inscribed"},{"uMm",0},{"vMm",0},{"sizeMm",15},{"count",6},{"anchorSnapTarget",snap}}));
 const auto pg=p(polygon).value("polygonGroups").toArray()[0].toObject();
 // A surviving line reuses a generated vertex identity. The point survives,
 // while the polygon's generated driver constraint must still be removed.
 QJsonObject point;for(const auto& row:p(polygon).value("points").toArray())if(row.toObject().value("id")==pg.value("pointIds").toArray()[0])point=row.toObject();
 polygon=good(SketchGeometry::solveDraft(SketchGeometry::addLine(polygon,{{"u1Mm",point.value("uMm")},{"v1Mm",point.value("vMm")},{"u2Mm",40},{"v2Mm",35},{"startPointId",point.value("id")}})));
 for(const auto& fixture:QList<QPair<QJsonObject,QString>>{{slot,"slotGroups"},{arc,"arcSlotGroups"},{polygon,"polygonGroups"}}){
  const auto before=fixture.first,params=p(before),group=params.value(fixture.second).toArray()[0].toObject();
  const auto target=fixture.second=="slotGroups" ? group.value("sideIds").toArray()[0] : fixture.second=="arcSlotGroups" ? group.value("outerArcId") : group.value("entityIds").toArray()[0];
  const auto result=good(SketchGeometry::deleteEntities(before,{target}));
  check(!p(result).contains(fixture.second),"owner metadata survived");check(p(result).value("entities").toArray().contains(sourceEntity),"source entity was lost");
  check(p(result).value("dimensions").toArray().isEmpty(),"member dimension survived");
  if(fixture.second=="polygonGroups"){
   check(p(result).value("entities").toArray().size()==2,"external line was lost");
   for(const auto& c:p(result).value("constraints").toArray())check(!group.value("constraintIds").toArray().contains(c.toObject().value("id")),"owned polygon driver survived");
  }else {auto expected=p(source);expected["nextDimensionNameIndex"]=params.value("nextDimensionNameIndex");check(p(result)==expected,"unused generated points/relations survived or source changed");}
 }
 auto rect=good(SketchGeometry::addCenterRectangle(empty,{{"centerUMm",0},{"centerVMm",0},{"widthMm",80},{"heightMm",50}}));
 const auto es=p(rect).value("entities").toArray();auto chamfer=good(SketchGeometry::chamferCorner(rect,{{"entityIds",QJsonArray{es[1].toObject().value("id"),es[2].toObject().value("id")}},{"distanceMm",5}}));
 chamfer=good(SketchGeometry::chamferCorner(chamfer,{{"entityIds",QJsonArray{es[2].toObject().value("id"),es[3].toObject().value("id")}},{"distanceMm",7}}));
 const auto groups=p(chamfer).value("chamferGroups").toArray();
 const auto removed=good(SketchGeometry::deleteEntities(chamfer,{groups[0].toObject().value("lineId")}));
 check(p(removed).value("chamferGroups")==QJsonArray{groups[1]},"adjacent Chamfer changed");
 check(p(removed).value("entities")==p(rect).value("entities") && p(removed).value("points")==p(rect).value("points"),"source endpoints changed");
 const auto restored=good(SketchGeometry::deleteEntities(removed,{groups[1].toObject().value("lineId")}));check(p(restored)==p(rect),"last Chamfer did not restore exact source");
 const auto refused=SketchGeometry::deleteEntities(chamfer,{es[2].toObject().value("id")});check(!refused.ok && refused.feature==chamfer,"source deletion must refuse atomically");
 const auto stale=SketchGeometry::deleteEntities(slot,{sg.value("sideIds").toArray()[0],"stale"});check(!stale.ok && stale.feature==slot,"mixed stale target published partial deletion");
 auto mirror=SketchGeometry::addLine(empty,{{"u1Mm",-20},{"v1Mm",0},{"u2Mm",20},{"v2Mm",0}});
 const auto axis=p(mirror).value("entities").toArray()[0].toObject().value("id");
 mirror=SketchGeometry::addLine(mirror,{{"u1Mm",0},{"v1Mm",10},{"u2Mm",10},{"v2Mm",10}});
 const auto seed=p(mirror).value("entities").toArray()[1].toObject().value("id");
 mirror=good(SketchGeometry::mirrorEntities(mirror,{{"sourceEntityIds",QJsonArray{seed}},{"axisEntityId",axis}}));
 const auto member=p(mirror).value("mirrorGroups").toArray()[0].toObject().value("entityPairs").toArray()[0].toObject().value("resultEntityId");
 const auto removedMirror=good(SketchGeometry::deleteEntities(mirror,{member}));check(!p(removedMirror).contains("mirrorGroups") && p(removedMirror).value("entities").toArray().size()==2,"Mirror owner removal failed");

 auto atomic=[](const QJsonObject& f,const QJsonArray& ids){const auto r=SketchGeometry::deleteEntities(f,ids);check(!r.ok && r.feature==f,"unsafe Delete was not atomic");};
 atomic(mirror,{seed});atomic(mirror,{axis});atomic(mirror,{member,seed});atomic(mirror,{member,"stale"});
 // A shared on-axis source/result point must survive owner removal.
 auto shared=SketchGeometry::addLine(empty,{{"u1Mm",-30},{"v1Mm",0},{"u2Mm",30},{"v2Mm",0}});
 const auto sharedAxis=p(shared).value("entities").toArray().last().toObject().value("id");
 shared=SketchGeometry::addLine(shared,{{"u1Mm",0},{"v1Mm",0},{"u2Mm",10},{"v2Mm",12}});
 const auto sharedLine=p(shared).value("entities").toArray().last().toObject();
 shared=SketchGeometry::addArc(shared,{{"centerUMm",18},{"centerVMm",15},{"radiusMm",4},{"startAngleDeg",0},{"sweepAngleDeg",120}});
 const auto arcId=p(shared).value("entities").toArray().last().toObject().value("id");
 shared=good(SketchGeometry::solveDraft(shared));auto sharedBaseline=p(shared);sharedBaseline["constraints"]=QJsonArray{};
 shared=good(SketchGeometry::mirrorEntities(shared,{{"sourceEntityIds",QJsonArray{sharedLine.value("id"),arcId}},{"axisEntityId",sharedAxis}}));
 const auto sharedGroup=p(shared).value("mirrorGroups").toArray()[0].toObject();
 const auto sharedTarget=sharedGroup.value("entityPairs").toArray()[1].toObject().value("resultEntityId");
 const auto sharedAfter=p(good(SketchGeometry::deleteEntities(shared,{sharedTarget})));check(sharedAfter==sharedBaseline,"mixed line/arc Mirror lost shared source point or left generated points");
 auto measured=good(SketchGeometry::addDimension(shared,{{"kind","radius"},{"entityId",sharedTarget},{"driving",false}}));
 const auto measuredRemoved=good(SketchGeometry::deleteEntities(measured,{sharedTarget}));
 check(p(measuredRemoved).value("dimensions").toArray().isEmpty(),"Mirror read-only measurement survived");
 // A surviving Chamfer owns a Mirror result: refuse the group before publishing.
 const auto mirrorLine=sharedGroup.value("entityPairs").toArray()[0].toObject().value("resultEntityId");
 auto dependent=good(SketchGeometry::chamferCorner(shared,{{"entityIds",QJsonArray{sharedLine.value("id"),mirrorLine}},{"distanceMm",2}}));
 atomic(dependent,{sharedTarget});
 auto spline=good(SketchSpline::add(mirror,{{"points",QJsonArray{QJsonObject{{"uMm",-15},{"vMm",20}},QJsonObject{{"uMm",0},{"vMm",25}},QJsonObject{{"uMm",15},{"vMm",20}}}}}));
 const auto splineId=p(spline).value("entities").toArray().last().toObject().value("id");
 spline=good(SketchGeometry::mirrorEntities(spline,{{"sourceEntityIds",QJsonArray{splineId}},{"axisEntityId",axis}}));
 atomic(spline,{p(spline).value("mirrorGroups").toArray().last().toObject().value("entityPairs").toArray()[0].toObject().value("resultEntityId")});
 auto offset=empty;const double xy[][4]={{-12,10,-8,10},{-8,10,-6,0},{-6,0,6,0},{6,0,8,10},{8,10,12,10},{-8,10,8,10}};
 for(const auto& e:xy)offset=SketchGeometry::addLine(offset,{{"u1Mm",e[0]},{"v1Mm",e[1]},{"u2Mm",e[2]},{"v2Mm",e[3]}});
 offset=good(SketchGeometry::solveDraft(offset));auto offsetBaseline=p(offset);offsetBaseline["constraints"]=QJsonArray{};const auto offsetLines=offsetBaseline.value("entities").toArray();QJsonArray sources;
 for(int i=0;i<5;++i)sources.append(offsetLines[i].toObject().value("id"));
 offset=good(SketchGeometry::solveDraft(SketchOpenChainOffset::add(offset,p(offset),{{"sourceEntityIds",sources},{"topEntityId",offsetLines[5].toObject().value("id")},{"signedDistanceMm",-0.25}})));
 const auto og=p(offset).value("openChainOffsets").toArray()[0].toObject();const auto ot=og.value("generatedEntityIds").toArray().at(2);
 check(p(good(SketchGeometry::deleteEntities(offset,{ot})))==offsetBaseline,"Offset deletion changed sources or retained generated points");
 auto offsetMeasured=good(SketchGeometry::addDimension(offset,{{"kind","linear"},{"entityId",ot},{"driving",false}}));
 check(p(good(SketchGeometry::deleteEntities(offsetMeasured,{ot}))).value("dimensions").toArray().isEmpty(),"Offset measurement survived");
 atomic(offset,{sources[0]});atomic(offset,{ot,sources[0]});atomic(offset,{ot,"stale"});
 // Explicit closure uses genuine derived point identities. Keep both points and
 // the unrelated closure after deleting the owning offset.
 const auto hits=og.value("topIntersectionPointIds").toArray();QJsonObject left,right;
 for(const auto& v:p(offset).value("points").toArray()){const auto q=v.toObject();if(q.value("id")==hits[0])left=q;if(q.value("id")==hits[1])right=q;}
 offset=good(SketchGeometry::solveDraft(SketchGeometry::addLine(offset,{{"u1Mm",left.value("uMm")},{"v1Mm",left.value("vMm")},{"u2Mm",right.value("uMm")},{"v2Mm",right.value("vMm")},{"startPointId",hits[0]},{"endPointId",hits[1]}})));
 const auto closure=p(offset).value("entities").toArray().last();
 const auto offsetRemoved=p(good(SketchGeometry::deleteEntities(offset,{ot})));
 check(offsetRemoved.value("entities").toArray().contains(closure) && offsetRemoved.value("points").toArray().contains(left) && offsetRemoved.value("points").toArray().contains(right),"shared closure IDs lost");
 std::cout<<"Constructor group Delete core contract passed\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
