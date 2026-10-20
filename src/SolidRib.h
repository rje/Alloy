// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"
class SolidRib final {
public:
    static void evaluate(const QJsonObject& feature,EvaluatedDocument& prior);
};
