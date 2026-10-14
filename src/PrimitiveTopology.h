// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"

namespace PrimitiveTopology {
// Name the two complete circular rims by authored bottom/top semantics.
void cylinderRims(const TopoDS_Shape& shape, const QJsonObject& parameters,
    const QString& featureId, std::vector<EvaluatedSolidEdge>& edges);
// Name only unique, actual box faces and edges. An ambiguous kernel result
// fails evaluation instead of assigning a reference by traversal order.
void box(const TopoDS_Shape& shape, const QJsonObject& parameters,
    const QString& featureId, const QString& bodyId,
    std::vector<EvaluatedSupportFace>& faces,
    std::vector<EvaluatedSolidEdge>& edges);
}
