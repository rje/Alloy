// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include "SketchGeometry.h"
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
 const auto unsupported=SketchGeometry::deleteEntities(mirror,{member});check(!unsupported.ok && unsupported.feature==mirror,"unsupported Mirror must refuse atomically");
 std::cout<<"Constructor group Delete core contract passed\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
