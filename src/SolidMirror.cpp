// SPDX-License-Identifier: MIT
#include "SolidMirror.h"
#include "SolidTransform.h"
#include <gp_Ax2.hxx>
EvaluatedBody SolidMirror::evaluate(const EvaluatedBody& source,const EvaluatedSupportFace& plane,
    const QString& featureId,const QString& bodyId,const QString& name,bool visible){
    const auto frame=plane.frame;const auto origin=frame["origin"].toArray(),normal=frame["normal"].toArray();
    if(origin.size()!=3||normal.size()!=3)throw GeometryKernel::EvaluationError({{"code","mirror_plane_invalid"},{"featureId",featureId},{"message","Mirror plane has no resolved frame"}});
    gp_Trsf reflection;reflection.SetMirror(gp_Ax2(gp_Pnt(origin[0].toDouble(),origin[1].toDouble(),origin[2].toDouble()),gp_Dir(normal[0].toDouble(),normal[1].toDouble(),normal[2].toDouble())));
    EvaluatedBody result;try{result=SolidTransform::copy(source,reflection,featureId,bodyId,name,visible,"alloy-mirror/1/","mirror","mirroredSourceEdgeId");}catch(const GeometryKernel::EvaluationError& e){auto d=e.diagnostics();d["mirrorPlane"]=QJsonObject{{"producerFeatureId",plane.producerFeatureId},{"bodyId",plane.bodyId},{"faceId",plane.faceId}};throw GeometryKernel::EvaluationError(d);}
    result.summary["mirrorPlane"]=QJsonObject{{"producerFeatureId",plane.producerFeatureId},{"bodyId",plane.bodyId},{"faceId",plane.faceId}};return result;
}
