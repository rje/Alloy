// SPDX-License-Identifier: MIT
#include "SolidThicken.h"
#include "PlanarFaces.h"
#include <BRepOffsetAPI_MakeThickSolid.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRep_Tool.hxx>
#include <GProp_GProps.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <QUuid>
#include <QSet>
#include <algorithm>
#include <cmath>

namespace {
QString id(const QString& feature,const QByteArray& role){
    return QUuid::createUuidV5(QUuid(feature),QByteArray("alloy-thicken/1/")+role).toString(QUuid::WithoutBraces);
}
QJsonArray point(const gp_Pnt& p){return {p.X(),p.Y(),p.Z()};}
}

EvaluatedBody SolidThicken::evaluate(const QJsonObject& feature,const EvaluatedBody& source){
    const auto p=feature["parameters"].toObject(),ref=p["sourceFace"].toObject();
    auto reject=[&](const QString& code,const QString& message){throw GeometryKernel::EvaluationError({
        {"featureId",feature["id"]},{"featureType","thicken"},{"code",code},{"message",message}});};
    if(source.bodyId!=ref["bodyId"].toString() || source.featureId!=ref["producerFeatureId"].toString()
        || source.summary["type"]!="surface" || source.summary["topology"].toObject()["faces"]!=1
        || source.supportFaces.size()!=1 || source.supportFaces[0].faceId!=ref["faceId"].toString()
        || source.supportFaces[0].boundaryEdges.size()!=4)
        reject("surface_unavailable","Thicken needs one exact earlier surface face in its component");
    const auto sourceFace=source.supportFaces[0].face;
    TopTools_IndexedMapOfShape sourceFaces;TopExp::MapShapes(source.shape,TopAbs_FACE,sourceFaces);
    if(sourceFaces.Extent()!=1 || !sourceFaces.Contains(sourceFace) || !BRepCheck_Analyzer(sourceFace).IsValid())
        reject("surface_unavailable","The retained source face is not current and valid");
    const double thickness=p["signedThicknessMm"].toDouble();
    BRepOffsetAPI_MakeThickSolid builder;builder.MakeThickSolidBySimple(source.shape,thickness);
    if(!builder.IsDone())reject("thicken_failed","OCCT could not thicken the selected open sheet");
    auto shape=builder.Shape();
    if(shape.IsNull() || shape.ShapeType()!=TopAbs_SOLID || !BRepCheck_Analyzer(shape).IsValid())
        reject("thicken_failed","Thicken did not produce one valid closed solid");
    GProp_GProps mass;BRepGProp::VolumeProperties(shape,mass,true);
    if(!std::isfinite(mass.Mass()) || std::abs(mass.Mass())<1e-6)reject("thicken_failed","Thicken has no positive physical volume");
    if(mass.Mass()<0){shape.Reverse();BRepGProp::VolumeProperties(shape,mass,true);}
    if(mass.Mass()<=0)reject("thicken_failed","Solid orientation could not be normalized");
    TopTools_IndexedMapOfShape faces,edges,vertices;
    TopExp::MapShapes(shape,TopAbs_FACE,faces);TopExp::MapShapes(shape,TopAbs_EDGE,edges);TopExp::MapShapes(shape,TopAbs_VERTEX,vertices);
    if(faces.Extent()!=6 || edges.Extent()!=12 || vertices.Extent()!=8 || !faces.Contains(sourceFace))
        reject("unsupported_surface_topology","This Thicken requires the source-sized four-rim open sheet");
    const int sourceIndex=faces.FindIndex(sourceFace);
    QMap<int,QSet<int>> faceEdges,edgeFaces;
    for(int i=1;i<=faces.Extent();++i)for(TopExp_Explorer e(faces(i),TopAbs_EDGE);e.More();e.Next()){
        const int j=edges.FindIndex(e.Current());if(j<1)reject("thicken_lineage_unavailable","A result boundary is not current");
        faceEdges[i].insert(j);edgeFaces[j].insert(i);
    }
    QMap<QString,int> sourceEdges,sideFaces,offsetEdges;QSet<int> usedSideFaces;
    for(const auto& boundary:source.supportFaces[0].boundaryEdges){
        const int edgeIndex=edges.FindIndex(boundary.edge);
        if(edgeIndex<1 || sourceEdges.values().contains(edgeIndex) || edgeFaces[edgeIndex].size()!=2
            || !edgeFaces[edgeIndex].contains(sourceIndex))
            reject("thicken_lineage_unavailable","A retained source rim did not survive uniquely");
        const int side=*std::find_if(edgeFaces[edgeIndex].begin(),edgeFaces[edgeIndex].end(),[&](int face){return face!=sourceIndex;});
        if(usedSideFaces.contains(side))reject("thicken_lineage_unavailable","Source rims do not own distinct side faces");
        sourceEdges[boundary.edgeId]=edgeIndex;sideFaces[boundary.edgeId]=side;usedSideFaces.insert(side);
    }
    int offsetIndex=0;for(int i=1;i<=faces.Extent();++i)if(i!=sourceIndex && !usedSideFaces.contains(i)){
        if(offsetIndex)reject("thicken_lineage_unavailable","The offset face is ambiguous");offsetIndex=i;
    }
    if(!offsetIndex || faceEdges[offsetIndex].size()!=4)
        reject("thicken_lineage_unavailable","The offset face is unavailable");
    for(auto it=sideFaces.begin();it!=sideFaces.end();++it){
        const auto common=faceEdges[it.value()]&faceEdges[offsetIndex];
        if(common.size()!=1)reject("thicken_lineage_unavailable","An offset rim is not unique");
        offsetEdges[it.key()]=*common.begin();
    }
    QSet<int> uniqueOffsetEdges;for(const int index:offsetEdges)uniqueOffsetEdges.insert(index);
    if(uniqueOffsetEdges.size()!=4)
        reject("thicken_lineage_unavailable","Offset rims are not distinct");
    // The offset direction is checked against the selected face's oriented normal.
    const auto current=TopoDS::Face(faces(sourceIndex)),offset=TopoDS::Face(faces(offsetIndex));
    BRepAdaptor_Surface surface(sourceFace),offsetSurface(offset);
    const double u=(surface.FirstUParameter()+surface.LastUParameter())*.5,
        v=(surface.FirstVParameter()+surface.LastVParameter())*.5;
    gp_Pnt sample;gp_Vec du,dv;surface.D1(u,v,sample,du,dv);
    auto normal=du.Crossed(dv);if(sourceFace.Orientation()==TopAbs_REVERSED)normal.Reverse();
    if(normal.Magnitude()<1e-9)reject("thicken_lineage_unavailable","The selected surface normal is singular");normal.Normalize();
    const auto displacement=gp_Vec(sample,offsetSurface.Value(u,v));
    if(std::abs(displacement.Dot(normal)-thickness)>1e-4 || std::abs(displacement.Magnitude()-std::abs(thickness))>1e-4)
        reject("thicken_wrong_side","The result is not on the requested signed side");
    EvaluatedBody body;body.featureId=feature["id"].toString();body.bodyId=feature["bodyId"].toString();
    body.ownerId=feature["ownerId"].toString();body.name=feature["name"].toString();body.visible=feature["visible"].toBool();body.shape=shape;
    auto publishFace=[&](int index,const QByteArray& role,const QString& sourceEdge=QString{}){
        EvaluatedSupportFace f{id(body.featureId,QByteArray("face/")+role),body.bodyId,body.featureId,QString::fromLatin1(role),TopoDS::Face(faces(index)),{}, {}};
        f.provenance={{"sourceBodyId",source.bodyId},{"sourceFaceId",ref["faceId"]},{"sourceEdgeId",sourceEdge}};
        body.supportFaces.push_back(f);
    };
    publishFace(sourceIndex,"source");publishFace(offsetIndex,"offset");
    for(auto it=sideFaces.begin();it!=sideFaces.end();++it)publishFace(it.value(),"side/"+it.key().toUtf8(),it.key());
    QSet<int> published;
    auto publishEdge=[&](int index,const QByteArray& role,const QString& sourceEdge=QString{}){
        if(index<1 || published.contains(index))reject("thicken_lineage_unavailable","Duplicate result edge identity");
        published.insert(index);const auto edge=TopoDS::Edge(edges(index));BRepAdaptor_Curve curve(edge);
        QJsonObject provenance{{"sourceBodyId",source.bodyId},{"sourceFaceId",ref["faceId"]},{"sourceEdgeId",sourceEdge},
            {"role",QString::fromLatin1(role)},{"startMm",point(curve.Value(curve.FirstParameter()))},
            {"endMm",point(curve.Value(curve.LastParameter()))}};
        const auto edgeId=id(body.featureId,QByteArray("edge/")+role);
        body.solidEdges.push_back({edgeId,edge,provenance});
        for(auto& face:body.supportFaces)if(faceEdges[faces.FindIndex(face.face)].contains(index))
            face.boundaryEdges.push_back({edgeId,curve.GetType()==GeomAbs_Line ? "line":"curve",edge,provenance});
    };
    for(auto it=sourceEdges.begin();it!=sourceEdges.end();++it){
        publishEdge(it.value(),"source/"+it.key().toUtf8(),it.key());
        publishEdge(offsetEdges[it.key()],"offset/"+it.key().toUtf8(),it.key());
    }
    TopTools_IndexedMapOfShape sourceVertices;TopExp::MapShapes(sourceFace,TopAbs_VERTEX,sourceVertices);
    if(sourceVertices.Extent()!=4)reject("thicken_lineage_unavailable","The source corners are ambiguous");
    for(int i=1;i<=sourceVertices.Extent();++i){
        QStringList meeting;for(auto it=sourceEdges.begin();it!=sourceEdges.end();++it){
            TopoDS_Vertex a,b;TopExp::Vertices(TopoDS::Edge(edges(it.value())),a,b,true);
            if(a.IsSame(sourceVertices(i)) || b.IsSame(sourceVertices(i)))meeting.append(it.key());
        }
        if(meeting.size()!=2)reject("thicken_lineage_unavailable","A source corner lacks two retained rims");
        meeting.sort();int connector=0;
        for(int j=1;j<=edges.Extent();++j)if(!published.contains(j)){
            TopoDS_Vertex a,b;TopExp::Vertices(TopoDS::Edge(edges(j)),a,b,true);
            if(a.IsSame(sourceVertices(i)) || b.IsSame(sourceVertices(i))){if(connector)reject("thicken_lineage_unavailable","Corner connector is ambiguous");connector=j;}
        }
        if(!connector)reject("thicken_lineage_unavailable","Corner connector is missing");
        publishEdge(connector,"corner/"+meeting[0].toUtf8()+"/"+meeting[1].toUtf8());
    }
    if(published.size()!=12)reject("thicken_lineage_unavailable","The result has unnamed topology");
    body.summary=GeometryKernel::summarize(body.shape);
    if(!body.summary["valid"].toBool() || !body.summary["closed"].toBool()
        || body.summary["topology"].toObject()["solids"]!=1 || body.summary["volumeMm3"].toDouble()<=0)
        reject("thicken_failed","Thicken is not one positive-volume closed solid");
    body.summary["sourceFace"]=ref;body.summary["signedThicknessMm"]=thickness;
    body.summary["supportFaces"]=PlanarFaces::descriptors(body.supportFaces);
    return body;
}
