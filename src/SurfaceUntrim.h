// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"

class SurfaceUntrim final {
public:
    // Resolve one exact current Trim portion to its finite pretrim stage.
    // General surface-domain expansion and multi-Trim histories are outside this slice.
    static EvaluatedBody resolveSource(const EvaluatedDocument& prior,const QJsonObject& sourceFace,
        const QString& trimFeatureId);
    static void evaluate(EvaluatedBody& target,const QJsonObject& feature,const EvaluatedDocument& prior);
};
