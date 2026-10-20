// SPDX-License-Identifier: MIT
#include "SurfaceOffset.h"
#include "PlanarFaces.h"
#include "SolidFillet.h"
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepOffsetAPI_MakeOffsetShape.hxx>
#include <Precision.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <QUuid>
#include <cmath>
#include <numbers>

EvaluatedSupportFace SurfaceOffset::resolveSource(const EvaluatedDocument& prior,const QJsonObject& ref){
    auto reject=[&](const QString& code,const QString& message){throw GeometryKernel::EvaluationError({{"sourceFace",ref},{"faceId",ref.value("faceId")},{"code",code},{"message",message}});};
    const EvaluatedSupportFace* source=nullptr;const EvaluatedBody* owner=nullptr;
    for(const auto& body:prior.bodies)if(body.bodyId==ref.value("bodyId").toString())for(const auto& face:body.supportFaces)
        if(face.faceId==ref.value("faceId").toString() && face.producerFeatureId==ref.value("producerFeatureId").toString()){
            if(source)reject("ambiguous_surface_source","Source face resolves more than once");source=&face;owner=&body;
        }
    if(!source || source->face.IsNull())reject("surface_source_unavailable","Select one exact earlier retained face");
    int matches=0;for(TopExp_Explorer face(owner->shape,TopAbs_FACE);face.More();face.Next())if(face.Current().IsSame(source->face))++matches;
    if(matches!=1 || !BRepCheck_Analyzer(source->face).IsValid())reject("surface_source_unavailable","Source must be one unique valid face of the retained body");
    const BRepAdaptor_Surface surface(source->face);
    if(surface.GetType()!=GeomAbs_Plane){
        TopTools_IndexedMapOfShape faces,edges;TopExp::MapShapes(owner->shape,TopAbs_FACE,faces);TopExp::MapShapes(source->face,TopAbs_EDGE,edges);
        const double span=surface.LastUParameter()-surface.FirstUParameter();
        if(surface.GetType()!=GeomAbs_Cylinder || source->role!="surfaceRevolve.sheet"
            || source->provenance.value("lineage")!="surfaceRevolveBuilder"
            || source->provenance.value("generatorFeatureId")!=source->producerFeatureId
            || owner->featureId!=source->producerFeatureId || faces.Extent()!=1 || edges.Extent()!=4
            || source->boundaryEdges.size()!=4 || !std::isfinite(span) || span<=Precision::Angular()
            || span>=2*std::numbers::pi-Precision::Angular())
            reject("unsupported_surface_source","Select one planar face or one retained partial cylindrical Surface Revolve face");
    }
    return *source;
}

EvaluatedBody SurfaceOffset::evaluate(const QJsonObject& feature,const EvaluatedDocument& prior){
    const auto p=feature.value("parameters").toObject(),ref=p.value("sourceFace").toObject();
    const auto id=feature.value("id").toString();
    auto reject=[&](const QString& code,const QString& message,QJsonObject detail=QJsonObject{}){detail["featureId"]=id;detail["featureType"]="surfaceOffset";detail["sourceFace"]=ref;detail["faceId"]=ref.value("faceId");detail["code"]=code;detail["message"]=message;throw GeometryKernel::EvaluationError(detail);};
    EvaluatedSupportFace source;try{source=resolveSource(prior,ref);}catch(const GeometryKernel::EvaluationError& e){reject(e.diagnostics().value("code").toString(),QString::fromUtf8(e.what()));}
    const BRepAdaptor_Surface surface(source.face);const bool planar=surface.GetType()==GeomAbs_Plane;
    const double distance=p.value("distanceMm").toDouble();
    if(!p.value("distanceMm").isDouble() || !std::isfinite(distance) || std::abs(distance)>100000)reject("invalid_distance","Distance must be finite and within 100000 mm");
    const double u=(surface.FirstUParameter()+surface.LastUParameter())/2.,v=(surface.FirstVParameter()+surface.LastVParameter())/2.;
    gp_Pnt sample;gp_Vec du,dv;surface.D1(u,v,sample,du,dv);auto normal=du.Crossed(dv);
    if(normal.Magnitude()<=Precision::Confusion())reject("surface_source_unavailable","Source face has no regular interior normal");
    normal.Normalize();if(source.face.Orientation()==TopAbs_REVERSED)normal.Reverse();
    double radius=0,radialSign=0,resultRadius=0;
    if(!planar){
        const auto cylinder=surface.Cylinder();radius=cylinder.Radius();
        const auto axis=cylinder.Axis();const gp_Vec toSample(axis.Location(),sample),axisDirection(axis.Direction());
        auto radial=toSample-axisDirection*toSample.Dot(axisDirection);radial.Normalize();radialSign=normal.Dot(radial);
        if(std::abs(radialSign)<1.-1e-9)reject("unsupported_surface_source","Cylindrical source normal must be regular and radial");
        radialSign=radialSign>0?1.:-1.;resultRadius=radius+radialSign*distance;
        if(resultRadius<=Precision::Confusion())reject("surface_offset_collapse","Distance collapses or inverts the cylindrical sheet; choose an inward distance smaller than its radius",{{"category","collapse"},{"sourceRadiusMm",radius},{"candidateRadiusMm",resultRadius},{"distanceMm",distance},{"normalRadialSign",radialSign}});
    }
    gp_Trsf transform;transform.SetTranslation(normal*distance);
    BRepBuilderAPI_Transform translation(transform);BRepOffsetAPI_MakeOffsetShape curved;
    if(planar){translation.Perform(source.face,true);if(!translation.IsDone())reject("surface_offset_failed","Exact planar face translation failed");}
    else{curved.PerformBySimple(source.face,distance);if(!curved.IsDone())reject("surface_offset_failed","Cylindrical face offset failed");}
    EvaluatedBody body;body.featureId=id;body.bodyId=feature.value("bodyId").toString();body.name=feature.value("name").toString();body.visible=feature.value("visible").toBool(true);body.shape=planar?translation.Shape():curved.Shape();body.summary=GeometryKernel::summarize(body.shape);
    const auto topology=body.summary.value("topology").toObject();const double area=body.summary.value("areaMm2").toDouble();
    if(!body.summary.value("valid").toBool() || body.summary.value("closed").toBool() || topology.value("faces").toInt()!=1 || topology.value("solids").toInt()!=0 || !std::isfinite(area) || area<=0)
        reject("surface_offset_invalid","Offset must be one valid positive-area open face without solids");
    auto stable=[&](const QString& role){return QUuid::createUuidV5(QUuid(id),("alloy-surface-offset/1/"+ref.value("bodyId").toString()+"/"+ref.value("producerFeatureId").toString()+"/"+ref.value("faceId").toString()+"/"+role).toUtf8()).toString(QUuid::WithoutBraces);};
    auto xyz=[](const auto& q){return QJsonArray{q.X(),q.Y(),q.Z()};};
    QJsonObject provenance{{"generatorFeatureId",id},{"sourceFace",ref},{"lineage",planar?"surfaceOffsetExactTransform":"surfaceOffsetSimpleCylinder"}};
    TopTools_IndexedMapOfShape faces,edges;TopExp::MapShapes(body.shape,TopAbs_FACE,faces);TopExp::MapShapes(body.shape,TopAbs_EDGE,edges);
    auto mapped=[&](const TopoDS_Shape& original,const TopTools_IndexedMapOfShape& present,TopAbs_ShapeEnum type){
        TopTools_IndexedMapOfShape found;
        if(planar){const auto next=translation.ModifiedShape(original);if(!next.IsNull() && next.ShapeType()==type && present.Contains(next))found.Add(next);}
        else for(const auto& next:curved.Generated(original))if(next.ShapeType()==type && present.Contains(next))found.Add(next);
        if(found.Extent()!=1)reject("surface_lineage_unavailable","Offset requires one exact kernel descendant for each source face and boundary");
        return present(present.FindIndex(found(1)));
    };
    const auto resultFace=TopoDS::Face(mapped(source.face,faces,TopAbs_FACE));
    QJsonObject frame;
    if(planar){const auto plane=surface.Plane();auto x=plane.Position().XDirection(),y=plane.Position().YDirection();if(source.face.Orientation()==TopAbs_REVERSED)y.Reverse();
        frame={{"origin",xyz(plane.Location().Transformed(transform))},{"uAxis",xyz(x)},{"vAxis",xyz(y)},{"normal",xyz(normal)}};}
    else{const BRepAdaptor_Surface result(resultFace);
        if(result.GetType()!=GeomAbs_Cylinder || std::abs(result.Cylinder().Radius()-resultRadius)>1e-6 || edges.Extent()!=4)
            reject("surface_offset_invalid","Offset must preserve the regular cylindrical sheet and its four boundaries");}
    EvaluatedSupportFace support{stable("face"),body.bodyId,id,"surfaceOffset.sheet",resultFace,frame,{},provenance};
    TopTools_IndexedMapOfShape sourceEdges,retained;TopExp::MapShapes(source.face,TopAbs_EDGE,sourceEdges);
    for(const auto& edge:source.boundaryEdges){
        if(edge.edge.IsNull() || !sourceEdges.Contains(edge.edge))reject("surface_lineage_unavailable","Source boundary is not a current source-face edge");
        const auto next=TopoDS::Edge(mapped(edge.edge,edges,TopAbs_EDGE));if(retained.Contains(next))reject("surface_lineage_unavailable","Source boundaries must map to distinct result edges");retained.Add(next);
        auto detail=provenance;detail["sourceEdgeId"]=edge.edgeId;
        if(!PlanarFaces::updateStraightBoundary(edge.edge,next,detail)){const BRepAdaptor_Curve curve(next);detail["startMm"]=xyz(curve.Value(curve.FirstParameter()));detail["endMm"]=xyz(curve.Value(curve.LastParameter()));detail["endpointProvenance"]="currentKernelBoundary";}
        const auto edgeId=stable("edge/"+edge.edgeId);support.boundaryEdges.push_back({edgeId,edge.type,next,detail});body.solidEdges.push_back({edgeId,next,detail});
    }
    if(support.boundaryEdges.size()!=static_cast<size_t>(edges.Extent()) || sourceEdges.Extent()!=edges.Extent())reject("surface_lineage_unavailable","Source face must retain each boundary edge exactly");
    body.supportFaces.push_back(support);body.summary["supportFaces"]=PlanarFaces::descriptors(body.supportFaces);body.summary["solidEdges"]=SolidFillet::descriptors(body.solidEdges);
    body.summary["sourceFace"]=ref;body.summary["distanceMm"]=distance;body.summary["sourceNormal"]=xyz(normal);body.summary["sourceNormalPointMm"]=xyz(sample);body.summary["sourceFaceOrientation"]=source.face.Orientation()==TopAbs_REVERSED?"reversed":"forward";
    if(!planar){body.summary["sourceRadiusMm"]=radius;body.summary["radiusMm"]=resultRadius;body.summary["normalRadialSign"]=radialSign;}
    body.summary["operation"]="newBody";body.summary["isSolid"]=false;return body;
}
