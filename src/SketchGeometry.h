#pragma once
#include "GeometryKernel.h"
#include <QSet>
#include <QJsonArray>
#include <TopoDS_Edge.hxx>

// Authored 2D geometry and its OCCT evaluation. A sketch is not a solid body.
class SketchGeometry final {
public:
    struct SolveResult {
        bool ok = false;
        QJsonObject feature;
        QJsonObject diagnostics;
        QString error;
    };
    static QJsonObject newFeature(const QString& ownerId, const QString& plane,
        const QString& name = "Sketch");
    static QJsonObject newSupportedFeature(const QString& ownerId, const QJsonObject& support, const QString& name = "Sketch", const QJsonArray& supportEdges = {});
    static QString curveCenterPointId(const QString& sketchId, const QJsonObject& source);
    static QString projectionPointId(const QString& projectionId, bool end);
    static QJsonObject expandedProjectionFeature(const QJsonObject& feature, const QJsonObject& projectionContext);
    static QJsonObject findFeature(const QJsonObject& authored, const QString& sketchId);
    static QJsonObject replaceFeature(const QJsonObject& authored, const QJsonObject& draft);
    // Canonical arguments: u1Mm, v1Mm, u2Mm, v2Mm. Lines may reuse explicit point IDs.
    static QJsonObject addLine(const QJsonObject& draft, const QJsonObject& arguments);
    // Curve inputs normalize to analytic radius and signed angular sweep.
    static QJsonObject addCircle(const QJsonObject& draft, const QJsonObject& arguments);
    static QJsonObject addArc(const QJsonObject& draft, const QJsonObject& arguments);
    // Exact OCCT edge from validated analytic intent, shared by display/profiles.
    static TopoDS_Edge analyticEdge(const QJsonObject& parameters, const QJsonObject& entity, const QJsonObject& resolvedFrame = {});
    static QJsonObject addRectangle(const QJsonObject& draft, const QJsonObject& arguments);
    // Pointer creation is transactional. Optional startSnapTarget/endSnapTarget
    // are {kind:origin} or {kind:point,pointId:UUID}; accepted snaps retain
    // distinct endpoint IDs and real coincidence constraints. Explicit coordinate
    // creation above never acquires an implicit target.
    static SolveResult addPointerEntity(const QJsonObject& draft, const QString& tool,
        const QJsonObject& arguments, const QJsonObject& projectionContext = {});
    // Explicit normal/construction conversion retains geometry and relation IDs.
    // Pointer-only circle creation retains optional midpoint centerSnapTarget and
    // finite-line tangentSnapTarget; the signed tangent branch survives edits.
    static SolveResult filletCorners(const QJsonObject& draft, const QJsonObject& arguments, const QJsonObject& projectionContext = {});
    static SolveResult addPointerCircle(const QJsonObject& draft, const QJsonObject& arguments, const QJsonObject& projectionContext = {});
    static SolveResult setConstruction(const QJsonObject& draft, const QJsonObject& arguments, const QJsonObject& projectionContext = {});
    static SolveResult solveDraft(const QJsonObject& draft, const QJsonObject& projectionContext = {});
    static SolveResult addConstraint(const QJsonObject& draft, const QJsonObject& arguments, const QJsonObject& projectionContext = {});
    // Line angles use kind=angle, two entityIds, optional raySigns (+1/-1),
    // and driving valueDeg. Ray choice is independent of annotation placement.
    static SolveResult addDimension(const QJsonObject& draft, const QJsonObject& arguments, const QJsonObject& projectionContext = {});
    static SolveResult updateDimension(const QJsonObject& draft, const QJsonObject& arguments, const QJsonObject& projectionContext = {});
    // Presentation intent in canonical sketch millimeters; never changes the measurement.
    static SolveResult updateAnnotation(const QJsonObject& draft, const QJsonObject& arguments, const QJsonObject& projectionContext = {});
    static SolveResult movePoint(const QJsonObject& draft, const QJsonObject& arguments, const QJsonObject& projectionContext = {});
    static QString validateParameters(const QJsonObject& parameters, QSet<QString>& authoredIds, const QSet<QString>& derivedPointIds = {});
    static QJsonObject frame(const QString& plane);
    static QJsonObject evaluationFrame(const QJsonObject& parameters, const QJsonObject& resolvedFrame = {});
    static EvaluatedSketch evaluateFeature(const QJsonObject& feature, const QJsonObject& resolvedFrame = {}, const QJsonObject& projectionContext = {});
};
