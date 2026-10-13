// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"

namespace SolidSphere {
// Face-local authored placement, resolved against the actual trimmed face.
TopoDS_Shape tool(const EvaluatedDocument& document, const QJsonObject& parameters,
                  QJsonArray* centerMm = nullptr);
void evaluate(EvaluatedDocument& document, const QJsonObject& feature);
}
