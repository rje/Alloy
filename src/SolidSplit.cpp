// SPDX-License-Identifier: MIT
#include "SolidSplit.h"
#include "PlanarFaces.h"
#include "SolidFillet.h"
#include <BRepAlgoAPI_Common.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepPrimAPI_MakeHalfSpace.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <Bnd_Box.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <QUuid>
#include <QSet>
#include <cmath>

namespace {
gp_Pnt point(const QJsonArray& a){return {a[0].toDouble(),a[1].toDouble(),a[2].toDouble()};}
gp_Dir direction(const QJsonArray& a){return {a[0].toDouble(),a[1].toDouble(),a[2].toDouble()};}
QJsonArray coordinates(const gp_Pnt& p){return {p.X(),p.Y(),p.Z()};}
QJsonArray coordinates(const gp_Dir& d){return {d.X(),d.Y(),d.Z()};}
QString identity(const QString& feature,const QByteArray& role){
    return QUuid::createUuidV5(QUuid(feature),role).toString(QUuid::WithoutBraces);
}
}
QString SolidSplit::positiveBodyId(const QString& featureId,const QString& sourceBodyId){
    return identity(featureId,QByteArray("alloy-split-positive-body/1\n")+sourceBodyId.toUtf8());
}
std::pair<EvaluatedBody,EvaluatedBody> SolidSplit::evaluate(const EvaluatedBody& source,
    const QJsonObject& planeFrame,const QString& featureId,const QString& positiveId,const QString& name){
    auto fail=[&](const QString& code,const QString& message){throw GeometryKernel::EvaluationError(
        {{"code",code},{"message",message},{"featureId",featureId},{"targetBodyId",source.bodyId}});};
    const auto o=planeFrame["origin"].toArray(),n=planeFrame["normal"].toArray(),u=planeFrame["uAxis"].toArray();
    if(o.size()!=3 || n.size()!=3 || u.size()!=3)fail("split_plane_unavailable","Splitting plane has no resolved frame");
    if(!source.occurrenceId.isEmpty())fail("split_import_occurrence_unsupported","Split requires a local owned solid body");
    const gp_Pnt origin=point(o);const gp_Dir normal=direction(n),horizontal=direction(u);
    Bnd_Box bounds;BRepBndLib::Add(source.shape,bounds);double x0,y0,z0,x1,y1,z1;bounds.Get(x0,y0,z0,x1,y1,z1);
    const gp_Pnt center((x0+x1)/2,(y0+y1)/2,(z0+z1)/2);
    const double diagonal=std::hypot(std::hypot(x1-x0,y1-y0),z1-z0);
    const double extent=4*(diagonal+origin.Distance(center))+100;
    BRepBuilderAPI_MakeFace cutter(gp_Pln(gp_Ax3(origin,normal,horizontal)),-extent,extent,-extent,extent);
    if(!cutter.IsDone())fail("split_plane_invalid","Cannot construct the retained splitting plane");
    auto splitHalf=[&](int sign,const QString& bodyId,const QString& bodyName){
        const auto sample=origin.Translated(gp_Vec(normal)*double(sign)*extent/2);
        BRepPrimAPI_MakeHalfSpace space(cutter.Face(),sample);
        if(!space.IsDone())fail("split_plane_invalid","Cannot extend the splitting plane through the body");
        BRepAlgoAPI_Common common(source.shape,space.Solid());common.Build();
        if(!common.IsDone())fail("split_failed","Plane/solid intersection failed");
        TopoDS_Solid result;int solids=0;
        for(TopExp_Explorer it(common.Shape(),TopAbs_SOLID);it.More();it.Next()){
            result=TopoDS::Solid(it.Current());++solids;
        }
        if(solids!=1)fail("split_not_two_solids","Plane must leave one connected solid on each side");
        auto body=source;body.bodyId=bodyId;body.name=bodyName;body.shape=result;
        body.summary=GeometryKernel::summarize(result);
        if(!body.summary["valid"].toBool() || !body.summary["closed"].toBool()
            || body.summary["topology"].toObject()["solids"].toInt()!=1
            || body.summary["volumeMm3"].toDouble()<=1e-7)
            fail("split_invalid_half","Split result must be one valid closed positive-volume solid per side");
        body.supportFaces=PlanarFaces::transfer(common,source.supportFaces,result,true);
        for(auto& face:body.supportFaces)face.bodyId=bodyId;
        body.solidEdges=SolidFillet::transferEdges(common,source.solidEdges,result,true);
        if(sign>0)body.featureId=featureId;
        // The cap is identified on the exact Boolean result by its support plane.
        // It is new topology, not a replacement for a missing authored reference.
        TopoDS_Face cap;int caps=0;
        for(TopExp_Explorer it(result,TopAbs_FACE);it.More();it.Next()){
            const auto face=TopoDS::Face(it.Current());const BRepAdaptor_Surface surface(face);
            if(surface.GetType()!=GeomAbs_Plane || surface.Plane().Distance(origin)>1e-6
                || std::abs(surface.Plane().Axis().Direction().Dot(normal))<1-1e-8)continue;
            cap=face;++caps;
        }
        if(caps!=1)fail("split_cap_ambiguous","The split produced no unique planar partition face");
        auto frame=planeFrame;frame.remove("displayBoundsMm");
        const auto outward=sign<0 ? normal : normal.Reversed();frame["normal"]=coordinates(outward);
        if(sign>0){const auto v=frame["vAxis"].toArray();if(v.size()==3)frame["vAxis"]=QJsonArray{-v[0].toDouble(),-v[1].toDouble(),-v[2].toDouble()};}
        EvaluatedSupportFace support;support.faceId=identity(featureId,QByteArray("alloy-split-cap/1\n")+(sign<0?"negative":"positive"));
        support.bodyId=bodyId;support.producerFeatureId=featureId;support.role=sign<0?"split.negative.cap":"split.positive.cap";
        support.face=cap;support.frame=frame;
        TopTools_IndexedDataMapOfShapeListOfShape adjacency;TopExp::MapShapesAndAncestors(result,TopAbs_EDGE,TopAbs_FACE,adjacency);
        QSet<QString> capEdgeIds;
        for(TopExp_Explorer it(cap,TopAbs_EDGE);it.More();it.Next()){
            const auto edge=TopoDS::Edge(it.Current());const auto curve=BRepAdaptor_Curve(edge);
            if(curve.GetType()!=GeomAbs_Line && curve.GetType()!=GeomAbs_Circle)continue;
            QString adjacentId;int matches=0;
            for(const auto& neighbor:adjacency.FindFromKey(edge))if(!neighbor.IsSame(cap))
                for(const auto& retained:body.supportFaces)if(retained.face.IsSame(neighbor)){
                    adjacentId=retained.faceId;++matches;
                }
            if(matches!=1)continue;
            const auto edgeId=identity(featureId,QByteArray("alloy-split-cap-edge/1\n")
                +(sign<0?"negative":"positive")+'\n'+adjacentId.toUtf8());
            if(capEdgeIds.contains(edgeId))fail("split_cap_edge_ambiguous","A cap has multiple edges adjoining the same retained face");
            capEdgeIds.insert(edgeId);
            QJsonObject provenance{{"sourceFaceId",adjacentId},{"splitSide",sign<0?"negative":"positive"},
                {"role","splitCap"}};
            const auto a=curve.Value(curve.FirstParameter()),b=curve.Value(curve.LastParameter());
            provenance["startMm"]=coordinates(a);provenance["endMm"]=coordinates(b);
            support.boundaryEdges.push_back({edgeId,curve.GetType()==GeomAbs_Line?"line":"circle",edge,provenance});
            body.solidEdges.push_back({edgeId,edge,provenance});
        }
        body.supportFaces.push_back(support);
        body.summary["supportFaces"]=PlanarFaces::descriptors(body.supportFaces);
        body.summary["solidEdges"]=SolidFillet::descriptors(body.solidEdges);
        body.summary["operation"]="splitBody";body.summary["sourceBodyId"]=source.bodyId;
        body.summary["splitSide"]=sign<0?"negative":"positive";body.summary["lastModifierFeatureId"]=featureId;
        return body;
    };
    auto negative=splitHalf(-1,source.bodyId,source.name);
    auto positive=splitHalf(+1,positiveId,name+" positive");
    const double original=GeometryKernel::summarize(source.shape)["volumeMm3"].toDouble();
    const double left=negative.summary["volumeMm3"].toDouble(),right=positive.summary["volumeMm3"].toDouble();
    if(std::abs(left+right-original)>std::max(1e-5,original*1e-7))
        fail("split_volume_mismatch","Split halves do not partition the source volume");
    return {negative,positive};
}
