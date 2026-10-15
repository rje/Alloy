// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"

class SurfaceExtrude final {
public:
    static QString profileBodyId(const QString& featureId,const QString& sketchId,const QJsonArray& entityIds);
    static QStringList resultBodyIds(const QJsonObject& feature);
    // A retained-ID maximal open chain; throws on branches/closed or unavailable topology.
    static QJsonArray openChain(const EvaluatedSketch& sketch,const QString& entityId);
    static std::vector<EvaluatedBody> evaluateResults(const QJsonObject& feature,const EvaluatedDocument& prior);
    static EvaluatedBody evaluate(const QJsonObject& feature,const EvaluatedDocument& prior);
};
