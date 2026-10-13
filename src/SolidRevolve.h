// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"
class SolidRevolve final {
public:
    static EvaluatedBody evaluate(const QJsonObject& feature,const EvaluatedDocument& prior,const QJsonObject& source);
};
