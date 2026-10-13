#pragma once
#include "GeometryKernel.h"
class BRepPrimAPI_MakePrism;
class BRepBuilderAPI_MakeShape;
// Authored extrusion provenance, carried by actual OCCT history only.
namespace SolidFillet {
std::vector<EvaluatedSolidEdge> prismEdges(BRepPrimAPI_MakePrism& prism,
    const EvaluatedProfile& profile, const QString& generatorFeatureId);
std::vector<EvaluatedSolidEdge> transferEdges(BRepBuilderAPI_MakeShape& operation,
    const std::vector<EvaluatedSolidEdge>& inputs, const TopoDS_Shape& resultSolid,
    bool straightModifiedOnly = false);
QJsonArray descriptors(const std::vector<EvaluatedSolidEdge>& edges);
void evaluate(EvaluatedBody& body, const QJsonObject& parameters, const QString& featureId);
}
