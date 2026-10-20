// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"
namespace RectangularBodyPattern {
QString bodyId(const QString& featureId,const QString& sourceBodyId,int i,int j);
QStringList generatedBodyIds(const QJsonObject& feature);
std::vector<EvaluatedBody> evaluate(const QJsonObject& feature,const EvaluatedBody& source);
}
