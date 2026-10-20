// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"
#include <TopoDS_Wire.hxx>
#include <functional>
struct RetainedSweepPath {
    TopoDS_Wire wire;
    gp_Pnt origin, finish;
    gp_Vec initialTangent, lastTangent;
    double length=0;
    QJsonArray oriented;
};
RetainedSweepPath retainedSweepPath(const QJsonObject& parameters,const EvaluatedSketch& sketch,const QJsonObject& pathSource,const std::function<void(const QString&,const QString&,QJsonObject)>& reject);
