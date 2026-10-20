// SPDX-License-Identifier: MIT
#include "SurfaceOffset.h"
#include "PlanarFaces.h"
#include "SolidFillet.h"
#include <BRepAdaptor_Surface.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <QUuid>
#include <cmath>
EvaluatedBody SurfaceOffset::evaluate(const QJsonObject& feature,const EvaluatedDocument& prior){
    const auto p=feature.value("parameters").toObject(),ref=p.value("sourceFace").toObject();
    const auto id=feature.value("id").toString();
    auto reject=[&](const QString& code,const QString& message){throw GeometryKernel::EvaluationError({{"featureId",id},{"featureType","surfaceOffset"},{"sourceFace",ref},{"faceId",ref.value("faceId")},{"code",code},{"message",message}});};
    const EvaluatedSupportFace* source=nullptr;
    for(const auto& body:prior.bodies)if(body.bodyId==ref.value("bodyId").toString())for(const auto& face:body.supportFaces)
        if(face.faceId==ref.value("faceId").toString() && face.producerFeatureId==ref.value("producerFeatureId").toString()){
            if(source)reject("ambiguous_surface_source","Source face resolves more than once");source=&face;
        }
    if(!source || source->face.IsNull())reject("surface_source_unavailable","Select one exact earlier retained face");
    const BRepAdaptor_Surface surface(source->face);
    if(surface.GetType()!=GeomAbs_Plane)reject("unsupported_surface_source","Surface Offset currently supports regular planar faces only; curved offset and collapse validation remain unavailable");
    const double distance=p.value("distanceMm").toDouble();
    if(!p.value("distanceMm").isDouble() || !std::isfinite(distance) || std::abs(distance)>100000)reject("invalid_distance","Distance must be finite and within 100000 mm");
    auto plane=surface.Plane();auto normal=plane.Axis().Direction();if(source->face.Orientation()==TopAbs_REVERSED)normal.Reverse();
    gp_Trsf transform;transform.SetTranslation(gp_Vec(normal)*distance);
    BRepBuilderAPI_Transform offset(source->face,transform,true);
    if(!offset.IsDone())reject("surface_offset_failed","Exact planar face translation failed");
    EvaluatedBody body;body.featureId=id;body.bodyId=feature.value("bodyId").toString();body.name=feature.value("name").toString();body.visible=feature.value("visible").toBool(true);body.shape=offset.Shape();body.summary=GeometryKernel::summarize(body.shape);
    const auto topology=body.summary.value("topology").toObject();
    if(!body.summary.value("valid").toBool() || body.summary.value("closed").toBool() || topology.value("faces").toInt()!=1 || topology.value("solids").toInt()!=0 || body.summary.value("areaMm2").toDouble()<=0)
        reject("surface_offset_invalid","Offset must be one valid positive-area open face without solids");
    auto stable=[&](const QString& role){return QUuid::createUuidV5(QUuid(id),("alloy-surface-offset/1/"+ref.value("bodyId").toString()+"/"+ref.value("producerFeatureId").toString()+"/"+ref.value("faceId").toString()+"/"+role).toUtf8()).toString(QUuid::WithoutBraces);};
    auto xyz=[](const auto& v){return QJsonArray{v.X(),v.Y(),v.Z()};};
    QJsonObject provenance{{"generatorFeatureId",id},{"sourceFace",ref},{"lineage","surfaceOffsetExactTransform"}};
    const auto resultFace=TopoDS::Face(offset.ModifiedShape(source->face));
    if(resultFace.IsNull())reject("surface_lineage_unavailable","Exact source-face transform lineage unavailable");
    auto u=plane.Position().XDirection(),v=plane.Position().YDirection();if(source->face.Orientation()==TopAbs_REVERSED)v.Reverse();
    const QJsonObject frame{{"origin",xyz(plane.Location().Transformed(transform))},{"uAxis",xyz(u)},{"vAxis",xyz(v)},{"normal",xyz(normal)}};
    EvaluatedSupportFace support{stable("face"),body.bodyId,id,"surfaceOffset.sheet",resultFace,frame,{},provenance};
    TopTools_IndexedMapOfShape edges;TopExp::MapShapes(body.shape,TopAbs_EDGE,edges);
    for(const auto& edge:source->boundaryEdges){
        const auto mapped=offset.ModifiedShape(edge.edge);
        if(mapped.IsNull() || mapped.ShapeType()!=TopAbs_EDGE || !edges.Contains(mapped))reject("surface_lineage_unavailable","Exact source boundary transform unavailable");
        const auto next=TopoDS::Edge(mapped);auto detail=provenance;detail["sourceEdgeId"]=edge.edgeId;
        PlanarFaces::updateStraightBoundary(edge.edge,next,detail);const auto edgeId=stable("edge/"+edge.edgeId);
        support.boundaryEdges.push_back({edgeId,edge.type,next,detail});body.solidEdges.push_back({edgeId,next,detail});
    }
    if(support.boundaryEdges.size()!=static_cast<size_t>(edges.Extent()))reject("surface_lineage_unavailable","Source face must retain each boundary edge exactly");
    body.supportFaces.push_back(support);body.summary["supportFaces"]=PlanarFaces::descriptors(body.supportFaces);body.summary["solidEdges"]=SolidFillet::descriptors(body.solidEdges);
    body.summary["sourceFace"]=ref;body.summary["distanceMm"]=distance;body.summary["sourceNormal"]=xyz(normal);body.summary["operation"]="newBody";body.summary["isSolid"]=false;return body;
}
