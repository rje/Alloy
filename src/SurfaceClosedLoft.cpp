// SPDX-License-Identifier: MIT
#include "SurfaceClosedLoft.h"
#include "PlanarFaces.h"
#include <BRepOffsetAPI_ThruSections.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <BRep_Tool.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Wire.hxx>
#include <TopLoc_Location.hxx>
#include <QUuid>
#include <QStringList>
#include <QSet>
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <vector>

namespace {
QString stableId(const QString& feature,const QByteArray& role) {
    return QUuid::createUuidV5(QUuid(feature),QByteArray("alloy-closed-surface-loft/1/")+role)
        .toString(QUuid::WithoutBraces);
}
gp_Pnt startPoint(const TopoDS_Edge& edge) {
    TopoDS_Vertex a,b;TopExp::Vertices(edge,a,b,true);return BRep_Tool::Pnt(a);
}
gp_Pnt endPoint(const TopoDS_Edge& edge) {
    TopoDS_Vertex a,b;TopExp::Vertices(edge,a,b,true);return BRep_Tool::Pnt(b);
}
double curveDistance(const gp_Pnt& point,const TopoDS_Edge& edge) {
    BRepExtrema_DistShapeShape distance(BRepBuilderAPI_MakeVertex(point),edge);
    distance.Perform();return distance.IsDone() ? distance.Value() : std::numeric_limits<double>::infinity();
}
bool sameSampledCurve(const TopoDS_Edge& a,const TopoDS_Edge& b) {
    if(a.IsSame(b))return true;
    BRepAdaptor_Curve first(a),second(b);
    if(first.GetType()!=second.GetType())return false;
    GProp_GProps firstLength,secondLength;
    BRepGProp::LinearProperties(a,firstLength);BRepGProp::LinearProperties(b,secondLength);
    if(std::abs(firstLength.Mass()-secondLength.Mass())>std::max(1e-6,firstLength.Mass()*1e-8))return false;
    for(int step=0;step<=8;++step) {
        const double ratio=step/8.;
        if(curveDistance(first.Value(first.FirstParameter()+(first.LastParameter()-first.FirstParameter())*ratio),b)>1e-6
            || curveDistance(second.Value(second.FirstParameter()+(second.LastParameter()-second.FirstParameter())*ratio),a)>1e-6)
            return false;
    }
    return true;
}
bool sameUnderlyingCurve(const TopoDS_Edge& a,const TopoDS_Edge& b) {
    if(a.IsSame(b))return true;
    TopLoc_Location firstLocation,secondLocation;
    Standard_Real firstStart,firstEnd,secondStart,secondEnd;
    const auto first=BRep_Tool::Curve(a,firstLocation,firstStart,firstEnd);
    const auto second=BRep_Tool::Curve(b,secondLocation,secondStart,secondEnd);
    return !first.IsNull() && first==second && firstLocation.IsEqual(secondLocation)
        && std::abs(firstStart-secondStart)<1e-10 && std::abs(firstEnd-secondEnd)<1e-10;
}
struct Section {
    const EvaluatedSketch* sketch=nullptr;
    QStringList sourceIds;
    std::vector<TopoDS_Edge> edges;
    TopoDS_Wire wire;
    gp_Pnt firstAuthoredStart,firstAuthoredEnd;
};
}

EvaluatedBody SurfaceClosedLoft::evaluate(const QJsonObject& feature,const EvaluatedDocument& prior) {
    const auto parameters=feature["parameters"].toObject();
    const auto references=parameters["sections"].toArray();
    auto reject=[&](const QString& code,const QString& message)->void {
        throw GeometryKernel::EvaluationError({{"featureId",feature["id"]},{"featureType","loft"},
            {"code",code},{"message",message}});
    };
    if(references.size()!=3)reject("loft_sections_required","Select three ordered closed surface sections");
    std::array<Section,3> sections;
    QSet<QString> sourceSketches;
    for(int index=0;index<3;++index) {
        auto& section=sections[index];const auto reference=references[index].toObject();
        const auto sketchId=reference["sketchId"].toString();
        const auto ids=reference["entityIds"].toArray();
        if(sourceSketches.contains(sketchId))
            reject("loft_section_unsupported","Each closed section must come from its own sketch");
        sourceSketches.insert(sketchId);
        for(const auto& candidate:prior.sketches)if(candidate.featureId==sketchId){section.sketch=&candidate;break;}
        if(!section.sketch)
            reject("loft_source_unavailable","Each section must be an earlier root sketch");
        if(ids.size()!=(index==1 ? 1 : 2))reject("loft_section_unsupported","Use two fit splines, one full ellipse, then two fit splines");
        for(const auto& value:ids) {
            const auto id=value.toString();
            const auto expected=index==1 ? "ellipse" : "fitSpline";
            bool found=false;
            for(const auto& row:section.sketch->summary["entities"].toArray()) {
                const auto entity=row.toObject();
                if(entity["id"]==id && entity["type"]==expected && !entity["construction"].toBool())found=true;
            }
            if(!found || !section.sketch->entityEdges.contains(id))
                reject("loft_source_unavailable","A selected section curve is no longer an ordinary solved spline or ellipse");
            auto edge=TopoDS::Edge(BRepBuilderAPI_Copy(section.sketch->entityEdges.value(id),false,false).Shape());
            BRepAdaptor_Curve curve(edge);
            if(index==1 ? curve.GetType()!=GeomAbs_Ellipse || !curve.IsClosed()
                        : curve.GetType()!=GeomAbs_BSplineCurve || curve.IsClosed())
                reject("loft_section_unsupported","The selected section curve has changed its required analytic type");
            section.sourceIds.append(id);section.edges.push_back(edge);
        }
        BRepBuilderAPI_MakeWire assembled;
        section.firstAuthoredStart=startPoint(section.edges[0]);
        section.firstAuthoredEnd=endPoint(section.edges[0]);
        assembled.Add(section.edges[0]);
        if(index!=1) {
            const auto a0=startPoint(section.edges[0]),a1=endPoint(section.edges[0]);
            const auto b0=startPoint(section.edges[1]),b1=endPoint(section.edges[1]);
            const bool same=a0.Distance(b0)<1e-6 && a1.Distance(b1)<1e-6;
            const bool opposite=a0.Distance(b1)<1e-6 && a1.Distance(b0)<1e-6;
            if(a0.Distance(a1)<1e-6 || (!same && !opposite))
                reject("loft_section_open","Each two-spline section must close at exactly two distinct shared endpoints");
            assembled.Add(same ? TopoDS::Edge(section.edges[1].Reversed()) : section.edges[1]);
        }
        if(!assembled.IsDone() || !assembled.Wire().Closed() || !BRepCheck_Analyzer(assembled.Wire()).IsValid())
            reject("loft_wire_failed","OCCT could not assemble a valid closed section wire");
        section.wire=assembled.Wire();
        TopTools_IndexedMapOfShape assembledEdges;TopExp::MapShapes(section.wire,TopAbs_EDGE,assembledEdges);
        if(assembledEdges.Extent()!=ids.size())reject("loft_wire_failed","Section wire edge count changed during assembly");
        // MakeWire may replace edge vertices. Rebind authored identities to the
        // assembled edges that ThruSections actually receives, never to copies.
        std::vector<TopoDS_Edge> rebound(section.edges.size());
        for(int n=1;n<=assembledEdges.Extent();++n) {
            const auto current=TopoDS::Edge(assembledEdges(n));
            int match=-1;
            for(int k=0;k<static_cast<int>(section.edges.size());++k)
                if(sameUnderlyingCurve(current,section.edges[k]) || sameSampledCurve(current,section.edges[k])) {
                    if(match>=0)reject("loft_lineage_ambiguous","Two source curves match one assembled wire edge");
                    match=k;
                }
            if(match<0 || !rebound[match].IsNull())reject("loft_lineage_ambiguous","An assembled wire edge has no unique source curve");
            rebound[match]=current;
        }
        section.edges=std::move(rebound);
    }
    BRepOffsetAPI_ThruSections loft(false,false,1e-6);
    loft.SetMutableInput(false);loft.CheckCompatibility(true);
    for(const auto& section:sections)loft.AddWire(section.wire);
    loft.Build();
    if(!loft.IsDone())reject("loft_failed","OCCT could not make the three-section surface Loft");
    EvaluatedBody body;body.featureId=feature["id"].toString();body.bodyId=feature["bodyId"].toString();
    body.name=feature["name"].toString();body.visible=feature["visible"].toBool();body.shape=loft.Shape();
    body.summary=GeometryKernel::summarize(body.shape);
    const auto topology=body.summary["topology"].toObject();
    if(!body.summary["valid"].toBool() || body.shape.Closed() || topology["shells"]!=1
        || topology["faces"]!=2 || topology["solids"]!=0)
        reject("loft_invalid","No-guide Loft must produce one valid two-face open shell without caps");
    TopTools_IndexedMapOfShape resultFaces,resultEdges;
    TopExp::MapShapes(body.shape,TopAbs_FACE,resultFaces);
    TopExp::MapShapes(body.shape,TopAbs_EDGE,resultEdges);
    // All three retained inputs must still contact the evaluated surface; the
    // middle ellipse is an interpolation section, not necessarily an output edge.
    double maxSectionDistance=0;
    for(const auto& section:sections)for(const auto& edge:section.edges) {
        BRepAdaptor_Curve curve(edge);
        for(int sample=0;sample<=32;++sample) {
            const auto t=curve.FirstParameter()+(curve.LastParameter()-curve.FirstParameter())*sample/32.;
            BRepExtrema_DistShapeShape distance(BRepBuilderAPI_MakeVertex(curve.Value(t)),body.shape);
            distance.Perform();
            if(!distance.IsDone())reject("loft_section_unresolved","OCCT could not verify a retained section on the result");
            maxSectionDistance=std::max(maxSectionDistance,distance.Value());
        }
    }
    if(maxSectionDistance>1e-4)reject("loft_section_unresolved","A selected section no longer lies on the resulting surface");
    std::array<int,2> faceIndices{};
    for(int source=0;source<2;++source) {
        const auto generated=loft.GeneratedFace(sections[0].edges[source]);
        faceIndices[source]=generated.IsNull() ? 0 : resultFaces.FindIndex(generated);
        if(faceIndices[source]==0 || (source==1 && faceIndices[0]==faceIndices[1]))
            reject("loft_lineage_ambiguous","The first section curves do not uniquely identify the generated faces");
    }
    // Match exterior result edges by builder/curve identity when retained, or
    // unique bidirectional sampled correspondence after OCCT replacement. The
    // middle ellipse is intentionally not assigned a boundary identity.
    std::vector<QString> edgeIds(resultEdges.Extent()+1);
    std::vector<QJsonObject> edgeProvenance(resultEdges.Extent()+1);
    for(int sectionIndex:{0,2})for(int source=0;source<2;++source) {
        const auto& section=sections[sectionIndex];
        int match=0;
        for(int edgeIndex=1;edgeIndex<=resultEdges.Extent();++edgeIndex) {
            if(sameUnderlyingCurve(section.edges[source],TopoDS::Edge(resultEdges(edgeIndex)))
                || sameSampledCurve(section.edges[source],TopoDS::Edge(resultEdges(edgeIndex)))) {
                if(match)reject("loft_lineage_ambiguous","One source boundary matches multiple result edges");
                match=edgeIndex;
            }
        }
        if(!match || !edgeIds[match].isEmpty())reject("loft_lineage_ambiguous","A source boundary lacks a unique result edge");
        const auto role=sectionIndex==0 ? "start" : "end";
        edgeIds[match]=stableId(body.featureId,QByteArray("edge/")+role+'/'+section.sourceIds[source].toUtf8());
        edgeProvenance[match]={{"generatorFeatureId",body.featureId},{"sourceSketchId",section.sketch->featureId},
            {"sourceEntityId",section.sourceIds[source]},{"role",role},{"lineage","assembledThruSections"}};
    }
    // The two remaining boundary seams correspond to the stable start/end of
    // the first authored spline. Refuse any topology requiring arbitrary edge
    // enumeration or an unsupported split seam.
    if(resultEdges.Extent()!=6)reject("loft_lineage_ambiguous","This no-guide Loft needs two unsplit source-endpoint seams");
    for(int side=0;side<2;++side) {
        const auto point=side==0 ? sections[0].firstAuthoredStart : sections[0].firstAuthoredEnd;
        int match=0;
        for(int edgeIndex=1;edgeIndex<=resultEdges.Extent();++edgeIndex)if(edgeIds[edgeIndex].isEmpty()) {
            const auto edge=TopoDS::Edge(resultEdges(edgeIndex));
            if(point.Distance(startPoint(edge))<1e-6 || point.Distance(endPoint(edge))<1e-6) {
                if(match)reject("loft_lineage_ambiguous","A section endpoint matches multiple seam edges");
                match=edgeIndex;
            }
        }
        if(!match)reject("loft_lineage_ambiguous","A section endpoint has no unique generated seam");
        const auto label=side==0 ? "start" : "end";
        edgeIds[match]=stableId(body.featureId,QByteArray("edge/seam/")+label+'/'+sections[0].sourceIds[0].toUtf8());
        edgeProvenance[match]={{"generatorFeatureId",body.featureId},{"sourceSketchId",sections[0].sketch->featureId},
            {"sourceEntityId",sections[0].sourceIds[0]},{"role",QString("seam.")+label},{"lineage","assembledThruSections"}};
    }
    for(int edgeIndex=1;edgeIndex<=resultEdges.Extent();++edgeIndex) {
        if(edgeIds[edgeIndex].isEmpty())reject("loft_lineage_ambiguous","A surface edge lacks retained source lineage");
        body.solidEdges.push_back({edgeIds[edgeIndex],TopoDS::Edge(resultEdges(edgeIndex)),edgeProvenance[edgeIndex]});
    }
    for(int source=0;source<2;++source) {
        EvaluatedSupportFace face{stableId(body.featureId,QByteArray("face/")+sections[0].sourceIds[source].toUtf8()),
            body.bodyId,body.featureId,"loft.closedSection",TopoDS::Face(resultFaces(faceIndices[source])),{}, {}};
        face.provenance={{"generatorFeatureId",body.featureId},{"sourceSketchId",sections[0].sketch->featureId},
            {"sourceEntityId",sections[0].sourceIds[source]},{"orderedSections",references},
            {"lineage","assembledThruSections"}};
        for(TopExp_Explorer edge(face.face,TopAbs_EDGE);edge.More();edge.Next()) {
            const auto resultIndex=resultEdges.FindIndex(edge.Current());
            if(resultIndex<1 || edgeIds[resultIndex].isEmpty())
                reject("loft_lineage_ambiguous","A generated face boundary lacks retained edge identity");
            face.boundaryEdges.push_back({edgeIds[resultIndex],"curve",TopoDS::Edge(resultEdges(resultIndex)),edgeProvenance[resultIndex]});
        }
        body.supportFaces.push_back(std::move(face));
    }
    body.summary["supportFaces"]=PlanarFaces::descriptors(body.supportFaces);
    body.summary["operation"]="newComponent";body.summary["guideType"]="none";
    body.summary["sections"]=references;body.summary["maxSectionDistanceMm"]=maxSectionDistance;
    body.summary["interpolation"]=QJsonObject{{"kernel","OCCT ThruSections"},{"ruled",false},
        {"closed",false},{"compatibility",true},{"mutableInput",false}};
    return body;
}
