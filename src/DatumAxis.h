// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"
namespace DatumAxis {
QString validate(const QJsonObject& source);
QJsonObject evaluate(const EvaluatedDocument& evaluation,const QJsonObject& source,const QJsonArray& features);
}
