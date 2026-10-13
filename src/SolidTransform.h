// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"
namespace SolidTransform {
// Exact kernel transform history shared by retained Mirror and circular occurrences.
EvaluatedBody copy(const EvaluatedBody& source,const gp_Trsf& transform,
    const QString& featureId,const QString& bodyId,const QString& name,bool visible,
    const QByteArray& idPrefix,const QString& operation,const QString& sourceEdgeKey,bool copyGeometry = true);
}
