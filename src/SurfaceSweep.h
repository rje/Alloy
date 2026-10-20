// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"

// Bounded retained open Line section along one analytic Line or Arc.
class SurfaceSweep final {
public:
    static EvaluatedBody evaluate(const QJsonObject& feature,const EvaluatedDocument& prior);
};
