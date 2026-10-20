// SPDX-License-Identifier: MIT
#include "ZebraPresentation.h"
#include <AIS_ColoredDrawer.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <Prs3d_ShadingAspect.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <iostream>
#include <stdexcept>
namespace {void check(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}}
int main(){try{
    Handle(AIS_ColoredShape) model=new AIS_ColoredShape(BRepPrimAPI_MakeBox(40,30,10).Shape());
    TopExp_Explorer faces(model->Shape(),TopAbs_FACE);const auto selected=TopoDS::Face(faces.Current());faces.Next();const auto other=TopoDS::Face(faces.Current());
    model->SetColor(Quantity_Color(.7,.6,.3,Quantity_TOC_RGB));
    const auto ordinary=model->Attributes()->ShadingAspect()->Aspect();
    const auto color=ordinary->InteriorColor();
    ZebraPresentation::apply(model,selected);
    const auto custom=model->CustomAspectsMap().Find(selected);
    check(custom->ShadingAspect()->Aspect()!=ordinary,"face fill must own its aspect");
    check(custom->ShadingAspect()->Aspect()->ShaderProgram()==ZebraPresentation::shader(),"face shader missing");
    check(ordinary->ShaderProgram().IsNull(),"ordinary fill shader changed");
    check(model->CustomAspectsMap().Size()==1 && !model->CustomAspectsMap().IsBound(other),"shader leaked to adjacent face");
    check(ZebraPresentation::shader()==ZebraPresentation::shader(),"shader not cached");
    ZebraPresentation::remove(model,selected);
    check(model->CustomAspectsMap().IsEmpty(),"off must restore linked material");
    check(ordinary->ShaderProgram().IsNull() && ordinary->InteriorColor()==color,"ordinary material changed");
    std::cout<<"zebra face-specific fill shader ownership, adjacent material and off restoration passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
