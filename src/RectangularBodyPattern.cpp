// SPDX-License-Identifier: MIT
#include "RectangularBodyPattern.h"
#include "SolidTransform.h"
#include <BRepAdaptor_Curve.hxx>
#include <QUuid>
#include <cmath>
#include <algorithm>
namespace {
[[noreturn]] void refuse(const QString& code,const QString& message){throw GeometryKernel::EvaluationError({{"code",code},{"message",message}});}
}
QString RectangularBodyPattern::bodyId(const QString& featureId,const QString& sourceBodyId,int i,int j){
 return !i&&!j?sourceBodyId:QUuid::createUuidV5(QUuid(featureId),QByteArray("alloy-rectangular-body-pattern/1/")+QByteArray::number(i)+'/'+QByteArray::number(j)+"/body/"+sourceBodyId.toUtf8()).toString(QUuid::WithoutBraces);
}
QStringList RectangularBodyPattern::generatedBodyIds(const QJsonObject& feature){
 const auto p=feature["parameters"].toObject();const auto axes=p["axes"].toArray();QStringList ids;
 const int m=axes[0].toObject()["quantity"].toInt(),n=axes.size()==2?axes[1].toObject()["quantity"].toInt():1;
 for(int j=0;j<n;++j)for(int i=0;i<m;++i)if(i||j)ids.append(bodyId(feature["id"].toString(),p["sourceBodyId"].toString(),i,j));return ids;
}
std::vector<EvaluatedBody> RectangularBodyPattern::evaluate(const QJsonObject& feature,const EvaluatedBody& source){
 if(source.summary["kind"]=="surface" || source.summary["topology"].toObject()["solids"].toInt()!=1)refuse("unsupported_pattern_body","Select one committed solid body");
 const auto p=feature["parameters"].toObject();const auto axes=p["axes"].toArray();std::vector<gp_Vec> steps;QJsonArray resolved;
 for(const auto& value:axes){const auto a=value.toObject();const auto edge=std::find_if(source.solidEdges.begin(),source.solidEdges.end(),[&](const auto& e){return e.edgeId==a["edgeId"].toString();});
  if(edge==source.solidEdges.end()||edge->edge.IsNull())refuse("pattern_axis_unavailable","Select a current retained straight edge of the source body");
  BRepAdaptor_Curve curve(edge->edge);if(curve.GetType()!=GeomAbs_Line)refuse("unsupported_pattern_axis","Body Pattern directions require straight source-body edges");
  const auto kernelFirst=curve.Value(curve.FirstParameter()),kernelLast=curve.Value(curve.LastParameter());
  const auto start=edge->provenance.contains("startMm")?edge->provenance["startMm"].toArray():QJsonArray{kernelFirst.X(),kernelFirst.Y(),kernelFirst.Z()},end=edge->provenance.contains("endMm")?edge->provenance["endMm"].toArray():QJsonArray{kernelLast.X(),kernelLast.Y(),kernelLast.Z()};
  if(start.size()!=3||end.size()!=3)refuse("pattern_axis_unavailable","Direction edge has no retained oriented endpoints");
  gp_Pnt first(start[0].toDouble(),start[1].toDouble(),start[2].toDouble()),last(end[0].toDouble(),end[1].toDouble(),end[2].toDouble());
  if(!((first.Distance(curve.Value(curve.FirstParameter()))<1e-6&&last.Distance(curve.Value(curve.LastParameter()))<1e-6)||(first.Distance(curve.Value(curve.LastParameter()))<1e-6&&last.Distance(curve.Value(curve.FirstParameter()))<1e-6)))refuse("pattern_axis_unavailable","Retained direction endpoints do not match the current edge");
  gp_Vec direction(first,last);if(direction.Magnitude()<1e-6)refuse("pattern_axis_unavailable","Direction edge is degenerate");direction.Normalize();
  if(!steps.empty()&&direction.Crossed(steps.front().Normalized()).Magnitude()<1e-8)refuse("unsupported_pattern_axes","Body Pattern requires nonparallel directions");
  steps.push_back(direction*a["spacingMm"].toDouble());auto r=a;r["direction"]=QJsonArray{direction.X(),direction.Y(),direction.Z()};resolved.append(r);
 }
 const int m=axes[0].toObject()["quantity"].toInt(),n=axes.size()==2?axes[1].toObject()["quantity"].toInt():1;std::vector<EvaluatedBody> result;
 for(int j=0;j<n;++j)for(int i=0;i<m;++i){if(!i&&!j){result.push_back(source);continue;}const auto displacement=steps[0]*i+(steps.size()==2?steps[1]*j:gp_Vec());gp_Trsf translation;translation.SetTranslation(displacement);
  auto body=SolidTransform::copy(source,translation,feature["id"].toString(),bodyId(feature["id"].toString(),source.bodyId,i,j),QString("%1 (%2,%3)").arg(feature["name"].toString()).arg(i).arg(j),feature["visible"].toBool(true),QByteArray("alloy-rectangular-body-pattern/1/")+QByteArray::number(i)+'/'+QByteArray::number(j)+'/',"rectangularBodyPattern","patternedSourceEdgeId",false);
  body.summary["rectangularBodyPattern"]=QJsonObject{{"featureId",feature["id"]},{"sourceBodyId",source.bodyId},{"i",i},{"j",j},{"axes",resolved},{"translationMm",QJsonArray{displacement.X(),displacement.Y(),displacement.Z()}}};result.push_back(std::move(body));
 }return result;
}
