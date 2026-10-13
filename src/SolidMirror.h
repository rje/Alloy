#pragma once
#include "GeometryKernel.h"

class SolidMirror final {
public:
    static EvaluatedBody evaluate(const EvaluatedBody& source, const EvaluatedSupportFace& plane,
        const QString& featureId, const QString& bodyId, const QString& name, bool visible);
};
