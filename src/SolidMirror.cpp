#include "SolidMirror.h"
#include "SolidFillet.h"
#include "PlanarFaces.h"
#include <BRepBuilderAPI_Transform.hxx>
#include <gp_Ax2.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <QUuid>
#include <cmath>

namespace {
QString derivedId(const QString& featureId,const QByteArray& kind,const QString& sourceId) {
    return QUuid::createUuidV5(QUuid(featureId),QByteArray("alloy-mirror/1/")+kind+'/'+sourceId.toUtf8()).toString(QUuid::WithoutBraces);
}
gp_Pnt asPoint(const QJsonArray& array){return {array[0].toDouble(),array[1].toDouble(),array[2].toDouble()};}
gp_Dir asDirection(const QJsonArray& array){return {array[0].toDouble(),array[1].toDouble(),array[2].toDouble()};}
QJsonArray values(const gp_Pnt& p){return {p.X(),p.Y(),p.Z()};}
QJsonArray values(const gp_Dir& d){return {d.X(),d.Y(),d.Z()};}
QJsonObject reflectedFrame(const QJsonObject& source,const gp_Trsf& reflection){
    auto origin=asPoint(source["origin"].toArray());origin.Transform(reflection);
    auto normal=asDirection(source["normal"].toArray());normal.Transform(reflection);
    auto u=asDirection(source["uAxis"].toArray());u.Transform(reflection);
    const auto v=normal.Crossed(u);
    return {{"origin",values(origin)},{"normal",values(normal)},{"uAxis",values(u)},{"vAxis",values(v)}};
}
QJsonObject reflectedProvenance(QJsonObject source,const gp_Trsf& reflection){
    for(const auto& field:{"startMm","endMm","centerMm"})if(source[field].isArray()){
        const auto coordinates=source[field].toArray();if(coordinates.size()!=3)continue;
        auto point=asPoint(coordinates);point.Transform(reflection);source[field]=values(point);
    }
    return source;
}
TopoDS_Shape uniqueMapped(BRepBuilderAPI_Transform& builder,const TopoDS_Shape& source,
    const TopTools_IndexedMapOfShape& present,TopAbs_ShapeEnum kind){
    TopTools_IndexedMapOfShape choices;
    for(const auto& descendant:builder.Modified(source))if(descendant.ShapeType()==kind && present.Contains(descendant))choices.Add(descendant);
    const auto direct=builder.ModifiedShape(source);
    if(!direct.IsNull() && direct.ShapeType()==kind && present.Contains(direct))choices.Add(direct);
    return choices.Extent()==1 ? choices(1) : TopoDS_Shape{};
}
}
EvaluatedBody SolidMirror::evaluate(const EvaluatedBody& source,const EvaluatedSupportFace& plane,
    const QString& featureId,const QString& bodyId,const QString& name,bool visible){
    auto fail=[&](const QString& code,const QString& message){throw GeometryKernel::EvaluationError({
        {"code",code},{"message",message},{"featureId",featureId},{"sourceBodyId",source.bodyId},
        {"mirrorPlane",QJsonObject{{"producerFeatureId",plane.producerFeatureId},{"bodyId",plane.bodyId},{"faceId",plane.faceId}}}});};
    const auto frame=plane.frame;const auto origin=frame["origin"].toArray(),normal=frame["normal"].toArray();
    if(origin.size()!=3 || normal.size()!=3)fail("mirror_plane_invalid","Mirror plane has no resolved frame");
    gp_Trsf reflection;reflection.SetMirror(gp_Ax2(asPoint(origin),asDirection(normal)));
    BRepBuilderAPI_Transform builder(source.shape,reflection,true);
    if(!builder.IsDone())fail("mirror_failed","Body reflection failed");
    const auto result=builder.Shape();auto summary=GeometryKernel::summarize(result);
    const auto sourceVolume=source.summary["volumeMm3"].toDouble(),resultVolume=summary["volumeMm3"].toDouble();
    if(!summary["valid"].toBool() || !summary["closed"].toBool()
        || summary["topology"].toObject()["solids"].toInt()!=1 || resultVolume<=0
        || std::abs(resultVolume-sourceVolume)>std::max(1e-5,sourceVolume*1e-8))
        fail("mirror_invalid_result","Mirror must preserve one valid closed solid and its volume");
    TopTools_IndexedMapOfShape faces,edges;TopExp::MapShapes(result,TopAbs_FACE,faces);TopExp::MapShapes(result,TopAbs_EDGE,edges);
    std::vector<EvaluatedSupportFace> supportFaces;
    for(const auto& original:source.supportFaces){
        const auto mapped=uniqueMapped(builder,original.face,faces,TopAbs_FACE);
        if(mapped.IsNull())fail("mirror_face_ambiguous","A named source face has no unique reflected descendant");
        auto face=original;face.face=TopoDS::Face(mapped);face.faceId=derivedId(featureId,"face",original.faceId);
        face.producerFeatureId=featureId;face.bodyId=bodyId;face.role="mirror."+original.role;
        face.frame=reflectedFrame(original.frame,reflection);
        std::vector<EvaluatedSupportEdge> boundaries;
        for(auto edge:original.boundaryEdges){
            const auto descendant=uniqueMapped(builder,edge.edge,edges,TopAbs_EDGE);
            if(descendant.IsNull())continue;
            const auto prior=edge.edgeId;edge.edge=TopoDS::Edge(descendant);
            edge.edgeId=derivedId(featureId,"support-edge",prior);
            edge.provenance=reflectedProvenance(edge.provenance,reflection);
            edge.provenance["mirroredSourceEdgeId"]=prior;boundaries.push_back(edge);
        }
        face.boundaryEdges=boundaries;supportFaces.push_back(face);
    }
    std::vector<EvaluatedSolidEdge> solidEdges;
    for(auto edge:source.solidEdges){
        if(edge.edge.IsNull())continue;
        const auto descendant=uniqueMapped(builder,edge.edge,edges,TopAbs_EDGE);
        if(descendant.IsNull())fail("mirror_edge_ambiguous","A named source edge has no unique reflected descendant");
        const auto prior=edge.edgeId;edge.edge=TopoDS::Edge(descendant);
        edge.edgeId=derivedId(featureId,"solid-edge",prior);
        edge.provenance=reflectedProvenance(edge.provenance,reflection);
        edge.provenance["mirroredSourceEdgeId"]=prior;edge.provenance["generatorFeatureId"]=featureId;
        solidEdges.push_back(edge);
    }
    summary["operation"]="mirror";summary["sourceBodyId"]=source.bodyId;
    summary["mirrorPlane"]=QJsonObject{{"producerFeatureId",plane.producerFeatureId},{"bodyId",plane.bodyId},{"faceId",plane.faceId}};
    summary["supportFaces"]=PlanarFaces::descriptors(supportFaces);
    summary["solidEdges"]=SolidFillet::descriptors(solidEdges);
    return {featureId,bodyId,name,visible,result,summary,supportFaces,solidEdges};
}
