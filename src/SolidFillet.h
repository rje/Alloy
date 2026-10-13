#pragma once
#include "GeometryKernel.h"
class BRepPrimAPI_MakePrism;
class BRepAlgoAPI_BooleanOperation;
// Authored extrusion provenance, carried by actual OCCT history only.
namespace SolidFillet {
std::vector<EvaluatedSolidEdge> prismEdges(BRepPrimAPI_MakePrism& prism,
    const EvaluatedProfile& profile, const QString& generatorFeatureId);
std::vector<EvaluatedSolidEdge> transferEdges(BRepAlgoAPI_BooleanOperation& operation,
    const std::vector<EvaluatedSolidEdge>& inputs, const TopoDS_Shape& resultSolid);
QJsonArray descriptors(const std::vector<EvaluatedSolidEdge>& edges);
void evaluate(EvaluatedBody& body, const QJsonObject& parameters, const QString& featureId);
}
