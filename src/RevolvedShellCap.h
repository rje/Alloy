// SPDX-License-Identifier: MIT
#pragma once
#include "GeometryKernel.h"
#include <optional>
namespace RevolvedShellCap {
// Bounded closed-cap policy for an unmodified full circular-meridian Revolve.
// Other Shell families continue through OCCT's existing thick-solid authority.
std::optional<EvaluatedBody> evaluate(const EvaluatedBody& source,
    const EvaluatedSupportFace& selected,double thickness,const QString& featureId);
}
