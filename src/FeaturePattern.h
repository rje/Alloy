// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"
class FeaturePattern final {
public:
    static void evaluate(EvaluatedBody& target,const QJsonObject& feature,const QJsonObject& seed,const EvaluatedDocument& prior);
    static void mirrorCuts(EvaluatedBody& target,const QJsonObject& feature,const EvaluatedDocument& prior);
    static void mirrorSnapGroup(EvaluatedBody& target,const QJsonObject& feature,
        const QJsonArray& features,const EvaluatedDocument& prior);
};
