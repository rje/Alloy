// SPDX-License-Identifier: MIT
#include "SolidLoft.h"
#include "PlanarFaces.h"
#include <BRepOffsetAPI_ThruSections.hxx>
#include <BRepOffsetAPI_MakePipeShell.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepTools.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <BRep_Tool.hxx>
#include <QUuid>
#include <QMap>
#include <array>
#include <cmath>

namespace {
QString stableId(const QString& feature,const QByteArray& role) {
    return QUuid::createUuidV5(QUuid(feature),QByteArray("alloy-loft-surface/1/")+role).toString(QUuid::WithoutBraces);
}
EvaluatedBody surfaceLoft(const QJsonObject& feature,const EvaluatedDocument& prior) {
    const auto p=feature["parameters"].toObject();
    auto reject=[&](const QString& code,const QString& message){throw GeometryKernel::EvaluationError({
        {"featureId",feature["id"]},{"featureType","loft"},{"code",code},{"message",message}});};
    auto source=[&](const QJsonObject& ref,const QString& type)->std::pair<const EvaluatedSketch*,QJsonObject> {
        for(const auto& sketch:prior.sketches)if(sketch.featureId==ref["sketchId"].toString()) {
            if(!sketch.entityEdges.contains(ref["entityId"].toString()))break;
            for(const auto& row:sketch.summary["entities"].toArray())if(row.toObject()["id"]==ref["entityId"] && row.toObject()["type"]==type
                && !row.toObject()["construction"].toBool())
                return {&sketch,row.toObject()};
            break;
        }
        reject("loft_source_unavailable","Select the exact earlier arc, line and fit-spline entities");
        return {nullptr,{}};
    };
    const auto guide=p["centerline"].toObject();
    const auto [pathSketch,pathEntity]=source(guide,"fitSpline");
    auto pathEdge=TopoDS::Edge(BRepBuilderAPI_Copy(pathSketch->entityEdges.value(guide["entityId"].toString()),false,false).Shape());
    BRepAdaptor_Curve pathCurve(pathEdge);
    if(pathCurve.GetType()!=GeomAbs_BSplineCurve)reject("unsupported_centerline","Surface Loft centerline must be one fit-point spline");
    const auto pathPoints=pathEntity["pointIds"].toArray();
    QMap<QString,gp_Pnt> world;
    for(const auto& row:pathSketch->summary["worldPoints"].toArray()) {
        const auto point=row.toObject();const auto xyz=point["positionMm"].toArray();
        if(xyz.size()==3)world.insert(point["id"].toString(),gp_Pnt(xyz[0].toDouble(),xyz[1].toDouble(),xyz[2].toDouble()));
    }
    if(pathPoints.size()<2 || !world.contains(pathPoints.first().toString()) || !world.contains(pathPoints.last().toString()))
        reject("loft_centerline_unavailable","Retained centerline endpoint positions are unavailable");
    gp_Pnt first,last;gp_Vec firstTangent,lastTangent;
    pathCurve.D1(pathCurve.FirstParameter(),first,firstTangent);
    pathCurve.D1(pathCurve.LastParameter(),last,lastTangent);
    const auto start=world.value(pathPoints.first().toString()),end=world.value(pathPoints.last().toString());
    if(start.Distance(end)<1e-7)reject("loft_centerline_unavailable","Surface Loft requires distinct retained centerline endpoints");
    if(first.Distance(end)<1e-7 && last.Distance(start)<1e-7) {
        pathEdge.Reverse();std::swap(first,last);std::swap(firstTangent,lastTangent);
        firstTangent.Reverse();lastTangent.Reverse();
    }
    if(first.Distance(start)>1e-7 || last.Distance(end)>1e-7 || firstTangent.Magnitude()<1e-8 || lastTangent.Magnitude()<1e-8)
        reject("loft_centerline_unavailable","Centerline endpoints do not uniquely match retained fit-point order");
    firstTangent.Normalize();lastTangent.Normalize();
    const auto refs=p["sections"].toArray();
    const auto [frontSketch,arcEntity]=source(refs[0].toObject(),"arc");
    const auto [rearSketch,lineEntity]=source(refs[1].toObject(),"line");
    auto arcEdge=TopoDS::Edge(BRepBuilderAPI_Copy(frontSketch->entityEdges.value(arcEntity["id"].toString()),false,false).Shape());
    auto lineEdge=TopoDS::Edge(BRepBuilderAPI_Copy(rearSketch->entityEdges.value(lineEntity["id"].toString()),false,false).Shape());
    BRepAdaptor_Curve arc(arcEdge),line(lineEdge);
    if(arc.GetType()!=GeomAbs_Circle || line.GetType()!=GeomAbs_Line || arc.IsClosed() || line.IsClosed())
        reject("unsupported_loft_section","Surface Loft requires one open circular arc followed by one open line");
    const auto arcMid=arc.Value((arc.FirstParameter()+arc.LastParameter())*.5),
        lineMid=line.Value((line.FirstParameter()+line.LastParameter())*.5);
    if(arcMid.Distance(start)>1e-6 || lineMid.Distance(end)>1e-6)
        reject("loft_section_misaligned","Each open section midpoint must follow its retained centerline endpoint");
    auto aligned=[&](const EvaluatedSketch& sketch,const gp_Vec& tangent) {
        const auto n=sketch.resolvedFrame["normal"].toArray();
        return n.size()==3 && std::abs(gp_Vec(n[0].toDouble(),n[1].toDouble(),n[2].toDouble()).Dot(tangent))>1-1e-8;
    };
    if(!aligned(*frontSketch,firstTangent) || !aligned(*rearSketch,lastTangent))
        reject("loft_section_misaligned","Section planes must stay normal to the retained centerline endpoints");
    BRepBuilderAPI_MakeWire spine(pathEdge),front(arcEdge),rear(lineEdge);
    if(!spine.IsDone() || !front.IsDone() || !rear.IsDone())reject("loft_wire_failed","OCCT could not make exact source wires");
    BRepOffsetAPI_MakePipeShell loft(spine.Wire());loft.SetMode(false);
    loft.Add(front.Wire(),BRepBuilderAPI_MakeVertex(start).Vertex(),false,false);
    loft.Add(rear.Wire(),BRepBuilderAPI_MakeVertex(end).Vertex(),false,false);
    loft.Build();
    if(!loft.IsDone())reject("loft_failed","OCCT guided surface Loft did not complete");
    EvaluatedBody body;body.featureId=feature["id"].toString();body.bodyId=feature["bodyId"].toString();
    body.name=feature["name"].toString();body.visible=feature["visible"].toBool();body.shape=loft.Shape();
    body.summary=GeometryKernel::summarize(body.shape);
    const auto topology=body.summary["topology"].toObject();
    if(!body.summary["valid"].toBool() || topology["faces"]!=1 || topology["solids"]!=0 || topology["edges"]!=4)
        reject("loft_invalid","Surface Loft must produce one valid open face with four boundary edges");
    TopTools_IndexedMapOfShape faceMap,edgeMap;
    TopExp::MapShapes(body.shape,TopAbs_FACE,faceMap);TopExp::MapShapes(body.shape,TopAbs_EDGE,edgeMap);
    const auto exactEdge=[&](const TopoDS_Shape& source)->TopoDS_Edge {
        TopTools_IndexedMapOfShape found;
        if(source.IsNull())return {};
        if(source.ShapeType()==TopAbs_WIRE) {
            for(TopExp_Explorer e(source,TopAbs_EDGE);e.More();e.Next())if(edgeMap.Contains(e.Current()))found.Add(e.Current());
        } else if(source.ShapeType()==TopAbs_EDGE && edgeMap.Contains(source))found.Add(source);
        return found.Extent()==1 ? TopoDS::Edge(edgeMap(edgeMap.FindIndex(found(1)))) : TopoDS_Edge{};
    };
    const auto startEdge=exactEdge(loft.FirstShape()),endEdge=exactEdge(loft.LastShape());
    TopoDS_Vertex arcA,arcB;TopExp::Vertices(arcEdge,arcA,arcB,true);
    const auto side=[&](const TopoDS_Vertex& vertex)->TopoDS_Edge {
        TopTools_IndexedMapOfShape found;
        for(const auto& result:loft.Generated(vertex))if(result.ShapeType()==TopAbs_EDGE && edgeMap.Contains(result))found.Add(result);
        return found.Extent()==1 ? TopoDS::Edge(edgeMap(edgeMap.FindIndex(found(1)))) : TopoDS_Edge{};
    };
    const auto sideA=side(arcA),sideB=side(arcB);
    if(startEdge.IsNull() || endEdge.IsNull() || sideA.IsNull() || sideB.IsNull()
        || startEdge.IsSame(endEdge) || startEdge.IsSame(sideA) || startEdge.IsSame(sideB)
        || endEdge.IsSame(sideA) || endEdge.IsSame(sideB) || sideA.IsSame(sideB))
        reject("loft_lineage_unavailable","The four current surface boundaries have no unique builder lineage");
    EvaluatedSupportFace face{stableId(body.featureId,"face"),body.bodyId,body.featureId,"loft.surface",
        TopoDS::Face(faceMap(1)),{}, {}};
    const std::array<std::pair<QByteArray,TopoDS_Edge>,4> boundaries{{{"start",startEdge},{"end",endEdge},{"sideA",sideA},{"sideB",sideB}}};
    for(const auto& [role,edge]:boundaries) {
        BRepAdaptor_Curve curve(edge);
        QJsonObject provenance{{"generatorFeatureId",body.featureId},{"sourceSketchId",role=="end" ? rearSketch->featureId : frontSketch->featureId},
            {"sourceEntityId",role=="end" ? lineEntity["id"] : arcEntity["id"]},{"role",QString::fromLatin1(role)},
            {"lineage","guidedLoftBuilder"}};
        const auto a=curve.Value(curve.FirstParameter()),b=curve.Value(curve.LastParameter());
        provenance["startMm"]=QJsonArray{a.X(),a.Y(),a.Z()};provenance["endMm"]=QJsonArray{b.X(),b.Y(),b.Z()};
        const auto id=stableId(body.featureId,QByteArray("edge/")+role);
        face.boundaryEdges.push_back({id,curve.GetType()==GeomAbs_Line ? "line" : "curve",edge,provenance});
        body.solidEdges.push_back({id,edge,provenance});
    }
    body.supportFaces.push_back(face);
    body.summary["supportFaces"]=PlanarFaces::descriptors(body.supportFaces);
    body.summary["operation"]="newComponent";body.summary["sections"]=refs;body.summary["centerline"]=guide;
    body.summary["interpolation"]=QJsonObject{{"kernel","OCCT MakePipeShell"},{"mode","correctedFrenet"},{"withContact",false},{"withCorrection",false}};
    return body;
}
}

EvaluatedBody SolidLoft::evaluate(const QJsonObject& feature,const EvaluatedDocument& prior) {
    if(feature["parameters"].toObject().value("operation")=="newComponent")return surfaceLoft(feature,prior);
    auto reject=[](const QString& code,const QString& message){throw GeometryKernel::EvaluationError({{"code",code},{"message",message}});};
    const auto references=feature["parameters"].toObject()["profiles"].toArray();
    if(references.size()<2 || references.size()>32)reject("loft_sections_required","Loft requires 2–32 ordered circular profiles");
    std::vector<EvaluatedProfile> profiles;gp_Pnt firstCenter;gp_Dir firstNormal;double lastDistance=0;int direction=0;
    BRepOffsetAPI_ThruSections loft(true,false,1e-7);loft.SetMutableInput(false);loft.CheckCompatibility(true);
    loft.SetSmoothing(false);loft.SetContinuity(GeomAbs_C2);loft.SetMaxDegree(8);
    for(const auto& value:references) {
        const auto ref=value.toObject();const EvaluatedProfile* profile=nullptr;
        for(const auto& sketch:prior.sketches)if(sketch.featureId==ref["sketchId"].toString())for(const auto& candidate:sketch.profiles)if(candidate.profileId==ref["profileId"].toString())profile=&candidate;
        if(!profile)reject("loft_profile_unavailable","Each Loft profile must resolve to an earlier sketch region");
        if(profile->summary["type"]!="closed_circle_profile" || profile->boundaryEdges.size()!=1)reject("unsupported_loft_profile","This Loft supports analytic circular sections only");
        BRepAdaptor_Curve curve(profile->boundaryEdges.front().edge);
        if(curve.GetType()!=GeomAbs_Circle)reject("unsupported_loft_profile","Each Loft section must be an analytic circle");
        const auto circle=curve.Circle();const auto center=circle.Location();const auto normal=circle.Axis().Direction();
        if(profiles.empty()){firstCenter=center;firstNormal=normal;}
        else {
            const gp_Vec delta(firstCenter,center);const auto distance=delta.Dot(gp_Vec(firstNormal));
            if(std::abs(normal.Dot(firstNormal))<1-1e-9 || (delta-gp_Vec(firstNormal)*distance).Magnitude()>1e-6)reject("unsupported_loft_alignment","Circular Loft sections must be parallel and coaxial");
            if(direction==0)direction=distance>0 ? 1 : -1;
            if(direction*(distance-lastDistance)<0.001)reject("invalid_loft_order","Choose distinct sections in monotonic order along their shared axis");
            lastDistance=distance;
        }
        profiles.push_back(*profile);loft.AddWire(BRepTools::OuterWire(profile->face));
    }
    loft.Build();if(!loft.IsDone())reject("loft_failed","OCCT smooth Loft did not complete");
    EvaluatedBody body;body.featureId=feature["id"].toString();body.bodyId=feature["bodyId"].toString();body.name=feature["name"].toString();body.visible=feature["visible"].toBool();body.shape=loft.Shape();
    body.summary=GeometryKernel::summarize(body.shape);
    if(!body.summary["valid"].toBool() || !body.summary["closed"].toBool() || body.summary["topology"].toObject()["solids"]!=1)reject("loft_invalid","Loft must produce one valid closed solid");
    for(int end=0;end<2;++end) {
        const auto shape=end ? loft.LastShape() : loft.FirstShape();
        if(shape.IsNull() || shape.ShapeType()!=TopAbs_FACE || BRepAdaptor_Surface(TopoDS::Face(shape)).GetType()!=GeomAbs_Plane)reject("loft_cap_unavailable","Loft endpoint cap provenance is unavailable");
        auto frame=(end ? profiles.back() : profiles.front()).summary["frame"].toObject();
        auto n=frame["normal"].toArray(),v=frame["vAxis"].toArray();
        const auto source=n[0].toDouble()*firstNormal.X()+n[1].toDouble()*firstNormal.Y()+n[2].toDouble()*firstNormal.Z();
        if(source*(end ? direction : -direction)<0){for(int i=0;i<3;++i){n[i]=-n[i].toDouble();v[i]=-v[i].toDouble();}frame["normal"]=n;frame["vAxis"]=v;}
        const auto role=end ? "loft.end" : "loft.start";
        const auto faceId=QUuid::createUuidV5(QUuid(body.featureId),QByteArray("alloy-loft-cap/1/")+role).toString(QUuid::WithoutBraces);
        body.supportFaces.push_back({faceId,body.bodyId,body.featureId,role,TopoDS::Face(shape),frame,{}});
    }
    body.summary["supportFaces"]=PlanarFaces::descriptors(body.supportFaces);body.summary["operation"]="loft";body.summary["profiles"]=references;
    body.summary["interpolation"]=QJsonObject{{"kernel","OCCT ThruSections"},{"ruled",false},{"continuity","C2"},{"maxDegree",8},{"smoothingOptimization",false},{"endpointCondition","Free"}};
    return body;
}
