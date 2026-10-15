// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"

// The deliberately bounded no-guide, three-section open surface Loft.
class SurfaceClosedLoft final {
public:
    static EvaluatedBody evaluate(const QJsonObject& feature,const EvaluatedDocument& prior);
};
