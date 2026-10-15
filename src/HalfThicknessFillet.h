// SPDX-License-Identifier: MIT
#pragma once
#include "RuledCornerFillet.h"
#include <QMap>
class BRepFilletAPI_MakeFillet;

// Bounded full rounding of both rims of an exact offset sheet with four
// retained ruled walls and four retained corner rounds.
namespace HalfThicknessFillet {
std::optional<RuledCornerFillet::Result> evaluate(const EvaluatedBody& body,
    const BRepFilletAPI_MakeFillet& contours,const QMap<int,double>& radii,
    const QString& featureId);
}
