#pragma once
#include "SketchProfiles.h"
#include <QJsonObject>
#include <TopoDS_Shape.hxx>
#include <stdexcept>
#include <vector>

struct EvaluatedBody {
    QString featureId, bodyId, name;
    bool visible = true;
    TopoDS_Shape shape;
    QJsonObject summary;
};

struct EvaluatedSketch {
    QString featureId, name, plane;
    bool visible = true;
    TopoDS_Shape shape;
    QJsonObject summary;
    std::vector<EvaluatedProfile> profiles;
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
    static EvaluatedProfile unionProfiles(const std::vector<EvaluatedProfile>& profiles);
    static TopoDS_Shape extrusion(const EvaluatedProfile& profile, double distanceMm);
    static TopoDS_Shape booleanOperation(const QString& operation, const TopoDS_Shape& target,
        const std::vector<TopoDS_Shape>& tools);
    static QJsonObject summarize(const TopoDS_Shape& shape);
    static std::vector<EvaluatedBody> evaluate(const QJsonObject& authored);
    static EvaluatedDocument evaluateDocument(const QJsonObject& authored);
    static QJsonObject policy();
};
