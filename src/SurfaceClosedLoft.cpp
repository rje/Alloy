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
#include <BRepTools.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <Geom_Surface.hxx>
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
    QString sourceBodyId,sourceProducerId,sourceFaceId,startPointId,endPointId;
    std::vector<QJsonObject> edgeProvenance;
};
[[noreturn]] void boundaryReject(const QString& message){
    throw GeometryKernel::EvaluationError({{"featureType","loft"},{"code","loft_boundary_unavailable"},{"message",message}});
}
const EvaluatedBody& boundaryBody(const EvaluatedDocument& prior,const QString& bodyId){
    for(const auto& body:prior.bodies)if(body.bodyId==bodyId){
        if(body.summary["type"]!="surface" || body.summary["topology"].toObject()["solids"].toInt()!=0 || !body.occurrenceId.isEmpty())
            boundaryReject("Select an earlier local trimmed surface body");return body;
    }boundaryReject("The selected surface body is no longer available");
}
QJsonObject boundaryReference(const EvaluatedDocument& prior,const QString& bodyId,const QString& edgeId){
    const auto& body=boundaryBody(prior,bodyId);QJsonObject result;int matches=0;
    for(const auto& face:body.supportFaces){
        if(face.role!="surfaceTrim.outer" || face.provenance["region"]!="outside")continue;
        const auto outer=BRepTools::OuterWire(face.face);
        for(TopExp_Explorer it(face.face,TopAbs_WIRE);it.More();it.Next()){
            if(it.Current().IsSame(outer))continue;
            TopTools_IndexedMapOfShape edges;TopExp::MapShapes(it.Current(),TopAbs_EDGE,edges);
            if(edges.Extent()!=2 || !it.Current().Closed() || !BRepCheck_Analyzer(it.Current()).IsValid())continue;
            QJsonArray ids;bool selected=false;QString toolSketch,sourceFace;QSet<QString> tools;bool eligible=true;
            for(int i=1;i<=edges.Extent();++i){const EvaluatedSolidEdge* named=nullptr;
                for(const auto& edge:body.solidEdges)if(edge.edge.IsSame(edges(i))){if(named)boundaryReject("An inner edge has ambiguous retained identity");named=&edge;}
                if(!named || named->provenance["role"]!="trimBoundary"
                    || named->provenance["lineage"]!="splitterCutterIntersection"
                    || named->provenance["generatorFeatureId"]!=face.producerFeatureId){eligible=false;break;}
                const auto sketch=named->provenance["sourceSketchId"].toString(),ancestor=named->provenance["sourceFaceId"].toString();
                if((!toolSketch.isEmpty() && toolSketch!=sketch) || (!sourceFace.isEmpty() && sourceFace!=ancestor)){eligible=false;break;}
                toolSketch=sketch;sourceFace=ancestor;tools.insert(named->provenance["sourceEntityId"].toString());
                if(named->edgeId==edgeId)selected=true;else ids.append(named->edgeId);
            }
            if(!selected || !eligible || tools.size()!=2 || ids.size()!=1 || toolSketch.isEmpty()
                || sourceFace!=face.provenance["sourceFaceId"].toString())continue;
            ids.prepend(edgeId);result={{"bodyId",bodyId},{"producerFeatureId",face.producerFeatureId},{"faceId",face.faceId},{"edgeIds",ids}};++matches;
        }
    }
    if(matches!=1)boundaryReject("Select one ordinary edge of a unique retained two-edge inner Trim loop");return result;
}
Section boundarySection(const EvaluatedDocument& prior,const QJsonObject& reference){
    const auto ids=reference["edgeIds"].toArray();
    if(ids.size()!=2 || ids[0]==ids[1] || reference!=boundaryReference(prior,reference["bodyId"].toString(),ids[0].toString()))
        boundaryReject("The retained Trim face or ordered inner-loop edges no longer resolve uniquely");
    const auto& body=boundaryBody(prior,reference["bodyId"].toString());Section section;
    section.sourceBodyId=body.bodyId;section.sourceProducerId=reference["producerFeatureId"].toString();
    section.sourceFaceId=reference["faceId"].toString();
    for(const auto& value:ids){const EvaluatedSolidEdge* named=nullptr;
        for(const auto& edge:body.solidEdges)if(edge.edgeId==value.toString()){if(named)boundaryReject("Duplicate retained Trim edge ID");named=&edge;}
        if(!named || BRep_Tool::Degenerated(named->edge))boundaryReject("A retained ordinary Trim boundary is unavailable");
        const auto sketchId=named->provenance["sourceSketchId"].toString(),entityId=named->provenance["sourceEntityId"].toString();
        const EvaluatedSketch* sketch=nullptr;for(const auto& candidate:prior.sketches)if(candidate.featureId==sketchId)sketch=&candidate;
        QJsonObject entity;if(sketch)for(const auto& candidate:sketch->summary["entities"].toArray())if(candidate.toObject()["id"]==entityId)entity=candidate.toObject();
        const auto pointIds=entity["pointIds"].toArray();
        if(!sketch || entity["type"]!="fitSpline" || pointIds.size()<2 || entity["construction"].toBool())
            boundaryReject("Trim loop requires retained fit-spline cutter endpoint ancestry");
        auto point=[&](const QJsonValue& id){for(const auto& row:sketch->summary["worldPoints"].toArray()){
            const auto p=row.toObject();const auto xyz=p["positionMm"].toArray();if(p["id"]==id && xyz.size()==3)return gp_Pnt(xyz[0].toDouble(),xyz[1].toDouble(),xyz[2].toDouble());
        }boundaryReject("A retained cutter endpoint is unavailable");return gp_Pnt{};};
        const auto normalValues=sketch->resolvedFrame["normal"].toArray();if(normalValues.size()!=3)boundaryReject("Cutter sketch normal unavailable");
        gp_Vec normal(normalValues[0].toDouble(),normalValues[1].toDouble(),normalValues[2].toDouble());
        if(normal.Magnitude()<1e-8)boundaryReject("Cutter sketch normal is degenerate");normal.Normalize();
        auto projects=[&](const gp_Pnt& actual,const gp_Pnt& source){const gp_Vec d(source,actual);return (d-normal*d.Dot(normal)).Magnitude()<1e-6;};
        auto edge=TopoDS::Edge(BRepBuilderAPI_Copy(named->edge,false,false).Shape());
        const auto a=point(pointIds.first()),b=point(pointIds.last());
        const bool direct=projects(startPoint(edge),a)&&projects(endPoint(edge),b);
        const bool reversed=projects(endPoint(edge),a)&&projects(startPoint(edge),b);
        if(direct==reversed)boundaryReject("Trim boundary endpoints need unique retained cutter projection contacts");
        if(reversed)edge.Reverse();
        section.sourceIds.append(named->edgeId);section.edges.push_back(edge);
        section.edgeProvenance.push_back(named->provenance);
        if(section.edges.size()==1){section.firstAuthoredStart=startPoint(edge);section.firstAuthoredEnd=endPoint(edge);
            section.startPointId=pointIds.first().toString();section.endPointId=pointIds.last().toString();}
    }
    return section;
}
}

QJsonObject SurfaceClosedLoft::boundaryReference(const EvaluatedDocument& prior,const QString& bodyId,const QString& edgeId){
    return ::boundaryReference(prior,bodyId,edgeId);
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
        if(reference.contains("bodyId")){
            if(index==1)reject("loft_section_unsupported","The middle section must be one full sketch ellipse");
            try{section=boundarySection(prior,reference);}
            catch(const GeometryKernel::EvaluationError& error){auto diagnostics=error.diagnostics();diagnostics["featureId"]=feature["id"];throw GeometryKernel::EvaluationError(diagnostics);}
        } else {
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
            if(section.edges.empty() && index!=1){const auto pointIds=section.sketch->summary["entities"].toArray();
                for(const auto& row:pointIds)if(row.toObject()["id"]==id){const auto points=row.toObject()["pointIds"].toArray();
                    if(points.size()>=2){section.startPointId=points.first().toString();section.endPointId=points.last().toString();}break;}}
            section.sourceIds.append(id);section.edges.push_back(edge);
        }
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
        if(assembledEdges.Extent()!=int(section.edges.size()))reject("loft_wire_failed","Section wire edge count changed during assembly");
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
    if(!sections[0].sourceBodyId.isEmpty() || !sections[2].sourceBodyId.isEmpty()){
        if(sections[0].sourceBodyId.isEmpty() || sections[0].sourceBodyId!=sections[2].sourceBodyId
            || sections[0].sourceProducerId!=sections[2].sourceProducerId || sections[0].sourceFaceId==sections[2].sourceFaceId)
            reject("loft_section_unsupported","Use opposite retained inner loops from one Trim result");
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
            double distance=std::numeric_limits<double>::infinity();
            for(int face=1;face<=resultFaces.Extent();++face){
                const auto support=TopoDS::Face(resultFaces(face));double u0,u1,v0,v1;BRepTools::UVBounds(support,u0,u1,v0,v1);
                GeomAPI_ProjectPointOnSurf projected(curve.Value(t),BRep_Tool::Surface(support),u0,u1,v0,v1,1e-8);
                if(projected.NbPoints()>0)distance=std::min(distance,projected.LowerDistance());
            }
            if(!std::isfinite(distance))reject("loft_section_unresolved","Cannot verify retained section against actual output supports");
            maxSectionDistance=std::max(maxSectionDistance,distance);
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
    auto sourceProvenance=[&](const Section& section,int source){
        if(section.sketch)return QJsonObject{{"sourceSketchId",section.sketch->featureId},{"sourceEntityId",section.sourceIds[source]}};
        return QJsonObject{{"sourceBodyId",section.sourceBodyId},{"sourceProducerFeatureId",section.sourceProducerId},
            {"sourceFaceId",section.sourceFaceId},{"sourceEdgeId",section.sourceIds[source]},
            {"trimAncestry",section.edgeProvenance[source]}};
    };
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
        edgeProvenance[match]=sourceProvenance(section,source);
        edgeProvenance[match]["generatorFeatureId"]=body.featureId;edgeProvenance[match]["role"]=role;
        edgeProvenance[match]["lineage"]="assembledThruSections";
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
        edgeProvenance[match]=sourceProvenance(sections[0],0);
        edgeProvenance[match]["generatorFeatureId"]=body.featureId;edgeProvenance[match]["role"]=QString("seam.")+label;
        edgeProvenance[match]["sourcePointId"]=side==0 ? sections[0].startPointId : sections[0].endPointId;
        edgeProvenance[match]["lineage"]="assembledThruSections";
    }
    for(int edgeIndex=1;edgeIndex<=resultEdges.Extent();++edgeIndex) {
        if(edgeIds[edgeIndex].isEmpty())reject("loft_lineage_ambiguous","A surface edge lacks retained source lineage");
        body.solidEdges.push_back({edgeIds[edgeIndex],TopoDS::Edge(resultEdges(edgeIndex)),edgeProvenance[edgeIndex]});
    }
    for(int source=0;source<2;++source) {
        EvaluatedSupportFace face{stableId(body.featureId,QByteArray("face/")+sections[0].sourceIds[source].toUtf8()),
            body.bodyId,body.featureId,"loft.closedSection",TopoDS::Face(resultFaces(faceIndices[source])),{}, {}};
        face.provenance=sourceProvenance(sections[0],source);
        face.provenance["generatorFeatureId"]=body.featureId;face.provenance["orderedSections"]=references;
        face.provenance["lineage"]="assembledThruSections";
        for(TopExp_Explorer edge(face.face,TopAbs_EDGE);edge.More();edge.Next()) {
            const auto resultIndex=resultEdges.FindIndex(edge.Current());
            if(resultIndex<1 || edgeIds[resultIndex].isEmpty())
                reject("loft_lineage_ambiguous","A generated face boundary lacks retained edge identity");
            face.boundaryEdges.push_back({edgeIds[resultIndex],"curve",TopoDS::Edge(resultEdges(resultIndex)),edgeProvenance[resultIndex]});
        }
        body.supportFaces.push_back(std::move(face));
    }
    body.summary["supportFaces"]=PlanarFaces::descriptors(body.supportFaces);
    body.summary["operation"]=parameters["operation"];body.summary["guideType"]="none";
    body.summary["sections"]=references;body.summary["maxSectionDistanceMm"]=maxSectionDistance;
    body.summary["sectionContactCheck"]="33 samples per source against actual bounded output support surfaces";
    body.summary["interpolation"]=QJsonObject{{"kernel","OCCT ThruSections"},{"ruled",false},
        {"closed",false},{"compatibility",true},{"mutableInput",false}};
    return body;
}
