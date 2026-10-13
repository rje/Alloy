// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"
class FeaturePattern final {
public:
    static void evaluate(EvaluatedBody& target,const QJsonObject& feature,const QJsonObject& seed,const EvaluatedDocument& prior);
};
