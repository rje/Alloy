// SPDX-License-Identifier: MIT
#include "SolidThreadGeometry.h"
#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepClass3d_SolidClassifier.hxx>
#include <GProp_GProps.hxx>
#include <StlAPI_Writer.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <chrono>
#include <cstdlib>

static double volume(const TopoDS_Shape& s) { GProp_GProps p; BRepGProp::VolumeProperties(s,p); return p.Mass(); }
static void require(bool c,const char* m) { if(!c) throw std::runtime_error(m); }
int main(int argc,char** argv) { try {
    const std::filesystem::path output=argc>1?argv[1]:"thread-spike";
    std::filesystem::create_directories(output);
    for(const auto& [name,radius,pitch,height,start,length,right]:{
        std::tuple{"m8-partial",4.,1.25,16.,2.,10.,true},
        std::tuple{"m8-full",4.,1.25,16.,0.,16.,true},
        std::tuple{"m8-height-edit",4.,1.25,20.,2.,10.,true},
        std::tuple{"m10-size-edit",5.,1.5,16.,2.,10.,true},
        std::tuple{"m8-left",4.,1.25,16.,2.,10.,false}}) {
        if(argc>2 && std::string(argv[2])!=name)continue;
        std::cerr<<"building "<<name<<std::endl;
        const auto source=BRepPrimAPI_MakeCylinder(radius,height).Shape();
        const double before=volume(source);
        SolidThreadGeometry::Parameters p; p.majorRadius=radius;p.pitch=pitch;p.start=start;p.length=length;p.rightHand=right;
        const auto shape=SolidThreadGeometry::cutExternal(source,p);
        require(std::abs(volume(source)-before)<1e-9,"Source mutated");
        require(BRepCheck_Analyzer(shape).IsValid(),"Invalid BRep");
        int solids=0;TopoDS_Solid solid; for(TopExp_Explorer it(shape,TopAbs_SOLID);it.More();it.Next()){++solids;solid=TopoDS::Solid(it.Current());}
        require(solids==1,"Wrong solid count");
        // The phase-zero groove center is on +X at z=start+n*pitch.
        // Quarter-turn samples independently establish handedness and axial rise.
        const double z=start+3*pitch; const double r=radius-0.15;
        const double sign=right?1.:-1.;
        BRepClass3d_SolidClassifier groove(solid,gp_Pnt(0,sign*r,z+pitch/4),1e-6);
        BRepClass3d_SolidClassifier crest(solid,gp_Pnt(0,sign*r,z+pitch*0.75),1e-6);
        require(groove.State()==TopAbs_OUT,"Expected helical groove missing");
        require(crest.State()==TopAbs_IN,"Expected helical crest missing");
        const auto meshStart=std::chrono::steady_clock::now();
        BRepMesh_IncrementalMesh mesh(shape,0.025,false,0.12,true); require(mesh.IsDone(),"Mesh failed");
        if(std::getenv("ALLOY_THREAD_TIMINGS"))std::cerr<<"thread.phase stl_mesh ms="<<std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-meshStart).count()<<'\n';
        const auto exportStart=std::chrono::steady_clock::now();
        StlAPI_Writer writer;writer.ASCIIMode()=false;
        require(writer.Write(shape,(output/(std::string(name)+".stl")).c_str()),"STL export failed");
        if(std::getenv("ALLOY_THREAD_TIMINGS"))std::cerr<<"thread.phase stl_write ms="<<std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-exportStart).count()<<'\n';
        std::cout<<name<<" solids="<<solids<<" valid=1 volume="<<volume(shape)<<" sourceVolume="<<before<<" removed="<<before-volume(shape)<<std::endl;
    }
    return 0;
} catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;} }
