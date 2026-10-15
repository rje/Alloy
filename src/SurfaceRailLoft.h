// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"

// Retained endpoint-contact arc/two-line Loft and bounded symmetric
// arc/Mirror-spline-pair Loft with an actual interior rail.
class SurfaceRailLoft final {
public:
    static EvaluatedBody evaluate(const QJsonObject& feature,const EvaluatedDocument& prior);
};
