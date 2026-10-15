// SPDX-License-Identifier: MIT
#include "SurfaceExtrude.h"
#include "PlanarFaces.h"
#include "SolidFillet.h"
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
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
#include <TopoDS_Wire.hxx>
#include <QUuid>
#include <QSet>
#include <array>
#include <cmath>
#include <memory>
#include <algorithm>

namespace {
QString stableId(const QString& feature,const QString& entity,const QByteArray& role) {
    return QUuid::createUuidV5(QUuid(feature),QByteArray("alloy-surface-extrude/1/")+entity.toUtf8()+"/"+role)
        .toString(QUuid::WithoutBraces);
}
}

namespace {
QMap<QString,QJsonObject> openEntities(const EvaluatedSketch& sketch) {
    QMap<QString,QJsonObject> result;
    for(const auto& row:sketch.summary["entities"].toArray()) {
        const auto e=row.toObject();const auto type=e["type"].toString();
        if(!e["construction"].toBool() && (type=="line" || type=="fitSpline") && sketch.entityEdges.contains(e["id"].toString()))result[e["id"].toString()]=e;
    }return result;
}
QStringList orderedChain(const EvaluatedSketch& sketch,const QJsonArray& members) {
    const auto entities=openEntities(sketch);QMap<QString,QStringList> contacts;QSet<QString> selected;
    for(const auto& value:members){const auto id=value.toString();const auto e=entities.value(id);
        const auto a=e["startPointId"].toString(),b=e["endPointId"].toString();
        if(e.isEmpty() || a.isEmpty() || b.isEmpty() || a==b || selected.contains(id))throw std::invalid_argument("Profile requires distinct retained ordinary open curves");
        selected.insert(id);contacts[a].append(id);contacts[b].append(id);
    }
    QStringList ends;for(auto i=contacts.begin();i!=contacts.end();++i){if(i.value().size()==1)ends.append(i.key());else if(i.value().size()!=2)throw std::invalid_argument("Open profile has ambiguous branching endpoint ancestry");}
    if(ends.size()!=2)throw std::invalid_argument("Profile must be one connected open chain with two retained ends");
    QStringList ordered;auto point=ends.first();QSet<QString> used;
    while(ordered.size()<selected.size()){
        QString next;for(const auto& id:contacts.value(point))if(!used.contains(id)){if(!next.isEmpty())throw std::invalid_argument("Ambiguous profile chain");next=id;}
        if(next.isEmpty())throw std::invalid_argument("Profile members are disconnected");
        used.insert(next);ordered.append(next);const auto e=entities.value(next);point=e["startPointId"]==point ? e["endPointId"].toString() : e["startPointId"].toString();
    }return ordered;
}
}
QString SurfaceExtrude::profileBodyId(const QString& featureId,const QString& sketchId,const QJsonArray& entityIds){
    QStringList members;for(const auto& v:entityIds)members.append(v.toString());members.sort();
    return QUuid::createUuidV5(QUuid(featureId),QByteArray("alloy-surface-extrude/profile-body/1/")+sketchId.toUtf8()+"/"+members.join('/').toUtf8()).toString(QUuid::WithoutBraces);
}
QStringList SurfaceExtrude::resultBodyIds(const QJsonObject& feature){
    const auto p=feature["parameters"].toObject();if(p["resultMode"]!="perProfile")return {feature["bodyId"].toString()};
    QStringList ids;for(const auto& value:p["profiles"].toArray())ids.append(profileBodyId(feature["id"].toString(),p["sketchId"].toString(),value.toObject()["entityIds"].toArray()));return ids;
}
QJsonArray SurfaceExtrude::openChain(const EvaluatedSketch& sketch,const QString& entityId){
    const auto entities=openEntities(sketch);if(!entities.contains(entityId))throw std::invalid_argument("Open source curve unavailable");
    QSet<QString> connected{entityId};bool changed=true;
    while(changed){changed=false;QSet<QString> points;for(const auto& id:connected){points.insert(entities[id]["startPointId"].toString());points.insert(entities[id]["endPointId"].toString());}
        for(auto i=entities.begin();i!=entities.end();++i)if(!connected.contains(i.key()) && (points.contains(i.value()["startPointId"].toString()) || points.contains(i.value()["endPointId"].toString()))){connected.insert(i.key());changed=true;}}
    QJsonArray members;for(const auto& id:connected)members.append(id);QJsonArray result;for(const auto& id:orderedChain(sketch,members))result.append(id);return result;
}

static EvaluatedBody evaluateSheet(const QJsonObject& feature,const EvaluatedDocument& prior,bool connected) {
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
    QMap<QString,TopoDS_Edge> translatedEdges;QMap<QString,QString> endpointBoundary;
    std::unique_ptr<BRepPrimAPI_MakePrism> chainPrism;
    if(connected){
        QStringList ordered;try{ordered=orderedChain(*sketch,p["entityIds"].toArray());}catch(const std::exception& e){reject("surface_profile_topology",QString::fromUtf8(e.what()));}
        const auto entities=openEntities(*sketch);BRepBuilderAPI_MakeWire wire;QMap<QString,TopoDS_Edge> wireMembers;
        for(const auto& id:ordered){wire.Add(TopoDS::Edge(BRepBuilderAPI_Copy(sketch->entityEdges.value(id),false,false).Shape()));if(!wire.IsDone())reject("surface_profile_topology","OCCT could not connect the retained open chain");
            auto member=wire.Edge();member.Orientation(sketch->entityEdges.value(id).Orientation());wireMembers[id]=member;
            const auto e=entities.value(id);for(const auto& pair:{std::pair{"startPointId","endA"},std::pair{"endPointId","endB"}}){
                const auto point=e[pair.first].toString(),name=id+"/"+pair.second;
                if(!endpointBoundary.contains(point) || name<endpointBoundary[point])endpointBoundary[point]=name;
            }
        }
        TopTools_IndexedMapOfShape vertices;TopExp::MapShapes(wire.Wire(),TopAbs_VERTEX,vertices);
        if(vertices.Extent()!=ordered.size()+1 || wire.Wire().Closed())reject("surface_profile_topology","Profile contacts must retain one nonbranching open wire");
        gp_Trsf shift;shift.SetTranslation(-normal*sideOne);BRepBuilderAPI_Transform translated(wire.Wire(),shift,true);
        if(!translated.IsDone())reject("surface_prism_failed","OCCT could not position the source wire");
        // MakeWire::Edge is the actual inserted member, including any shared-vertex copy.
        // Transform and prism histories then carry that retained source correspondence.
        for(const auto& id:ordered)translatedEdges[id]=TopoDS::Edge(translated.ModifiedShape(wireMembers.value(id)));
        chainPrism=std::make_unique<BRepPrimAPI_MakePrism>(translated.Shape(),normal*(sideOne+sideTwo),false,true);chainPrism->Build();
        if(!chainPrism->IsDone() || !BRepCheck_Analyzer(chainPrism->Shape()).IsValid())reject("surface_prism_invalid","Open chain must produce a valid connected sheet");
    }
    QSet<QString> publishedEdges;
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
        std::unique_ptr<BRepPrimAPI_MakePrism> singlePrism;TopoDS_Edge start;
        if(connected)start=translatedEdges.value(entityId);
        else {gp_Trsf shift;shift.SetTranslation(-normal*sideOne);BRepBuilderAPI_Transform translated(edge,shift,true);
            if(!translated.IsDone())reject("surface_prism_failed","OCCT could not position the source curve");
            start=TopoDS::Edge(translated.Shape());singlePrism=std::make_unique<BRepPrimAPI_MakePrism>(start,normal*(sideOne+sideTwo),false,true);singlePrism->Build();}
        auto& prism=connected ? *chainPrism : *singlePrism;
        if(!prism.IsDone())reject("surface_prism_failed","OCCT could not make a sheet from the source curve");
        TopoDS_Shape generated;
        if(connected){for(const auto& face:prism.Generated(start))if(face.ShapeType()==TopAbs_FACE){if(!generated.IsNull())reject("surface_lineage_unavailable","Multiple generated faces for source edge");generated=face;}}
        else generated=prism.Shape();
        TopTools_IndexedMapOfShape faces,edges;
        TopExp::MapShapes(generated,TopAbs_FACE,faces);TopExp::MapShapes(generated,TopAbs_EDGE,edges);
        if(faces.Extent()!=1 || edges.Extent()!=4 || !BRepCheck_Analyzer(generated).IsValid())
            reject("surface_prism_invalid","Each selected open curve must produce one valid four-edge sheet");
        auto exactEdge=[&](const TopoDS_Shape& shape)->TopoDS_Edge {
            if(shape.IsNull() || shape.ShapeType()!=TopAbs_EDGE || !edges.Contains(shape))return {};
            return TopoDS::Edge(edges(edges.FindIndex(shape)));
        };
        const auto side1=exactEdge(connected ? prism.FirstShape(start) : prism.FirstShape()),side2=exactEdge(connected ? prism.LastShape(start) : prism.LastShape());
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
        if(connected)support.provenance={{"generatorFeatureId",featureId},{"sourceSketchId",sketchId},{"sourceEntityId",entityId},{"lineage","surfacePrismBuilder"}};
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
            auto source=entityId;auto boundaryRole=role;
            if(connected && (role=="endA" || role=="endB")){
                const auto contact=entity[role=="endA" ? "startPointId" : "endPointId"].toString();
                const auto canonical=endpointBoundary.value(contact).split('/');source=canonical[0];boundaryRole=canonical[1].toLatin1();
            }
            const auto id=stableId(featureId,source,QByteArray("edge/")+boundaryRole);
            QJsonObject provenance{{"generatorFeatureId",featureId},{"sourceSketchId",sketchId},
                {"sourceEntityId",source},{"role",QString::fromLatin1(boundaryRole)},
                {"lineage","surfacePrismBuilder"}};
            BRepAdaptor_Curve boundaryCurve(current);
            const auto a=boundaryCurve.Value(boundaryCurve.FirstParameter()),b=boundaryCurve.Value(boundaryCurve.LastParameter());
            provenance["startMm"]=QJsonArray{a.X(),a.Y(),a.Z()};
            provenance["endMm"]=QJsonArray{b.X(),b.Y(),b.Z()};
            support.boundaryEdges.push_back({id,boundaryCurve.GetType()==GeomAbs_Line ? "line" : "curve",current,provenance});
            if(!publishedEdges.contains(id)){body.solidEdges.push_back({id,current,provenance});publishedEdges.insert(id);}
        }
        body.supportFaces.push_back(std::move(support));
    }
    body.shape=connected ? chainPrism->Shape() : TopoDS_Shape(compound);body.summary=GeometryKernel::summarize(body.shape);
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

EvaluatedBody SurfaceExtrude::evaluate(const QJsonObject& feature,const EvaluatedDocument& prior){return evaluateSheet(feature,prior,false);}
std::vector<EvaluatedBody> SurfaceExtrude::evaluateResults(const QJsonObject& feature,const EvaluatedDocument& prior){
    const auto p=feature["parameters"].toObject();if(p["resultMode"]!="perProfile")return {evaluate(feature,prior)};
    std::vector<EvaluatedBody> results;QSet<QString> selected;
    for(const auto& value:p["profiles"].toArray()){
        const auto members=value.toObject()["entityIds"].toArray();for(const auto& member:members){if(selected.contains(member.toString()))throw GeometryKernel::EvaluationError({{"featureId",feature["id"]},{"code","surface_profile_overlap"},{"message","Select each retained curve in one profile only"}});selected.insert(member.toString());}
        auto source=feature;source["bodyId"]=profileBodyId(feature["id"].toString(),p["sketchId"].toString(),members);
        auto params=p;params.remove("profiles");params.remove("resultMode");params["entityIds"]=members;source["parameters"]=params;
        auto body=evaluateSheet(source,prior,true);body.summary["resultMode"]="perProfile";body.summary["profile"] = value;
        body.summary["resultBodyId"]=body.bodyId;body.summary["producerFeatureId"]=feature["id"];
        body.name=feature["name"].toString()+" · "+body.bodyId.left(8);results.push_back(std::move(body));
    }return results;
}
