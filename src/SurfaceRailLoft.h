// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"

// Endpoint-contact, arc-to-two-line open-sheet Loft with one actual boundary rail.
class SurfaceRailLoft final {
public:
    static EvaluatedBody evaluate(const QJsonObject& feature,const EvaluatedDocument& prior);
};
