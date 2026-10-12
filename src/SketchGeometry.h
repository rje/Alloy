#pragma once
#include "GeometryKernel.h"
#include <QSet>
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
    static QJsonObject findFeature(const QJsonObject& authored, const QString& sketchId);
    static QJsonObject replaceFeature(const QJsonObject& authored, const QJsonObject& draft);
    // Canonical arguments: u1Mm, v1Mm, u2Mm, v2Mm. Lines may reuse explicit point IDs.
    static QJsonObject addLine(const QJsonObject& draft, const QJsonObject& arguments);
    // Curve inputs normalize to analytic radius and signed angular sweep.
    static QJsonObject addCircle(const QJsonObject& draft, const QJsonObject& arguments);
    static QJsonObject addArc(const QJsonObject& draft, const QJsonObject& arguments);
    // Exact OCCT edge from validated analytic intent, shared by display/profiles.
    static TopoDS_Edge analyticEdge(const QJsonObject& parameters, const QJsonObject& entity);
    static QJsonObject addRectangle(const QJsonObject& draft, const QJsonObject& arguments);
    static SolveResult solveDraft(const QJsonObject& draft);
    static SolveResult addConstraint(const QJsonObject& draft, const QJsonObject& arguments);
    // Line angles use kind=angle, two entityIds, optional raySigns (+1/-1),
    // and driving valueDeg. Ray choice is independent of annotation placement.
    static SolveResult addDimension(const QJsonObject& draft, const QJsonObject& arguments);
    static SolveResult updateDimension(const QJsonObject& draft, const QJsonObject& arguments);
    // Presentation intent in canonical sketch millimeters; never changes the measurement.
    static SolveResult updateAnnotation(const QJsonObject& draft, const QJsonObject& arguments);
    static SolveResult movePoint(const QJsonObject& draft, const QJsonObject& arguments);
    static QString validateParameters(const QJsonObject& parameters, QSet<QString>& authoredIds);
    static QJsonObject frame(const QString& plane);
    static EvaluatedSketch evaluateFeature(const QJsonObject& feature);
};
