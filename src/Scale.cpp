// SPDX-License-Identifier: MIT
#include "Scale.h"
#include "PlanarFaces.h"
#include "SolidFillet.h"
#include <BRepBuilderAPI_Transform.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <QUuid>
#include <cmath>
namespace {
void reject(const QString& code,const QString& message){throw GeometryKernel::EvaluationError({{"code",code},{"message",message}});}
QJsonArray position(const QJsonArray& a,double factor){if(a.size()!=3)reject("scale_position_unavailable","Scale requires exact retained XYZ positional provenance");QJsonArray result;for(const auto& v:a){if(!v.isDouble() || !std::isfinite(v.toDouble()*factor))reject("scale_position_unavailable","Scale position must remain finite");result.append(v.toDouble()*factor);}return result;}
QJsonObject provenance(QJsonObject p,double factor){for(const auto* field:{"startMm","endMm","centerMm"})if(p.contains(field))p[field]=position(p.value(field).toArray(),factor);return p;}
TopoDS_Shape mapped(BRepBuilderAPI_Transform& builder,const TopoDS_Shape& source,const TopTools_IndexedMapOfShape& present,TopAbs_ShapeEnum kind){
    TopTools_IndexedMapOfShape choices;const auto direct=builder.ModifiedShape(source);
    if(!direct.IsNull() && direct.ShapeType()==kind && present.Contains(direct))choices.Add(direct);
    for(const auto& s:builder.Modified(source))if(s.ShapeType()==kind && present.Contains(s))choices.Add(s);
    if(choices.Extent()!=1)reject("scale_lineage_ambiguous","A retained reference has no unique exact scaled descendant");return choices(1);
}
}
QString Scale::validate(const QJsonObject& p){
    if(p.size()!=3 || !p.value("targetBodyId").isString() || QUuid(p.value("targetBodyId").toString()).isNull() || p.value("pivot")!="documentOrigin")return "Scale requires one retained targetBodyId and explicit documentOrigin pivot";
    if(!p.value("factor").isDouble() || !std::isfinite(p.value("factor").toDouble()) || p.value("factor").toDouble()<=0)return "Scale factor must be finite and strictly positive";
    return {};
}
EvaluatedBody Scale::evaluate(const EvaluatedDocument& prior,const QJsonObject& feature,const QJsonObject& authored){
    const auto p=feature.value("parameters").toObject();const auto error=validate(p);if(!error.isEmpty())reject("invalid_scale",error);
    const EvaluatedBody* source=nullptr;for(const auto& body:prior.bodies)if(body.bodyId==p.value("targetBodyId").toString()){if(source)reject("scale_ambiguous_instance","Scale requires a unique root body");source=&body;}
    if(!source)reject("body_unavailable","Scale target must be active earlier in history");
    QString owner=source->ownerId;for(const auto& value:authored.value("features").toArray()){const auto f=value.toObject();if(f.value("id").toString()==source->featureId)owner=f.value("ownerId").toString();}
    owner=authored.value("objectOwners").toObject().value("bodies").toObject().value(source->bodyId).toString(owner);
    if(owner!=authored.value("documentId").toString() || !source->occurrenceId.isEmpty())reject("scale_root_required","Scale currently requires one root-owned solid");
    const double factor=p.value("factor").toDouble();const auto before=GeometryKernel::summarize(source->shape);
    if(!before.value("valid").toBool() || !before.value("closed").toBool() || before.value("topology").toObject().value("solids").toInt()!=1 || before.value("volumeMm3").toDouble()<=0)reject("scale_solid_required","Scale requires one valid closed solid");
    gp_Trsf transform;transform.SetScale(gp_Pnt(0,0,0),factor);
    BRepBuilderAPI_Transform builder(source->shape,transform,true,false);if(!builder.IsDone())reject("scale_failed","Scale geometry evaluation failed");
    auto result=*source;result.shape=builder.Shape();result.summary=GeometryKernel::summarize(result.shape);
    const double expected=before.value("volumeMm3").toDouble()*factor*factor*factor,volume=result.summary.value("volumeMm3").toDouble();
    if(!std::isfinite(expected) || !result.summary.value("valid").toBool() || !result.summary.value("closed").toBool() || result.summary.value("topology").toObject().value("solids").toInt()!=1 || volume<=0 || std::abs(volume-expected)>std::max(1e-6,expected*1e-8))reject("scale_invalid_result","Scale must produce one valid closed solid with factor-cubed volume");
    TopTools_IndexedMapOfShape faces,edges;TopExp::MapShapes(result.shape,TopAbs_FACE,faces);TopExp::MapShapes(result.shape,TopAbs_EDGE,edges);
    for(auto& face:result.supportFaces){face.face=TopoDS::Face(mapped(builder,face.face,faces,TopAbs_FACE));face.frame["origin"]=position(face.frame.value("origin").toArray(),factor);face.provenance=provenance(face.provenance,factor);
        // Positive uniform scale leaves the normalized retained U/V/normal axes unchanged.
        for(auto& edge:face.boundaryEdges){edge.edge=TopoDS::Edge(mapped(builder,edge.edge,edges,TopAbs_EDGE));edge.provenance=provenance(edge.provenance,factor);}}
    for(auto& edge:result.solidEdges){if(edge.edge.IsNull())continue;edge.edge=TopoDS::Edge(mapped(builder,edge.edge,edges,TopAbs_EDGE));edge.provenance=provenance(edge.provenance,factor);}
    result.summary["lastModifierFeatureId"]=feature.value("id");result.summary["operation"]="scale";result.summary["scale"]=p;
    result.summary["supportFaces"]=PlanarFaces::descriptors(result.supportFaces);result.summary["solidEdges"]=SolidFillet::descriptors(result.solidEdges);return result;
}
