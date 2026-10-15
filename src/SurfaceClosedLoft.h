// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"

// The deliberately bounded no-guide, three-section open surface Loft.
class SurfaceClosedLoft final {
public:
    static QJsonObject boundaryReference(const EvaluatedDocument& prior,const QString& bodyId,const QString& edgeId);
    static EvaluatedBody evaluate(const QJsonObject& feature,const EvaluatedDocument& prior);
};
