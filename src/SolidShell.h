#pragma once
#include "GeometryKernel.h"

// One retained planar face, inward constant-thickness sharp shell.
class SolidShell final {
public:
    static void evaluate(EvaluatedBody& body, const QJsonObject& parameters, const QString& featureId);
};
