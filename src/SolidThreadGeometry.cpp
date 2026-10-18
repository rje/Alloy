// SPDX-License-Identifier: MIT
#include "SolidThreadGeometry.h"
#include <BRepAlgoAPI_Common.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp.hxx>
#include <BRepLib.hxx>
#include <BRepOffsetAPI_MakePipeShell.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <GProp_GProps.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <Geom_CylindricalSurface.hxx>
#include <Geom2d_Line.hxx>
#include <Standard_Failure.hxx>
#include <TopExp_Explorer.hxx>
#include <gp_Trsf.hxx>
#include <cmath>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace {
constexpr double pi = 3.14159265358979323846;
void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
double volume(const TopoDS_Shape& shape) { GProp_GProps p; BRepGProp::VolumeProperties(shape,p); return p.Mass(); }
}
TopoDS_Shape SolidThreadGeometry::cutExternal(const TopoDS_Shape& source, const Parameters& p, const std::function<void(BRepAlgoAPI_Cut&)>& inspectOperation)
{
    require(!source.IsNull(),"Thread source is null");
    require(std::isfinite(p.majorRadius) && std::isfinite(p.pitch) && std::isfinite(p.start) && std::isfinite(p.length)
        && p.majorRadius>0 && p.pitch>0 && p.pitch<p.majorRadius && p.start>=0 && p.length>0,
        "Invalid external thread dimensions");
    // Avoid unbounded kernel work from malformed authored parameters.
    require(p.length/p.pitch<=100,"Thread exceeds 100-turn geometry limit");
    const bool timing=std::getenv("ALLOY_THREAD_TIMINGS")!=nullptr;
    auto phaseStart=std::chrono::steady_clock::now();
    const auto mark=[&](const char* name){if(timing){const auto now=std::chrono::steady_clock::now();std::cerr<<"thread.phase "<<name<<" ms="<<std::chrono::duration<double,std::milli>(now-phaseStart).count()<<'\n';phaseStart=now;}};
    try {
        // Nominal 60-degree truncated V groove. A true circular helix's Frenet
        // frame rotates rigidly about Z. WithCorrection=false retains this
        // intentionally oblique RADIAL/AXIAL profile, rather than rotating it
        // into the helix normal plane and changing its axial pitch dimensions.
        const double depth=5*std::sqrt(3.0)*p.pitch/16;
        const double rootHalf=p.pitch/8;
        const double overrun=p.pitch/5;
        const double outsideHalf=rootHalf+(depth+overrun)/std::sqrt(3.0);
        const double z0=p.start-p.pitch;
        // Rounded external root: tangent depth 5H/8, root radius H/6,
        // deepest point 17H/24; H=sqrt(3)P/2. The crest remains P/8 wide.
        const gp_Pnt lower(p.majorRadius-depth,0,z0-rootHalf);
        const gp_Pnt upper(p.majorRadius-depth,0,z0+rootHalf);
        const gp_Pnt outsideLower(p.majorRadius+overrun,0,z0-outsideHalf);
        const gp_Pnt outsideUpper(p.majorRadius+overrun,0,z0+outsideHalf);
        const gp_Pnt bottom(p.majorRadius-17*std::sqrt(3.0)*p.pitch/48,0,z0);
        BRepBuilderAPI_MakeWire profile;
        profile.Add(BRepBuilderAPI_MakeEdge(lower,outsideLower).Edge());
        profile.Add(BRepBuilderAPI_MakeEdge(outsideLower,outsideUpper).Edge());
        profile.Add(BRepBuilderAPI_MakeEdge(outsideUpper,upper).Edge());
        profile.Add(BRepBuilderAPI_MakeEdge(GC_MakeArcOfCircle(upper,bottom,lower).Value()).Edge());
        require(profile.IsDone(),"Thread profile failed");
        const double sign=p.rightHand?1.0:-1.0;
        Handle(Geom_CylindricalSurface) cylinder=new Geom_CylindricalSurface(gp_Ax3(gp_Pnt(0,0,z0),gp_Dir(0,0,1)),p.majorRadius);
        const gp_Dir2d slope(sign,p.pitch/(2*pi));
        Handle(Geom2d_Line) line=new Geom2d_Line(gp_Pnt2d(0,0),slope);
        const double span=(p.length+2*p.pitch)/slope.Y();
        BRepBuilderAPI_MakeEdge edge(line,cylinder,0,span);
        require(edge.IsDone(),"Thread helix edge failed");
        TopoDS_Wire spine=BRepBuilderAPI_MakeWire(edge.Edge()).Wire();
        require(BRepLib::BuildCurves3d(spine),"Thread helix 3D curve failed");
        mark("profile_helix");
        BRepOffsetAPI_MakePipeShell sweep(spine);
        sweep.SetMode(true);
        sweep.SetTolerance(1e-6,1e-6,1e-4);
        sweep.Add(profile.Wire(),false,false);
        sweep.Build(); require(sweep.IsDone(),"Thread groove sweep failed");
        require(sweep.MakeSolid(),"Thread groove solid failed");
        mark("pipe_build_solid");
        require(BRepCheck_Analyzer(sweep.Shape()).IsValid(),"Thread groove sweep invalid");
        mark("sweep_validity");
        // Clip overrun to exact axial stations; the finite ends are deliberately
        // abrupt and do not claim a lead-in or chamfer.
        const auto clip=BRepPrimAPI_MakeCylinder(gp_Ax2(gp_Pnt(0,0,p.start),gp_Dir(0,0,1)),p.majorRadius+2*overrun,p.length).Shape();
        TopTools_ListOfShape sweepArguments,clipTools;sweepArguments.Append(sweep.Shape());clipTools.Append(clip);
        BRepAlgoAPI_Common common;common.SetArguments(sweepArguments);common.SetTools(clipTools);common.SetNonDestructive(true);common.Build();
        require(common.IsDone()&&!common.Shape().IsNull(),"Thread end clipping failed");
        mark("clip_common");
        gp_Trsf placement; placement.SetDisplacement(gp_Ax3(gp::XOY()),gp_Ax3(p.axis));
        const auto cutter=BRepBuilderAPI_Transform(common.Shape(),placement,true).Shape();
        TopTools_ListOfShape sourceArguments,cutterTools;sourceArguments.Append(source);cutterTools.Append(cutter);
        BRepAlgoAPI_Cut cut;cut.SetArguments(sourceArguments);cut.SetTools(cutterTools);cut.SetNonDestructive(true);cut.Build();
        require(cut.IsDone()&&!cut.Shape().IsNull(),"Thread Boolean cut failed");
        mark("placement_boolean_cut");
        const auto result=cut.Shape();
        int solids=0; for(TopExp_Explorer it(result,TopAbs_SOLID);it.More();it.Next()) ++solids;
        require(solids==1 && BRepCheck_Analyzer(result).IsValid(),"Thread output is not one valid solid");
        require(volume(result)>0 && volume(result)<volume(source)-1e-6,"Thread cut removed no material");
        mark("output_validity_volume");
        if(inspectOperation)inspectOperation(cut);
        return result;
    } catch(const Standard_Failure& error) { throw std::runtime_error(std::string("Thread kernel failure: ")+error.GetMessageString()); }
}
