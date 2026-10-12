#include "SketchGeometry.h"
#include "SketchSolver.h"
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRep_Builder.hxx>
#include <Bnd_Box.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Edge.hxx>
#include <gp_Circ.hxx>
#include <gp_Ax2.hxx>
#include <numbers>
#include <QJsonArray>
#include <QMap>
#include <QPointF>
#include <QUuid>
#include <cmath>
#include <algorithm>
#include <array>
#include <stdexcept>

namespace {
constexpr double minimumLength = 0.001;
constexpr double maximumLength = 100000;
constexpr double degrees = std::numbers::pi / 180.0;
constexpr double endpointTolerance = 1e-6;
QString newId() { return QUuid::createUuid().toString(QUuid::WithoutBraces); }
bool validId(const QString& id) {
    return !QUuid(id).isNull() && QUuid(id).toString(QUuid::WithoutBraces) == id;
}
[[noreturn]] void reject(const QString& message) { throw std::invalid_argument(message.toStdString()); }
double coordinate(const QJsonObject& object, const QString& key) {
    const auto value = object.value(key);
    if (!value.isDouble() || !std::isfinite(value.toDouble()) || std::abs(value.toDouble()) > maximumLength)
        reject("Sketch coordinate out of supported range: " + key);
    return value.toDouble();
}
void validateDraft(const QJsonObject& draft) {
    if (draft["type"].toString() != "sketch" || draft.contains("bodyId")
        || !validId(draft["id"].toString()) || !validId(draft["ownerId"].toString())
        || !draft["name"].isString() || draft["name"].toString().trimmed().isEmpty()
        || !draft["visible"].isBool() || !draft["parameters"].isObject())
        reject("Invalid authored sketch");
    QSet<QString> ids{draft["id"].toString(), draft["ownerId"].toString()};
    const auto error = SketchGeometry::validateParameters(draft["parameters"].toObject(), ids);
    if (!error.isEmpty()) reject(error);
}
void validateArguments(const QJsonObject& arguments, bool line) {
    const QSet<QString> fields = line ? QSet<QString>{"u1Mm", "v1Mm", "u2Mm", "v2Mm", "startPointId", "endPointId"}
                                     : QSet<QString>{"u1Mm", "v1Mm", "u2Mm", "v2Mm"};
    for (auto it = arguments.begin(); it != arguments.end(); ++it)
        if (!fields.contains(it.key())) reject("Unknown sketch argument: " + it.key());
    for (const auto& key : {"u1Mm", "v1Mm", "u2Mm", "v2Mm"}) coordinate(arguments, key);
    for (const auto& key : {"startPointId", "endPointId"})
        if (arguments.contains(key) && (!arguments[key].isString() || !validId(arguments[key].toString())))
            reject("Expected a stable sketch point UUID");
}
QString addPoint(QJsonArray& points, double u, double v, const QString& requestedId = {}) {
    for (const auto& entry : points) {
        const auto point = entry.toObject();
        if (!requestedId.isEmpty() && point["id"].toString() != requestedId) continue;
        if (std::hypot(point["uMm"].toDouble()-u, point["vMm"].toDouble()-v) < 1e-9) return point["id"].toString();
        if (!requestedId.isEmpty()) reject("Point ID coordinates do not match the authored endpoint");
    }
    if (!requestedId.isEmpty()) reject("Sketch point does not exist");
    const auto id = newId();
    points.append(QJsonObject{{"id", id}, {"uMm", u}, {"vMm", v}});
    return id;
}
gp_Pnt worldPoint(const QJsonObject& frame, double u, double v) {
    const auto origin=frame["origin"].toArray(),uAxis=frame["uAxis"].toArray(),vAxis=frame["vAxis"].toArray();
    return {origin[0].toDouble()+u*uAxis[0].toDouble()+v*vAxis[0].toDouble(),
        origin[1].toDouble()+u*uAxis[1].toDouble()+v*vAxis[1].toDouble(),
        origin[2].toDouble()+u*uAxis[2].toDouble()+v*vAxis[2].toDouble()};
}
void checkPointIds(const QJsonObject& arguments) {
    for(const auto& key : {"centerPointId","startPointId","endPointId"})
        if(arguments.contains(key) && (!arguments[key].isString() || !validId(arguments[key].toString())))
            reject("Expected a stable sketch point UUID");
}
void checkFields(const QJsonObject& arguments, const QSet<QString>& allowed) {
    for (auto it = arguments.begin(); it != arguments.end(); ++it)
        if (!allowed.contains(it.key())) reject("Unknown sketch argument: " + it.key());
}
SketchGeometry::SolveResult invalidDraft(const QJsonObject& draft, const QString& error) {
    return {false, draft, {{"solver", "PlaneGCS"}, {"status", "invalid"}}, error};
}
}

QJsonObject SketchGeometry::newFeature(const QString& ownerId, const QString& plane, const QString& name) {
    frame(plane);
    QJsonObject feature{{"id", newId()}, {"ownerId", ownerId}, {"type", "sketch"},
        {"name", name}, {"visible", true},
        {"parameters", QJsonObject{{"plane", plane}, {"points", QJsonArray{}}, {"entities", QJsonArray{}}}}};
    validateDraft(feature);
    return feature;
}

QJsonObject SketchGeometry::newSupportedFeature(const QString& ownerId, const QJsonObject& support, const QString& name, const QJsonArray& supportEdges) {
    auto feature=newFeature(ownerId,"XY",name);auto parameters=feature["parameters"].toObject();
    parameters["plane"]="face";parameters["support"]=support;
    QJsonArray projections;
    for(const auto& value:supportEdges){const auto edge=value.toObject();if(!edge["supported"].toBool())continue;
        auto source=support;source["edgeId"]=edge["edgeId"];
        const auto id=QUuid::createUuidV5(QUuid(feature["id"].toString()),QByteArray("alloy-projection/1\n")+edge["edgeId"].toString().toUtf8()).toString(QUuid::WithoutBraces);
        projections.append(QJsonObject{{"id",id},{"source",source}});
    }
    if(!projections.isEmpty())parameters["projections"]=projections;
    feature["parameters"]=parameters;validateDraft(feature);return feature;
}

QString SketchGeometry::projectionPointId(const QString& id,bool end) {
    return QUuid::createUuidV5(QUuid(id),end ? QByteArray("end") : QByteArray("start")).toString(QUuid::WithoutBraces);
}
QJsonObject SketchGeometry::expandedProjectionFeature(const QJsonObject& feature,const QJsonObject& context) {
    auto expanded=feature;auto parameters=feature["parameters"].toObject();
    const auto projections=parameters["projections"].toArray();
    auto points=parameters["points"].toArray(),entities=parameters["entities"].toArray();
    for(const auto& value:projections) {
        const auto projection=value.toObject();QJsonObject line;
        for(const auto& resolved:context["lines"].toArray())if(resolved.toObject()["id"]==projection["id"]){if(!line.isEmpty())reject("Ambiguous resolved projection");line=resolved.toObject();}
        if(line.isEmpty() || line["source"]!=projection["source"])reject("Retained projection requires current resolved support geometry");
        const auto id=projection["id"].toString();const auto start=projectionPointId(id,false),end=projectionPointId(id,true);
        points.append(QJsonObject{{"id",start},{"uMm",coordinate(line,"u1Mm")},{"vMm",coordinate(line,"v1Mm")}});
        points.append(QJsonObject{{"id",end},{"uMm",coordinate(line,"u2Mm")},{"vMm",coordinate(line,"v2Mm")}});
        entities.append(QJsonObject{{"id",id},{"type","line"},{"startPointId",start},{"endPointId",end},{"construction",true}});
    }
    parameters.remove("projections");parameters["points"]=points;parameters["entities"]=entities;expanded["parameters"]=parameters;return expanded;
}

QJsonObject SketchGeometry::findFeature(const QJsonObject& authored, const QString& sketchId) {
    for (const auto& entry : authored["features"].toArray()) {
        const auto feature = entry.toObject();
        if (feature["id"].toString() != sketchId) continue;
        if (feature["type"].toString() != "sketch") reject("Feature is not a sketch");
        validateDraft(feature);
        return feature;
    }
    reject("Sketch does not exist");
}

QJsonObject SketchGeometry::replaceFeature(const QJsonObject& authored, const QJsonObject& draft) {
    validateDraft(draft);
    if (draft["ownerId"] != authored["documentId"]) reject("Sketch belongs to a different document");
    auto candidate = authored;
    auto features = candidate["features"].toArray();
    bool found = false;
    for (int i = 0; i < features.size(); ++i) {
        const auto feature = features[i].toObject();
        if (feature["id"] != draft["id"]) continue;
        if (found || feature["type"].toString() != "sketch") reject("Invalid sketch feature identity");
        features[i] = draft;
        found = true;
    }
    if (!found) features.append(draft);
    candidate["features"] = features;
    return candidate;
}

QJsonObject SketchGeometry::addLine(const QJsonObject& draft, const QJsonObject& arguments) {
    validateDraft(draft);
    validateArguments(arguments, true);
    const double u1 = coordinate(arguments, "u1Mm"), v1 = coordinate(arguments, "v1Mm");
    const double u2 = coordinate(arguments, "u2Mm"), v2 = coordinate(arguments, "v2Mm");
    const auto length = std::hypot(u2 - u1, v2 - v1);
    if (length < minimumLength || length > maximumLength) reject("Sketch line length out of supported range (0.001..100000 mm)");
    auto changed = draft;
    auto parameters = changed["parameters"].toObject();
    auto points = parameters["points"].toArray();
    auto entities = parameters["entities"].toArray();
    const auto start = addPoint(points, u1, v1, arguments["startPointId"].toString());
    const auto end = addPoint(points, u2, v2, arguments["endPointId"].toString());
    entities.append(QJsonObject{{"id", newId()}, {"type", "line"}, {"startPointId", start}, {"endPointId", end}});
    parameters["points"] = points;
    parameters["entities"] = entities;
    changed["parameters"] = parameters;
    return changed;
}

QJsonObject SketchGeometry::addCircle(const QJsonObject& draft, const QJsonObject& arguments) {
    validateDraft(draft);
    checkFields(arguments, {"centerUMm", "centerVMm", "diameterMm", "centerPointId"});
    checkPointIds(arguments);
    const double u = coordinate(arguments, "centerUMm"), v = coordinate(arguments, "centerVMm");
    const double radius = coordinate(arguments, "diameterMm") / 2;
    if (radius < minimumLength || radius > maximumLength) reject("Circle radius out of supported range (0.001..100000 mm)");
    auto changed = draft; auto parameters = changed["parameters"].toObject();
    auto points = parameters["points"].toArray(), entities = parameters["entities"].toArray();
    const auto center = addPoint(points, u, v, arguments["centerPointId"].toString());
    entities.append(QJsonObject{{"id",newId()},{"type","circle"},{"centerPointId",center},{"radiusMm",radius}});
    parameters["points"] = points; parameters["entities"] = entities; changed["parameters"] = parameters;
    validateDraft(changed); return changed;
}

QJsonObject SketchGeometry::addArc(const QJsonObject& draft, const QJsonObject& arguments) {
    validateDraft(draft);
    checkPointIds(arguments);
    const bool threePoint = arguments.contains("throughUMm") || arguments.contains("throughVMm");
    double u, v, radius, start, sweep;
    if (threePoint) {
        checkFields(arguments, {"startUMm","startVMm","endUMm","endVMm","throughUMm","throughVMm","startPointId","endPointId","centerPointId"});
        const double ax = coordinate(arguments,"startUMm"), ay = coordinate(arguments,"startVMm");
        const double bx = coordinate(arguments,"endUMm"), by = coordinate(arguments,"endVMm");
        const double cx = coordinate(arguments,"throughUMm"), cy = coordinate(arguments,"throughVMm");
        const double x = bx-ax, y = by-ay, q = cx-ax, t = cy-ay;
        const double determinant = 2*(x*t-y*q);
        const double scale = std::max({std::hypot(x,y),std::hypot(q,t),std::hypot(cx-bx,cy-by)});
        if (scale < minimumLength || std::abs(determinant) <= 1e-9*scale*scale) reject("Three-point arc requires distinct noncollinear points");
        u = ax + ((x*x+y*y)*t-(q*q+t*t)*y)/determinant;
        v = ay + (x*(q*q+t*t)-q*(x*x+y*y))/determinant;
        radius = std::hypot(ax-u,ay-v); start = std::atan2(ay-v,ax-u)/degrees;
        auto positive = [](double angle) { angle = std::fmod(angle,360.0); return angle < 0 ? angle+360.0 : angle; };
        const double end = positive(std::atan2(by-v,bx-u)/degrees-start);
        const double through = positive(std::atan2(cy-v,cx-u)/degrees-start);
        sweep = through < end ? end : end-360;
    } else {
        checkFields(arguments, {"centerUMm","centerVMm","radiusMm","startAngleDeg","sweepAngleDeg","centerPointId","startPointId","endPointId"});
        u = coordinate(arguments,"centerUMm"); v = coordinate(arguments,"centerVMm");
        radius = coordinate(arguments,"radiusMm"); start = coordinate(arguments,"startAngleDeg"); sweep = coordinate(arguments,"sweepAngleDeg");
    }
    if (!std::isfinite(radius) || radius < minimumLength || radius > maximumLength || !std::isfinite(sweep)
        || std::abs(sweep) < 1e-6 || std::abs(sweep) >= 360 || radius*std::abs(sweep)*degrees < minimumLength)
        reject("Arc requires a usable radius and nonzero sweep smaller than 360 degrees");
    start = std::remainder(start,360.0);
    auto changed = draft; auto parameters = changed["parameters"].toObject();
    auto points = parameters["points"].toArray(), entities = parameters["entities"].toArray();
    const auto center = addPoint(points,u,v,arguments["centerPointId"].toString());
    const auto first = addPoint(points,u+radius*std::cos(start*degrees),v+radius*std::sin(start*degrees),arguments["startPointId"].toString());
    const auto last = addPoint(points,u+radius*std::cos((start+sweep)*degrees),v+radius*std::sin((start+sweep)*degrees),arguments["endPointId"].toString());
    entities.append(QJsonObject{{"id",newId()},{"type","arc"},{"centerPointId",center},{"startPointId",first},{"endPointId",last},
        {"radiusMm",radius},{"startAngleDeg",start},{"sweepAngleDeg",sweep}});
    parameters["points"] = points; parameters["entities"] = entities; changed["parameters"] = parameters;
    validateDraft(changed); return changed;
}

TopoDS_Edge SketchGeometry::analyticEdge(const QJsonObject& parameters, const QJsonObject& entity, const QJsonObject& resolvedFrame) {
    QMap<QString,gp_Pnt> points;
    const auto sketchFrame=evaluationFrame(parameters,resolvedFrame);
    for (const auto& value : parameters["points"].toArray()) { const auto p=value.toObject();
        points[p["id"].toString()] = worldPoint(sketchFrame,p["uMm"].toDouble(),p["vMm"].toDouble()); }
    if (entity["type"] == "line") return BRepBuilderAPI_MakeEdge(points[entity["startPointId"].toString()],points[entity["endPointId"].toString()]).Edge();
    const auto f=sketchFrame; const auto n=f["normal"].toArray(), u=f["uAxis"].toArray();
    const gp_Ax2 axes(points[entity["centerPointId"].toString()],gp_Dir(n[0].toDouble(),n[1].toDouble(),n[2].toDouble()),gp_Dir(u[0].toDouble(),u[1].toDouble(),u[2].toDouble()));
    const gp_Circ circle(axes,entity["radiusMm"].toDouble());
    if (entity["type"] == "circle") return BRepBuilderAPI_MakeEdge(circle).Edge();
    const double start=entity["startAngleDeg"].toDouble()*degrees, sweep=entity["sweepAngleDeg"].toDouble()*degrees;
    auto edge=BRepBuilderAPI_MakeEdge(circle,std::min(start,start+sweep),std::max(start,start+sweep)).Edge();
    if (sweep < 0) edge.Reverse(); return edge;
}

QJsonObject SketchGeometry::addRectangle(const QJsonObject& draft, const QJsonObject& arguments) {
    validateArguments(arguments, false);
    const double u1 = coordinate(arguments, "u1Mm"), v1 = coordinate(arguments, "v1Mm");
    const double u2 = coordinate(arguments, "u2Mm"), v2 = coordinate(arguments, "v2Mm");
    if (std::abs(u2 - u1) < minimumLength || std::abs(v2 - v1) < minimumLength)
        reject("Sketch rectangle sides must be at least 0.001 mm");
    auto changed = addLine(draft, {{"u1Mm", u1}, {"v1Mm", v1}, {"u2Mm", u2}, {"v2Mm", v1}});
    changed = addLine(changed, {{"u1Mm", u2}, {"v1Mm", v1}, {"u2Mm", u2}, {"v2Mm", v2}});
    changed = addLine(changed, {{"u1Mm", u2}, {"v1Mm", v2}, {"u2Mm", u1}, {"v2Mm", v2}});
    changed = addLine(changed, {{"u1Mm", u1}, {"v1Mm", v2}, {"u2Mm", u1}, {"v2Mm", v1}});
    auto parameters = changed["parameters"].toObject();
    const auto entities = parameters["entities"].toArray();
    auto constraints = parameters["constraints"].toArray();
    for (int i = 0; i < 4; ++i)
        constraints.append(QJsonObject{{"id", newId()}, {"type", i % 2 == 0 ? "horizontal" : "vertical"},
            {"entityIds", QJsonArray{entities[entities.size() - 4 + i].toObject()["id"]}}});
    parameters["constraints"] = constraints;
    changed["parameters"] = parameters;
    return changed;
}

SketchGeometry::SolveResult SketchGeometry::addPointerEntity(const QJsonObject& draft, const QString& tool, const QJsonObject& arguments, const QJsonObject& projectionContext) {
    try {
        validateDraft(draft);
        if (tool!="line" && tool!="rectangle") reject("Pointer authoring supports Line and Rectangle");
        checkFields(arguments,{"u1Mm","v1Mm","u2Mm","v2Mm","startSnapTarget","endSnapTarget"});
        double u1=coordinate(arguments,"u1Mm"),v1=coordinate(arguments,"v1Mm"),u2=coordinate(arguments,"u2Mm"),v2=coordinate(arguments,"v2Mm");
        auto parameters=draft["parameters"].toObject();auto points=parameters["points"].toArray();
        auto entities=parameters["entities"].toArray();auto constraints=parameters["constraints"].toArray();
        auto freshPoint=[&](double u,double v) {const auto id=newId();points.append(QJsonObject{{"id",id},{"uMm",u},{"vMm",v}});return id;};
        auto target=[&](const QString& key,double& u,double& v) -> QString {
            if (!arguments.contains(key)) return {};
            if (!arguments[key].isObject()) reject("Snap target must be an object");
            const auto selected=arguments[key].toObject();const auto kind=selected["kind"].toString();QString id;
            if (kind=="origin") {
                if (selected.size()!=1) reject("Origin snap target only accepts kind");
                id=parameters["originPointId"].toString();
                if (id.isEmpty()) {
                    id=freshPoint(0,0);parameters["originPointId"]=id;
                    constraints.append(QJsonObject{{"id",newId()},{"type","fixed"},{"pointIds",QJsonArray{id}},{"uMm",0},{"vMm",0}});
                }
            } else if (kind=="point") {
                if (selected.size()!=2 || !selected["pointId"].isString() || !validId(selected["pointId"].toString())) reject("Endpoint snap requires a stable pointId");
                id=selected["pointId"].toString();
            } else reject("Unsupported snap target kind");
            for (const auto& entry:points) {const auto p=entry.toObject();if(p["id"]==id){u=p["uMm"].toDouble();v=p["vMm"].toDouble();return id;}}
            reject("Snap target point does not exist");
        };
        const auto startTarget=target("startSnapTarget",u1,v1),endTarget=target("endSnapTarget",u2,v2);
        if (tool=="line" && (std::hypot(u2-u1,v2-v1)<minimumLength || std::hypot(u2-u1,v2-v1)>maximumLength)) reject("Snapped line length out of supported range");
        if (tool=="rectangle" && (std::abs(u2-u1)<minimumLength || std::abs(v2-v1)<minimumLength)) reject("Snapped rectangle sides must be at least 0.001 mm");
        const auto first=freshPoint(u1,v1);QString last;
        auto edge=[&](const QString& a,const QString& b) {const auto id=newId();entities.append(QJsonObject{{"id",id},{"type","line"},{"startPointId",a},{"endPointId",b}});return id;};
        if (tool=="line") {last=freshPoint(u2,v2);edge(first,last);}
        else {
            const auto second=freshPoint(u2,v1);last=freshPoint(u2,v2);const auto fourth=freshPoint(u1,v2);
            const QString corners[]{first,second,last,fourth};
            for (int i=0;i<4;++i) {
                const auto id=edge(corners[i],corners[(i+1)%4]);
                constraints.append(QJsonObject{{"id",newId()},{"type",i%2==0 ? "horizontal" : "vertical"},{"entityIds",QJsonArray{id}}});
            }
        }
        auto coincide=[&](const QString& endpoint,const QString& selected) {
            if (!selected.isEmpty())constraints.append(QJsonObject{{"id",newId()},{"type","coincident"},{"pointIds",QJsonArray{endpoint,selected}}});
        };
        coincide(first,startTarget);coincide(last,endTarget);
        parameters["points"]=points;parameters["entities"]=entities;parameters["constraints"]=constraints;
        auto candidate=draft;candidate["parameters"]=parameters;
        const auto result=solveDraft(candidate,projectionContext);
        return result.ok ? result : SolveResult{false,draft,result.diagnostics,result.error};
    } catch (const std::exception& error) {return invalidDraft(draft,QString::fromUtf8(error.what()));}
}

QString SketchGeometry::validateParameters(const QJsonObject& parameters, QSet<QString>& authoredIds) {
    const auto plane = parameters["plane"].toString();
    if (plane != "XY" && plane != "XZ" && plane != "YZ" && plane != "face") return "Unsupported sketch plane";
    if(plane=="face") {
        if(!parameters["support"].isObject())return "Face sketch requires authored support references";
        const auto support=parameters["support"].toObject();
        if(support.size()!=3)return "Unexpected sketch support fields";
        for(const auto& key:{"producerFeatureId","bodyId","faceId"})if(!support[key].isString() || !validId(support[key].toString()))return "Sketch support requires stable producer, body and face UUIDs";
    } else if(parameters.contains("support"))return "Origin-plane sketch cannot carry face support";
    if(parameters.contains("projections")) {
        if(plane!="face" || !parameters["projections"].isArray())return "Projections require a supported face sketch and array";
        QJsonArray lines;QSet<QString> sources;
        for(const auto& value:parameters["projections"].toArray()) {
            if(!value.isObject())return "Projection must be an object";const auto projection=value.toObject();
            if(projection.size()!=2 || !validId(projection["id"].toString()) || !projection["source"].isObject())return "Invalid retained projection";
            const auto source=projection["source"].toObject();if(source.size()!=4)return "Invalid projection source fields";
            for(const auto& key:{"producerFeatureId","bodyId","faceId","edgeId"})if(!source[key].isString() || !validId(source[key].toString()))return "Projection source requires stable UUIDs";
            for(const auto& key:{"producerFeatureId","bodyId","faceId"})if(source[key]!=parameters["support"].toObject()[key])return "Projection source must belong to sketch support";
            if(sources.contains(source["edgeId"].toString()))return "Duplicate projected source edge";sources.insert(source["edgeId"].toString());
            lines.append(QJsonObject{{"id",projection["id"]},{"source",source},{"u1Mm",0},{"v1Mm",0},{"u2Mm",1},{"v2Mm",0}});
        }
        try {return validateParameters(expandedProjectionFeature({{"parameters",parameters}},{{"lines",lines}})["parameters"].toObject(),authoredIds);}
        catch(const std::exception& error){return QString::fromUtf8(error.what());}
    }
    const QSet<QString> allowed{"plane", "points", "entities", "constraints", "dimensions", "originPointId", "support"};
    for (auto it = parameters.begin(); it != parameters.end(); ++it)
        if (!allowed.contains(it.key())) return "Unknown sketch parameter: " + it.key();
    if (!parameters["points"].isArray() || !parameters["entities"].isArray()) return "Sketch requires points and entities";
    for (const auto& field : {"constraints", "dimensions"})
        if (parameters.contains(field) && !parameters[field].isArray()) return QString(field) + " must be an array";
    QMap<QString, QJsonObject> points;
    auto claimId = [&](const QString& id) {
        if (!validId(id) || authoredIds.contains(id)) return false;
        authoredIds.insert(id);
        return true;
    };
    try {
        for (const auto& entry : parameters["points"].toArray()) {
            if (!entry.isObject()) return "Sketch point must be an object";
            const auto point = entry.toObject();
            const auto id = point["id"].toString();
            if (point.size() != 3 || !claimId(id)) return "Invalid or duplicate sketch point UUID";
            coordinate(point, "uMm");
            coordinate(point, "vMm");
            points.insert(id, point);
        }
        QSet<QString> usedPoints, entityIds;
        const auto originId=parameters["originPointId"].toString();
        if (parameters.contains("originPointId")) {
            if (!parameters["originPointId"].isString() || !points.contains(originId)) return "Sketch origin reference point does not exist";
            const auto origin=points[originId];
            if (std::abs(origin["uMm"].toDouble())>endpointTolerance || std::abs(origin["vMm"].toDouble())>endpointTolerance) return "Sketch origin reference must remain at zero";
            usedPoints.insert(originId);
        }
        for (const auto& entry : parameters["entities"].toArray()) {
            if (!entry.isObject()) return "Sketch entity must be an object";
            const auto entity = entry.toObject();
            const auto type = entity["type"].toString();
            if (!claimId(entity["id"].toString())) return "Invalid or duplicate sketch entity UUID";
            if (type != "line" && type != "circle" && type != "arc") return "Unsupported sketch entity type";
            const bool hasConstruction = entity.contains("construction");
            if (hasConstruction && (type != "line" || !entity["construction"].isBool()))
                return "Construction must be a boolean on a line entity";
            if (entity.size() != (type == "line" ? 4 : type == "circle" ? 4 : 8) + (hasConstruction ? 1 : 0)) return "Unexpected sketch entity fields";
            if (type != "line") {
                const auto centerId = entity["centerPointId"].toString();
                if (!points.contains(centerId)) return "Curve center does not exist";
                usedPoints.insert(centerId);
                const double radius=coordinate(entity,"radiusMm");
                if (radius < minimumLength) return "Curve radius out of supported range";
                if (type == "arc") {
                    const double start=coordinate(entity,"startAngleDeg"), sweep=coordinate(entity,"sweepAngleDeg");
                    if (std::abs(start) > 180 || std::abs(sweep) < 1e-6 || std::abs(sweep) >= 360 || radius*std::abs(sweep)*degrees < minimumLength)
                        return "Invalid normalized arc angles";
                    const auto center=points[centerId];
                    for (const auto& key : {"startPointId","endPointId"}) {
                        const auto endpoint=entity[key].toString(); if (!points.contains(endpoint)) return "Arc endpoint does not exist";
                        const double angle=(start+(QString(key)=="endPointId" ? sweep : 0))*degrees;
                        const auto p=points[endpoint];
                        if (std::hypot(p["uMm"].toDouble()-center["uMm"].toDouble()-radius*std::cos(angle),
                            p["vMm"].toDouble()-center["vMm"].toDouble()-radius*std::sin(angle)) > endpointTolerance)
                            return "Arc endpoints do not match its analytic radius and sweep";
                        usedPoints.insert(endpoint);
                    }
                    if (entity["startPointId"] == entity["endPointId"]) return "Arc endpoints must be distinct";
                }
            } else {
                const auto startId=entity["startPointId"].toString(), endId=entity["endPointId"].toString();
                if (!points.contains(startId) || !points.contains(endId)) return "Sketch line endpoint does not exist";
                const auto start=points[startId], end=points[endId];
                const double length=std::hypot(end["uMm"].toDouble()-start["uMm"].toDouble(),end["vMm"].toDouble()-start["vMm"].toDouble());
                if (length < minimumLength || length > maximumLength) return "Sketch line length out of supported range (0.001..100000 mm)";
                usedPoints.insert(startId); usedPoints.insert(endId);
            }
            entityIds.insert(entity["id"].toString());
        }
        if (usedPoints.size() != points.size()) return "Sketch contains unreferenced points";
        auto checkReferences = [](const QJsonObject& object, const QString& field, int count, const QSet<QString>& valid) {
            if (!object[field].isArray() || object[field].toArray().size() != count) return false;
            QSet<QString> used;
            for (const auto& id : object[field].toArray()) {
                if (!id.isString() || !valid.contains(id.toString()) || used.contains(id.toString())) return false;
                used.insert(id.toString());
            }
            return true;
        };
        const QSet<QString> pointIds(points.keyBegin(), points.keyEnd());
        QSet<QString> lineIds, curveIds, arcIds;
        for (const auto& entry : parameters["entities"].toArray()) {const auto e=entry.toObject();
            if(e["type"]=="line") lineIds.insert(e["id"].toString());
            else {curveIds.insert(e["id"].toString()); if(e["type"]=="arc") arcIds.insert(e["id"].toString());}}
        for (const auto& entry : parameters["constraints"].toArray()) {
            if (!entry.isObject()) return "Sketch constraint must be an object";
            const auto constraint = entry.toObject();
            if (!claimId(constraint["id"].toString())) return "Invalid or duplicate constraint UUID";
            const auto type = constraint["type"].toString();
            if (type == "coincident" || type == "fixed") {
                const bool fixed = type == "fixed";
                if (constraint.size() != (fixed ? 5 : 3) || constraint.contains("entityIds")
                    || !checkReferences(constraint, "pointIds", fixed ? 1 : 2, pointIds)) return "Invalid point constraint references";
                if (fixed) { coordinate(constraint, "uMm"); coordinate(constraint, "vMm"); }
            } else if (type == "arcTangent") {
                if (constraint.size()!=5 || !checkReferences(constraint,"entityIds",2,entityIds)
                    || !checkReferences(constraint,"pointIds",1,pointIds) || !constraint["side"].isDouble()
                    || (constraint["side"].toDouble()!=1 && constraint["side"].toDouble()!=-1))return "Arc tangency requires endpoint and signed side";
                const auto refs=constraint["entityIds"].toArray(); const auto contact=constraint["pointIds"].toArray()[0];
                if(!arcIds.contains(refs[0].toString()) || !lineIds.contains(refs[1].toString()))return "Arc tangent references must be arc then line";
                for(const auto& value:parameters["entities"].toArray()) {const auto e=value.toObject();
                    if((e["id"]==refs[0] || e["id"]==refs[1]) && e["startPointId"]!=contact && e["endPointId"]!=contact)
                        return "Arc tangent contact must be a shared endpoint";}
            } else if (type == "tangent") {
                if (constraint.size()!=4 || !checkReferences(constraint,"entityIds",2,entityIds)
                    || !constraint["side"].isDouble() || (constraint["side"].toDouble()!=1 && constraint["side"].toDouble()!=-1))
                    return "Tangent requires one circle, one line and signed side";
                const auto refs=constraint["entityIds"].toArray();
                bool circle=false;
                for (const auto& entity:parameters["entities"].toArray())
                    if (entity.toObject()["id"]==refs[0] && entity.toObject()["type"]=="circle") circle=true;
                if (!circle || !lineIds.contains(refs[1].toString())) return "Tangent references must be ordered circle then line";
            } else if (type == "midpoint") {
                if (constraint.size() != 4 || !checkReferences(constraint,"pointIds",1,pointIds)
                    || !checkReferences(constraint,"entityIds",1,lineIds)) return "Midpoint requires one point and one line";
                const auto refs = constraint["pointIds"].toArray();
                for (const auto& entity : parameters["entities"].toArray()) {
                    const auto line = entity.toObject();
                    if (line["id"] == constraint["entityIds"].toArray()[0]
                        && (line["startPointId"] == refs[0] || line["endPointId"] == refs[0]))
                        return "Midpoint point must be distinct from the line endpoints";
                }
            } else if (type == "equal") {
                if (constraint.size() != 3 || !checkReferences(constraint,"entityIds",2,entityIds))
                    return "Equal requires two distinct sketch entities";
                const auto refs = constraint["entityIds"].toArray();
                const bool lines = lineIds.contains(refs[0].toString()) && lineIds.contains(refs[1].toString());
                const bool curves = curveIds.contains(refs[0].toString()) && curveIds.contains(refs[1].toString());
                if (!lines && !curves) return "Equal requires two lines or two circles/arcs";
            } else if (type == "horizontal" || type == "vertical" || type == "parallel" || type == "perpendicular") {
                if (constraint.size() != 3 || constraint.contains("pointIds")
                    || !checkReferences(constraint, "entityIds", (type == "parallel" || type == "perpendicular") ? 2 : 1, lineIds))
                    return "Invalid line constraint references";
            } else return "Unsupported sketch constraint type";
        }
        if (!originId.isEmpty()) {
            bool fixed=false;
            for (const auto& entry:parameters["constraints"].toArray()) {const auto c=entry.toObject();
                if (c["type"]=="fixed" && c["pointIds"].toArray()==QJsonArray{originId} && c["uMm"].toDouble()==0 && c["vMm"].toDouble()==0) fixed=true;}
            if (!fixed) return "Sketch origin reference requires a retained fixed relation at zero";
        }
        for (const auto& entry : parameters["dimensions"].toArray()) {
            if (!entry.isObject()) return "Sketch dimension must be an object";
            const auto dimension = entry.toObject();
            if (!claimId(dimension["id"].toString())) return "Invalid or duplicate dimension UUID";
            const auto kind = dimension["kind"].toString();
            const bool curve = kind == "radius" || kind == "diameter" || kind == "arcSweep";
            const bool angle = kind == "angle";
            if (!dimension["driving"].isBool()) return "Dimension requires driving flag";
            if (curve) {
                if (!(kind=="arcSweep" ? arcIds : curveIds).contains(dimension["entityId"].toString())) return "Invalid curve dimension reference";
            } else if (angle) {
                if (!checkReferences(dimension,"entityIds",2,lineIds)) return "Angle requires two distinct lines";
                if (dimension.contains("raySigns")) {
                    const auto signs=dimension["raySigns"].toArray();
                    if (!dimension["raySigns"].isArray() || signs.size()!=2) return "Angle raySigns requires two signs";
                    for (const auto& sign:signs) if (!sign.isDouble() || (sign.toDouble()!=1 && sign.toDouble()!=-1)) return "Angle ray signs must be +1 or -1";
                }
            } else if ((kind != "linear" && kind != "horizontal" && kind != "vertical")
                || !checkReferences(dimension,"pointIds",2,pointIds)) return "Invalid dimension kind or references";
            const bool driving=dimension["driving"].toBool(), placed=dimension.contains("annotation");
            const auto valueKey=kind=="arcSweep" || angle ? "valueDeg" : "valueMm";
            if (dimension.size() != (driving ? 5 : 4)+int(placed)+int(angle && dimension.contains("raySigns")) || (!driving && dimension.contains(valueKey))) return "Unexpected dimension fields";
            if (placed) {
                if (!dimension["annotation"].isObject()) return "Dimension annotation must be an object";
                const auto annotation = dimension["annotation"].toObject();
                if (annotation.size() != 2) return "Dimension annotation requires only uMm and vMm";
                coordinate(annotation, "uMm"); coordinate(annotation, "vMm");
            }
            if (driving) {
                const double value = coordinate(dimension, valueKey);
                if (kind=="arcSweep" && (value < 1e-6 || value >= 360)) return "Angular dimension must be between zero and 360 degrees";
                if (angle && (value < 1e-6 || value >= 180)) return "Line angle must be between zero and 180 degrees";
                if (kind!="arcSweep" && !angle && value < minimumLength) return "Driving dimension must be positive (0.001..100000 mm)";
            }
        }
    } catch (const std::invalid_argument& error) { return QString::fromUtf8(error.what()); }
    return {};
}

SketchGeometry::SolveResult SketchGeometry::solveDraft(const QJsonObject& draft, const QJsonObject& projectionContext) {
    try {
        validateDraft(draft);
        const auto expanded=expandedProjectionFeature(draft,projectionContext);validateDraft(expanded);
        QSet<QString> lockedPoints,virtualEntities;
        for(const auto& value:draft["parameters"].toObject()["projections"].toArray()){const auto id=value.toObject()["id"].toString();
            virtualEntities.insert(id);lockedPoints.insert(projectionPointId(id,false));lockedPoints.insert(projectionPointId(id,true));}
        const auto result=SketchSolver::solve(expanded["parameters"].toObject(),lockedPoints);
        if(!result.accepted)return {false,draft,result.diagnostics,result.error};
        auto solved=draft;auto parameters=result.parameters;
        QJsonArray points,entities;
        for(const auto& value:parameters["points"].toArray())if(!lockedPoints.contains(value.toObject()["id"].toString()))points.append(value);
        for(const auto& value:parameters["entities"].toArray())if(!virtualEntities.contains(value.toObject()["id"].toString()))entities.append(value);
        parameters["points"]=points;parameters["entities"]=entities;
        if(draft["parameters"].toObject().contains("projections"))parameters["projections"]=draft["parameters"].toObject()["projections"];
        solved["parameters"]=parameters;validateDraft(solved);
        return {true,solved,result.diagnostics,{}};
    } catch(const std::exception& error){return invalidDraft(draft,QString::fromUtf8(error.what()));}
}

SketchGeometry::SolveResult SketchGeometry::addConstraint(const QJsonObject& draft, const QJsonObject& arguments, const QJsonObject& projectionContext) {
    try {
        validateDraft(draft);
        checkFields(arguments, {"type", "pointIds", "entityIds", "side"});
        auto constraint = arguments;
        constraint["id"] = newId();
        auto parameters = draft["parameters"].toObject();
        if (constraint["type"].toString() == "fixed") {
            const auto pointIds = constraint["pointIds"].toArray();
            if (pointIds.size() != 1) reject("Fixed constraint requires one point");
            bool found = false;
            for (const auto& entry : parameters["points"].toArray()) {
                const auto point = entry.toObject();
                if (point["id"] != pointIds[0]) continue;
                constraint["uMm"] = point["uMm"]; constraint["vMm"] = point["vMm"]; found = true; break;
            }
            if (!found) reject("Fixed point does not exist");
        }
        auto constraints = parameters["constraints"].toArray(); constraints.append(constraint);
        parameters["constraints"] = constraints;
        auto candidate = draft; candidate["parameters"] = parameters;
        const auto result = solveDraft(candidate,projectionContext);
        return result.ok ? result : SolveResult{false, draft, result.diagnostics, result.error};
    } catch (const std::exception& error) { return invalidDraft(draft, QString::fromUtf8(error.what())); }
}

SketchGeometry::SolveResult SketchGeometry::addDimension(const QJsonObject& draft, const QJsonObject& arguments, const QJsonObject& projectionContext) {
    try {
        validateDraft(draft);
        checkFields(arguments, {"kind", "pointIds", "entityId", "entityIds", "raySigns", "driving", "valueMm", "valueDeg", "annotation"});
        if (arguments.contains("driving") && !arguments["driving"].isBool()) reject("Dimension driving must be Boolean");
        auto parameters = draft["parameters"].toObject();
        QJsonArray pointIds = arguments["pointIds"].toArray();
        const auto kind=arguments["kind"].toString();
        const bool curve=kind=="radius" || kind=="diameter" || kind=="arcSweep";
        const bool angle=kind=="angle";
        if (angle && (arguments.contains("pointIds") || arguments.contains("entityId"))) reject("Angle dimensions require two line entityIds");
        if (!angle && (arguments.contains("entityIds") || arguments.contains("raySigns"))) reject("Only line angles accept entityIds/raySigns");
        if (arguments.contains("entityId") && !curve) {
            if (arguments.contains("pointIds")) reject("Choose dimension points or one line, not both");
            bool found = false;
            for (const auto& entry : expandedProjectionFeature(draft,projectionContext)["parameters"].toObject()["entities"].toArray()) {
                const auto entity = entry.toObject();
                if (entity["id"] != arguments["entityId"]) continue;
                pointIds = {entity["startPointId"], entity["endPointId"]}; found = true; break;
            }
            if (!found) reject("Dimension line does not exist");
        }
        const bool driving = arguments.value("driving").toBool(true);
        QJsonObject dimension{{"id", newId()}, {"kind", arguments["kind"]}, {"pointIds", pointIds}, {"driving", driving}};
        if(curve && arguments.contains("pointIds"))reject("Curve dimensions require one entity reference");
        if(curve) {dimension.remove("pointIds"); dimension["entityId"]=arguments["entityId"];}
        if(angle) {dimension.remove("pointIds");dimension["entityIds"]=arguments["entityIds"];
            if(arguments.contains("raySigns"))dimension["raySigns"]=arguments["raySigns"];}
        const auto valueKey=kind=="arcSweep" || angle ? "valueDeg" : "valueMm";
        if (arguments.contains("annotation")) dimension["annotation"] = arguments["annotation"];
        if (driving) dimension[valueKey] = arguments[valueKey];
        else if (arguments.contains("valueMm") || arguments.contains("valueDeg")) reject("Driven dimensions measure geometry; omit its value");
        if (arguments.contains(kind=="arcSweep" || angle ? "valueMm" : "valueDeg")) reject("Dimension value has incorrect units");
        auto dimensions = parameters["dimensions"].toArray(); dimensions.append(dimension);
        parameters["dimensions"] = dimensions;
        auto candidate = draft; candidate["parameters"] = parameters;
        const auto result = solveDraft(candidate,projectionContext);
        return result.ok ? result : SolveResult{false, draft, result.diagnostics, result.error};
    } catch (const std::exception& error) { return invalidDraft(draft, QString::fromUtf8(error.what())); }
}

SketchGeometry::SolveResult SketchGeometry::filletCorners(const QJsonObject& draft, const QJsonObject& arguments, const QJsonObject& projectionContext) {
    try {
        validateDraft(draft); checkFields(arguments,{"corners","radiusMm"});
        const double radius=coordinate(arguments,"radiusMm");
        if(radius<minimumLength || !arguments["corners"].isArray() || arguments["corners"].toArray().isEmpty())
            reject("Fillet requires corners and a positive radius");
        auto parameters=draft["parameters"].toObject(); auto entities=parameters["entities"].toArray();
        auto points=parameters["points"].toArray(); auto constraints=parameters["constraints"].toArray();
        auto dimensions=parameters["dimensions"].toArray();
        QMap<QString,QJsonObject> pointMap; QMap<QString,QString> roots;
        for(const auto& value:points){const auto p=value.toObject();pointMap[p["id"].toString()]=p;roots[p["id"].toString()]=p["id"].toString();}
        auto root=[&](QString id){while(roots[id]!=id)id=roots[id];return id;};
        for(const auto& value:constraints){const auto c=value.toObject();if(c["type"]!="coincident")continue;
            const auto refs=c["pointIds"].toArray();roots[root(refs[1].toString())]=root(refs[0].toString());}
        QSet<QString> touchedLines,retired; QJsonArray arcs,newRelations,removedDimensions;
        auto xy=[&](const QString& id){const auto p=pointMap[id];return QPointF(p["uMm"].toDouble(),p["vMm"].toDouble());};
        auto fresh=[&](const QPointF& position){const auto id=newId();points.append(QJsonObject{{"id",id},{"uMm",position.x()},{"vMm",position.y()}});return id;};
        for(const auto& value:arguments["corners"].toArray()) {
            if(!value.isObject())reject("Fillet corner must be an object");const auto corner=value.toObject();checkFields(corner,{"entityIds"});
            const auto refs=corner["entityIds"].toArray();if(!corner["entityIds"].isArray() || refs.size()!=2 || refs[0]==refs[1])reject("Fillet needs two distinct lines");
            int indices[2]={-1,-1};QJsonObject lines[2];
            for(int i=0;i<entities.size();++i)for(int j=0;j<2;++j)if(entities[i].toObject()["id"]==refs[j]){indices[j]=i;lines[j]=entities[i].toObject();}
            for(int j=0;j<2;++j){if(indices[j]<0 || lines[j]["type"]!="line" || lines[j].value("construction").toBool())reject("Fillet supports normal lines only");
                const auto id=lines[j]["id"].toString();if(touchedLines.contains(id))reject("Fillet corners cannot reuse a line in this slice");touchedLines.insert(id);}
            QString keys[2];int matches=0;
            for(const auto& a:{QString("startPointId"),QString("endPointId")})for(const auto& b:{QString("startPointId"),QString("endPointId")})
                if(root(lines[0][a].toString())==root(lines[1][b].toString())){keys[0]=a;keys[1]=b;++matches;}
            if(matches!=1)reject("Fillet requires one unambiguous connected corner");
            const auto cornerRoot=root(lines[0][keys[0]].toString());
            for(auto it=roots.begin();it!=roots.end();++it)if(root(it.key())==cornerRoot)retired.insert(it.key());
            const QPointF vertex=xy(lines[0][keys[0]].toString()); QPointF directions[2]; double lengths[2];
            for(int j=0;j<2;++j){const auto other=keys[j]=="startPointId" ? "endPointId" : "startPointId";
                directions[j]=xy(lines[j][other].toString())-vertex;lengths[j]=std::hypot(directions[j].x(),directions[j].y());directions[j]/=lengths[j];}
            const double dot=std::clamp(QPointF::dotProduct(directions[0],directions[1]),-1.0,1.0),angle=std::acos(dot);
            if(angle<1e-6 || std::numbers::pi-angle<1e-6)reject("Fillet corner lines must not be collinear");
            const double setback=radius/std::tan(angle/2);
            for(double length:lengths)if(setback>=length-minimumLength)reject("Fillet radius exceeds available line length");
            QPointF bisector=directions[0]+directions[1];bisector/=std::hypot(bisector.x(),bisector.y());
            const QPointF center=vertex+bisector*(radius/std::sin(angle/2));
            const QPointF contacts[2]={vertex+directions[0]*setback,vertex+directions[1]*setback};
            const auto centerId=fresh(center),firstId=fresh(contacts[0]),lastId=fresh(contacts[1]),arcId=newId();
            const double start=std::atan2(contacts[0].y()-center.y(),contacts[0].x()-center.x())/degrees;
            const double sweep=std::remainder(std::atan2(contacts[1].y()-center.y(),contacts[1].x()-center.x())/degrees-start,360.0);
            arcs.append(QJsonObject{{"id",arcId},{"type","arc"},{"centerPointId",centerId},{"startPointId",firstId},{"endPointId",lastId},
                {"radiusMm",radius},{"startAngleDeg",start},{"sweepAngleDeg",sweep}});
            for(int j=0;j<2;++j){const auto contactId=j==0 ? firstId : lastId;lines[j][keys[j]]=contactId;entities[indices[j]]=lines[j];
                const auto a=keys[j]=="startPointId" ? contacts[j] : xy(lines[j]["startPointId"].toString());
                const auto b=keys[j]=="endPointId" ? contacts[j] : xy(lines[j]["endPointId"].toString());
                const double cross=(b.x()-a.x())*(center.y()-a.y())-(b.y()-a.y())*(center.x()-a.x());
                newRelations.append(QJsonObject{{"id",newId()},{"type","arcTangent"},{"entityIds",QJsonArray{arcId,lines[j]["id"]}},
                    {"pointIds",QJsonArray{contactId}},{"side",cross>0 ? 1 : -1}});
            }
        }
        // Retiring a corner never silently drops unrelated authored references.
        for(const auto& value:entities){const auto e=value.toObject();for(const auto& field:{"startPointId","endPointId","centerPointId"})
            if(retired.contains(e[field].toString()))reject("Fillet corner is referenced by unrelated geometry");}
        QJsonArray keptConstraints;
        for(const auto& value:constraints){const auto c=value.toObject();bool references=false,allRetired=true;
            for(const auto& id:c["pointIds"].toArray()){references|=retired.contains(id.toString());allRetired&=retired.contains(id.toString());}
            if(references){if(c["type"]!="coincident" || !allRetired)reject("Fillet corner has an unrelated point constraint");continue;}keptConstraints.append(c);}
        QJsonArray keptDimensions;
        for(const auto& value:dimensions){const auto d=value.toObject();bool pointReference=false;for(const auto& id:d["pointIds"].toArray())pointReference|=retired.contains(id.toString());
            const bool lengthDimension=d["kind"]=="linear" || d["kind"]=="horizontal" || d["kind"]=="vertical";
            bool affected=lengthDimension && touchedLines.contains(d["entityId"].toString());
            const auto refs=d["pointIds"].toArray();
            if(lengthDimension && refs.size()==2)for(const auto& original:parameters["entities"].toArray()) {
                const auto line=original.toObject();if(!touchedLines.contains(line["id"].toString()))continue;
                const auto start=root(line["startPointId"].toString()),end=root(line["endPointId"].toString());
                const auto a=root(refs[0].toString()),b=root(refs[1].toString());
                if((a==start && b==end) || (a==end && b==start))affected=true;
            }
            if(affected)removedDimensions.append(d["id"]);
            else {if(pointReference)reject("Fillet corner has an unrelated point dimension");keptDimensions.append(d);}}
        QJsonArray keptPoints;for(const auto& value:points)if(!retired.contains(value.toObject()["id"].toString()))keptPoints.append(value);
        for(const auto& value:arcs)entities.append(value);for(const auto& value:newRelations)keptConstraints.append(value);
        const auto firstArc=arcs.first().toObject()["id"];
        keptDimensions.append(QJsonObject{{"id",newId()},{"kind","radius"},{"entityId",firstArc},{"driving",true},{"valueMm",radius}});
        for(int i=1;i<arcs.size();++i)keptConstraints.append(QJsonObject{{"id",newId()},{"type","equal"},{"entityIds",QJsonArray{firstArc,arcs[i].toObject()["id"]}}});
        parameters["points"]=keptPoints;parameters["entities"]=entities;parameters["constraints"]=keptConstraints;parameters["dimensions"]=keptDimensions;
        auto candidate=draft;candidate["parameters"]=parameters;auto result=solveDraft(candidate,projectionContext);
        if(!result.ok)return {false,draft,result.diagnostics,result.error};
        QJsonArray arcIds;for(const auto& value:arcs)arcIds.append(value.toObject()["id"]);
        result.diagnostics["createdArcIds"]=arcIds;result.diagnostics["removedDimensionIds"]=removedDimensions;return result;
    }catch(const std::exception& error){return invalidDraft(draft,QString::fromUtf8(error.what()));}
}

SketchGeometry::SolveResult SketchGeometry::addPointerCircle(const QJsonObject& draft, const QJsonObject& arguments, const QJsonObject& projectionContext) {
    try {
        validateDraft(draft);
        checkFields(arguments, {"centerUMm", "centerVMm", "diameterMm", "centerSnapTarget", "tangentSnapTarget"});
        double u = coordinate(arguments,"centerUMm"), v = coordinate(arguments,"centerVMm");
        double radius = coordinate(arguments,"diameterMm") / 2;
        if (radius < minimumLength || radius > maximumLength) reject("Circle radius out of supported range");
        auto parameters = draft["parameters"].toObject();
        auto points = parameters["points"].toArray(), entities = parameters["entities"].toArray();
        auto constraints = parameters["constraints"].toArray();
        QString midpointLine;
        if (arguments.contains("centerSnapTarget")) {
            if (!arguments["centerSnapTarget"].isObject()) reject("Circle snap target must be an object");
            const auto target = arguments["centerSnapTarget"].toObject();
            checkFields(target, {"kind","entityId"});
            if (target["kind"] != "midpoint" || !target["entityId"].isString() || !validId(target["entityId"].toString()))
                reject("Circle center currently supports a line midpoint target");
            for (const auto& entry : entities) {
                const auto line = entry.toObject();
                if (line["id"] != target["entityId"]) continue;
                if (line["type"] != "line") reject("Midpoint target must be a line");
                midpointLine = line["id"].toString();
                QJsonObject first,last;
                for (const auto& value : points) {
                    if (value.toObject()["id"] == line["startPointId"]) first = value.toObject();
                    if (value.toObject()["id"] == line["endPointId"]) last = value.toObject();
                }
                u = (first["uMm"].toDouble()+last["uMm"].toDouble())/2;
                v = (first["vMm"].toDouble()+last["vMm"].toDouble())/2;
                break;
            }
            if (midpointLine.isEmpty()) reject("Circle midpoint target no longer exists");
        }
        QString tangentLine; int tangentSide = 0;
        if (arguments.contains("tangentSnapTarget")) {
            if (!arguments["tangentSnapTarget"].isObject()) reject("Tangent target must be an object");
            const auto target = arguments["tangentSnapTarget"].toObject();
            checkFields(target,{"kind","entityId"});
            if (target["kind"] != "tangent" || !target["entityId"].isString() || !validId(target["entityId"].toString()))
                reject("Circle tangent target requires a line UUID");
            for (const auto& entry : entities) {
                const auto line = entry.toObject(); if (line["id"] != target["entityId"]) continue;
                if (line["type"] != "line") reject("Circle tangent target must be a line");
                QJsonObject first,last;
                for (const auto& value : points) {
                    if (value.toObject()["id"] == line["startPointId"]) first = value.toObject();
                    if (value.toObject()["id"] == line["endPointId"]) last = value.toObject();
                }
                const double dx=last["uMm"].toDouble()-first["uMm"].toDouble(), dy=last["vMm"].toDouble()-first["vMm"].toDouble();
                const double px=u-first["uMm"].toDouble(), py=v-first["vMm"].toDouble(), length=std::hypot(dx,dy);
                const double foot=(px*dx+py*dy)/(length*length);
                if (foot < -endpointTolerance/length || foot > 1+endpointTolerance/length)
                    reject("Tangent contact is outside the finite line");
                const double distance=(dx*py-dy*px)/length;
                radius=std::abs(distance);
                if (radius < minimumLength || radius > maximumLength) reject("Tangent circle radius out of supported range");
                tangentSide=distance>0 ? 1 : -1; tangentLine=line["id"].toString(); break;
            }
            if (tangentLine.isEmpty()) reject("Circle tangent target no longer exists");
        }
        const auto centerId = newId();
        points.append(QJsonObject{{"id",centerId},{"uMm",u},{"vMm",v}});
        const auto circleId = newId();
        entities.append(QJsonObject{{"id",circleId},{"type","circle"},{"centerPointId",centerId},{"radiusMm",radius}});
        if (!tangentLine.isEmpty()) constraints.append(QJsonObject{{"id",newId()},{"type","tangent"},
            {"entityIds",QJsonArray{circleId,tangentLine}},{"side",tangentSide}});
        if (!midpointLine.isEmpty()) constraints.append(QJsonObject{{"id",newId()},{"type","midpoint"},
            {"pointIds",QJsonArray{centerId}},{"entityIds",QJsonArray{midpointLine}}});
        parameters["points"] = points; parameters["entities"] = entities;
        if (!midpointLine.isEmpty() || !tangentLine.isEmpty()) parameters["constraints"] = constraints;
        auto candidate = draft; candidate["parameters"] = parameters;
        const auto result = solveDraft(candidate,projectionContext);
        return result.ok ? result : SolveResult{false,draft,result.diagnostics,result.error};
    } catch (const std::exception& error) { return invalidDraft(draft,QString::fromUtf8(error.what())); }
}

SketchGeometry::SolveResult SketchGeometry::setConstruction(const QJsonObject& draft, const QJsonObject& arguments, const QJsonObject& projectionContext) {
    try {
        validateDraft(draft);
        checkFields(arguments, {"entityIds", "construction"});
        if (!arguments["construction"].isBool()) reject("Construction requires an explicit boolean");
        if (!arguments["entityIds"].isArray() || arguments["entityIds"].toArray().isEmpty())
            reject("Select at least one line for construction conversion");
        QSet<QString> selected;
        for (const auto& value : arguments["entityIds"].toArray()) {
            if (!value.isString() || QUuid(value.toString()).isNull() || selected.contains(value.toString()))
                reject("Construction selection requires distinct line UUIDs");
            selected.insert(value.toString());
        }
        auto parameters = draft["parameters"].toObject();
        auto entities = parameters["entities"].toArray();
        auto remaining = selected;
        bool changed = false;
        for (int i = 0; i < entities.size(); ++i) {
            auto entity = entities[i].toObject();
            if (!selected.contains(entity["id"].toString())) continue;
            if (entity["type"] != "line") reject("Construction conversion currently supports lines only");
            remaining.remove(entity["id"].toString());
            if (entity.value("construction").toBool() == arguments["construction"].toBool()) continue;
            if (arguments["construction"].toBool()) entity["construction"] = true;
            else entity.remove("construction");
            entities[i] = entity; changed = true;
        }
        if (!remaining.isEmpty()) reject("Construction selection contains a missing entity");
        if (!changed) {
            auto result = solveDraft(draft,projectionContext);
            result.feature = draft;
            return result;
        }
        parameters["entities"] = entities;
        auto candidate = draft; candidate["parameters"] = parameters;
        const auto result = solveDraft(candidate,projectionContext);
        return result.ok ? result : SolveResult{false, draft, result.diagnostics, result.error};
    } catch (const std::exception& error) { return invalidDraft(draft, QString::fromUtf8(error.what())); }
}

SketchGeometry::SolveResult SketchGeometry::updateDimension(const QJsonObject& draft, const QJsonObject& arguments, const QJsonObject& projectionContext) {
    try {
        validateDraft(draft);
        checkFields(arguments, {"dimensionId", "valueMm", "valueDeg"});
        auto parameters = draft["parameters"].toObject();
        auto dimensions = parameters["dimensions"].toArray();
        bool found = false;
        for (int i = 0; i < dimensions.size(); ++i) {
            auto dimension = dimensions[i].toObject();
            if (dimension["id"] != arguments["dimensionId"]) continue;
            if (!dimension["driving"].toBool()) reject("A driven dimension cannot set geometry");
            const bool angular=dimension["kind"]=="arcSweep" || dimension["kind"]=="angle";
            const auto key=angular ? "valueDeg" : "valueMm";
            coordinate(arguments,key);
            if(arguments.contains(angular ? "valueMm" : "valueDeg")) reject("Dimension value has incorrect units");
            dimension[key] = arguments[key]; dimensions[i] = dimension; found = true; break;
        }
        if (!found) reject("Driving dimension does not exist");
        parameters["dimensions"] = dimensions;
        auto candidate = draft; candidate["parameters"] = parameters;
        const auto result = solveDraft(candidate,projectionContext);
        return result.ok ? result : SolveResult{false, draft, result.diagnostics, result.error};
    } catch (const std::exception& error) { return invalidDraft(draft, QString::fromUtf8(error.what())); }
}

SketchGeometry::SolveResult SketchGeometry::updateAnnotation(const QJsonObject& draft, const QJsonObject& arguments, const QJsonObject& projectionContext) {
    try {
        validateDraft(draft);
        checkFields(arguments, {"dimensionId", "annotation"});
        if (!arguments["annotation"].isObject()) reject("Dimension annotation must be an object");
        auto parameters = draft["parameters"].toObject();
        auto dimensions = parameters["dimensions"].toArray();
        bool found = false;
        for (int i = 0; i < dimensions.size(); ++i) {
            auto dimension = dimensions[i].toObject();
            if (dimension["id"] != arguments["dimensionId"]) continue;
            dimension["annotation"] = arguments["annotation"];
            dimensions[i] = dimension; found = true; break;
        }
        if (!found) reject("Dimension does not exist");
        parameters["dimensions"] = dimensions;
        auto candidate = draft; candidate["parameters"] = parameters;
        const auto result = solveDraft(candidate,projectionContext);
        // A presentation edit preserves accepted authored coordinates verbatim.
        // Obtain diagnostics without applying any solver normalization to intent.
        return result.ok ? SolveResult{true, candidate, result.diagnostics, {}}
                         : SolveResult{false, draft, result.diagnostics, result.error};
    } catch (const std::exception& error) { return invalidDraft(draft, QString::fromUtf8(error.what())); }
}

SketchGeometry::SolveResult SketchGeometry::movePoint(const QJsonObject& draft, const QJsonObject& arguments, const QJsonObject& projectionContext) {
    try {
        validateDraft(draft);
        checkFields(arguments, {"pointId", "uMm", "vMm"});
        coordinate(arguments, "uMm"); coordinate(arguments, "vMm");
        auto parameters = draft["parameters"].toObject();
        if (arguments["pointId"]==parameters.value("originPointId") && parameters.contains("originPointId")) reject("Sketch origin reference cannot be moved");
        auto points = parameters["points"].toArray();
        bool found = false;
        for (int i = 0; i < points.size(); ++i) {
            auto point = points[i].toObject();
            if (point["id"] != arguments["pointId"]) continue;
            point["uMm"] = arguments["uMm"]; point["vMm"] = arguments["vMm"]; points[i] = point; found = true; break;
        }
        if (!found) reject("Sketch point does not exist");
        parameters["points"] = points;
        auto candidate = draft; candidate["parameters"] = parameters;
        // Point manipulation supplies a solver seed. ArcRules restore normalized
        // analytic intent before final validation; no inconsistent state is accepted.
        if(parameters["projections"].toArray().isEmpty()) {
            const auto result=SketchSolver::solve(parameters);
            if(!result.accepted)return {false,draft,result.diagnostics,result.error};
            candidate["parameters"]=result.parameters;validateDraft(candidate);return {true,candidate,result.diagnostics,{}};
        }
        return solveDraft(candidate,projectionContext);
    } catch (const std::exception& error) { return invalidDraft(draft, QString::fromUtf8(error.what())); }
}

QJsonObject SketchGeometry::frame(const QString& plane) {
    QJsonArray u, v, normal;
    if (plane == "XY") { u = {1, 0, 0}; v = {0, 1, 0}; normal = {0, 0, 1}; }
    else if (plane == "XZ") { u = {1, 0, 0}; v = {0, 0, 1}; normal = {0, -1, 0}; }
    else if (plane == "YZ") { u = {0, 1, 0}; v = {0, 0, 1}; normal = {1, 0, 0}; }
    else reject("Unsupported sketch origin plane");
    return {{"origin", QJsonArray{0, 0, 0}}, {"uAxis", u}, {"vAxis", v}, {"normal", normal}};
}

QJsonObject SketchGeometry::evaluationFrame(const QJsonObject& parameters,const QJsonObject& resolvedFrame) {
    if(parameters["plane"]!="face")return frame(parameters["plane"].toString());
    if(resolvedFrame.size()!=4)reject("Face sketch requires an explicit resolved support frame");
    std::array<std::array<double,3>,4> vectors;int index=0;
    for(const auto& key:{"origin","uAxis","vAxis","normal"}){
        const auto values=resolvedFrame[key].toArray();if(!resolvedFrame[key].isArray() || values.size()!=3)reject("Invalid resolved support frame");
        for(int j=0;j<3;++j){if(!values[j].isDouble() || !std::isfinite(values[j].toDouble()))reject("Nonfinite support frame");vectors[index][j]=values[j].toDouble();}++index;}
    auto dot=[](const auto& a,const auto& b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];};
    for(int i=1;i<4;++i)if(std::abs(dot(vectors[i],vectors[i])-1)>1e-9)reject("Support frame axes must be unit length");
    if(std::abs(dot(vectors[1],vectors[2]))>1e-9 || std::abs(dot(vectors[1],vectors[3]))>1e-9 || std::abs(dot(vectors[2],vectors[3]))>1e-9)reject("Support frame axes must be orthogonal");
    const std::array<double,3> cross{vectors[1][1]*vectors[2][2]-vectors[1][2]*vectors[2][1],vectors[1][2]*vectors[2][0]-vectors[1][0]*vectors[2][2],vectors[1][0]*vectors[2][1]-vectors[1][1]*vectors[2][0]};
    if(dot(cross,vectors[3])<1-1e-9)reject("Support frame must be right handed");return resolvedFrame;
}

EvaluatedSketch SketchGeometry::evaluateFeature(const QJsonObject& feature, const QJsonObject& resolvedFrame, const QJsonObject& projectionContext) {
    const auto solution = solveDraft(feature,projectionContext);
    if (!solution.ok) reject(solution.error);
    const auto parameters = expandedProjectionFeature(solution.feature,projectionContext)["parameters"].toObject();
    const auto plane = parameters["plane"].toString();
    const auto sketchFrame=evaluationFrame(parameters,resolvedFrame);
    QMap<QString, gp_Pnt> points;
    QJsonArray worldPoints;
    for (const auto& entry : parameters["points"].toArray()) {
        const auto point = entry.toObject();
        const auto world = worldPoint(sketchFrame, point["uMm"].toDouble(), point["vMm"].toDouble());
        points.insert(point["id"].toString(), world);
        worldPoints.append(QJsonObject{{"id", point["id"]}, {"positionMm", QJsonArray{world.X(), world.Y(), world.Z()}}});
    }
    TopoDS_Shape shape;
    QJsonValue bounds(QJsonValue::Null);
    const auto entities = parameters["entities"].toArray();
    if (!entities.isEmpty()) {
        TopoDS_Compound compound;
        BRep_Builder builder;
        builder.MakeCompound(compound);
        for (const auto& entry : entities) {
            const auto entity = entry.toObject();
            const auto edge = analyticEdge(parameters,entity,sketchFrame);
            builder.Add(compound, edge);
        }
        if (!BRepCheck_Analyzer(compound).IsValid()) throw std::runtime_error("Kernel did not produce valid sketch edges");
        shape = compound;
        Bnd_Box box;
        BRepBndLib::AddOptimal(shape, box, false, false);
        double x0, y0, z0, x1, y1, z1;
        box.Get(x0, y0, z0, x1, y1, z1);
        bounds = QJsonObject{{"min", QJsonArray{x0, y0, z0}}, {"max", QJsonArray{x1, y1, z1}},
            {"size", QJsonArray{x1-x0, y1-y0, z1-z0}}};
    }
    QJsonArray geometry;
    for(const auto& value : entities) {const auto e=value.toObject(); auto detail=e;
        if(e["type"]=="line") detail["lengthMm"]=points[e["startPointId"].toString()].Distance(points[e["endPointId"].toString()]);
        else {const auto center=points[e["centerPointId"].toString()]; detail["centerMm"]=QJsonArray{center.X(),center.Y(),center.Z()};
            detail["lengthMm"]=e["radiusMm"].toDouble()*(e["type"]=="circle" ? 2*std::numbers::pi : std::abs(e["sweepAngleDeg"].toDouble())*degrees);
            if(e["type"]=="arc") {const auto first=points[e["startPointId"].toString()],last=points[e["endPointId"].toString()];
                detail["startMm"]=QJsonArray{first.X(),first.Y(),first.Z()};detail["endMm"]=QJsonArray{last.X(),last.Y(),last.Z()};}}
        geometry.append(detail);
    }
    QJsonObject summary{{"kernel", "OCCT"}, {"version", ALLOY_OCCT_VERSION}, {"type", "sketch"}, {"valid", true},
        {"lengthUnit", "mm"}, {"plane", plane}, {"frame", sketchFrame}, {"solverIntegrated", true}, {"solver", solution.diagnostics},
        {"entities",geometry}, {"pointCount", points.size()}, {"entityCount", entities.size()}, {"worldPoints", worldPoints}, {"boundsMm", bounds}};
    const auto profile = SketchProfiles::deriveRegions(solution.feature,sketchFrame);
    QJsonArray profileSummaries;
    std::vector<EvaluatedProfile> profiles;
    if (profile.ok) {
        profiles = profile.profiles;
        for (const auto& region : profiles) profileSummaries.append(region.summary);
    }
    summary["projectedLines"]=projectionContext["lines"].toArray();
    summary["unsupportedProjectedEdges"]=projectionContext["unsupportedEdges"].toArray();
    summary["profiles"] = profileSummaries;
    summary["profileDiagnostics"] = profile.diagnostics;
    return {feature["id"].toString(), feature["name"].toString(), plane, feature["visible"].toBool(), shape, summary, profiles, sketchFrame};
}
