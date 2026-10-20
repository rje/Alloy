// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"

// Bounded retained ordinary line / coplanar construction-line revolution.
class SurfaceRevolve final {
public:
    static EvaluatedBody evaluate(const QJsonObject& feature,const EvaluatedDocument& prior);
};
