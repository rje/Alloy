#pragma once
#include "GeometryKernel.h"

// Inward constant-thickness sharp shell, with optional retained removal face.
class SolidShell final {
public:
    static void evaluate(EvaluatedBody& body, const QJsonObject& parameters, const QString& featureId);
};
