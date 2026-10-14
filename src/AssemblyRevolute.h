// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"
#include <QJsonObject>
#include <optional>

// One retained as-built circular-origin Revolute on root-level leaf occurrences.
// Definition shapes remain at rest; the replaceable assembly solver supplies
// occurrence presentation poses.
class AssemblyRevolute final {
public:
    static QJsonObject candidate(const QJsonObject& authored,const EvaluatedDocument& definitions,
        const QJsonObject& arguments);
    static QJsonObject inspect(const QJsonObject& authored,const EvaluatedDocument& definitions,
        std::optional<double> transientAngleDeg = {});
    static std::vector<EvaluatedBody> worldBodies(const QJsonObject& authored,const EvaluatedDocument& definitions,
        std::optional<double> transientAngleDeg = {});
};
