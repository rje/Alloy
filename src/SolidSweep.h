// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"
class SolidSweep final {
public:
    static EvaluatedBody evaluate(const QJsonObject& feature,const EvaluatedDocument& prior,const QJsonObject& pathSource);
};
