// SPDX-License-Identifier: MIT
#pragma once
#include <TopoDS_Shape.hxx>
#include <gp_Ax2.hxx>
#include <functional>
class BRepAlgoAPI_Cut;

namespace SolidThreadGeometry {
// A nominal geometric profile; no tolerance class or mating-fit certification.
struct Parameters {
    gp_Ax2 axis;
    double majorRadius = 4.0;
    double pitch = 1.25;
    double start = 0.0;
    double length = 12.0;
    bool rightHand = true;
};
// Detached, non-destructive helical groove cut. Throws on invalid input/output.
TopoDS_Shape cutExternal(const TopoDS_Shape& source, const Parameters& parameters,
    const std::function<void(BRepAlgoAPI_Cut&)>& inspectOperation = {});
}
