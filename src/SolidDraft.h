// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"
class SolidDraft final {
public:
    static void evaluate(EvaluatedDocument& document,const QJsonObject& feature);
};
