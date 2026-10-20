// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"
namespace Interference {
// A transient exact-BRep result. Feedback is never authored or persisted.
struct Result { QJsonObject json; TopoDS_Shape feedback; std::vector<TopoDS_Shape> sources; };
QJsonObject resolve(const std::vector<EvaluatedBody>& bodies,const QJsonObject& reference);
Result evaluate(const std::vector<EvaluatedBody>& bodies,const QJsonArray& sources,const QString& units);
}
