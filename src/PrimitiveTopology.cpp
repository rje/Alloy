// SPDX-License-Identifier: MIT
#include "PrimitiveTopology.h"
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRep_Tool.hxx>
#include <QMap>
#include <QUuid>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <array>
#include <cmath>

namespace {
QJsonArray point(const gp_Pnt& p) {return {p.X(),p.Y(),p.Z()};}
QJsonArray axis(const gp_Dir& d) {return {d.X(),d.Y(),d.Z()};}
QString id(const QString& feature,const QByteArray& kind,const QString& semantic) {
    return QUuid::createUuidV5(QUuid(feature),"alloy-box/1\n"+kind+'\n'+semantic.toUtf8())
        .toString(QUuid::WithoutBraces);
}
[[noreturn]] void ambiguous(const QString& what) {
    throw GeometryKernel::EvaluationError({{"code","primitive_topology_ambiguous"},
        {"message","Box kernel did not produce unique semantic "+what}});
}
int side(double value,double low,double high) {
    if(std::abs(value-low)<1e-6)return 0;
    if(std::abs(value-high)<1e-6)return 1;
    return -1;
}
}

void PrimitiveTopology::box(const TopoDS_Shape& shape,const QJsonObject& p,
    const QString& featureId,const QString& bodyId,
    std::vector<EvaluatedSupportFace>& faces,std::vector<EvaluatedSolidEdge>& edges) {
    const std::array<double,3> low{p["xMm"].toDouble(),p["yMm"].toDouble(),p["zMm"].toDouble()};
    const std::array<double,3> high{low[0]+p["widthMm"].toDouble(),low[1]+p["depthMm"].toDouble(),low[2]+p["heightMm"].toDouble()};
    const char* names[3]={"x","y","z"};
    TopTools_IndexedMapOfShape actualEdges;TopExp::MapShapes(shape,TopAbs_EDGE,actualEdges);
    if(actualEdges.Extent()!=12)ambiguous("edges");
    QMap<QString,EvaluatedSolidEdge> byName;
    for(int i=1;i<=actualEdges.Extent();++i) {
        const auto edge=TopoDS::Edge(actualEdges(i));
        if(BRepAdaptor_Curve(edge).GetType()!=GeomAbs_Line)ambiguous("straight edges");
        TopoDS_Vertex first,last;TopExp::Vertices(edge,first,last,true);
        if(first.IsNull() || last.IsNull())ambiguous("edge endpoints");
        const auto a=BRep_Tool::Pnt(first),b=BRep_Tool::Pnt(last);
        const std::array<double,3> av{a.X(),a.Y(),a.Z()},bv{b.X(),b.Y(),b.Z()};
        QString semantic;int freeAxes=0;
        for(int k=0;k<3;++k) {
            const auto as=side(av[k],low[k],high[k]),bs=side(bv[k],low[k],high[k]);
            if(as<0 || bs<0)ambiguous("edge corner");
            if(as!=bs){semantic+=QString("%1*").arg(names[k]);++freeAxes;}
            else semantic+=QString("%1%2").arg(names[k]).arg(as);
        }
        if(freeAxes!=1 || byName.contains(semantic))ambiguous("edge identity");
        // Give each edge an ordered low-to-high endpoint along its free axis.
        gp_Pnt start=a,end=b;
        if((av[0]+av[1]+av[2])>(bv[0]+bv[1]+bv[2]))std::swap(start,end);
        QJsonObject provenance{{"role","box.edge"},{"boxEdge",semantic},
            {"generatorFeatureId",featureId},{"endpointProvenance","currentKernelBoundary"},
            {"startMm",point(start)},{"endMm",point(end)}};
        byName.insert(semantic,{id(featureId,"edge",semantic),edge,provenance});
    }
    if(byName.size()!=12)ambiguous("edge identities");
    for(const auto& edge:byName)edges.push_back(edge);

    TopTools_IndexedMapOfShape actualFaces;TopExp::MapShapes(shape,TopAbs_FACE,actualFaces);
    if(actualFaces.Extent()!=6)ambiguous("faces");
    QMap<QString,EvaluatedSupportFace> byFace;
    for(int i=1;i<=actualFaces.Extent();++i) {
        const auto face=TopoDS::Face(actualFaces(i));
        if(BRepAdaptor_Surface(face).GetType()!=GeomAbs_Plane)ambiguous("planar faces");
        TopTools_IndexedMapOfShape vertices;TopExp::MapShapes(face,TopAbs_VERTEX,vertices);
        if(vertices.Extent()!=4)ambiguous("face corners");
        int fixed=-1,fixedSide=-1;
        for(int k=0;k<3;++k) {
            const auto first=BRep_Tool::Pnt(TopoDS::Vertex(vertices(1)));
            const std::array<double,3> xyz{first.X(),first.Y(),first.Z()};
            const int candidate=side(xyz[k],low[k],high[k]);
            bool same=candidate>=0;
            for(int v=2;v<=vertices.Extent();++v) {
                const auto pnt=BRep_Tool::Pnt(TopoDS::Vertex(vertices(v)));
                const std::array<double,3> coordinates{pnt.X(),pnt.Y(),pnt.Z()};
                same &= side(coordinates[k],low[k],high[k])==candidate;
            }
            if(same) {if(fixed>=0)ambiguous("face plane");fixed=k;fixedSide=candidate;}
        }
        if(fixed<0)ambiguous("face plane");
        const QString semantic=QString("%1%2").arg(names[fixed]).arg(fixedSide);
        if(byFace.contains(semantic))ambiguous("face identity");
        gp_Dir normal,u,v;
        if(fixed==0 && fixedSide==0){normal=gp::DX().Reversed();u=gp::DZ();v=gp::DY();}
        else if(fixed==0){normal=gp::DX();u=gp::DY();v=gp::DZ();}
        else if(fixed==1 && fixedSide==0){normal=gp::DY().Reversed();u=gp::DX();v=gp::DZ();}
        else if(fixed==1){normal=gp::DY();u=gp::DZ();v=gp::DX();}
        else if(fixedSide==0){normal=gp::DZ().Reversed();u=gp::DY();v=gp::DX();}
        else {normal=gp::DZ();u=gp::DX();v=gp::DY();}
        auto actual=BRepAdaptor_Surface(face).Plane().Axis().Direction();
        if(face.Orientation()==TopAbs_REVERSED)actual.Reverse();
        if(actual.Dot(normal)<1-1e-9)ambiguous("face orientation");
        std::array<double,3> origin=low;origin[fixed]=fixedSide ? high[fixed] : low[fixed];
        QJsonObject frame{{"origin",QJsonArray{origin[0],origin[1],origin[2]}},
            {"uAxis",axis(u)},{"vAxis",axis(v)},{"normal",axis(normal)}};
        EvaluatedSupportFace support{id(featureId,"face",semantic),bodyId,featureId,
            "box."+semantic,face,frame,{},{{"boxFace",semantic}}};
        TopTools_IndexedMapOfShape boundary;TopExp::MapShapes(face,TopAbs_EDGE,boundary);
        if(boundary.Extent()!=4)ambiguous("face boundary");
        for(int edgeIndex=1;edgeIndex<=boundary.Extent();++edgeIndex) {
            const auto actual=TopoDS::Edge(boundary(edgeIndex));
            const auto found=std::find_if(edges.begin(),edges.end(),[&](const auto& edge){return edge.edge.IsSame(actual);});
            if(found==edges.end())ambiguous("face edge lineage");
            support.boundaryEdges.push_back({found->edgeId,"line",found->edge,found->provenance});
        }
        byFace.insert(semantic,support);
    }
    if(byFace.size()!=6)ambiguous("face identities");
    for(const auto& face:byFace)faces.push_back(face);
}
