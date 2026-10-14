#pragma once
#include "GeometryKernel.h"
#include <QJsonObject>
#include <optional>

// One isolated root-sibling as-built Slider. Definition geometry is never moved.
class AssemblySlider final {
public:
    static QJsonObject candidate(const QJsonObject& authored,const QJsonObject& arguments);
    static QJsonObject inspect(const QJsonObject& authored,const EvaluatedDocument& definitions,
        std::optional<double> transientPositionMm = {});
    static std::vector<EvaluatedBody> worldBodies(const QJsonObject& authored,const EvaluatedDocument& definitions,
        std::optional<double> transientPositionMm = {});
    static double position(const QJsonObject& authored,std::optional<double> transientPositionMm = {});
};
