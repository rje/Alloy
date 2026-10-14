#include "SolidChamfer.h"
#include "SolidFillet.h"
#include "PlanarFaces.h"
#include <BRepFilletAPI_MakeChamfer.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <algorithm>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepClass3d_SolidClassifier.hxx>
#include <BRepGProp.hxx>
#include <BRep_Tool.hxx>
#include <GProp_GProps.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopoDS.hxx>
#include <cmath>
#include <memory>

namespace {
TopTools_ListOfShape singleton(const TopoDS_Shape& shape) {
    TopTools_ListOfShape result;result.Append(shape);return result;
}
double volume(const TopoDS_Shape& shape) {
    GProp_GProps properties;BRepGProp::VolumeProperties(shape,properties);return properties.Mass();
}
bool emptyIntersection(const TopoDS_Shape& a,const TopoDS_Shape& b,double tolerance) {
    BRepAlgoAPI_Common common;common.SetArguments(singleton(a));common.SetTools(singleton(b));
    common.SetNonDestructive(true);common.Build();
    return common.IsDone() && !common.HasErrors() && std::abs(volume(common.Shape()))<=tolerance;
}
// Exact planar concave corner only. Native OCCT remains the first choice.
// Every leg must fit the actual trimmed adjacent face, and both ends must be
// bounded by perpendicular incident planes. No proximity-based edge lookup.
std::unique_ptr<BRepAlgoAPI_Fuse> planarConcaveFallback(
    const TopoDS_Shape& source,const TopoDS_Edge& edge,double distance) {
    if(!std::isfinite(distance) || distance<=0)return {};
    BRepAdaptor_Curve curve(edge);if(curve.GetType()!=GeomAbs_Line)return {};
    const gp_Pnt a=curve.Value(curve.FirstParameter()),b=curve.Value(curve.LastParameter());
    const double length=a.Distance(b),tol=1e-7;
    if(length<=tol || distance<=tol)return {};
    const gp_Dir axis(gp_Vec(a,b));
    TopTools_IndexedDataMapOfShapeListOfShape incidence;
    TopExp::MapShapesAndAncestors(source,TopAbs_EDGE,TopAbs_FACE,incidence);
    if(!incidence.Contains(edge))return {};
    TopTools_IndexedMapOfShape adjacent;
    for(const auto& face:incidence.FindFromKey(edge))adjacent.Add(face);
    if(adjacent.Extent()!=2)return {};
    gp_Dir normals[2];gp_Pln legPlanes[2];
    for(int i=0;i<2;++i) {
        const auto face=TopoDS::Face(adjacent(i+1));BRepAdaptor_Surface surface(face);
        if(surface.GetType()!=GeomAbs_Plane)return {};
        legPlanes[i]=surface.Plane();normals[i]=legPlanes[i].Axis().Direction();
        if(face.Orientation()==TopAbs_REVERSED)normals[i].Reverse();
        else if(face.Orientation()!=TopAbs_FORWARD)return {};
        if(std::abs(normals[i].Dot(axis))>tol)return {};
    }
    if(std::abs(normals[0].Dot(normals[1]))>tol)return {};
    // Certify an empty outward quadrant and material behind both faces.
    const gp_Pnt middle((a.X()+b.X())/2,(a.Y()+b.Y())/2,(a.Z()+b.Z())/2);
    const double probe=std::min(length,distance)*1e-4;
    const auto state=[&](const gp_Vec& offset) {
        BRepClass3d_SolidClassifier classifier(source,middle.Translated(offset),tol);
        return classifier.State();
    };
    const gp_Vec n0(normals[0]),n1(normals[1]);
    if(state((n0+n1)*probe)!=TopAbs_OUT || state((n0-n1)*probe)!=TopAbs_IN
        || state((n1-n0)*probe)!=TopAbs_IN)return {};
    // The source root endpoints must each lie on a unique perpendicular cap.
    TopTools_IndexedDataMapOfShapeListOfShape vertexFaces;
    TopExp::MapShapesAndAncestors(source,TopAbs_VERTEX,TopAbs_FACE,vertexFaces);
    TopoDS_Vertex vertices[2];TopExp::Vertices(edge,vertices[0],vertices[1]);
    bool transitionEndpoint=false;
    for(const auto& vertex:vertices) {
        if(vertex.IsNull() || !vertexFaces.Contains(vertex))return {};
        TopTools_IndexedMapOfShape ends;
        for(const auto& shape:vertexFaces.FindFromKey(vertex))if(!adjacent.Contains(shape))ends.Add(shape);
        const auto point=BRep_Tool::Pnt(vertex);
        const bool atStart=point.Distance(a)<=tol,atEnd=point.Distance(b)<=tol;
        if(atStart==atEnd)return {};
        int capCount=0;
        for(int i=1;i<=ends.Extent();++i) {
            const auto endFace=TopoDS::Face(ends(i));BRepAdaptor_Surface cap(endFace);
            if(cap.GetType()!=GeomAbs_Plane)return {};
            auto normal=cap.Plane().Axis().Direction();
            if(endFace.Orientation()==TopAbs_REVERSED)normal.Reverse();
            else if(endFace.Orientation()!=TopAbs_FORWARD)return {};
            if(cap.Plane().Distance(point)>tol)return {};
            if(normal.Dot(axis)*(atStart ? -1 : 1)>1-tol){++capCount;continue;}
            // A wall ending at the stem can expose the opposite side of the
            // same leg plane. No unrelated third support plane is admissible.
            int continuationCount=0;
            for(int j=0;j<2;++j)if(normal.Dot(normals[j])<-1+tol
                && legPlanes[j].Distance(cap.Plane().Location())<=tol)++continuationCount;
            if(continuationCount!=1)return {};
            transitionEndpoint=true;
        }
        if(capCount!=1)return {};
    }
    for(int i=0;i<2;++i) {
        const gp_Vec shift=gp_Vec(normals[1-i])*distance;
        BRepBuilderAPI_MakePolygon polygon;polygon.Add(a);polygon.Add(b);
        polygon.Add(b.Translated(shift));polygon.Add(a.Translated(shift));polygon.Close();
        const auto leg=BRepBuilderAPI_MakeFace(polygon.Wire()).Face();
        BRepAlgoAPI_Common common;common.SetArguments(singleton(leg));common.SetTools(singleton(adjacent(i+1)));
        common.SetNonDestructive(true);common.Build();if(!common.IsDone() || common.HasErrors())return {};
        GProp_GProps area;BRepGProp::SurfaceProperties(common.Shape(),area);
        if(std::abs(area.Mass()-distance*length)>std::max(1e-7,distance*length*1e-7))return {};
    }
    BRepBuilderAPI_MakePolygon triangle;triangle.Add(a);
    triangle.Add(a.Translated(n0*distance));triangle.Add(a.Translated(n1*distance));triangle.Close();
    const auto wedge=BRepPrimAPI_MakePrism(BRepBuilderAPI_MakeFace(triangle.Wire()).Face(),gp_Vec(a,b)).Shape();
    const double expected=distance*distance*length/2,volumeTolerance=std::max(1e-7,expected*1e-7);
    if(!emptyIntersection(source,wedge,volumeTolerance))return {};
    // No third face may share positive area with the proposed wedge boundary.
    // Zero common solid volume alone would permit an accidental second contact.
    TopTools_IndexedMapOfShape sourceFaces,wedgeFaces;
    TopExp::MapShapes(source,TopAbs_FACE,sourceFaces);TopExp::MapShapes(wedge,TopAbs_FACE,wedgeFaces);
    for(int i=1;i<=sourceFaces.Extent();++i) {
        const auto& sourceFace=sourceFaces(i);if(adjacent.Contains(sourceFace))continue;
        for(int j=1;j<=wedgeFaces.Extent();++j) {
            const auto& wedgeFace=wedgeFaces(j);
            BRepAlgoAPI_Common contact;contact.SetArguments(singleton(sourceFace));contact.SetTools(singleton(wedgeFace));
            contact.SetNonDestructive(true);contact.Build();if(!contact.IsDone() || contact.HasErrors())return {};
            GProp_GProps area;BRepGProp::SurfaceProperties(contact.Shape(),area);
            if(std::abs(area.Mass())>std::max(1e-7,distance*length*1e-7))return {};
        }
    }
    if(transitionEndpoint) {
        // Establish the native endpoint convention at half size. The returned
        // geometry is still built at the exact requested distance below.
        BRepBuilderAPI_Copy copy(source,true,false);
        const auto copiedEdge=TopoDS::Edge(copy.ModifiedShape(edge));
        BRepFilletAPI_MakeChamfer witness(copy.Shape());witness.Add(distance/2,copiedEdge);witness.Build();
        if(!witness.IsDone())return {};
        BRepBuilderAPI_MakePolygon smallTriangle;smallTriangle.Add(a);
        smallTriangle.Add(a.Translated(n0*(distance/2)));smallTriangle.Add(a.Translated(n1*(distance/2)));smallTriangle.Close();
        const auto smallWedge=BRepPrimAPI_MakePrism(BRepBuilderAPI_MakeFace(smallTriangle.Wire()).Face(),gp_Vec(a,b)).Shape();
        BRepAlgoAPI_Fuse small;small.SetArguments(singleton(source));small.SetTools(singleton(smallWedge));
        small.SetNonDestructive(true);small.Build();if(!small.IsDone() || small.HasErrors())return {};
        for(const auto& shape:std::vector<TopoDS_Shape>{witness.Shape(),small.Shape()}) {
            const auto summary=GeometryKernel::summarize(shape);
            if(!summary["valid"].toBool() || !summary["closed"].toBool()
                || summary["topology"].toObject()["solids"].toInt()!=1)return {};
        }
        for(int direction=0;direction<2;++direction) {
            BRepAlgoAPI_Cut difference;
            difference.SetArguments(singleton(direction ? witness.Shape() : small.Shape()));
            difference.SetTools(singleton(direction ? small.Shape() : witness.Shape()));
            difference.SetNonDestructive(true);difference.Build();
            if(!difference.IsDone() || difference.HasErrors() || std::abs(volume(difference.Shape()))>volumeTolerance)return {};
        }
    }
    auto result=std::make_unique<BRepAlgoAPI_Fuse>();result->SetArguments(singleton(source));
    result->SetTools(singleton(wedge));result->SetNonDestructive(true);result->Build();
    if(!result->IsDone() || result->HasErrors())return {};
    result->SimplifyResult(true,true);
    if(std::abs(volume(result->Shape())-volume(source)-expected)>volumeTolerance)return {};
    BRepAlgoAPI_Cut removed;removed.SetArguments(singleton(source));removed.SetTools(singleton(result->Shape()));
    removed.SetNonDestructive(true);removed.Build();
    if(!removed.IsDone() || removed.HasErrors() || std::abs(volume(removed.Shape()))>volumeTolerance)return {};
    return result;
}
}

void SolidChamfer::evaluate(EvaluatedBody& body,const QJsonObject& parameters,const QString& featureId) {
    auto fail=[&](const QString& code,const QString& message,const QString& edgeId=QString{}) {
        throw GeometryKernel::EvaluationError({{"code",code},{"message",message},{"featureId",featureId},{"targetBodyId",body.bodyId},{"edgeId",edgeId}});
    };
    BRepFilletAPI_MakeChamfer builder(body.shape);
    for(const auto& value:parameters["edgeIds"].toArray()) {
        const auto id=value.toString();
        const auto edge=std::find_if(body.solidEdges.begin(),body.solidEdges.end(),[&](const auto& item){return item.edgeId==id;});
        if(edge==body.solidEdges.end())fail("chamfer_edge_unavailable","Authored edge provenance is no longer present",id);
        if(edge->edge.IsNull())fail("chamfer_edge_"+edge->provenance["resolutionError"].toString("unavailable"),"Authored edge was deleted or has ambiguous descendants",id);
        builder.Add(parameters["distanceMm"].toDouble(),edge->edge);
    }
    builder.Build();
    std::unique_ptr<BRepAlgoAPI_Fuse> fallback;
    if(!builder.IsDone() && parameters["edgeIds"].toArray().size()==1) {
        const auto id=parameters["edgeIds"].toArray().first().toString();
        const auto edge=std::find_if(body.solidEdges.begin(),body.solidEdges.end(),[&](const auto& item){return item.edgeId==id;});
        fallback=planarConcaveFallback(body.shape,edge->edge,parameters["distanceMm"].toDouble());
    }
    if(!builder.IsDone() && !fallback)fail("chamfer_failed","The selected edges cannot be chamfered at this distance");
    BRepBuilderAPI_MakeShape& operation=fallback ? static_cast<BRepBuilderAPI_MakeShape&>(*fallback) : builder;
    const auto result=operation.Shape();auto summary=GeometryKernel::summarize(result);
    if(!summary["valid"].toBool() || !summary["closed"].toBool() || summary["topology"].toObject()["solids"].toInt()!=1 || summary["volumeMm3"].toDouble()<=0)
        fail("chamfer_invalid_result","Chamfer must produce one valid closed solid");
    auto caps=PlanarFaces::transfer(operation,body.supportFaces,result);
    auto edges=SolidFillet::transferEdges(operation,body.solidEdges,result,true);
    body.shape=result;body.supportFaces=caps;body.solidEdges=std::move(edges);
    QJsonArray capValues;for(const auto& cap:caps)capValues.append(QJsonObject{{"faceId",cap.faceId},{"bodyId",cap.bodyId},{"producerFeatureId",cap.producerFeatureId},{"role",cap.role},{"planar",true},{"frame",cap.frame},{"edges",GeometryKernel::supportEdgeDescriptors(cap)}});
    summary["chamferMethod"]=fallback ? "planarConcave" : "native";summary["supportFaces"]=capValues;summary["solidEdges"]=SolidFillet::descriptors(body.solidEdges);summary["lastModifierFeatureId"]=featureId;summary["producerFeatureId"]=body.featureId;summary["operation"]="chamfer";summary["distanceMm"]=parameters["distanceMm"];summary["selectedEdgeIds"]=parameters["edgeIds"];body.summary=summary;
}
