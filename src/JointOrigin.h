// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>
namespace JointOrigin {
struct Resolved {gp_Pnt center;gp_Dir axis;double radiusMm;TopoDS_Face face;};
// Definition-local reference validation/resolution. Pair IDs identify exactly
// one current finite cylinder face; no topology index or nearest fallback.
QString validateReference(const QJsonObject& position);
Resolved resolve(const EvaluatedBody& body,const QJsonObject& position);
// UI geometry-only helpers accept exact geometry from the same worldBody.
// position includes occurrenceId; center/axis are in that body's world frame.
QJsonObject cylindricalFaceCandidate(const EvaluatedBody& worldBody,const TopoDS_Face& exactWorldFace);
QJsonObject circleCandidate(const EvaluatedBody& worldBody,const QString& retainedEdgeId);
std::vector<QJsonObject> cylindricalCandidates(const EvaluatedBody& body);
}
