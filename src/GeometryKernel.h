#pragma once
#include "SketchProfiles.h"
#include <QJsonObject>
#include <QJsonArray>
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
};

struct EvaluatedBody {
    QString featureId, bodyId, name;
    bool visible = true;
    TopoDS_Shape shape;
    QJsonObject summary;
    std::vector<EvaluatedSupportFace> supportFaces;
};

struct EvaluatedSketch {
    QString featureId, name, plane;
    bool visible = true;
    TopoDS_Shape shape;
    QJsonObject summary;
    std::vector<EvaluatedProfile> profiles;
    QJsonObject resolvedFrame;
};

struct EvaluatedDocument {
    std::vector<EvaluatedBody> bodies;
    std::vector<EvaluatedSketch> sketches;
    std::vector<QString> featureOrder;
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
    static TopoDS_Shape primitive(const QString& type, const QJsonObject& parameters);
    static EvaluatedSupportFace resolveSketchSupport(const EvaluatedDocument& evaluation, const QJsonObject& support);
    static QJsonArray supportEdgeDescriptors(const EvaluatedSupportFace& support);
    static QJsonObject projectionContext(const EvaluatedDocument& evaluation, const QJsonObject& feature);
    static EvaluatedProfile unionProfiles(const std::vector<EvaluatedProfile>& profiles);
    static TopoDS_Shape extrusion(const EvaluatedProfile& profile, double distanceMm, EvaluatedSupportFace* endCap = nullptr);
    static TopoDS_Shape booleanOperation(const QString& operation, const TopoDS_Shape& target,
        const std::vector<TopoDS_Shape>& tools);
    static QJsonObject summarize(const TopoDS_Shape& shape);
    static std::vector<EvaluatedBody> evaluate(const QJsonObject& authored);
    static EvaluatedDocument evaluateDocument(const QJsonObject& authored);
    static QJsonObject policy();
};
