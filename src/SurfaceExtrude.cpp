// SPDX-License-Identifier: MIT
#include "SurfaceExtrude.h"
#include "PlanarFaces.h"
#include "SolidFillet.h"
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRep_Builder.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <QUuid>
#include <QSet>
#include <array>
#include <cmath>

namespace {
QString stableId(const QString& feature,const QString& entity,const QByteArray& role) {
    return QUuid::createUuidV5(QUuid(feature),QByteArray("alloy-surface-extrude/1/")+entity.toUtf8()+"/"+role)
        .toString(QUuid::WithoutBraces);
}
}

EvaluatedBody SurfaceExtrude::evaluate(const QJsonObject& feature,const EvaluatedDocument& prior) {
    const auto p=feature["parameters"].toObject();
    const auto featureId=feature["id"].toString(),bodyId=feature["bodyId"].toString();
    auto reject=[&](const QString& code,const QString& message) {
        throw GeometryKernel::EvaluationError({{"featureId",featureId},{"featureType","surfaceExtrude"},
            {"code",code},{"message",message}});
    };
    const auto sketchId=p["sketchId"].toString();
    const EvaluatedSketch* sketch=nullptr;
    for(const auto& candidate:prior.sketches)if(candidate.featureId==sketchId){sketch=&candidate;break;}
    if(!sketch)reject("surface_source_unavailable","Select an earlier solved sketch");
    const auto frame=sketch->resolvedFrame;const auto components=frame["normal"].toArray();
    if(components.size()!=3)reject("surface_source_unavailable","Source sketch has no resolved normal");
    gp_Vec normal(components[0].toDouble(),components[1].toDouble(),components[2].toDouble());
    if(normal.Magnitude()<1e-8)reject("surface_source_unavailable","Source sketch normal is degenerate");
    normal.Normalize();
    const double sideOne=p["sideOneMm"].toDouble(),sideTwo=p["sideTwoMm"].toDouble();
    TopoDS_Compound compound;BRep_Builder builder;builder.MakeCompound(compound);
    EvaluatedBody body;body.featureId=featureId;body.bodyId=bodyId;body.name=feature["name"].toString();
    body.visible=feature["visible"].toBool(true);
    QSet<QString> chosen;
    for(const auto& value:p["entityIds"].toArray()) {
        const auto entityId=value.toString();
        if(chosen.contains(entityId))reject("surface_source_duplicate","Select each open source curve once");
        chosen.insert(entityId);
        QJsonObject entity;
        for(const auto& row:sketch->summary["entities"].toArray())if(row.toObject()["id"]==entityId){entity=row.toObject();break;}
        const auto type=entity["type"].toString();
        if((type!="line" && type!="fitSpline") || entity["construction"].toBool()
            || !sketch->entityEdges.contains(entityId))
            reject("unsupported_surface_source","Select ordinary open line or fit-spline sketch curves");
        auto edge=TopoDS::Edge(BRepBuilderAPI_Copy(sketch->entityEdges.value(entityId),false,false).Shape());
        BRepAdaptor_Curve curve(edge);
        if(curve.IsClosed() || (type=="line" && curve.GetType()!=GeomAbs_Line)
            || (type=="fitSpline" && curve.GetType()!=GeomAbs_BSplineCurve))
            reject("unsupported_surface_source","Source curve must remain open and match its retained sketch type");
        gp_Trsf shift;shift.SetTranslation(-normal*sideOne);
        BRepBuilderAPI_Transform translated(edge,shift,true);
        if(!translated.IsDone())reject("surface_prism_failed","OCCT could not position the source curve");
        const auto start=TopoDS::Edge(translated.Shape());
        BRepPrimAPI_MakePrism prism(start,normal*(sideOne+sideTwo),false,true);
        prism.Build();
        if(!prism.IsDone())reject("surface_prism_failed","OCCT could not make a sheet from the source curve");
        TopTools_IndexedMapOfShape faces,edges;
        TopExp::MapShapes(prism.Shape(),TopAbs_FACE,faces);TopExp::MapShapes(prism.Shape(),TopAbs_EDGE,edges);
        if(faces.Extent()!=1 || edges.Extent()!=4 || !BRepCheck_Analyzer(prism.Shape()).IsValid())
            reject("surface_prism_invalid","Each selected open curve must produce one valid four-edge sheet");
        auto exactEdge=[&](const TopoDS_Shape& shape)->TopoDS_Edge {
            if(shape.IsNull() || shape.ShapeType()!=TopAbs_EDGE || !edges.Contains(shape))return {};
            return TopoDS::Edge(edges(edges.FindIndex(shape)));
        };
        const auto side1=exactEdge(prism.FirstShape()),side2=exactEdge(prism.LastShape());
        TopoDS_Vertex vertexA,vertexB;TopExp::Vertices(start,vertexA,vertexB,true);
        auto endEdge=[&](const TopoDS_Vertex& vertex)->TopoDS_Edge {
            TopTools_IndexedMapOfShape found;
            for(const auto& generated:prism.Generated(vertex))if(generated.ShapeType()==TopAbs_EDGE && edges.Contains(generated))found.Add(generated);
            return found.Extent()==1 ? TopoDS::Edge(edges(edges.FindIndex(found(1)))) : TopoDS_Edge{};
        };
        const auto endA=endEdge(vertexA),endB=endEdge(vertexB);
        if(side1.IsNull() || side2.IsNull() || endA.IsNull() || endB.IsNull()
            || side1.IsSame(side2) || side1.IsSame(endA) || side1.IsSame(endB)
            || side2.IsSame(endA) || side2.IsSame(endB) || endA.IsSame(endB))
            reject("surface_lineage_unavailable","OCCT could not identify four unique current sheet boundaries");
        auto face=TopoDS::Face(faces(1));builder.Add(compound,face);
        EvaluatedSupportFace support{stableId(featureId,entityId,"face"),bodyId,featureId,
            "surfaceExtrude.sheet",face,{}, {}};
        const BRepAdaptor_Surface surface(face);
        if(surface.GetType()==GeomAbs_Plane){
            const auto plane=surface.Plane();const auto u=plane.Position().XDirection();
            auto v=plane.Position().YDirection();auto n=u.Crossed(v);
            if(face.Orientation()==TopAbs_REVERSED){v.Reverse();n.Reverse();}
            auto point=[](const gp_Pnt& p){return QJsonArray{p.X(),p.Y(),p.Z()};};
            auto axis=[](const gp_Dir& d){return QJsonArray{d.X(),d.Y(),d.Z()};};
            support.frame={{"origin",point(plane.Location())},{"uAxis",axis(u)},
                {"vAxis",axis(v)},{"normal",axis(n)}};
        }
        const std::array<std::pair<QByteArray,TopoDS_Edge>,4> boundary{{{"side1",side1},{"side2",side2},
            {"endA",endA},{"endB",endB}}};
        for(const auto& [role,current]:boundary) {
            const auto id=stableId(featureId,entityId,QByteArray("edge/")+role);
            QJsonObject provenance{{"generatorFeatureId",featureId},{"sourceSketchId",sketchId},
                {"sourceEntityId",entityId},{"role",QString::fromLatin1(role)},
                {"lineage","surfacePrismBuilder"}};
            BRepAdaptor_Curve boundaryCurve(current);
            const auto a=boundaryCurve.Value(boundaryCurve.FirstParameter()),b=boundaryCurve.Value(boundaryCurve.LastParameter());
            provenance["startMm"]=QJsonArray{a.X(),a.Y(),a.Z()};
            provenance["endMm"]=QJsonArray{b.X(),b.Y(),b.Z()};
            support.boundaryEdges.push_back({id,boundaryCurve.GetType()==GeomAbs_Line ? "line" : "curve",current,provenance});
            body.solidEdges.push_back({id,current,provenance});
        }
        body.supportFaces.push_back(std::move(support));
    }
    body.shape=compound;body.summary=GeometryKernel::summarize(body.shape);
    const auto topology=body.summary["topology"].toObject();
    if(!body.summary["valid"].toBool() || topology["faces"].toInt()!=chosen.size()
        || topology["solids"].toInt()!=0 || body.summary["volumeMm3"].isDouble()
        || body.summary["areaMm2"].toDouble()<=0)
        reject("surface_invalid","Surface Extrude must produce current valid zero-volume sheets");
    body.summary["supportFaces"]=PlanarFaces::descriptors(body.supportFaces);
    body.summary["solidEdges"]=SolidFillet::descriptors(body.solidEdges);
    body.summary["sourceSketchId"]=sketchId;body.summary["sourceEntityIds"]=p["entityIds"];
    body.summary["sideOneMm"]=sideOne;body.summary["sideTwoMm"]=sideTwo;
    body.summary["operation"]="newBody";
    return body;
}
