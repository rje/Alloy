// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"
class SolidFaceOffset final {
public:
    static void evaluate(EvaluatedBody& body,const QJsonObject& parameters,const QString& featureId);
};
