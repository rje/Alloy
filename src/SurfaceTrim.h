// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"

struct SurfaceTrimRegion {
    QString regionId, sourceFaceId;
    bool inside = false;
    TopoDS_Face face;
    double areaMm2 = 0;
};

struct SurfaceTrimSplit {
    TopoDS_Shape splitShape;
    std::vector<SurfaceTrimRegion> regions;
    std::vector<EvaluatedSupportFace> faces;
    std::vector<EvaluatedSolidEdge> edges;
    QJsonObject projection;
};

class SurfaceTrim final {
public:
    static SurfaceTrimSplit split(const QJsonObject& feature,const EvaluatedBody& target,
        const EvaluatedDocument& prior);
    static void evaluate(EvaluatedBody& target,const QJsonObject& feature,const EvaluatedDocument& prior);
    static QJsonArray descriptors(const SurfaceTrimSplit& split);
};
