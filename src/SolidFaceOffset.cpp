// SPDX-License-Identifier: MIT
#include "SolidFaceOffset.h"
#include "PlanarFaces.h"
#include "SolidFillet.h"
#include <BRepOffset_MakeOffset.hxx>
#include <BRepAlgo_Image.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRep_Tool.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <QSet>
#include <QUuid>
#include <algorithm>
#include <cmath>

namespace {
QJsonArray coordinates(const gp_Pnt& point) {return {point.X(),point.Y(),point.Z()};}
bool finite(const gp_Pnt& point) {
    return std::isfinite(point.X()) && std::isfinite(point.Y()) && std::isfinite(point.Z());
}
}

void SolidFaceOffset::evaluate(EvaluatedBody& body,const QJsonObject& p,const QString& id) {
    const auto ref=p["face"].toObject();
    auto fail=[&](const QString& code,const QString& message){throw GeometryKernel::EvaluationError(
        {{"code",code},{"message",message},{"featureId",id},{"featureType","faceOffset"},{"face",ref},{"targetBodyId",body.bodyId}});};
    const double distance=p["distanceMm"].toDouble();
    if(!std::isfinite(distance)||std::abs(distance)<0.001||std::abs(distance)>100000)
        fail("face_offset_distance_invalid","Offset Face requires a nonzero signed distance of 0.001..100000 mm");
    const auto before=GeometryKernel::summarize(body.shape);
    if(!body.occurrenceId.isEmpty()||!before["valid"].toBool()||!before["closed"].toBool()
        ||before["topology"].toObject()["solids"].toInt()!=1)
        fail("face_offset_source_unsupported","Offset Face requires one valid closed local solid");
    const EvaluatedSupportFace* selected=nullptr;
    for(const auto& face:body.supportFaces)if(face.bodyId==ref["bodyId"]&&face.producerFeatureId==ref["producerFeatureId"]&&face.faceId==ref["faceId"]){
        if(selected)fail("face_offset_face_ambiguous","The retained face has multiple current descendants");selected=&face;}
    TopTools_IndexedMapOfShape initialFaces;TopExp::MapShapes(body.shape,TopAbs_FACE,initialFaces);
    if(!selected||selected->face.IsNull()||!initialFaces.Contains(selected->face))
        fail("face_offset_face_unavailable","The exact retained face is unavailable on this body");
    const auto original=TopoDS::Face(initialFaces(initialFaces.FindIndex(selected->face)));
    const BRepAdaptor_Surface surface(original);
    if(surface.GetType()!=GeomAbs_Plane)fail("face_offset_face_unsupported","Offset Face currently requires one planar solid face");
    auto normal=surface.Plane().Axis().Direction();if(original.Orientation()==TopAbs_REVERSED)normal.Reverse();
    const auto origin=selected->frame["origin"].toArray();
    if(origin.size()!=3)fail("face_offset_frame_unavailable","The retained planar frame is unavailable");
    const gp_Pnt expected=gp_Pnt(origin[0].toDouble(),origin[1].toDouble(),origin[2].toDouble()).Translated(gp_Vec(normal)*distance);
    BRepOffset_MakeOffset operation;
    try {operation.Initialize(body.shape,0,1e-6,BRepOffset_Skin,false,false,GeomAbs_Intersection);
        operation.SetOffsetOnFace(original,distance);operation.MakeOffsetShape();}
    catch(const Standard_Failure&){fail("face_offset_failed","The selected face cannot be offset at this distance");}
    if(!operation.IsDone()||operation.Shape().IsNull())fail("face_offset_failed","Offset Face did not produce a solid");
    const auto result=operation.Shape();QJsonObject summary;
    try {summary=GeometryKernel::summarize(result);}
    catch(const Standard_Failure&){fail("face_offset_invalid_result","Offset Face produced geometry that cannot be validated");}
    if(!summary["valid"].toBool()||!summary["closed"].toBool()||summary["topology"].toObject()["solids"].toInt()!=1
        ||!std::isfinite(summary["volumeMm3"].toDouble())||summary["volumeMm3"].toDouble()<=1e-9)
        fail("face_offset_invalid_result","Offset Face must produce one valid closed positive-volume solid");
    const double delta=summary["volumeMm3"].toDouble()-before["volumeMm3"].toDouble();
    if(delta*distance<=0||std::abs(delta)<1e-6)fail("face_offset_no_change","Offset Face must change volume in the requested direction");
    TopTools_IndexedMapOfShape resultFaces;TopExp::MapShapes(result,TopAbs_FACE,resultFaces);
    auto faceDescendants=[&](const TopoDS_Face& face){
        TopTools_IndexedMapOfShape matches;
        if(face.IsNull())return matches;
        TopTools_ListOfShape images;operation.OffsetFacesFromShapes().LastImage(face,images);
        if(resultFaces.Contains(face))matches.Add(resultFaces(resultFaces.FindIndex(face)));
        for(const auto& image:images)if(image.ShapeType()==TopAbs_FACE&&resultFaces.Contains(image))matches.Add(resultFaces(resultFaces.FindIndex(image)));
        return matches;
    };
    auto descendant=[&](const TopoDS_Face& face){
        const auto matches=faceDescendants(face);
        return matches.Extent()==1?TopoDS::Face(matches(1)):TopoDS_Face{};
    };
    const auto moved=descendant(original);
    if(moved.IsNull())fail("face_offset_face_ambiguous","Offset Face must retain one exact kernel descendant of its selected face");
    const BRepAdaptor_Surface movedSurface(moved);
    auto movedNormal=movedSurface.GetType()==GeomAbs_Plane?movedSurface.Plane().Axis().Direction():normal;
    if(moved.Orientation()==TopAbs_REVERSED)movedNormal.Reverse();
    if(movedSurface.GetType()!=GeomAbs_Plane||movedSurface.Plane().Distance(expected)>1e-5||movedNormal.Dot(normal)<1-1e-7)
        fail("face_offset_plane_mismatch","The resulting face does not match the requested signed planar offset");

    // Resolve the complete source topology, not just the named catalog: an
    // unnamed edge can also contribute to an ambiguous merged descendant.
    TopTools_IndexedMapOfShape initialEdges,resultEdges;
    TopExp::MapShapes(body.shape,TopAbs_EDGE,initialEdges);TopExp::MapShapes(result,TopAbs_EDGE,resultEdges);
    std::vector<TopTools_IndexedMapOfShape> edgeImages(initialEdges.Extent()+1);
    std::vector<int> edgeOwners(resultEdges.Extent()+1,0);
    for(int i=1;i<=initialEdges.Extent();++i) {
        auto& matches=edgeImages[i];const auto& source=initialEdges(i);
        const auto offer=[&](const TopoDS_Shape& image) {
            if(image.ShapeType()==TopAbs_EDGE && resultEdges.Contains(image))
                matches.Add(resultEdges(resultEdges.FindIndex(image)));
        };
        if(resultEdges.Contains(source))offer(source);
        TopTools_ListOfShape images;operation.OffsetEdgesFromShapes().LastImage(source,images);
        for(const auto& image:images)offer(image);
        for(const auto& image:operation.Generated(source))offer(image);
        for(int j=1;j<=matches.Extent();++j)++edgeOwners[resultEdges.FindIndex(matches(j))];
    }
    std::vector<TopoDS_Face> faceImages(initialFaces.Extent()+1);
    std::vector<int> faceOwners(resultFaces.Extent()+1,0);
    for(int i=1;i<=initialFaces.Extent();++i) {
        const auto matches=faceDescendants(TopoDS::Face(initialFaces(i)));
        if(matches.Extent()==1)faceImages[i]=TopoDS::Face(matches(1));
        for(int j=1;j<=matches.Extent();++j)++faceOwners[resultFaces.FindIndex(matches(j))];
    }
    TopTools_IndexedDataMapOfShapeListOfShape initialIncidence,resultIncidence;
    TopExp::MapShapesAndAncestors(body.shape,TopAbs_EDGE,TopAbs_FACE,initialIncidence);
    TopExp::MapShapesAndAncestors(result,TopAbs_EDGE,TopAbs_FACE,resultIncidence);
    const auto incidentFaces=[](const TopTools_IndexedDataMapOfShapeListOfShape& incidence,const TopoDS_Edge& edge) {
        TopTools_IndexedMapOfShape faces;
        if(incidence.Contains(edge))for(const auto& face:incidence.FindFromKey(edge))faces.Add(face);
        return faces;
    };
    const auto qualifyStraight=[&](const TopoDS_Edge& source,const TopoDS_Edge& next,QJsonObject& provenance)->QString {
        const BRepAdaptor_Curve beforeCurve(source),afterCurve(next);
        if(beforeCurve.GetType()!=GeomAbs_Line || afterCurve.GetType()!=GeomAbs_Line)
            return "modified_curve_by_face_offset";
        const auto beforeFaces=incidentFaces(initialIncidence,source),afterFaces=incidentFaces(resultIncidence,next);
        if(beforeFaces.Extent()!=2 || afterFaces.Extent()!=2)return "face_offset_incidence_unqualified";
        auto a=afterCurve.Value(afterCurve.FirstParameter()),b=afterCurve.Value(afterCurve.LastParameter());
        if(!finite(a) || !finite(b) || a.Distance(b)<1e-6)return "face_offset_endpoints_unqualified";
        TopoDS_Vertex first,last;TopExp::Vertices(next,first,last);
        if(first.IsNull() || last.IsNull())return "face_offset_endpoints_unqualified";
        const auto firstPoint=BRep_Tool::Pnt(first),lastPoint=BRep_Tool::Pnt(last);
        if(!((a.Distance(firstPoint)<=1e-5 && b.Distance(lastPoint)<=1e-5)
            || (a.Distance(lastPoint)<=1e-5 && b.Distance(firstPoint)<=1e-5)))
            return "face_offset_endpoints_unqualified";
        TopTools_IndexedMapOfShape mappedFaces;
        for(int i=1;i<=beforeFaces.Extent();++i) {
            const auto sourceFace=TopoDS::Face(beforeFaces(i));
            const auto faceIndex=initialFaces.FindIndex(sourceFace);
            if(!faceIndex || faceImages[faceIndex].IsNull())return "face_offset_incidence_unqualified";
            const auto& nextFace=faceImages[faceIndex];
            if(faceOwners[resultFaces.FindIndex(nextFace)]!=1 || !afterFaces.Contains(nextFace))
                return "face_offset_incidence_unqualified";
            mappedFaces.Add(nextFace);
            const BRepAdaptor_Surface beforeSurface(sourceFace),afterSurface(nextFace);
            if(beforeSurface.GetType()!=GeomAbs_Plane || afterSurface.GetType()!=GeomAbs_Plane)
                return "face_offset_incidence_unqualified";
            const auto oldPlane=beforeSurface.Plane(),newPlane=afterSurface.Plane();
            const auto expectedOrigin=sourceFace.IsSame(original)
                ? oldPlane.Location().Translated(gp_Vec(normal)*distance) : oldPlane.Location();
            if(std::abs(oldPlane.Axis().Direction().Dot(newPlane.Axis().Direction()))<1-1e-9
                || newPlane.Distance(expectedOrigin)>1e-5 || newPlane.Distance(a)>1e-5 || newPlane.Distance(b)>1e-5)
                return "face_offset_plane_mismatch";
        }
        if(mappedFaces.Extent()!=2)return "face_offset_incidence_unqualified";
        auto oldA=beforeCurve.Value(beforeCurve.FirstParameter()),oldB=beforeCurve.Value(beforeCurve.LastParameter());
        const auto start=provenance["startMm"].toArray(),end=provenance["endMm"].toArray();
        if(start.size()==3 && end.size()==3) {
            oldA=gp_Pnt(start[0].toDouble(),start[1].toDouble(),start[2].toDouble());
            oldB=gp_Pnt(end[0].toDouble(),end[1].toDouble(),end[2].toDouble());
            if(!finite(oldA) || !finite(oldB) || beforeCurve.Line().Distance(oldA)>1e-5
                || beforeCurve.Line().Distance(oldB)>1e-5)return "face_offset_direction_unqualified";
        }
        const gp_Vec oldDirection(oldA,oldB),newDirection(a,b);
        if(oldDirection.Magnitude()<1e-6
            || oldDirection.Crossed(newDirection).Magnitude()>1e-8*oldDirection.Magnitude()*newDirection.Magnitude())
            return "face_offset_direction_unqualified";
        if(oldDirection.Dot(newDirection)<0)std::swap(a,b);
        if(provenance.contains("startContact") || provenance.contains("endContact")) {
            provenance["sourceBoundaryContacts"]=QJsonObject{{"start",provenance["startContact"]},{"end",provenance["endContact"]}};
            provenance.remove("startContact");provenance.remove("endContact");
        }
        provenance["endpointProvenance"]="currentKernelBoundary";
        provenance["lineage"]="uniqueOffsetStraight";
        provenance["startMm"]=coordinates(a);provenance["endMm"]=coordinates(b);
        return {};
    };
    const auto resolveEdge=[&](const TopoDS_Edge& source,QJsonObject& provenance) {
        const auto reject=[&](const QString& error) {provenance["resolutionError"]=error;return TopoDS_Edge{};};
        if(provenance["resolutionError"].toString().startsWith("ambiguous"))
            return reject(provenance["resolutionError"].toString());
        if(source.IsNull())return reject(provenance["resolutionError"].toString("unavailable"));
        const int index=initialEdges.FindIndex(source);
        if(!index)return reject("unavailable");
        const auto& images=edgeImages[index];
        if(images.Extent()!=1)return reject(images.Extent()>1 ? "ambiguous_split" : "deleted");
        const auto next=TopoDS::Edge(images(1));
        if(edgeOwners[resultEdges.FindIndex(next)]!=1)return reject("ambiguous_merge");
        if(!next.IsSame(source)) {
            const auto error=qualifyStraight(source,next,provenance);
            if(!error.isEmpty())return reject(error);
        }
        provenance.remove("resolutionError");return next;
    };
    std::vector<EvaluatedSolidEdge> retainedEdges;
    QMap<QString,TopTools_IndexedMapOfShape> namedSources;
    QSet<QString> ambiguousIds;
    const auto registerSource=[&](const QString& edgeId,const TopoDS_Edge& edge,const QJsonObject& provenance) {
        if(provenance["resolutionError"].toString().startsWith("ambiguous"))ambiguousIds.insert(edgeId);
        if(!edge.IsNull() && namedSources[edgeId].Add(edge)>1)ambiguousIds.insert(edgeId);
    };
    for(const auto& source:body.solidEdges)registerSource(source.edgeId,source.edge,source.provenance);
    for(const auto& face:body.supportFaces)for(const auto& edge:face.boundaryEdges)
        registerSource(edge.edgeId,edge.edge,edge.provenance);
    for(auto edge:body.solidEdges) {
        if(ambiguousIds.contains(edge.edgeId)) {edge.edge.Nullify();edge.provenance["resolutionError"]="ambiguous_source";}
        else edge.edge=resolveEdge(edge.edge,edge.provenance);
        retainedEdges.push_back(edge);
    }
    std::vector<EvaluatedSupportFace> retained;
    for(const auto& source:body.supportFaces){
        const auto next=descendant(source.face);if(next.IsNull())continue;
        auto face=source;face.face=next;
        face.boundaryEdges.clear();
        TopTools_IndexedMapOfShape boundaries;TopExp::MapShapes(next,TopAbs_EDGE,boundaries);
        TopTools_IndexedMapOfShape sourceBoundaries;TopExp::MapShapes(source.face,TopAbs_EDGE,sourceBoundaries);
        for(auto edge:source.boundaryEdges) {
            if(edge.edge.IsNull() || !sourceBoundaries.Contains(edge.edge) || ambiguousIds.contains(edge.edgeId))continue;
            edge.edge=resolveEdge(edge.edge,edge.provenance);
            if(!edge.edge.IsNull() && boundaries.Contains(edge.edge))face.boundaryEdges.push_back(edge);
        }
        if(source.face.IsSame(original)){
            face.producerFeatureId=id;face.role="faceOffset.end";
            face.faceId=QUuid::createUuidV5(QUuid(id),QByteArray("alloy-face-offset-end/1\n")+source.producerFeatureId.toUtf8()+'\n'+source.faceId.toUtf8()).toString(QUuid::WithoutBraces);
            face.frame["origin"]=QJsonArray{expected.X(),expected.Y(),expected.Z()};
            face.frame["normal"]=QJsonArray{normal.X(),normal.Y(),normal.Z()};
            face.provenance={{"role","faceOffset.end"},{"generatorFeatureId",id},{"sourceFace",ref}};
        }
        retained.push_back(face);
    }
    summary["supportFaces"]=PlanarFaces::descriptors(retained);summary["solidEdges"]=SolidFillet::descriptors(retainedEdges);
    summary["lastModifierFeatureId"]=id;summary["producerFeatureId"]=body.featureId;
    summary["operation"]="faceOffset";summary["faceOffset"]=p;
    body.shape=result;body.supportFaces=std::move(retained);body.solidEdges=std::move(retainedEdges);body.summary=summary;
}
