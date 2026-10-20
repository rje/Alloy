// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"
class SurfacePatch final {
public:
    static EvaluatedBody evaluate(const QJsonObject& feature,const EvaluatedDocument& prior);
};
