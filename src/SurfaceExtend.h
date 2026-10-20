// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"
#include "SurfaceRuled.h"
class SurfaceExtend final {
public:
    static SurfaceRuled::Source resolveSource(const EvaluatedDocument& prior,const QJsonObject& ref);
    static void evaluate(EvaluatedBody& target,const QJsonObject& feature,const EvaluatedDocument& prior);
};
