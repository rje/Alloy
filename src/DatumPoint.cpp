// SPDX-License-Identifier: MIT
#include "DatumPoint.h"
#include "DatumAxis.h"
#include <BRep_Tool.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <QUuid>
#include <cmath>
namespace {
[[noreturn]] void fail(const QString& code,const QString& text){throw GeometryKernel::EvaluationError({{"code",code},{"message",text}});}
bool uuid(const QJsonValue& v){return v.isString() && !QUuid(v.toString()).isNull();}
}
QString DatumPoint::validate(const QJsonObject& p) {
 const auto source=p.value("source").toObject();
 if(source.size()!=1 || (!source.contains("vertex") && !source.contains("line")))return "Point requires one exact Box edge endpoint or authored straight sketch Line";
 if(source.contains("vertex")){
  const auto v=source.value("vertex").toObject();
  if(p.size()!=1 || v.size()!=3 || !uuid(v.value("bodyId")) || !uuid(v.value("edgeId")) || !v.value("endpoint").isDouble() || (v.value("endpoint")!=0 && v.value("endpoint")!=1))return "At Vertex requires exact bodyId, edgeId and endpoint 0 or 1; no location";
 } else {
  const auto error=DatumAxis::validate(source);if(!error.isEmpty())return QString(error).replace("Axis","Line");
  const auto loc=p.value("location").toObject();const auto mode=loc.value("mode").toString();const auto key=mode=="proportional" ? "ratio":"distanceMm";
  if(p.size()!=2 || loc.size()!=2 || (mode!="proportional" && mode!="physical") || !loc.value(key).isDouble() || !std::isfinite(loc.value(key).toDouble()))return "Along Path requires explicit proportional ratio or physical distanceMm";
  const double value=loc.value(key).toDouble();if(value<0 || (mode=="proportional" && value>1))return "Point location must be within the finite source segment";
 }
 return {};
}
QJsonObject DatumPoint::evaluate(const EvaluatedDocument& e,const QJsonObject& p,const QJsonArray& features) {
 const auto error=validate(p);if(!error.isEmpty())fail("invalid_point_source",error);
 const auto source=p.value("source").toObject();
 if(source.contains("line")){
  QJsonObject line;
  try{line=DatumAxis::evaluate(e,source,features);}catch(const GeometryKernel::EvaluationError& error){auto d=error.diagnostics();d["code"]=d.value("code").toString().replace("axis","point");d["message"]=d.value("message").toString().replace("Select authored standalone sketch Points or an authored straight sketch Line","Along Path supports one authored straight sketch Line").replace("Axis sources","Line endpoints").replace("Axis","Point");throw GeometryKernel::EvaluationError(d);}
  const auto loc=p.value("location").toObject();const double length=line.value("sourceSpanMm").toDouble();const bool physical=loc.value("mode")=="physical";
  const double distance=physical?loc.value("distanceMm").toDouble():loc.value("ratio").toDouble()*length;
  if(distance>length)fail("point_location_out_of_range","Physical distance exceeds the current finite Line length");
  auto xyz=line.value("originMm").toArray();const auto direction=line.value("direction").toArray();for(int i=0;i<3;++i)xyz[i]=xyz[i].toDouble()+distance*direction[i].toDouble();
  return {{"positionMm",xyz},{"sourceLengthMm",length},{"ratio",distance/length},{"distanceMm",distance},{"directionPolicy","authoredStartToEnd"}};
 }
 const auto v=source.value("vertex").toObject();const EvaluatedBody* body=nullptr;
 for(const auto& candidate:e.bodies)if(candidate.bodyId==v.value("bodyId").toString()){if(body)fail("point_source_ambiguous","The retained body is ambiguous");body=&candidate;}
 if(!body)fail("point_source_unavailable","The exact source body is unavailable at this history position");
 const EvaluatedSolidEdge* edge=nullptr;
 for(const auto& candidate:body->solidEdges)if(candidate.edgeId==v.value("edgeId").toString()){if(edge)fail("point_source_ambiguous","The retained edge is ambiguous");edge=&candidate;}
 if(!edge || edge->edge.IsNull())fail("point_source_unavailable","The exact source edge is unavailable");
 const auto meta=edge->provenance;
 if(meta.value("role")!="box.edge" || meta.value("endpointProvenance")!="currentKernelBoundary" || meta.value("boxEdge").toString().isEmpty() || meta.value("generatorFeatureId").toString().isEmpty())fail("unsupported_point_source","At Vertex currently supports semantic Box edge endpoints only");
 TopTools_IndexedMapOfShape currentEdges;TopExp::MapShapes(body->shape,TopAbs_EDGE,currentEdges);
 if(!currentEdges.Contains(edge->edge) || BRepAdaptor_Curve(edge->edge).GetType()!=GeomAbs_Line)fail("point_source_unavailable","Retained edge is not exact current Box boundary topology");
 TopoDS_Vertex first,last;TopExp::Vertices(edge->edge,first,last,true);
 if(first.IsNull() || last.IsNull() || first.IsSame(last))fail("point_source_unavailable","Retained edge must have two distinct current vertices");
 const auto start=meta.value("startMm").toArray(),end=meta.value("endMm").toArray();
 if(start.size()!=3 || end.size()!=3)fail("point_source_unavailable","Retained edge has no oriented endpoint provenance");
 const gp_Pnt a=BRep_Tool::Pnt(first),b=BRep_Tool::Pnt(last),s(start[0].toDouble(),start[1].toDouble(),start[2].toDouble()),t(end[0].toDouble(),end[1].toDouble(),end[2].toDouble());
 const bool forward=a.Distance(s)<1e-6 && b.Distance(t)<1e-6,reversed=b.Distance(s)<1e-6 && a.Distance(t)<1e-6;
 if(forward==reversed)fail("point_source_ambiguous","Current vertices do not uniquely match retained endpoint provenance");
 const auto point=v.value("endpoint")==0 ? (forward?a:b):(forward?b:a);
 return {{"positionMm",QJsonArray{point.X(),point.Y(),point.Z()}}};
}
