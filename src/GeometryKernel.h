#pragma once
#include <QJsonObject>
#include <TopoDS_Shape.hxx>
#include <vector>

struct EvaluatedBody {
    QString featureId, bodyId, name;
    bool visible = true;
    TopoDS_Shape shape;
    QJsonObject summary;
};

// Small OCCT boundary. Authored values and presentation objects stay elsewhere.
class GeometryKernel final {
public:
    static TopoDS_Shape primitive(const QString& type, const QJsonObject& parameters);
    static QJsonObject summarize(const TopoDS_Shape& shape);
    static std::vector<EvaluatedBody> evaluate(const QJsonObject& authored);
    static QJsonObject policy();
};
