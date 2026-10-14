// SPDX-License-Identifier: MIT
#include "CadDocument.h"
#include <QCoreApplication>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <GeomAbs_CurveType.hxx>
#include <cmath>
#include <iostream>
#include <stdexcept>
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
bool hasEye(const TopoDS_Shape& shape,double x,double z,double radius){
    for(TopExp_Explorer explorer(shape,TopAbs_EDGE);explorer.More();explorer.Next()){
        BRepAdaptor_Curve curve(TopoDS::Edge(explorer.Current()));if(curve.GetType()!=GeomAbs_Circle)continue;
        const auto circle=curve.Circle();const auto center=circle.Location();const auto axis=circle.Axis().Direction();
        if(std::abs(center.X()-x)<1e-5 && std::abs(center.Z()-z)<1e-5 && std::abs(circle.Radius()-radius)<1e-5
            && std::abs(axis.Y())>0.999)return true;
    }
    return false;
}
bool hasPistonWall(const TopoDS_Shape& shape,double radius){
    for(TopExp_Explorer explorer(shape,TopAbs_FACE);explorer.More();explorer.Next()){
        BRepAdaptor_Surface surface(TopoDS::Face(explorer.Current()));if(surface.GetType()!=GeomAbs_Cylinder)continue;
        const auto cylinder=surface.Cylinder();const auto axis=cylinder.Axis();
        if(std::abs(cylinder.Radius()-radius)<1e-5 && std::abs(axis.Location().X()-90)<1e-5
            && std::abs(axis.Location().Y())<1e-5 && std::abs(axis.Direction().Z())>0.999)return true;
    }
    return false;
}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    check(argc==2,"Usage: piston_fixture_geometry <authored .alloy>");CadDocument document;
    const auto error=document.open(QString::fromLocal8Bit(argv[1]));check(error.isEmpty(),"Open fixture");
    const auto bodies=document.bodies();check(bodies.size()==4,"Four active solid parts");
    for(const auto& body:bodies)check(body.summary["valid"].toBool() && body.summary["closed"].toBool()
        && body.summary["volumeMm3"].toDouble()>0,"Each part is a valid closed solid");
    check(hasEye(bodies[0].shape,0,80,4.5) && hasEye(bodies[1].shape,0,80,4.5),"Common Y-axis hinge holes");
    check(hasEye(bodies[0].shape,90,0,4.5) && hasEye(bodies[1].shape,90,180,4.5),"Piston mounts at +X");
    check(hasEye(bodies[0].shape,-90,0,4.5) && hasEye(bodies[1].shape,-90,180,4.5),"Copy mounts at -X");
    check(hasEye(bodies[2].shape,90,0,4.5) && hasEye(bodies[3].shape,90,180,4.5),"Piston end-eye Y axes");
    check(hasPistonWall(bodies[2].shape,4.2) && hasPistonWall(bodies[3].shape,5)
        && hasPistonWall(bodies[3].shape,6),"Coaxial native shaft/sleeve walls and .8 mm clearance");
    for(size_t i=0;i<bodies.size();++i)for(size_t j=i+1;j<bodies.size();++j){
        BRepAlgoAPI_Common common(bodies[i].shape,bodies[j].shape);common.Build();check(common.IsDone(),"Pairwise OCCT Common");
        GProp_GProps properties;BRepGProp::VolumeProperties(common.Shape(),properties);
        if(std::abs(properties.Mass())>1e-6){std::cerr<<"overlap pair "<<i<<","<<j<<" = "<<properties.Mass()<<'\n';check(false,"Four parts intersect");}
    }
    // These independently authored dimensions define a 30 mm telescoping span:
    // rod z8..110, annular sleeve z80..174; radial clearance after source edit is 5-4.2=.8 mm.
    check(std::abs(bodies[2].summary["boundsMm"].toObject()["max"].toArray()[2].toDouble()-110)<1e-4
        && std::abs(bodies[3].summary["boundsMm"].toObject()["min"].toArray()[2].toDouble()-80)<1e-4,
        "Retained piston engagement endpoints");
    std::cout<<"piston four closed solids, shared Y hinge/mount eyes, pairwise zero-overlap, 30mm engagement passed\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
