// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"
class SurfaceRuled final {
public:
    struct Source { EvaluatedSupportFace face; EvaluatedSupportEdge edge; };
    static Source resolveSource(const EvaluatedDocument& prior,const QJsonObject& ref);
    static EvaluatedBody evaluate(const QJsonObject& feature,const EvaluatedDocument& prior);
};
