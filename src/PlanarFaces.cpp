// SPDX-License-Identifier: MIT
#include "PlanarFaces.h"
#include <BRepBuilderAPI_MakeShape.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopExp.hxx>
#include <TopoDS.hxx>
#include <QUuid>
#include <QSet>
#include <algorithm>
#include <cmath>
namespace {
QJsonArray point(const gp_Pnt& p) {return {p.X(),p.Y(),p.Z()};}
QJsonArray axis(const gp_Dir& p) {return {p.X(),p.Y(),p.Z()};}
TopoDS_Shape descendant(BRepBuilderAPI_MakeShape& op,const TopoDS_Shape& source,
    const TopTools_IndexedMapOfShape& present,TopAbs_ShapeEnum type) {
    TopTools_IndexedMapOfShape choices;
    if(present.Contains(source))choices.Add(source);
    for(const auto& next:op.Modified(source))if(next.ShapeType()==type && present.Contains(next))choices.Add(next);
    return choices.Extent()==1 ? choices(1) : TopoDS_Shape{};
}
}
bool PlanarFaces::updateStraightBoundary(const TopoDS_Edge& source,
    const TopoDS_Edge& next,QJsonObject& provenance) {
    if(source.IsNull() || next.IsNull())return false;
    BRepAdaptor_Curve before(source),after(next);
    if(before.GetType()!=GeomAbs_Line || after.GetType()!=GeomAbs_Line)return false;
    auto a=after.Value(after.FirstParameter()),b=after.Value(after.LastParameter());
    const auto oldA=provenance["startMm"].toArray(),oldB=provenance["endMm"].toArray();
    // Support boundaries already carry their oriented endpoints. Solid edges
    // initially use the curve parameter direction, as their public descriptors do.
    const auto first=oldA.size()==3 ? gp_Pnt(oldA[0].toDouble(),oldA[1].toDouble(),oldA[2].toDouble()) : before.Value(before.FirstParameter());
    const auto last=oldB.size()==3 ? gp_Pnt(oldB[0].toDouble(),oldB[1].toDouble(),oldB[2].toDouble()) : before.Value(before.LastParameter());
    const gp_Vec direction(first,last),current(a,b);
    if(direction.Magnitude()<1e-6 || current.Magnitude()<1e-6
        || direction.Crossed(current).Magnitude()>1e-9*direction.Magnitude()*current.Magnitude()
        || before.Line().Distance(a)>1e-6 || before.Line().Distance(b)>1e-6)return false;
    if(direction.Dot(current)<0)std::swap(a,b);
    if(provenance.contains("startContact") || provenance.contains("endContact")) {
        provenance["sourceBoundaryContacts"]=QJsonObject{{"start",provenance["startContact"]},{"end",provenance["endContact"]}};
        provenance.remove("startContact");provenance.remove("endContact");
    }
    provenance["endpointProvenance"]="currentKernelBoundary";
    provenance["lineage"]="uniqueModifiedStraight";
    provenance["startMm"]=point(a);provenance["endMm"]=point(b);
    return true;
}
std::vector<EvaluatedSupportFace> PlanarFaces::transfer(BRepBuilderAPI_MakeShape& op,
    const std::vector<EvaluatedSupportFace>& sources,const TopoDS_Shape& result,bool retainModifiedStraight) {
    TopTools_IndexedMapOfShape present;TopExp::MapShapes(result,TopAbs_FACE,present);
    std::vector<EvaluatedSupportFace> output;QSet<QString> seen;
    for(auto source:sources) {
        const auto next=descendant(op,source.face,present,TopAbs_FACE);
        if(next.IsNull() || BRepAdaptor_Surface(TopoDS::Face(next)).GetType()!=GeomAbs_Plane)continue;
        // A retained frame must still lie on the result plane. This verifies
        // kernel history; it is not used to search for replacement faces.
        const auto origin=source.frame["origin"].toArray(),normal=source.frame["normal"].toArray();
        const auto plane=BRepAdaptor_Surface(TopoDS::Face(next)).Plane();
        if(origin.size()!=3 || normal.size()!=3 || plane.Distance(gp_Pnt(origin[0].toDouble(),origin[1].toDouble(),origin[2].toDouble()))>1e-6
            || std::abs(plane.Axis().Direction().Dot(gp_Dir(normal[0].toDouble(),normal[1].toDouble(),normal[2].toDouble())))<1-1e-9)continue;
        source.face=TopoDS::Face(next);
        TopTools_IndexedMapOfShape edges;TopExp::MapShapes(next,TopAbs_EDGE,edges);
        std::vector<EvaluatedSupportEdge> retained;
        for(auto edge:source.boundaryEdges) {
            const auto mapped=descendant(op,edge.edge,edges,TopAbs_EDGE);if(mapped.IsNull())continue;
            // Kernel history alone chooses the edge; coordinates only verify
            // straight-line scope and preserve endpoint roles on that choice.
            if(!mapped.IsSame(edge.edge)
                && (!retainModifiedStraight || edge.type!="line" || !updateStraightBoundary(edge.edge,TopoDS::Edge(mapped),edge.provenance)))continue;
            edge.edge=TopoDS::Edge(mapped);retained.push_back(edge);
        }
        source.boundaryEdges=retained;
        const auto key=source.producerFeatureId+"/"+source.faceId;
        if(seen.contains(key)) {
            const auto prior=std::find_if(output.begin(),output.end(),[&](const auto& f){return f.producerFeatureId==source.producerFeatureId && f.faceId==source.faceId;});
            if(!prior->face.IsSame(source.face))throw std::runtime_error("Ambiguous retained planar face identity");
        } else {output.push_back(source);seen.insert(key);}
    }
    return output;
}
std::vector<EvaluatedSupportFace> PlanarFaces::prismSides(BRepPrimAPI_MakePrism& prism,
    const EvaluatedProfile& profile,const QString& generator,double distance) {
    std::vector<EvaluatedSupportFace> result;
    TopTools_IndexedMapOfShape solidFaces;TopExp::MapShapes(prism.Shape(),TopAbs_FACE,solidFaces);
    const auto offer=[&](const QString& role,const QByteArray& signature,const TopoDS_Shape& shape,const QJsonObject& frame) {
        if(shape.IsNull() || shape.ShapeType()!=TopAbs_FACE || BRepAdaptor_Surface(TopoDS::Face(shape)).GetType()!=GeomAbs_Plane)return;
        const auto id=QUuid::createUuidV5(QUuid(generator),QByteArray("alloy-planar-face/1\n")+signature).toString(QUuid::WithoutBraces);
        const auto prior=std::find_if(result.begin(),result.end(),[&](const auto& value){return value.faceId==id;});
        if(prior!=result.end()){if(!prior->face.IsSame(shape))throw std::runtime_error("Ambiguous planar face provenance");return;}
        result.push_back({id,{},generator,role,TopoDS::Face(shape),frame,{}});
    };
    auto base=profile.summary["frame"].toObject();
    auto n=base["normal"].toArray(),v=base["vAxis"].toArray();
    if(distance>0){for(int i=0;i<3;++i){n[i]=-n[i].toDouble();v[i]=-v[i].toDouble();}base["normal"]=n;base["vAxis"]=v;}
    offer("extrude.base",profile.profileId.toUtf8()+"/base",prism.FirstShape(),base);
    for(const auto& boundary:profile.boundaryEdges) {
        if(boundary.type!="line")continue;
        TopTools_IndexedMapOfShape generated;
        for(const auto& shape:prism.Generated(boundary.edge))if(shape.ShapeType()==TopAbs_FACE)generated.Add(shape);
        if(generated.Extent()!=1)continue;
        const auto face=TopoDS::Face(solidFaces(solidFaces.FindIndex(generated(1))));const auto plane=BRepAdaptor_Surface(face).Plane();
        auto normal=plane.Axis().Direction();if(face.Orientation()==TopAbs_REVERSED)normal.Reverse();
        const auto sourceNormal=profile.summary["frame"].toObject()["normal"].toArray();
        gp_Dir vertical(sourceNormal[0].toDouble(),sourceNormal[1].toDouble(),sourceNormal[2].toDouble());
        gp_Dir horizontal=vertical.Crossed(normal);
        BRepAdaptor_Curve curve(boundary.edge);const auto origin=curve.Value(curve.FirstParameter());
        QJsonObject frame{{"origin",point(origin)},{"uAxis",axis(horizontal)},{"vAxis",axis(vertical)},{"normal",axis(normal)}};
        QStringList contacts{boundary.startContact,boundary.endContact};contacts.sort();
        offer("extrude.side",(profile.profileId+"/"+boundary.sourceEntityId+"/"+contacts.join('/')).toUtf8(),face,frame);
    }
    return result;
}
QJsonArray PlanarFaces::descriptors(const std::vector<EvaluatedSupportFace>& faces) {
    QJsonArray output;
    for(const auto& f:faces)output.append(QJsonObject{{"faceId",f.faceId},{"bodyId",f.bodyId},{"producerFeatureId",f.producerFeatureId},{"role",f.role},{"planar",true},{"frame",f.frame},{"edges",GeometryKernel::supportEdgeDescriptors(f)}});
    return output;
}
