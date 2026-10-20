// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"
namespace DatumPoint {
QString validate(const QJsonObject& parameters);
QJsonObject evaluate(const EvaluatedDocument& evaluation,const QJsonObject& parameters,const QJsonArray& features);
}
