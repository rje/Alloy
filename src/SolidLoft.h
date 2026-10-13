// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"
class SolidLoft final {
public:
    static EvaluatedBody evaluate(const QJsonObject& feature,const EvaluatedDocument& prior);
};
