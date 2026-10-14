// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"
#include <BRepBuilderAPI_MakeShape.hxx>
#include <TopoDS_Face.hxx>

namespace JoinRootEdges {
std::vector<EvaluatedSolidEdge> publish(BRepBuilderAPI_MakeShape& operation,
    const std::vector<EvaluatedSupportFace>& targetFaces,
    const std::vector<EvaluatedJoinToolBase>& toolBases,
    const std::vector<EvaluatedSolidEdge>& toolEdges,
    const TopoDS_Shape& result,
    const std::vector<EvaluatedSupportFace>& retainedFaces,
    const QString& featureId);
}
