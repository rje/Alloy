// SPDX-License-Identifier: MIT
#include "SolidFaceOffset.h"
#include "PlanarFaces.h"
#include <BRepOffset_MakeOffset.hxx>
#include <BRepAlgo_Image.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <QUuid>
#include <cmath>

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
    auto descendant=[&](const TopoDS_Face& face){
        TopTools_ListOfShape images;operation.OffsetFacesFromShapes().LastImage(face,images);
        TopTools_IndexedMapOfShape matches;
        if(resultFaces.Contains(face))matches.Add(resultFaces(resultFaces.FindIndex(face)));
        for(const auto& image:images)if(image.ShapeType()==TopAbs_FACE&&resultFaces.Contains(image))matches.Add(resultFaces(resultFaces.FindIndex(image)));
        return matches.Extent()==1?TopoDS::Face(matches(1)):TopoDS_Face{};
    };
    const auto moved=descendant(original);
    if(moved.IsNull())fail("face_offset_face_ambiguous","Offset Face must retain one exact kernel descendant of its selected face");
    const BRepAdaptor_Surface movedSurface(moved);
    auto movedNormal=movedSurface.GetType()==GeomAbs_Plane?movedSurface.Plane().Axis().Direction():normal;
    if(moved.Orientation()==TopAbs_REVERSED)movedNormal.Reverse();
    if(movedSurface.GetType()!=GeomAbs_Plane||movedSurface.Plane().Distance(expected)>1e-5||movedNormal.Dot(normal)<1-1e-7)
        fail("face_offset_plane_mismatch","The resulting face does not match the requested signed planar offset");
    std::vector<EvaluatedSupportFace> retained;
    for(const auto& source:body.supportFaces){
        const auto next=descendant(source.face);if(next.IsNull())continue;
        auto face=source;face.face=next;
        // Offset history does not yet qualify boundary-edge identities. Never
        // retain stale edges or assign replacements by coordinate proximity.
        face.boundaryEdges.clear();
        if(source.face.IsSame(original)){
            face.producerFeatureId=id;face.role="faceOffset.end";
            face.faceId=QUuid::createUuidV5(QUuid(id),QByteArray("alloy-face-offset-end/1\n")+source.producerFeatureId.toUtf8()+'\n'+source.faceId.toUtf8()).toString(QUuid::WithoutBraces);
            face.frame["origin"]=QJsonArray{expected.X(),expected.Y(),expected.Z()};
            face.frame["normal"]=QJsonArray{normal.X(),normal.Y(),normal.Z()};
            face.provenance={{"role","faceOffset.end"},{"generatorFeatureId",id},{"sourceFace",ref}};
        }
        retained.push_back(face);
    }
    summary["supportFaces"]=PlanarFaces::descriptors(retained);summary["solidEdges"]=QJsonArray{};
    summary["lastModifierFeatureId"]=id;summary["producerFeatureId"]=body.featureId;
    summary["operation"]="faceOffset";summary["faceOffset"]=p;
    body.shape=result;body.supportFaces=std::move(retained);body.solidEdges.clear();body.summary=summary;
}
