// SPDX-License-Identifier: MIT
#include "SurfaceRailLoft.h"
#include "PlanarFaces.h"
#include <BRepOffsetAPI_MakePipeShell.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRep_Tool.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <Geom_BSplineCurve.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <QUuid>
#include <array>
#include <algorithm>
#include <cmath>

namespace {
constexpr double contactToleranceMm=1e-6;
constexpr double boundaryToleranceMm=1e-6;
constexpr double surfaceToleranceMm=1e-4;
QString id(const QString& feature,const QByteArray& role){
    return QUuid::createUuidV5(QUuid(feature),QByteArray("alloy-rail-loft/1/")+role).toString(QUuid::WithoutBraces);
}
struct Source {const EvaluatedSketch* sketch=nullptr;QJsonObject entity;TopoDS_Edge edge;};
QJsonObject provenance(const QString& feature,const QString& sketch,const QString& entity,const QString& role){
    return {{"generatorFeatureId",feature},{"sourceSketchId",sketch},{"sourceEntityId",entity},
        {"role",role},{"lineage","railLoftBuilder"}};
}
bool contains(const TopoDS_Shape& container,const TopoDS_Shape& child){
    for(TopExp_Explorer explorer(container,child.ShapeType());explorer.More();explorer.Next())if(explorer.Current().IsSame(child))return true;
    return false;
}
double pointToEdge(const gp_Pnt& point,const TopoDS_Edge& edge){
    Standard_Real first,last;const auto curve=BRep_Tool::Curve(edge,first,last);
    if(curve.IsNull())return 1e100;
    GeomAPI_ProjectPointOnCurve projected(point,curve,first,last);
    return projected.NbPoints() ? projected.LowerDistance() : 1e100;
}
double correspondence(const TopoDS_Edge& from,const TopoDS_Edge& to){
    BRepAdaptor_Curve curve(from);double maximum=0;
    for(int sample=0;sample<=40;++sample){
        const auto parameter=curve.FirstParameter()+(curve.LastParameter()-curve.FirstParameter())*sample/40.0;
        maximum=std::max(maximum,pointToEdge(curve.Value(parameter),to));
    }
    return maximum;
}
bool matches(const TopoDS_Edge& a,const TopoDS_Edge& b){
    return correspondence(a,b)<boundaryToleranceMm && correspondence(b,a)<boundaryToleranceMm;
}
std::pair<gp_Pnt,gp_Pnt> ends(const TopoDS_Edge& edge){
    TopoDS_Vertex first,last;TopExp::Vertices(edge,first,last,true);
    return {BRep_Tool::Pnt(first),BRep_Tool::Pnt(last)};
}
std::pair<gp_Pnt,gp_Pnt> parameterEnds(const TopoDS_Edge& edge){
    BRepAdaptor_Curve curve(edge);return {curve.Value(curve.FirstParameter()),curve.Value(curve.LastParameter())};
}
TopoDS_Edge uniqueGeneratedEdge(BRepOffsetAPI_MakePipeShell& loft,const TopoDS_Vertex& vertex,
    const TopTools_IndexedMapOfShape& current){
    TopTools_IndexedMapOfShape found;
    for(const auto& shape:loft.Generated(vertex))if(shape.ShapeType()==TopAbs_EDGE && current.Contains(shape))found.Add(shape);
    return found.Extent()==1 ? TopoDS::Edge(current(current.FindIndex(found(1)))) : TopoDS_Edge{};
}
struct Boundary {QString edgeId;TopoDS_Edge edge;QJsonObject provenance;};
}

EvaluatedBody SurfaceRailLoft::evaluate(const QJsonObject& feature,const EvaluatedDocument& prior){
    const auto parameters=feature["parameters"].toObject();
    const auto sections=parameters["sections"].toArray();
    const auto featureId=feature["id"].toString();
    auto reject=[&](const QString& code,const QString& message){throw GeometryKernel::EvaluationError({
        {"featureId",featureId},{"featureType","loft"},{"code",code},{"message",message}});};
    auto source=[&](const QString& sketchId,const QString& entityId,const QString& type)->Source{
        for(const auto& sketch:prior.sketches)if(sketch.featureId==sketchId){
            if(!sketch.entityEdges.contains(entityId))break;
            for(const auto& row:sketch.summary["entities"].toArray()){
                const auto entity=row.toObject();
                if(entity["id"]==entityId && entity["type"]==type && !entity["construction"].toBool())
                    return {&sketch,entity,TopoDS::Edge(BRepBuilderAPI_Copy(sketch.entityEdges.value(entityId),false,false).Shape())};
            }
            break;
        }
        reject("loft_source_unavailable","Select exact earlier ordinary arc, two lines and fit-spline rail");return {};
    };
    const auto first=sections[0].toObject(),last=sections[1].toObject(),railRef=parameters["rail"].toObject();
    const auto firstIds=first["entityIds"].toArray(),lastIds=last["entityIds"].toArray();
    auto arc=source(first["sketchId"].toString(),firstIds[0].toString(),"arc");
    auto line0=source(last["sketchId"].toString(),lastIds[0].toString(),"line");
    auto line1=source(last["sketchId"].toString(),lastIds[1].toString(),"line");
    auto rail=source(railRef["sketchId"].toString(),railRef["entityId"].toString(),"fitSpline");
    BRepAdaptor_Curve railCurve(rail.edge),arcCurve(arc.edge),leftCurve(line0.edge),rightCurve(line1.edge);
    if(railCurve.GetType()!=GeomAbs_BSplineCurve || arcCurve.GetType()!=GeomAbs_Circle
        || leftCurve.GetType()!=GeomAbs_Line || rightCurve.GetType()!=GeomAbs_Line
        || arcCurve.IsClosed() || leftCurve.IsClosed() || rightCurve.IsClosed())
        reject("unsupported_rail_loft_source","Rail Loft supports one open arc, an ordered two-line chain and one open fit spline");
    const auto spline=railCurve.BSpline();
    if(spline.IsNull() || !spline->IsCN(1) || railCurve.IsClosed())
        reject("rail_not_tangent_continuous","The selected rail must be open and tangent-continuous");
    const auto [railStart,railEnd]=parameterEnds(rail.edge);
    const auto railPoints=rail.entity["pointIds"].toArray();
    auto solvedPoint=[&](const QString& pointId)->gp_Pnt{
        for(const auto& value:rail.sketch->summary["worldPoints"].toArray()){
            const auto row=value.toObject();const auto xyz=row["positionMm"].toArray();
            if(row["id"]==pointId && xyz.size()==3)
                return gp_Pnt(xyz[0].toDouble(),xyz[1].toDouble(),xyz[2].toDouble());
        }
        reject("rail_contact_unavailable","Rail fit-point identity no longer resolves in the solved sketch");return {};
    };
    if(railPoints.size()<2 || railStart.Distance(solvedPoint(railPoints.first().toString()))>=contactToleranceMm
        || railEnd.Distance(solvedPoint(railPoints.last().toString()))>=contactToleranceMm)
        reject("rail_contact_unavailable","Rail evaluation must follow retained fit-point endpoint order");
    if(railStart.Distance(railEnd)<contactToleranceMm)
        reject("rail_contact_unavailable","Rail endpoints must be distinct");
    const auto [arcA,arcB]=parameterEnds(arc.edge);
    const auto startA=railStart.Distance(arcA)<contactToleranceMm,startB=railStart.Distance(arcB)<contactToleranceMm;
    if(startA==startB)reject("rail_contact_unavailable","Rail start must meet one unique back-arc endpoint");
    if(startB)arc.edge.Reverse();
    const auto [lineA,lineB]=parameterEnds(line0.edge);
    const auto endA=railEnd.Distance(lineA)<contactToleranceMm,endB=railEnd.Distance(lineB)<contactToleranceMm;
    if(endA==endB)reject("rail_contact_unavailable","Rail end must meet the first lower-line free endpoint");
    if(endB)line0.edge.Reverse();
    const auto [leftA,leftB]=ends(line0.edge);
    const auto [rightA,rightB]=ends(line1.edge);
    const auto sharedA=leftB.Distance(rightA)<contactToleranceMm,sharedB=leftB.Distance(rightB)<contactToleranceMm;
    if(sharedA==sharedB || leftA.Distance(rightA)<contactToleranceMm || leftA.Distance(rightB)<contactToleranceMm)
        reject("loft_chain_disconnected","The two lower lines must form one ordered open chain from the rail contact");
    if(sharedB)line1.edge.Reverse();
    if(ends(line1.edge).second.Distance(railStart)<contactToleranceMm)
        reject("loft_chain_closed","The lower section must remain an open chain");
    BRepBuilderAPI_MakeWire spine(rail.edge),back(arc.edge),lower(line0.edge,line1.edge);
    if(!spine.IsDone() || !back.IsDone() || !lower.IsDone())
        reject("loft_wire_failed","OCCT could not build retained rail and section wires");
    TopoDS_Vertex arcContact,arcOpposite;TopExp::Vertices(arc.edge,arcContact,arcOpposite,true);
    if(BRep_Tool::Pnt(arcOpposite).Distance(railStart)<contactToleranceMm)std::swap(arcContact,arcOpposite);
    if(BRep_Tool::Pnt(arcContact).Distance(railStart)>=contactToleranceMm)
        reject("rail_contact_unavailable","The selected arc endpoint is unavailable in the evaluated edge");
    BRepOffsetAPI_MakePipeShell loft(spine.Wire());loft.SetMode(false);loft.SetTolerance(1e-5,1e-5,1e-4);
    loft.Add(back.Wire(),BRepBuilderAPI_MakeVertex(railStart).Vertex(),false,false);
    loft.Add(lower.Wire(),BRepBuilderAPI_MakeVertex(railEnd).Vertex(),false,false);
    loft.Build();
    if(!loft.IsDone() || !std::isfinite(loft.ErrorOnSurface()) || loft.ErrorOnSurface()>surfaceToleranceMm)
        reject("loft_failed","The intersecting rail could not build a bounded surface");
    EvaluatedBody body;body.featureId=featureId;body.bodyId=feature["bodyId"].toString();
    body.name=feature["name"].toString();body.visible=feature["visible"].toBool();body.shape=loft.Shape();
    body.summary=GeometryKernel::summarize(body.shape);
    const auto topology=body.summary["topology"].toObject();
    if(!body.summary["valid"].toBool() || topology["faces"].toInt()!=2
        || topology["edges"].toInt()!=7 || topology["solids"].toInt()!=0)
        reject("loft_invalid","Rail Loft must produce two valid open faces, seven edges and no solid");
    TopTools_IndexedMapOfShape currentFaces,currentEdges,startEdges,endEdges;
    TopExp::MapShapes(body.shape,TopAbs_FACE,currentFaces);TopExp::MapShapes(body.shape,TopAbs_EDGE,currentEdges);
    if(loft.FirstShape().ShapeType()!=TopAbs_WIRE || loft.LastShape().ShapeType()!=TopAbs_WIRE)
        reject("loft_lineage_unavailable","Loft section wire provenance is unavailable");
    TopExp::MapShapes(loft.FirstShape(),TopAbs_EDGE,startEdges);
    TopExp::MapShapes(loft.LastShape(),TopAbs_EDGE,endEdges);
    if(startEdges.Extent()!=2 || endEdges.Extent()!=2)
        reject("loft_lineage_unavailable","Loft must retain both section boundary spans");
    const auto railEdge=uniqueGeneratedEdge(loft,arcContact,currentEdges);
    const auto oppositeEdge=uniqueGeneratedEdge(loft,arcOpposite,currentEdges);
    if(railEdge.IsNull() || oppositeEdge.IsNull() || railEdge.IsSame(oppositeEdge)
        || !matches(rail.edge,railEdge))
        reject("loft_rail_lineage_unavailable","The selected rail needs one current free boundary with sampled curve correspondence");
    std::array<TopoDS_Edge,2> lowerEdges;
    const std::array<Source,2> sources{line0,line1};
    for(int i=0;i<2;++i){
        TopTools_IndexedMapOfShape matched;
        for(int j=1;j<=endEdges.Extent();++j){const auto edge=TopoDS::Edge(endEdges(j));
            if(currentEdges.Contains(edge) && matches(sources[i].edge,edge))matched.Add(edge);}
        if(matched.Extent()!=1)reject("loft_section_lineage_unavailable","Each lower source line must retain one unique current boundary");
        lowerEdges[i]=TopoDS::Edge(currentEdges(currentEdges.FindIndex(matched(1))));
    }
    if(lowerEdges[0].IsSame(lowerEdges[1]))
        reject("loft_section_lineage_unavailable","Lower source lines merged ambiguously");
    std::array<TopoDS_Face,2> faces;
    for(int i=0;i<2;++i){
        TopTools_IndexedMapOfShape matched;
        for(const auto& generated:loft.Generated(arc.edge))if(generated.ShapeType()==TopAbs_FACE
            && currentFaces.Contains(generated) && contains(generated,lowerEdges[i]))matched.Add(generated);
        if(matched.Extent()!=1)reject("loft_face_lineage_unavailable","Each lower source span must identify one generated face");
        faces[i]=TopoDS::Face(currentFaces(currentFaces.FindIndex(matched(1))));
    }
    if(faces[0].IsSame(faces[1]))reject("loft_face_lineage_unavailable","Source spans mapped to the same face");
    std::array<TopoDS_Edge,2> backEdges;
    for(int i=0;i<2;++i){
        TopTools_IndexedMapOfShape matched;
        for(int j=1;j<=startEdges.Extent();++j)if(currentEdges.Contains(startEdges(j)) && contains(faces[i],startEdges(j)))
            matched.Add(startEdges(j));
        if(matched.Extent()!=1)reject("loft_section_lineage_unavailable","Back arc span has no unique current face boundary");
        backEdges[i]=TopoDS::Edge(currentEdges(currentEdges.FindIndex(matched(1))));
        if(correspondence(backEdges[i],arc.edge)>boundaryToleranceMm)
            reject("loft_section_lineage_unavailable","A generated back span departed from the selected arc");
    }
    if(backEdges[0].IsSame(backEdges[1]))reject("loft_section_lineage_unavailable","Back arc spans merged ambiguously");
    TopoDS_Edge middle;
    for(int i=1;i<=currentEdges.Extent();++i){const auto edge=TopoDS::Edge(currentEdges(i));
        if(edge.IsSame(railEdge)||edge.IsSame(oppositeEdge)||edge.IsSame(lowerEdges[0])||edge.IsSame(lowerEdges[1])
            ||edge.IsSame(backEdges[0])||edge.IsSame(backEdges[1]))continue;
        if(!middle.IsNull())reject("loft_lineage_unavailable","Multiple unnamed interior boundaries");middle=edge;}
    if(middle.IsNull())reject("loft_lineage_unavailable","Interior boundary unavailable");
    const auto railId=railRef["entityId"].toString(),arcId=firstIds[0].toString();
    std::vector<Boundary> boundaries;
    boundaries.push_back({id(featureId,"edge/rail"),railEdge,provenance(featureId,railRef["sketchId"].toString(),railId,"rail")});
    boundaries.push_back({id(featureId,"edge/opposite"),oppositeEdge,provenance(featureId,first["sketchId"].toString(),arcId,"opposite")});
    boundaries.push_back({id(featureId,"edge/middle"),middle,provenance(featureId,last["sketchId"].toString(),lastIds[0].toString(),"middle")});
    for(int i=0;i<2;++i){
        const auto span=lastIds[i].toString().toUtf8();
        boundaries.push_back({id(featureId,"edge/back/"+span),backEdges[i],provenance(featureId,first["sketchId"].toString(),arcId,"back")});
        boundaries.push_back({id(featureId,"edge/lower/"+span),lowerEdges[i],provenance(featureId,last["sketchId"].toString(),lastIds[i].toString(),"lower")});
    }
    int railFaces=0,middleFaces=0;
    for(int i=0;i<2;++i){
        EvaluatedSupportFace support{id(featureId,"face/"+lastIds[i].toString().toUtf8()),body.bodyId,featureId,
            "loft.railSurface",faces[i],{}, {}};
        for(const auto& boundary:boundaries)if(contains(faces[i],boundary.edge)){
            BRepAdaptor_Curve curve(boundary.edge);
            support.boundaryEdges.push_back({boundary.edgeId,curve.GetType()==GeomAbs_Line ? "line" : "curve",
                boundary.edge,boundary.provenance});
            if(boundary.edge.IsSame(railEdge))++railFaces;
            if(boundary.edge.IsSame(middle))++middleFaces;
        }
        if(support.boundaryEdges.size()!=4 || !contains(faces[i],lowerEdges[i]) || !contains(faces[i],backEdges[i]))
            reject("loft_lineage_unavailable","Each generated face needs four uniquely named boundaries");
        body.supportFaces.push_back(support);
    }
    if(railFaces!=1 || middleFaces!=2)
        reject("loft_rail_lineage_unavailable","Selected rail must be a free boundary and middle edge must join both faces");
    for(const auto& boundary:boundaries)body.solidEdges.push_back({boundary.edgeId,boundary.edge,boundary.provenance});
    body.summary["supportFaces"]=PlanarFaces::descriptors(body.supportFaces);
    body.summary["operation"]="newComponent";body.summary["guideType"]="rail";
    body.summary["sections"]=sections;body.summary["rail"]=railRef;
    body.summary["interpolation"]=QJsonObject{{"kernel","OCCT MakePipeShell"},{"mode","correctedFrenet"},
        {"contact","source endpoints on rail"},{"outputSurfaceErrorMm",loft.ErrorOnSurface()},
        {"sampledBoundaryCorrespondenceToleranceMm",boundaryToleranceMm}};
    return body;
}
