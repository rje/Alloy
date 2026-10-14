#include <gp_Trsf.hxx>
#pragma once
#include "SketchProfiles.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QMap>
#include <TopoDS_Shape.hxx>
#include <stdexcept>
#include <vector>

struct EvaluatedSupportEdge {
    QString edgeId, type;
    TopoDS_Edge edge;
    QJsonObject provenance;
};
struct EvaluatedSupportFace {
    QString faceId, bodyId, producerFeatureId, role;
    TopoDS_Face face;
    QJsonObject frame;
    std::vector<EvaluatedSupportEdge> boundaryEdges;
    QJsonObject provenance; // Retained generator identity, independent of current coordinates.
};

struct EvaluatedSolidEdge {
    QString edgeId;
    TopoDS_Edge edge;
    QJsonObject provenance;
};

struct EvaluatedBody {
    QString featureId, bodyId, name;
    bool visible = true;
    TopoDS_Shape shape;
    QJsonObject summary;
    std::vector<EvaluatedSupportFace> supportFaces;
    std::vector<EvaluatedSolidEdge> solidEdges;
    QString ownerId;
    QString occurrenceId, definitionId; // Populated only for retained imported assembly leaves.
};

struct EvaluatedSketch {
    QString featureId, name, plane;
    bool visible = true;
    TopoDS_Shape shape;
    QJsonObject summary;
    std::vector<EvaluatedProfile> profiles;
    QJsonObject resolvedFrame;
    QString ownerId;
    QMap<QString,TopoDS_Edge> entityEdges; // Exact solved source-entity lineage.
};

struct EvaluatedConstructionPlane {
    QString featureId, name;
    bool visible = true;
    QJsonObject frame;
    QString ownerId;
    QJsonArray displayBoundsMm;
};

struct EvaluatedDocument {
    std::vector<EvaluatedBody> bodies;
    std::vector<EvaluatedSketch> sketches;
    std::vector<EvaluatedConstructionPlane> constructionPlanes;
    std::vector<QString> featureOrder;
    QJsonObject joinBodyResults;
    // Disposable, exact source-stage tools for retained feature replication.
    // These are evaluated geometry, never a second authored representation.
    QMap<QString,std::vector<TopoDS_Shape>> cutFeatureTools;
    QMap<QString,std::vector<EvaluatedSolidEdge>> cutFeatureEdges;
    QString recompute = "full_ordered";
};

// Small OCCT boundary. Authored values and presentation objects stay elsewhere.
class GeometryKernel final {
public:
    class EvaluationError final : public std::runtime_error {
    public:
        explicit EvaluationError(const QJsonObject& diagnostics);
        const QJsonObject& diagnostics() const { return diagnostics_; }
    private:
        QJsonObject diagnostics_;
    };
    struct JoinResult {std::vector<EvaluatedBody> bodies;QJsonArray ownership;};
    static JoinResult joinResults(const QJsonObject& feature, const std::vector<EvaluatedBody>& participants,
        const std::vector<EvaluatedProfile>& profiles);
    static gp_Trsf moveTransform(const QJsonObject& parameters);
    static TopoDS_Shape primitive(const QString& type, const QJsonObject& parameters);
    static EvaluatedSupportFace resolveSketchSupport(const EvaluatedDocument& evaluation, const QJsonObject& support);
    static QJsonObject resolvePlaneSource(const EvaluatedDocument& evaluation, const QJsonObject& source);
    static QJsonObject offsetPlaneFrame(const QJsonObject& frame, double offsetMm);
    static QJsonObject midplaneFrame(const EvaluatedDocument& evaluation, const QJsonArray& faces);
    static QJsonArray supportEdgeDescriptors(const EvaluatedSupportFace& support);
    static QJsonObject projectionContext(const EvaluatedDocument& evaluation, const QJsonObject& feature);
    static EvaluatedProfile unionProfiles(const std::vector<EvaluatedProfile>& profiles);
    static double extrusionDistance(const QJsonObject& parameters, const EvaluatedProfile& profile, const EvaluatedDocument& prior);
    static TopoDS_Shape extrusion(const EvaluatedProfile& profile, double distanceMm, EvaluatedSupportFace* endCap = nullptr, std::vector<EvaluatedSolidEdge>* edges = nullptr, const QString& generatorFeatureId = {}, std::vector<EvaluatedSupportFace>* planarFaces = nullptr, double startOffsetMm = 0);
    static TopoDS_Shape booleanOperation(const QString& operation, const TopoDS_Shape& target,
        const std::vector<TopoDS_Shape>& tools, std::vector<EvaluatedSupportFace>* retainedFaces = nullptr, std::vector<EvaluatedSolidEdge>* retainedEdges = nullptr,
        const std::vector<EvaluatedSolidEdge>* cutToolEdges = nullptr, const QString& cutFeatureId = {});
    static QJsonObject summarize(const TopoDS_Shape& shape);
    static std::vector<EvaluatedBody> evaluate(const QJsonObject& authored);
    static EvaluatedDocument evaluateDocument(const QJsonObject& authored);
    static EvaluatedDocument evaluateDocument(const QJsonObject& authored,const EvaluatedDocument& prefix);
    static QJsonObject policy();
};
