// SPDX-License-Identifier: MIT
#pragma once
#include <AIS_ColoredShape.hxx>
#include <Graphic3d_ShaderProgram.hxx>
#include <TopoDS_Face.hxx>

// Original procedural display; geometry and selection owners stay with OCCT.
namespace ZebraPresentation {
Handle(Graphic3d_ShaderProgram) shader();
void apply(const Handle(AIS_ColoredShape)& presentation,const TopoDS_Face& face);
void remove(const Handle(AIS_ColoredShape)& presentation,const TopoDS_Face& face);
}
