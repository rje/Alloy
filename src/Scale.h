// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"
namespace Scale {
QString validate(const QJsonObject& parameters);
EvaluatedBody evaluate(const EvaluatedDocument& prior,const QJsonObject& feature,const QJsonObject& authored);
}
