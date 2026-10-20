// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"
namespace BodyCopy {
// Frozen OCCT geometry only: no source feature/body references or live topology.
QJsonObject capture(const EvaluatedBody& source);
QString validate(const QJsonObject& parameters);
EvaluatedBody evaluate(const QJsonObject& feature);
}
