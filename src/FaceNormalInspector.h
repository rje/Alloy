// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"
// Pure evaluated B-Rep inspection; no authored document, tessellation or history.
namespace FaceNormalInspector {
QJsonObject sample(const TopoDS_Face& face);
QJsonObject evaluate(const std::vector<EvaluatedBody>& bodies,const QJsonObject& source);
}
