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
#include <QUuid>
#include <cmath>
#include <algorithm>
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
gp_Pnt worldPoint(const QString& plane, double u, double v) {
    if (plane == "XY") return {u, v, 0};
    if (plane == "XZ") return {u, 0, v};
    if (plane == "YZ") return {0, u, v};
    reject("Unsupported sketch origin plane");
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

TopoDS_Edge SketchGeometry::analyticEdge(const QJsonObject& parameters, const QJsonObject& entity) {
    QMap<QString,gp_Pnt> points;
    const auto plane = parameters["plane"].toString();
    for (const auto& value : parameters["points"].toArray()) { const auto p=value.toObject();
        points[p["id"].toString()] = worldPoint(plane,p["uMm"].toDouble(),p["vMm"].toDouble()); }
    if (entity["type"] == "line") return BRepBuilderAPI_MakeEdge(points[entity["startPointId"].toString()],points[entity["endPointId"].toString()]).Edge();
    const auto f=frame(plane); const auto n=f["normal"].toArray(), u=f["uAxis"].toArray();
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

QString SketchGeometry::validateParameters(const QJsonObject& parameters, QSet<QString>& authoredIds) {
    const auto plane = parameters["plane"].toString();
    if (plane != "XY" && plane != "XZ" && plane != "YZ") return "Unsupported sketch origin plane";
    const QSet<QString> allowed{"plane", "points", "entities", "constraints", "dimensions"};
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
        for (const auto& entry : parameters["entities"].toArray()) {
            if (!entry.isObject()) return "Sketch entity must be an object";
            const auto entity = entry.toObject();
            const auto type = entity["type"].toString();
            if (!claimId(entity["id"].toString())) return "Invalid or duplicate sketch entity UUID";
            if (type != "line" && type != "circle" && type != "arc") return "Unsupported sketch entity type";
            if (entity.size() != (type == "line" ? 4 : type == "circle" ? 4 : 8)) return "Unexpected sketch entity fields";
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
            } else if (type == "horizontal" || type == "vertical" || type == "parallel" || type == "perpendicular") {
                if (constraint.size() != 3 || constraint.contains("pointIds")
                    || !checkReferences(constraint, "entityIds", (type == "parallel" || type == "perpendicular") ? 2 : 1, lineIds))
                    return "Invalid line constraint references";
            } else return "Unsupported sketch constraint type";
        }
        for (const auto& entry : parameters["dimensions"].toArray()) {
            if (!entry.isObject()) return "Sketch dimension must be an object";
            const auto dimension = entry.toObject();
            if (!claimId(dimension["id"].toString())) return "Invalid or duplicate dimension UUID";
            const auto kind = dimension["kind"].toString();
            const bool curve = kind == "radius" || kind == "diameter" || kind == "arcSweep";
            if (!dimension["driving"].isBool()) return "Dimension requires driving flag";
            if (curve) {
                if (!(kind=="arcSweep" ? arcIds : curveIds).contains(dimension["entityId"].toString())) return "Invalid curve dimension reference";
            } else if ((kind != "linear" && kind != "horizontal" && kind != "vertical")
                || !checkReferences(dimension,"pointIds",2,pointIds)) return "Invalid dimension kind or references";
            const bool driving=dimension["driving"].toBool(), placed=dimension.contains("annotation");
            const auto valueKey=kind=="arcSweep" ? "valueDeg" : "valueMm";
            if (dimension.size() != (driving ? 5 : 4)+int(placed) || (!driving && dimension.contains(valueKey))) return "Unexpected dimension fields";
            if (placed) {
                if (!dimension["annotation"].isObject()) return "Dimension annotation must be an object";
                const auto annotation = dimension["annotation"].toObject();
                if (annotation.size() != 2) return "Dimension annotation requires only uMm and vMm";
                coordinate(annotation, "uMm"); coordinate(annotation, "vMm");
            }
            if (driving) {
                const double value = coordinate(dimension, valueKey);
                if (kind=="arcSweep" && (value < 1e-6 || value >= 360)) return "Angular dimension must be between zero and 360 degrees";
                if (kind!="arcSweep" && value < minimumLength) return "Driving dimension must be positive (0.001..100000 mm)";
            }
        }
    } catch (const std::invalid_argument& error) { return QString::fromUtf8(error.what()); }
    return {};
}

SketchGeometry::SolveResult SketchGeometry::solveDraft(const QJsonObject& draft) {
    try {
        validateDraft(draft);
        const auto result = SketchSolver::solve(draft["parameters"].toObject());
        if (!result.accepted) return {false, draft, result.diagnostics, result.error};
        auto solved = draft;
        solved["parameters"] = result.parameters;
        validateDraft(solved);
        return {true, solved, result.diagnostics, {}};
    } catch (const std::exception& error) { return invalidDraft(draft, QString::fromUtf8(error.what())); }
}

SketchGeometry::SolveResult SketchGeometry::addConstraint(const QJsonObject& draft, const QJsonObject& arguments) {
    try {
        validateDraft(draft);
        checkFields(arguments, {"type", "pointIds", "entityIds"});
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
        const auto result = solveDraft(candidate);
        return result.ok ? result : SolveResult{false, draft, result.diagnostics, result.error};
    } catch (const std::exception& error) { return invalidDraft(draft, QString::fromUtf8(error.what())); }
}

SketchGeometry::SolveResult SketchGeometry::addDimension(const QJsonObject& draft, const QJsonObject& arguments) {
    try {
        validateDraft(draft);
        checkFields(arguments, {"kind", "pointIds", "entityId", "driving", "valueMm", "valueDeg", "annotation"});
        if (arguments.contains("driving") && !arguments["driving"].isBool()) reject("Dimension driving must be Boolean");
        auto parameters = draft["parameters"].toObject();
        QJsonArray pointIds = arguments["pointIds"].toArray();
        const auto kind=arguments["kind"].toString();
        const bool curve=kind=="radius" || kind=="diameter" || kind=="arcSweep";
        if (arguments.contains("entityId") && !curve) {
            if (arguments.contains("pointIds")) reject("Choose dimension points or one line, not both");
            bool found = false;
            for (const auto& entry : parameters["entities"].toArray()) {
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
        const auto valueKey=kind=="arcSweep" ? "valueDeg" : "valueMm";
        if (arguments.contains("annotation")) dimension["annotation"] = arguments["annotation"];
        if (driving) dimension[valueKey] = arguments[valueKey];
        else if (arguments.contains("valueMm") || arguments.contains("valueDeg")) reject("Driven dimensions measure geometry; omit its value");
        if (arguments.contains(kind=="arcSweep" ? "valueMm" : "valueDeg")) reject("Dimension value has incorrect units");
        auto dimensions = parameters["dimensions"].toArray(); dimensions.append(dimension);
        parameters["dimensions"] = dimensions;
        auto candidate = draft; candidate["parameters"] = parameters;
        const auto result = solveDraft(candidate);
        return result.ok ? result : SolveResult{false, draft, result.diagnostics, result.error};
    } catch (const std::exception& error) { return invalidDraft(draft, QString::fromUtf8(error.what())); }
}

SketchGeometry::SolveResult SketchGeometry::updateDimension(const QJsonObject& draft, const QJsonObject& arguments) {
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
            const auto key=dimension["kind"]=="arcSweep" ? "valueDeg" : "valueMm";
            coordinate(arguments,key);
            if(arguments.contains(dimension["kind"]=="arcSweep" ? "valueMm" : "valueDeg")) reject("Dimension value has incorrect units");
            dimension[key] = arguments[key]; dimensions[i] = dimension; found = true; break;
        }
        if (!found) reject("Driving dimension does not exist");
        parameters["dimensions"] = dimensions;
        auto candidate = draft; candidate["parameters"] = parameters;
        const auto result = solveDraft(candidate);
        return result.ok ? result : SolveResult{false, draft, result.diagnostics, result.error};
    } catch (const std::exception& error) { return invalidDraft(draft, QString::fromUtf8(error.what())); }
}

SketchGeometry::SolveResult SketchGeometry::updateAnnotation(const QJsonObject& draft, const QJsonObject& arguments) {
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
        const auto result = solveDraft(candidate);
        // A presentation edit preserves accepted authored coordinates verbatim.
        // Obtain diagnostics without applying any solver normalization to intent.
        return result.ok ? SolveResult{true, candidate, result.diagnostics, {}}
                         : SolveResult{false, draft, result.diagnostics, result.error};
    } catch (const std::exception& error) { return invalidDraft(draft, QString::fromUtf8(error.what())); }
}

SketchGeometry::SolveResult SketchGeometry::movePoint(const QJsonObject& draft, const QJsonObject& arguments) {
    try {
        validateDraft(draft);
        checkFields(arguments, {"pointId", "uMm", "vMm"});
        coordinate(arguments, "uMm"); coordinate(arguments, "vMm");
        auto parameters = draft["parameters"].toObject();
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
        const auto result=SketchSolver::solve(parameters);
        if(!result.accepted)return {false,draft,result.diagnostics,result.error};
        candidate["parameters"]=result.parameters;validateDraft(candidate);
        return {true,candidate,result.diagnostics,{}};
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

EvaluatedSketch SketchGeometry::evaluateFeature(const QJsonObject& feature) {
    const auto solution = solveDraft(feature);
    if (!solution.ok) reject(solution.error);
    const auto parameters = solution.feature["parameters"].toObject();
    const auto plane = parameters["plane"].toString();
    QMap<QString, gp_Pnt> points;
    QJsonArray worldPoints;
    for (const auto& entry : parameters["points"].toArray()) {
        const auto point = entry.toObject();
        const auto world = worldPoint(plane, point["uMm"].toDouble(), point["vMm"].toDouble());
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
            const auto edge = analyticEdge(parameters,entity);
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
        {"lengthUnit", "mm"}, {"plane", plane}, {"frame", frame(plane)}, {"solverIntegrated", true}, {"solver", solution.diagnostics},
        {"entities",geometry}, {"pointCount", points.size()}, {"entityCount", entities.size()}, {"worldPoints", worldPoints}, {"boundsMm", bounds}};
    const auto profile = SketchProfiles::derive(solution.feature);
    QJsonArray profileSummaries;
    std::vector<EvaluatedProfile> profiles;
    if (profile.ok) {
        profileSummaries.append(profile.profile.summary);
        profiles.push_back(profile.profile);
    }
    summary["profiles"] = profileSummaries;
    summary["profileDiagnostics"] = profile.diagnostics;
    return {feature["id"].toString(), feature["name"].toString(), plane, feature["visible"].toBool(), shape, summary, profiles};
}
