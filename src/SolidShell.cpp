#include "SolidShell.h"
#include "SolidFillet.h"
#include <BRepOffsetAPI_MakeThickSolid.hxx>
#include <BRepOffset_MakeOffset.hxx>
#include <BRepAlgo_Image.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopoDS.hxx>
#include <GeomAbs_JoinType.hxx>
#include <GeomAbs_SurfaceType.hxx>
#include <algorithm>
#include <cmath>
#include <QUuid>

namespace {
gp_Dir direction(const QJsonArray& coordinates) {
    return gp_Dir(coordinates[0].toDouble(),coordinates[1].toDouble(),coordinates[2].toDouble());
}
gp_Pnt point(const QJsonArray& coordinates) {
    return gp_Pnt(coordinates[0].toDouble(),coordinates[1].toDouble(),coordinates[2].toDouble());
}
QJsonArray coordinates(const gp_Pnt& value) {return {value.X(),value.Y(),value.Z()};}

std::vector<EvaluatedSupportFace> innerFaces(const EvaluatedBody& body,
    const EvaluatedSupportFace& removed,const BRepOffsetAPI_MakeThickSolid& builder,
    const TopoDS_Shape& result,double thickness,const QString& featureId) {
    TopTools_IndexedMapOfShape resultFaces;TopExp::MapShapes(result,TopAbs_FACE,resultFaces);
    const auto& images=builder.MakeOffset().OffsetFacesFromShapes();
    const auto& edgeImages=builder.MakeOffset().OffsetEdgesFromShapes();
    std::vector<EvaluatedSupportFace> output;
    for(const auto& source:body.supportFaces) {
        if(source.face.IsNull() || source.face.IsSame(removed.face))continue;
        const auto oldOrigin=source.frame["origin"].toArray(),oldNormal=source.frame["normal"].toArray();
        if(oldOrigin.size()!=3 || oldNormal.size()!=3)continue;
        const auto normal=direction(oldNormal);
        const auto expected=point(oldOrigin).Translated(gp_Vec(normal).Multiplied(-thickness));
        TopTools_ListOfShape descendants;images.LastImage(source.face,descendants);
        TopTools_IndexedMapOfShape matches;
        for(const auto& candidate:descendants) {
            if(candidate.ShapeType()!=TopAbs_FACE || !resultFaces.Contains(candidate))continue;
            const auto actual=TopoDS::Face(resultFaces(resultFaces.FindIndex(candidate)));
            const BRepAdaptor_Surface surface(actual);
            if(surface.GetType()!=GeomAbs_Plane || surface.Plane().Distance(expected)>1e-5)continue;
            auto outward=surface.Plane().Axis().Direction();
            if(actual.Orientation()==TopAbs_REVERSED)outward.Reverse();
            if(outward.Dot(normal)>-1+1e-7)continue;
            matches.Add(actual);
        }
        if(matches.Extent()!=1)continue;
        auto inner=source;
        inner.face=TopoDS::Face(matches(1));inner.producerFeatureId=featureId;inner.role="shell-inner";
        inner.faceId=QUuid::createUuidV5(QUuid(featureId),
            QByteArray("alloy-shell-inner-face/1\n")+source.producerFeatureId.toUtf8()+'\n'+source.faceId.toUtf8())
            .toString(QUuid::WithoutBraces);
        auto frame=source.frame;frame["origin"]=coordinates(expected);
        auto opposite=normal.Reversed();frame["normal"]=QJsonArray{opposite.X(),opposite.Y(),opposite.Z()};
        const auto v=source.frame["vAxis"].toArray();
        if(v.size()==3)frame["vAxis"]=QJsonArray{-v[0].toDouble(),-v[1].toDouble(),-v[2].toDouble()};
        inner.frame=frame;inner.boundaryEdges.clear();
        TopTools_IndexedMapOfShape roofEdges,sourceEdges;
        TopExp::MapShapes(inner.face,TopAbs_EDGE,roofEdges);
        TopExp::MapShapes(source.face,TopAbs_EDGE,sourceEdges);
        for(const auto& edge:body.solidEdges) {
            if(edge.edge.IsNull() || !sourceEdges.Contains(edge.edge)
                || edge.provenance["sourceSketchId"].toString().isEmpty()
                || edge.provenance["sourceEntityId"].toString().isEmpty())continue;
            if(BRepAdaptor_Curve(edge.edge).GetType()!=GeomAbs_Circle)continue;
            TopTools_ListOfShape mapped;edgeImages.LastImage(edge.edge,mapped);
            TopTools_IndexedMapOfShape candidates;
            for(const auto& image:mapped) {
                if(image.ShapeType()!=TopAbs_EDGE || !roofEdges.Contains(image))continue;
                const auto actual=TopoDS::Edge(roofEdges(roofEdges.FindIndex(image)));
                const BRepAdaptor_Curve curve(actual);
                if(curve.GetType()!=GeomAbs_Circle || BRepAdaptor_Surface(inner.face).Plane().Distance(curve.Circle().Location())>1e-5)continue;
                candidates.Add(actual);
            }
            int segment=0;
            for(int i=1;i<=candidates.Extent();++i) {
                auto id=QUuid::createUuidV5(QUuid(featureId),QByteArray("alloy-shell-inner-edge/1\n")
                    +edge.edgeId.toUtf8()+'\n'+QByteArray::number(++segment)).toString(QUuid::WithoutBraces);
                inner.boundaryEdges.push_back({id,"circle",TopoDS::Edge(candidates(i)),edge.provenance});
            }
        }
        output.push_back(inner);
    }
    return output;
}
}

void SolidShell::evaluate(EvaluatedBody& body,const QJsonObject& parameters,const QString& featureId) {
    const auto support=parameters["removedFace"].toObject();
    auto fail=[&](const QString& code,const QString& message) {
        throw GeometryKernel::EvaluationError({{"code",code},{"message",message},{"featureId",featureId},
            {"targetBodyId",body.bodyId},{"removedFace",support}});
    };
    const auto matches=std::count_if(body.supportFaces.begin(),body.supportFaces.end(),[&](const auto& face){return face.bodyId==support["bodyId"].toString()&&face.faceId==support["faceId"].toString()&&face.producerFeatureId==support["producerFeatureId"].toString();});
    if(matches>1)fail("shell_face_ambiguous","More than one current face matches the removed-face lineage");
    const auto source=std::find_if(body.supportFaces.begin(),body.supportFaces.end(),[&](const auto& face){
        return face.bodyId==support["bodyId"].toString() && face.faceId==support["faceId"].toString()
            && face.producerFeatureId==support["producerFeatureId"].toString();
    });
    if(source==body.supportFaces.end() || source->face.IsNull())fail("shell_face_unavailable","The selected planar producer face is unavailable");
    TopTools_IndexedMapOfShape oldFaces;TopExp::MapShapes(body.shape,TopAbs_FACE,oldFaces);
    if(!oldFaces.Contains(source->face) || BRepAdaptor_Surface(source->face).GetType()!=GeomAbs_Plane)
        fail("shell_face_unavailable","The selected face must be a current planar solid face");
    const double thickness=parameters["thicknessMm"].toDouble();
    TopTools_ListOfShape removed;removed.Append(source->face);
    BRepOffsetAPI_MakeThickSolid builder;
    builder.MakeThickSolidByJoin(body.shape,removed,-thickness,1e-4,BRepOffset_Skin,false,false,GeomAbs_Intersection);
    if(!builder.IsDone())fail("shell_failed","This face and thickness cannot produce a sharp inward shell");
    const auto result=builder.Shape();const auto summary=GeometryKernel::summarize(result);
    if(!summary["valid"].toBool() || !summary["closed"].toBool()
        || summary["topology"].toObject()["solids"].toInt()!=1 || summary["volumeMm3"].toDouble()<=0)
        fail("shell_invalid_result","Shell must produce one valid closed solid");
    const auto sourcePlane=BRepAdaptor_Surface(source->face).Plane();
    TopTools_IndexedMapOfShape sourceEdges;TopExp::MapShapes(source->face,TopAbs_EDGE,sourceEdges);
    TopTools_IndexedMapOfShape historyFaces;
    for(const auto& descendant:builder.Modified(source->face))if(descendant.ShapeType()==TopAbs_FACE)historyFaces.Add(descendant);
    for(const auto& descendant:builder.Generated(source->face))if(descendant.ShapeType()==TopAbs_FACE)historyFaces.Add(descendant);
    TopTools_IndexedMapOfShape faces;TopExp::MapShapes(result,TopAbs_FACE,faces);
    TopoDS_Face rim;int rimMatches=0;
    for(int i=1;i<=faces.Extent();++i){
        const auto face=TopoDS::Face(faces(i));const BRepAdaptor_Surface surface(face);
        if(surface.GetType()!=GeomAbs_Plane)continue;
        TopTools_IndexedMapOfShape rimEdges;TopExp::MapShapes(face,TopAbs_EDGE,rimEdges);
        bool retainsBoundary=sourceEdges.Extent()>0;
        for(int edge=1;edge<=sourceEdges.Extent();++edge)retainsBoundary &= rimEdges.Contains(sourceEdges(edge));
        if((historyFaces.Contains(face) || retainsBoundary)
            && surface.Plane().Axis().Direction().IsParallel(sourcePlane.Axis().Direction(),1e-7)
            && surface.Plane().Distance(sourcePlane.Location())<1e-5){
            rim=face;++rimMatches;
        }
    }
    if(rimMatches!=1)fail("shell_rim_ambiguous","Shell did not preserve one planar rim with the removed face boundary");
    std::vector<EvaluatedSupportFace> retained;
    auto opening=*source;opening.face=rim;opening.role="shell-rim";opening.boundaryEdges.clear();retained.push_back(opening);
    for(const auto& face:body.supportFaces)if(face.faceId!=source->faceId && faces.Contains(face.face))retained.push_back(face);
    const auto derived=innerFaces(body,*source,builder,result,thickness,featureId);
    retained.insert(retained.end(),derived.begin(),derived.end());
    TopTools_IndexedMapOfShape edges;TopExp::MapShapes(result,TopAbs_EDGE,edges);
    for(auto& edge:body.solidEdges)if(!edge.edge.IsNull() && !edges.Contains(edge.edge)){
        edge.edge.Nullify();edge.provenance["resolutionError"]="modified_by_shell";
    }
    body.shape=result;body.supportFaces=retained;
    auto updated=summary;QJsonArray supports;
    for(const auto& face:retained)supports.append(QJsonObject{{"faceId",face.faceId},{"bodyId",face.bodyId},
        {"producerFeatureId",face.producerFeatureId},{"role",face.role},{"planar",true},
        {"frame",face.frame},{"edges",GeometryKernel::supportEdgeDescriptors(face)}});
    updated["supportFaces"]=supports;updated["solidEdges"]=SolidFillet::descriptors(body.solidEdges);
    updated["lastModifierFeatureId"]=featureId;updated["producerFeatureId"]=body.featureId;
    updated["operation"]="shell";updated["thicknessMm"]=thickness;updated["removedFace"]=support;
    body.summary=updated;
}
