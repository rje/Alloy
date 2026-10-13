// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"
namespace CircularPattern {
QString bodyId(const QString& featureId,const QString& sourceBodyId,int index);
std::vector<EvaluatedBody> evaluate(const QJsonObject& feature,const EvaluatedBody& source);
}
