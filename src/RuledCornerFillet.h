// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"
#include <QMap>
#include <optional>

// A bounded exact construction for the four ruled sidewalls of a retained
// Thicken. Unsupported topology returns no result; callers keep their normal
// fillet behavior and refuse atomically if neither construction is valid.
class RuledCornerFillet final {
public:
    struct Result {
        TopoDS_Shape shape;
        std::vector<EvaluatedSupportFace> supportFaces;
        std::vector<EvaluatedSolidEdge> solidEdges;
    };
    static std::optional<Result> evaluate(const EvaluatedBody& body,
        const QMap<QString,double>& seedRadii,const QString& featureId);
};
