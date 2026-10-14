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
void retainLinearCutBoundaries(BRepBuilderAPI_MakeShape& operation,
    const std::vector<EvaluatedSupportFace>& sources,
    const std::vector<EvaluatedSolidEdge>& toolEdges,
    std::vector<EvaluatedSupportFace>& retained, const QString& featureId);
void retainLinearCutSides(BRepBuilderAPI_MakeShape& operation,
    const std::vector<EvaluatedSupportFace>& toolFaces,
    const std::vector<EvaluatedSolidEdge>& toolEdges,
    const TopoDS_Shape& result,std::vector<EvaluatedSupportFace>& retained,
    const QString& featureId);
// Promote only uniquely traced current cut boundaries on one retained target
// face to the solid-edge catalog used by Fillet.
std::vector<EvaluatedSolidEdge> solidLinearCutRims(const std::vector<EvaluatedSupportFace>& faces,
    const std::vector<EvaluatedSolidEdge>& toolEdges,const TopoDS_Shape& result,const QString& featureId);
QJsonArray descriptors(const std::vector<EvaluatedSupportFace>& faces);
}
