// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"
#include <QJsonArray>

// Exact bounded fallback for an authored Box with a complete grouped vertical/rim Fillet.
// Returns false only when the authored ancestry is outside this family. A matching
// ancestry with unrecognized evaluated geometry fails atomically.
class ClosedBlendShell final {
public:
    static bool evaluate(EvaluatedBody& body,const QJsonObject& shellParameters,
        const QString& shellFeatureId,const QJsonArray& features,int shellIndex,
        const EvaluatedDocument& evaluated);
};
