// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>

namespace SolidHole {
// Builds a cutter at this occurrence against the current target. A positive
// distance stops at that inward depth; zero recomputes All from target bounds.
// Existing callers retain Countersink All; simple=true returns a flat cylinder.
TopoDS_Shape tool(const TopoDS_Shape& target,const gp_Pnt& entry,const gp_Dir& outward,
    double boreDiameterMm,double sinkDiameterMm,double includedAngleDeg,
    double* sinkDepthMm=nullptr,bool simple=false,double distanceMm=0,double recessDiameterMm=0,double recessDepthMm=0);
// Resolve the retained point and support from the evaluated history before
// this feature. Native preview uses this same authoritative cutter.
TopoDS_Shape cutter(const EvaluatedDocument& document,const QJsonObject& feature,
    const QJsonObject& sourceSketch,QJsonObject* details=nullptr);
void evaluate(EvaluatedDocument& document,const QJsonObject& feature,const QJsonObject& sourceSketch);
}
