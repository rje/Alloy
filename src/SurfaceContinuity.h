// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"
// Pure current B-Rep inspection. Inputs are two exact retained face references
// and one retained edge ID; no topology healing or document mutation.
namespace SurfaceContinuity {
QJsonObject evaluate(const std::vector<EvaluatedBody>& bodies,const QJsonArray& faces,
    const QString& edgeId,const QString& displayUnits="mm");
}
