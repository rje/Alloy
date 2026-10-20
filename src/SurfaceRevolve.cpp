// SPDX-License-Identifier: MIT
#include "SurfaceRevolve.h"
#include "PlanarFaces.h"
#include "SolidFillet.h"
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepPrimAPI_MakeRevol.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <QUuid>
#include <array>
#include <limits>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepTools_WireExplorer.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <cmath>
#include <numbers>

namespace {
EvaluatedBody evaluateLineChain(const QJsonObject& feature,const EvaluatedDocument& prior) {
    const auto p=feature.value("parameters").toObject();const auto ids=p.value("entityIds").toArray();
    const auto fid=feature.value("id").toString(),sid=p.value("sketchId").toString(),axisId=p.value("axisEntityId").toString();
    auto reject=[&](const QString& code,const QString& message){throw GeometryKernel::EvaluationError({{"featureId",fid},{"featureType","surfaceRevolve"},{"code",code},{"message",message}});};
    const EvaluatedSketch* sketch=nullptr;for(const auto& s:prior.sketches)if(s.featureId==sid)sketch=&s;
    if(!sketch)reject("surface_source_unavailable","Select an earlier solved sketch");
    std::vector<TopoDS_Edge> sources;std::vector<gp_Pnt> starts,ends;std::vector<bool> reversed;
    for(const auto& id:ids){
        // Reuse the accepted single-Line validator for axis, angle and strict half-plane admission.
        auto single=feature;auto q=p;q.remove("entityIds");q.remove("sectionVersion");q["entityId"]=id;single["parameters"]=q;
        SurfaceRevolve::evaluate(single,prior);
        auto edge=TopoDS::Edge(BRepBuilderAPI_Copy(sketch->entityEdges.value(id.toString()),false,false).Shape());BRepAdaptor_Curve curve(edge);
        starts.push_back(curve.Value(curve.FirstParameter()));ends.push_back(curve.Value(curve.LastParameter()));sources.push_back(edge);
    }
    auto same=[](const gp_Pnt& a,const gp_Pnt& b){const double scale=std::max({1.,std::abs(a.X()),std::abs(a.Y()),std::abs(a.Z()),std::abs(b.X()),std::abs(b.Y()),std::abs(b.Z())});return a.Distance(b)<=64*std::numeric_limits<double>::epsilon()*scale;};
    std::vector<gp_Pnt> nodes;std::vector<int> first,last,degree;
    auto node=[&](const gp_Pnt& a){for(int i=0;i<int(nodes.size());++i)if(same(a,nodes[i]))return i;nodes.push_back(a);degree.push_back(0);return int(nodes.size()-1);};
    for(int i=0;i<int(sources.size());++i){first.push_back(node(starts[i]));last.push_back(node(ends[i]));if(first.back()==last.back())reject("degenerate_section","Section Lines must have nonzero length");++degree[first.back()];++degree[last.back()];}
    int endpoints=0;for(int d:degree){if(d>2)reject("branched_section","Select one unbranched open Line chain");if(d==1)++endpoints;}
    if(endpoints!=2)reject("open_section_required","Select exactly one open chain; closed or disconnected sections are unsupported");
    int current=degree[first[0]]==1 ? first[0]:last[0];if(degree[current]!=1)reject("ordered_section_required","The first selected Line must begin at a chain endpoint");
    BRepBuilderAPI_MakeWire wire;QJsonArray orientation;
    for(int i=0;i<int(sources.size());++i){bool rev=current==last[i];if(current!=first[i] && !rev)reject("disconnected_section","Ordered section Lines must connect exactly; gaps are not healed");
        auto edge=sources[i];if(rev)edge.Reverse();wire.Add(edge);if(!wire.IsDone())reject("section_wire_failed","Could not connect the selected Lines");
        reversed.push_back(rev);orientation.append(QJsonObject{{"entityId",ids[i]},{"reversed",rev}});current=rev?first[i]:last[i];
    }
    if(degree[current]!=1)reject("open_section_required","The selected chain must end at its other endpoint");
    for(int i=0;i<int(sources.size());++i)for(int j=i+1;j<int(sources.size());++j){
        BRepExtrema_DistShapeShape distance(sources[i],sources[j]);distance.Perform();
        if(j>i+1 && distance.IsDone() && distance.Value()<1e-7)reject("self_intersecting_section","Nonadjacent section Lines must not intersect or touch");
        if(j==i+1){gp_Vec u(starts[i],ends[i]),v(starts[j],ends[j]);if(reversed[i])u.Reverse();if(reversed[j])v.Reverse();if(u.Normalized().Dot(v.Normalized()) < -1.+1e-10)reject("self_intersecting_section","Adjacent section Lines must not overlap");}
    }
    const BRepAdaptor_Curve ac(sketch->entityEdges.value(axisId));const auto origin=ac.Value(ac.FirstParameter());const gp_Ax1 axis(origin,gp_Dir(gp_Vec(origin,ac.Value(ac.LastParameter()))));
    BRepPrimAPI_MakeRevol sweep(wire.Wire(),axis,p.value("angleDeg").toDouble()*std::numbers::pi/180.*(p.value("reverseDirection").toBool()?-1.:1.),true);sweep.Build();
    if(!sweep.IsDone())reject("surface_revolve_failed","OCCT could not revolve the retained chain");
    EvaluatedBody body;body.featureId=fid;body.bodyId=feature.value("bodyId").toString();body.name=feature.value("name").toString();body.visible=feature.value("visible").toBool(true);body.shape=sweep.Shape();body.summary=GeometryKernel::summarize(body.shape);
    const auto topology=body.summary.value("topology").toObject();if(!body.summary.value("valid").toBool() || body.summary.value("closed").toBool() || topology.value("solids").toInt()!=0 || topology.value("shells").toInt()!=1 || topology.value("faces").toInt()!=ids.size() || !body.summary.value("volumeMm3").isNull())reject("surface_revolve_invalid","Revolve must produce one connected valid uncapped open sheet");
    auto stable=[&](const QByteArray& role){return QUuid::createUuidV5(QUuid(fid),QByteArray("alloy-surface-revolve/2/")+sid.toUtf8()+"/"+axisId.toUtf8()+"/"+role).toString(QUuid::WithoutBraces);};
    QJsonObject provenance{{"generatorFeatureId",fid},{"sourceSketchId",sid},{"sourceEntityIds",ids},{"axisEntityId",axisId},{"lineage","surfaceRevolveChainBuilder"}};
    TopTools_IndexedMapOfShape allEdges;TopExp::MapShapes(body.shape,TopAbs_EDGE,allEdges);QMap<int,QString> edgeIds;
    int index=0;for(BRepTools_WireExplorer it(wire.Wire());it.More();it.Next(),++index){const auto source=it.Current();TopoDS_Face face;
        for(const auto& generated:sweep.Generated(source))if(generated.ShapeType()==TopAbs_FACE)face=TopoDS::Face(generated);
        if(face.IsNull())reject("surface_lineage_unavailable","Could not identify the generated face for each source Line");
        auto detail=provenance;detail["sourceEntityId"]=ids[index];
        EvaluatedSupportFace support{stable(ids[index].toString().toUtf8()+"/face"),body.bodyId,fid,"surfaceRevolve.sheet",face,{},{},detail};
        TopTools_IndexedMapOfShape edges;TopExp::MapShapes(face,TopAbs_EDGE,edges);
        TopTools_IndexedMapOfShape firstEdges,lastEdges;TopExp::MapShapes(sweep.FirstShape(source),TopAbs_EDGE,firstEdges);TopExp::MapShapes(sweep.LastShape(source),TopAbs_EDGE,lastEdges);
        TopoDS_Vertex va,vb;TopExp::Vertices(source,va,vb,true);
        for(int e=1;e<=edges.Extent();++e){auto edge=TopoDS::Edge(edges(e));const int key=allEdges.FindIndex(edge);QString role;
            if(firstEdges.Contains(edge))role=ids[index].toString()+"/start";else if(lastEdges.Contains(edge))role=ids[index].toString()+"/end";
            else {bool atStart=false;for(const auto& generated:sweep.Generated(va))if(generated.IsSame(edge))atStart=true;role="junction/"+QString::number(index+(atStart?0:1));}
            const auto eid=edgeIds.contains(key)?edgeIds.value(key):stable(role.toUtf8());auto ed=detail;ed["role"]=role;
            if(!edgeIds.contains(key)){edgeIds[key]=eid;body.solidEdges.push_back({eid,edge,ed});}support.boundaryEdges.push_back({eid,"curve",edge,ed});
        }body.supportFaces.push_back(std::move(support));
    }
    body.summary["supportFaces"]=PlanarFaces::descriptors(body.supportFaces);body.summary["solidEdges"]=SolidFillet::descriptors(body.solidEdges);body.summary["sourceSketchId"]=sid;body.summary["sourceEntityIds"]=ids;body.summary["orientedSources"]=orientation;body.summary["axisEntityId"]=axisId;body.summary["axisStartMm"]=QJsonArray{origin.X(),origin.Y(),origin.Z()};body.summary["axisDirection"]=QJsonArray{axis.Direction().X(),axis.Direction().Y(),axis.Direction().Z()};
    for(const auto& key:{"extent","direction","angleDeg","reverseDirection","operation","isSolid"})body.summary[key]=p.value(key);return body;
}
}

EvaluatedBody SurfaceRevolve::evaluate(const QJsonObject& feature,const EvaluatedDocument& prior) {
    const auto p=feature.value("parameters").toObject();
    if(p.contains("entityIds"))return evaluateLineChain(feature,prior);
    const auto featureId=feature.value("id").toString(),sketchId=p.value("sketchId").toString();
    const auto entityId=p.value("entityId").toString(),axisId=p.value("axisEntityId").toString();
    auto reject=[&](const QString& code,const QString& message){throw GeometryKernel::EvaluationError({
        {"featureId",featureId},{"featureType","surfaceRevolve"},{"sketchId",sketchId},
        {"entityId",entityId},{"axisEntityId",axisId},{"code",code},{"message",message}});};
    const EvaluatedSketch* sketch=nullptr;
    for(const auto& candidate:prior.sketches)if(candidate.featureId==sketchId){sketch=&candidate;break;}
    if(!sketch)reject("surface_source_unavailable","Surface Revolve requires an earlier solved sketch");
    QJsonObject source,axisSource;
    for(const auto& value:sketch->summary.value("entities").toArray()){
        const auto e=value.toObject();if(e.value("id")==entityId)source=e;if(e.value("id")==axisId)axisSource=e;
    }
    if(source.value("type")!="line" || source.value("construction").toBool() || !sketch->entityEdges.contains(entityId))
        reject("unsupported_surface_source","Select one ordinary open straight sketch Line; connected chains remain unsupported");
    if(axisId==entityId || axisSource.value("type")!="line" || !axisSource.value("construction").toBool()
        || !sketch->entityEdges.contains(axisId))
        reject("axis_unavailable","Select a distinct retained construction Line in the same sketch");
    auto edge=TopoDS::Edge(BRepBuilderAPI_Copy(sketch->entityEdges.value(entityId),false,false).Shape());
    const BRepAdaptor_Curve profile(edge),axisCurve(sketch->entityEdges.value(axisId));
    if(profile.GetType()!=GeomAbs_Line || profile.IsClosed())reject("unsupported_surface_source","Profile must remain one open straight Line");
    if(axisCurve.GetType()!=GeomAbs_Line)reject("axis_unavailable","Axis must remain a straight construction Line");
    const auto a=axisCurve.Value(axisCurve.FirstParameter()),b=axisCurve.Value(axisCurve.LastParameter());
    if(a.Distance(b)<1e-7)reject("degenerate_axis","Construction axis must have nonzero solved length");
    const gp_Ax1 axis(a,gp_Dir(gp_Vec(a,b)));
    const auto normalArray=sketch->resolvedFrame.value("normal").toArray();
    if(normalArray.size()!=3)reject("surface_source_unavailable","Source sketch must have a resolved plane");
    const gp_Vec normal(normalArray[0].toDouble(),normalArray[1].toDouble(),normalArray[2].toDouble());
    const gp_Vec transverse=gp_Vec(axis.Direction()).Crossed(normal);
    if(transverse.Magnitude()<1e-7)reject("axis_unavailable","Axis must lie in the source sketch plane");
    const gp_Dir side(transverse);
    const auto first=profile.Value(profile.FirstParameter()),last=profile.Value(profile.LastParameter());
    const double d1=gp_Vec(a,first).Dot(gp_Vec(side)),d2=gp_Vec(a,last).Dot(gp_Vec(side));
    if(std::abs(d1)<=1e-7 || std::abs(d2)<=1e-7 || (d1<0)!=(d2<0))
        reject("axis_crosses_profile","Surface section must stay strictly on one side of its axis, without touching it");
    const double angle=p.value("angleDeg").toDouble();
    if(!std::isfinite(angle) || angle<=0 || angle>=360)reject("invalid_angle","Partial Surface Revolve angle must be greater than 0 and less than 360 degrees");
    const double radians=angle*std::numbers::pi/180.*(p.value("reverseDirection").toBool() ? -1.:1.);
    BRepPrimAPI_MakeRevol sweep(edge,axis,radians,true);sweep.Build();
    if(!sweep.IsDone())reject("surface_revolve_failed","OCCT could not revolve the retained open section");
    EvaluatedBody body;body.featureId=featureId;body.bodyId=feature.value("bodyId").toString();
    body.name=feature.value("name").toString();body.visible=feature.value("visible").toBool(true);
    body.shape=sweep.Shape();body.summary=GeometryKernel::summarize(body.shape);
    const auto topology=body.summary.value("topology").toObject();
    if(!body.summary.value("valid").toBool() || body.summary.value("closed").toBool()
        || topology.value("faces").toInt()!=1 || topology.value("solids").toInt()!=0
        || topology.value("edges").toInt()!=4 || !body.summary.value("volumeMm3").isNull()
        || !std::isfinite(body.summary.value("areaMm2").toDouble()) || body.summary.value("areaMm2").toDouble()<=0)
        reject("surface_revolve_invalid","Surface Revolve must produce one valid positive-area open sheet with no caps or solids");
    auto stable=[&](const QByteArray& role){return QUuid::createUuidV5(QUuid(featureId),
        QByteArray("alloy-surface-revolve/1/")+sketchId.toUtf8()+"/"+entityId.toUtf8()+"/"+axisId.toUtf8()+"/"+role).toString(QUuid::WithoutBraces);};
    QJsonObject provenance{{"generatorFeatureId",featureId},{"sourceSketchId",sketchId},
        {"sourceEntityId",entityId},{"axisEntityId",axisId},{"lineage","surfaceRevolveBuilder"}};
    TopTools_IndexedMapOfShape faces,edges;TopExp::MapShapes(body.shape,TopAbs_FACE,faces);TopExp::MapShapes(body.shape,TopAbs_EDGE,edges);
    EvaluatedSupportFace support{stable("face"),body.bodyId,featureId,"surfaceRevolve.sheet",TopoDS::Face(faces(1)),{},{},provenance};
    const BRepAdaptor_Surface surface(support.face);
    auto xyz=[](const auto& q){return QJsonArray{q.X(),q.Y(),q.Z()};};
    if(surface.GetType()==GeomAbs_Plane){const auto plane=surface.Plane();auto u=plane.Position().XDirection(),v=plane.Position().YDirection();auto n=u.Crossed(v);
        if(support.face.Orientation()==TopAbs_REVERSED){v.Reverse();n.Reverse();}
        support.frame={{"origin",xyz(plane.Location())},{"uAxis",xyz(u)},{"vAxis",xyz(v)},{"normal",xyz(n)}};
    }
    auto exact=[&](const TopoDS_Shape& shape)->TopoDS_Edge{if(shape.IsNull() || shape.ShapeType()!=TopAbs_EDGE || !edges.Contains(shape))return {};return TopoDS::Edge(edges(edges.FindIndex(shape)));};
    TopoDS_Vertex va,vb;TopExp::Vertices(edge,va,vb,true);
    auto generated=[&](const TopoDS_Vertex& vertex)->TopoDS_Edge{TopTools_IndexedMapOfShape found;
        for(const auto& shape:sweep.Generated(vertex))if(shape.ShapeType()==TopAbs_EDGE && edges.Contains(shape))found.Add(shape);
        return found.Extent()==1 ? exact(found(1)) : TopoDS_Edge{};};
    const std::array<std::pair<QByteArray,TopoDS_Edge>,4> boundaries{{{"start",exact(sweep.FirstShape())},
        {"end",exact(sweep.LastShape())},{"endA",generated(va)},{"endB",generated(vb)}}};
    TopTools_IndexedMapOfShape retained;
    for(const auto& [role,current]:boundaries){
        if(current.IsNull() || retained.Contains(current))reject("surface_lineage_unavailable","Could not identify four distinct current sheet boundaries");retained.Add(current);
        auto detail=provenance;detail["role"]=QString::fromLatin1(role);const BRepAdaptor_Curve curve(current);
        detail["startMm"]=xyz(curve.Value(curve.FirstParameter()));detail["endMm"]=xyz(curve.Value(curve.LastParameter()));
        const auto id=stable(QByteArray("edge/")+role);body.solidEdges.push_back({id,current,detail});
        support.boundaryEdges.push_back({id,curve.GetType()==GeomAbs_Line ? "line":"curve",current,detail});
    }
    body.supportFaces.push_back(std::move(support));
    body.summary["supportFaces"]=PlanarFaces::descriptors(body.supportFaces);body.summary["solidEdges"]=SolidFillet::descriptors(body.solidEdges);
    body.summary["sourceSketchId"]=sketchId;body.summary["sourceEntityId"]=entityId;body.summary["axisEntityId"]=axisId;
    for(const auto& key:{"extent","direction","angleDeg","reverseDirection","operation","isSolid"})body.summary[key]=p.value(key);
    body.summary["axisStartMm"]=xyz(a);body.summary["axisDirection"]=xyz(axis.Direction());return body;
}
