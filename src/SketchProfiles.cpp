#include "SketchProfiles.h"
#include "SketchGeometry.h"
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp.hxx>
#include <BRep_Tool.hxx>
#include <BRep_Builder.hxx>
#include <GProp_GProps.hxx>
#include <Geom_Curve.hxx>
#include <Geom2d_Circle.hxx>
#include <Geom2d_TrimmedCurve.hxx>
#include <Geom2dAPI_InterCurveCurve.hxx>
#include <GCE2d_MakeSegment.hxx>
#include <gp_Ax2d.hxx>
#include <numbers>
#include <Standard_Failure.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Wire.hxx>
#include <gp_Ax3.hxx>
#include <gp_Pln.hxx>
#include <QJsonArray>
#include <QMap>
#include <QSet>
#include <QStringList>
#include <QUuid>
#include <algorithm>
#include <cmath>
#include <vector>

namespace {
// PlaneGCS's independently checked coincidence residual, in canonical mm.
constexpr double endpointTolerance = 1e-6;
// OCCT's default absolute geometry tolerance, in canonical mm.
constexpr double geometryTolerance = 1e-7;
struct Point { double u, v; };
struct Edge { QString id, sourceStart, sourceEnd, start, end; QJsonObject entity; };
struct DirectedEdge { int index; QString start, end; bool reversed; };
QJsonArray ids(const QStringList& values) {
    QJsonArray result;
    for (const auto& value : values) result.append(value);
    return result;
}
double cross(const Point& a, const Point& b, const Point& c) {
    return (b.u-a.u)*(c.v-a.v)-(b.v-a.v)*(c.u-a.u);
}
bool onSegment(const Point& p, const Point& a, const Point& b, double tolerance) {
    return std::abs(cross(a,b,p))/std::hypot(b.u-a.u,b.v-a.v) <= tolerance
        && p.u >= std::min(a.u,b.u)-tolerance && p.u <= std::max(a.u,b.u)+tolerance
        && p.v >= std::min(a.v,b.v)-tolerance && p.v <= std::max(a.v,b.v)+tolerance;
}
bool intersects(const Point& a, const Point& b, const Point& c, const Point& d, double tolerance) {
    const double ab = std::hypot(b.u-a.u,b.v-a.v), cd = std::hypot(d.u-c.u,d.v-c.v);
    const double cSide = cross(a,b,c)/ab, dSide = cross(a,b,d)/ab;
    const double aSide = cross(c,d,a)/cd, bSide = cross(c,d,b)/cd;
    const auto opposite = [tolerance](double p, double q) {
        return (p > tolerance && q < -tolerance) || (p < -tolerance && q > tolerance);
    };
    return (opposite(cSide,dSide) && opposite(aSide,bSide))
        || onSegment(a,c,d,tolerance) || onSegment(b,c,d,tolerance)
        || onSegment(c,a,b,tolerance) || onSegment(d,a,b,tolerance);
}
}

SketchProfiles::Result SketchProfiles::derive(const QJsonObject& feature) {
    const auto sketchId = feature["id"].toString();
    const auto parameters = feature["parameters"].toObject();
    const auto plane = parameters["plane"].toString();
    QStringList sourcePointIds, sourceEntityIds;
    const auto refuse = [&](const QString& code, const QString& message, QJsonObject detail = {}) {
        detail["code"] = code; detail["message"] = message; detail["sketchId"] = sketchId;
        detail["profileCount"] = 0; detail["sourcePointIds"] = ids(sourcePointIds);
        detail["sourceEntityIds"] = ids(sourceEntityIds);
        return Result{false, {{}, sketchId, plane, {}, {}}, detail, message};
    };
    if (feature["type"].toString() != "sketch" || QUuid(sketchId).isNull()
        || feature.contains("bodyId") || !feature["parameters"].isObject())
        return refuse("invalid_sketch", "Derive a profile from a validated, solved sketch feature.");
    QSet<QString> authoredIds{sketchId, feature["ownerId"].toString()};
    const auto validation = SketchGeometry::validateParameters(parameters, authoredIds);
    if (!validation.isEmpty()) return refuse("invalid_sketch", validation);
    for (const auto& value : parameters["points"].toArray()) sourcePointIds.append(value.toObject()["id"].toString());
    QJsonArray entities;
    for (const auto& value : parameters["entities"].toArray()) {
        if (value.toObject()["construction"].toBool()) continue;
        entities.append(value);
        sourceEntityIds.append(value.toObject()["id"].toString());
    }
    sourcePointIds.sort(); sourceEntityIds.sort();
    if (sourceEntityIds.isEmpty()) return refuse("empty_sketch", "Draw a closed line boundary to create a profile.");

    const bool hasCurves=std::any_of(entities.begin(),entities.end(),[](const auto& value){return value.toObject()["type"]!="line";});
    const auto profileId=QUuid::createUuidV5(QUuid(sketchId),QByteArray("alloy-profile/1\n")+sourceEntityIds.join('\n').toUtf8()).toString(QUuid::WithoutBraces);
    const auto frame=SketchGeometry::frame(plane);const auto origin=frame["origin"].toArray();
    const auto u=frame["uAxis"].toArray(),v=frame["vAxis"].toArray(),n=frame["normal"].toArray();
    const gp_Ax3 axes(gp_Pnt(origin[0].toDouble(),origin[1].toDouble(),origin[2].toDouble()),
        gp_Dir(n[0].toDouble(),n[1].toDouble(),n[2].toDouble()),gp_Dir(u[0].toDouble(),u[1].toDouble(),u[2].toDouble()));
    int circles=0;for(const auto& value:entities)if(value.toObject()["type"]=="circle")++circles;
    if(circles) {
        if(entities.size()!=1)return refuse("multiple_loops","This slice supports a single circle or one connected line/arc boundary; separate additional regions into another sketch.");
        try {
            const auto entity=entities.first().toObject(); BRepBuilderAPI_MakeWire wire(SketchGeometry::analyticEdge(parameters,entity));
            BRepBuilderAPI_MakeFace made(gp_Pln(axes),wire.Wire(),true);
            if(!made.IsDone() || !BRepCheck_Analyzer(made.Face()).IsValid())return refuse("invalid_face","OCCT rejected the analytic circle profile.");
            GProp_GProps properties;BRepGProp::SurfaceProperties(made.Face(),properties);
            const double analyticArea=std::numbers::pi*std::pow(entity["radiusMm"].toDouble(),2),area=properties.Mass();
            if(!std::isfinite(area) || std::abs(area-analyticArea)>std::max(1e-6,analyticArea*1e-9))return refuse("invalid_face","Circle face area does not match its analytic radius.");
            QJsonObject summary{{"profileId",profileId},{"sketchId",sketchId},{"plane",plane},{"frame",frame},{"kernel","OCCT"},{"valid",true},
                {"type","closed_circle_profile"},{"areaUnit","mm2"},{"areaMm2",area},{"analyticAreaMm2",analyticArea},
                {"sourcePointIds",ids(sourcePointIds)},{"sourceEntityIds",ids(sourceEntityIds)},
                {"edgeProvenance",QJsonArray{QJsonObject{{"entityId",entity["id"]},{"centerPointId",entity["centerPointId"]},{"type","circle"}}}},
                {"vertices",QJsonArray{}},{"maxEndpointAdjustmentMm",0}};
            return {true,{profileId,sketchId,plane,made.Face(),summary},{{"code","closed_profile"},{"profileCount",1},{"sketchId",sketchId}}, {}};
        } catch(const Standard_Failure& error){return refuse("kernel_failure",QString::fromUtf8(error.GetMessageString()));}
    }
    QMap<QString, Point> points;
    QMap<QString, QString> representative;
    for (const auto& value : parameters["points"].toArray()) {
        const auto point = value.toObject(); const auto id = point["id"].toString();
        points[id] = {point["uMm"].toDouble(), point["vMm"].toDouble()}; representative[id] = id;
    }
    for (const auto& value : parameters["constraints"].toArray()) {
        const auto constraint = value.toObject();
        if (constraint["type"] != "coincident") continue;
        const auto references = constraint["pointIds"].toArray();
        const auto aId = references[0].toString(), bId = references[1].toString();
        const auto a = points.value(aId), b = points.value(bId);
        if (std::hypot(a.u-b.u,a.v-b.v) > endpointTolerance)
            return refuse("unresolved_coincidence", "Solve the declared coincident endpoints before deriving their closure.",
                {{"constraintId", constraint["id"]}, {"pointIds", references}, {"endpointToleranceMm", endpointTolerance}});
        const auto aRoot = representative.value(aId), bRoot = representative.value(bId);
        const auto retained = std::min(aRoot,bRoot), merged = std::max(aRoot,bRoot);
        if (retained != merged) for (auto it = representative.begin(); it != representative.end(); ++it)
            if (it.value() == merged) it.value() = retained;
    }
    QMap<QString, QStringList> aliases;
    double maxEndpointAdjustment = 0;
    for (auto it = representative.begin(); it != representative.end(); ++it) {
        const auto a = points.value(it.key()), b = points.value(it.value());
        const double adjustment = std::hypot(a.u-b.u,a.v-b.v);
        if (adjustment > endpointTolerance)
            return refuse("unresolved_coincidence", "The declared endpoint group cannot share one vertex within the solved coincidence tolerance.",
                {{"pointIds", QJsonArray{it.key(),it.value()}}, {"endpointToleranceMm", endpointTolerance}});
        maxEndpointAdjustment = std::max(maxEndpointAdjustment,adjustment);
        aliases[it.value()].append(it.key());
    }

    std::vector<Edge> edges;
    QMap<QString, std::vector<int>> adjacency;
    for (const auto& value : entities) {
        const auto entity = value.toObject(); const auto start = entity["startPointId"].toString(), end = entity["endPointId"].toString();
        Edge edge{entity["id"].toString(),start,end,representative.value(start),representative.value(end),entity};
        const auto a = points.value(edge.start), b = points.value(edge.end);
        if (edge.start == edge.end || std::hypot(a.u-b.u,a.v-b.v) <= geometryTolerance)
            return refuse("collapsed_profile", "A boundary line collapses after declared endpoint closure; repair that line.", {{"entityIds", QJsonArray{edge.id}}});
        const int index = static_cast<int>(edges.size()); edges.push_back(edge);
        adjacency[edge.start].push_back(index); adjacency[edge.end].push_back(index);
    }
    for (auto it = adjacency.cbegin(); it != adjacency.cend(); ++it)
        if (it.value().size() > 2)
            return refuse("branched_profile", "A profile boundary cannot branch; remove the extra line or use a separate sketch.", {{"pointIds", ids(aliases.value(it.key()))}});

    QSet<QString> visited;
    int components = 0;
    for (auto it = adjacency.cbegin(); it != adjacency.cend(); ++it) {
        if (visited.contains(it.key())) continue;
        ++components; std::vector<QString> pending{it.key()};
        while (!pending.empty()) {
            const auto point = pending.back(); pending.pop_back();
            if (visited.contains(point)) continue;
            visited.insert(point);
            for (const int index : adjacency.value(point)) {
                const auto& edge = edges[index]; pending.push_back(edge.start == point ? edge.end : edge.start);
            }
        }
    }
    const bool closedDegrees = std::all_of(adjacency.cbegin(),adjacency.cend(),[](const auto& incident){return incident.size() == 2;});
    if (components > 1) return refuse(closedDegrees ? "multiple_loops" : "disconnected_profile",
        "This slice supports one connected closed loop; join the boundary or use a separate sketch.", {{"componentCount", components}});
    if (!closedDegrees || edges.size() < (hasCurves ? 2u : 3u)) return refuse("open_profile",
        "Close the boundary by reusing an endpoint or applying a solved coincidence relation.");

    // Walk the one degree-two cycle. UUID ordering makes provenance deterministic.
    const auto first = adjacency.firstKey(); auto current = first;
    QSet<int> used; std::vector<DirectedEdge> ordered;
    while (used.size() < static_cast<int>(edges.size())) {
        int chosen = -1;
        for (const int index : adjacency.value(current)) if (!used.contains(index)
            && (chosen < 0 || edges[index].id < edges[chosen].id)) chosen = index;
        if (chosen < 0) return refuse("open_profile", "The boundary cannot be walked as one closed loop.");
        const auto& edge = edges[chosen]; const bool reversed = edge.end == current;
        const auto next = reversed ? edge.start : edge.end;
        ordered.push_back({chosen,current,next,reversed}); used.insert(chosen); current = next;
    }
    if (current != first) return refuse("open_profile", "The line boundary does not return to its first endpoint.");

    const double tolerance = geometryTolerance;
    if(hasCurves) {
        // Intersect exact trimmed 2D curves. Chords are never used to decide a
        // curved boundary's crossing or overlap. Common endpoints alone are valid.
        std::vector<Handle(Geom2d_Curve)> curves;
        for(const auto& edge:edges) {
            const auto a=points.value(edge.start),b=points.value(edge.end);
            if(edge.entity["type"]=="line")curves.push_back(GCE2d_MakeSegment(gp_Pnt2d(a.u,a.v),gp_Pnt2d(b.u,b.v)).Value());
            else {const auto center=points.value(edge.entity["centerPointId"].toString());
                Handle(Geom2d_Circle) circle=new Geom2d_Circle(gp_Ax2d(gp_Pnt2d(center.u,center.v),gp_Dir2d(1,0)),edge.entity["radiusMm"].toDouble());
                const double start=edge.entity["startAngleDeg"].toDouble()*std::numbers::pi/180,
                    end=start+edge.entity["sweepAngleDeg"].toDouble()*std::numbers::pi/180;
                curves.push_back(new Geom2d_TrimmedCurve(circle,std::min(start,end),std::max(start,end)));}
        }
        for(size_t i=0;i<edges.size();++i)for(size_t j=i+1;j<edges.size();++j) {
            Geom2dAPI_InterCurveCurve intersection(curves[i],curves[j],geometryTolerance);
            bool crossing=intersection.NbSegments()>0;
            for(int k=1;k<=intersection.NbPoints();++k) {
                const auto hit=intersection.Point(k);bool common=false;
                for(const auto& endpoint : {edges[i].start,edges[i].end}) {
                    if(endpoint!=edges[j].start && endpoint!=edges[j].end)continue;const auto p=points.value(endpoint);
                    if(std::hypot(hit.X()-p.u,hit.Y()-p.v)<=endpointTolerance)common=true;
                }
                if(!common)crossing=true;
            }
            if(crossing)return refuse("self_intersecting_profile","Analytic boundary curves cross or overlap; repair these entities.",
                {{"entityIds",QJsonArray{edges[i].id,edges[j].id}}});
        }
    } else {
    for (size_t i = 0; i < ordered.size(); ++i) for (size_t j = i+1; j < ordered.size(); ++j) {
        const auto& e = ordered[i]; const auto& f = ordered[j];
        const auto a = points.value(e.start), b = points.value(e.end), c = points.value(f.start), d = points.value(f.end);
        const bool adjacent = j == i+1 || (i == 0 && j+1 == ordered.size());
        bool crossing;
        if (adjacent) {
            const auto common = j == i+1 ? b : a, outerA = j == i+1 ? a : b, outerB = j == i+1 ? d : c;
            // Adjacent edges may meet, but may not fold back over one another.
            crossing = std::abs(cross(common,outerA,outerB))/std::hypot(outerA.u-common.u,outerA.v-common.v) <= tolerance
                && (outerA.u-common.u)*(outerB.u-common.u)+(outerA.v-common.v)*(outerB.v-common.v) > 0;
        } else crossing = intersects(a,b,c,d,tolerance);
        if (crossing) return refuse("self_intersecting_profile", "The boundary crosses, touches or overlaps itself; repair the identified lines.",
            {{"entityIds", QJsonArray{edges[e.index].id,edges[f.index].id}}});
    }
    }
    long double twiceArea = 0;
    for (const auto& edge : ordered) {
        const auto a = points.value(edge.start), b = points.value(edge.end);
        const auto& entity=edges[edge.index].entity;
        if(entity["type"]=="line") twiceArea += static_cast<long double>(a.u)*b.v-static_cast<long double>(b.u)*a.v;
        else {const auto center=points.value(entity["centerPointId"].toString());
            const double sweep=entity["sweepAngleDeg"].toDouble()*std::numbers::pi/180*(edge.reversed ? -1 : 1);
            const double radius=entity["radiusMm"].toDouble();
            twiceArea += center.u*(b.v-a.v)-center.v*(b.u-a.u)+radius*radius*sweep;
        }
    }
    const double analyticArea = static_cast<double>(std::abs(twiceArea)/2);
    if (!std::isfinite(analyticArea) || analyticArea <= tolerance*tolerance)
        return refuse("collapsed_profile", "The closed boundary encloses no usable area; repair the lines.");
    if (twiceArea < 0) {
        std::reverse(ordered.begin(),ordered.end());
        for (auto& edge : ordered) {std::swap(edge.start,edge.end);edge.reversed = !edge.reversed;}
    }

    const auto world = [&](const Point& p) {return gp_Pnt(origin[0].toDouble()+p.u*u[0].toDouble()+p.v*v[0].toDouble(),
        origin[1].toDouble()+p.u*u[1].toDouble()+p.v*v[1].toDouble(),origin[2].toDouble()+p.u*u[2].toDouble()+p.v*v[2].toDouble());};
    try {
        QMap<QString, TopoDS_Vertex> vertices;
        QJsonArray vertexProvenance, edgeProvenance;
        for (auto it = adjacency.cbegin(); it != adjacency.cend(); ++it) {
            const auto position = world(points.value(it.key()));
            vertices[it.key()] = BRepBuilderAPI_MakeVertex(position).Vertex();
            BRep_Builder vertexBuilder;vertexBuilder.UpdateVertex(vertices[it.key()],endpointTolerance);
            vertexProvenance.append(QJsonObject{{"sourcePointIds", ids(aliases.value(it.key()))},
                {"positionMm", QJsonArray{position.X(),position.Y(),position.Z()}}});
        }
        BRepBuilderAPI_MakeWire wire;
        for (const auto& directed : ordered) {
            const auto& edge=edges[directed.index];
            TopoDS_Edge boundary;
            if(edge.entity["type"]=="line") {
                BRepBuilderAPI_MakeEdge made(vertices.value(directed.start),vertices.value(directed.end));
                if(!made.IsDone())return refuse("invalid_boundary","OCCT could not construct a boundary edge.");boundary=made.Edge();
            } else {
                const auto original=SketchGeometry::analyticEdge(parameters,edge.entity);double first,last;
                const auto curve=BRep_Tool::Curve(original,first,last);const bool negative=edge.entity["sweepAngleDeg"].toDouble()<0;
                const auto startVertex=vertices.value(edge.start),endVertex=vertices.value(edge.end);
                BRepBuilderAPI_MakeEdge made(curve,negative ? endVertex : startVertex,negative ? startVertex : endVertex,first,last);
                if(!made.IsDone())return refuse("invalid_boundary","OCCT could not connect the analytic arc endpoints.");
                boundary=made.Edge();if(negative!=directed.reversed)boundary.Reverse();
            }
            wire.Add(boundary);
            if(!wire.IsDone())return refuse("invalid_boundary","OCCT could not connect the declared boundary vertices.");
            edgeProvenance.append(QJsonObject{{"entityId", edge.id}, {"startPointId", edge.sourceStart},
                {"endPointId", edge.sourceEnd}, {"reversed", directed.reversed}});
        }
        if (!BRep_Tool::IsClosed(wire.Wire())) return refuse("invalid_boundary", "OCCT could not close the declared line loop.");
        BRepBuilderAPI_MakeFace made(gp_Pln(axes),wire.Wire(),true);
        if (!made.IsDone() || !BRepCheck_Analyzer(made.Face()).IsValid())
            return refuse("invalid_face", "OCCT rejected the planar region; repair its boundary.");
        GProp_GProps properties; BRepGProp::SurfaceProperties(made.Face(),properties);
        const double area = properties.Mass();
        if (!std::isfinite(area) || area <= 0 || std::abs(area-analyticArea) > std::max(1e-6,analyticArea*1e-9))
            return refuse("invalid_face", "The evaluated face area does not match the authored analytic boundary.");

        QJsonObject summary{{"profileId", profileId}, {"sketchId", sketchId}, {"plane", plane}, {"frame", frame},
            {"kernel", "OCCT"}, {"valid", true}, {"type",hasCurves ? "closed_curve_profile" : "closed_line_profile"}, {"areaUnit", "mm2"},
            {"areaMm2", area}, {"analyticAreaMm2", analyticArea}, {"sourcePointIds", ids(sourcePointIds)},
            {"sourceEntityIds", ids(sourceEntityIds)}, {"edgeProvenance", edgeProvenance}, {"vertices", vertexProvenance},
            {"maxEndpointAdjustmentMm", maxEndpointAdjustment}};
        QJsonObject diagnostics{{"code", "closed_profile"}, {"message",hasCurves ? "One simple closed analytic curve profile." : "One simple closed line profile."},
            {"profileCount", 1}, {"sketchId", sketchId}, {"endpointToleranceMm", endpointTolerance}};
        return {true, {profileId,sketchId,plane,made.Face(),summary}, diagnostics, {}};
    } catch (const Standard_Failure& error) {
        return refuse("kernel_failure", "OCCT profile construction failed: "+QString::fromUtf8(error.GetMessageString()));
    }
}
