// SPDX-License-Identifier: MIT
#pragma once
#include <QJsonObject>
#include <QString>
#include <vector>
#include <optional>
namespace DocumentAppend {
struct Plan {int featureCount;QString hiddenSketchId;};
// Exact one-feature append admission; no IDs-only or partial-state cache key.
std::optional<Plan> classify(const QJsonObject& current,const QJsonObject& candidate,
    const QJsonObject& resolvedCurrent,const QJsonObject& resolvedCandidate,
    const std::vector<QString>& evaluatedOrder);
}
