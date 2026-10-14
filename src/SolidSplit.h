#pragma once
#include "GeometryKernel.h"
#include <utility>

namespace SolidSplit {
QString positiveBodyId(const QString& featureId,const QString& sourceBodyId);
std::pair<EvaluatedBody,EvaluatedBody> evaluate(const EvaluatedBody& source,
    const QJsonObject& planeFrame,const QString& featureId,const QString& positiveId,
    const QString& name);
}
