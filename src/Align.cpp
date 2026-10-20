// SPDX-License-Identifier: MIT
#include "Align.h"
#include "LocalReference.h"
#include <BRepAdaptor_Surface.hxx>
#include <gp_Ax3.hxx>
namespace {
void reject(const QString& code,const QString& message){throw GeometryKernel::EvaluationError({{"code",code},{"message",message}});}
gp_Ax3 axes(const QJsonObject& frame,bool flip){
    auto read=[&](const char* key){auto a=frame.value(key).toArray();if(a.size()!=3)reject("align_frame_unavailable","Align requires a retained orthonormal face frame");for(const auto& v:a)if(!v.isDouble() || !std::isfinite(v.toDouble()))reject("align_frame_unavailable","Nonfinite Align frame");return gp_Vec(a[0].toDouble(),a[1].toDouble(),a[2].toDouble());};
    const auto o=read("origin"),u=read("uAxis"),v=read("vAxis"),n=read("normal");
    if(std::abs(u.Magnitude()-1)>1e-8 || std::abs(v.Magnitude()-1)>1e-8 || std::abs(n.Magnitude()-1)>1e-8 || u.Crossed(v).Subtracted(n).Magnitude()>1e-8)reject("align_frame_unavailable","Align face has no unique retained right-handed frame");
    return gp_Ax3(gp_Pnt(o.X(),o.Y(),o.Z()),gp_Dir(flip ? -n : n),gp_Dir(u));
}
}
QString Align::validate(const QJsonObject& p){
    if(p.size()!=4 || !p.value("flip").isBool() || p.value("frameRule")!="retained-face-uv-v1")return "Align requires sourceFace, targetFace, flip and retained-face-uv-v1 frame rule";
    for(const auto* key:{"sourceFace","targetFace"}){const auto f=p.value(key).toObject();if(f.size()!=3)return "Align needs exact retained planar face references";for(const auto* field:{"producerFeatureId","bodyId","faceId"})if(!f.value(field).isString() || QUuid(f.value(field).toString()).isNull())return "Align face references need stable UUIDs";}
    if(p.value("sourceFace").toObject().value("bodyId")==p.value("targetFace").toObject().value("bodyId"))return "Align source and target bodies must be distinct";
    return {};
}
EvaluatedBody Align::evaluate(const EvaluatedDocument& prior,const QJsonObject& feature,const QJsonObject& authored){
    const auto p=feature.value("parameters").toObject();const auto error=validate(p);if(!error.isEmpty())reject("invalid_align",error);
    auto find=[&](const QJsonObject& ref)->const EvaluatedBody& {const EvaluatedBody* found=nullptr;for(const auto& b:prior.bodies)if(b.bodyId==ref.value("bodyId").toString()){if(found)reject("ambiguous_instance","Align needs unique root solids");found=&b;}if(!found)reject("body_unavailable","Align body must be active earlier in history");QString owner=found->ownerId;for(const auto& value:authored.value("features").toArray()){const auto f=value.toObject();if(f.value("id").toString()==found->featureId)owner=f.value("ownerId").toString();}owner=authored.value("objectOwners").toObject().value("bodies").toObject().value(found->bodyId).toString(owner);if(owner!=authored.value("documentId").toString() || !found->occurrenceId.isEmpty())reject("align_root_required","Align currently requires two root-owned solids");const auto s=GeometryKernel::summarize(found->shape);if(!s.value("valid").toBool() || !s.value("closed").toBool() || s.value("topology").toObject().value("solids").toInt()!=1 || s.value("volumeMm3").toDouble()<=0)reject("align_solid_required","Align requires one valid closed solid per reference");return *found;};
    const auto sourceRef=p.value("sourceFace").toObject(),targetRef=p.value("targetFace").toObject();const auto& source=find(sourceRef);find(targetRef);
    const auto from=GeometryKernel::resolveSketchSupport(prior,sourceRef),to=GeometryKernel::resolveSketchSupport(prior,targetRef);
    gp_Trsf transform;transform.SetDisplacement(axes(from.frame,false),axes(to.frame,p.value("flip").toBool()));
    auto result=LocalReference::placed(source,transform);result.summary["lastModifierFeatureId"]=feature.value("id");result.summary["operation"]="align";
    QJsonArray matrix;for(int row=1;row<=3;++row){QJsonArray values;for(int col=1;col<=4;++col)values.append(transform.Value(row,col));matrix.append(values);}matrix.append(QJsonArray{0,0,0,1});
    result.summary["align"]=QJsonObject{{"sourceFrame",from.frame},{"targetFrame",to.frame},{"flip",p.value("flip")},{"frameRule",p.value("frameRule")},{"transform",matrix}};
    return result;
}
