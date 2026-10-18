#pragma once
#include "GeometryKernel.h"

// Independent retained offsets of the exact outer boundary of a named face.
// No solver entities or copied BRep are authored in the sketch.
class RoundedLoopOffset final {
public:
    static QString outerLoopId(const EvaluatedSupportFace& support);
    static QString insetLoopId(const EvaluatedSupportFace& support);
    static QJsonObject deleteProfile(const QJsonObject& feature, const EvaluatedSupportFace& support,
        const QJsonArray& profileIds);
    static QString validate(const QJsonObject& parameters);
    static EvaluatedSketch evaluate(const QJsonObject& feature, const EvaluatedSupportFace& support,
        const QJsonObject& resolvedFrame = {});
};
