// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"

class SolidThicken {
public:
    static EvaluatedBody evaluate(const QJsonObject& feature,const EvaluatedBody& source);
};
