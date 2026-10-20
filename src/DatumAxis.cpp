// SPDX-License-Identifier: MIT
#include "DatumAxis.h"
#include <BRepAdaptor_Curve.hxx>
#include <QUuid>
#include <cmath>
namespace {
[[noreturn]] void fail(const QString& code,const QString& text){throw GeometryKernel::EvaluationError({{"code",code},{"message",text}});}
}
QString DatumAxis::validate(const QJsonObject& source) {
 if(source.size()!=1 || (!source.contains("points") && !source.contains("line")))return "Axis requires exactly two standalone sketch Points or one straight sketch Line";
 const bool points=source.contains("points");
 const auto refs=points ? source.value("points").toArray() : QJsonArray{source.value("line")};
 if(points && (!source.value("points").isArray() || refs.size()!=2))return "Axis requires two Point references";
 for(const auto& value:refs){const auto ref=value.toObject();if(ref.size()!=(points?3:4))return "Axis source requires exact typed IDs";
  const auto fields=points ? QStringList{"sketchId","entityId","pointId"} : QStringList{"sketchId","entityId","startPointId","endPointId"};
  for(const auto& key:fields)if(!ref.value(key).isString() || QUuid(ref.value(key).toString()).isNull())return "Axis source requires stable UUIDs";
  if(!points && ref.value("startPointId")==ref.value("endPointId"))return "Axis Line endpoints must be distinct";
 }
 return {};
}
QJsonObject DatumAxis::evaluate(const EvaluatedDocument& evaluation,const QJsonObject& source,const QJsonArray& features) {
 const auto error=validate(source);if(!error.isEmpty())fail("invalid_axis_source",error);
 const bool points=source.contains("points");const auto refs=points ? source.value("points").toArray() : QJsonArray{source.value("line")};
 std::vector<gp_Pnt> positions;
 for(const auto& value:refs){const auto ref=value.toObject();const auto sketchId=ref.value("sketchId").toString(),entityId=ref.value("entityId").toString();
  const EvaluatedSketch* solved=nullptr;for(const auto& sketch:evaluation.sketches)if(sketch.featureId==sketchId)solved=&sketch;
  if(!solved)fail("axis_source_unavailable","The exact source sketch is unavailable at this history position");
  QJsonObject entity,parameters;for(const auto& row:features)if(row.toObject().value("id")==sketchId) {parameters=row.toObject().value("parameters").toObject();
   for(const auto& item:row.toObject().value("parameters").toObject().value("entities").toArray())if(item.toObject().value("id")==entityId)entity=item.toObject();}
  for(const auto& group:parameters.value("mirrorGroups").toArray())for(const auto& pair:group.toObject().value("entityPairs").toArray())if(pair.toObject().value("resultEntityId")==entityId)fail("unsupported_axis_source","Generated Mirror results are outside the authored source slice");
  for(const auto& family:{"openChainOffsets","closedRectOffsets"})for(const auto& group:parameters.value(family).toArray())for(const auto& id:group.toObject().value(QString(family)=="openChainOffsets"?"generatedEntityIds":"entityIds").toArray())if(id==entityId)fail("unsupported_axis_source","Generated Offset results are outside the authored source slice");
  if(entity.value("type")!=(points?"point":"line"))fail("unsupported_axis_source","Select authored standalone sketch Points or an authored straight sketch Line");
  if(points && entity.value("pointId")!=ref.value("pointId"))fail("axis_source_identity_changed","The retained Point identity changed");
  if(!points && (entity.value("startPointId")!=ref.value("startPointId") || entity.value("endPointId")!=ref.value("endPointId")))fail("axis_line_order_changed","The retained Line endpoint identity or order changed");
  QMap<QString,gp_Pnt> world;for(const auto& item:solved->summary.value("worldPoints").toArray()){const auto row=item.toObject();const auto xyz=row.value("positionMm").toArray();if(xyz.size()==3)world[row.value("id").toString()]=gp_Pnt(xyz[0].toDouble(),xyz[1].toDouble(),xyz[2].toDouble());}
  const auto ids=points ? QStringList{ref.value("pointId").toString()} : QStringList{ref.value("startPointId").toString(),ref.value("endPointId").toString()};
  for(const auto& id:ids){if(!world.contains(id))fail("axis_source_unavailable","The exact solved source point is unavailable");positions.push_back(world.value(id));}
  if(!points){const auto edge=solved->entityEdges.value(entityId);if(edge.IsNull() || BRepAdaptor_Curve(edge).GetType()!=GeomAbs_Line)fail("unsupported_axis_source","The exact source must evaluate as a straight Line");
   const BRepAdaptor_Curve curve(edge);const auto a=curve.Value(curve.FirstParameter()),b=curve.Value(curve.LastParameter());
   if(!((a.Distance(positions[0])<1e-6 && b.Distance(positions[1])<1e-6)||(a.Distance(positions[1])<1e-6 && b.Distance(positions[0])<1e-6)))fail("axis_source_unavailable","The evaluated Line no longer matches its retained endpoints");}
 }
 const auto a=positions[0],b=positions[1];gp_Vec direction(a,b);const double span=direction.Magnitude();if(!std::isfinite(span) || span<1e-7)fail("axis_coincident_points","Axis sources must have distinct solved positions");direction.Normalize();
 return {{"originMm",QJsonArray{a.X(),a.Y(),a.Z()}},{"direction",QJsonArray{direction.X(),direction.Y(),direction.Z()}},{"sourceSpanMm",span}};
}
