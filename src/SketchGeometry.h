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
    static QJsonObject newConstructionPlaneFeature(const QString& ownerId,const QString& planeId,const QString& name = "Sketch");
    static QString curveCenterPointId(const QString& sketchId, const QJsonObject& source);
    static QString projectionPointId(const QString& projectionId, bool end);
    static QJsonObject linkedProjectionCandidate(const EvaluatedDocument& evaluation,
        const QJsonObject& sketch, const QJsonObject& source);
    static SolveResult addClosedRectOffset(const QJsonObject& draft,const QJsonArray& sourceEntityIds,
        double signedDistanceMm,const QJsonObject& projectionContext);
    static QJsonObject expandedProjectionFeature(const QJsonObject& feature, const QJsonObject& projectionContext);
    static QJsonObject findFeature(const QJsonObject& authored, const QString& sketchId);
    static QJsonObject replaceFeature(const QJsonObject& authored, const QJsonObject& draft);
    // Canonical arguments: u1Mm, v1Mm, u2Mm, v2Mm. Lines may reuse explicit point IDs.
    static QJsonObject addLine(const QJsonObject& draft, const QJsonObject& arguments);
    // A true point entity owns one stable solver point, with no curve/profile.
    static SolveResult addStandalonePoint(const QJsonObject& draft, const QJsonObject& arguments,
        const QJsonObject& projectionContext = {});
    // Curve inputs normalize to analytic radius and signed angular sweep.
    static QJsonObject addCircle(const QJsonObject& draft, const QJsonObject& arguments);
    static QJsonObject addArc(const QJsonObject& draft, const QJsonObject& arguments);
    // Exact OCCT edge from validated analytic intent, shared by display/profiles.
    static TopoDS_Edge analyticEdge(const QJsonObject& parameters, const QJsonObject& entity, const QJsonObject& resolvedFrame = {});
    static QJsonObject addRectangle(const QJsonObject& draft, const QJsonObject& arguments);
    // Center/corner rectangle: four profile edges, two construction diagonals,
    // a retained center point, and driving width/height dimensions.
    static SolveResult addCenterRectangle(const QJsonObject& draft, const QJsonObject& arguments,
        const QJsonObject& projectionContext = {});
    // Center, one end-center direction/half-length and full width. The guide is
    // construction geometry; the four outer analytic edges form one profile.
    static SolveResult addCenterPointSlot(const QJsonObject& draft, const QJsonObject& arguments,
        const QJsonObject& projectionContext = {});
    // Two picked cap centers and full width. Optional snaps attach to the
    // corresponding guide endpoints, never to the slot midpoint.
    static SolveResult addCenterToCenterSlot(const QJsonObject& draft, const QJsonObject& arguments,
        const QJsonObject& projectionContext = {});
    // Two outer arc endpoints and a full width; total length drives center
    // separation as length minus width, with retained analytic slot edges.
    static SolveResult addOverallSlot(const QJsonObject& draft,const QJsonObject& arguments,
        const QJsonObject& projectionContext = {});
    static SolveResult addCenterPointArcSlot(const QJsonObject& draft, const QJsonObject& arguments,
        const QJsonObject& projectionContext = {});
    // Center, first-axis point, and an arbitrary point on the resulting
    // ellipse contour. Two retained full construction axes carry dimensions.
    static SolveResult addCenterPointEllipse(const QJsonObject& draft,const QJsonObject& arguments,
        const QJsonObject& projectionContext = {});
    // Pointer creation is transactional. Optional startSnapTarget/endSnapTarget
    // are {kind:origin}, {kind:point,pointId:UUID}, or for Line endpoints
    // {kind:midpoint,entityId:lineUUID}. Accepted snaps retain distinct
    // endpoint IDs and real coincidence or midpoint constraints. Explicit coordinate
    // creation above never acquires an implicit target.
    static SolveResult addPointerEntity(const QJsonObject& draft, const QString& tool,
        const QJsonObject& arguments, const QJsonObject& projectionContext = {});
    // Explicit normal/construction conversion retains geometry and relation IDs.
    // Pointer-only circle creation retains optional midpoint centerSnapTarget and
    // finite-line tangentSnapTarget; the signed tangent branch survives edits.
    static SolveResult trimLine(const QJsonObject& draft, const QJsonObject& arguments, const QJsonObject& projectionContext = {});
    static SolveResult filletCorners(const QJsonObject& draft, const QJsonObject& arguments, const QJsonObject& projectionContext = {});
    // Optional drivingDiameterMm retains the inline typed diameter in the same transaction.
    static SolveResult addPointerCircle(const QJsonObject& draft, const QJsonObject& arguments, const QJsonObject& projectionContext = {});
    // Retains the first diameter endpoint and direction on an analytic circle.
    static SolveResult addTwoPointCircle(const QJsonObject& draft, const QJsonObject& arguments, const QJsonObject& projectionContext = {});
    // Retained circle-only radial offset. The constraint holds source and
    // result UUIDs; a distinct coincident center point follows source moves.
    static SolveResult addCircleOffset(const QJsonObject& draft,const QJsonObject& arguments,const QJsonObject& projectionContext = {});
    static SolveResult updateCircleOffset(const QJsonObject& draft,const QJsonObject& arguments,const QJsonObject& projectionContext = {});
    // Retained symmetry group: source line/arc entities and one authored straight axis.
    static SolveResult mirrorEntities(const QJsonObject& draft,const QJsonObject& arguments,const QJsonObject& projectionContext = {});
    static double centerlineDiameterValue(const QJsonObject& draft, const QJsonArray& entityIds, const QJsonObject& projectionContext = {});
    static SolveResult setCenterline(const QJsonObject& draft, const QJsonObject& arguments, const QJsonObject& projectionContext = {});
    static SolveResult setConstruction(const QJsonObject& draft, const QJsonObject& arguments, const QJsonObject& projectionContext = {});
    static SolveResult solveDraft(const QJsonObject& draft, const QJsonObject& projectionContext = {}, const QString& initialReferencePointId = {});
    static SolveResult addConstraint(const QJsonObject& draft, const QJsonObject& arguments, const QJsonObject& projectionContext = {});
    // Line angles use kind=angle, two entityIds, optional raySigns (+1/-1),
    // and driving valueDeg. Ray choice is independent of annotation placement.
    static SolveResult addDimension(const QJsonObject& draft, const QJsonObject& arguments,
        const QJsonObject& projectionContext = {}, const QSet<QString>& userLengthNames = {});
    static SolveResult updateDimension(const QJsonObject& draft, const QJsonObject& arguments,
        const QJsonObject& projectionContext = {}, const QSet<QString>& userLengthNames = {});
    // Re-evaluate retained local formulas by stable dimension IDs for draft,
    // document replay and inspection; absent naming metadata stays legacy.
    static QJsonObject resolvedDimensionFeature(const QJsonObject& feature);
    // Presentation intent in canonical sketch millimeters; never changes the measurement.
    static SolveResult updateAnnotation(const QJsonObject& draft, const QJsonObject& arguments, const QJsonObject& projectionContext = {});
    static SolveResult movePoint(const QJsonObject& draft, const QJsonObject& arguments, const QJsonObject& projectionContext = {});
    static QString validateParameters(const QJsonObject& parameters, QSet<QString>& authoredIds, const QSet<QString>& derivedPointIds = {});
    static QJsonObject frame(const QString& plane);
    static QJsonObject evaluationFrame(const QJsonObject& parameters, const QJsonObject& resolvedFrame = {});
    static EvaluatedSketch evaluateFeature(const QJsonObject& feature, const QJsonObject& resolvedFrame = {}, const QJsonObject& projectionContext = {});
};
