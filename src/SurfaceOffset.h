// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"
class SurfaceOffset final {
public:
    static EvaluatedSupportFace resolveSource(const EvaluatedDocument& prior,const QJsonObject& ref);
    static EvaluatedBody evaluate(const QJsonObject& feature,const EvaluatedDocument& prior);
};
