// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"
#include <QJsonObject>
#include <QString>
// Bounded retained Revolute/Cylindrical source graph, separate from legacy Slider.
// Containers are traversal-only. Every connected island has one actual Pin or
// reported temporary rest-pose gauge; all poses derive from the shared solver.
class AssemblyGraph final {
public:
    static bool isJointGraph(const QJsonObject& authored);
    static bool participantLeaf(const QJsonObject& authored,const QString& occurrenceId);
    static bool identityLeaf(const QJsonObject& authored,const QString& occurrenceId);
    static QString validateRecords(const QJsonObject& authored);
    static QJsonObject revoluteCandidate(const QJsonObject& authored,const EvaluatedDocument& definitions,const QJsonObject& arguments);
    static QJsonObject cylindricalCandidate(const QJsonObject& authored,const EvaluatedDocument& definitions,const QJsonObject& arguments);
    // Override: {jointId,angleDeg} or {jointId,rotationDeg,slideMm}.
    static QJsonObject inspect(const QJsonObject& authored,const EvaluatedDocument& definitions,const QJsonObject& coordinateOverride={});
    static std::vector<EvaluatedBody> worldBodies(const QJsonObject& authored,const EvaluatedDocument& definitions,const QJsonObject& coordinateOverride={});
};
