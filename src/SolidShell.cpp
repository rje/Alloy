#include "SolidShell.h"
#include "SolidFillet.h"
#include <BRepOffsetAPI_MakeThickSolid.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopoDS.hxx>
#include <GeomAbs_JoinType.hxx>
#include <GeomAbs_SurfaceType.hxx>
#include <algorithm>

void SolidShell::evaluate(EvaluatedBody& body,const QJsonObject& parameters,const QString& featureId) {
    const auto support=parameters["removedFace"].toObject();
    auto fail=[&](const QString& code,const QString& message) {
        throw GeometryKernel::EvaluationError({{"code",code},{"message",message},{"featureId",featureId},
            {"targetBodyId",body.bodyId},{"removedFace",support}});
    };
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
    TopTools_IndexedMapOfShape faces;TopExp::MapShapes(result,TopAbs_FACE,faces);
    TopoDS_Face rim;int rimMatches=0;
    for(int i=1;i<=faces.Extent();++i){
        const auto face=TopoDS::Face(faces(i));const BRepAdaptor_Surface surface(face);
        if(surface.GetType()==GeomAbs_Plane
            && surface.Plane().Axis().Direction().IsParallel(sourcePlane.Axis().Direction(),1e-7)
            && surface.Plane().Distance(sourcePlane.Location())<1e-5){
            rim=face;++rimMatches;
        }
    }
    if(rimMatches!=1)fail("shell_rim_ambiguous","Shell did not preserve one identifiable planar opening rim");
    std::vector<EvaluatedSupportFace> retained;
    auto opening=*source;opening.face=rim;opening.role="shell-rim";opening.boundaryEdges.clear();retained.push_back(opening);
    for(const auto& face:body.supportFaces)if(face.faceId!=source->faceId && faces.Contains(face.face))retained.push_back(face);
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
