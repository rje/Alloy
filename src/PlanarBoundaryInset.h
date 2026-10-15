// SPDX-License-Identifier: MIT
#pragma once

#include "GeometryKernel.h"
#include <QStringList>

// Exact inward erosion of one retained analytic planar face boundary.
// This is geometry authority only; no sketch feature or UI is authored here.
class PlanarBoundaryInset final {
public:
    struct Result {
        TopoDS_Face face;
        std::vector<EvaluatedSupportEdge> boundaryEdges;
        QStringList vanishedSourceEdgeIds;
        QJsonObject summary;
    };
    // insetFeatureId namespaces derived edges when multiple retained insets
    // use the same support; it must remain stable across source edits.
    static Result evaluate(const EvaluatedSupportFace& support,double distanceMm,
        const QString& insetFeatureId);
};
