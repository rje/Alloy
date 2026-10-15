// SPDX-License-Identifier: MIT
#include "PlanarBoundaryInset.h"
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepClass_FaceClassifier.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepGProp.hxx>
#include <BRepTools.hxx>
#include <BRepTools_WireExplorer.hxx>
#include <BRep_Tool.hxx>
#include <ElCLib.hxx>
#include <GProp_GProps.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Wire.hxx>
#include <QUuid>
#include <QSet>
#include <cmath>
#include <set>

namespace {
[[noreturn]] void fail(const QString& code,const QString& message) {
    throw GeometryKernel::EvaluationError({{"code",code},{"message",message}});
}
int count(const TopoDS_Shape& shape,TopAbs_ShapeEnum kind) {
    int total=0;for(TopExp_Explorer it(shape,kind);it.More();it.Next())++total;return total;
}
double area(const TopoDS_Shape& shape) {
    GProp_GProps props;BRepGProp::SurfaceProperties(shape,props);return std::abs(props.Mass());
}
TopoDS_Face cutterFace(const std::vector<TopoDS_Edge>& edges) {
    BRepBuilderAPI_MakeWire wire;for(const auto& edge:edges)wire.Add(edge);
    if(!wire.IsDone())fail("invalid_inset_cutter","An analytic inset cutter did not close");
    BRepBuilderAPI_MakeFace maker(wire.Wire(),true);
    if(!maker.IsDone() || !BRepCheck_Analyzer(maker.Face()).IsValid())
        fail("invalid_inset_cutter","An analytic inset cutter is not a valid planar face");
    return maker.Face();
}
TopoDS_Face lineStrip(const TopoDS_Edge& edge,const gp_Dir& normal,double distance) {
    const BRepAdaptor_Curve curve(edge);
    const auto a=curve.Value(curve.FirstParameter()),b=curve.Value(curve.LastParameter());
    gp_Vec tangent(a,b);
    if(tangent.Magnitude()<1e-7)fail("unsupported_inset_boundary","A source line has no finite length");
    tangent.Normalize();const gp_Vec across=gp_Vec(normal).Crossed(tangent)*distance;
    const auto a0=a.Translated(across),a1=a.Translated(-across);
    const auto b0=b.Translated(across),b1=b.Translated(-across);
    return cutterFace({BRepBuilderAPI_MakeEdge(a0,b0).Edge(),BRepBuilderAPI_MakeEdge(b0,b1).Edge(),
        BRepBuilderAPI_MakeEdge(b1,a1).Edge(),BRepBuilderAPI_MakeEdge(a1,a0).Edge()});
}
TopoDS_Face arcSector(const TopoDS_Edge& edge,double distance) {
    const BRepAdaptor_Curve curve(edge);
    auto outer=curve.Circle(),inner=curve.Circle();
    if(inner.Radius()<=distance+1e-7)
        fail("unsupported_inset_boundary","Inset distance reaches a source arc center");
    outer.SetRadius(outer.Radius()+distance);inner.SetRadius(inner.Radius()-distance);
    const auto first=curve.FirstParameter(),last=curve.LastParameter();
    const auto outerStart=ElCLib::Value(first,outer),outerEnd=ElCLib::Value(last,outer);
    const auto innerStart=ElCLib::Value(first,inner),innerEnd=ElCLib::Value(last,inner);
    return cutterFace({BRepBuilderAPI_MakeEdge(outer,first,last).Edge(),
        BRepBuilderAPI_MakeEdge(outerEnd,innerEnd).Edge(),
        TopoDS::Edge(BRepBuilderAPI_MakeEdge(inner,first,last).Edge().Reversed()),
        BRepBuilderAPI_MakeEdge(innerStart,outerStart).Edge()});
}
TopoDS_Face endpointDisk(const gp_Pnt& point,const gp_Dir& normal,double distance) {
    const gp_Circ circle(gp_Ax2(point,normal),distance);
    return cutterFace({BRepBuilderAPI_MakeEdge(circle).Edge()});
}
struct Source {TopoDS_Edge edge;TopoDS_Vertex first;QString id,vertexKey;};
struct Lineage {TopoDS_Edge edge;std::set<QString> labels;};
QString labelKey(const std::set<QString>& labels) {
    QStringList sorted;for(const auto& label:labels)sorted.append(label);return sorted.join("\n");
}
}

PlanarBoundaryInset::Result PlanarBoundaryInset::evaluate(const EvaluatedSupportFace& support,
    double distanceMm,const QString& insetFeatureId) {
    if(!std::isfinite(distanceMm) || distanceMm<0.001 || distanceMm>100000)
        fail("invalid_inset_distance","Inset distance must be positive and finite");
    if(QUuid(insetFeatureId).isNull())
        fail("invalid_inset_identity","Inset requires a stable owning feature ID");
    if(support.face.IsNull() || QUuid(support.faceId).isNull() || count(support.face,TopAbs_FACE)!=1
        || count(support.face,TopAbs_WIRE)!=1 || !BRepCheck_Analyzer(support.face).IsValid())
        fail("unsupported_inset_boundary","Inset requires one valid retained planar face with one boundary");
    const BRepAdaptor_Surface surface(support.face);
    if(surface.GetType()!=GeomAbs_Plane)
        fail("unsupported_inset_boundary","Inset requires an exact planar support face");
    const auto normal=surface.Plane().Axis().Direction();
    const auto wire=BRepTools::OuterWire(support.face);
    if(wire.IsNull() || !wire.Closed())
        fail("unsupported_inset_boundary","Inset requires one closed outer wire");
    std::vector<Source> sources;QSet<QString> sourceIds;
    for(BRepTools_WireExplorer it(wire,support.face);it.More();it.Next()) {
        const auto edge=TopoDS::Edge(it.Current());
        QString id;int matches=0;
        for(const auto& retained:support.boundaryEdges)if(retained.edge.IsSame(edge)){
            id=retained.edgeId;++matches;
        }
        if(matches!=1 || QUuid(id).isNull() || sourceIds.contains(id))
            fail("ambiguous_inset_lineage","Every boundary edge needs one distinct retained source ID");
        sourceIds.insert(id);
        const BRepAdaptor_Curve curve(edge);
        if(curve.GetType()!=GeomAbs_Line && curve.GetType()!=GeomAbs_Circle)
            fail("unsupported_inset_boundary","Inset supports only finite straight and circular edges");
        sources.push_back({edge,TopExp::FirstVertex(edge,true),id,{}});
    }
    if(sources.size()<3 || sources.size()!=size_t(count(wire,TopAbs_EDGE))
        || sources.size()!=support.boundaryEdges.size())
        fail("ambiguous_inset_lineage","The support wire and retained edge registry differ");
    QSet<QString> vertexKeys;
    for(size_t i=0;i<sources.size();++i) {
        const auto& previous=sources[(i+sources.size()-1)%sources.size()];
        if(!TopExp::LastVertex(previous.edge,true).IsSame(sources[i].first))
            fail("unsupported_inset_boundary","Source wire has a disconnected corner");
        QStringList incident{previous.id,sources[i].id};incident.sort();
        sources[i].vertexKey=incident.join("/");
        if(vertexKeys.contains(sources[i].vertexKey))
            fail("ambiguous_inset_lineage","Two corners have the same retained incident edges");
        vertexKeys.insert(sources[i].vertexKey);
    }
    TopoDS_Shape result=support.face;
    std::vector<Lineage> lineage;
    for(const auto& source:sources)lineage.push_back({source.edge,{"edge:"+source.id}});
    auto subtract=[&](const TopoDS_Face& cutter,const QString& label) {
        BRepAlgoAPI_Cut operation(result,cutter);operation.Build();
        if(!operation.IsDone())fail("inset_boolean_failed","Exact boundary-neighborhood subtraction failed");
        const auto next=operation.Shape();
        if(next.IsNull() || count(next,TopAbs_FACE)!=1 || !BRepCheck_Analyzer(next).IsValid())
            fail("inset_disconnected","Inset must remain one valid planar face after each cutter");
        std::vector<Lineage> descendants;
        for(TopExp_Explorer it(next,TopAbs_EDGE);it.More();it.Next())
            descendants.push_back({TopoDS::Edge(it.Current()),{}});
        auto assign=[&](const TopoDS_Shape& edge,const std::set<QString>& labels) {
            if(edge.ShapeType()!=TopAbs_EDGE)return;
            for(auto& descendant:descendants)if(descendant.edge.IsSame(edge))
                descendant.labels.insert(labels.begin(),labels.end());
        };
        auto propagate=[&](const TopoDS_Shape& edge,const std::set<QString>& labels) {
            assign(edge,labels);
            for(const auto& modified:operation.Modified(edge))assign(modified,labels);
            for(const auto& generated:operation.Generated(edge))assign(generated,labels);
        };
        for(const auto& entry:lineage)propagate(entry.edge,entry.labels);
        for(TopExp_Explorer it(cutter,TopAbs_EDGE);it.More();it.Next())
            propagate(it.Current(),{label});
        lineage=std::move(descendants);result=next;
    };
    for(const auto& source:sources) {
        const BRepAdaptor_Curve curve(source.edge);
        subtract(curve.GetType()==GeomAbs_Line?lineStrip(source.edge,normal,distanceMm)
            :arcSector(source.edge,distanceMm),"edge:"+source.id);
    }
    for(const auto& source:sources)
        subtract(endpointDisk(BRep_Tool::Pnt(source.first),normal,distanceMm),"vertex:"+source.vertexKey);
    if(count(result,TopAbs_WIRE)!=1 || area(result)<=1e-7)
        fail("inset_disconnected","Inset produced no one connected positive-area loop");
    BRepAlgoAPI_Cut outside(result,support.face);outside.Build();
    const double tolerance=std::max(1e-5,area(support.face)*1e-7);
    if(!outside.IsDone() || area(outside.Shape())>tolerance)
        fail("invalid_inset","Inset extends outside the source face");
    auto insetFace=TopoDS::Face(TopExp_Explorer(result,TopAbs_FACE).Current());
    const auto resultWire=BRepTools::OuterWire(insetFace);
    for(TopExp_Explorer it(resultWire,TopAbs_EDGE);it.More();it.Next()) {
        const BRepAdaptor_Curve curve(TopoDS::Edge(it.Current()));
        for(int i=0;i<=16;++i) {
            const auto parameter=curve.FirstParameter()+(curve.LastParameter()-curve.FirstParameter())*i/16.0;
            const auto sample=curve.Value(parameter);
            BRepClass_FaceClassifier classification(support.face,sample,1e-7);
            BRepExtrema_DistShapeShape distance(BRepBuilderAPI_MakeVertex(sample).Vertex(),wire);
            if(classification.State()==TopAbs_OUT || !distance.IsDone()
                || distance.Value()<distanceMm-std::max(1e-5,distanceMm*1e-6))
                fail("invalid_inset","Inset boundary failed source containment or clearance");
        }
    }
    Result output;output.face=insetFace;
    QSet<QString> usedKeys,usedSourceIds;
    for(const auto& entry:lineage) {
        if(entry.labels.empty())fail("ambiguous_inset_lineage","Inset edge has no source ancestry");
        const auto key=labelKey(entry.labels);
        if(usedKeys.contains(key))fail("ambiguous_inset_lineage","Two inset edges have the same source ancestry");
        usedKeys.insert(key);
        QJsonArray edgeIds,vertexKeys;
        for(const auto& label:entry.labels) {
            if(label.startsWith("edge:")){const auto id=label.mid(5);edgeIds.append(id);usedSourceIds.insert(id);}
            else if(label.startsWith("vertex:"))vertexKeys.append(label.mid(7));
        }
        const auto id=QUuid::createUuidV5(QUuid(insetFeatureId),
            QByteArray("alloy-planar-boundary-inset/1\n")+support.faceId.toUtf8()+'\n'+key.toUtf8())
            .toString(QUuid::WithoutBraces);
        const BRepAdaptor_Curve curve(entry.edge);
        output.boundaryEdges.push_back({id,curve.GetType()==GeomAbs_Circle?"arc":"line",entry.edge,
            QJsonObject{{"sourceFaceId",support.faceId},{"insetFeatureId",insetFeatureId},{"sourceEdgeIds",edgeIds},
                {"sourceVertexKeys",vertexKeys},{"distanceMm",distanceMm},{"role","planarBoundaryInset"}}});
    }
    for(const auto& source:sources)if(!usedSourceIds.contains(source.id))
        output.vanishedSourceEdgeIds.append(source.id);
    output.vanishedSourceEdgeIds.sort();
    output.summary={{"areaMm2",area(insetFace)},{"sourceAreaMm2",area(support.face)},
        {"distanceMm",distanceMm},{"edgeCount",int(output.boundaryEdges.size())},
        {"vanishedSourceEdgeIds",QJsonArray::fromStringList(output.vanishedSourceEdgeIds)},
        {"sourceFaceId",support.faceId},{"insetFeatureId",insetFeatureId}};
    return output;
}
