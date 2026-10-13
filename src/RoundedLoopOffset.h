#pragma once
#include "GeometryKernel.h"

// One retained, generated offset of the exact outer boundary of a named face.
// No solver entities or copied BRep are authored in the sketch.
class RoundedLoopOffset final {
public:
    static QString outerLoopId(const EvaluatedSupportFace& support);
    static QString validate(const QJsonObject& parameters);
    static EvaluatedSketch evaluate(const QJsonObject& feature, const EvaluatedSupportFace& support);
};
