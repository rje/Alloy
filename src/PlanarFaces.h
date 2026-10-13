// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"
class BRepBuilderAPI_MakeShape;
class BRepPrimAPI_MakePrism;
namespace PlanarFaces {
// Only unique kernel descendants are admissible; never select by proximity.
std::vector<EvaluatedSupportFace> transfer(BRepBuilderAPI_MakeShape& operation,
    const std::vector<EvaluatedSupportFace>& sources, const TopoDS_Shape& result);
std::vector<EvaluatedSupportFace> prismSides(BRepPrimAPI_MakePrism& prism,
    const EvaluatedProfile& profile, const QString& generator, double distance);
QJsonArray descriptors(const std::vector<EvaluatedSupportFace>& faces);
}
