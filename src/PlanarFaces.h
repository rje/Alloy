// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"
class BRepBuilderAPI_MakeShape;
class BRepPrimAPI_MakePrism;
namespace PlanarFaces {
// Only unique kernel descendants are admissible; never select by proximity.
std::vector<EvaluatedSupportFace> transfer(BRepBuilderAPI_MakeShape& operation,
    const std::vector<EvaluatedSupportFace>& sources, const TopoDS_Shape& result,
    bool retainModifiedStraight = false);
std::vector<EvaluatedSupportFace> prismSides(BRepPrimAPI_MakePrism& prism,
    const EvaluatedProfile& profile, const QString& generator, double distance);
// Update current endpoints only for one straight descendant of a straight
// boundary. The source direction fixes start/end roles even if OCCT reverses it.
bool updateStraightBoundary(const TopoDS_Edge& source, const TopoDS_Edge& next,
    QJsonObject& provenance);
QJsonArray descriptors(const std::vector<EvaluatedSupportFace>& faces);
}
