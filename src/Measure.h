// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"
// Pure inspection of current evaluated topology. No document or history access.
QJsonObject measureGeometry(const EvaluatedDocument& evaluation,
    const std::vector<EvaluatedBody>& bodies, const QJsonArray& sources, const QString& units);
