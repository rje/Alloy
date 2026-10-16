// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"
#include <TopoDS_Solid.hxx>

struct BoundaryFillCell {
    QString cellId;
    QJsonObject reference;
    TopoDS_Solid shape;
    std::vector<EvaluatedSupportFace> supportFaces;
    std::vector<EvaluatedSolidEdge> solidEdges;
    QJsonObject summary;
    QJsonArray selectionPointMm; // Classifier-qualified display point, never identity.
};
struct BoundaryFillPartition {
    QString featureId, bodyId;
    QJsonArray toolBodyIds, toolReferences;
    std::vector<BoundaryFillCell> cells;
    QJsonObject diagnostics;
};
class BoundaryFill final {
public:
    static BoundaryFillPartition partition(const QJsonObject& feature,const EvaluatedDocument& prior);
    static EvaluatedBody applyCell(const QJsonObject& feature,const BoundaryFillPartition& partition);
    static EvaluatedBody evaluate(const QJsonObject& feature,const EvaluatedDocument& prior);
    static QJsonArray descriptors(const BoundaryFillPartition& partition);
};
