// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>

namespace SolidHole {
// Builds a through cutter at this occurrence against the current target. The
// caller supplies the retained entry and outward support direction; the reach
// is recomputed from target bounds on every call, including pattern instances.
TopoDS_Shape tool(const TopoDS_Shape& target,const gp_Pnt& entry,const gp_Dir& outward,
    double boreDiameterMm,double sinkDiameterMm,double includedAngleDeg,
    double* sinkDepthMm=nullptr);
// Resolve the retained point and support from the evaluated history before
// this feature. Native preview uses this same authoritative cutter.
TopoDS_Shape cutter(const EvaluatedDocument& document,const QJsonObject& feature,
    const QJsonObject& sourceSketch,QJsonObject* details=nullptr);
void evaluate(EvaluatedDocument& document,const QJsonObject& feature,const QJsonObject& sourceSketch);
}
