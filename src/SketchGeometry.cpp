#include "SketchGeometry.h"
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRep_Builder.hxx>
#include <Bnd_Box.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Edge.hxx>
#include <QJsonArray>
#include <QMap>
#include <QUuid>
#include <cmath>
#include <stdexcept>

namespace {
constexpr double minimumLength = 0.001;
constexpr double maximumLength = 100000;
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
        if (point["uMm"].toDouble() == u && point["vMm"].toDouble() == v) return point["id"].toString();
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

QJsonObject SketchGeometry::addRectangle(const QJsonObject& draft, const QJsonObject& arguments) {
    validateArguments(arguments, false);
    const double u1 = coordinate(arguments, "u1Mm"), v1 = coordinate(arguments, "v1Mm");
    const double u2 = coordinate(arguments, "u2Mm"), v2 = coordinate(arguments, "v2Mm");
    if (std::abs(u2 - u1) < minimumLength || std::abs(v2 - v1) < minimumLength)
        reject("Sketch rectangle sides must be at least 0.001 mm");
    auto changed = addLine(draft, {{"u1Mm", u1}, {"v1Mm", v1}, {"u2Mm", u2}, {"v2Mm", v1}});
    changed = addLine(changed, {{"u1Mm", u2}, {"v1Mm", v1}, {"u2Mm", u2}, {"v2Mm", v2}});
    changed = addLine(changed, {{"u1Mm", u2}, {"v1Mm", v2}, {"u2Mm", u1}, {"v2Mm", v2}});
    return addLine(changed, {{"u1Mm", u1}, {"v1Mm", v2}, {"u2Mm", u1}, {"v2Mm", v1}});
}

QString SketchGeometry::validateParameters(const QJsonObject& parameters, QSet<QString>& authoredIds) {
    const auto plane = parameters["plane"].toString();
    if (plane != "XY" && plane != "XZ" && plane != "YZ") return "Unsupported sketch origin plane";
    if (parameters.size() != 3 || !parameters["points"].isArray() || !parameters["entities"].isArray())
        return "Sketch parameters require only plane, points and entities";
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
        QSet<QString> usedPoints;
        for (const auto& entry : parameters["entities"].toArray()) {
            if (!entry.isObject()) return "Sketch entity must be an object";
            const auto entity = entry.toObject();
            if (entity.size() != 4 || entity["type"].toString() != "line" || !claimId(entity["id"].toString()))
                return "Invalid line type or duplicate sketch entity UUID";
            const auto startId = entity["startPointId"].toString(), endId = entity["endPointId"].toString();
            if (!points.contains(startId) || !points.contains(endId)) return "Sketch line endpoint does not exist";
            const auto start = points[startId], end = points[endId];
            const auto length = std::hypot(end["uMm"].toDouble() - start["uMm"].toDouble(), end["vMm"].toDouble() - start["vMm"].toDouble());
            if (length < minimumLength || length > maximumLength) return "Sketch line length out of supported range (0.001..100000 mm)";
            usedPoints.insert(startId);
            usedPoints.insert(endId);
        }
        if (usedPoints.size() != points.size()) return "Sketch contains unreferenced points";
    } catch (const std::invalid_argument& error) { return QString::fromUtf8(error.what()); }
    return {};
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
    validateDraft(feature);
    const auto parameters = feature["parameters"].toObject();
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
            const auto edge = BRepBuilderAPI_MakeEdge(points[entity["startPointId"].toString()], points[entity["endPointId"].toString()]).Edge();
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
    QJsonObject summary{{"kernel", "OCCT"}, {"version", ALLOY_OCCT_VERSION}, {"type", "sketch"}, {"valid", true},
        {"lengthUnit", "mm"}, {"plane", plane}, {"frame", frame(plane)}, {"solverIntegrated", false},
        {"pointCount", points.size()}, {"entityCount", entities.size()}, {"worldPoints", worldPoints}, {"boundsMm", bounds}};
    return {feature["id"].toString(), feature["name"].toString(), plane, feature["visible"].toBool(), shape, summary};
}
